#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "dimmerlink.h"

namespace esphome {
namespace dimmerlink {

class DimmerLinkSensor : public PollingComponent {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

  void set_ac_frequency_sensor(sensor::Sensor *sens) { this->ac_frequency_sensor_ = sens; }
  void set_level_sensor(sensor::Sensor *sens) { this->level_sensor_ = sens; }
  void set_firmware_version_sensor(sensor::Sensor *sens) { this->firmware_version_sensor_ = sens; }
  void set_ac_period_sensor(sensor::Sensor *sens) { this->ac_period_sensor_ = sens; }

  void update() override {
    if (this->ac_frequency_sensor_ != nullptr) {
      uint8_t freq = this->parent_->get_ac_frequency();
      if (freq == 50 || freq == 60) {
        this->ac_frequency_sensor_->publish_state(freq);
      }
    }

    if (this->level_sensor_ != nullptr) {
      this->level_sensor_->publish_state(this->parent_->get_level());
    }

    if (this->firmware_version_sensor_ != nullptr) {
      this->firmware_version_sensor_->publish_state(this->parent_->get_firmware_version());
    }

    if (this->ac_period_sensor_ != nullptr) {
      this->ac_period_sensor_->publish_state(this->parent_->get_ac_period());
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG("dimmerlink.sensor", "DimmerLink Sensors:");
    if (this->ac_frequency_sensor_)
      ESP_LOGCONFIG("dimmerlink.sensor", "  AC Frequency: %s", this->ac_frequency_sensor_->get_name().c_str());
    if (this->level_sensor_)
      ESP_LOGCONFIG("dimmerlink.sensor", "  Level: %s", this->level_sensor_->get_name().c_str());
    if (this->firmware_version_sensor_)
      ESP_LOGCONFIG("dimmerlink.sensor", "  Firmware Version: %s", this->firmware_version_sensor_->get_name().c_str());
    if (this->ac_period_sensor_)
      ESP_LOGCONFIG("dimmerlink.sensor", "  AC Period: %s", this->ac_period_sensor_->get_name().c_str());
  }

 protected:
  DimmerLinkHub *parent_{nullptr};
  sensor::Sensor *ac_frequency_sensor_{nullptr};
  sensor::Sensor *level_sensor_{nullptr};
  sensor::Sensor *firmware_version_sensor_{nullptr};
  sensor::Sensor *ac_period_sensor_{nullptr};
};

}  // namespace dimmerlink
}  // namespace esphome
