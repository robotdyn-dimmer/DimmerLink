#pragma once

#include "esphome/components/light/light_output.h"
#include "dimmerlink.h"

namespace esphome {
namespace dimmerlink {

class DimmerLinkLight : public light::LightOutput {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }

  void write_state(light::LightState *state) override {
    float brightness;
    state->current_values_as_brightness(&brightness);

    // Convert 0.0-1.0 to 0-100
    uint8_t level = static_cast<uint8_t>(brightness * 100.0f);
    this->parent_->set_level(level);
  }

 protected:
  DimmerLinkHub *parent_{nullptr};
};

}  // namespace dimmerlink
}  // namespace esphome
