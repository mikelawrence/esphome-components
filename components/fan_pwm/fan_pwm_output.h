#pragma once

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fan_pwm {

class FanPWMOutput : public output::FloatOutput, public Component {
 public:
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; }

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void setup() override;
  void dump_config() override;

 protected:
  void write_state(float state) override;
  void build_symbol_(float state);
  bool start_output_();
  bool stop_output_();
  bool restart_output_();

  uint8_t pin_{0};
  uint32_t frequency_{25000};
  uint32_t resolution_hz_{10000000};
  uint32_t period_ticks_{0};
  float last_state_{0.0f};

  rmt_symbol_word_t pwm_symbol_{};
  rmt_channel_handle_t tx_channel_{nullptr};
  rmt_encoder_handle_t encoder_{nullptr};
};

}  // namespace fan_pwm
}  // namespace esphome