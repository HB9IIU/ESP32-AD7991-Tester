#include <Wire.h>
#include <Arduino.h>


#define AD7991_ADDR 0x29
#define VREF_VOLTS  3.3f

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);
  Serial.println("AD7991 VIN0+VIN1 sequence (VIN3 = 3.3V ref)");
}

void loop() {
  // Config: D7..D4=0011 (VIN0 & VIN1), D3=1 (external ref), D2..D0=000  => 0x38
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(0x38);
  if (Wire.endTransmission() != 0) {
    Serial.println("Config write failed!");
    delay(1000);
    return;
  }

  delayMicroseconds(5);

  // Each 2-byte read returns one conversion; repeat to get both channels in sequence
  for (int i = 0; i < 2; i++) {
    if (Wire.requestFrom(AD7991_ADDR, 2) == 2) {
      uint8_t msb = Wire.read();
      uint8_t lsb = Wire.read();
      uint8_t ch   = (msb >> 4) & 0x03;           
      uint16_t raw = ((msb & 0x0F) << 8) | lsb;
      float volts  = raw * (VREF_VOLTS / 4095.0f);
      Serial.printf("SEQ %d  CH%d = %u (%.4f V)\n", i, ch, raw, volts);
    } else {
      Serial.println("No data from AD7991");
    }
    delayMicroseconds(5);
  }

  delay(500);
}
