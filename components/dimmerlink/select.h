#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "dimmerlink.h"

namespace esphome {
namespace dimmerlink {

class DimmerLinkCurveSelect : public select::Select, public Component {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

  void setup() override {
    // Read current curve from device and publish initial state
    DimmingCurve curve = this->parent_->get_curve();
    switch (curve) {
      case DimmingCurve::LINEAR:
        this->publish_state("LINEAR");
        break;
      case DimmingCurve::RMS:
        this->publish_state("RMS");
        break;
      case DimmingCurve::LOG:
        this->publish_state("LOG");
        break;
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG("dimmerlink.select", "DimmerLink Curve Select: %s", this->get_name().c_str());
  }

  float get_setup_priority() const override { return setup_priority::DATA - 1; }

 protected:
  void control(const std::string &value) override {
    DimmingCurve curve;
    if (value == "LINEAR") {
      curve = DimmingCurve::LINEAR;
    } else if (value == "RMS") {
      curve = DimmingCurve::RMS;
    } else if (value == "LOG") {
      curve = DimmingCurve::LOG;
    } else {
      ESP_LOGW("dimmerlink.select", "Unknown curve value: %s", value.c_str());
      return;
    }

    if (this->parent_->set_curve(curve)) {
      this->publish_state(value);
    }
  }

  DimmerLinkHub *parent_{nullptr};
};

}  // namespace dimmerlink
}  // namespace esphome
