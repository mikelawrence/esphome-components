#include "dfrobot_c4001.h"
#include "commands.h"

#include <sstream>

namespace esphome {
namespace dfrobot_c4001 {
static const char *const TAG = "dfrobot_c4001.commands";

uint8_t Command::execute(DFRobotC4001Hub *parent) {
  this->parent_ = parent;
  // send command is separate from the rest of the state machine
  if (this->state_ == STATE_CMD_SEND) {
    this->done_ = false;
    this->error_ = false;
    if (this->parent_->send_cmd_(this->cmd_.c_str(), this->cmd_duration_ms_)) {
      this->state_ = STATE_WAIT_ECHO;
    }
    if (this->cmd_ == "resetSystem") {
      // resetSystem command only returns prompt, bypass central part of state machine
      std::string message = "";
      on_message(message);
      ESP_LOGV(TAG, "Send Cmd: Shortcutting Reset System Command");
      this->state_ = STATE_WAIT_PROMPT;
    }
    return false;
  }
  // surround state machine with a successful read message
  if (this->parent_->read_message_()) {
    std::string message(this->parent_->read_buffer_);
    switch (this->state_) {
      case STATE_WAIT_ECHO:
        if (str_startswith(message, this->cmd_)) {
          this->state_ = STATE_PROCESS;
        }
        break;
      case STATE_PROCESS:
        if (str_startswith(message, "Error")) {
          this->error_ = true;
        }
        if (str_startswith(message, "sensor is not stopped")) {
          ESP_LOGE(TAG, "Sensor is not stopped");
        }
        if (!this->error_) {
          on_message(message);
        }
        if (this->error_ || this->done_) {
          this->state_ = STATE_WAIT_PROMPT;
        }
        break;
      case STATE_WAIT_PROMPT:
        if (str_startswith(message, "DFRobot:/>")) {
          if (this->error_) {
            if (this->retries_left_ > 0) {
              this->retries_left_ -= 1;
              ESP_LOGE(TAG, "Command Error Retrying");
              this->state_ = STATE_CMD_SEND;
              this->error_count_ -= 1;
              return false;  // command not done
            } else {
              ESP_LOGE(TAG, "Command Failure: %s", this->cmd_.c_str());
              this->state_ = STATE_DONE;
              this->error_count_ -= 1;
              // Command done
              return this->error_count_ < 0 ? this->error_count_ : true;
            }
          } else {
            ESP_LOGV(TAG, "Send Cmd: Complete");
            // Command done
            return this->error_count_ < 0 ? this->error_count_ : true;
          }
        }
        break;
      default:  // STATE_DONE
        // Command done
        return this->error_count_ < 0 ? this->error_count_ : true;
    }
  }
  // check for timeout
  if (millis() - this->parent_->ts_last_cmd_sent_ > this->timeout_ms_) {
    if (this->retries_left_ > 0) {
      this->retries_left_ -= 1;
      ESP_LOGE(TAG, "Command Retrying");
      this->state_ = STATE_CMD_SEND;
      this->error_count_ -= 1;
      return false;  // command not done
    } else {
      ESP_LOGE(TAG, "Command Failure: %s", this->cmd_.c_str());
      this->state_ = STATE_DONE;
      this->error_count_ -= 1;
      // Command done
      return this->error_count_ < 0 ? this->error_count_ : true;
    }
  }
  return false;
}

uint8_t ReadStateCommand::execute(DFRobotC4001Hub *parent) {
  this->parent_ = parent;
  if (this->parent_->read_message_()) {
    std::string message(this->parent_->read_buffer_);
    if (str_startswith(message, "$DFHPD,0, , , *")) {
      this->parent_->set_occupancy(false);
      this->parent_->set_enable(true);
      ESP_LOGV(TAG, "Recv Rpt: Occupancy Clear");
      return 1;  // Command done
    } else if (str_startswith(message, "$DFHPD,1, , , *")) {
      this->parent_->set_occupancy(true);
      this->parent_->set_enable(true);
      ESP_LOGV(TAG, "Recv Rpt: Occupancy Detected");
      return 1;  // Command done
    } else if (str_startswith(message, "$DFDMD")) {
      std::string str1, str2, str3, str4, str5;
      std::replace(message.begin(), message.end(), ',', ' ');
      std::stringstream s(message);
      s >> str1 >> str1 >> str2 >> str3 >> str4 >> str5;
      auto target = parse_number<int>(str1);
      auto target_distance = parse_number<float>(str3);
      auto target_speed = parse_number<float>(str4);
      auto target_energy = parse_number<float>(str5);
      if (target.has_value() && target == 1) {
        // one target is detected
        if (target_distance.has_value() && target_speed.has_value() && target_energy.has_value()) {
          // 1 target is detected, that is all this sensor can do
          this->parent_->set_target_distance(target_distance.value());
          this->parent_->set_target_speed(target_speed.value());
          this->parent_->set_target_energy(target_energy.value());
          this->parent_->set_occupancy(true);
          this->parent_->set_enable(true);
          ESP_LOGV(TAG, "Recv Rpt: Target Detected, Dist=%.3f, Speed=%.3f, Energy=%d", target_distance.value(),
                   target_speed.value(), (uint) target_energy.value());
          return 1;  // command completed successfully
        } else {
          ESP_LOGE(TAG, "Error parsing Distance and Speed Detection Output");
          return 1;  // command done with error
        }
      } else if (target.has_value() && target == 0) {
        // no target is detected
        this->parent_->set_target_distance(0.0);
        this->parent_->set_target_speed(0.0);
        this->parent_->set_target_energy(0.0);
        this->parent_->set_occupancy(false);
        this->parent_->set_enable(true);
        ESP_LOGV(TAG, "Recv Rpt: No Target");
        return 1;  // command completed successfully
      }
    }
  }
  if (millis() - this->parent_->ts_last_cmd_sent_ > this->timeout_ms_) {
    return 1;  // Command done, timeout
  }
  return 0;  // Command not done yet.
}

void ReadStateCommand::on_message(std::string &message) { return; }

PowerCommand::PowerCommand(bool power_on) : power_on_(power_on) {
  if (power_on) {
    this->cmd_ = "sensorStart";
  } else {
    this->cmd_ = "sensorStop";
  }
};

void PowerCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->parent_->set_enable(this->power_on_);
    this->done_ = true;  // command is done
  }
}

GetRangeCommand::GetRangeCommand() { this->cmd_ = "getRange"; }

void GetRangeCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1 >> str2;
    auto min = parse_number<float>(str1);
    auto max = parse_number<float>(str2);
    if (min.has_value() && max.has_value()) {
      this->min_range_ = round(min.value() * 100.0) / 100.0;
      this->max_range_ = round(max.value() * 100.0) / 100.0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_min_range(this->min_range_, false);
    this->parent_->set_max_range(this->max_range_, false);
    this->done_ = true;  // command is done
  }
}

SetRangeCommand::SetRangeCommand(float min_range, float max_range) {
  this->cmd_ = str_sprintf("setRange %.3f %.3f", min_range, max_range);
};

void SetRangeCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetTrigRangeCommand::GetTrigRangeCommand() { this->cmd_ = "getTrigRange"; }

void GetTrigRangeCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1;
    auto trig = parse_number<float>(str1);
    if (trig.has_value()) {
      this->trigger_range_ = round(trig.value() * 100.0) / 100.0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_trigger_range(this->trigger_range_, false);
    this->done_ = true;  // command is done
  }
}

SetTrigRangeCommand::SetTrigRangeCommand(float trigger_range) {
  this->trigger_range_ = trigger_range;
  this->cmd_ = str_sprintf("setTrigRange %.3f", this->trigger_range_);
};

void SetTrigRangeCommand::on_message(std::string &message) {
  if (message == "Done") {
    ESP_LOGV(TAG, "Send Cmd: Complete");
    this->done_ = true;  // command is done
  }
}

GetSensitivityCommand::GetSensitivityCommand() { this->cmd_ = "getSensitivity"; }

void GetSensitivityCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1 >> str2;
    auto hold = parse_number<uint16_t>(str1);
    auto trig = parse_number<uint16_t>(str2);
    if (hold.has_value() && trig.has_value()) {
      this->hold_sensitivity_ = hold.value();
      this->trigger_sensitivity_ = trig.value();
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_hold_sensitivity(this->hold_sensitivity_, false);
    this->parent_->set_trigger_sensitivity(this->trigger_sensitivity_, false);
    this->done_ = true;  // command is done
  }
}

SetSensitivityCommand::SetSensitivityCommand(uint16_t hold_sensitivity, uint16_t trigger_sensitivity) {
  this->hold_sensitivity_ = hold_sensitivity;
  this->trigger_sensitivity_ = trigger_sensitivity;
  this->cmd_ = str_sprintf("setSensitivity %d %d", this->hold_sensitivity_, this->trigger_sensitivity_);
};

void SetSensitivityCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetLatencyCommand::GetLatencyCommand() { this->cmd_ = "getLatency"; }

void GetLatencyCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1 >> str2;
    auto on = parse_number<float>(str1);
    auto off = parse_number<float>(str2);
    if (on.has_value() && off.has_value()) {
      this->on_latency_ = round(on.value() * 100.0) / 100.0;
      this->off_latency_ = round(off.value() * 100.0) / 100.0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_on_latency(this->on_latency_, false);
    this->parent_->set_off_latency(this->off_latency_, false);
    this->done_ = true;  // command is done
  }
}

SetLatencyCommand::SetLatencyCommand(float on_latency, float off_latency) {
  this->on_latency_ = on_latency;
  this->off_latency_ = off_latency;
  this->cmd_ = str_sprintf("setLatency %.3f %.3f", this->on_latency_, this->off_latency_);
};

void SetLatencyCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetInhibitTimeCommand::GetInhibitTimeCommand() { this->cmd_ = "getInhibit"; }

void GetInhibitTimeCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1;
    auto inhibit = parse_number<float>(str1);
    if (inhibit.has_value()) {
      this->inhibit_time_ = round(inhibit.value() * 100.0) / 100.0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_inhibit_time(this->inhibit_time_, false);
    this->done_ = true;  // command is done
  }
}

SetInhibitTimeCommand::SetInhibitTimeCommand(float inhibit) {
  this->inhibit_time_ = inhibit;
  this->cmd_ = str_sprintf("setInhibit %.3f", this->inhibit_time_);
};

void SetInhibitTimeCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetThrFactorCommand::GetThrFactorCommand() { this->cmd_ = "getThrFactor"; }

void GetThrFactorCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1;
    auto thr = parse_number<float>(str1);
    if (thr.has_value()) {
      this->threshold_factor_ = round(thr.value());
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_threshold_factor(this->threshold_factor_, false);
    this->done_ = true;  // command is done
  }
}

SetThrFactorCommand::SetThrFactorCommand(float threshold_factor) {
  this->threshold_factor_ = threshold_factor;
  this->cmd_ = str_sprintf("setThrFactor %.0f", this->threshold_factor_);
};

void SetThrFactorCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetUartOutputCommand::GetUartOutputCommand() { this->cmd_ = "getUartOutput 1"; }

void GetUartOutputCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1 >> str1;
    auto enable = parse_number<uint8_t>(str1);
    if (enable.has_value()) {
      this->uart_output_enable_ = enable.value();
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

SetUartOutputCommand::SetUartOutputCommand(bool enable) : uart_output_enable_(enable) {
  if (enable) {
    this->cmd_ = "setUartOutput 1 1 1 1.0";
  } else {
    this->cmd_ = "setUartOutput 1 0 1 1.0";
  }
};

void SetUartOutputCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

SetLedModeCommand1::SetLedModeCommand1(bool led_mode) : led_enable_(led_mode) {
  if (led_mode) {
    this->cmd_ = "setLedMode 1 0";
  } else {
    this->cmd_ = "setLedMode 1 1";
  }
};

void SetLedModeCommand1::on_message(std::string &message) {
  // this command is not in Communication Protocol document it appears to be a leftover from similar products
  // this command only controls the green LED which flashes when the sensor is running the blue LED is always on when
  // powered
  if (message == "Done") {
    this->parent_->set_led_enable(this->led_enable_, false);
    this->done_ = true;  // command is done
  }
}

SetLedModeCommand2::SetLedModeCommand2(bool led_mode) : led_enable_(led_mode) {
  if (led_mode) {
    this->cmd_ = "setLedMode 2 0";
  } else {
    this->cmd_ = "setLedMode 2 1";
  }
};

void SetLedModeCommand2::on_message(std::string &message) {
  // this command is not in Communication Protocol document it appears to be a leftover from similar products
  // this command only controls the green LED which flashes when the sensor is running the blue LED is always on when
  // powered
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetMicroMotionCommand::GetMicroMotionCommand() { this->cmd_ = "getMicroMotion"; }

void GetMicroMotionCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (str_startswith(message, "Response")) {
    s >> str1 >> str1;
    auto led = parse_number<uint8_t>(str1);
    if (led.has_value()) {
      this->micro_motion_ = led.value();
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      this->error_ = true;  // command is done
    }
  } else if (message == "Done") {
    this->parent_->set_micro_motion_enable(this->micro_motion_, false);
    this->done_ = true;  // command is done
  }
}

SetMicroMotionCommand::SetMicroMotionCommand(bool enable) : micro_motion_(enable) {
  if (enable) {
    this->cmd_ = "setMicroMotion 1";
  } else {
    this->cmd_ = "setMicroMotion 0";
  }
};

void SetMicroMotionCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->parent_->set_micro_motion_enable(this->micro_motion_, false);
    this->done_ = true;  // command is done
  }
}

FactoryResetCommand::FactoryResetCommand() { this->cmd_ = "resetCfg"; }

void FactoryResetCommand::on_message(std::string &message) {
  if (message == "Done") {
    // reload settings
    this->parent_->set_led_enable(true, false);
    this->parent_->flash_led_enable();
    this->parent_->config_load();
    this->done_ = true;  // command is done
  }
}

ResetSystemCommand::ResetSystemCommand() { this->cmd_ = "resetSystem"; }

void ResetSystemCommand::on_message(std::string &message) {
  this->parent_->config_load();
  this->done_ = true;  // command is done
}

SaveCfgCommand::SaveCfgCommand() { this->cmd_ = "saveConfig"; }

void SaveCfgCommand::on_message(std::string &message) {
  if (message == "no parameter has changed") {
    ESP_LOGV(TAG, "Send Cmd: Complete: Nothing Changed");
  } else if (message == "Done") {
    this->parent_->config_load();
    this->done_ = true;  // command is done
  }
}

SetRunAppCommand::SetRunAppCommand(uint8_t mode) : mode_(mode) {
  if (mode == MODE_PRESENCE) {
    this->cmd_ = "setRunApp 0";
  } else {
    this->cmd_ = "setRunApp 1";
  }
}

void SetRunAppCommand::on_message(std::string &message) {
  if (message == "Done") {
    this->done_ = true;  // command is done
  }
}

GetSWVCommand::GetSWVCommand() { this->cmd_ = "getSWV"; }

void GetSWVCommand::on_message(std::string &message) {
  if (str_startswith(message, "SoftwareVersion:")) {
    std::string str1;
    std::replace(message.begin(), message.end(), ':', ' ');
    std::stringstream s(message);
    s >> str1 >> str1;
    this->version_ = str1;
  } else if (message == "Done") {
    this->parent_->set_software_version(this->version_);
    this->done_ = true;  // command is done
  }
}

GetHWVCommand::GetHWVCommand() { this->cmd_ = "getHWV"; }

void GetHWVCommand::on_message(std::string &message) {
  if (str_startswith(message, "HardwareVersion:")) {
    std::string str1;
    std::replace(message.begin(), message.end(), ':', ' ');
    std::stringstream s(message);
    s >> str1 >> str1;
    this->version_ = str1;
  } else if (message == "Done") {
    this->parent_->set_hardware_version(this->version_);
    this->done_ = true;  // command is done
  }
}

}  // namespace dfrobot_c4001
}  // namespace esphome
