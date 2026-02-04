#pragma once

#include "esphome/core/log.h"
#include "esphome/components/button/button.h"
#include "dimmerlink.h"

namespace esphome {
namespace dimmerlink {

class DimmerLinkResetButton : public button::Button {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

 protected:
  void press_action() override {
    ESP_LOGI("dimmerlink.button", "Sending reset command");
    this->parent_->send_command(CMD_RESET);
  }

  DimmerLinkHub *parent_{nullptr};
};

class DimmerLinkRecalibrateButton : public button::Button {
 public:
  void set_parent(DimmerLinkHub *parent) { this->parent_ = parent; }

 protected:
  void press_action() override {
    ESP_LOGI("dimmerlink.button", "Sending recalibrate command");
    this->parent_->send_command(CMD_RECALIBRATE);
  }

  DimmerLinkHub *parent_{nullptr};
};

}  // namespace dimmerlink
}  // namespace esphome
