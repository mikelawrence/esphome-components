#include "threshold_trigger_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void ThresholdTriggerNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_gate_trig_threshold(this->gate_, value);
}

}  // namespace esphome::ld2410s
