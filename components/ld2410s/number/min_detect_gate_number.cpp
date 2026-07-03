#include "min_detect_gate_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void MinDetectGateNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_min_detect_gate(value);
}

}  // namespace esphome::ld2410s
