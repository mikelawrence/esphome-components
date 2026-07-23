#pragma once

#include "esphome/core/automation.h"
#include "rgbww_power_limited_light.h"

namespace esphome {
namespace rgbww_power_limited {

template<typename... Ts> class SetMaxPowerAction : public Action<Ts...> {
 public:
  explicit SetMaxPowerAction(RGBWWPowerLimitedLight *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(float, max_power)

  void play(const Ts &...x) override { this->parent_->update_max_power(this->max_power_.value(x...)); }

 protected:
  RGBWWPowerLimitedLight *parent_;
};

}  // namespace rgbww_power_limited
}  // namespace esphome