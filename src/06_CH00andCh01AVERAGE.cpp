#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t AD_ADDR     = 0x29;       // from your scan
const float   VREF_VOLTS  = 2.6f;       // external ref on VIN3

// CH1 + CH0 + external ref (VIN3). Keep your value:
const uint8_t CFG = 0x38;

// how many samples to average per channel
const uint8_t AVG_N = 16;

bool readSample(uint8_t &chid, uint16_t &raw12) {
  if (Wire.requestFrom(AD_ADDR, (uint8_t)2) != 2) return false;
  uint8_t msb = Wire.read(), lsb = Wire.read();
  uint16_t w  = ((uint16_t)msb << 8) | lsb;
  chid  = (w & 0x3000) >> 12;    // 0..3
  raw12 = (w & 0x0FFF);          // 12-bit
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Wire.beginTransmission(AD_ADDR);
  Wire.write(CFG);
  if (Wire.endTransmission() != 0) {
    Serial.println("AD7991 config failed. Check wiring/address.");
    while (true) delay(1000);
  }
  Serial.println("AD7991: CH1+CH0, ext ref (2.6V).");
  delay(5);
}

void loop() {
  // accumulate AVG_N samples per channel
  uint32_t sum0 = 0, sum1 = 0;
  uint8_t  n0 = 0,  n1 = 0;

  // simple cap to avoid endless loop if reads fail
  for (int tries = 0; tries < 64 && (n0 < AVG_N || n1 < AVG_N); ++tries) {
    uint8_t ch; uint16_t v;
    if (!readSample(ch, v)) continue;
    if (ch == 0 && n0 < AVG_N) { sum0 += v; ++n0; }
    if (ch == 1 && n1 < AVG_N) { sum1 += v; ++n1; }
  }

  if (n0 > 0) {
    uint16_t avg0 = (uint16_t)(sum0 / n0);
    float v0 = (VREF_VOLTS * (float)avg0) / 4096.0f;
    Serial.printf("CH0 avg of %u: %4u -> %.6f V   ", n0, avg0, v0);
  } else {
    Serial.print("CH0: n/a   ");
  }

  if (n1 > 0) {
    uint16_t avg1 = (uint16_t)(sum1 / n1);
    float v1 = (VREF_VOLTS * (float)avg1) / 4096.0f;
    Serial.printf("CH1 avg of %u: %4u -> %.6f V\n", n1, avg1, v1);
  } else {
    Serial.println("CH1: n/a");
  }

  delay(200);
}
