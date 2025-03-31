#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "stusb4500.h"

namespace esphome
{
    namespace stusb4500
    {

        template <typename... Ts>
        class TurnGpioOn : public Action<Ts...>
        {
        public:
            explicit TurnGpioOn(STUSB4500Hub *stusb4500) : stusb4500_(stusb4500) {}

            void play(Ts... x) override { this->stusb4500_->turn_gpio_on(); }

        protected:
            STUSB4500Hub *stusb4500_;
        };

        template <typename... Ts>
        class TurnGpioOff : public Action<Ts...>
        {
        public:
            explicit TurnGpioOff(STUSB4500Hub *stusb4500) : stusb4500_(stusb4500) {}

            void play(Ts... x) override { this->stusb4500_->turn_gpio_off(); }

        protected:
            STUSB4500Hub *stusb4500_;
        };

    } // namespace stusb4500_
} // namespace esphome
