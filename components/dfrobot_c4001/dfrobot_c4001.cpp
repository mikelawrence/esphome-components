#include "dfrobot_c4001.h"

#include <cmath>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace dfrobot_c4001 {
static const char *const TAG = "dfrobot_c4001";
const char ASCII_CR = 0x0D;
const char ASCII_LF = 0x0A;

void DFRobotC4001Hub::set_occupancy(bool occupancy) {
  this->occupancy_ = occupancy;
#ifdef USE_BINARY_SENSOR
  if (this->occupancy_binary_sensor_ != nullptr) {
    this->occupancy_binary_sensor_->publish_state(occupancy);
  }
#endif
}

void DFRobotC4001Hub::set_target_distance(float value) {
  this->target_distance_ = value;
#ifdef USE_SENSOR
  if (this->target_distance_sensor_ != nullptr) {
    this->target_distance_sensor_->publish_state(value);
  }
#endif
}

void DFRobotC4001Hub::set_target_speed(float value) {
  this->target_speed_ = value;
#ifdef USE_SENSOR
  if (this->target_speed_sensor_ != nullptr) {
    this->target_speed_sensor_->publish_state(value);
  }
#endif
}

void DFRobotC4001Hub::set_target_energy(float value) {
  this->target_energy_ = value;
#ifdef USE_SENSOR
  if (this->target_energy_sensor_ != nullptr) {
    this->target_energy_sensor_->publish_state(value);
  }
#endif
}

void DFRobotC4001Hub::set_max_range(float max, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
#ifdef USE_NUMBER
    if (this->max_range_number_ != nullptr)
      this->max_range_number_->publish_state(max);
#endif
    this->max_range_ = max;
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_min_range(float min, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
#ifdef USE_NUMBER
    if (this->min_range_number_ != nullptr) {
      this->min_range_number_->publish_state(min);
    }
#endif
    this->min_range_ = min;
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}
void DFRobotC4001Hub::set_trigger_range(float trig, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
#ifdef USE_NUMBER
    if (this->trigger_range_number_ != nullptr)
      this->trigger_range_number_->publish_state(trig);
#endif
    this->trigger_range_ = trig;
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_hold_sensitivity(float value, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
    this->hold_sensitivity_ = value;
#ifdef USE_NUMBER
    if (this->hold_sensitivity_number_ != nullptr)
      this->hold_sensitivity_number_->publish_state(value);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_trigger_sensitivity(float value, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
    this->trigger_sensitivity_ = value;
#ifdef USE_NUMBER
    if (this->trigger_sensitivity_number_ != nullptr)
      this->trigger_sensitivity_number_->publish_state(value);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_on_latency(float value, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
    this->on_latency_ = value;
#ifdef USE_NUMBER
    if (this->on_latency_number_ != nullptr)
      this->on_latency_number_->publish_state(value);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_off_latency(float value, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
    this->off_latency_ = value;
#ifdef USE_NUMBER
    if (this->off_latency_number_ != nullptr)
      this->off_latency_number_->publish_state(value);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_inhibit_time(float value, bool needs_save) {
  if (this->mode_ == MODE_PRESENCE) {
    this->inhibit_time_ = value;
#ifdef USE_NUMBER
    if (this->inhibit_time_number_ != nullptr)
      this->inhibit_time_number_->publish_state(value);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_threshold_factor(float value, bool needs_save) {
  if (this->mode_ == MODE_SPEED_AND_DISTANCE) {
    this->threshold_factor_ = value;
#ifdef USE_NUMBER
    if (this->threshold_factor_number_ != nullptr)
      this->threshold_factor_number_->publish_state(this->threshold_factor_);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_led_enable(bool value, bool needs_save) {
  this->led_enable_ = value;
#ifdef USE_SWITCH
  if (this->led_enable_switch_ != nullptr)
    this->led_enable_switch_->publish_state(value);
#endif
  if (needs_save) {
    this->set_needs_save(true);
  }
}

void DFRobotC4001Hub::set_micro_motion_enable(bool enable, bool needs_save) {
  if (this->mode_ == MODE_SPEED_AND_DISTANCE) {
    this->micro_motion_enable_ = enable;
#ifdef USE_SWITCH
    if (this->micro_motion_enable_switch_ != nullptr)
      this->micro_motion_enable_switch_->publish_state(enable);
#endif
    if (needs_save) {
      this->set_needs_save(true);
    }
  }
}

void DFRobotC4001Hub::set_software_version(const std::string &version) {
#ifdef USE_TEXT_SENSOR
  if (this->software_version_text_sensor_ != nullptr)
    this->software_version_text_sensor_->publish_state(version);
#endif
  this->sw_version_ = version;
}

void DFRobotC4001Hub::set_hardware_version(const std::string &version) {
#ifdef USE_TEXT_SENSOR
  if (this->hardware_version_text_sensor_ != nullptr)
    this->hardware_version_text_sensor_->publish_state(version);
#endif
  this->hw_version_ = version;
}

void DFRobotC4001Hub::flash_led_enable() {
#ifdef USE_SWITCH
  // Save LED Enable preferences (to flash storage)
  if (this->led_enable_switch_ != nullptr) {
    ESP_LOGV(TAG, "Writing LED Enable setting to flash.");
    this->pref_.save(&this->led_enable_);
  }
#endif
}

void DFRobotC4001Hub::set_mode(ModeConfig value) { this->mode_ = value; }

void DFRobotC4001Hub::set_needs_save(bool needs_save) {
  this->needs_save_ = needs_save;
#ifdef USE_BINARY_SENSOR
  if (this->config_changed_binary_sensor_ != nullptr) {
    this->config_changed_binary_sensor_->publish_state(needs_save);
  }
#endif
}

void DFRobotC4001Hub::config_load() {
  // set dfrobot_c4001 hub configuration
  cmd_queue_.enqueue(make_unique<PowerCommand>(false));
  cmd_queue_.enqueue(make_unique<SetUartOutputCommand>());
  cmd_queue_.enqueue(make_unique<GetLedModeCommand1>());
  // have to be in the right mode to read that mode's parameters
  cmd_queue_.enqueue(make_unique<SetRunAppCommand>(this->mode_));
  cmd_queue_.enqueue(make_unique<PowerCommand>(true));
  cmd_queue_.enqueue(make_unique<GetHWVCommand>());
  cmd_queue_.enqueue(make_unique<GetSWVCommand>());
  // Read current parameters
  if (this->mode_ == MODE_PRESENCE) {
    cmd_queue_.enqueue(make_unique<GetRangeCommand>());
    cmd_queue_.enqueue(make_unique<GetTrigRangeCommand>());
    cmd_queue_.enqueue(make_unique<GetSensitivityCommand>());
    cmd_queue_.enqueue(make_unique<GetLatencyCommand>());
    cmd_queue_.enqueue(make_unique<GetInhibitTimeCommand>());
  } else {
    cmd_queue_.enqueue(make_unique<GetMicroMotionCommand>());
    cmd_queue_.enqueue(make_unique<GetThrFactorCommand>());
  }

  this->set_needs_save(false);
}

void DFRobotC4001Hub::config_save() {
  if (this->needs_save_) {
    this->flash_led_enable();
    this->enqueue(make_unique<PowerCommand>(false));
    if (this->mode_ == MODE_PRESENCE) {
      this->enqueue(make_unique<SetRangeCommand>(this->min_range_, max_range_));
      this->enqueue(make_unique<SetTrigRangeCommand>(this->trigger_range_));
      this->enqueue(make_unique<SetSensitivityCommand>(this->hold_sensitivity_, this->trigger_sensitivity_));
      this->enqueue(make_unique<SetLatencyCommand>(this->on_latency_, this->off_latency_));
      this->enqueue(make_unique<SetInhibitTimeCommand>(this->inhibit_time_));
      this->enqueue(make_unique<SetLedModeCommand1>(this->led_enable_));
      this->enqueue(make_unique<SetLedModeCommand2>(this->led_enable_));
    } else {
      this->enqueue(make_unique<SetThrFactorCommand>(this->threshold_factor_));
      this->enqueue(make_unique<SetMicroMotionCommand>(this->micro_motion_enable_));
      this->enqueue(make_unique<SetLedModeCommand1>(this->led_enable_));
      this->enqueue(make_unique<SetLedModeCommand2>(this->led_enable_));
    }
    this->enqueue(make_unique<SaveCfgCommand>());
    this->enqueue(make_unique<PowerCommand>(true));
    this->set_needs_save(false);
  }
}

void DFRobotC4001Hub::factory_reset() {
  ESP_LOGD(TAG, "Factory Reset Started");
  this->enqueue(make_unique<PowerCommand>(false));
  this->enqueue(make_unique<FactoryResetCommand>());
}

void DFRobotC4001Hub::restart() {
  ESP_LOGD(TAG, "Restart Started");
  this->enqueue(make_unique<PowerCommand>(false));
  this->enqueue(make_unique<ResetSystemCommand>());
}

void DFRobotC4001Hub::dump_config() {
  ESP_LOGCONFIG(TAG,
                "DFRobot C4001 mmWave Radar:\n"
                "  SW Version: %s\n"
                "  HW Version: %s\n"
                "  Mode: %s\n",
                this->sw_version_.c_str(), this->hw_version_.c_str(),
                this->mode_ == MODE_PRESENCE ? "PRESENCE" : "SPEED_AND_DISTANCE");
#ifdef USE_BUTTON
  ESP_LOGCONFIG(TAG, "Buttons:");
  LOG_BUTTON("  ", "Config Save", this->config_save_button_);
  LOG_BUTTON("  ", "Restart", this->restart_button_);
  LOG_BUTTON("  ", "Factory Reset", this->factory_reset_button_);
#endif
#ifdef USE_BINARY_SENSOR
  ESP_LOGCONFIG(TAG, "Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Occupancy", this->occupancy_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Config Changed", this->config_changed_binary_sensor_);
#endif
#ifdef USE_NUMBER
  ESP_LOGCONFIG(TAG, "Numbers:");
  LOG_NUMBER("  ", "Maximum Range", this->max_range_number_);
  LOG_NUMBER("  ", "Minimum Range", this->min_range_number_);
  LOG_NUMBER("  ", "Trigger Range", this->trigger_range_number_);
  LOG_NUMBER("  ", "Hold Sensitivity", this->hold_sensitivity_number_);
  LOG_NUMBER("  ", "Trigger Sensitivity", this->trigger_sensitivity_number_);
  LOG_NUMBER("  ", "On Latency", this->on_latency_number_);
  LOG_NUMBER("  ", "Off Latency", this->off_latency_number_);
  LOG_NUMBER("  ", "Inhibit Time", this->inhibit_time_number_);
  LOG_NUMBER("  ", "Threshold Factor", this->threshold_factor_number_);
#endif
#ifdef USE_SWITCH
  ESP_LOGCONFIG(TAG, "Switches:");
  LOG_SWITCH("  ", "Turn on LED", this->led_enable_switch_);
  LOG_SWITCH("  ", "Micro Motion Enable", this->micro_motion_enable_switch_);
#endif
#ifdef USE_TEXT_SENSOR
  ESP_LOGCONFIG(TAG, "Text Sensors:");
  LOG_TEXT_SENSOR("  ", "Software Version", this->software_version_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Hardware Version", this->hardware_version_text_sensor_);
#endif
}

void DFRobotC4001Hub::setup() {
#ifdef USE_SWITCH
  // Restore LED Enable preferences (from flash storage)
  if (this->led_enable_switch_ != nullptr) {
    this->pref_ = global_preferences->make_preference<bool>(this->led_enable_switch_->get_object_id_hash());

    bool value;
    ESP_LOGV(TAG, "Reading LED Enable setting from flash.");
    if (!this->pref_.load(&value)) {
      ESP_LOGV(TAG, "Reading LED Enable unsuccessful, using default (false).");
      value = false;
    }
    this->set_led_enable(value, false);
  }
#endif
  ESP_LOGCONFIG(TAG, "Running setup");
}

void DFRobotC4001Hub::loop() {
  static uint64_t start_time = millis();
  static bool prompt = false;

  if (this->is_failed()) {
    return;
  }
  if (!prompt) {
    // still waiting to module to come alive and send a prompt
    if (!this->find_prompt_()) {
      if (millis() - start_time > 4000) {
        this->mark_failed("Module not responding");
        return;
      }
    } else {
      ESP_LOGCONFIG(TAG, "Recv Prompt (%ldms)", millis() - start_time);
      this->config_load();
      prompt = true;
    }
    return;
  }
  if (cmd_queue_.is_empty()) {
    // Command queue empty, first time this happens setup is complete
    if (!this->is_setup_) {
      this->is_setup_ = true;
      ESP_LOGCONFIG(TAG, "Setup complete");
    }
    // Read sensor state.
    cmd_queue_.enqueue(make_unique<ReadStateCommand>());
  }
  // Commands are non-blocking and need to be called repeatedly.
  if (cmd_queue_.process(this)) {
    // Dequeue if command is done
    cmd_queue_.dequeue();
  }
}

int8_t DFRobotC4001Hub::enqueue(std::unique_ptr<Command> cmd) {
  return cmd_queue_.enqueue(std::move(cmd));  // Transfer ownership using std::move
}

uint8_t DFRobotC4001Hub::read_message_() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);

    if (this->read_pos_ == MMWAVE_READ_BUFFER_LENGTH)
      this->read_pos_ = 0;

    ESP_LOGVV(TAG, "Buffer pos: %u %d", this->read_pos_, byte);

    if (byte == ASCII_CR)
      continue;
    if (byte >= 0x7F)
      byte = '?';  // needs to be valid utf8 string for log functions.
    this->read_buffer_[this->read_pos_] = byte;

    if (this->read_pos_ == 9 && byte == '>')
      this->read_buffer_[++this->read_pos_] = ASCII_LF;

    if (this->read_buffer_[this->read_pos_] == ASCII_LF) {
      this->read_buffer_[this->read_pos_] = 0;
      this->read_pos_ = 0;
      ESP_LOGV(TAG, "Recv Msg: %s", this->read_buffer_);
      return 1;  // Full message in buffer
    } else {
      this->read_pos_++;
    }
  }
  return 0;  // No full message yet
}

uint8_t DFRobotC4001Hub::find_prompt_() {
  if (this->read_message_()) {
    std::string message(this->read_buffer_);
    if (message.rfind("DFRobot:/>") != std::string::npos) {
      return 1;  // Prompt found
    }
  }
  return 0;  // Not found yet
}

uint8_t DFRobotC4001Hub::send_cmd_(const char *cmd, uint32_t duration) {
  // The interval between two commands must be larger than the specified duration (in ms).
  if (millis() - ts_last_cmd_sent_ > duration) {
    this->write_str(cmd);
    ts_last_cmd_sent_ = millis();
    ESP_LOGV(TAG, "Send Cmd: %s", cmd);
    return 1;  // Command sent
  }
  // Could not send command yet as command duration did not fully pass yet.
  return 0;
}

int8_t CircularCommandQueue::enqueue(std::unique_ptr<Command> cmd) {
  if (this->is_full()) {
    ESP_LOGE(TAG, "Command queue is full");
    return -1;
  } else if (this->is_empty())
    front_++;
  rear_ = (rear_ + 1) % COMMAND_QUEUE_SIZE;
  commands_[rear_] = std::move(cmd);  // Transfer ownership using std::move
  return 1;
}

std::unique_ptr<Command> CircularCommandQueue::dequeue() {
  if (this->is_empty())
    return nullptr;
  std::unique_ptr<Command> dequeued_cmd = std::move(commands_[front_]);
  if (front_ == rear_) {
    front_ = -1;
    rear_ = -1;
  } else {
    front_ = (front_ + 1) % COMMAND_QUEUE_SIZE;
  }

  return dequeued_cmd;
}

bool CircularCommandQueue::is_empty() { return front_ == -1; }

bool CircularCommandQueue::is_full() { return (rear_ + 1) % COMMAND_QUEUE_SIZE == front_; }

// Run execute method of first in line command.
// Execute is non-blocking and has to be called until it returns 1.
uint8_t CircularCommandQueue::process(DFRobotC4001Hub *parent) {
  if (!is_empty()) {
    return commands_[front_]->execute(parent);
  } else {
    return 1;
  }
}

}  // namespace dfrobot_c4001
}  // namespace esphome
