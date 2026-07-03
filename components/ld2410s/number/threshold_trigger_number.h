#pragma once

#include "threshold_trigger_number.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class ThresholdTriggerNumber : public number::Number, public Parented<LD2410SComponent> {
 public:
  ThresholdTriggerNumber(uint8_t gate) : gate_(gate) {};

 protected:
  uint8_t gate_;
  void control(float value) override;
};

}  // namespace esphome::ld2410s
