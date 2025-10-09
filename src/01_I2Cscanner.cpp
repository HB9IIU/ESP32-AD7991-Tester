#include <Wire.h>
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(200);

  // --- I2C CONNECTIONS ---
  // Connect AD7991 (or other I2C device) as follows:
  //   ESP32 GPIO21  → SDA (data)
  //   ESP32 GPIO22  → SCL (clock)
  //   3.3V          → VDD
  //   GND           → GND
  //
  // Note: ESP32 I2C pins are flexible. If you need other pins, change them here:
  Wire.begin(21, 22);   // SDA=21, SCL=22

  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  I2C device found at 0x%02X\n", address);
      nDevices++;
    } else if (error == 4) {
      Serial.printf("  Unknown error at 0x%02X\n", address);
    }
  }

  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Scan complete\n");

  delay(2000);  // wait 2 seconds between scans
}
