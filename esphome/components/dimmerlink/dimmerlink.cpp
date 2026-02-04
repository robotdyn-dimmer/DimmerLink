#include "dimmerlink.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace dimmerlink {

static const char *const TAG = "dimmerlink";
static const uint32_t STARTUP_DELAY_MS = 2000;
static const uint32_t STATUS_UPDATE_INTERVAL_MS = 1000;

void DimmerLinkHub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DimmerLink Hub...");
  this->startup_time_ = millis();
}

void DimmerLinkHub::loop() {
  // Wait for startup delay (device calibration)
  if (!this->initialized_) {
    if (millis() - this->startup_time_ < STARTUP_DELAY_MS) {
      return;
    }

    // Try to read firmware version to verify communication
    uint8_t version;
    if (!this->read_register(REG_VERSION, &version, 1)) {
      ESP_LOGE(TAG, "Failed to communicate with DimmerLink device");
      this->mark_failed();
      return;
    }

    this->cached_version_ = version;
    this->initialized_ = true;
    ESP_LOGI(TAG, "DimmerLink initialized, firmware version: %d", version);

    // Read initial state
    this->get_level();
    this->get_curve();
    this->get_ac_frequency();
  }

  // Periodically update status cache
  if (millis() - this->last_status_update_ >= STATUS_UPDATE_INTERVAL_MS) {
    this->update_status_cache_();
    this->last_status_update_ = millis();
  }
}

void DimmerLinkHub::dump_config() {
  ESP_LOGCONFIG(TAG, "DimmerLink Hub:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed!");
  } else {
    ESP_LOGCONFIG(TAG, "  Firmware Version: %d", this->cached_version_);
    ESP_LOGCONFIG(TAG, "  AC Frequency: %d Hz", this->cached_ac_freq_);
  }
}

bool DimmerLinkHub::read_register(uint8_t reg, uint8_t *data, size_t len) {
  return this->read_bytes(reg, data, len);
}

bool DimmerLinkHub::write_register(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

bool DimmerLinkHub::set_level(uint8_t level) {
  if (level > 100)
    level = 100;
  if (this->write_register(REG_DIM0_LEVEL, level)) {
    this->cached_level_ = level;
    ESP_LOGD(TAG, "Set level to %d%%", level);
    return true;
  }
  ESP_LOGW(TAG, "Failed to set level");
  return false;
}

uint8_t DimmerLinkHub::get_level() {
  uint8_t level;
  if (this->read_register(REG_DIM0_LEVEL, &level, 1)) {
    this->cached_level_ = level;
  }
  return this->cached_level_;
}

bool DimmerLinkHub::set_curve(DimmingCurve curve) {
  uint8_t curve_val = static_cast<uint8_t>(curve);
  if (this->write_register(REG_DIM0_CURVE, curve_val)) {
    this->cached_curve_ = curve_val;
    ESP_LOGD(TAG, "Set curve to %d", curve_val);
    return true;
  }
  ESP_LOGW(TAG, "Failed to set curve");
  return false;
}

DimmingCurve DimmerLinkHub::get_curve() {
  uint8_t curve;
  if (this->read_register(REG_DIM0_CURVE, &curve, 1)) {
    this->cached_curve_ = curve;
  }
  return static_cast<DimmingCurve>(this->cached_curve_);
}

bool DimmerLinkHub::set_fade_time(uint8_t time_100ms) {
  return this->write_register(REG_DIM0_FADE_TIME, time_100ms);
}

uint8_t DimmerLinkHub::get_fade_time() {
  uint8_t time;
  if (this->read_register(REG_DIM0_FADE_TIME, &time, 1)) {
    return time;
  }
  return 0;
}

bool DimmerLinkHub::send_command(uint8_t cmd) {
  ESP_LOGD(TAG, "Sending command: 0x%02X", cmd);
  return this->write_register(REG_COMMAND, cmd);
}

bool DimmerLinkHub::is_ready() {
  return (this->cached_status_ & STATUS_READY) != 0;
}

bool DimmerLinkHub::has_error() {
  return (this->cached_status_ & STATUS_ERROR) != 0;
}

uint8_t DimmerLinkHub::get_error_code() {
  uint8_t error;
  if (this->read_register(REG_ERROR, &error, 1)) {
    this->cached_error_ = error;
  }
  return this->cached_error_;
}

uint8_t DimmerLinkHub::get_firmware_version() {
  return this->cached_version_;
}

uint8_t DimmerLinkHub::get_ac_frequency() {
  uint8_t freq;
  if (this->read_register(REG_AC_FREQ, &freq, 1)) {
    this->cached_ac_freq_ = freq;
  }
  return this->cached_ac_freq_;
}

uint16_t DimmerLinkHub::get_ac_period() {
  uint8_t data[2];
  if (this->read_register(REG_AC_PERIOD_L, data, 2)) {
    this->cached_ac_period_ = data[0] | (data[1] << 8);  // Little-endian
  }
  return this->cached_ac_period_;
}

bool DimmerLinkHub::is_calibration_done() {
  uint8_t cal;
  if (this->read_register(REG_CALIBRATION, &cal, 1)) {
    this->cached_calibration_ = (cal == 1);
  }
  return this->cached_calibration_;
}

void DimmerLinkHub::update_status_cache_() {
  uint8_t status;
  if (this->read_register(REG_STATUS, &status, 1)) {
    this->cached_status_ = status;
  }
}

}  // namespace dimmerlink
}  // namespace esphome
