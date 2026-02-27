#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/sensor/sensor.h"  // Required for temperature sensor type

namespace esphome {
namespace ds3231 {

/// DS3231 Real Time Clock component for ESPHome
/// Provides accurate timekeeping via I2C and optional temperature sensor readings
class DS3231Component : public time::RealTimeClock, public i2c::I2CDevice {
 public:
  /// Initialize the DS3231 component and verify I2C communication
  void setup() override;
  
  /// Called periodically to update time and temperature readings
  void update() override;
  
  /// Output component configuration to logs
  void dump_config() override;
  
  /// Return setup priority for component initialization order
  float get_setup_priority() const override;
  
  /// Read current time from the RTC and synchronize system clock
  void read_time();
  
  /// Write a specific time to the RTC
  void write_time(esphome::ESPTime epoch);
  
  /// Write current system time to the RTC
  void write_time();

  /// Set the temperature sensor for publishing DS3231 internal temperature readings
  void set_temperature_sensor(esphome::sensor::Sensor *temperature_sensor) {
    this->temperature_sensor_ = temperature_sensor;
  }

 protected:
  /// Read RTC registers via I2C
  bool read_rtc_();
  
  /// Write RTC registers via I2C
  bool write_rtc_();

  /// Union structure for DS3231 register layout
  /// Maps to the actual DS3231 hardware registers for BCD-encoded time values
  union DS3231Reg {
    struct {
      // Register 0x00: Seconds (BCD format)
      uint8_t second : 4;      // Seconds (0-9)
      uint8_t second_10 : 3;   // Tens of seconds (0-5)
      bool ch : 1;              // Clock Halt bit (oscillator enable/disable)

      // Register 0x01: Minutes (BCD format)
      uint8_t minute : 4;      // Minutes (0-9)
      uint8_t minute_10 : 3;   // Tens of minutes (0-5)
      uint8_t unused_1 : 1;

      // Register 0x02: Hours (BCD format, 24-hour mode)
      uint8_t hour : 4;        // Hours (0-9)
      uint8_t hour_10 : 2;     // Tens of hours (0-2)
      uint8_t unused_2 : 2;

      // Register 0x03: Day of week (1-7)
      uint8_t weekday : 3;
      uint8_t unused_3 : 5;

      // Register 0x04: Day of month (BCD format)
      uint8_t day : 4;         // Day (0-9)
      uint8_t day_10 : 2;      // Tens of day (0-3)
      uint8_t unused_4 : 2;

      // Register 0x05: Month (BCD format)
      uint8_t month : 4;       // Month (0-9)
      uint8_t month_10 : 1;    // Tens of month (0-1)
      uint8_t unused_5 : 3;

      // Register 0x06: Year (BCD format, 00-99)
      uint8_t year : 4;        // Year (0-9)
      uint8_t year_10 : 4;     // Tens of year (0-9)

      // Register 0x07: Control register
      uint8_t rs : 2;          // Rate Select bits
      uint8_t unused_6 : 2;
      bool sqwe : 1;            // Square Wave Enable
      uint8_t unused_7 : 2;
      bool out : 1;             // Output control

      // Temperature registers (not directly written, read separately)
      int8_t temp_msb;          // Register 0x11: Temperature MSB (signed)
      uint8_t temp_lsb;         // Register 0x12: Temperature LSB (upper 2 bits)
    } reg;
    mutable uint8_t raw[sizeof(reg)];  // Raw byte access to register structure
  } ds3231_;

  /// Optional temperature sensor for publishing DS3231 internal temperature
  esphome::sensor::Sensor *temperature_sensor_{nullptr};  
};

/// Automation action to write current system time to the RTC
template<typename... Ts>
class WriteAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->write_time(); }
};

/// Automation action to read time from the RTC and synchronize system clock
template<typename... Ts>
class ReadAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->read_time(); }
};

}  // namespace ds3231
}  // namespace esphome