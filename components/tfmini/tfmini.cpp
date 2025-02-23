#include "tfmini.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace tfmini
  {
    static const char *const TAG = "tfmini";

    static const char *model_to_str(TFminiModel model)
    {
      switch (model)
      {
      case TFminiModel::TFMINI_MODEL_TFMINI_S:
        return "TFmini-S";
      case TFminiModel::TFMINI_MODEL_TFMINI_PLUS:
        return "TFmini Plus";
      case TFminiModel::TFMINI_MODEL_TF_LUNA:
        return "TF-Luna";
      default:
        return "";
      }
    }

    void TFminiComponent::setup()
    {
      uint64_t start_time = millis();
      uint16_t attempts = 0;

      ESP_LOGCONFIG(TAG, "Setting up TFmini...");

      // CONFIG must be high for UART comms and low for I2C comms
      // applies to TF Luna model only
      if (this->config_pin_ != nullptr)
      {
        this->config_pin_->setup();
        this->config_pin_->digital_write(true);
        delay(10);
      }

      this->send_command_(TFMINI_CMD_SOFT_RESET); // send first Soft Reset Command
      ESP_LOGV(TAG, "Send first Soft Reset Command");

      while (!this->soft_reset_received_)
      {
        process_rx_data();
        if ((millis() - this->cmd_time_) > 50)
        {
          this->send_command_(TFMINI_CMD_SOFT_RESET); // send next Soft Reset Command
          ESP_LOGV(TAG, "Send repeat Soft Reset Command");
          attempts++;
        }
        if (attempts > 20)
        {
          ESP_LOGE(TAG, "No response from %s", LOG_STR_ARG(model_to_str(this->model_)));
          this->mark_failed();
          return;
        }
      }
      if (this->low_power_)
      {
        this->sample_rate_received_ = true;        // bypass sending Sample Rate Command
        this->send_command_(TFMINI_CMD_LOW_POWER); // send first Low Power Command
      }
      else
      {
        this->low_power_received_ = true; // bypass sending Low Power Command
        if (this->sample_rate_ == 100)
        {
          this->sample_rate_received_ = true; // no need to send default Sample Rate Command
        }
        else
        {
          this->send_command_(TFMINI_CMD_SAMPLE_RATE); // send first Set Sample Rate Command
        }
      }

      this->send_command_(TFMINI_CMD_FW_VERSION); // resend Get Firmware Version Command

      attempts = 0;

      while (!this->low_power_received_ || !this->sample_rate_received_ || !this->version_received_)
      {
        process_rx_data();
        if ((millis() - this->cmd_time_) > 100)
        {
          // timeout, send command again
          if (!this->low_power_received_)
          {
            this->send_command_(TFMINI_CMD_LOW_POWER); // resend Low Power Command
          }
          if (!this->sample_rate_received_)
          {
            this->send_command_(TFMINI_CMD_SAMPLE_RATE); // resend Sample Rate Command
          }
          if (!this->version_received_)
          {
            this->send_command_(TFMINI_CMD_FW_VERSION); // resend Get Firmware Version Command
          }
          if (++attempts >= 40)
          {
            // ESP_LOGE(TAG, "Low Power=%s, Sample Rate=%s, Version=%s", this->low_power_received_ ? "True" : "False", this->sample_rate_received_ ? "True" : "False", this->version_received_ ? "True" : "False");
            ESP_LOGE(TAG, "No response from %s", LOG_STR_ARG(model_to_str(this->model_)));
            // this->mark_failed();
            break;
          }
        }
      }
      // received responses from all commands
      this->setup_ = true;
      this->config_elapse_ = (millis() - start_time) / 1000.0;
      ESP_LOGV(TAG, "Config completed in %0.3f sec", this->config_elapse_);
    }

    void TFminiComponent::loop()
    {
      if (this->is_failed())
      {
        return;
      }
      process_rx_data(); // get data from UART
    }

    void TFminiComponent::dump_config()
    {
      ESP_LOGCONFIG(TAG, "TFmini:");
      ESP_LOGCONFIG(TAG, "  Model: %s", LOG_STR_ARG(model_to_str(this->model_)));
      LOG_PIN("  CONFIG Pin: ", this->config_pin_);
      LOG_PIN("  MULTI Pin: ", this->multi_pin_);
      ESP_LOGCONFIG(TAG, "  Sample Rate: %u", this->sample_rate_);
      if (this->model_ != TFMINI_MODEL_TFMINI_PLUS)
        ESP_LOGCONFIG(TAG, "  Low Power Mode: %s", this->low_power_ ? "True" : "False");
      LOG_SENSOR("  ", "Distance:", this->distance_sensor_);
      LOG_SENSOR("  ", "Signal Strength:", this->signal_strength_sensor_);
      LOG_SENSOR("  ", "Temperature:", this->temperature_sensor_);
      LOG_TEXT_SENSOR("  ", "Version:", this->version_sensor_);
      ESP_LOGCONFIG(TAG, "  Config time: %0.3f sec", this->config_elapse_);
    }

    void TFminiComponent::process_rx_data()
    {
      uint8_t data;
      static uint8_t packet_length = 0;
      uint64_t start_time = millis();

      // get data from UART
      while ((this->available() > 0) && (millis() - start_time < 29))
      {
        this->read_byte(&data);
        rx_buffer_.push_back(data);
        if (this->rx_buffer_.size() == 1)
        {
          // looking for the beginning of a frame
          if ((data != 0x59) && (data != 0x5A))
          {
            this->rx_buffer_.clear();
          }
        }
        else if ((this->rx_buffer_.size() == 2))
        {
          // second part of data frame header
          if ((this->rx_buffer_[0] == 0x59) && (this->rx_buffer_[1] == 0x59))
          {
            // valid start of data frame detected
            packet_length = 9;
          }
          else if (this->rx_buffer_[0] == 0x5A)
          {
            // start of response frame detected, second byte is packet_length
            packet_length = this->rx_buffer_[1];
            // packet_length only has a few valid values
            if ((packet_length < 5) || (packet_length > 8))
            {
              // invalid frame header, clear the buffer
              if (this->setup_)
              {
                ESP_LOGD(TAG, "Invalid Response Frame Header, packet length is out of range");
              }
              this->rx_buffer_.clear();
            }
            else
            {
              // valid response frame header
              if (this->setup_)
              {
                ESP_LOGV(TAG, "Start Response Frame Header received");
              }
            }
          }
          else if ((this->rx_buffer_[0] == 0x59) && (this->rx_buffer_[1] != 0x59))
          {
            // invalid start of data frame
            if (this->setup_)
            {
              ESP_LOGD(TAG, "Invalid Data Frame Header");
            }
            this->rx_buffer_.clear();
          }
          else
          {
            // still looking for the start of frame header
            this->rx_buffer_.clear();
          }
        }
        else
        {
          // we are looking for the rest of a frame
          if (this->rx_buffer_[0] == 0x5A)
          {
            // we processing a response frame
            if (this->rx_buffer_.size() == packet_length)
            {
              ESP_LOGV(TAG, "Processing Response Frame");
              this->process_response_frame();
              this->rx_buffer_.clear();
            }
          }
          else
          {
            // processing a 9-byte data frame
            if (this->rx_buffer_.size() == packet_length)
            {
              // received a 9-byte data frame
              if (this->setup_)
              {
                ESP_LOGV(TAG, "Processing Data Frame");
                this->process_data_frame();
              }
              // good or bad response, we are done, start over
              this->rx_buffer_.clear();
            }
          }
        }
      }
    }

    void TFminiComponent::process_data_frame()
    {
      int16_t distance;
      uint16_t strength, temperature;

      if (this->verify_rx_packet_checksum_())
      {
        // checksum is good, grab the three results as little endian 16-bit numbers
        distance = (this->rx_buffer_[3] << 8) + this->rx_buffer_[2];
        strength = (this->rx_buffer_[5] << 8) + this->rx_buffer_[4];
        temperature = (this->rx_buffer_[7] << 8) + this->rx_buffer_[6];

        if (strength < 100)
        {
          // not enough signal, most likely open air
          distance = 10000; // 100 meters, out of range
          // ++badReadings;
        }
        else if ((strength == 65535) || (distance == -4))
        {
          // too much signal
          distance = 0; // 0 meters, unable to determine range
        }
        if (this->distance_sensor_ != nullptr)
          this->distance_sensor_->publish_state(float(distance));

        if (this->signal_strength_sensor_ != nullptr)
        {
          this->signal_strength_sensor_->publish_state(float(strength));
        }
        if (this->temperature_sensor_ != nullptr)
        {
          this->temperature_sensor_->publish_state((float(temperature) / 8.0) - 256.0);
        }
      }
    }

    void TFminiComponent::process_response_frame()
    {
      uint16_t value;

      if (this->rx_buffer_[2] == TFMINI_CMD_FW_VERSION)
      {
        if (this->verify_rx_packet_checksum_())
        {
          std::string version = 'v' + std::to_string(this->rx_buffer_[5]) + '.' +
                                std::to_string(this->rx_buffer_[4]) + '.' +
                                std::to_string(this->rx_buffer_[3]);
          // checksums match
          if (this->version_sensor_ != nullptr)
          {
            this->version_sensor_->publish_state(version);
          }
          this->version_received_ = true;
          ESP_LOGV(TAG, "Received Firmware Version Response Frame, Version = %s", version.c_str());
        }
      }
      else if (this->rx_buffer_[2] == TFMINI_CMD_SOFT_RESET)
      {
        if (this->verify_rx_packet_checksum_())
        {
          // checksums match
          this->soft_reset_received_ = true;
          ESP_LOGV(TAG, "Received Soft Reset Response Frame, Soft Reset was %s", this->rx_buffer_[3] == 0 ? "successful" : "failed");
        }
      }
      else if (this->rx_buffer_[2] == TFMINI_CMD_SAMPLE_RATE)
      {
        if (this->verify_rx_packet_checksum_())
        {
          // checksums match
          value = (this->rx_buffer_[4] << 8) + this->rx_buffer_[3];
          if (this->sample_rate_ == value)
          {
            this->sample_rate_received_ = true;
            ESP_LOGV(TAG, "Received Sample Rate Response Frame, Sample Rate received matches set rate");
          }
          else
          {
            ESP_LOGD(TAG, "Received Sample Rate Response Frame, Sample Rate received (%u) does NOT match set rate (%u)", value, this->sample_rate_);
          }
        }
      }
      else if (this->rx_buffer_[2] == TFMINI_CMD_LOW_POWER)
      {
        if (this->verify_rx_packet_checksum_())
        {
          // checksums match
          value = this->low_power_;
          if (this->sample_rate_ > 10)
          {
            value = 10;
          }
          if (!this->low_power_)
          {
            value = 0;
          }
          if (this->rx_buffer_[3] == value)
          {
            this->low_power_received_ = true;
            ESP_LOGV(TAG, "Received Low Power Response Frame, Low Power Sample Rate received matches set rate");
          }
          else
          {
            ESP_LOGD(TAG, "Received Low Power Response Frame, Low Power Sample Rate received (%u) does NOT match set rate (%u)", this->rx_buffer_[3], value);
          }
        }
      }
      else
      {
        // not a valid response command, clear the buffer
        ESP_LOGD(TAG, "Invalid Response Frame Command 0x%02X", this->rx_buffer_[2]);
      }
    }

    void TFminiComponent::send_command_(uint8_t cmd)
    {
      uint16_t value;

      this->tx_buffer_.clear();
      switch (cmd)
      {
      case TFMINI_CMD_FW_VERSION:
        this->tx_buffer_.push_back(0x5A); // Commands always starts with 0x5A
        this->tx_buffer_.push_back(0x04); // This command is 4 bytes long
        this->tx_buffer_.push_back(TFMINI_CMD_FW_VERSION);
        this->comp_cs_send_command_();
        this->version_received_ = false;
        ESP_LOGV(TAG, "Sent Get Firmware Version command. %s", this->tx_packet_to_str_().c_str());
        break;

      case TFMINI_CMD_SOFT_RESET:
        this->tx_buffer_.push_back(0x5A); // Commands always starts with 0x5A
        this->tx_buffer_.push_back(0x04); // This command is 4 bytes long
        this->tx_buffer_.push_back(TFMINI_CMD_SOFT_RESET);
        this->comp_cs_send_command_();
        this->soft_reset_received_ = false;
        ESP_LOGV(TAG, "Sent Soft Reset command. %s", this->tx_packet_to_str_().c_str());
        break;

      case TFMINI_CMD_SAMPLE_RATE:
        value = this->sample_rate_;
        this->tx_buffer_.push_back(0x5A); // Commands always starts with 0x5A
        this->tx_buffer_.push_back(0x06); // This command is 6 bytes long
        this->tx_buffer_.push_back(TFMINI_CMD_SAMPLE_RATE);
        this->tx_buffer_.push_back(value & 0xFF);        // Sample Rate low byte
        this->tx_buffer_.push_back((value >> 8) & 0xFF); // Sample Rate high byte
        this->comp_cs_send_command_();
        this->sample_rate_received_ = false;
        ESP_LOGV(TAG, "Sent Set Sample Rate command. Sample Rate = %u. %s", value, this->tx_packet_to_str_().c_str());
        break;

      case TFMINI_CMD_TRIGGER:
        this->tx_buffer_.push_back(0x5A); // Commands always starts with 0x5A
        this->tx_buffer_.push_back(0x04); // This command is 4 bytes long
        this->tx_buffer_.push_back(TFMINI_CMD_TRIGGER);
        this->comp_cs_send_command_();
        ESP_LOGV(TAG, "Sent Trigger Detection command. %s", this->tx_packet_to_str_().c_str());
        break;

      case TFMINI_CMD_LOW_POWER:
        value = this->sample_rate_;
        if (this->sample_rate_ > 10)
          value = 10;
        if (!this->low_power_)
          value = 0;
        this->tx_buffer_.push_back(0x5A); // Commands always starts with 0x5A
        this->tx_buffer_.push_back(0x06); // This command is 6 bytes long
        this->tx_buffer_.push_back(TFMINI_CMD_LOW_POWER);
        this->tx_buffer_.push_back(value); // Sample Rate low byte
        this->tx_buffer_.push_back(0x00);  // always 0x00
        this->comp_cs_send_command_();
        this->low_power_received_ = false;
        ESP_LOGV(TAG, "Sent Low Power command. Low Power Mode = %s. %s",
                 this->low_power_ ? "Enabled" : "Disabled", this->tx_packet_to_str_().c_str());
        break;
      }
      this->cmd_time_ = millis();
    }

    void TFminiComponent::comp_cs_send_command_()
    {
      uint8_t checksum = 0;
      uint8_t length = this->tx_buffer_.size();

      for (uint8_t i = 0; i < length; i++)
      {
        checksum += this->tx_buffer_[i];
        this->write_byte(this->tx_buffer_[i]);
      }
      this->tx_buffer_.push_back(checksum);
      this->write_byte(checksum);
    }

    bool TFminiComponent::verify_rx_packet_checksum_()
    {
      uint8_t length = this->rx_buffer_.size();
      uint8_t checksum = 0;

      for (uint8_t i = 0; i < (length - 1); i++)
      {
        checksum += this->rx_buffer_[i];
      }
      if (checksum == this->rx_buffer_[length - 1])
      {
        return true;
      }
      ESP_LOGD(TAG, "%s Frame checksum verification failed, computed checksum = %02X, %s", this->rx_buffer_[0] == 0x59 ? "Data" : "Response", checksum, this->rx_packet_to_str_().c_str());
      return false;
    }

    std::string TFminiComponent::rx_packet_to_str_()
    {
      uint8_t length = this->rx_buffer_.size();
      std::string packet_str = "Packet <- ";

      for (uint8_t i = 0; i < length - 1; i++)
      {
        packet_str += format_hex(this->rx_buffer_[i]) + ' ';
      }
      packet_str += format_hex(this->rx_buffer_[length - 1]);
      return packet_str;
    }

    std::string TFminiComponent::tx_packet_to_str_()
    {
      uint8_t length = this->tx_buffer_.size();
      std::string packet_str = "Packet -> ";

      for (uint8_t i = 0; i < length - 1; i++)
      {
        packet_str += format_hex(this->tx_buffer_[i]) + ' ';
      }
      packet_str += format_hex(this->tx_buffer_[length - 1]);
      return packet_str;
    }

  } // namespace tfmini
} // namespace esphome
