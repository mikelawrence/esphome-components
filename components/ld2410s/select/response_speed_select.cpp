#include "response_speed_select.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.select";

void ResponseSpeedSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_response_speed(index);
}

}  // namespace esphome::ld2410s
