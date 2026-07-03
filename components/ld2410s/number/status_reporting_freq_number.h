#pragma once

#include "esphome/components/number/number.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class StatusReportingFreqNumber : public number::Number, public Parented<LD2410SComponent> {
 public:
  StatusReportingFreqNumber() = default;

 protected:
  void control(float value) override ;
};

}  // namespace esphome::ld2410s
