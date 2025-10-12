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
      if (address >= 0x28 && address <= 0x2B) {
        ad7991_addr = address;
        Serial.print("AD7991 found at 0x");
        Serial.println(ad7991_addr, HEX);
      }
    }
  }

  if (ad7991_addr == 0x00) {
    Serial.println("No AD7991 found! Stopping.");
    while (1);
  }

  // --- Configure AD7991: enable CH0, CH1, CH2 only (VIN3 is VREF) ---
  Wire.beginTransmission(ad7991_addr);
  Wire.write(0xE0);  // 1110 xxxx -> enable CH0, CH1, CH2
  Wire.endTransmission();
  Serial.println("AD7991 configured: CH0–CH2 enabled, VDD reference = VIN3 = 3.3V");
}

void loop() {
  Wire.requestFrom(ad7991_addr, (uint8_t)2);
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    int channel = (msb >> 4) & 0x03;   // channel ID (0–2)
    int value   = ((msb & 0x0F) << 8) | lsb; // 12-bit ADC value (0–4095)

    if (channel <= 2) {  // only print valid inputs
      Serial.print("CH");
      Serial.print(channel);
      Serial.print(": ");
      Serial.println(value);
    }
  }

  delay(100);
}
