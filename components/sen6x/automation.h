#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "sen6x.h"

namespace esphome {
namespace sen6x {

template<typename... Ts> class SetAmbientPressurehPa : public Action<Ts...>, public Parented<Sen6xComponent> {
 public:
  void play(const Ts &...x) override {
    auto value = this->value_.value(x...);
    this->parent_->action_set_ambient_pressure_compensation(value);
  }

 protected:
  TEMPLATABLE_VALUE(uint16_t, value)
};

template<typename... Ts>
class PerformForcedCo2CalibrationAction : public Action<Ts...>, public Parented<Sen6xComponent> {
 public:
  void play(const Ts &...x) override {
    auto value = this->value_.value(x...);
    this->parent_->action_perform_forced_co2_calibration(value);
  }

 protected:
  TEMPLATABLE_VALUE(uint16_t, value)
};

template<typename... Ts> class StartFanAction : public Action<Ts...>, public Parented<Sen6xComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->action_start_fan_cleaning(); }
};

template<typename... Ts> class ActivateHeaterAction : public Action<Ts...>, public Parented<Sen6xComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->action_activate_heater(); }
};

}  // namespace sen6x
}  // namespace esphome
