#include "ld2410s.h"

namespace esphome::ld2410s {

#pragma region LD2410S Specific Constants
static const char *const TAG = "ld2410s";

static const uint16_t CMD_CONFIRMATION = 0x0100;  // Command confirmation response code

static const uint8_t SHORT_DATA_FRAME_HEADER = 0x6E;
static const uint8_t SHORT_DATA_FRAME_FOOTER = 0x62;
static const uint32_t STD_DATA_FRAME_HEADER = 0xF1F2F3F4;
static const uint32_t STD_DATA_FRAME_FOOTER = 0xF5F6F7F8;
static const uint32_t CMD_FRAME_HEADER = 0xFAFBFCFD;
static const uint32_t CMD_FRAME_FOOTER = 0x01020304;

static const uint16_t CONFIG_MODE_START_CMD = 0x00FF;
static const uint16_t CONFIG_MODE_START_VALUE = 0x0001;
static const uint16_t CONFIG_MODE_END_CMD = 0x00FE;

static const uint16_t OUTPUT_MODE_SWITCH_CMD = 0x007A;
static const uint8_t OUTPUT_MODE_VALUE_STD[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const uint8_t OUTPUT_MODE_VALUE_MIN[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint16_t CALIBRATION_CMD = 0x0009;
static const uint16_t CALIBRATION_TRIGGER_VALUE = 0x0002;
static const uint16_t CALIBRATION_RETENTION_VALUE = 0x0001;
static const uint16_t CALIBRATION_TIME_VALUE = 0x0078;

static const uint16_t CFG_FW_READ_CMD = 0x0000;

static const uint16_t CFG_PARAMS_READ_CMD = 0x0071;
static const uint16_t CFG_PARAMS_WRITE_CMD = 0x0070;
static const uint16_t CFG_MAX_DETECTION_VALUE = 0x0005;
static const uint16_t CFG_MIN_DETECTION_VALUE = 0x000A;
static const uint16_t CFG_NO_DELAY_VALUE = 0x0006;
static const uint16_t CFG_STATUS_FREQ_VALUE = 0x0002;
static const uint16_t CFG_DISTANCE_FREQ_VALUE = 0x000C;
static const uint16_t CFG_RESPONSE_SPEED_VALUE = 0x000B;
static const std::string CFG_RESPONSE_SPEED_NORMAL = "Normal";
static const std::string CFG_RESPONSE_SPEED_FAST = "Fast";

static const uint16_t CFG_GATE_THRESHOLDS_READ_CMD = 0x0073;
static const uint16_t CFG_GATE_THRESHOLDS_WRITE_CMD = 0x0072;
static const uint32_t CFG_GATE_THRESHOLDS_WRITE_DATA[] = {
    48, 42, 36, 34, 32, 31, 31, 31,  // Distance Gate 0~7 Trigger Thresholds range 10~95 dB
    31, 31, 31, 31, 31, 31, 31, 31   // Distance Gate 0~7 Holding Thresholds range 10~95 dB
};

static const uint16_t CFG_GATE_SNRS_READ_CMD = 0x0075;
static const uint16_t CFG_GATE_SNRS_WRITE_CMD = 0x0074;
static const uint32_t CFG_GATE_SNRS_WRITE_DATA[] = {
    51, 50, 30, 28, 25, 25, 25, 25,  // Distance Gate 8~15 Trigger Thresholds range 5~63 dB
    25, 25, 25, 25, 25, 22, 22, 22   // Distance Gate 8~15 Holding Thresholds range 5~63 dB
};

// ToDo
// static const uint16_t SN_READ_CMD = 0x0011;
// static const uint16_t SN_WRITE_CMD = 0x0010;

#pragma endregion

#pragma region Defines
#define SAFE_PUBLISH_BINARY_SENSOR(sensor, value) \
  if (sensor != nullptr) \
    if (sensor->state != static_cast<bool>(value)) \
      sensor->publish_state(static_cast<bool>(value));
#define SAFE_PUBLISH_NUMBER(sensor, value) \
  if (sensor != nullptr) \
    if (sensor->state != static_cast<float>(value)) \
      sensor->publish_state(static_cast<float>(value));
#pragma endregion

// Helper function to format firmware version with stack allocation
// Buffer must be exactly 20 bytes (format: "vx.x.x" fits in 20 {19 + null terminator})
static inline void format_version_str(const uint16_t *version, std::span<char, 20> buffer) {
  snprintf(buffer.data(), buffer.size(), "v%" PRId16 ".%" PRId16 ".%" PRId16, version[0], version[1], version[2]);
}

#pragma region LD2410S

void LD2410SComponent::setup() {
  ESP_LOGD(TAG, "Setup");
  SAFE_PUBLISH_SENSOR(this->target_distance_sensor_, this->target_distance_);
  SAFE_PUBLISH_SENSOR(this->cal_progress_sensor_, this->cal_progress_);
  if (this->has_target_binary_sensor_ != nullptr) {
    this->has_target_binary_sensor_->publish_state(this->has_target_);
  }
  if (this->cal_running_binary_sensor_ != nullptr) {
    this->cal_running_binary_sensor_->publish_state(this->cal_running_);
  }
  this->init_done_ = false;
  this->minimal_output_ = true;
  this->read_all_();
}

void LD2410SComponent::loop() {
  if (!this->receive_()) {
    if (!this->pause_tx_) {
      this->send_();
    }
  }
  this->loop_count_++;
}

float LD2410SComponent::get_setup_priority() const { return setup_priority::HARDWARE; }

void LD2410SComponent::dump_config() {
  char version_s[20];
  format_version_str(this->version_, version_s);

  ESP_LOGCONFIG(TAG,
                "LD2410S:\n"
                "  Firmware version: %s",
                version_s);

#ifdef USE_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Has Target", this->has_target_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Cal Running", this->cal_running_binary_sensor_);
#endif
#ifdef USE_SENSOR
  ESP_LOGCONFIG(TAG, "Sensors:");
  LOG_SENSOR_WITH_DEDUP_SAFE("  ", "Cal Progress", this->cal_progress_sensor_);
  for (auto &s : this->gate_energy_sensor_) {
    LOG_SENSOR_WITH_DEDUP_SAFE("  ", "Gate Energy", s);
  }
  LOG_SENSOR_WITH_DEDUP_SAFE("  ", "Target Distance", this->target_distance_sensor_);
#endif
#ifdef USE_TEXT_SENSOR
  ESP_LOGCONFIG(TAG, "Text Sensors:");
  LOG_TEXT_SENSOR("  ", "FW Version", this->fw_version_text_sensor_);
#endif
#ifdef USE_NUMBER
  ESP_LOGCONFIG(TAG, "Numbers:");
  for (auto &n : this->gate_trig_threshold_number_) {
    LOG_NUMBER("  ", "Trigger Threshold", n);
  }
  for (number::Number *n : this->gate_hold_threshold_number_) {
    LOG_NUMBER("  ", "Hold Threshold", n);
  }
#endif
#ifdef USE_SELECT
  ESP_LOGCONFIG(TAG, "Selects:");
  LOG_SELECT("  ", "Response Speed", this->response_speed_select_);
#endif
#ifdef USE_SWITCH
  ESP_LOGCONFIG(TAG, "Switches:");
  LOG_SWITCH("  ", "Minimal Output", this->minimal_output_switch_);
#endif
#ifdef USE_BUTTON
  ESP_LOGCONFIG(TAG, "Buttons:");
  LOG_BUTTON("  ", "Factory Reset", this->factory_reset_button_);
  LOG_BUTTON("  ", "Cal Start", this->cal_start_button_);
#endif
}

// button
void LD2410SComponent::cal_start() {
  this->cal_progress_ = 0;
  this->cal_running_ = true;
  SAFE_PUBLISH_SENSOR(this->cal_progress_sensor_, this->cal_progress_);
  SAFE_PUBLISH_BINARY_SENSOR(this->cal_running_binary_sensor_, this->cal_running_);
  this->tx_schedule_.append(CALIBRATION_CMD);
}
void LD2410SComponent::factory_reset() {
  ESP_LOGI(TAG, "factory_reset");

  // load defaults
  this->max_detect_gate_ = 15;
  this->min_detect_gate_ = 0;
  this->delay_ = 10;
  this->status_reporting_freq_ = 80;
  this->distance_reporting_freq_ = 80;

  this->minimal_output_ = true;

  this->resp_speed_ = 5;

  for (uint8_t gate = 0; gate < TOTAL_GATES / 2; gate++) {
    this->gate_trig_threshold_[gate] = CFG_GATE_THRESHOLDS_WRITE_DATA[gate];
    this->gate_hold_threshold_[gate] = CFG_GATE_THRESHOLDS_WRITE_DATA[gate + 8];
    this->gate_trig_threshold_[gate + 8] = CFG_GATE_SNRS_WRITE_DATA[gate];
    this->gate_hold_threshold_[gate + 8] = CFG_GATE_SNRS_WRITE_DATA[gate + 8];
  }

  // send defaults to radar
  this->tx_schedule_.append(OUTPUT_MODE_SWITCH_CMD);

  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD);
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_WRITE_CMD);
  this->tx_schedule_.append(CFG_GATE_SNRS_WRITE_CMD);

  this->tx_schedule_.append(CFG_PARAMS_READ_CMD);
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_READ_CMD);
  this->tx_schedule_.append(CFG_GATE_SNRS_READ_CMD);
}
// number
void LD2410SComponent::set_no_delay(float delay) {
  this->delay_ = delay;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_NO_DELAY_VALUE);
}
void LD2410SComponent::set_distance_reporting_freq(float distance_reporting_freq) {
  this->distance_reporting_freq_ = distance_reporting_freq * 10;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_DISTANCE_FREQ_VALUE);
}
void LD2410SComponent::set_max_detect_gate(float max_detect_gate) {
  this->max_detect_gate_ = max_detect_gate;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_MAX_DETECTION_VALUE);
}
void LD2410SComponent::set_min_detect_gate(float min_detect_gate) {
  this->min_detect_gate_ = min_detect_gate;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_MIN_DETECTION_VALUE);
}
void LD2410SComponent::set_status_reporting_freq(float status_reporting_freq) {
  this->status_reporting_freq_ = status_reporting_freq * 10;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_STATUS_FREQ_VALUE);
}
void LD2410SComponent::set_gate_hold_threshold(uint8_t gate, float hold_threshold) {
  this->gate_hold_threshold_[gate] = hold_threshold;
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_WRITE_CMD, gate);
}
void LD2410SComponent::set_gate_trig_threshold(uint8_t gate, float trigger_threshold) {
  this->gate_trig_threshold_[gate] = trigger_threshold;
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_WRITE_CMD, gate);
}
// select
void LD2410SComponent::set_response_speed(size_t index) {
  this->resp_speed_ = index == 1 ? 10 : 5;
  this->tx_schedule_.append(CFG_PARAMS_WRITE_CMD, CFG_RESPONSE_SPEED_VALUE);
}
// switch
void LD2410SComponent::set_minimal_output(bool state) {
  this->minimal_output_ = state;
  this->tx_schedule_.append(OUTPUT_MODE_SWITCH_CMD);
#ifdef USE_SENSOR
  if (state) {
    for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
      SAFE_PUBLISH_SENSOR_UNKNOWN(this->gate_energy_sensor_[gate]);
    }
  }
#endif
}
void LD2410SComponent::read_all_() {
  this->tx_schedule_.append(OUTPUT_MODE_SWITCH_CMD);
  this->tx_schedule_.append(CFG_FW_READ_CMD);
  this->tx_schedule_.append(CFG_PARAMS_READ_CMD);
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_READ_CMD);
  this->tx_schedule_.append(CFG_GATE_SNRS_READ_CMD);
}
// prepares scheduled frames for sending
// executes actual data sending
void LD2410SComponent::send_() {
  switch (this->tx_schedule_.check_state(this->loop_count_)) {
    case TxCmdState::SCHEDULED:
      this->build_cmd_frame_(this->tx_schedule_.get_command(), this->tx_schedule_.get_sub_command());
      this->tx_schedule_.confirm_ready_to_send();
      break;

    case TxCmdState::SEND:
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGVV(TAG, ">   [loop:%" PRIu32 "] cmd:%04" PRIX16 " > %s", this->loop_count_,
                this->tx_schedule_.get_command(),
                format_hex_pretty_to(hex_buf, this->tx_frame_, this->tx_frame_size_, ' '));
#endif
      this->write_array(this->tx_frame_, this->tx_frame_size_);
      this->flush();

      this->tx_schedule_.confirm_sent();
      this->sending_pause_();

      break;

    case TxCmdState::FAILED:
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
      ESP_LOGV(TAG, ">XX [loop:%" PRIu32 "] Scheduling command send failed!!!, re-initializing...", this->loop_count_);
#else
      ESP_LOGD(TAG, "Scheduling command send failed!!!, re-initializing...");
#endif
      this->tx_schedule_.reset_schedule();
      this->pause_tx_ = true;
      this->set_timeout("Pausing Sending", 15000, [this]() { this->pause_tx_ = false; });
      static const uint8_t CFG_END[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
      this->write_array(CFG_END, sizeof(CFG_END));
      this->flush();
      break;

    case TxCmdState::IDLE:
      if (!this->init_done_) {
        this->init_done_ = true;
        ESP_LOGCONFIG(TAG, "Initialized");
      }
      break;

    case TxCmdState::SENT:
    default:
      break;
  }
}
// builds CMD_FRAME
void LD2410SComponent::build_cmd_frame_(uint16_t command, uint16_t sub_command) {
  ESP_LOGV(TAG, ":>> [loop:%" PRIu32 "] cmd:%04" PRIX16 " Prepare frame ", this->loop_count_, command);

  this->tx_frame_size_ = 0;

  // Header
  append_seq_data(this->tx_frame_, this->tx_frame_size_, &CMD_FRAME_HEADER);

  // Frame size placeholder
  uint16_t size_start = this->tx_frame_size_;
  this->tx_frame_size_ += sizeof(size_start);

  // Data start
  uint16_t data_start = this->tx_frame_size_;

  // Command
  append_seq_data(this->tx_frame_, this->tx_frame_size_, &command, 1);

  // Parameters
  switch (command) {
    case OUTPUT_MODE_SWITCH_CMD: {
      if (this->minimal_output_) {
        append_seq_data(this->tx_frame_, this->tx_frame_size_, OUTPUT_MODE_VALUE_MIN, 6);
      } else {
        append_seq_data(this->tx_frame_, this->tx_frame_size_, OUTPUT_MODE_VALUE_STD, 6);
      }
    } break;

    case CONFIG_MODE_START_CMD:
      append_seq_data(this->tx_frame_, this->tx_frame_size_, &CONFIG_MODE_START_VALUE);
      break;

    case CONFIG_MODE_END_CMD:
      break;

    case CFG_PARAMS_READ_CMD:

      switch (sub_command) {
        case CFG_MAX_DETECTION_VALUE:
        case CFG_MIN_DETECTION_VALUE:
        case CFG_NO_DELAY_VALUE:
        case CFG_STATUS_FREQ_VALUE:
        case CFG_DISTANCE_FREQ_VALUE:
        case CFG_RESPONSE_SPEED_VALUE:
          append_seq_data(this->tx_frame_, this->tx_frame_size_, &sub_command);
          break;

        default:
          const uint16_t cfg_values[] = {CFG_MAX_DETECTION_VALUE, CFG_MIN_DETECTION_VALUE, CFG_NO_DELAY_VALUE,
                                         CFG_STATUS_FREQ_VALUE,   CFG_DISTANCE_FREQ_VALUE, CFG_RESPONSE_SPEED_VALUE};
          append_seq_data(this->tx_frame_, this->tx_frame_size_, &cfg_values, 6, sizeof(sub_command));
          break;
      }

      break;

    case CFG_FW_READ_CMD:
      break;

    case CFG_PARAMS_WRITE_CMD:
      if (this->resp_speed_ == 0) {
        ESP_LOGD(TAG, "CFG_PARAMS_WRITE_CMD Error, bad new_config");
        return;
      } else {
        switch (sub_command) {
          case CFG_MAX_DETECTION_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->max_detect_gate_);
            break;

          case CFG_MIN_DETECTION_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->min_detect_gate_);
            break;

          case CFG_NO_DELAY_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->delay_);
            break;

          case CFG_STATUS_FREQ_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->status_reporting_freq_);
            break;

          case CFG_DISTANCE_FREQ_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->distance_reporting_freq_);
            break;

          case CFG_RESPONSE_SPEED_VALUE:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, sub_command, &this->resp_speed_);
            break;

          default:
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_MAX_DETECTION_VALUE,
                                  &this->max_detect_gate_);
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_MIN_DETECTION_VALUE,
                                  &this->min_detect_gate_);
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_NO_DELAY_VALUE, &this->delay_);
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_STATUS_FREQ_VALUE,
                                  &this->status_reporting_freq_);
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_DISTANCE_FREQ_VALUE,
                                  &this->distance_reporting_freq_);
            append_seq_data_value(this->tx_frame_, this->tx_frame_size_, CFG_RESPONSE_SPEED_VALUE, &this->resp_speed_);
            break;
        }
        break;
      }

    case CALIBRATION_CMD:
      append_seq_data(this->tx_frame_, this->tx_frame_size_, &CALIBRATION_TRIGGER_VALUE);
      append_seq_data(this->tx_frame_, this->tx_frame_size_, &CALIBRATION_RETENTION_VALUE);
      append_seq_data(this->tx_frame_, this->tx_frame_size_, &CALIBRATION_TIME_VALUE);
      break;

    case CFG_GATE_THRESHOLDS_READ_CMD:
    case CFG_GATE_SNRS_READ_CMD:
      if (sub_command != NO_SUB_CMD) {
        append_seq_data(this->tx_frame_, this->tx_frame_size_, &sub_command);
      } else {
        for (uint16_t i = 0; i < 16; i++) {
          append_seq_data(this->tx_frame_, this->tx_frame_size_, &i);
        }
      }
      break;

    case CFG_GATE_THRESHOLDS_WRITE_CMD:
      append_gate_threshold(this->tx_frame_, this->tx_frame_size_, sub_command, this->gate_trig_threshold_,
                            this->gate_hold_threshold_);
      break;

    case CFG_GATE_SNRS_WRITE_CMD:
      append_gate_snrs(this->tx_frame_, this->tx_frame_size_, sub_command, this->gate_trig_threshold_,
                       this->gate_hold_threshold_);
      break;

    default:
      break;
  }

  // Frame size
  uint16_t data_size = this->tx_frame_size_ - data_start;
  append_seq_data(this->tx_frame_, size_start, &data_size);

  // Footer
  append_seq_data(this->tx_frame_, this->tx_frame_size_, &CMD_FRAME_FOOTER);
}

void LD2410SComponent::sending_pause_() {
  this->pause_tx_ = true;
  this->set_timeout("Pausing Sending", TX_PAUSE_TIMEOUT, [this]() {
    ESP_LOGVV(TAG, "Proceeding after tx pause of %" PRIu32 " ms", TX_PAUSE_TIMEOUT);
    this->pause_tx_ = false;
  });
}

// receives frames and starts processing
bool LD2410SComponent::receive_() {
  uint8_t rx;
  int rx_bytes_count = 0;

  while (this->available() && rx_bytes_count < RX_MAX_BYTES_PER_LOOP) {
    if (!this->read_byte(&rx))
      break;
    rx_bytes_count++;

    if (this->rx_.receive_byte(this->loop_count_, rx) == RxEvaluationResult::OK) {
      this->parse_();
      this->rx_.reset();
    }
  }
  return rx_bytes_count > 0;
}
// starts received frame decoding, and handling received data
void LD2410SComponent::parse_() {
  switch (this->rx_.frame_type()) {
    case RxFrameType::SHORT_DATA_FRAME:
      this->parse_short_data_frame_();
      break;

    case RxFrameType::STD_DATA_FRAME:
      this->parse_data_frame_();
      break;

    case RxFrameType::CMD_FRAME:
      this->parse_cmd_frame_();
      break;

    default:
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGE(TAG, "Received Unknown package type!!! < %s",
               format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
      break;
  }
}
void LD2410SComponent::parse_short_data_frame_() {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
  char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
  ESP_LOGVV(TAG, "<   [loop:%" PRIu32 "] short data < %s", this->loop_count_,
            format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
#endif
  if (this->rx_.payload_size() < 3) {
    ESP_LOGW(TAG, "Payload too small, ignored");
    return;
  }

  this->has_target_ = this->rx_.payload_data()[0] > 1;
  this->target_distance_ = encode_uint16(this->rx_.payload_data()[2], this->rx_.payload_data()[1]);

  if (!this->has_target_)
    this->target_distance_ = 0;

  SAFE_PUBLISH_SENSOR(this->target_distance_sensor_, this->target_distance_);
  SAFE_PUBLISH_BINARY_SENSOR(this->has_target_binary_sensor_, this->has_target_);
}
void LD2410SComponent::parse_data_frame_() {
  if (this->rx_.payload_size() < 1) {
    ESP_LOGW(TAG, "Payload too small, ignored");
    return;
  }

  switch (this->rx_.payload_data()[0]) {
    case 0x01:  // standard data
    {
      if (this->rx_.payload_size() < 4) {
        ESP_LOGW(TAG, "Payload too small, ignored");
        break;
      }

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGV(TAG, "<   [loop:%" PRIu32 "] Std Data < %s", this->loop_count_,
               format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
#endif
      this->has_target_ = this->rx_.payload_data()[1] > 1;

      this->target_distance_ = encode_uint16(this->rx_.payload_data()[3], this->rx_.payload_data()[2]);
      if (!this->has_target_) {
        this->target_distance_ = 0;
      }
      SAFE_PUBLISH_SENSOR(this->target_distance_sensor_, this->target_distance_);
      SAFE_PUBLISH_BINARY_SENSOR(this->has_target_binary_sensor_, this->has_target_);

      if (this->rx_.payload_size() < 7) {
        // no energy values
        ESP_LOGV(TAG, "Std Data Frame Parsed Has Target=%s, Target Distance=%" PRIu16 "cm",
                 TRUEFALSE(this->has_target_), this->target_distance_);
      } else {
        // energy values included
        this->parse_data_energy_values_read_(&this->rx_.payload_data()[6]);
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
        char energy_s[65];
        snprintf(energy_s, 65,
                 "%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16
                 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16,
                 this->gate_energy_[0], this->gate_energy_[1], this->gate_energy_[2], this->gate_energy_[3],
                 this->gate_energy_[4], this->gate_energy_[5], this->gate_energy_[6], this->gate_energy_[7],
                 this->gate_energy_[8], this->gate_energy_[9], this->gate_energy_[10], this->gate_energy_[11],
                 this->gate_energy_[12], this->gate_energy_[13], this->gate_energy_[14], this->gate_energy_[15]);
        ESP_LOGV(TAG, "Std Data Frame Parsed Has Target=%s, Target Distance=%" PRIu16 "cm, Energy=%s",
                 TRUEFALSE(this->has_target_), this->target_distance_, energy_s);
#endif
      }

      break;
    }

    case 0x03:  // calibration progress
    {
      if (this->rx_.payload_size() < 3) {
        ESP_LOGW(TAG, "Payload too small, ignored");
        break;
      }

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGVV(TAG, "<   [loop:%" PRIu32 "] std calibration < %s", this->loop_count_,
                format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
#endif
      this->cal_progress_ = encode_uint16(this->rx_.payload_data()[2], this->rx_.payload_data()[1]);
      this->sending_pause_();

      if (this->cal_progress_ == 100) {
        this->cal_running_ = false;
        SAFE_PUBLISH_SENSOR(this->cal_progress_sensor_, this->cal_progress_);
        SAFE_PUBLISH_BINARY_SENSOR(this->cal_running_binary_sensor_, this->cal_running_);
        this->read_all_thresholds_();
        this->set_timeout("Cal Prog Delay", 5000,
                          [this]() { SAFE_PUBLISH_SENSOR_UNKNOWN(this->cal_progress_sensor_); });
      } else {
        this->cal_running_ = true;
        SAFE_PUBLISH_SENSOR(this->cal_progress_sensor_, this->cal_progress_);
      }
      break;
    }

    default:
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGVV(TAG, "<XX [loop:%" PRIu32 "] std, Unknown std frame type < %s", this->loop_count_,
                format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
#endif
      break;
  }
}
void LD2410SComponent::parse_cmd_frame_() {
  if (this->rx_.payload_size() < 4)
    return;

  uint8_t *data_start = this->rx_.payload_data();
  uint16_t read_position = 0;
  uint16_t command_word = 0;
  uint16_t ack = 0;

  read_seq_data(data_start, read_position, &command_word);
  read_seq_data(data_start, read_position, &ack);

  char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
  if (ack == 0x0000) {
    ESP_LOGVV(TAG, "<   [loop:%" PRIu32 "] cmd:%04" PRIX16 " < %s", this->loop_count_, command_word,
              format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
  } else {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
    ESP_LOGD(TAG, "<XX [loop:%" PRIu32 "] cmd:%04" PRIX16 " Failed ack:%" PRIX16 " < %s", this->loop_count_,
             command_word, ack, format_hex_pretty_to(hex_buf, this->rx_.frame_data(), this->rx_.frame_size(), ' '));
#else
    ESP_LOGE(TAG, "Failed ack");
#endif
  }

  this->tx_schedule_.verify_response(command_word, this->loop_count_);

  uint8_t *data = &data_start[read_position];

  switch (command_word) {
      // Process acknowledgements

    case CONFIG_MODE_START_CMD | CMD_CONFIRMATION:
      if (this->rx_.payload_size() < 8)
        break;
      this->parse_ack_config_start_(data);
      break;

    case CONFIG_MODE_END_CMD | CMD_CONFIRMATION:
      this->parse_ack_config_end_(data);
      break;

    case CALIBRATION_CMD | CMD_CONFIRMATION:
      ESP_LOGI(TAG, "Calibration started");
      break;

      // Write command acknowledgements

    case CFG_PARAMS_WRITE_CMD | CMD_CONFIRMATION:
      ESP_LOGI(TAG, "Config written");
      break;

    case OUTPUT_MODE_SWITCH_CMD | CMD_CONFIRMATION:
      this->parse_ack_minimal_output_(data);
      break;

    case CFG_GATE_THRESHOLDS_WRITE_CMD | CMD_CONFIRMATION:
      ESP_LOGI(TAG, "Gate Threshold written");
      break;

    case CFG_GATE_SNRS_WRITE_CMD | CMD_CONFIRMATION:
      ESP_LOGI(TAG, "Gate SNR written");
      break;

      // Read command acknowledgements

    case CFG_PARAMS_READ_CMD | CMD_CONFIRMATION:
      if (this->rx_.payload_size() < 28)
        break;
      this->parse_ack_config_read_(data);
      break;

    case CFG_FW_READ_CMD | CMD_CONFIRMATION:
      if (this->rx_.payload_size() < 14)
        break;
      this->parse_ack_fw_read_(data);
      break;

    case CFG_GATE_THRESHOLDS_READ_CMD | CMD_CONFIRMATION:
      if (this->rx_.payload_size() < 68)
        break;
      this->parse_ack_thresholds_read_(data);
      break;

    case CFG_GATE_SNRS_READ_CMD | CMD_CONFIRMATION:
      if (this->rx_.payload_size() < 68)
        break;
      this->parse_ack_snrs_read_(data);
      break;

    default:
      ESP_LOGD(TAG, "< Unknown: %4x", command_word);
      break;
  }
}

void LD2410SComponent::read_all_thresholds_() {
  this->tx_schedule_.append(CFG_GATE_THRESHOLDS_READ_CMD);
  this->tx_schedule_.append(CFG_GATE_SNRS_READ_CMD);
}

void LD2410SComponent::parse_data_energy_values_read_(uint8_t *data) {
#ifdef USE_SENSOR
  uint16_t read_position = 0;
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    uint32_t val = 0;
    read_seq_data(data, read_position, &val);

    uint32_t db = 0;
    if (val > 0) {
      db = 10 * log10(val);
    }
    this->gate_energy_[gate] = db;

    SAFE_PUBLISH_SENSOR(this->gate_energy_sensor_[gate], this->gate_energy_[gate]);
  }
#endif
}

void LD2410SComponent::parse_ack_config_start_(const uint8_t *data) {
  uint16_t read_position = 0;
  uint16_t protocol_version = 0;
  uint16_t buffer_size = 0;
  read_seq_data(data, read_position, &protocol_version);  // does not exist in both documents
  read_seq_data(data, read_position, &buffer_size);

  ESP_LOGV(TAG, "CONFIG MODE ENABLED, protocol_version:%" PRIu16 "  buffer_size:%" PRIu16, protocol_version,
           buffer_size);
}
void LD2410SComponent::parse_ack_config_end_(const uint8_t *data) { ESP_LOGV(TAG, "CONFIG MODE DISABLED"); }
void LD2410SComponent::parse_ack_minimal_output_(uint8_t *data) {
  if (this->minimal_output_) {
    ESP_LOGV(TAG, "Parsed Minimal Output: ON");
  } else {
    ESP_LOGV(TAG, "Parsed Minimal Output: OFF");
  }
#ifdef USE_SWITCH
  if (this->minimal_output_switch_ != nullptr) {
    this->minimal_output_switch_->publish_state(this->minimal_output_);
  }
#endif
}
void LD2410SComponent::parse_ack_config_read_(uint8_t *data) {
  uint16_t read_position = 0;
  read_seq_data(data, read_position, &this->max_detect_gate_);
  ESP_LOGV(TAG, "Parsed Max Detect: %" PRIu32, this->max_detect_gate_);
  read_seq_data(data, read_position, &this->min_detect_gate_);
  ESP_LOGV(TAG, "Parsed Min Detect: %" PRIu32, this->min_detect_gate_);
  read_seq_data(data, read_position, &this->delay_);
  ESP_LOGV(TAG, "Parsed Delay: %" PRIu32, this->delay_);
  read_seq_data(data, read_position, &this->status_reporting_freq_);
  ESP_LOGV(TAG, "Parsed Status Reporting Frequency: %" PRIu32, this->status_reporting_freq_);
  read_seq_data(data, read_position, &this->distance_reporting_freq_);
  ESP_LOGV(TAG, "Parsed Distance Reporting Frequency: %" PRIu32, this->distance_reporting_freq_);
  read_seq_data(data, read_position, &this->resp_speed_);
  if (this->resp_speed_ != 5) {
    ESP_LOGV(TAG, "Parsed Response Speed: %s", "Normal");
  } else {
    ESP_LOGV(TAG, "Parsed Response Speed: %s", "Fast");
  }

#ifdef USE_NUMBER
  SAFE_PUBLISH_NUMBER(this->max_detect_gate_number_, static_cast<float>(this->max_detect_gate_));
  SAFE_PUBLISH_NUMBER(this->min_detect_gate_number_, static_cast<float>(this->min_detect_gate_));
  SAFE_PUBLISH_NUMBER(this->no_delay_number_, static_cast<float>(this->delay_));
  SAFE_PUBLISH_NUMBER(this->status_reporting_freq_number_, static_cast<float>(this->status_reporting_freq_) / 10);
  SAFE_PUBLISH_NUMBER(this->distance_reporting_freq_number_, static_cast<float>(this->distance_reporting_freq_) / 10);
#endif
#ifdef USE_SELECT
  if (this->response_speed_select_ != nullptr) {
    this->response_speed_select_->publish_state(this->resp_speed_ == 10 ? 1 : 0);
  }
#endif
}
void LD2410SComponent::parse_ack_fw_read_(const uint8_t *data) {
  uint16_t read_position = 4;  // skip equipment type
  read_seq_data(data, read_position, &this->version_[0]);
  read_seq_data(data, read_position, &this->version_[1]);
  read_seq_data(data, read_position, &this->version_[2]);
#ifdef USE_TEXT_SENSOR
  if (this->fw_version_text_sensor_ != nullptr) {
    char version_s[20];
    format_version_str(this->version_, version_s);
    ESP_LOGV(TAG, "Parsed Firmware Version: %s", version_s);
    if (this->fw_version_text_sensor_ != nullptr) {
      this->fw_version_text_sensor_->publish_state(version_s);
    }
  }
#endif
}
void LD2410SComponent::parse_ack_thresholds_read_(uint8_t *data) {
#ifdef USE_NUMBER
  uint16_t read_position = 0;
  uint32_t gate_threshold[16];
  read_seq_data(data, read_position, gate_threshold, 16, 4);
  for (uint8_t gate = 0; gate < TOTAL_GATES / 2; gate++) {
    this->gate_trig_threshold_[gate] = gate_threshold[gate];
    ESP_LOGV(TAG, "Parsed Trigger Threshold, Gate: %" PRIu8 ", Value: %" PRIu32, gate, this->gate_trig_threshold_[gate]);
    SAFE_PUBLISH_NUMBER(this->gate_trig_threshold_number_[gate], this->gate_trig_threshold_[gate]);
    this->gate_hold_threshold_[gate] = gate_threshold[gate + 8];
    ESP_LOGV(TAG, "Parsed Hold Threshold, Gate: %" PRIu8 ", Value: %" PRIu32, gate, this->gate_hold_threshold_[gate]);
    SAFE_PUBLISH_NUMBER(this->gate_hold_threshold_number_[gate], this->gate_hold_threshold_[gate]);
  }
#endif
}
void LD2410SComponent::parse_ack_snrs_read_(uint8_t *data) {
#ifdef USE_NUMBER
  uint16_t read_position = 0;
  uint32_t gate_threshold[16];
  read_seq_data(data, read_position, gate_threshold, 16, 4);
  for (uint8_t gate = 0; gate < TOTAL_GATES / 2; gate++) {
    this->gate_trig_threshold_[gate + 8] = gate_threshold[gate];
    SAFE_PUBLISH_NUMBER(this->gate_trig_threshold_number_[gate + 8], this->gate_trig_threshold_[gate + 8]);
    ESP_LOGV(TAG, "Parsed Trigger Threshold, Gate: %" PRIu8 ", Value: %" PRIu32, gate + 8,
             this->gate_trig_threshold_[gate + 8]);
    this->gate_hold_threshold_[gate + 8] = gate_threshold[gate + 8];
    ESP_LOGV(TAG, "Parsed Hold Threshold, Gate: %" PRIu8 ", Value: %" PRIu32, gate + 8, this->gate_hold_threshold_[gate + 8]);
    SAFE_PUBLISH_NUMBER(this->gate_hold_threshold_number_[gate + 8], this->gate_hold_threshold_[gate + 8]);
  }
#endif
}
#pragma endregion

#pragma region LD2410Srx

// appends one byte to rx buffer, and checks if that makes complete frame
RxEvaluationResult LD2410Srx::receive_byte(uint32_t loop_count, uint8_t byte) {
  if (this->payload_ready_) {
    this->reset();
  }

  this->rcv_buffer_[this->end_pos_] = byte;

  RxEvaluationResult result = this->evaluate_header_(this->end_pos_ + 1);
  if (result == RxEvaluationResult::OK) {
    result = this->evaluate_size_(this->end_pos_ + 1);
    if (result == RxEvaluationResult::OK) {
      result = this->evaluate_footer_(this->end_pos_ + 1);
    }
  }

  switch (result) {
    case RxEvaluationResult::OK:
      this->payload_ready_ = true;
      break;

    case RxEvaluationResult::UNKNOWN:
      this->end_pos_++;
      if (this->end_pos_ >= RX_TX_BUFFER_SIZE) {
        ESP_LOGVV(TAG, "XX< [loop:%" PRIu32 "] Received data buffer overflow, resetting", loop_count);
        this->reset();
      }
      break;

    case RxEvaluationResult::NOK:
    default:
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
      char hex_buf[format_hex_pretty_size(RX_TX_BUFFER_SIZE)];
      ESP_LOGVV(TAG, "<XX [loop:%" PRIu32 "] %s < %s", loop_count, this->msg_.c_str(),
                format_hex_pretty_to(hex_buf, this->rcv_buffer_, end_pos_ + 1, ' '));
#endif
      this->reset();
      result = RxEvaluationResult::UNKNOWN;
      break;
  }

  return result;
}
// checks if current rx buffer contains header
RxEvaluationResult LD2410Srx::evaluate_header_(uint16_t end_pos) {
  switch (this->frame_type_) {
    case RxFrameType::CMD_FRAME:
    case RxFrameType::STD_DATA_FRAME:
    case RxFrameType::SHORT_DATA_FRAME:
      return RxEvaluationResult::OK;  // already determined frame type

    case RxFrameType::NOK:
      return RxEvaluationResult::NOK;  // already determined bad header

    case RxFrameType::UNKNOWN:
    default:
      break;  // need to determine frame type
  }

  if (end_pos == sizeof(SHORT_DATA_FRAME_HEADER) &&
      memcmp(&this->rcv_buffer_[0], &SHORT_DATA_FRAME_HEADER, sizeof(SHORT_DATA_FRAME_HEADER)) == 0) {
    this->frame_type_ = RxFrameType::SHORT_DATA_FRAME;
    this->header_footer_size_ = sizeof(SHORT_DATA_FRAME_HEADER);
    return RxEvaluationResult::OK;
  }

  if (end_pos == sizeof(STD_DATA_FRAME_HEADER) &&
      memcmp(&this->rcv_buffer_[0], &STD_DATA_FRAME_HEADER, sizeof(STD_DATA_FRAME_HEADER)) == 0) {
    this->frame_type_ = RxFrameType::STD_DATA_FRAME;
    this->header_footer_size_ = sizeof(STD_DATA_FRAME_HEADER);
    return RxEvaluationResult::OK;
  }

  if (end_pos == sizeof(CMD_FRAME_HEADER) &&
      memcmp(&this->rcv_buffer_[0], &CMD_FRAME_HEADER, sizeof(CMD_FRAME_HEADER)) == 0) {
    this->frame_type_ = RxFrameType::CMD_FRAME;
    this->header_footer_size_ = sizeof(CMD_FRAME_HEADER);
    return RxEvaluationResult::OK;
  }

  if (end_pos < sizeof(STD_DATA_FRAME_HEADER) && memcmp(&this->rcv_buffer_[0], &STD_DATA_FRAME_HEADER, end_pos) == 0) {
    this->frame_type_ =
        RxFrameType::UNKNOWN;  // not enough data yet to determine frame type, but it fits STD frame header
    this->header_footer_size_ = 0;
    return RxEvaluationResult::UNKNOWN;
  }

  if (end_pos < sizeof(CMD_FRAME_HEADER) && memcmp(&this->rcv_buffer_[0], &CMD_FRAME_HEADER, end_pos) == 0) {
    this->frame_type_ =
        RxFrameType::UNKNOWN;  // not enough data yet to determine frame type, but it fits CMD frame header
    this->header_footer_size_ = 0;
    return RxEvaluationResult::UNKNOWN;
  }

  this->msg_ = "Unknown header";
  this->frame_type_ = RxFrameType::NOK;  // bad header
  return RxEvaluationResult::NOK;
}
// checks if current rx buffer has proper size for decoded header
RxEvaluationResult LD2410Srx::evaluate_size_(uint16_t end_pos) {
  switch (this->frame_type_) {
    case RxFrameType::SHORT_DATA_FRAME:
      if (this->expected_frame_size_ == 0) {
        this->size_field_size_ = 0;
        this->payload_size_ = 3;
        this->payload_pos_ = this->header_footer_size_;
        this->expected_frame_size_ = 2 * this->header_footer_size_ + 3;
      }
      break;

    case RxFrameType::STD_DATA_FRAME:
    case RxFrameType::CMD_FRAME:
      if (this->expected_frame_size_ == 0) {
        this->size_field_size_ = FRAME_DATA_LENGTH_SIZE;
        if (end_pos - 1 >= this->header_footer_size_ + this->size_field_size_) {
          this->payload_size_ = read_int(this->rcv_buffer_, this->header_footer_size_, 2);
          uint16_t overhead = 2 * this->header_footer_size_ + this->size_field_size_;
          if (this->payload_size_ > RX_TX_BUFFER_SIZE - overhead) {
            this->msg_ = "payload size exceeds buffer capacity";
            return RxEvaluationResult::NOK;
          }
          this->payload_pos_ = this->header_footer_size_ + this->size_field_size_;
          this->expected_frame_size_ = overhead + this->payload_size_;
        }
      }
      break;

    case RxFrameType::UNKNOWN:
      return RxEvaluationResult::UNKNOWN;  // not enough data yet to determine size
    case RxFrameType::NOK:                 // already determined bad header
    default:                               // unknown header type
      return RxEvaluationResult::NOK;
  }

  if (this->expected_frame_size_ == 0 || end_pos < this->expected_frame_size_) {
    return RxEvaluationResult::UNKNOWN;  // not enough data yet to determine size
  } else if (end_pos > this->expected_frame_size_) {
    char msg_buf[48];
    snprintf(msg_buf, sizeof(msg_buf), "rx passed the expected frame, expected:%" PRIu8, this->expected_frame_size_);
    this->msg_ = msg_buf;
    return RxEvaluationResult::NOK;  // passed the end of short data frame
  } else {
    return RxEvaluationResult::OK;  // correct size
  }
}
// checks if current rx buffer containts proper footer for decoded header
RxEvaluationResult LD2410Srx::evaluate_footer_(uint16_t end_pos) {
  switch (this->frame_type_) {
    case RxFrameType::SHORT_DATA_FRAME:  // footer matches expected for short data frame
      if (memcmp(&rcv_buffer_[end_pos - this->header_footer_size_], &SHORT_DATA_FRAME_FOOTER,
                 sizeof(SHORT_DATA_FRAME_FOOTER)) == 0) {
        return RxEvaluationResult::OK;
      }
      break;

    case RxFrameType::STD_DATA_FRAME:  // footer matches expected for standard data frame
      if (memcmp(&rcv_buffer_[end_pos - this->header_footer_size_], &STD_DATA_FRAME_FOOTER,
                 sizeof(STD_DATA_FRAME_FOOTER)) == 0) {
        return RxEvaluationResult::OK;
      }
      break;

    case RxFrameType::CMD_FRAME:  // footer matches expected for command frame
      if (memcmp(&rcv_buffer_[end_pos - this->header_footer_size_], &CMD_FRAME_FOOTER, sizeof(CMD_FRAME_FOOTER)) == 0) {
        return RxEvaluationResult::OK;
      }
      break;

    case RxFrameType::UNKNOWN:  // not enough data yet to determine size
      return RxEvaluationResult::UNKNOWN;
    case RxFrameType::NOK:  // already known bad data frame
    default:                // unknown header type
      break;
  }
  this->msg_ = "footer does not match header: ";
  return RxEvaluationResult::NOK;  // footer does not match expected footer for frame type
}
// reset rx buffer
void LD2410Srx::reset() {
  // ESP_LOGV(TAG, "rx reset");
  std::memset(this->rcv_buffer_, 0, RX_TX_BUFFER_SIZE);
  this->end_pos_ = 0;
  this->header_footer_size_ = 0;
  this->size_field_size_ = 0;
  this->frame_type_ = RxFrameType::UNKNOWN;
  this->payload_ready_ = false;
  this->payload_pos_ = 0;
  this->payload_size_ = 0;
  this->expected_frame_size_ = 0;
}

int LD2410Srx::read_int(const uint8_t *buffer, size_t pos, size_t len) {
  unsigned int ret = 0;
  int shift = 0;
  for (size_t i = 0; i < len; i++) {
    ret |= static_cast<unsigned int>(buffer[pos + i]) << shift;
    shift += 8;
  }
  return ret;
};

#pragma endregion

#pragma region LD2410Sschedule

// Appends new task to schedule
void LD2410Sschedule::append(uint16_t command, uint16_t sub_command) {
  if (this->last_ > 0) {
    ESP_LOGVV(TAG, "append => cmd:%04" PRIX16 ", prev_cmd:%04" PRIX16 ", active:%" PRIu8 ", last:%" PRIu8, command,
              this->commands_[this->last_ - 1].command, this->active_, this->last_);
  } else {
    ESP_LOGVV(TAG, "append => cmd:%04" PRIX16 ", prev_cmd:none, active:%" PRIu8 ", last:%" PRIu8, command,
              this->active_, this->last_);
  }

  if (this->last_ >= TX_SCHEDULE_BUFFER_SIZE) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
    ESP_LOGVV(TAG, "++: pos:[%" PRIu8 "], cmd:%04" PRIX16 ", Schedule buffer overflow, resetting buffer !!!",
              this->last_ - 1, command);
#else
    ESP_LOGW(TAG, "Schedule buffer overflow, resetting buffer !!!");
#endif
    this->reset_schedule();
    this->state_ = TxCmdState::FAILED;
    return;
  }

  if (command != CONFIG_MODE_START_CMD) {
    if (this->last_ <= 0) {
      ESP_LOGVV(TAG, "First cmd must be config start => appending config start and new cmd");
      this->append(CONFIG_MODE_START_CMD);
    } else {
      // if previous cmd is config end, it's not possible to just append new command
      if (this->commands_[this->last_ - 1].command == CONFIG_MODE_END_CMD) {
        if (command == CONFIG_MODE_END_CMD) {
          ESP_LOGVV(TAG, "Ignoring duplicated config end cmd");
          return;
        }

        if (this->active_ == this->last_ - 1) {
          ESP_LOGVV(TAG, "Previous cmd is config end and it's already executing => appending config start and new cmd");
          this->append(CONFIG_MODE_START_CMD);
        } else {
          ESP_LOGVV(TAG, "Last cmd was config end and it's not executing yet => deleting last config end and "
                         "appending new cmd");
          this->last_--;
        }
      }
    }
  }

  ESP_LOGV(TAG, "++: pos:[%" PRIu8 "], cmd:%04" PRIX16, this->last_, command);

  this->commands_[this->last_].command = command;
  this->commands_[this->last_].sub_command = sub_command;

  this->last_++;

  if (command != CONFIG_MODE_START_CMD && command != CONFIG_MODE_END_CMD) {
    ESP_LOGV(TAG, "Appending end");
    this->append(CONFIG_MODE_END_CMD);
  }

  if (this->state_ == TxCmdState::IDLE) {
    this->state_ = TxCmdState::SCHEDULED;
  }
}
// Returns active scheduled task status
TxCmdState LD2410Sschedule::check_state(uint32_t loop_count) {
  switch (this->state_) {
    case TxCmdState::SCHEDULED:
      this->retry_count_ = 0;
      ESP_LOGVV(TAG, "::> [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", Scheduled", loop_count,
                this->active_, this->last_ - 1, this->get_command());
      break;

    case TxCmdState::SEND:
      this->time_started_ = App.get_loop_component_start_time();
      ESP_LOGVV(TAG, "::> [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", Send", loop_count,
                this->active_, this->last_ - 1, this->get_command());
      break;

    case TxCmdState::SENT:
      if (App.get_loop_component_start_time() > this->time_started_ + TX_CONFIRMATION_TIMEOUT) {
        this->time_started_ = App.get_loop_component_start_time();

        if (this->retry_count_ < TX_MAX_RESEND) {
          ESP_LOGVV(TAG,
                    ":>> [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", retry:%" PRIu8
                    ", restart:%" PRIu8,
                    loop_count, this->active_, this->last_ - 1, this->get_command(), this->retry_count_,
                    this->restart_count_);
          ESP_LOGD(TAG, "Send Timeout Expired, Resend!");
          this->state_ = TxCmdState::SEND;
          this->retry_count_++;
        } else {
          if (this->restart_count_ < TX_MAX_RESTART) {
            ESP_LOGVV(TAG,
                      ":>> [loop:%" PRIu32 "] pos:%" PRIu8 "[:%" PRIu8 "], cmd:%04" PRIX16 ", retry:%" PRIu8
                      ", restart:%" PRIu8 ", Resend limit reached, Restart sequence!!",
                      loop_count, this->active_, this->last_ - 1, this->get_command(), this->retry_count_,
                      this->restart_count_);
            ESP_LOGD(TAG, "Resend limit reached, Restart sequence!!");
            this->state_ = TxCmdState::SCHEDULED;
            this->retry_count_ = 0;
            this->restart_count_++;
            this->active_ = 0;
          } else {
            ESP_LOGVV(TAG,
                      ":>> [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", retry:%" PRIu8
                      ", restart:%" PRIu8 ", Restart sequence limit reached, Giving up, "
                      "Reseting buffer!!!",
                      loop_count, this->active_, this->last_ - 1, this->get_command(), this->retry_count_,
                      this->restart_count_);
            ESP_LOGE(TAG, "Restart sequence limit reached, Giving up, Reseting buffer!!!");
            this->state_ = TxCmdState::FAILED;
            this->retry_count_ = 0;
            this->restart_count_ = 0;
            this->active_ = 0;
            this->last_ = 0;
          }
        }
      }
      break;

    case TxCmdState::IDLE:
      // schedule has passed the end
      if (this->last_ > 0 && !this->config_mode_ && this->active_ >= this->last_ - 1) {
        this->reset_schedule();
      }
      break;

    default:
      break;
  }

  return this->state_;
}
// Verifies if received response matches expected, if so procedes to next scheduled command
void LD2410Sschedule::verify_response(uint16_t command_word, uint32_t loop_count) {
  int16_t expected = this->get_command() | CMD_CONFIRMATION;
  if (this->state_ == TxCmdState::SENT) {
    if (command_word == expected) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
      ESP_LOGV(TAG, "Sent cmd: %04" PRIX16, this->get_command());
      ESP_LOGVV(
          TAG, "::< [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", Sending confirmed, rx:%04" PRIX16,
          loop_count, this->active_, this->last_ - 1, this->get_command(), command_word);
#endif

      switch (command_word) {
        // config start confirmed
        case CONFIG_MODE_START_CMD | CMD_CONFIRMATION:
          this->config_mode_ = true;
          break;

        // config end confirmed
        case CONFIG_MODE_END_CMD | CMD_CONFIRMATION:
          this->config_mode_ = false;
          break;

        default:
          break;
      }

      // procede to next task
      this->active_++;
      this->state_ = TxCmdState::SCHEDULED;

      if (this->active_ >= this->last_) {
        ESP_LOGV(TAG, "Schedule emptied, Resetting");
        this->reset_schedule();
      }

      if (this->active_ >= TX_SCHEDULE_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Schedule overflow, Reseting");
        this->reset_schedule();
      }
    } else {
      if (this->active_ > 0 && command_word == (this->commands_[this->active_ - 1].command | CMD_CONFIRMATION)) {
        ESP_LOGVV(TAG,
                  "::< [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", received:%04" PRIX16
                  ", Received unexpected confirmation for previous command",
                  loop_count, this->active_, this->last_, this->get_command(), command_word);
        ESP_LOGW(TAG, "Received unexpected confirmation for previous command");
      } else {
        ESP_LOGVV(TAG,
                  "::< [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", received:%04" PRIX16
                  ", Received confirmation for wrong command",
                  loop_count, this->active_, this->last_, this->get_command(), command_word);
        ESP_LOGW(TAG, "Received confirmation for wrong command");
      }
    }
  } else {
    ESP_LOGVV(TAG,
              "::< [loop:%" PRIu32 "] pos:%" PRIu8 "[%" PRIu8 "], cmd:%04" PRIX16 ", received:%04" PRIX16
              ", Received unexpected command confirmation",
              loop_count, this->active_, this->last_, this->get_command(), command_word);
    ESP_LOGW(TAG, "Received unexpected command confirmation");
  }
}

// Confirm frame ready for sending
void LD2410Sschedule::confirm_ready_to_send() { this->state_ = TxCmdState::SEND; }

// Confirm frame sent
void LD2410Sschedule::confirm_sent() {
  this->time_started_ = App.get_loop_component_start_time();
  this->state_ = TxCmdState::SENT;
  this->config_mode_ = true;
}

uint16_t LD2410Sschedule::get_command() { return this->commands_[this->active_].command; }
uint16_t LD2410Sschedule::get_sub_command() { return this->commands_[this->active_].sub_command; }
// Resets schedule buffer
void LD2410Sschedule::reset_schedule() {
  if (this->state_ == TxCmdState::IDLE)
    return;

  std::memset(this->commands_, 0x88, this->last_ * sizeof(TxTaskT));
  this->last_ = 0;
  this->active_ = 0;
  this->time_started_ = App.get_loop_component_start_time();
  this->retry_count_ = 0;
  this->restart_count_ = 0;
  this->state_ = TxCmdState::IDLE;
  ESP_LOGV(TAG, "::: Schedule cleared");
}

#pragma endregion

}  // namespace esphome::ld2410s