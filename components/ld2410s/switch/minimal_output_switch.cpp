#include "minimal_output_switch.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.switch";

void MinimalOutputSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->set_minimal_output(state);
}

}  // namespace esphome::ld2410s
