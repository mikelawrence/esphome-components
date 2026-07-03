#include "max_detect_gate_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void MaxDetectGateNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_max_detect_gate(value);
}

}  // namespace esphome::ld2410s
