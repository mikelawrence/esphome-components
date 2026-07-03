#include "calibration_start_button.h"

namespace esphome::ld2410s {

// static const char *const TAG = "ld2410s.button";

void CalibrationStartButton::press_action() {
  this->parent_->cal_start(); 
}

}  // namespace esphome::ld2410s
