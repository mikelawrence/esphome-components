#include "fan_pwm_output.h"

#include "esphome/core/log.h"

namespace esphome {
namespace fan_pwm {

static const char *const TAG = "fan_pwm";

void FanPWMOutput::setup() {
  this->resolution_hz_ = 10000000;
  this->period_ticks_ = this->resolution_hz_ / this->frequency_;

  if (this->period_ticks_ < 2) {
    ESP_LOGE(TAG, "Frequency too high for 10MHz resolution");
    this->mark_failed();
    return;
  }

  if (this->period_ticks_ > 32767) {
    ESP_LOGE(TAG, "Frequency too low: period exceeds 15-bit RMT symbol limit");
    this->mark_failed();
    return;
  }

  rmt_tx_channel_config_t tx_config = {};
  tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
  tx_config.gpio_num = static_cast<gpio_num_t>(this->pin_);
  tx_config.mem_block_symbols = 48;
  tx_config.resolution_hz = this->resolution_hz_;
  tx_config.trans_queue_depth = 1;
  tx_config.flags.invert_out = false;
  tx_config.flags.with_dma = false;

  esp_err_t err = rmt_new_tx_channel(&tx_config, &this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT TX channel creation failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  rmt_copy_encoder_config_t copy_config = {};
  err = rmt_new_copy_encoder(&copy_config, &this->encoder_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT copy encoder creation failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  this->build_symbol_(0.0f);

  if (!this->start_output_()) {
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "RMT Fan PWM configured: pin=GPIO%d freq=%luHz period=%lu ticks resolution=%luHz", this->pin_,
           (unsigned long) this->frequency_, (unsigned long) this->period_ticks_, (unsigned long) this->resolution_hz_);
}

void FanPWMOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "RMT Fan PWM Output:");
  ESP_LOGCONFIG(TAG, "  Pin: GPIO%d", this->pin_);
  ESP_LOGCONFIG(TAG, "  Frequency: %lu Hz", (unsigned long) this->frequency_);
  ESP_LOGCONFIG(TAG, "  Resolution: %lu Hz", (unsigned long) this->resolution_hz_);
  ESP_LOGCONFIG(TAG, "  Period: %lu ticks", (unsigned long) this->period_ticks_);
  ESP_LOGCONFIG(TAG, "  Last state: %.3f", this->last_state_);
}

void FanPWMOutput::write_state(float state) {
  if (state < 0.0f) {
    state = 0.0f;
  } else if (state > 1.0f) {
    state = 1.0f;
  }

  this->last_state_ = state;
  this->build_symbol_(state);

  if (!this->restart_output_()) {
    ESP_LOGE(TAG, "Failed to restart RMT output for duty update");
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
}

void FanPWMOutput::build_symbol_(float state) {
  uint32_t high_ticks = static_cast<uint32_t>(state * this->period_ticks_);
  uint32_t low_ticks = this->period_ticks_ - high_ticks;

  if (high_ticks == 0) {
    this->pwm_symbol_.duration0 = this->period_ticks_;
    this->pwm_symbol_.level0 = 0;
    this->pwm_symbol_.duration1 = 0;
    this->pwm_symbol_.level1 = 0;
    return;
  }

  if (low_ticks == 0) {
    this->pwm_symbol_.duration0 = this->period_ticks_;
    this->pwm_symbol_.level0 = 1;
    this->pwm_symbol_.duration1 = 0;
    this->pwm_symbol_.level1 = 0;
    return;
  }

  this->pwm_symbol_.duration0 = high_ticks;
  this->pwm_symbol_.level0 = 1;
  this->pwm_symbol_.duration1 = low_ticks;
  this->pwm_symbol_.level1 = 0;
}

bool FanPWMOutput::start_output_() {
  esp_err_t err = rmt_enable(this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT enable failed: %s", esp_err_to_name(err));
    return false;
  }

  rmt_transmit_config_t tx_config = {};
  tx_config.loop_count = -1;
  tx_config.flags.eot_level = 0;

  err = rmt_transmit(this->tx_channel_, this->encoder_, &this->pwm_symbol_, sizeof(this->pwm_symbol_), &tx_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT transmit start failed: %s", esp_err_to_name(err));
    return false;
  }

  return true;
}

bool FanPWMOutput::stop_output_() {
  esp_err_t err = rmt_disable(this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT disable failed: %s", esp_err_to_name(err));
    return false;
  }
  return true;
}

bool FanPWMOutput::restart_output_() {
  if (!this->stop_output_()) {
    return false;
  }
  return this->start_output_();
}

}  // namespace fan_pwm
}  // namespace esphome