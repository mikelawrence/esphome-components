#pragma once

#include <cstdint>
#include <string>

#include "esphome/core/helpers.h"

namespace esphome {
namespace dfrobot_c4001 {
class DFRobotC4001Hub;

// Use command queue and time stamps to avoid blocking.
// When component has run time, check if minimum time (1s) between
// commands has passed. After that run a command from the queue.
class Command {
 public:
  virtual ~Command() = default;
  virtual uint8_t execute(DFRobotC4001Hub *parent);
  virtual uint8_t on_message(std::string &message) = 0;

 protected:
  DFRobotC4001Hub *parent_{nullptr};
  std::string cmd_;
  bool cmd_sent_{false};
  int8_t retries_left_{2};
  uint32_t cmd_duration_ms_{10};
  uint32_t timeout_ms_{1500};
};

class ReadStateCommand : public Command {
 public:
  uint8_t execute(DFRobotC4001Hub *parent) override;
  uint8_t on_message(std::string &message) override;
};

class PowerCommand : public Command {
 public:
  PowerCommand(bool power_on);
  uint8_t on_message(std::string &message) override;

 protected:
  bool power_on_;
};

class GetRangeCommand : public Command {
 public:
  GetRangeCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  float min_range_;
  float max_range_;
};

class SetRangeCommand : public Command {
 public:
  SetRangeCommand(float min_range, float max_range);
  uint8_t on_message(std::string &message) override;

 protected:
  float min_range_;
  float max_range_;
};

class GetTrigRangeCommand : public Command {
 public:
  GetTrigRangeCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  float trigger_range_;
};

class SetTrigRangeCommand : public Command {
 public:
  SetTrigRangeCommand(float trigger_range);
  uint8_t on_message(std::string &message) override;

 protected:
  float trigger_range_;
};

class GetSensitivityCommand : public Command {
 public:
  GetSensitivityCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  uint16_t hold_sensitivity_;
  uint16_t trigger_sensitivity_;
};

class SetSensitivityCommand : public Command {
 public:
  SetSensitivityCommand(uint16_t hold_sensitivity, uint16_t trigger_sensitivity);
  uint8_t on_message(std::string &message) override;

 protected:
  uint16_t hold_sensitivity_;
  uint16_t trigger_sensitivity_;
};

class GetLatencyCommand : public Command {
 public:
  GetLatencyCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  float on_latency_;
  float off_latency_;
};

class SetLatencyCommand : public Command {
 public:
  SetLatencyCommand(float on_latency, float off_latency);
  uint8_t on_message(std::string &message) override;

 protected:
  float on_latency_;
  float off_latency_;
};

class GetInhibitTimeCommand : public Command {
 public:
  GetInhibitTimeCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  float inhibit_time_;
};

class SetInhibitTimeCommand : public Command {
 public:
  SetInhibitTimeCommand(float inhibit_time);
  uint8_t on_message(std::string &message) override;

 protected:
  float inhibit_time_;
};

class GetThrFactorCommand : public Command {
 public:
  GetThrFactorCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  float threshold_factor_;
};

class SetThrFactorCommand : public Command {
 public:
  SetThrFactorCommand(float threshold_factor);
  uint8_t on_message(std::string &message) override;

 protected:
  float threshold_factor_;
};

class GetLedModeCommand : public Command {
 public:
  GetLedModeCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  bool led_enable_;
};

class SetLedModeCommand : public Command {
 public:
  SetLedModeCommand(bool led_mode);
  uint8_t on_message(std::string &message) override;

 protected:
  bool led_enable_;
};

class GetMicroMotionCommand : public Command {
 public:
  GetMicroMotionCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  bool micro_motion_;
};

class SetMicroMotionCommand : public Command {
 public:
  SetMicroMotionCommand(bool enable);
  uint8_t on_message(std::string &message) override;

 protected:
  bool micro_motion_;
};

class FactoryResetCommand : public Command {
 public:
  FactoryResetCommand();
  uint8_t on_message(std::string &message) override;
};

class ResetSystemCommand : public Command {
 public:
  ResetSystemCommand();
  uint8_t on_message(std::string &message) override;
};

class SaveCfgCommand : public Command {
 public:
  SaveCfgCommand();
  uint8_t on_message(std::string &message) override;
};

class SetRunAppCommand : public Command {
 public:
  SetRunAppCommand(uint8_t mode);
  uint8_t on_message(std::string &message) override;

 protected:
  bool mode_;
};

class GetHWVCommand : public Command {
 public:
  GetHWVCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  std::string version_;
};

class GetSWVCommand : public Command {
 public:
  GetSWVCommand();
  uint8_t on_message(std::string &message) override;

 protected:
  std::string version_;
};

}  // namespace dfrobot_c4001
}  // namespace esphome
