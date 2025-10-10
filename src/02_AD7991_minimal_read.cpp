#include <Wire.h>
#include <Arduino.h>

#define AD7991_ADDR 0x29   // AD7991-1, you already confirmed with scanner
#define VREF_VOLTS  2.60f  // VIN3 = external 2.60 V reference

void setup() {
  Serial.begin(115200);
  delay(200);

  // SDA=21, SCL=22, 100 kHz safe speed
  Wire.begin(21, 22, 100000);
  Serial.println("AD7991 minimal test (VIN0+VIN1, VIN3=2.60V ref)");

  // Configure once: VIN0+VIN1 sequence, REF_SEL=external
  Wire.beginTransmission(AD7991_ADDR);
  Wire.write(0x38);   // D7..D4=0011, D3=1 (ext ref), rest=0
  if (Wire.endTransmission() == 0) {
    Serial.println("Config written OK");
  } else {
    Serial.println("Config write failed!");
  }
}

void loop() {
  uint8_t buf[4];
  int n = Wire.requestFrom(AD7991_ADDR, 4);  // get two results (VIN0 + VIN1)
  if (n == 4) {
    buf[0] = Wire.read();
    buf[1] = Wire.read();
    buf[2] = Wire.read();
    buf[3] = Wire.read();

    // Decode first sample
    uint8_t ch0 = (buf[0] >> 4) & 0x03;
    uint16_t raw0 = ((buf[0] & 0x0F) << 8) | buf[1];
    float volts0 = raw0 * (VREF_VOLTS / 4095.0f);

    // Decode second sample
    uint8_t ch1 = (buf[2] >> 4) & 0x03;
    uint16_t raw1 = ((buf[2] & 0x0F) << 8) | buf[3];
    float volts1 = raw1 * (VREF_VOLTS / 4095.0f);

    Serial.printf("CH%d raw=%u  %.4f V | CH%d raw=%u  %.4f V\n",
                  ch0, raw0, volts0, ch1, raw1, volts1);
  } else {
    Serial.println("No data from AD7991");
  }

  delay(500);
}
