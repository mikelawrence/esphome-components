#pragma once

#include "esphome/components/button/button.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class CalibrationStartButton : public button::Button, public Parented<LD2410SComponent> {
 public:
  CalibrationStartButton() = default;

 protected:
  void press_action() override;
};
}  // namespace esphome::ld2410s
