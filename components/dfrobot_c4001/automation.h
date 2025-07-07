#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"

#include "dfrobot_c4001.h"

namespace esphome {
namespace dfrobot_c4001 {
template<typename... Ts> class DFRobotC4001FactoryResetAction : public Action<Ts...>, public Parented<DFRobotC4001Hub> {
 public:
  void play(Ts... x) {
    this->parent_->factory_reset();
  }
};
template<typename... Ts> class DFRobotC4001RestartAction : public Action<Ts...>, public Parented<DFRobotC4001Hub> {
 public:
  void play(Ts... x) {
    this->parent_->restart();
  }
};
}  // namespace dfrobot_c4001
}  // namespace esphome
