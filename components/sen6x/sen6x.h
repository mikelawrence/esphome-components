#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/sensirion_common/i2c_sensirion.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"
#include <algorithm>

namespace esphome {
namespace sen6x {

enum class SetupStates : uint8_t {
  SM_START,
  SM_START_1,
  SM_GET_SN,
  SM_GET_SN_1,
  SM_GET_PN,
  SM_GET_FW,
  SM_SET_ACCEL,
  SM_SET_VOCB,
  SM_SET_VOCT,
  SM_SET_NOXT,
  SM_SET_TP,
  SM_SET_CO2ASC,
  SM_SET_CO2AC,
  SM_SENSOR_CHECK,
  SM_START_MEAS,
  SM_DONE
};

enum class Sen6xType : uint8_t { SEN62, SEN63C, SEN65, SEN66, SEN68, SEN69C, UNKNOWN };

struct GasTuning {
  uint16_t index_offset;
  uint16_t learning_time_offset_hours;
  uint16_t learning_time_gain_hours;
  uint16_t gating_max_duration_minutes;
  uint16_t std_initial;
  uint16_t gain_factor;
};

struct TemperatureCompensation {
  int16_t offset;
  int16_t normalized_offset_slope;
  uint16_t time_constant;
  uint8_t slot;

  TemperatureCompensation() : offset(0), normalized_offset_slope(0), time_constant(0), slot(0) {}
  TemperatureCompensation(float offset, float normalized_offset_slope, uint16_t time_constant, uint8_t slot = 0) {
    this->offset = static_cast<int16_t>(offset * 200.0);
    this->normalized_offset_slope = static_cast<int16_t>(normalized_offset_slope * 10000.0);
    this->time_constant = time_constant;
    this->slot = slot;
  }
};

struct TemperatureAcceleration {
  uint16_t k;
  uint16_t p;
  uint16_t t1;
  uint16_t t2;
  TemperatureAcceleration() : k(20), p(20), t1(100), t2(300) {}
  TemperatureAcceleration(float k, float p, float t1, float t2) {
    this->k = static_cast<uint16_t>(k * 10.0);
    this->p = static_cast<uint16_t>(p * 10.0);
    this->t1 = static_cast<uint16_t>(t1 * 10.0);
    this->t2 = static_cast<uint16_t>(t2 * 10.0);
  }
};

// Shortest time interval of 2H (in milliseconds) for storing baseline values.
// Prevents wear of the flash because of too many write operations
static const uint32_t SHORTEST_BASELINE_STORE_INTERVAL = 2 * 60 * 60 * 1000;

class Sen6xComponent : public PollingComponent, public sensirion_common::SensirionI2CDevice {
  SUB_SENSOR(pm_1_0)
  SUB_SENSOR(pm_2_5)
  SUB_SENSOR(pm_4_0)
  SUB_SENSOR(pm_10_0)
  SUB_SENSOR(temperature)
  SUB_SENSOR(humidity)
  SUB_SENSOR(voc)
  SUB_SENSOR(nox)
  SUB_SENSOR(co2)
  SUB_SENSOR(hcho)
  SUB_SENSOR(co2_ambient_pressure_source)

 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  void set_store_voc_algorithm_state(bool store_voc_algorithm_state) {
    this->store_voc_algorithm_state_ = store_voc_algorithm_state;
  }
  void set_type(Sen6xType type) { this->type_ = type; }
  void set_voc_algorithm_tuning(uint16_t index_offset, uint16_t learning_time_offset_hours,
                                uint16_t learning_time_gain_hours, uint16_t gating_max_duration_minutes,
                                uint16_t std_initial, uint16_t gain_factor) {
    GasTuning tuning_params;
    tuning_params.index_offset = index_offset;
    tuning_params.learning_time_offset_hours = learning_time_offset_hours;
    tuning_params.learning_time_gain_hours = learning_time_gain_hours;
    tuning_params.gating_max_duration_minutes = gating_max_duration_minutes;
    tuning_params.std_initial = std_initial;
    tuning_params.gain_factor = gain_factor;
    this->voc_tuning_params_ = tuning_params;
  }
  void set_nox_algorithm_tuning(uint16_t index_offset, uint16_t learning_time_offset_hours,
                                uint16_t gating_max_duration_minutes, uint16_t gain_factor) {
    GasTuning tuning_params;
    tuning_params.index_offset = index_offset;
    tuning_params.learning_time_offset_hours = learning_time_offset_hours;
    tuning_params.learning_time_gain_hours = 12;  // set to default per datasheet, not used
    tuning_params.gating_max_duration_minutes = gating_max_duration_minutes;
    tuning_params.std_initial = 50;  // set to default per datasheet, not used
    tuning_params.gain_factor = gain_factor;
    this->nox_tuning_params_ = tuning_params;
  }
  bool set_temperature_compensation(float offset, float normalized_offset_slope, uint16_t time_constant,
                                    uint8_t slot = 0);
  void set_temperature_acceleration(float k, float p, float t1, float t2) {
    TemperatureAcceleration accel(k, p, t1, t2);
    this->temperature_acceleration_ = accel;
  }
  void set_automatic_self_calibration(bool value) { this->auto_self_calibration_ = value; }
  void set_altitude_compensation(uint16_t altitude) { this->altitude_compensation_ = altitude; }
  void set_ambient_pressure_compensation_source(sensor::Sensor *pressure) {
    this->ambient_pressure_compensation_source_ = pressure;
  }
  bool set_ambient_pressure_compensation(uint16_t pressure_in_hpa);
  bool start_fan_cleaning();
  bool activate_heater();
  bool perform_forced_co2_recalibration(uint16_t co2);
  bool busy() { return this->busy_ || this->updating_; };

 protected:
  void internal_setup_(SetupStates state);
  bool has_co2_() const;
  bool start_measurements_();
  bool stop_measurements_();
  bool write_tuning_parameters_(uint16_t i2c_command, const GasTuning &tuning);
  bool write_temperature_compensation_(const TemperatureCompensation &compensation);
  bool write_temperature_acceleration_();
  bool write_ambient_pressure_compensation_(uint16_t pressure_in_hpa);

  char serial_number_[17] = "UNKNOWN";
  uint16_t voc_algorithm_state_[4]{0};
  sensor::Sensor *ambient_pressure_compensation_source_;
  uint32_t voc_algorithm_time_;
  uint16_t ambient_pressure_compensation_{0};
  uint8_t firmware_major_{0xFF};
  uint8_t firmware_minor_{0xFF};
  bool initialized_{false};
  bool running_{false};
  bool updating_{false};
  bool busy_{false};
  bool voc_algorithm_error_{false};

  optional<Sen6xType> type_;
  optional<GasTuning> voc_tuning_params_;
  optional<GasTuning> nox_tuning_params_;
  optional<TemperatureCompensation> temperature_compensation_;
  optional<TemperatureAcceleration> temperature_acceleration_;
  optional<bool> auto_self_calibration_;
  optional<uint16_t> altitude_compensation_;
  optional<bool> store_voc_algorithm_state_;

  ESPPreferenceObject pref_;
};
}  // namespace sen6x
}  // namespace esphome
