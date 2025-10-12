#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t AD_ADDR   = 0x29;     
const float   VREF_VOLTS = 2.6f;    // external reference (VIN3)

// Config:
// D7..D4 = CH3 CH2 CH1 CH0
// CH1 + CH0 -> 0011 xxxx = 0x30
// + REF_SEL (external ref) bit D3 -> 0x38
const uint8_t CFG = 0x38;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Select CH1 + CH0, external ref (VIN3) if CFG=0x38
  Wire.beginTransmission(AD_ADDR);
  Wire.write(CFG);
  if (Wire.endTransmission() != 0) {
    Serial.println("AD7991 config failed. Check wiring/address.");
    while (true) delay(1000);
  }

  Serial.println("AD7991: CH1+CH0 enabled.");
  delay(2000); 
}

bool readSample(uint8_t &chid, uint16_t &raw12) {
  if (Wire.requestFrom(AD_ADDR, (uint8_t)2) != 2) return false;
  uint8_t msb = Wire.read(), lsb = Wire.read();
  uint16_t w  = ((uint16_t)msb << 8) | lsb;
  chid  = (w & 0x3000) >> 12;   // tag: 0..3
  raw12 = (w & 0x0FFF);         // 12-bit value
  return true;
}

void loop() {
  uint16_t val0 = 0, val1 = 0;
  bool have0 = false, have1 = false;

  // Keep reading until we’ve seen both CH0 and CH1 once
  for (int tries = 0; tries < 4 && !(have0 && have1); ++tries) {
    uint8_t ch; uint16_t v;
    if (readSample(ch, v)) {
      if (ch == 0) { val0 = v; have0 = true; }
      if (ch == 1) { val1 = v; have1 = true; }
    } else {
      Serial.println("Read failed");
      break;
    }
  }

  if (have0 || have1) {
    if (have0) {
      float v0 = (VREF_VOLTS * (float)val0) / 4096.0f;
      Serial.printf("CH0: %4u /4095 -> %.6f V  ", val0, v0);
    } else {
      Serial.print("CH0: n/a  ");
    }

    if (have1) {
      float v1 = (VREF_VOLTS * (float)val1) / 4096.0f;
      Serial.printf("CH1: %4u /4095 -> %.6f V\n", val1, v1);
    } else {
      Serial.println("CH1: n/a");
    }
  }

  delay(300);
}
