#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "dimmerlink.h"

namespace esphome {
namespace dimmerlink {

class DimmerLinkBinarySensor : public PollingComponent {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

  void set_ready_sensor(binary_sensor::BinarySensor *sens) { this->ready_sensor_ = sens; }
  void set_error_sensor(binary_sensor::BinarySensor *sens) { this->error_sensor_ = sens; }
  void set_calibration_done_sensor(binary_sensor::BinarySensor *sens) { this->calibration_done_sensor_ = sens; }

  void update() override {
    if (this->ready_sensor_ != nullptr) {
      this->ready_sensor_->publish_state(this->parent_->is_ready());
    }

    if (this->error_sensor_ != nullptr) {
      this->error_sensor_->publish_state(this->parent_->has_error());
    }

    if (this->calibration_done_sensor_ != nullptr) {
      this->calibration_done_sensor_->publish_state(this->parent_->is_calibration_done());
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG("dimmerlink.binary_sensor", "DimmerLink Binary Sensors:");
    if (this->ready_sensor_)
      ESP_LOGCONFIG("dimmerlink.binary_sensor", "  Ready: %s", this->ready_sensor_->get_name().c_str());
    if (this->error_sensor_)
      ESP_LOGCONFIG("dimmerlink.binary_sensor", "  Error: %s", this->error_sensor_->get_name().c_str());
    if (this->calibration_done_sensor_)
      ESP_LOGCONFIG("dimmerlink.binary_sensor", "  Calibration Done: %s", this->calibration_done_sensor_->get_name().c_str());
  }

 protected:
  DimmerLinkHub *parent_{nullptr};
  binary_sensor::BinarySensor *ready_sensor_{nullptr};
  binary_sensor::BinarySensor *error_sensor_{nullptr};
  binary_sensor::BinarySensor *calibration_done_sensor_{nullptr};
};

}  // namespace dimmerlink
}  // namespace esphome
