#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dfrobot_c4001 {
class DFRobotC4001Hub;
// Enumeration for Command States
enum CommandState {
  STATE_CMD_SEND = 0,  // command needs to be sent
  STATE_WAIT_ECHO,     // command was sent, now waiting for command echo
  STATE_PROCESS,       // command in process
  STATE_WAIT_PROMPT,   // waiting for prompt, teminates this transaction
  STATE_DONE,
};

// Use command queue and time stamps to avoid blocking.
// When component has run time, check if minimum time (1s) between
// commands has passed. After that run a command from the queue.
class Command {
 public:
  virtual ~Command() = default;
  virtual uint8_t execute(DFRobotC4001Hub *parent);
  virtual void on_message(std::string &message) = 0;

 protected:
  DFRobotC4001Hub *parent_{nullptr};
  uint8_t state_{STATE_CMD_SEND};
  uint8_t done_{false};
  uint8_t error_{false};
  std::string cmd_;
  int8_t error_count_{0};
  int8_t retries_left_{2};
  uint32_t cmd_duration_ms_{10};
  uint32_t timeout_ms_{1500};
};

class ReadStateCommand : public Command {
 public:
  uint8_t execute(DFRobotC4001Hub *parent) override;
  void on_message(std::string &message) override;
};

class PowerCommand : public Command {
 public:
  PowerCommand(bool power_on);
  void on_message(std::string &message) override;

 protected:
  bool power_on_;
};

class GetRangeCommand : public Command {
 public:
  GetRangeCommand();
  void on_message(std::string &message) override;

 protected:
  float min_range_;
  float max_range_;
};

class SetRangeCommand : public Command {
 public:
  SetRangeCommand(float min_range, float max_range);
  void on_message(std::string &message) override;

 protected:
  float min_range_;
  float max_range_;
};

class GetTrigRangeCommand : public Command {
 public:
  GetTrigRangeCommand();
  void on_message(std::string &message) override;

 protected:
  float trigger_range_;
};

class SetTrigRangeCommand : public Command {
 public:
  SetTrigRangeCommand(float trigger_range);
  void on_message(std::string &message) override;

 protected:
  float trigger_range_;
};

class GetSensitivityCommand : public Command {
 public:
  GetSensitivityCommand();
  void on_message(std::string &message) override;

 protected:
  uint16_t hold_sensitivity_;
  uint16_t trigger_sensitivity_;
};

class SetSensitivityCommand : public Command {
 public:
  SetSensitivityCommand(uint16_t hold_sensitivity, uint16_t trigger_sensitivity);
  void on_message(std::string &message) override;

 protected:
  uint16_t hold_sensitivity_;
  uint16_t trigger_sensitivity_;
};

class GetLatencyCommand : public Command {
 public:
  GetLatencyCommand();
  void on_message(std::string &message) override;

 protected:
  float on_latency_;
  float off_latency_;
};

class SetLatencyCommand : public Command {
 public:
  SetLatencyCommand(float on_latency, float off_latency);
  void on_message(std::string &message) override;

 protected:
  float on_latency_;
  float off_latency_;
};

class GetInhibitTimeCommand : public Command {
 public:
  GetInhibitTimeCommand();
  void on_message(std::string &message) override;

 protected:
  float inhibit_time_;
};

class SetInhibitTimeCommand : public Command {
 public:
  SetInhibitTimeCommand(float inhibit_time);
  void on_message(std::string &message) override;

 protected:
  float inhibit_time_;
};

class GetThrFactorCommand : public Command {
 public:
  GetThrFactorCommand();
  void on_message(std::string &message) override;

 protected:
  float threshold_factor_;
};

class SetThrFactorCommand : public Command {
 public:
  SetThrFactorCommand(float threshold_factor);
  void on_message(std::string &message) override;

 protected:
  float threshold_factor_;
};

class GetUartOutputCommand : public Command {
 public:
  GetUartOutputCommand();
  void on_message(std::string &message) override;

 protected:
  bool uart_output_enable_;
};

class SetUartOutputCommand : public Command {
 public:
  SetUartOutputCommand(bool uart_output_enable);
  void on_message(std::string &message) override;

 protected:
  bool uart_output_enable_;
};

class SetLedModeCommand1 : public Command {
 public:
  SetLedModeCommand1(bool led_mode);
  void on_message(std::string &message) override;

 protected:
  bool led_enable_;
};

class SetLedModeCommand2 : public Command {
 public:
  SetLedModeCommand2(bool led_mode);
  void on_message(std::string &message) override;

 protected:
  bool led_enable_;
};

class GetMicroMotionCommand : public Command {
 public:
  GetMicroMotionCommand();
  void on_message(std::string &message) override;

 protected:
  bool micro_motion_;
};

class SetMicroMotionCommand : public Command {
 public:
  SetMicroMotionCommand(bool enable);
  void on_message(std::string &message) override;

 protected:
  bool micro_motion_;
};

class FactoryResetCommand : public Command {
 public:
  FactoryResetCommand();
  void on_message(std::string &message) override;
};

class ResetSystemCommand : public Command {
 public:
  ResetSystemCommand();
  void on_message(std::string &message) override;
};

class SaveCfgCommand : public Command {
 public:
  SaveCfgCommand();
  void on_message(std::string &message) override;
};

class SetRunAppCommand : public Command {
 public:
  SetRunAppCommand(uint8_t mode);
  void on_message(std::string &message) override;

 protected:
  uint8_t mode_;
};

class GetHWVCommand : public Command {
 public:
  GetHWVCommand();
  void on_message(std::string &message) override;

 protected:
  std::string version_;
};

class GetSWVCommand : public Command {
 public:
  GetSWVCommand();
  void on_message(std::string &message) override;

 protected:
  std::string version_;
};

}  // namespace dfrobot_c4001
}  // namespace esphome
