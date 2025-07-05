#include "commands.h"

#include <cmath>
#include <sstream>

#include "esphome/core/log.h"

#include "dfrobot_c4001.h"

namespace esphome {
namespace dfrobot_c4001 {
static const char *const TAG = "dfrobot_c4001.commands";

uint8_t Command::execute(DFRobotC4001Hub *parent) {
  this->parent_ = parent;
  if (this->cmd_sent_) {
    if (this->parent_->read_message_()) {
      std::string message(this->parent_->read_buffer_);
      if (message.rfind("Error") != std::string::npos) {
        ESP_LOGD(TAG, "Command not recognized");
        if (this->retries_left_ > 0) {
          this->retries_left_ -= 1;
          this->cmd_sent_ = false;
          ESP_LOGD(TAG, "Retrying (%s)...", this->cmd_.c_str());
          return 0;
        } else {
          this->parent_->find_prompt_();
          return 1;  // Command done
        }
      }
      uint8_t rc = on_message(message);
      if (rc == 0) {
        // command is not done yet
        return 0;
      } else {
        // command is done
        this->parent_->find_prompt_();
        return 1;
      }
    }
    if (millis() - this->parent_->ts_last_cmd_sent_ > this->timeout_ms_) {
      if (this->retries_left_ > 0) {
        this->retries_left_ -= 1;
        this->cmd_sent_ = false;
        ESP_LOGD(TAG, "Command timeout, retrying (%s)...", this->cmd_.c_str());
      } else {
        ESP_LOGD(TAG, "Command failed (%s)", this->cmd_.c_str());
        return 1;  // Command done
      }
    }
  } else if (this->parent_->send_cmd_(this->cmd_.c_str(), this->cmd_duration_ms_)) {
    this->cmd_sent_ = true;
  }
  return 0;  // Command not done yet
}

uint8_t ReadStateCommand::execute(DFRobotC4001Hub *parent) {
  this->parent_ = parent;
  if (this->parent_->read_message_()) {
    std::string message(this->parent_->read_buffer_);
    if (message.rfind("$DFHPD,0, , , *") != std::string::npos) {
      this->parent_->set_occupancy(false);
      this->parent_->set_enable(true);
      ESP_LOGV(TAG, "Recv Rpt: Occupancy Detected");
      return 1;  // Command done
    } else if (message.rfind("$DFHPD,1, , , *") != std::string::npos) {
      this->parent_->set_occupancy(true);
      this->parent_->set_enable(true);
      ESP_LOGV(TAG, "Recv Rpt: Occupancy Clear");
      return 1;  // Command done
    } else if (message.starts_with("$DFDMD")) {
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

uint8_t ReadStateCommand::on_message(std::string &message) { return 1; }

PowerCommand::PowerCommand(bool power_on) : power_on_(power_on) {
  if (power_on) {
    cmd_ = "sensorStart";
  } else {
    cmd_ = "sensorStop";
  }
};

uint8_t PowerCommand::on_message(std::string &message) {
  if (message == "sensor stopped already") {
    this->parent_->set_enable(false);
    ESP_LOGV(TAG, "Stopped sensor");
    return 1;  // Command done
  } else if (message == "sensor started already") {
    this->parent_->set_enable(true);
    ESP_LOGV(TAG, "Started sensor");
    return 1;  // Command done
  } else if (message == "Done") {
    if (this->power_on_) {
      this->parent_->set_enable(true);
      ESP_LOGV(TAG, "Started sensor");
    } else {
      this->parent_->set_enable(false);
      ESP_LOGV(TAG, "Stopped sensor");
    }
    return 1;  // Command done
  }
  return 0;  // Command not done yet.
}

GetRangeCommand::GetRangeCommand() { cmd_ = "getRange"; }

uint8_t GetRangeCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1 >> str2;
    auto min = parse_number<float>(str1);
    auto max = parse_number<float>(str2);
    if (min.has_value() && max.has_value()) {
      this->min_range_ = round(min.value() * 100.0) / 100.0;
      this->max_range_ = round(max.value() * 100.0) / 100.0;
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_min_range(this->min_range_, false);
    this->parent_->set_max_range(this->max_range_, false);
    ESP_LOGV(TAG, "Get Range complete: Parsed Min Range (%.3f) and Max Range (%.3f)", this->min_range_,
             this->max_range_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetRangeCommand::SetRangeCommand(float min_range, float max_range) {
  this->cmd_ = str_sprintf("setRange %.3f %.3f", min_range, max_range);
};

uint8_t SetRangeCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Range complete.");
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

GetTrigRangeCommand::GetTrigRangeCommand() { cmd_ = "getTrigRange"; }

uint8_t GetTrigRangeCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1;
    auto trig = parse_number<float>(str1);
    if (trig.has_value()) {
      this->trigger_range_ = round(trig.value() * 100.0) / 100.0;
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_trigger_range(this->trigger_range_, false);
    ESP_LOGV(TAG, "Get Trigger Range complete: Parsed Trigger Range Range (%.3f)", this->trigger_range_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetTrigRangeCommand::SetTrigRangeCommand(float trigger_range) {
  this->trigger_range_ = clamp(trigger_range, 0.6f, 25.0f);
  this->cmd_ = str_sprintf("setTrigRange %.3f", this->trigger_range_);
};

uint8_t SetTrigRangeCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Trigger Range complete.");
    return 1;  // Command done
  } else {
    return 0;  // Command not done yet
  }
}

GetSensitivityCommand::GetSensitivityCommand() { cmd_ = "getSensitivity"; }

uint8_t GetSensitivityCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1 >> str2;
    auto hold = parse_number<uint16_t>(str1);
    auto trig = parse_number<uint16_t>(str2);
    if (hold.has_value() && trig.has_value()) {
      this->hold_sensitivity_ = hold.value();
      this->trigger_sensitivity_ = trig.value();
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_hold_sensitivity(this->hold_sensitivity_, false);
    this->parent_->set_trigger_sensitivity(this->trigger_sensitivity_, false);
    ESP_LOGV(TAG, "Get Sensitivity complete: Parsed Hold Sensitivity (%.3f) and Trigger Sensitivity (%.3f).",
             this->hold_sensitivity_, this->trigger_sensitivity_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetSensitivityCommand::SetSensitivityCommand(uint16_t hold_sensitivity, uint16_t trigger_sensitivity) {
  this->hold_sensitivity_ = clamp(hold_sensitivity, uint16_t(0), uint16_t(9));
  this->trigger_sensitivity_ = clamp(trigger_sensitivity, uint16_t(0), uint16_t(9));
  this->cmd_ = str_sprintf("setSensitivity %d %d", this->hold_sensitivity_, this->trigger_sensitivity_);
};

uint8_t SetSensitivityCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Sensitivity complete.");
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

GetLatencyCommand::GetLatencyCommand() { cmd_ = "getLatency"; }

uint8_t GetLatencyCommand::on_message(std::string &message) {
  std::string str1, str2;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1 >> str2;
    auto on = parse_number<float>(str1);
    auto off = parse_number<float>(str2);
    if (on.has_value() && off.has_value()) {
      this->on_latency_ = round(on.value() * 100.0) / 100.0;
      this->off_latency_ = round(off.value() * 100.0) / 100.0;
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_on_latency(this->on_latency_, false);
    this->parent_->set_off_latency(this->off_latency_, false);
    ESP_LOGV(TAG, "Get Latency complete: Parsed On Latency (%.3f) and Off Latency (%.3f)", this->on_latency_,
             this->off_latency_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetLatencyCommand::SetLatencyCommand(float on_latency, float off_latency) {
  this->on_latency_ = clamp(on_latency, 0.0f, 100.0f);
  this->off_latency_ = clamp(off_latency, 0.5f, 1500.0f);
  this->cmd_ = str_sprintf("setLatency %.3f %.3f", this->on_latency_, this->off_latency_);
};

uint8_t SetLatencyCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Latency complete.");
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

GetInhibitTimeCommand::GetInhibitTimeCommand() { cmd_ = "getInhibit"; }

uint8_t GetInhibitTimeCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1;
    auto inhibit = parse_number<float>(str1);
    if (inhibit.has_value()) {
      this->inhibit_time_ = round(inhibit.value() * 100.0) / 100.0;
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_inhibit_time(this->inhibit_time_, false);
    ESP_LOGV(TAG, "Get Inhibit Time complete: Parsed Inhibit Time (%.3f)", this->inhibit_time_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetInhibitTimeCommand::SetInhibitTimeCommand(float inhibit) {
  this->inhibit_time_ = clamp(inhibit, 0.6f, 25.0f);
  this->cmd_ = str_sprintf("setInhibit %.3f", this->inhibit_time_);
};

uint8_t SetInhibitTimeCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Inhibit Time complete.");
    return 1;  // Command done
  } else {
    return 0;  // Command not done yet
  }
}

GetThrFactorCommand::GetThrFactorCommand() { cmd_ = "getThrFactor"; }

uint8_t GetThrFactorCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1;
    auto thr = parse_number<float>(str1);
    if (thr.has_value()) {
      this->threshold_factor_ = round(thr.value());
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_threshold_factor(this->threshold_factor_, false);
    ESP_LOGV(TAG, "Get Threshold Factor complete: Parsed Threshold Factor (%.0f).", this->threshold_factor_);
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetThrFactorCommand::SetThrFactorCommand(float threshold_factor) {
  this->threshold_factor_ = threshold_factor;
  this->cmd_ = str_sprintf("setThrFactor %.0f", this->threshold_factor_);
};

uint8_t SetThrFactorCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Threshold Factor complete.");
    return 1;  // Command done
  } else {
    return 0;  // Command not done yet
  }
}

SetLedModeCommand::SetLedModeCommand(bool led_mode) : led_enable_(led_mode) {
  if (led_mode) {
    cmd_ = "setLedMode 1 0";
  } else {
    cmd_ = "setLedMode 1 1";
  }
};

uint8_t SetLedModeCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    if (this->led_enable_) {
      this->parent_->set_led_enable(true, false);
    } else {
      this->parent_->set_led_enable(false, false);
    }
    ESP_LOGV(TAG, "Set LED Mode complete");
    // we save the state of LED enable to flash because you cannot read this value from the module
    this->parent_->flash_led_enable();
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

GetMicroMotionCommand::GetMicroMotionCommand() { cmd_ = "getMicroMotion"; }

uint8_t GetMicroMotionCommand::on_message(std::string &message) {
  std::string str1;
  std::stringstream s(message);

  if (message.starts_with("Response")) {
    s >> str1 >> str1;
    auto led = parse_number<uint8_t>(str1);
    if (led.has_value()) {
      this->micro_motion_ = led.value();
      return 0;
    } else {
      ESP_LOGE(TAG, "Failed to parse response");
      return 1;  // Command done
    }
  } else if (message == "Done") {
    this->parent_->set_micro_motion_enable(this->micro_motion_, false);
    ESP_LOGV(TAG, "Get Micro Motion complete: Parsed Micro Motion (%s).", this->micro_motion_ ? "Enabled" : "Disabled");
    return 1;  // Command done
  } else {
    return 0;
  }
}

SetMicroMotionCommand::SetMicroMotionCommand(bool enable) : micro_motion_(enable) {
  if (enable) {
    cmd_ = "setMicroMotion 1";
  } else {
    cmd_ = "setMicroMotion 0";
  }
};

uint8_t SetMicroMotionCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    this->parent_->set_micro_motion_enable(this->micro_motion_, false);
    ESP_LOGV(TAG, "Set Micro Motion complete.");
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

FactoryResetCommand::FactoryResetCommand() { cmd_ = "resetCfg"; }

uint8_t FactoryResetCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Sensor factory reset done.");
    // reload settings
    this->parent_->set_led_enable(true, false);
    this->parent_->flash_led_enable();
    this->parent_->config_load();
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

ResetSystemCommand::ResetSystemCommand() { cmd_ = "resetSystem"; }

uint8_t ResetSystemCommand::on_message(std::string &message) {
  if (message == "leapMMW:/>") {
    ESP_LOGV(TAG, "Restarted sensor.");
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

SaveCfgCommand::SaveCfgCommand() { cmd_ = "saveConfig"; }

uint8_t SaveCfgCommand::on_message(std::string &message) {
  if (message == "no parameter has changed") {
    ESP_LOGV(TAG, "Not saving config (no parameter changed).");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Saved config. Saving a lot may damage the sensor.");
    // reload settings because they might have changed
    this->parent_->config_load();
    return 1;  // Command done
  }
  return 0;  // Command not done yet
}

SetRunAppCommand::SetRunAppCommand(uint8_t mode) : mode_(mode) {
  if (mode == 0) {
    cmd_ = "setRunApp 0";
  } else {
    cmd_ = "setRunApp 1";
  }
}

uint8_t SetRunAppCommand::on_message(std::string &message) {
  if (message == "sensor is not stopped") {
    ESP_LOGE(TAG, "Sensor is not stopped");
    return 1;  // Command done
  } else if (message == "Done") {
    ESP_LOGV(TAG, "Set Mode complete.");
    return 1;  // Command done
  } else {
    return 0;  // Command not done yet
  }
}

GetSWVCommand::GetSWVCommand() { cmd_ = "getSWV"; }

uint8_t GetSWVCommand::on_message(std::string &message) {
  if (message.starts_with("SoftwareVersion:")) {
    this->version_ = message.substr(16);
    return 0;
  } else if (message == "Done") {
    this->parent_->set_sw_version(this->version_);
    ESP_LOGV(TAG, "Get SW Version complete: Parsed Version (%s).", this->version_.c_str());
    return 1;  // Command done
  } else {
    return 0;
  }
}

GetHWVCommand::GetHWVCommand() { cmd_ = "getHWV"; }

uint8_t GetHWVCommand::on_message(std::string &message) {
  if (message.starts_with("HardwareVersion:")) {
    this->version_ = message.substr(16);
    return 0;
  } else if (message == "Done") {
    this->parent_->set_hw_version(this->version_);
    ESP_LOGV(TAG, "Get HW Version complete: Parsed Version (%s).", this->version_.c_str());
    return 1;  // Command done
  } else {
    return 0;
  }
}

}  // namespace dfrobot_c4001
}  // namespace esphome
