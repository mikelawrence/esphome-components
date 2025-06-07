#include "installation_angle_number.h"

namespace esphome {
namespace ld2450 {

  void InstallationAngleNumber::control(float value)
  {
    this->publish_state(value);
    this->parent_->set_installation_angle();
  }

} // namespace ld2450
} // namespace esphome
