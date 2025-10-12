#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21   // ESP32 default
#define SCL_PIN 22   // ESP32 default

byte ad7991_addr = 0x00; // Will be detected by I2C scanner

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // --- Scan I2C bus ---
  Serial.println("Scanning I2C bus...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);

      if (address >= 0x28 && address <= 0x2B) {
        ad7991_addr = address;
        Serial.print(" -> AD7991 detected at 0x");
        Serial.println(ad7991_addr, HEX);
      }
    }
  }

  if (ad7991_addr == 0x00) {
    Serial.println("No AD7991 found! Stopping.");
    while (1);
  }
}

void loop() {
  // The AD7991 cycles through enabled channels automatically.
  // Default after power-up: all 4 channels active.
  // Each read gives channel ID + 12-bit result.

  Wire.requestFrom(ad7991_addr, (uint8_t)2);
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    int channel = (msb >> 4) & 0x03;   // bits [5:4] = channel number
    int value   = ((msb & 0x0F) << 8) | lsb; // 12-bit ADC value

    Serial.print("CH");
    Serial.print(channel);
    Serial.print(": ");
    Serial.println(value);
  }

  delay(100); // adjust speed as needed
}
