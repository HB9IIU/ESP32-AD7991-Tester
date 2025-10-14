#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t AD7991_ADDR = 0x29;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(0x70);                 // CH0|CH1|CH2 enabled, REF_SEL=0 (VDD ref)
  Wire.endTransmission();
}

void loop() {
  if (Wire.requestFrom(AD7991_ADDR, (uint8_t)2) == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint16_t w = ((uint16_t)msb << 8) | lsb;
    uint8_t chid = (w & 0x3000) >> 12;   // CHID from bits [13:12]
    uint16_t val = (w & 0x0FFF);         // 12-bit value
    Serial.print("CH"); Serial.print(chid);
    Serial.print(" VALUE="); Serial.println(val);
  }
  delay(100);
}
