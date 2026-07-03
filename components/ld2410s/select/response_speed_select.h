#pragma once

#include "esphome/components/select/select.h"
#include "../ld2410s.h"

namespace esphome::ld2410s {

class ResponseSpeedSelect final : public select::Select, public Parented<LD2410SComponent> {
 public:
  ResponseSpeedSelect() = default;

 protected:
  void control(size_t index) override;
};

}  // namespace esphome::ld2410s
