#include <Arduino.h>
#include <Wire.h>
#include "AD7991.h"

// ---- Adjust to your wiring ----
static constexpr int PIN_SDA = 21;   
static constexpr int PIN_SCL = 22;  
static constexpr uint8_t ADC_ADDR = AD7991::AD7991_ADDR_0; // 0x28 or 0x29
static constexpr float VREF_VOLTS = 2.600f; 

AD7991 adc(Wire, ADC_ADDR, VREF_VOLTS);

void i2cScan() {
  Serial.println(F("\nI2C scan..."));
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  - Found device at 0x%02X\n", addr);
      delay(2);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!adc.begin(PIN_SDA, PIN_SCL, 400000)) { // 400kHz supported; fall back to 100k if needed
    Serial.println(F("AD7991 not ACKing at configured address. Running I2C scan..."));
    i2cScan();
  } else {
    Serial.printf("AD7991 online at 0x%02X\n", adc.i2cAddress());
  }


}

void loop() {
  AD7991::Sample s;

  for (uint8_t ch = 0; ch < 4; ch++) {
    // If VIN3 is used as external VREF on your board, skip ch==3
    if (!adc.readSingle(ch, s)) {
      Serial.printf("Read failed on CH%d\n", ch);
      continue;
    }
    Serial.printf("CH%d: raw=%4u  volts=%.4f V\n", s.channel, s.raw12, s.volts);
    delay(5); // tiny pause between reads
  }

  // Example: configure a sequence among VIN0, VIN1, VIN2 and read 6 samples
  adc.configure(AD7991::SEL_VIN0_1_2, /*useExternalRef=*/false);
  AD7991::Sample buf[6];
  size_t got = adc.readBurst(6, buf);
  for (size_t i = 0; i < got; i++) {
    Serial.printf("SEQ #%u  CH%d: %4u  %.4f V\n",
                  (unsigned)i, buf[i].channel, buf[i].raw12, buf[i].volts);
  }

  delay(1000);
}
