#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace dimmerlink {

// Register addresses
static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_COMMAND = 0x01;
static const uint8_t REG_ERROR = 0x02;
static const uint8_t REG_VERSION = 0x03;
static const uint8_t REG_DIM0_LEVEL = 0x10;
static const uint8_t REG_DIM0_CURVE = 0x11;
static const uint8_t REG_DIM0_FADE_TIME = 0x18;
static const uint8_t REG_AC_FREQ = 0x20;
static const uint8_t REG_AC_PERIOD_L = 0x21;
static const uint8_t REG_AC_PERIOD_H = 0x22;
static const uint8_t REG_CALIBRATION = 0x23;
static const uint8_t REG_I2C_ADDRESS = 0x30;

// Commands
static const uint8_t CMD_NOP = 0x00;
static const uint8_t CMD_RESET = 0x01;
static const uint8_t CMD_RECALIBRATE = 0x02;
static const uint8_t CMD_SWITCH_UART = 0x03;

// Status bits
static const uint8_t STATUS_READY = 0x01;
static const uint8_t STATUS_ERROR = 0x02;

// Error codes
static const uint8_t ERR_OK = 0x00;
static const uint8_t ERR_SYNTAX = 0xF9;
static const uint8_t ERR_NOT_READY = 0xFC;
static const uint8_t ERR_INDEX = 0xFD;
static const uint8_t ERR_PARAM = 0xFE;

// Dimming curves
enum class DimmingCurve : uint8_t {
  LINEAR = 0,
  RMS = 1,
  LOG = 2,
};

class DimmerLinkHub : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // I2C register access methods
  bool read_register(uint8_t reg, uint8_t *data, size_t len);
  bool write_register(uint8_t reg, uint8_t value);

  // Device control methods
  bool set_level(uint8_t level);
  uint8_t get_level();
  bool set_curve(DimmingCurve curve);
  DimmingCurve get_curve();
  bool set_fade_time(uint8_t time_100ms);
  uint8_t get_fade_time();
  bool send_command(uint8_t cmd);

  // Status methods
  bool is_ready();
  bool has_error();
  uint8_t get_error_code();
  uint8_t get_firmware_version();
  uint8_t get_ac_frequency();
  uint16_t get_ac_period();
  bool is_calibration_done();

 protected:
  bool initialized_{false};
  uint32_t startup_time_{0};

  // Cached state
  uint8_t cached_status_{0};
  uint8_t cached_level_{0};
  uint8_t cached_curve_{0};
  uint8_t cached_error_{0};
  uint8_t cached_version_{0};
  uint8_t cached_ac_freq_{0};
  uint16_t cached_ac_period_{0};
  bool cached_calibration_{false};

  uint32_t last_status_update_{0};
  void update_status_cache_();
};

}  // namespace dimmerlink
}  // namespace esphome
