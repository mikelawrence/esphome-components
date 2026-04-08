#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace rgbww_power_limited {

class RGBWWPowerLimitedLight : public light::LightOutput, public Component {
 public:
  void set_red(output::FloatOutput *out) { this->red_ = out; }
  void set_green(output::FloatOutput *out) { this->green_ = out; }
  void set_blue(output::FloatOutput *out) { this->blue_ = out; }
  void set_cold_white(output::FloatOutput *out) { this->cold_white_ = out; }
  void set_warm_white(output::FloatOutput *out) { this->warm_white_ = out; }
  void set_cold_white_temperature(float mireds) { this->cold_white_temperature_ = mireds; }
  void set_warm_white_temperature(float mireds) { this->warm_white_temperature_ = mireds; }
  void set_constant_brightness(bool v) { this->constant_brightness_ = v; }
  void set_color_interlock(bool v) { this->color_interlock_ = v; }
  void set_max_power(float v) { this->max_power_ = v; }
  void set_weight_red(float v) { this->weight_red_ = v; }
  void set_weight_green(float v) { this->weight_green_ = v; }
  void set_weight_blue(float v) { this->weight_blue_ = v; }
  void set_weight_cold_white(float v) { this->weight_cold_white_ = v; }
  void set_weight_warm_white(float v) { this->weight_warm_white_ = v; }
  void set_light_state(light::LightState *state) { this->light_state_ = state; }

  float get_max_power() const { return this->max_power_; }
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;
  void update_max_power(float value);

 protected:
  output::FloatOutput *red_{nullptr};
  output::FloatOutput *green_{nullptr};
  output::FloatOutput *blue_{nullptr};
  output::FloatOutput *cold_white_{nullptr};
  output::FloatOutput *warm_white_{nullptr};
  light::LightState *light_state_{nullptr};
  float cold_white_temperature_{0.0f};
  float warm_white_temperature_{0.0f};
  bool constant_brightness_{false};
  bool color_interlock_{false};
  float max_power_{1.0f};
  float weight_red_{1.0f};
  float weight_green_{1.0f};
  float weight_blue_{1.0f};
  float weight_cold_white_{1.0f};
  float weight_warm_white_{1.0f};
  bool adjusting_{false};
};

}  // namespace rgbww_power_limited
}  // namespace esphome