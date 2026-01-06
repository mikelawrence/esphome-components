#include "tfmini.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tfmini {
static const char *const TAG = "tfmini";

enum TFminiCmd {
  CMD_FW_VERSION = 0x01,
  CMD_SOFT_RESET = 0x02,
  CMD_SAMPLE_RATE = 0x03,
  CMD_TRIGGER = 0x04,
  CMD_OUTPUT_FORMAT = 0x05,
  CMD_BAUD_RATE = 0x06,
  CMD_OUTPUT_CONTROL = 0x07,
  CMD_HARD_RESET = 0x10,
  CMD_SAVE_SETTINGS = 0x11,
  CMD_LOW_POWER = 0x35,
};

static const char *model_to_str(TFminiModel model) {
  switch (model) {
    case MODEL_TFMINI_S:
      return "TFmini-S";
    case MODEL_TFMINI_PLUS:
      return "TFmini Plus";
    case MODEL_TF_LUNA:
      return "TF-Luna";
    default:
      return "Unknown";
  }
}

void TFminiComponent::setup() {
  this->rx_buffer_.init(10);
  this->tx_buffer_.init(10);
  // CONFIG must be high for UART comms and low for I2C comms, TF Luna model only
  if (this->config_pin_ != nullptr) {
    this->config_pin_->setup();
    this->config_pin_->digital_write(true);
  }
  this->set_timeout(500, [this]() { this->internal_setup_(TFMINI_SM_START); });
}

void TFminiComponent::internal_setup_(SetupStates state) {
  switch (state) {
    case TFMINI_SM_START:
      this->cmd_attempts_ = 0;
      if (this->low_power_) {
        this->send_command_(CMD_LOW_POWER);  // send first Low Power Command
        this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_LOW_POWER); });
      } else {
        this->send_command_(CMD_SAMPLE_RATE);  // send first Set Sample Rate Command
        this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_SAMPLE_RATE); });
      }
      break;
    case TFMINI_SM_LOW_POWER:
      if (!this->low_power_received_) {
        if (++this->cmd_attempts_ < 5) {
          this->cmd_attempts_++;
          this->send_command_(CMD_LOW_POWER);  // resend Low Power Command
          ESP_LOGW(TAG, "Resending Low Power Command");
          this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_LOW_POWER); });
        } else {
          // no response, we are in error
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          break;
        }
      } else {
        this->cmd_attempts_ = 0;
        this->send_command_(CMD_SAMPLE_RATE);  // send first Set Sample Rate Command
        this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_SAMPLE_RATE); });
      }
      break;
    case TFMINI_SM_SAMPLE_RATE:
      if (!this->sample_rate_received_) {
        if (++this->cmd_attempts_ < 5) {
          this->cmd_attempts_++;
          this->send_command_(CMD_SAMPLE_RATE);  // resend Sample Rate Command
          ESP_LOGW(TAG, "Resending Sample Rate Command");
          this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_SAMPLE_RATE); });
        } else {
          // no response, we are in error
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          break;
        }
      } else {
        this->cmd_attempts_ = 0;
        this->send_command_(CMD_FW_VERSION);  // send First Get Firmware Version Command
        this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_FW_VERSION); });
      }
      break;
    case TFMINI_SM_FW_VERSION:
      if (!this->version_received_) {
        if (++this->cmd_attempts_ < 5) {
          this->cmd_attempts_++;
          this->send_command_(CMD_FW_VERSION);  // resend Get Firmware Command
          ESP_LOGW(TAG, "Resending Sample Rate Command");
          this->set_timeout(100, [this]() { this->internal_setup_(TFMINI_SM_FW_VERSION); });
        } else {
          // no response, we are in error
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          break;
        }
      } else {
        this->internal_setup_(TFMINI_SM_DONE);
      }
      break;
    case TFMINI_SM_DONE:
      this->initialized_ = true;
      break;
  }
}

void TFminiComponent::loop() {
  process_rx_data();  // get data from UART
}

void TFminiComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "TFmini:\n"
                "  Model: %s\n"
                "  Firmware Version: %s\n"
                "  Sample Rate: %u\n"
                "  Low Power Mode: %s\n",
                LOG_STR_ARG(model_to_str(this->model_)), this->firmware_version_.c_str(), this->sample_rate_,
                TRUEFALSE(this->low_power_));
  LOG_PIN("  CONFIG Pin: ", this->config_pin_);
  LOG_SENSOR("  ", "Distance:", this->distance_sensor_);
  LOG_SENSOR("  ", "Signal Strength:", this->signal_strength_sensor_);
  LOG_SENSOR("  ", "Temperature:", this->temperature_sensor_);
}

void TFminiComponent::process_rx_data() {
  uint8_t data;
  static uint8_t packet_length = 0;
  uint64_t start_time = millis();

  // get data from UART
  while ((this->available() > 0) && (millis() - start_time < 29)) {
    this->read_byte(&data);
    rx_buffer_.push_back(data);
    if (this->rx_buffer_.size() == 1) {
      // looking for the beginning of a frame
      if ((data != 0x59) && (data != 0x5A)) {
        this->rx_buffer_.clear();
      }
    } else if ((this->rx_buffer_.size() == 2)) {
      // second part of data frame header
      if ((this->rx_buffer_[0] == 0x59) && (this->rx_buffer_[1] == 0x59)) {
        // valid start of data frame detected
        packet_length = 9;
      } else if (this->rx_buffer_[0] == 0x5A) {
        // start of response frame detected, second byte is packet_length
        packet_length = this->rx_buffer_[1];
        // packet_length only has a few valid values
        if ((packet_length < 5) || (packet_length > 8)) {
          // invalid frame header, clear the buffer
          if (this->initialized_) {
            ESP_LOGD(TAG, ESP_LOG_MSG_COMM_FAIL);
          }
          this->rx_buffer_.clear();
        } else {
          // valid response frame header
          if (this->initialized_) {
            ESP_LOGV(TAG, "Start Response Frame Header received");
          }
        }
      } else if ((this->rx_buffer_[0] == 0x59) && (this->rx_buffer_[1] != 0x59)) {
        // invalid start of data frame
        if (this->initialized_) {
          ESP_LOGD(TAG, ESP_LOG_MSG_COMM_FAIL);
        }
        this->rx_buffer_.clear();
      } else {
        // still looking for the start of frame header
        this->rx_buffer_.clear();
      }
    } else {
      // we are looking for the rest of a frame
      if (this->rx_buffer_[0] == 0x5A) {
        // we processing a response frame
        if (this->rx_buffer_.size() == packet_length) {
          ESP_LOGV(TAG, "Processing Response Frame");
          this->process_response_frame();
          this->rx_buffer_.clear();
        }
      } else {
        // processing a 9-byte data frame
        if (this->rx_buffer_.size() == packet_length) {
          // received a 9-byte data frame
          if (this->initialized_) {
            ESP_LOGV(TAG, "Processing Data Frame");
            this->process_data_frame();
          }
          this->rx_buffer_.clear();  // good or bad response, we are done, start over
        }
      }
    }
  }
}

void TFminiComponent::process_data_frame() {
  float distance, strength, temperature;

  if (this->verify_rx_packet_checksum_()) {
    // checksum is good, results are little endian 16-bit numbers
    distance = convert_little_endian(*reinterpret_cast<uint16_t *>(&this->rx_buffer_[2]));
    strength = convert_little_endian(*reinterpret_cast<uint16_t *>(&this->rx_buffer_[4]));
    if ((strength < 100) || (strength == 65535) || (distance == -4)) {
      distance = NAN;  // distance is invalid under these conditions
    }
    this->distance_sensor_->publish_state(static_cast<float>(distance));
    if (this->signal_strength_sensor_ != nullptr) {
      this->signal_strength_sensor_->publish_state(static_cast<float>(strength));
    }
    if (this->temperature_sensor_ != nullptr) {
      temperature = convert_little_endian(*reinterpret_cast<uint16_t *>(&this->rx_buffer_[6]));
      this->temperature_sensor_->publish_state((static_cast<float>(temperature) / 8.0) - 256.0);
    }
  }
}

void TFminiComponent::process_response_frame() {
  uint16_t value;

  if (this->rx_buffer_[2] == CMD_FW_VERSION) {
    if (this->verify_rx_packet_checksum_()) {
      this->firmware_version_ = 'v' + std::to_string(this->rx_buffer_[5]) + '.' + std::to_string(this->rx_buffer_[4]) +
                                '.' + std::to_string(this->rx_buffer_[3]);
      this->version_received_ = true;
      ESP_LOGV(TAG, "Received Firmware Version Response Frame, Version = %s", this->firmware_version_.c_str());
    }
  } else if (this->rx_buffer_[2] == CMD_SOFT_RESET) {
    if (this->verify_rx_packet_checksum_()) {
      this->soft_reset_received_ = true;
      ESP_LOGV(TAG, "Received Soft Reset Response Frame, Soft Reset was %s",
               this->rx_buffer_[3] == 0 ? "successful" : "failed");
    }
  } else if (this->rx_buffer_[2] == CMD_SAMPLE_RATE) {
    if (this->verify_rx_packet_checksum_()) {
      value = (this->rx_buffer_[4] << 8) + this->rx_buffer_[3];
      if (this->sample_rate_ == value) {
        this->sample_rate_received_ = true;
        ESP_LOGV(TAG, "Received Sample Rate Response Frame, match");
      } else {
        ESP_LOGE(TAG, "Command Response Error", value);
      }
    }
  } else if (this->rx_buffer_[2] == CMD_LOW_POWER) {
    if (this->verify_rx_packet_checksum_()) {
      value = this->low_power_;
      if (this->sample_rate_ > 10) {
        value = 10;
      }
      if (!this->low_power_) {
        value = 0;
      }
      if (this->rx_buffer_[3] == value) {
        this->low_power_received_ = true;
        ESP_LOGV(TAG, "Received Low Power Response Frame, match");
      } else {
        ESP_LOGD(TAG, "Command Response Error");
      }
    }
  } else {
    ESP_LOGD(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
}

void TFminiComponent::send_command_(uint8_t cmd) {
  uint16_t value;

  this->tx_buffer_.clear();
  switch (cmd) {
    case CMD_FW_VERSION:
      this->tx_buffer_.push_back(0x5A);  // Commands always starts with 0x5A
      this->tx_buffer_.push_back(0x04);  // This command is 4 bytes long
      this->tx_buffer_.push_back(CMD_FW_VERSION);
      this->comp_cs_send_command_();
      this->version_received_ = false;
      ESP_LOGV(TAG, "Sent Get Firmware Version command.");
      break;

    // case CMD_SOFT_RESET:
    //   this->tx_buffer_.push_back(0x5A);  // Commands always starts with 0x5A
    //   this->tx_buffer_.push_back(0x04);  // This command is 4 bytes long
    //   this->tx_buffer_.push_back(CMD_SOFT_RESET);
    //   this->comp_cs_send_command_();
    //   this->soft_reset_received_ = false;
    //   ESP_LOGV(TAG, "Sent Soft Reset command.");
    //   break;

    case CMD_SAMPLE_RATE:
      value = this->sample_rate_;
      this->tx_buffer_.push_back(0x5A);  // Commands always starts with 0x5A
      this->tx_buffer_.push_back(0x06);  // This command is 6 bytes long
      this->tx_buffer_.push_back(CMD_SAMPLE_RATE);
      this->tx_buffer_.push_back(value & 0xFF);         // Sample Rate low byte
      this->tx_buffer_.push_back((value >> 8) & 0xFF);  // Sample Rate high byte
      this->comp_cs_send_command_();
      this->sample_rate_received_ = false;
      ESP_LOGV(TAG, "Sent Set Sample Rate command. Sample Rate = %u.", value);
      break;

    // case CMD_TRIGGER:
    //   this->tx_buffer_.push_back(0x5A);  // Commands always starts with 0x5A
    //   this->tx_buffer_.push_back(0x04);  // This command is 4 bytes long
    //   this->tx_buffer_.push_back(CMD_TRIGGER);
    //   this->comp_cs_send_command_();
    //   ESP_LOGV(TAG, "Sent Trigger Detection command.");
    //   break;

    case CMD_LOW_POWER:
      value = this->sample_rate_;
      if (this->sample_rate_ > 10)
        value = 10;
      if (!this->low_power_)
        value = 0;
      this->tx_buffer_.push_back(0x5A);  // Commands always starts with 0x5A
      this->tx_buffer_.push_back(0x06);  // This command is 6 bytes long
      this->tx_buffer_.push_back(CMD_LOW_POWER);
      this->tx_buffer_.push_back(value);  // Sample Rate low byte
      this->tx_buffer_.push_back(0x00);   // always 0x00
      this->comp_cs_send_command_();
      this->low_power_received_ = false;
      ESP_LOGV(TAG, "Sent Low Power command. Low Power Mode = %s.", this->low_power_ ? "Enabled" : "Disabled");
      break;
  }
}

void TFminiComponent::comp_cs_send_command_() {
  uint8_t checksum = 0;
  uint8_t length = this->tx_buffer_.size();

  for (uint8_t i = 0; i < length; i++) {
    checksum += this->tx_buffer_[i];
    this->write_byte(this->tx_buffer_[i]);
  }
  this->tx_buffer_.push_back(checksum);
  this->write_byte(checksum);
}

bool TFminiComponent::verify_rx_packet_checksum_() {
  uint8_t length = this->rx_buffer_.size();
  uint8_t checksum = 0;

  for (uint8_t i = 0; i < (length - 1); i++) {
    checksum += this->rx_buffer_[i];
  }
  if (checksum == this->rx_buffer_[length - 1]) {
    return true;
  }
  ESP_LOGD(TAG, ESP_LOG_MSG_COMM_FAIL);
  return false;
}

}  // namespace tfmini
}  // namespace esphome
