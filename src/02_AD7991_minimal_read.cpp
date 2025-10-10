#include <Wire.h>
#include <Arduino.h>

#define AD7991_ADDR 0x29
#define VREF_VOLTS  3.3f

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);
  Serial.println("AD7991 VIN0+VIN1 seq (VIN3 = 3.3V ref)");
}

void loop() {
  // Config: VIN0 & VIN1, external ref on VIN3 => 0x38
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(0x38);
  if (Wire.endTransmission() != 0) {
    Serial.println("Config write failed!");
    delay(1000);
    return;
  }

  delayMicroseconds(5);

  // Discard first sample after config (common quirk)
  Wire.requestFrom(AD7991_ADDR, 2);
  if (Wire.available()==2) { Wire.read(); Wire.read(); }

  // Read two conversions (should alternate channels)
  for (int i = 0; i < 2; i++) {
    int n = Wire.requestFrom(AD7991_ADDR, 2);
    if (n == 2) {
      uint8_t msb = Wire.read();
      uint8_t lsb = Wire.read();
      uint8_t ch   = (msb >> 4) & 0x03;
      uint16_t raw = ((msb & 0x0F) << 8) | lsb;
      float volts  = raw * (VREF_VOLTS / 4095.0f);
      Serial.printf("SEQ %d  RAW: msb=0x%02X lsb=0x%02X  CH%d raw=%u  %.4f V\n",
                    i, msb, lsb, ch, raw, volts);
    } else {
      Serial.printf("No data (got %d bytes)\n", n);
    }
    delayMicroseconds(5);
  }

  delay(500);
}
