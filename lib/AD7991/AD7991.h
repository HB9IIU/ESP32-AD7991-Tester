#pragma once
#include <Arduino.h>
#include <Wire.h>

class AD7991 {
public:
  // Datasheet Table 8: I2C addresses (7-bit)
  static constexpr uint8_t AD7991_ADDR_0 = 0x28; // AD7991-0
  static constexpr uint8_t AD7991_ADDR_1 = 0x29; // AD7991-1
// Channel select (D7..D4). See Table 11 in the datasheet.
enum ChannelSel : uint8_t {
  SEL_NONE       = 0b00000000,
  SEL_VIN0       = 0b00010000,
  SEL_VIN1       = 0b00100000,
  SEL_VIN0_1     = 0b00110000,
  SEL_VIN2       = 0b01000000,
  SEL_VIN0_2     = 0b01010000,
  SEL_VIN1_2     = 0b01100000,
  SEL_VIN0_1_2   = 0b01110000,
  SEL_VIN3       = 0b10000000,
  SEL_VIN0_3     = 0b10010000,
  SEL_VIN1_3     = 0b10100000,
  SEL_VIN0_1_3   = 0b10110000
  // (VIN2_3 and VIN0_1_2_3 are not valid in the table; VIN3 may be VREF)
};

  struct Sample {
    uint8_t channel;   // 0..3 (decoded from result header)
    uint16_t raw12;    // 0..4095 (12-bit)
    float volts;       // scaled by vref
  };

  AD7991(TwoWire& wire = Wire, uint8_t i2c_addr = AD7991_ADDR_0, float vref = 3.300f)
  : _wire(&wire), _addr(i2c_addr), _vref(vref), _config(SEL_VIN0_1_2 | 0b0000) {}

  // Begin I2C and optionally set pins for ESP32
  bool begin(int sda = -1, int scl = -1, uint32_t freq = 400000UL) {
    if (sda >= 0 && scl >= 0) _wire->begin(sda, scl, freq);
    else _wire->begin(); // already configured elsewhere
    // Quick ping
    _wire->beginTransmission(_addr);
    return (_wire->endTransmission() == 0);
  }

  // Set the configuration byte:
  // - chSel: D7..D4 channel selection (see enum)
  // - useExternalRef: D3 = 1 to use VIN3 as VREF (device then acts as 3-ch ADC)
  // - bypassFilter: D2 = 1 to bypass SDA/SCL glitch filters
  // - bitTrialDelay: D1
  // - sampleDelay: D0
  void configure(ChannelSel chSel,
                 bool useExternalRef = false,
                 bool bypassFilter = false,
                 bool bitTrialDelay = false,
                 bool sampleDelay = false) {
    uint8_t cfg = (uint8_t)chSel;
    if (useExternalRef) cfg |= (1u << 3);
    if (bypassFilter)  cfg |= (1u << 2);
    if (bitTrialDelay) cfg |= (1u << 1);
    if (sampleDelay)   cfg |= (1u << 0);
    _config = cfg;
    writeConfig(_config);
  }

  // Change Vref used for volts conversion (VDD or external)
  void setVref(float vref_volts) { _vref = vref_volts; }

  // One-shot read on a single channel (helper): sets selection, reads once.
  bool readSingle(uint8_t channel, Sample& out) {
    if (channel > 3) return false;
    ChannelSel sel = (channel == 0) ? SEL_VIN0 :
                     (channel == 1) ? SEL_VIN1 :
                     (channel == 2) ? SEL_VIN2 : SEL_VIN3;
    configure(sel, (_config & (1u<<3)) != 0, (_config & (1u<<2)) != 0,
                   (_config & (1u<<1)) != 0, (_config & (1u<<0)) != 0);
    delayMicroseconds(2); // guard time > 0.6us wake + acq is handled during read
    return readNext(out);
  }

  // Read using the current configuration. Each call triggers a conversion and returns it.
  bool readNext(Sample& out) {
    // A read (addr+R, 2 bytes) initiates a conversion and returns {status+MSBs, LSBs}
    if (_wire->requestFrom((int)_addr, 2, true) != 2) return false;
    uint8_t b0 = _wire->read();
    uint8_t b1 = _wire->read();

    uint8_t chid = (b0 >> 4) & 0x03;              // two ID bits
    uint16_t raw = ((b0 & 0x0F) << 8) | b1;       // 12-bit straight binary

    out.channel = chid;
    out.raw12 = raw;
    out.volts = (raw / 4095.0f) * _vref;
    return true;
  }

  // Convenience: perform N reads (sequencing among selected channels) into a user buffer
  size_t readBurst(size_t count, Sample* buffer) {
    size_t n = 0;
    while (n < count) {
      if (!readNext(buffer[n])) break;
      n++;
    }
    return n;
  }

  uint8_t i2cAddress() const { return _addr; }
  uint8_t configByte() const { return _config; }

private:
  void writeConfig(uint8_t cfg) {
    _wire->beginTransmission(_addr);
    _wire->write(cfg);
    _wire->endTransmission(true);
  }

  TwoWire* _wire;
  uint8_t _addr;
  float _vref;
  uint8_t _config;
};
