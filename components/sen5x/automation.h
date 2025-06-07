#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "sen5x.h"

namespace esphome {
namespace sen5x {


template<typename... Ts> class SetAmbientPressurehPa : public Action<Ts...>, public Parented<SEN5XComponent> {
 public:
  void play(Ts... x) override {
    if (this->value_.has_value()) {
      this->parent_->set_ambient_pressure_compensation(this->value_.value(x...));
    }
  }

 protected:
  TEMPLATABLE_VALUE(uint16_t, value)
};

template<typename... Ts> class PerformForcedCo2CalibrationAction : public Action<Ts...>, public Parented<SEN5XComponent> {
 public:
  void play(Ts... x) override {
    if (this->value_.has_value()) {
      this->parent_->perform_forced_co2_calibration(this->value_.value(x...));
    }
  }

 protected:
  TEMPLATABLE_VALUE(uint16_t, value)
};

template<typename... Ts> class StartFanAction : public Action<Ts...> {
 public:
  explicit StartFanAction(SEN5XComponent *sen5x) : sen5x_(sen5x) {}

  void play(Ts... x) override { this->sen5x_->start_fan_cleaning(); }

 protected:
  SEN5XComponent *sen5x_;
};

template<typename... Ts> class ActivateHeaterAction : public Action<Ts...> {
 public:
  explicit ActivateHeaterAction(SEN5XComponent *sen5x) : sen5x_(sen5x) {}

  void play(Ts... x) override { this->sen5x_->activate_heater(); }

 protected:
  SEN5XComponent *sen5x_;
};

}  // namespace sen5x
}  // namespace esphome