#include "rgbww_power_limited_light.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace rgbww_power_limited {

static const char *const TAG = "rgbww_power_limited";

light::LightTraits RGBWWPowerLimitedLight::get_traits() {
  auto traits = light::LightTraits();

  if (this->color_interlock_) {
    traits.set_supported_color_modes({
        light::ColorMode::RGB,
        light::ColorMode::COLD_WARM_WHITE,
    });
  } else {
    traits.set_supported_color_modes({
        light::ColorMode::RGB_COLD_WARM_WHITE,
    });
  }

  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

float RGBWWPowerLimitedLight::compute_scale_(float red, float green, float blue, float cw, float ww) {
  const float weighted_sum = (red * this->weight_red_) + (green * this->weight_green_) + (blue * this->weight_blue_) +
                             (cw * this->weight_cold_white_) + (ww * this->weight_warm_white_);

  const float weight_total = this->weight_red_ + this->weight_green_ + this->weight_blue_ + this->weight_cold_white_ +
                             this->weight_warm_white_;

  const float budget = this->max_power_ * weight_total;

  if (weighted_sum > budget && weighted_sum > 0.0f) {
    return budget / weighted_sum;
  }
  return 1.0f;
}

void RGBWWPowerLimitedLight::write_state(light::LightState *state) {
  if (this->light_state_ == nullptr)
    this->light_state_ = state;

  // If this is NOT a self-triggered call (from update_max_power or
  // clamp_remote_values_), capture the user's requested brightness.
  if (!this->internal_call_) {
    this->requested_brightness_ = state->remote_values.get_brightness();
  }

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

  const float scale = this->compute_scale_(red, green, blue, cw, ww);

  if (scale < 0.999f) {
    red *= scale;
    green *= scale;
    blue *= scale;
    cw *= scale;
    ww *= scale;

    ESP_LOGD(TAG, "Power limited: scale=%.3f", scale);

    if (!state->is_transformer_active() && !this->internal_call_) {
      float new_brightness = this->requested_brightness_ * scale;
      if (new_brightness < 0.01f)
        new_brightness = 0.01f;

      this->internal_call_ = true;
      state->remote_values.set_brightness(new_brightness);
      state->publish_state();
      this->internal_call_ = false;

      ESP_LOGD(TAG, "HA brightness: requested=%.3f clamped=%.3f", this->requested_brightness_, new_brightness);
    }
  }

  this->red_->set_level(red);
  this->green_->set_level(green);
  this->blue_->set_level(blue);
  this->cold_white_->set_level(cw);
  this->warm_white_->set_level(ww);
}

void RGBWWPowerLimitedLight::update_max_power(float value) {
  if (value < 0.01f)
    value = 0.01f;
  if (value > 1.0f)
    value = 1.0f;

  this->max_power_ = value;
  ESP_LOGD(TAG, "Updated max_power to %.3f", this->max_power_);

  if (this->light_state_ != nullptr) {
    // Restore the original requested brightness before re-evaluating,
    // so compute_scale_ works against the true user request.
    this->light_state_->remote_values.set_brightness(this->requested_brightness_);

    this->internal_call_ = true;
    this->write_state(this->light_state_);
    this->internal_call_ = false;

    // Now clamp and publish with the new max_power
    float red, green, blue, cw, ww;
    this->light_state_->current_values_as_rgbww(&red, &green, &blue, &cw, &ww, this->constant_brightness_);
    const float scale = this->compute_scale_(red, green, blue, cw, ww);

    float new_brightness = this->requested_brightness_;
    if (scale < 0.999f) {
      new_brightness = this->requested_brightness_ * scale;
      if (new_brightness < 0.01f)
        new_brightness = 0.01f;
    }

    this->light_state_->remote_values.set_brightness(new_brightness);
    this->light_state_->publish_state();

    ESP_LOGD(TAG, "max_power changed: requested=%.3f displayed=%.3f scale=%.3f", this->requested_brightness_,
             new_brightness, scale);
  }
}

}  // namespace rgbww_power_limited
}  // namespace esphome