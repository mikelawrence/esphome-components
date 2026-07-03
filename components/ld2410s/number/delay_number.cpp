#include "delay_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void DelayNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_no_delay(value);
}

}  // namespace esphome::ld2410s
