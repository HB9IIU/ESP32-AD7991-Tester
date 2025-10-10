#include <Wire.h>
#include <Arduino.h>

#define AD7991_ADDR 0x29
#define VREF_VOLTS  2.60f   // VIN3 external reference

void setup() {
  Serial.begin(115200);
  delay(200);

  // Run slow first to be friendly with clock stretching
  Wire.begin(21, 22, 100000);
  Serial.println("AD7991 loopback test: VIN0=2.60V, VIN1=GND, VIN3=2.60V ref");

  // Quick address sanity
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("I2C device: 0x%02X\n", a);
  }
}

bool writeConfig(uint8_t cfg) {
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(cfg);
  return Wire.endTransmission() == 0;
}

bool readSample(uint8_t &ch, uint16_t &raw, uint8_t &msb, uint8_t &lsb) {
  int n = Wire.requestFrom(AD7991_ADDR, 2);
  if (n != 2) return false;
  msb = Wire.read();
  lsb = Wire.read();
  ch  = (msb >> 4) & 0x03;
  raw = ((msb & 0x0F) << 8) | lsb;
  return true;
}

void loop() {
  // Config: VIN0 & VIN1 sequence, REF_SEL=1 (external on VIN3) => 0x38
  if (!writeConfig(0x38)) {
    Serial.println("Config write failed");
    delay(1000);
    return;
  }

  // Give the device a moment; then discard 2 warm-up reads
  delayMicroseconds(10);
  uint8_t ch, msb, lsb; uint16_t raw;
  readSample(ch, raw, msb, lsb);
  readSample(ch, raw, msb, lsb);

  // Now read two samples which should be CH0 then CH1
  for (int i = 0; i < 2; i++) {
    if (readSample(ch, raw, msb, lsb)) {
      float volts = raw * (VREF_VOLTS / 4095.0f);
      Serial.printf("SEQ %d  RAW: msb=0x%02X lsb=0x%02X  CH%d raw=%u  %.4f V\n",
                    i, msb, lsb, ch, raw, volts);
    } else {
      Serial.println("No data");
    }
    delayMicroseconds(10);
  }

  delay(500);
}
