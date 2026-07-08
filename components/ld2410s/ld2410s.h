#pragma once

// core
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

// components
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#include "esphome/components/ld24xx/ld24xx.h"
#include "esphome/components/uart/uart.h"

// std
#include <cstddef>
#include <cstdint>

namespace esphome::ld2410s {

using namespace ld24xx;

#pragma region Constants
static constexpr uint8_t TOTAL_GATES = 16;  // Total number of gates supported by the LD2410S

static const uint16_t NO_SUB_CMD = 0xffff;
static const uint16_t FRAME_DATA_LENGTH_SIZE = 2;
static const size_t RX_TX_BUFFER_SIZE = 128;
static const uint16_t RX_MAX_BYTES_PER_LOOP = 128;
static const uint8_t TX_SCHEDULE_BUFFER_SIZE = 32;
static const uint8_t TX_MAX_RESEND = 5;
static const uint8_t TX_MAX_RESTART = 5;
static const uint32_t TX_CONFIRMATION_TIMEOUT = 1000;  // timeout for waiting for cmd response
static const uint32_t TX_PAUSE_TIMEOUT = 100;          // pause after receiving response
#pragma endregion

#pragma region enum
enum class TxCmdState { IDLE, SCHEDULED, SEND, SENT, FAILED };
enum class RxFrameType { UNKNOWN, SHORT_DATA_FRAME, STD_DATA_FRAME, CMD_FRAME, NOK };
enum class RxEvaluationResult { UNKNOWN, OK, NOK };
enum class ResponseSpeed : uint8_t { NORMAL = 0, FAST };
#pragma endregion

#pragma region struct
struct TxTaskT {
  uint16_t command;
  uint16_t sub_command;
};
#pragma endregion

class LD2410Srx {
 public:
  RxEvaluationResult receive_byte(uint32_t loop_count, uint8_t byte);

  RxFrameType frame_type() const { return this->frame_type_; }
  uint8_t *frame_data() { return this->rcv_buffer_; }
  uint8_t frame_size() const { return this->end_pos_; }

  uint8_t *payload_data() { return &this->rcv_buffer_[this->payload_pos_]; }
  uint8_t payload_size() const { return this->payload_size_; }
  bool payload_ready() const { return payload_ready_; }

  void reset();

 protected:
  uint8_t rcv_buffer_[RX_TX_BUFFER_SIZE] = {};
  uint16_t end_pos_{0};

  uint16_t header_footer_size_{0};
  uint16_t expected_frame_size_{0};
  uint16_t size_field_size_{0};

  uint16_t payload_pos_{0};
  uint16_t payload_size_{0};
  RxFrameType frame_type_{RxFrameType::UNKNOWN};
  bool payload_ready_{false};

  std::string msg_{""};

  RxEvaluationResult evaluate_header_(uint16_t end_pos);
  RxEvaluationResult evaluate_size_(uint16_t end_pos);
  RxEvaluationResult evaluate_footer_(uint16_t end_pos);
  static int read_int(const uint8_t *buffer, size_t pos, size_t len);
};

class LD2410Sschedule {
 public:
  void append(uint16_t command, uint16_t sub_command = NO_SUB_CMD);
  TxCmdState check_state(uint32_t loop_count);
  void confirm_ready_to_send();
  void confirm_sent();
  void verify_response(uint16_t command_word, uint32_t loop_count);
  void reset_schedule();
  uint16_t get_command();
  uint16_t get_sub_command();

 protected:
  TxTaskT commands_[TX_SCHEDULE_BUFFER_SIZE] = {};
  uint32_t time_started_{0};
  uint8_t retry_count_{0};
  uint8_t restart_count_{0};
  uint8_t last_{0};
  uint8_t active_{0};
  TxCmdState state_ = TxCmdState::IDLE;
  bool config_mode_{true};
};

class LD2410SComponent final : public Component, public uart::UARTDevice {
#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(cal_running)
  SUB_BINARY_SENSOR(has_target)
#endif
#ifdef USE_BUTTON
  SUB_BUTTON(cal_start)
  SUB_BUTTON(factory_reset)
#endif
#ifdef USE_NUMBER
  SUB_NUMBER(max_detect_gate)
  SUB_NUMBER(min_detect_gate)
  SUB_NUMBER(no_delay)
  SUB_NUMBER(status_reporting_freq)
  SUB_NUMBER(distance_reporting_freq)
#endif
#ifdef USE_SELECT
  SUB_SELECT(response_speed)
#endif
#ifdef USE_SENSOR
  SUB_SENSOR_WITH_DEDUP(target_distance, uint16_t)
  SUB_SENSOR_WITH_DEDUP(cal_progress, uint16_t)
#endif
#ifdef USE_SWITCH
  SUB_SWITCH(minimal_output)
#endif
#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(fw_version)
#endif

 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override;

  // button
#ifdef USE_BUTTON
  void cal_start();
  void factory_reset();
#endif
#ifdef USE_NUMBER
  // set number component, arrays prevent use of SUB_NUMBER
  void set_gate_trig_threshold_number(uint8_t gate, number::Number *n) { this->gate_trig_threshold_number_[gate] = n; }
  // set number component, arrays prevent use of SUB_NUMBER
  void set_gate_hold_threshold_number(uint8_t gate, number::Number *n) { this->gate_hold_threshold_number_[gate] = n; }
  // set number values
  void set_gate_trig_threshold(uint8_t gate, float trigger_threshold);
  void set_gate_hold_threshold(uint8_t gate, float hold_threshold);
  void set_max_detect_gate(float max_detect_gate);
  void set_min_detect_gate(float min_detect_gate);
  void set_no_delay(float delay);
  void set_distance_reporting_freq(float distance_reporting_freq);
  void set_status_reporting_freq(float status_reporting_freq);
#endif
#ifdef USE_SELECT
  void set_response_speed(size_t index);
#endif
#ifdef USE_SENSOR
  void set_gate_energy_sensor(uint8_t gate, sensor::Sensor *s) { this->gate_energy_sensor_[gate].set_sensor(s); }
#endif
#ifdef USE_SWITCH
  void set_minimal_output(bool state);
#endif

 protected:
  void send_();
  void build_cmd_frame_(uint16_t command, uint16_t sub_command = NO_SUB_CMD);
  void sending_pause_();

  bool receive_();
  void parse_();
  void parse_short_data_frame_();
  void parse_data_frame_();
  void parse_cmd_frame_();

  void read_all_();
  void read_all_thresholds_();

  void parse_data_energy_values_read_(uint8_t *data);

  void parse_ack_config_start_(const uint8_t *data);
  void parse_ack_config_end_(const uint8_t *data);
  void parse_ack_config_read_(uint8_t *data);
  void parse_ack_fw_read_(const uint8_t *data);
  void parse_ack_minimal_output_(uint8_t *data);
  void parse_ack_thresholds_read_(uint8_t *data);
  void parse_ack_snrs_read_(uint8_t *data);

  LD2410Sschedule tx_schedule_;
  LD2410Srx rx_;

  uint8_t tx_frame_[RX_TX_BUFFER_SIZE] = {};

  // settings_;
  uint8_t gate_energy_[16] = {};
  uint16_t has_target_{0};
  uint16_t version_[3] = {0, 0, 0};
  uint16_t target_distance_{0};
  uint16_t cal_progress_{0};
  uint16_t tx_frame_size_{0};
  uint32_t gate_trig_threshold_[16] = {};
  uint32_t gate_hold_threshold_[16] = {};
  uint32_t max_detect_gate_{0};
  uint32_t min_detect_gate_{0};
  uint32_t delay_{0};
  uint32_t status_reporting_freq_{0};
  uint32_t distance_reporting_freq_{0};
  uint32_t resp_speed_{5};
  uint32_t loop_count_{0};
  bool cal_running_{false};
  bool pause_tx_{false};
  bool minimal_output_{true};
  bool init_done_{false};

#ifdef USE_NUMBER
  std::array<number::Number *, TOTAL_GATES> gate_trig_threshold_number_{};
  std::array<number::Number *, TOTAL_GATES> gate_hold_threshold_number_{};
#endif
#ifdef USE_SENSOR
  std::array<SensorWithDedup<uint8_t>, TOTAL_GATES> gate_energy_sensor_{};
#endif

  // append variable sized append_data to data, returns true if not overflow
  template<typename T>
  static bool append_seq_data(uint8_t *data, uint16_t &insert_position, const T *append_data,
                              uint16_t append_array_size = 1, uint16_t actual_size = 0) {
    size_t data_object_size = (actual_size == 0 ? sizeof(T) : actual_size);
    auto bytes_to_copy = append_array_size * data_object_size;
    if (insert_position + bytes_to_copy > RX_TX_BUFFER_SIZE) {
      return false;
    }

    auto *write_ptr = &data[0] + insert_position;
    memcpy(write_ptr, append_data, bytes_to_copy);

    insert_position += bytes_to_copy;

    return true;
  }

  // read variable sized uint from data and move read_position
  template<typename T>
  static bool read_seq_data(const uint8_t *data, uint16_t &read_position, T *out_data, uint16_t out_array_size = 1,
                            uint16_t actual_size = 0) {
    size_t data_object_size = (actual_size == 0 ? sizeof(T) : actual_size);
    size_t bytes_to_read = out_array_size * data_object_size;

    if (read_position + bytes_to_read > RX_TX_BUFFER_SIZE) {
      return false;
    }

    const uint8_t *read_ptr = &data[0] + read_position;

    memcpy(out_data, read_ptr, bytes_to_read);

    read_position += bytes_to_read;
    return true;
  }

  template<typename T>
  static bool append_seq_data_value(uint8_t *data, uint16_t &insert_position, uint16_t identifier, const T *append_data,
                                    uint16_t append_array_size = 1, uint16_t actual_size = 0) {
    return append_seq_data(data, insert_position, &identifier) &&
           append_seq_data(data, insert_position, append_data, append_array_size, actual_size);
  }

  static void append_gate_threshold(uint8_t *data, uint16_t &insert_position, uint16_t sub_command,
                                    const uint32_t *triggers_array, const uint32_t *holds_array) {
    if (sub_command != NO_SUB_CMD) {
      if (sub_command >= 16)
        return;
      append_seq_data(data, insert_position, &sub_command);
      if (sub_command < 8) {
        append_seq_data(data, insert_position, &triggers_array[sub_command]);
      } else {
        append_seq_data(data, insert_position, &holds_array[sub_command - 8]);
      }
    } else {
      for (uint16_t i = 0; i < 16; i++) {
        append_seq_data(data, insert_position, &i, 1);
        if (i < 8) {
          append_seq_data(data, insert_position, &triggers_array[i]);
        } else {
          append_seq_data(data, insert_position, &holds_array[i - 8]);
        }
      }
    }
  }

  static void append_gate_snrs(uint8_t *data, uint16_t &insert_position, uint16_t sub_command,
                               const uint32_t *triggers_array, const uint32_t *holds_array) {
    if (sub_command != NO_SUB_CMD) {
      if (sub_command >= 16)
        return;
      append_seq_data(data, insert_position, &sub_command);
      if (sub_command < 8) {
        append_seq_data(data, insert_position, &triggers_array[sub_command + 8]);
      } else {
        append_seq_data(data, insert_position, &holds_array[sub_command]);
      }
    } else {
      for (uint16_t i = 0; i < 16; i++) {
        append_seq_data(data, insert_position, &i, 1);
        if (i < 8) {
          append_seq_data(data, insert_position, &triggers_array[i + 8]);
        } else {
          append_seq_data(data, insert_position, &holds_array[i]);
        }
      }
    }
  }
};

}  // namespace esphome::ld2410s
