#include <Wire.h>
#include <Arduino.h>

#define AD7991_ADDR_0 0x28  // AD7991-0
#define AD7991_ADDR_1 0x29  // AD7991-1

// Select which one you expect
#define AD7991_ADDR AD7991_ADDR_0  

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(21, 22, 100000);  // SDA=21, SCL=22, 100 kHz (adjust pins if needed)
  Serial.println("Scanning I2C...");

  // Simple I2C scan
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", addr);
    }
  }

  // Configure AD7991 to sample channel 0, internal reference (VDD)
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(0x10); // Config: D7..D4 = 0001 (VIN0), D3=0 (VDD ref), rest=0
  if (Wire.endTransmission() == 0) {
    Serial.printf("AD7991 config sent to 0x%02X\n", AD7991_ADDR);
  } else {
    Serial.printf("No response from AD7991 at 0x%02X\n", AD7991_ADDR);
  }
}

void loop() {
  // Request 2 bytes (1 sample)
  Wire.requestFrom(AD7991_ADDR, 2);
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    uint8_t channel = (msb >> 4) & 0x03;         // channel ID
    uint16_t value = ((msb & 0x0F) << 8) | lsb;  // 12-bit result

    float volts = value * (3.3 / 4095.0);        // scale to VDD=3.3 V

    Serial.printf("CH%d = %u (%.4f V)\n", channel, value, volts);
  } else {
    Serial.println("No data from AD7991");
  }

  delay(500);
}
