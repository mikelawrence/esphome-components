#include "button.h"

namespace esphome {
namespace dfrobot_c4001 {
void ConfigSaveButton::press_action() { this->parent_->config_save(); }
}  // namespace dfrobot_c4001
}  // namespace esphome
