#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tfmini {
enum TFminiModel { MODEL_TFMINI_S = 0, MODEL_TFMINI_PLUS, MODEL_TF_LUNA };
enum SetupStates {
  TFMINI_SM_START,
  TFMINI_SM_LOW_POWER,
  TFMINI_SM_SAMPLE_RATE,
  TFMINI_SM_FW_VERSION,
  TFMINI_SM_GET_FW,
  TFMINI_SM_DONE
};

class TFminiComponent : public uart::UARTDevice, public Component {
  SUB_SENSOR(distance)
  SUB_SENSOR(signal_strength)
  SUB_SENSOR(temperature)

 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_model(TFminiModel model) { this->model_ = model; };
  void set_sample_rate(uint32_t rate) { this->sample_rate_ = rate; };
  void set_config_pin(GPIOPin *pin) { this->config_pin_ = pin; }
  void set_low_power_mode(uint32_t mode) { this->low_power_ = mode; };

 protected:
  void internal_setup_(SetupStates state);
  bool verify_rx_packet_checksum_();
  void process_rx_data();
  void process_data_frame();
  void process_response_frame();
  void send_command_(uint8_t cmd);
  void comp_cs_send_command_();
  std::string rx_packet_to_str_();
  std::string tx_packet_to_str_();
  bool initialized_{false};
  bool low_power_{false};
  bool soft_reset_received_{false};
  bool sample_rate_received_{false};
  bool low_power_received_{false};
  bool version_received_{false};
  uint16_t cmd_attempts_;
  uint32_t sample_rate_{100};
  TFminiModel model_;
  GPIOPin *config_pin_{nullptr};
  std::string firmware_version_{"Unknown"};
  FixedVector<uint8_t> rx_buffer_;
  FixedVector<uint8_t> tx_buffer_;
};

}  // namespace tfmini
}  // namespace esphome
