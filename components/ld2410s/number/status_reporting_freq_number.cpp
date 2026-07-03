#include "status_reporting_freq_number.h"

namespace esphome::ld2410s {

static const char *const TAG = "ld2410s.number";

void StatusReportingFreqNumber::control(float value) {
  this->publish_state(value);
  this->parent_->set_status_reporting_freq(value);
}

}  // namespace esphome::ld2410s
