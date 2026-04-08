#include "rgbww_power_limited_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rgbww_power_limited {

static const char *const TAG = "rgbww_power_limited";

light::LightTraits RGBWWPowerLimitedLight::get_traits() {
  auto traits = light::LightTraits();
  if (this->color_interlock_) {
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLD_WARM_WHITE});
  } else {
    traits.set_supported_color_modes({light::ColorMode::RGB_COLD_WARM_WHITE});
  }
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

void RGBWWPowerLimitedLight::write_state(light::LightState *state) {
  if (this->adjusting_)
    return;

  float red, green, blue, cw, ww;
  state->current_values_as_rgbww(&red, &green, &blue, &cw, &ww, this->constant_brightness_);

  if (this->color_interlock_) {
    if (state->current_values.get_color_mode() & light::ColorCapability::RGB) {
      cw = 0.0f;
      ww = 0.0f;
    } else {
      red = 0.0f;
      green = 0.0f;
      blue = 0.0f;
    }
  }

  float weighted_sum = (red * this->weight_red_) + (green * this->weight_green_) + (blue * this->weight_blue_) +
                       (cw * this->weight_cold_white_) + (ww * this->weight_warm_white_);

  float weight_total = this->weight_red_ + this->weight_green_ + this->weight_blue_ + this->weight_cold_white_ +
                       this->weight_warm_white_;

  float budget = this->max_power_ * weight_total;
  float scale = 1.0f;

  if (weighted_sum > budget && weighted_sum > 0.0f) {
    scale = budget / weighted_sum;
    red *= scale;
    green *= scale;
    blue *= scale;
    cw *= scale;
    ww *= scale;
    ESP_LOGD(TAG, "Power limited: scale=%.3f, budget=%.2f, demand=%.2f", scale, budget, weighted_sum);
  }

  this->red_->set_level(red);
  this->green_->set_level(green);
  this->blue_->set_level(blue);
  this->cold_white_->set_level(cw);
  this->warm_white_->set_level(ww);

  if (scale < 1.0f) {
    this->adjusting_ = true;
    auto call = state->make_call();
    call.set_brightness(state->current_values.get_brightness() * scale);
    call.set_publish(true);
    call.set_save(true);
    call.set_transition_length(0);
    call.perform();
    this->adjusting_ = false;
  }
}

}  // namespace rgbww_power_limited
}  // namespace esphome