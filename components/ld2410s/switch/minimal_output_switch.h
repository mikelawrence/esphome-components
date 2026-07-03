#pragma once

#include "esphome/components/switch/switch.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class MinimalOutputSwitch : public switch_::Switch, public Parented<LD2410SComponent> {
 public:
  MinimalOutputSwitch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace esphome::ld2410s

