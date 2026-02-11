#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/sensirion_common/i2c_sensirion.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"
#include <algorithm>
#include <cstdint>

namespace esphome {
namespace sen6x {

enum class SetupStates : uint8_t {
  SM_IDLE,
  SM_MEAS_INIT,
  SM_MEAS_GET,
  SM_MEAS_VOCA,
  SM_MEAS_PRES,
  SM_MEAS_DONE,
  SM_HEAT_INIT,
  SM_HEAT_ON,
  SM_HEAT_START,
  SM_HEAT_DONE,
  SM_CO2_RECAL_INIT,
  SM_CO2_RECAL_ON,
  SM_CO2_RECAL_WAIT,
  SM_CO2_RECAL_START,
  SM_CO2_RECAL_DONE,
  SM_CO2_PRESS_INIT,
  SM_CO2_PRESS_DONE,
  SM_FAN_INIT,
  SM_FAN_ON,
  SM_FAN_WAIT,
  SM_FAN_START,
  SM_FAN_DONE,
  SM_TEMP_COMP_INIT,
  SM_TEMP_COMP_DONE,
  SM_VOC_STATE_INIT,
  SM_VOC_STATE_DONE,
  SM_VOC_CHECK_INIT,
  SM_VOC_CHECK_DONE,
  SM_SETUP_INIT,
  SM_SETUP_INIT_WAIT,
  SM_SETUP_GET_STATUS,
  SM_SETUP_GET_SN,
  SM_SETUP_GET_SN_1,
  SM_SETUP_GET_PN,
  SM_SETUP_GET_FW,
  SM_SETUP_SET_ACCEL,
  SM_SETUP_SET_VOCA,
  SM_SETUP_SET_VOCT,
  SM_SETUP_SET_NOXT,
  SM_SETUP_SET_TP,
  SM_SETUP_SET_CO2ASC,
  SM_SETUP_SET_CO2AC,
  SM_SETUP_SENSOR_CHECK,
  SM_SETUP_START_MEAS,
  SM_SETUP_DONE,
};

enum class Sen6xType : uint8_t { SEN62, SEN63C, SEN65, SEN66, SEN68, SEN69C, UNKNOWN };
enum class Sen6xVocStatus : uint8_t { NOTHING, WAITING, RESTORED, RESTORED_TIME, UPDATED, NO_PREF, TOO_OLD, ERROR };

struct Sen6xVocBaseline {
  uint16_t state[4];  // algorithm state is actually uint8_t[8] but uint16_t[4] is the transaction format
  time_t epoch;       // Used to determine age of algorithm state
};

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

// Time interval of 2 hours (in milliseconds) for storing algorithm state
static const uint32_t ALGORITHM_STATE_STORE_INTERVAL_MS = 2 * 60 * 60 * 1000;
// Time interval of 2 hours 15 minutes (in seconds) for max age of algorithm state
static const time_t ALGORITHM_STATE_MAX_AGE = 2 * 60 * 60 + 15 * 60;

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
  void loop() override;
  void set_store_voc_algorithm_state(bool store_voc_algorithm_state) {
    this->store_voc_algorithm_state_ = store_voc_algorithm_state;
  }
  void set_type(Sen6xType type);
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
  void set_time_source(time::RealTimeClock *time) { this->time_source_ = time; }
  bool start_fan_cleaning();
  bool activate_heater();
  bool perform_forced_co2_recalibration(uint16_t co2);
  bool set_voc_algorithm_state(int32_t epoch);
  bool is_initialized() const;

 protected:
  // void internal_setup_(SetupStates state);
  bool start_measurements_();
  bool stop_measurements_();
  bool write_tuning_parameters_(uint16_t i2c_command, const GasTuning &tuning);
  bool write_temperature_compensation_(const TemperatureCompensation &compensation);
  bool write_temperature_acceleration_();
  bool write_ambient_pressure_compensation_(uint16_t pressure_in_hpa);

  char serial_number_[17] = "UNKNOWN";
  Sen6xVocBaseline voc_algorithm_state_{0};
  sensor::Sensor *ambient_pressure_compensation_source_;
  time::RealTimeClock *time_source_;
  uint32_t voc_algorithm_state_time_{0};
  uint32_t stop_time_{0};
  uint32_t state_time_{0};
  uint32_t state_wait_time_{0};
  uint16_t ambient_pressure_compensation_{0};
  uint16_t co2_reference_{0};
  uint16_t ambient_pressure_{0};
  uint16_t measurement_cmd_;
  uint8_t measurement_cmd_len_;
  uint8_t firmware_major_{0xFF};
  uint8_t firmware_minor_{0xFF};
  uint8_t command_flag_{0};
  uint8_t meas_warning_{0};
  SetupStates loop_state_{SetupStates::SM_SETUP_INIT};
  Sen6xVocStatus voc_algorithm_state_status_{Sen6xVocStatus::NOTHING};

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
