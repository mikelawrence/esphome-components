#pragma once

#ifdef USE_ESP32

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

namespace esphome {
namespace esp32_rmt_pwm {

class ESP32RMTPWMOutput : public output::FloatOutput, public Component {
 public:
  ESP32RMTPWMOutput() = default;

  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; }
  void set_rmt_symbols(size_t rmt_symbols) { this->rmt_symbols_ = rmt_symbols; }

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void setup() override;
  void dump_config() override;

 protected:
  void write_state(float state) override;
  void build_symbol_(float state);
  bool start_pwm_();
  bool restart_pwm_();

  InternalGPIOPin *pin_{nullptr};
  uint32_t frequency_{25000};
  uint32_t resolution_hz_{10000000};
  size_t rmt_symbols_{48};
  uint32_t period_ticks_{0};
  float last_state_{0.0f};

  portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;

  rmt_symbol_word_t pwm_symbol_{};
  rmt_channel_handle_t tx_channel_{nullptr};
  rmt_encoder_handle_t encoder_{nullptr};
};

}  // namespace esp32_rmt_pwm
}  // namespace esphome

#endif  // USE_ESP32