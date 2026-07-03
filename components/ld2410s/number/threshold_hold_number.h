#pragma once

#include "esphome/components/number/number.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class ThresholdHoldNumber : public number::Number, public Parented<LD2410SComponent> {
 public:
  ThresholdHoldNumber(uint8_t gate) : gate_(gate) {};

 protected:
  uint8_t gate_;
  void control(float value) override;
};

}  // namespace esphome::ld2410s
