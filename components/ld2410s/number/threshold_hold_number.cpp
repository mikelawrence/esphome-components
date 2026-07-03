#include "threshold_hold_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void ThresholdHoldNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_gate_hold_threshold(this->gate_, value);
}

}  // namespace esphome::ld2410s
