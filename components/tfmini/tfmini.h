#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome
{
  namespace tfmini
  {
    enum TFminiModel
    {
      TFMINI_MODEL_TFMINI_S = 0,
      TFMINI_MODEL_TFMINI_PLUS,
      TFMINI_MODEL_TF_LUNA,
    };

    enum TFminiCmd
    {
      TFMINI_CMD_FW_VERSION = 0x01,
      TFMINI_CMD_SOFT_RESET = 0x02,
      TFMINI_CMD_SAMPLE_RATE = 0x03,
      TFMINI_CMD_TRIGGER = 0x04,
      TFMINI_CMD_OUTPUT_FORMAT = 0x05,
      TFMINI_CMD_BAUD_RATE = 0x06,
      TFMINI_CMD_OUTPUT_CONTROL = 0x07,
      TFMINI_CMD_HARD_RESET = 0x10,
      TFMINI_CMD_SAVE_SETTINGS = 0x11,
      TFMINI_CMD_LOW_POWER = 0x35,
    };

    class TFminiComponent : public uart::UARTDevice, public Component
    {
    public:
      void set_model(TFminiModel model) { this->model_ = model; };
      void set_distance_sensor(sensor::Sensor *distance_sensor) { this->distance_sensor_ = distance_sensor; };
      void set_sample_rate(uint32_t rate) { this->sample_rate_ = rate; };
      void set_multi_pin(GPIOPin *pin) { this->multi_pin_ = pin; }
      void set_config_pin(GPIOPin *pin) { this->config_pin_ = pin; }
      // void set_dir_sample_num(uint16_t sample_num) { this->dir_sample_num_ = sample_num; };
      // void set_direction_sensor(sensor::Sensor *direction_sensor) { this->direction_sensor_ = direction_sensor; };
      void set_signal_strength_sensor(sensor::Sensor *signal_strength_sensor) { this->signal_strength_sensor_ = signal_strength_sensor; };
      void set_temperature_sensor(sensor::Sensor *temperature_sensor) { this->temperature_sensor_ = temperature_sensor; };
      void set_version_sensor(text_sensor::TextSensor *version_sensor) { this->version_sensor_ = version_sensor; };
      void set_low_power_mode(uint32_t mode) { this->low_power_ = mode; };

      // ========== INTERNAL METHODS ==========
      void setup() override;
      void loop() override;
      void dump_config() override;

    protected:
      bool setup_{false};
      bool error_{false};
      uint64_t cmd_time_{0};
      float config_elapse_{0};

      // Config Variables
      TFminiModel model_;
      uint32_t sample_rate_{100};
      // uint16_t dir_sample_num_{10};
      bool low_power_{false};

      // Local Variables
      bool soft_reset_received_ = false;
      bool sample_rate_received_ = false;
      bool low_power_received_ = false;
      bool version_received_ = false;
      std::vector<uint8_t> rx_buffer_;
      std::vector<uint8_t> tx_buffer_;

      // Methods
      bool verify_rx_packet_checksum_();
      std::string rx_packet_to_str_();
      std::string tx_packet_to_str_();
      void process_rx_data();
      void process_data_frame();
      void process_response_frame();
      void send_command_(uint8_t cmd);
      void comp_cs_send_command_();

      // Pins
      GPIOPin *multi_pin_{nullptr};
      GPIOPin *config_pin_{nullptr};
      // Sensors
      text_sensor::TextSensor *version_sensor_{nullptr};
      sensor::Sensor *distance_sensor_{nullptr};
      // sensor::Sensor *direction_sensor_{nullptr};
      sensor::Sensor *signal_strength_sensor_{nullptr};
      sensor::Sensor *temperature_sensor_{nullptr};
    };

  } // namespace tfmini
} // namespace esphome
