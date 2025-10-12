#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21
#define SCL_PIN 22

byte ad7991_addr = 0x00;

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

  Serial.println("AD7991 ready (CH0–CH2 enabled, VIN3 as VREF=3.3V).");
}

void loop() {
  int values[3] = {0, 0, 0};

  // Request conversions for CH0–CH2
  for (int i = 0; i < 3; i++) {
    byte mask = (1 << (7 - i));  // CH0=0x80, CH1=0x40, CH2=0x20

    Wire.beginTransmission(ad7991_addr);
    Wire.write(mask);   // select channel
    Wire.endTransmission();

    delayMicroseconds(10); // wait for conversion (~1µs required)

    Wire.requestFrom(ad7991_addr, (uint8_t)2);
    if (Wire.available() == 2) {
      uint8_t msb = Wire.read();
      uint8_t lsb = Wire.read();
      int channel = (msb >> 4) & 0x03;
      int value   = ((msb & 0x0F) << 8) | lsb;
      if (channel <= 2) values[channel] = value;
    }
  }

  // Print all three values on one line
  Serial.print("CH0=");
  Serial.print(values[0]);
  Serial.print("  CH1=");
  Serial.print(values[1]);
  Serial.print("  CH2=");
  Serial.println(values[2]);

  delay(200);
}
