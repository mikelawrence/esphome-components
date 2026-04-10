#include "esp32_rmt_pwm_output.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace esp32_rmt_pwm {

static const char *const TAG = "esp32_rmt_pwm";

void ESP32RMTPWMOutput::setup() {
  if (this->pin_ == nullptr) {
    ESP_LOGE(TAG, "No pin configured");
    this->mark_failed();
    return;
  }

  if (this->frequency_ == 0) {
    ESP_LOGE(TAG, "Frequency must be greater than 0");
    this->mark_failed();
    return;
  }

  if (this->rmt_symbols_ < 48) {
    ESP_LOGE(TAG, "rmt_symbols must be at least 48");
    this->mark_failed();
    return;
  }

  this->resolution_hz_ = 10000000;
  this->period_ticks_ = this->resolution_hz_ / this->frequency_;

  if (this->period_ticks_ < 2) {
    ESP_LOGE(TAG, "Frequency too high for 10MHz resolution (period=%lu ticks)", (unsigned long) this->period_ticks_);
    this->mark_failed();
    return;
  }

  if (this->period_ticks_ > 32767) {
    ESP_LOGE(TAG, "Frequency too low: period %lu exceeds 15-bit RMT symbol limit", (unsigned long) this->period_ticks_);
    this->mark_failed();
    return;
  }

  rmt_tx_channel_config_t tx_config = {};
  tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
  tx_config.gpio_num = static_cast<gpio_num_t>(this->pin_->get_pin());
  tx_config.mem_block_symbols = this->rmt_symbols_;
  tx_config.resolution_hz = this->resolution_hz_;
  tx_config.trans_queue_depth = 1;
  tx_config.flags.invert_out = this->pin_->is_inverted();
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

  err = rmt_enable(this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT enable failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  portENTER_CRITICAL(&this->lock_);
  this->build_symbol_(0.0f);
  portEXIT_CRITICAL(&this->lock_);

  if (!this->start_pwm_()) {
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "RMT PWM configured: pin=GPIO%d freq=%luHz period=%lu ticks rmt_symbols=%u", this->pin_->get_pin(),
           (unsigned long) this->frequency_, (unsigned long) this->period_ticks_, (unsigned) this->rmt_symbols_);
}

void ESP32RMTPWMOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 RMT PWM Output:");
  LOG_PIN("  Pin: ", this->pin_);
  ESP_LOGCONFIG(TAG, "  Frequency: %lu Hz", (unsigned long) this->frequency_);
  ESP_LOGCONFIG(TAG, "  Resolution: %lu Hz", (unsigned long) this->resolution_hz_);
  ESP_LOGCONFIG(TAG, "  RMT symbols: %u", (unsigned) this->rmt_symbols_);
  ESP_LOGCONFIG(TAG, "  Period: %lu ticks", (unsigned long) this->period_ticks_);
  ESP_LOGCONFIG(TAG, "  Last state: %.3f", this->last_state_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  State: FAILED");
  }
}

void ESP32RMTPWMOutput::write_state(float state) {
  if (state < 0.0f) {
    state = 0.0f;
  } else if (state > 1.0f) {
    state = 1.0f;
  }

  this->last_state_ = state;

  portENTER_CRITICAL(&this->lock_);
  this->build_symbol_(state);
  portEXIT_CRITICAL(&this->lock_);

  if (!this->restart_pwm_()) {
    ESP_LOGE(TAG, "Failed to restart PWM after state update");
    this->mark_failed();
  }
}

void ESP32RMTPWMOutput::build_symbol_(float state) {
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

bool ESP32RMTPWMOutput::start_pwm_() {
  if (this->tx_channel_ == nullptr || this->encoder_ == nullptr) {
    return false;
  }

  rmt_transmit_config_t tx_config = {};
  tx_config.loop_count = -1;
  tx_config.flags.eot_level = 0;

  esp_err_t err =
      rmt_transmit(this->tx_channel_, this->encoder_, &this->pwm_symbol_, sizeof(this->pwm_symbol_), &tx_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT transmit failed: %s", esp_err_to_name(err));
    return false;
  }

  return true;
}

bool ESP32RMTPWMOutput::restart_pwm_() {
  if (this->tx_channel_ == nullptr) {
    return false;
  }

  esp_err_t err = rmt_disable(this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT disable failed: %s", esp_err_to_name(err));
    return false;
  }

  err = rmt_enable(this->tx_channel_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "RMT re-enable failed: %s", esp_err_to_name(err));
    return false;
  }

  return this->start_pwm_();
}

}  // namespace esp32_rmt_pwm
}  // namespace esphome

#endif  // USE_ESP32