#include "sen6x.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace sen6x {

static const char *const TAG = "sen6x";

static const uint8_t CMD_FLAG_SETUP = 0x01;
static const uint8_t CMD_FLAG_MEASUREMENT = 0x02;
static const uint8_t CMD_FLAG_HEATER = 0x04;
static const uint8_t CMD_FLAG_CO2_RECAL = 0x08;
static const uint8_t CMD_FLAG_CO2_PRESS = 0x10;
static const uint8_t CMD_FLAG_FAN_CLEAN = 0x20;
static const uint8_t CMD_FLAG_TEMP_COMP = 0x40;
static const uint8_t CMD_FLAG_VOC_CHECK = 0x80;

static const uint16_t SEN62_CMD_READ_MEASUREMENT = 0x04A3;
static const uint16_t SEN63C_CMD_READ_MEASUREMENT = 0x0471;
static const uint16_t SEN65_CMD_READ_MEASUREMENT = 0x0446;
static const uint16_t SEN66_CMD_READ_MEASUREMENT = 0x0300;
static const uint16_t SEN68_CMD_READ_MEASUREMENT = 0x0467;
static const uint16_t SEN69C_CMD_READ_MEASUREMENT = 0x04B5;

static const uint16_t CMD_TEMPERATURE_ACCEL_PARAMETERS = 0x6100;
static const uint16_t CMD_PERFORM_FORCED_CO2_RECAL = 0x6707;
static const uint16_t CMD_CO2_SENSOR_AUTO_SELF_CAL = 0x6711;
static const uint16_t CMD_AMBIENT_PRESSURE = 0x6720;
static const uint16_t CMD_SENSOR_ALTITUDE = 0x6736;
static const uint16_t CMD_ACTIVATE_SHT_HEATER = 0x6765;
static const uint16_t CMD_READ_DEVICE_STATUS = 0xD206;
static const uint16_t CMD_READ_AND_CLEAR_DEVICE_STATUS = 0xD210;

static const uint16_t CMD_RESET = 0xD304;
static const uint16_t CMD_START_MEASUREMENTS = 0x0021;
static const uint16_t CMD_STOP_MEASUREMENTS = 0x0104;
static const uint16_t CMD_START_CLEANING_FAN = 0x5607;
static const uint16_t CMD_TEMPERATURE_COMPENSATION = 0x60B2;
static const uint16_t CMD_VOC_ALGORITHM_TUNING = 0x60D0;
static const uint16_t CMD_NOX_ALGORITHM_TUNING = 0x60E1;
static const uint16_t CMD_VOC_ALGORITHM_STATE = 0x6181;
static const uint16_t CMD_GET_PRODUCT_NAME = 0xD014;
static const uint16_t CMD_GET_SERIAL_NUMBER = 0xD033;
static const uint16_t CMD_GET_DATA_READY_STATUS = 0x0202;
static const uint16_t CMD_GET_FIRMWARE_VERSION = 0xD100;

static const int8_t INDEX_SCALE_FACTOR = 10;                      // used for VOC and NOx index values
static const int8_t INDEX_MIN_VALUE = 1 * INDEX_SCALE_FACTOR;     // must be adjusted by the scale factor
static const int16_t INDEX_MAX_VALUE = 500 * INDEX_SCALE_FACTOR;  // must be adjusted by the scale factor

static inline const LogString *type_to_string(Sen6xType type) {
  switch (type) {
    case Sen6xType::SEN62:
      return LOG_STR("SEN62");
    case Sen6xType::SEN63C:
      return LOG_STR("SEN63C");
    case Sen6xType::SEN65:
      return LOG_STR("SEN65");
    case Sen6xType::SEN66:
      return LOG_STR("SEN66");
    case Sen6xType::SEN68:
      return LOG_STR("SEN68");
    default:
      return LOG_STR("SEN69C");
  }
}

static inline const LogString *status_to_string(Sen6xVocStatus status) {
  switch (status) {
    case Sen6xVocStatus::NOTHING:
      return LOG_STR("Nothing yet");
    case Sen6xVocStatus::WAITING:
      return LOG_STR("Waiting for timestamp");
    case Sen6xVocStatus::RESTORED:
    case Sen6xVocStatus::RESTORED_TIME:
      return LOG_STR("Previous state restored");
    case Sen6xVocStatus::UPDATED:
      return LOG_STR("New state updated");
    case Sen6xVocStatus::NO_PREF:
      return LOG_STR("No previous state");
    case Sen6xVocStatus::TOO_OLD:
      return LOG_STR("Previous state ignored");
    default:
      return LOG_STR("Error");
  }
}

// This function performs an in-place conversion of the provided buffer
// from uint16_t values to big endianness
static inline const char *sensirion_convert_to_string_in_place(uint16_t *array, size_t length) {
  for (size_t i = 0; i < length; i++) {
    array[i] = convert_big_endian(array[i]);
  }
  return reinterpret_cast<const char *>(array);
}

void Sen6xComponent::setup() {
  this->command_flag_ |= CMD_FLAG_SETUP;
  this->loop();
}

void Sen6xComponent::update() {
  if (this->loop_state_ == SetupStates::SM_IDLE) {
    this->command_flag_ |= CMD_FLAG_MEASUREMENT;
    this->loop();
  }
}

void Sen6xComponent::loop() {
  if (this->loop_state_ == SetupStates::SM_IDLE) {
    if (this->command_flag_ & CMD_FLAG_MEASUREMENT) {
      this->loop_state_ = SetupStates::SM_MEAS_INIT;
    } else if (this->command_flag_ & CMD_FLAG_HEATER) {
      this->loop_state_ = SetupStates::SM_HEAT_INIT;
    } else if (this->command_flag_ & CMD_FLAG_CO2_RECAL) {
      this->loop_state_ = SetupStates::SM_CO2_RECAL_INIT;
    } else if (this->command_flag_ & CMD_FLAG_CO2_PRESS) {
      this->loop_state_ = SetupStates::SM_CO2_PRESS_INIT;
    } else if (this->command_flag_ & CMD_FLAG_FAN_CLEAN) {
      this->loop_state_ = SetupStates::SM_FAN_INIT;
    } else if (this->command_flag_ & CMD_FLAG_TEMP_COMP) {
      this->loop_state_ = SetupStates::SM_TEMP_COMP_INIT;
    } else if (this->command_flag_ & CMD_FLAG_VOC_CHECK) {
      this->loop_state_ = SetupStates::SM_VOC_CHECK_INIT;
    } else if (this->command_flag_ & CMD_FLAG_SETUP) {
      this->loop_state_ = SetupStates::SM_SETUP_INIT;
    }
    this->state_time_ = App.get_loop_component_start_time();
    this->state_wait_time_ = 0;
  }
  if (this->state_wait_time_ > App.get_loop_component_start_time() - this->state_time_) {
    return;
  }
  switch (this->loop_state_) {
    case SetupStates::SM_IDLE:
      break;
    case SetupStates::SM_MEAS_INIT:
      ESP_LOGV(TAG, "SM_MEAS_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->meas_warning_ = false;
      this->state_wait_time_ = 20;  // all measurement states have a 20ms execution time
      if (!this->write_command(this->measurement_cmd_)) {
        ESP_LOGE(TAG, "Write Read Measurement command failed");
        this->meas_warning_ = true;
        this->loop_state_ = SetupStates::SM_MEAS_DONE;
      } else {
        this->loop_state_ = SetupStates::SM_MEAS_GET;
      }
      break;
    case SetupStates::SM_MEAS_GET:
      uint16_t measurements[10];
      ESP_LOGV(TAG, "SM_MEAS_GET State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(measurements, this->measurement_cmd_len_)) {
        ESP_LOGV(TAG, "Read Read Measurement data failed");
        this->meas_warning_ = true;
      } else {
        if (this->pm_1_0_sensor_ != nullptr) {
          ESP_LOGV(TAG, "pm_1_0 = 0x%.4x", measurements[0]);
          float pm_1_0 = measurements[0] == UINT16_MAX ? NAN : measurements[0] / 10.0f;
          this->pm_1_0_sensor_->publish_state(pm_1_0);
        }
        if (this->pm_2_5_sensor_ != nullptr) {
          ESP_LOGV(TAG, "pm_2_5 = 0x%.4x", measurements[1]);
          float pm_2_5 = measurements[1] == UINT16_MAX ? NAN : measurements[1] / 10.0f;
          this->pm_2_5_sensor_->publish_state(pm_2_5);
        }
        if (this->pm_4_0_sensor_ != nullptr) {
          ESP_LOGV(TAG, "pm_4_0 = 0x%.4x", measurements[2]);
          float pm_4_0 = measurements[2] == UINT16_MAX ? NAN : measurements[2] / 10.0f;
          this->pm_4_0_sensor_->publish_state(pm_4_0);
        }
        if (this->pm_10_0_sensor_ != nullptr) {
          ESP_LOGV(TAG, "pm_10_0 = 0x%.4x", measurements[3]);
          float pm_10_0 = measurements[3] == UINT16_MAX ? NAN : measurements[3] / 10.0f;
          this->pm_10_0_sensor_->publish_state(pm_10_0);
        }
        if (this->temperature_sensor_ != nullptr) {
          float temperature = static_cast<int16_t>(measurements[5]) / 200.0f;
          if (measurements[5] == INT16_MAX || measurements[5] == UINT16_MAX) {
            temperature = NAN;
          }
          ESP_LOGV(TAG, "temperature = 0x%.4x", measurements[5]);
          this->temperature_sensor_->publish_state(temperature);
        }
        if (this->humidity_sensor_ != nullptr) {
          float humidity = static_cast<int16_t>(measurements[4]) / 100.0f;
          if (measurements[4] == INT16_MAX || measurements[4] == UINT16_MAX) {
            humidity = NAN;
          }
          ESP_LOGV(TAG, "humidity = 0x%.4x", measurements[4]);
          this->humidity_sensor_->publish_state(humidity);
        }
        if (this->voc_sensor_ != nullptr) {
          ESP_LOGV(TAG, "voc = 0x%.4x", measurements[6]);
          int16_t voc_idx = static_cast<int16_t>(measurements[6]);
          float voc = (voc_idx < INDEX_MIN_VALUE || voc_idx > INDEX_MAX_VALUE) ? NAN : voc_idx / 10.0f;
          this->voc_sensor_->publish_state(voc);
        }
        if (this->nox_sensor_ != nullptr) {
          ESP_LOGV(TAG, "nox = 0x%.4x", measurements[7]);
          int16_t nox_idx = static_cast<int16_t>(measurements[7]);
          float nox = (nox_idx < INDEX_MIN_VALUE || nox_idx > INDEX_MAX_VALUE) ? NAN : nox_idx / 10.0f;
          this->nox_sensor_->publish_state(nox);
        }
        if (this->co2_sensor_ != nullptr) {
          if (this->type_.value() == Sen6xType::SEN63C || this->type_.value() == Sen6xType::SEN69C) {
            int16_t value = static_cast<int16_t>(measurements[6]);  // SEN63C reports as signed
            if (this->type_.value() == Sen6xType::SEN69C) {
              value = static_cast<int16_t>(measurements[9]);  // SEN69C reports as signed
            }
            ESP_LOGV(TAG, "co2 = 0x%.4x", value);
            float co2_1 = value == INT16_MAX ? NAN : value / 1.0f;
            this->co2_sensor_->publish_state(co2_1);
          } else {
            ESP_LOGV(TAG, "co2 = 0x%.4x", measurements[8]);  // SEN66 reports as unsigned
            float co2_2 = measurements[8] == UINT16_MAX ? NAN : measurements[8] / 1.0f;
            this->co2_sensor_->publish_state(co2_2);
          }
        }
        if (this->hcho_sensor_ != nullptr) {
          ESP_LOGV(TAG, "HCHO = 0x%.4x", measurements[8]);
          float hcho = measurements[8] == UINT16_MAX ? NAN : measurements[8] / 10.0f;
          this->hcho_sensor_->publish_state(hcho);
        }
      }
      if (this->store_voc_algorithm_state_.has_value() && this->store_voc_algorithm_state_.value() &&
          (App.get_loop_component_start_time() - this->voc_algorithm_state_time_) >=
              ALGORITHM_STATE_STORE_INTERVAL_MS) {
        this->voc_algorithm_state_time_ = App.get_loop_component_start_time();
        if (this->time_source_ != nullptr) {
          // add timestamp to VOC Algorithm Storage
          this->voc_algorithm_state_.epoch = this->time_source_->timestamp_now();
        }
        if (!this->write_command(CMD_VOC_ALGORITHM_STATE)) {
          ESP_LOGV(TAG, "Write VOC Algorithm State command failed");
          this->meas_warning_ = true;
          this->voc_algorithm_state_status_ = Sen6xVocStatus::ERROR;
        } else {
          this->loop_state_ = SetupStates::SM_MEAS_VOCA;
        }
      } else {
        this->loop_state_ = SetupStates::SM_MEAS_PRES;
      }
      break;
    case SetupStates::SM_MEAS_VOCA:
      ESP_LOGV(TAG, "SM_MEAS_VOCA State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(this->voc_algorithm_state_.state, 4)) {
        ESP_LOGV(TAG, "VOC Algorithm State read failed");
        this->voc_algorithm_state_status_ = Sen6xVocStatus::ERROR;
        this->state_wait_time_ = 0;
      } else {
        if (this->time_source_ != nullptr) {
          this->voc_algorithm_state_.epoch = this->time_source_->timestamp_now();
        }
        if (!this->pref_.save(&this->voc_algorithm_state_)) {
          ESP_LOGV(TAG, "VOC Algorithm State store failed");
          this->voc_algorithm_state_status_ = Sen6xVocStatus::ERROR;
        } else {
          ESP_LOGV(TAG, "VOC Algorithm State stored");
          this->voc_algorithm_state_status_ = Sen6xVocStatus::RESTORED;
        }
      }
      this->loop_state_ = SetupStates::SM_MEAS_PRES;
      break;
    case SetupStates::SM_MEAS_PRES:
      ESP_LOGV(TAG, "SM_MEAS_PRES State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->ambient_pressure_compensation_source_ != nullptr) {
        float pressure = this->ambient_pressure_compensation_source_->state;
        if (!std::isnan(pressure)) {
          uint16_t new_ambient_pressure = static_cast<uint16_t>(pressure);
          if (!write_ambient_pressure_compensation_(new_ambient_pressure)) {
            ESP_LOGV(TAG, "Write Ambient Pressure Compensation command failed");
            this->meas_warning_ = true;
          }
        }
      }
      this->loop_state_ = SetupStates::SM_MEAS_DONE;
      break;
    case SetupStates::SM_MEAS_DONE:
      ESP_LOGV(TAG, "SM_MEAS_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->meas_warning_ ? this->status_set_warning() : this->status_clear_warning();
      this->command_flag_ &= ~CMD_FLAG_MEASUREMENT;
      this->loop_state_ = SetupStates::SM_IDLE;
      break;
    case SetupStates::SM_HEAT_INIT:
      ESP_LOGV(TAG, "SM_HEAT_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      ESP_LOGD(TAG, "Activate Heater: Started");
      if (!this->stop_measurements_()) {
        ESP_LOGE(TAG, "Activate Heater: Stop Measurements failed");
        this->loop_state_ = SetupStates::SM_HEAT_DONE;
      } else {
        this->loop_state_ = SetupStates::SM_HEAT_ON;
      }
      this->state_wait_time_ = 20;
      break;
    case SetupStates::SM_HEAT_ON:
      ESP_LOGV(TAG, "SM_HEAT_ON State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_command(CMD_ACTIVATE_SHT_HEATER)) {
        ESP_LOGE(TAG, "Activate Heater: Command error");
      }
      this->state_wait_time_ = 2000;
      this->loop_state_ = SetupStates::SM_HEAT_START;
      break;
    case SetupStates::SM_HEAT_START:
      if (App.get_loop_component_start_time() - this->stop_time_ > 20000) {
        if (!this->start_measurements_()) {
          ESP_LOGE(TAG, "Activate Heater: Start Measurements failed");
        }
        this->state_wait_time_ = 50;
        this->loop_state_ = SetupStates::SM_HEAT_DONE;
      } else {
        float countdown = 20.f - (App.get_loop_component_start_time() - this->stop_time_) / 1000.f;
        ESP_LOGD(TAG, "Activate Heater: Waiting for sensor to cool %.0fs", countdown);
      }
      break;
    case SetupStates::SM_HEAT_DONE:
      ESP_LOGV(TAG, "SM_HEAT_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->command_flag_ &= ~CMD_FLAG_HEATER;
      this->loop_state_ = SetupStates::SM_IDLE;
      ESP_LOGD(TAG, "Activate Heater: Complete");
      break;
    case SetupStates::SM_CO2_RECAL_INIT:
      ESP_LOGV(TAG, "SM_CO2_RECAL_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      ESP_LOGD(TAG, "Forced CO₂ Recalibration: Started, co2=%d", this->co2_reference_);
      if (!this->stop_measurements_()) {
        ESP_LOGE(TAG, "Forced CO₂ Recalibration: Failed to Stop Measurements");
        this->loop_state_ = SetupStates::SM_CO2_RECAL_DONE;
      } else {
        this->loop_state_ = SetupStates::SM_CO2_RECAL_ON;
      }
      this->state_wait_time_ = 20;
      break;
    case SetupStates::SM_CO2_RECAL_ON:
      ESP_LOGV(TAG, "SM_CO2_RECAL_ON State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_command(CMD_PERFORM_FORCED_CO2_RECAL, this->co2_reference_)) {
        ESP_LOGE(TAG, "Forced CO₂ Recalibration: Failed to Start Recalibration");
        this->loop_state_ = SetupStates::SM_CO2_RECAL_START;
      } else {
        this->loop_state_ = SetupStates::SM_CO2_RECAL_WAIT;
        this->state_wait_time_ = 500;
      }
      break;
    case SetupStates::SM_CO2_RECAL_WAIT: {
      uint16_t correction;
      ESP_LOGV(TAG, "SM_CO2_RECAL_WAIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(correction)) {
        ESP_LOGE(TAG, "Forced CO₂ Recalibration: Failed to read correction");
      } else {
        if (correction == 0xFFFF) {
          ESP_LOGE(TAG, "Forced CO₂ Recalibration: Failure reported by sensor");
        } else {
          ESP_LOGD(TAG, "Forced CO₂ Recalibration: Success, corr=%d", static_cast<int32_t>(correction) - 0x8000);
        }
      }
      this->state_wait_time_ = 1420 - (App.get_loop_component_start_time() - this->stop_time_);
      this->loop_state_ = SetupStates::SM_CO2_RECAL_START;
      break;
    }
    case SetupStates::SM_CO2_RECAL_START:
      ESP_LOGV(TAG, "SM_HEAT_START State, stop_to_start_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->start_measurements_()) {
        ESP_LOGE(TAG, "Forced CO₂ Recalibration: Failed to Start Measurements");
      }
      this->loop_state_ = SetupStates::SM_CO2_RECAL_DONE;
      this->state_wait_time_ = 50;
      break;
    case SetupStates::SM_CO2_RECAL_DONE:
      ESP_LOGV(TAG, "SM_CO2_RECAL_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->command_flag_ &= ~CMD_FLAG_CO2_RECAL;
      this->loop_state_ = SetupStates::SM_IDLE;
      ESP_LOGD(TAG, "Forced CO₂ Recalibration: Complete");
      break;
    case SetupStates::SM_CO2_PRESS_INIT:
      ESP_LOGV(TAG, "SM_CO2_PRESS_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!write_ambient_pressure_compensation_(this->ambient_pressure_)) {
        ESP_LOGE(TAG, "Set Ambient Pressure Compensation: Failed");
      }
      this->state_wait_time_ = 20;
      this->loop_state_ = SetupStates::SM_CO2_PRESS_DONE;
      break;
    case SetupStates::SM_CO2_PRESS_DONE:
      ESP_LOGV(TAG, "SM_CO2_PRESS_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->command_flag_ &= ~CMD_FLAG_CO2_PRESS;
      this->loop_state_ = SetupStates::SM_IDLE;
      break;
    case SetupStates::SM_FAN_INIT:
      ESP_LOGV(TAG, "SM_FAN_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      ESP_LOGD(TAG, "Fan Cleaning: Started");
      if (!this->stop_measurements_()) {
        ESP_LOGE(TAG, "Fan Cleaning: Failed to Stop Measurements");
        this->loop_state_ = SetupStates::SM_FAN_DONE;
      } else {
        this->loop_state_ = SetupStates::SM_FAN_ON;
      }
      this->state_wait_time_ = 20;
      break;
    case SetupStates::SM_FAN_ON:
      ESP_LOGV(TAG, "SM_FAN_ON State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_command(CMD_START_CLEANING_FAN)) {
      } else {
        this->state_wait_time_ = 2000;
      }
      this->loop_state_ = SetupStates::SM_FAN_START;
      break;
    case SetupStates::SM_FAN_START:
      if (App.get_loop_component_start_time() - this->stop_time_ > 10000) {
        if (!this->start_measurements_()) {
          ESP_LOGE(TAG, "Activate Heater: Start Measurements failed");
        }
        this->state_wait_time_ = 50;
        this->loop_state_ = SetupStates::SM_FAN_DONE;
      } else {
        float countdown = 10.f - (App.get_loop_component_start_time() - this->stop_time_) / 1000.f;
        ESP_LOGD(TAG, "Fan Cleaning: Waiting for sensor to finish %.0fs", countdown);
      }
      break;
    case SetupStates::SM_FAN_DONE:
      ESP_LOGV(TAG, "SM_FAN_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->command_flag_ &= ~CMD_FLAG_FAN_CLEAN;
      this->loop_state_ = SetupStates::SM_IDLE;
      ESP_LOGD(TAG, "Fan Cleaning: Complete");
      break;
    case SetupStates::SM_TEMP_COMP_INIT:
      ESP_LOGV(TAG, "SM_TEMP_COMP_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_temperature_compensation_(this->temperature_compensation_.value())) {
        ESP_LOGE(TAG, "Set Temperature Compensation failed");
      }
      this->state_wait_time_ = 20;
      this->loop_state_ = SetupStates::SM_TEMP_COMP_DONE;
      break;
    case SetupStates::SM_TEMP_COMP_DONE:
      ESP_LOGD(TAG,
               "Temperature Compensation: Updated offset=%.3f, normalized_offset_slope=%.6f, time_constant=%u, "
               "slot=%u",
               this->temperature_compensation_.value().offset,
               this->temperature_compensation_.value().normalized_offset_slope,
               this->temperature_compensation_.value().time_constant, this->temperature_compensation_.value().slot);
      this->command_flag_ &= ~CMD_FLAG_TEMP_COMP;
      this->loop_state_ = SetupStates::SM_IDLE;
      break;
    case SetupStates::SM_VOC_CHECK_INIT: {
      ESP_LOGV(TAG, "SM_VOC_CHECK_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      time_t diff = this->time_source_->timestamp_now() - this->voc_algorithm_state_.epoch;
      if (diff <= ALGORITHM_STATE_MAX_AGE) {
        if (!this->write_command(CMD_VOC_ALGORITHM_STATE, this->voc_algorithm_state_.state, 4)) {
          ESP_LOGW(TAG, "VOC Algorithm State write to sensor failed");
          this->voc_algorithm_state_status_ = Sen6xVocStatus::ERROR;
        } else {
          ESP_LOGD(TAG, "VOC Algorithm State written to sensor, it is not too old");
          this->voc_algorithm_state_status_ = Sen6xVocStatus::RESTORED_TIME;
          this->voc_algorithm_state_time_ = App.get_loop_component_start_time() - diff * 1000;
          this->state_wait_time_ = 20;
        }
      } else {
        ESP_LOGD(TAG, "VOC Algorithm State ignored, it is too old");
        this->voc_algorithm_state_status_ = Sen6xVocStatus::TOO_OLD;
      }
      this->loop_state_ = SetupStates::SM_VOC_CHECK_DONE;
      break;
    }
    case SetupStates::SM_VOC_CHECK_DONE:
      this->command_flag_ &= ~CMD_FLAG_VOC_CHECK;
      this->loop_state_ = SetupStates::SM_IDLE;
      break;
    case SetupStates::SM_SETUP_INIT:
      ESP_LOGV(TAG, "SM_SETUP_INIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->state_wait_time_ = 100;
      this->loop_state_ = SetupStates::SM_SETUP_INIT_WAIT;
      break;
    case SetupStates::SM_SETUP_INIT_WAIT:
      ESP_LOGV(TAG, "SM_SETUP_INIT_WAIT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_command(CMD_GET_DATA_READY_STATUS)) {
        this->mark_failed(LOG_STR("Get Status failed"));
        return;
      }
      this->state_wait_time_ = 20;
      this->loop_state_ = SetupStates::SM_SETUP_GET_STATUS;
      break;
    case SetupStates::SM_SETUP_GET_STATUS:
      uint16_t raw_read_status;
      ESP_LOGV(TAG, "SM_SETUP_GET_STATUS State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(raw_read_status)) {
        this->mark_failed(LOG_STR("Get Status failed"));
        return;
      }
      // In order to query the device periodic measurement must be ceased
      if (raw_read_status) {
        ESP_LOGV(TAG, "Stopping periodic measurement");
        if (!this->stop_measurements_()) {
          this->mark_failed(LOG_STR("Stop Measurements failed"));
          return;
        }
      }
      this->loop_state_ = SetupStates::SM_SETUP_GET_SN;
      break;
    case SetupStates::SM_SETUP_GET_SN:
      ESP_LOGV(TAG, "SM_SETUP_GET_SN State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->write_command(CMD_GET_SERIAL_NUMBER)) {
        this->mark_failed(LOG_STR("Get Serial Number failed"));
        return;
      }
      this->loop_state_ = SetupStates::SM_SETUP_GET_SN_1;
      break;
    case SetupStates::SM_SETUP_GET_SN_1: {
      // Serial numbers are currently only 16 chars long, same on label, this could change
      uint16_t raw_serial_number[8];
      ESP_LOGV(TAG, "SM_SETUP_GET_SN_1 State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(raw_serial_number, 8)) {
        this->mark_failed(LOG_STR("Get Serial Number failed"));
        return;
      }
      // *serial_number is not null terminated, snprintf takes care of this
      const char *serial_number = sensirion_convert_to_string_in_place(raw_serial_number, 8);
      snprintf(this->serial_number_, sizeof(this->serial_number_), "%s", serial_number);
      ESP_LOGV(TAG, "Read Serial Number: %s", this->serial_number_);
      if (!this->write_command(CMD_GET_PRODUCT_NAME)) {
        this->mark_failed(LOG_STR("Get Product Name failed"));
        return;
      }
      this->loop_state_ = SetupStates::SM_SETUP_GET_PN;
      break;
    }
    case SetupStates::SM_SETUP_GET_PN: {
      // 8 chars is enough room for the at most 7 chars plus null
      uint16_t raw_product_name[4];
      ESP_LOGV(TAG, "SM_SETUP_GET_PN State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(raw_product_name, 4)) {
        this->mark_failed(LOG_STR("Get Product Name failed"));
        return;
      }
      const char *product_name = sensirion_convert_to_string_in_place(raw_product_name, 4);
      if (strlen(product_name) == 0) {
        // Can't verify configuration type matches connected sensor
        ESP_LOGW(TAG, "Product Name is empty");
      } else {
        // product name and type must match
        if (strncmp(product_name, LOG_STR_ARG(type_to_string(this->type_.value())), 10) != 0) {
          ESP_LOGE(TAG, "Product Name does not match: %.16s", product_name);
          this->mark_failed(LOG_STR("Product Name does not match"));
          return;
        }
      }
      ESP_LOGV(TAG, "Read Product Name: %.32s", product_name);
      if (!this->write_command(CMD_GET_FIRMWARE_VERSION)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->loop_state_ = SetupStates::SM_SETUP_GET_FW;
      break;
    }
    case SetupStates::SM_SETUP_GET_FW:
      uint16_t firmware;
      ESP_LOGV(TAG, "SM_SETUP_GET_FW State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (!this->read_data(&firmware, 1)) {
        this->mark_failed(LOG_STR("Get FW failed"));
        return;
      }
      this->firmware_minor_ = firmware & 0xFF;
      this->firmware_major_ = firmware >> 8;
      ESP_LOGV(TAG, "Read Firmware version: %u.%u", this->firmware_major_, this->firmware_minor_);
      this->loop_state_ = SetupStates::SM_SETUP_SET_VOCA;
      break;
    case SetupStates::SM_SETUP_SET_VOCA: {
      ESP_LOGV(TAG, "SM_SETUP_SET_VOCA State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      // Hash with serial number. Serial numbers are unique, so multiple sensors can be used without conflict
      // change the integer if Sen6xVocBaseline format changes
      uint32_t hash = fnv1a_hash_extend(1, fnv1a_hash(this->serial_number_));
      // algorithm state is actually uint8_t[8] but uint16_t[4] is the way this data is received from the sensor
      this->pref_ = global_preferences->make_preference<Sen6xVocBaseline>(hash, true);
      if (this->store_voc_algorithm_state_.has_value() && this->store_voc_algorithm_state_.value()) {
        if (this->pref_.load(&this->voc_algorithm_state_)) {
          if (this->time_source_ == nullptr) {
            if (!this->write_command(CMD_VOC_ALGORITHM_STATE, this->voc_algorithm_state_.state, 4)) {
              ESP_LOGW(TAG, "VOC Algorithm State write to sensor failed");
              this->voc_algorithm_state_status_ = Sen6xVocStatus::ERROR;
            } else {
              // we just restored the algorithm state without regard to age
              ESP_LOGV(TAG, "VOC Algorithm State written to sensor");
              this->voc_algorithm_state_status_ = Sen6xVocStatus::RESTORED;
            }
          } else {
            ESP_LOGV(TAG, "VOC Algorithm State loaded, waiting on timestamp");
            this->voc_algorithm_state_status_ = Sen6xVocStatus::WAITING;
            // callback when time is ready, needed only once hence state check
            this->time_source_->add_on_time_sync_callback([this] {
              if (this->voc_algorithm_state_status_ == Sen6xVocStatus::WAITING) {
                this->command_flag_ |= CMD_FLAG_VOC_CHECK;
              }
            });
          }
        } else {
          // hash has changed or preference didn't exist
          ESP_LOGV(TAG, "VOC Algorithm State does not exist");
          this->voc_algorithm_state_status_ = Sen6xVocStatus::NO_PREF;
        }
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_ACCEL;
      break;
    }
    case SetupStates::SM_SETUP_SET_ACCEL:
      ESP_LOGV(TAG, "SM_SETUP_SET_ACCEL State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->temperature_acceleration_.has_value()) {
        if (!this->write_temperature_acceleration_()) {
          this->mark_failed(LOG_STR("Set Temperature Acceleration failed"));
          return;
        }
        ESP_LOGV(TAG, "Set Temperature Acceleration: T1: %.1f T2: %.1f K: %.1f P: %.1f", accel.t1 / 10.0,
                 accel.t2 / 10.0, accel.k / 10.0, accel.p / 10.0);
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_VOCT;
      break;
    case SetupStates::SM_SETUP_SET_VOCT:
      ESP_LOGV(TAG, "SM_SETUP_SET_VOCT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->voc_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_VOC_ALGORITHM_TUNING, this->voc_tuning_params_.value())) {
          this->mark_failed(LOG_STR("Set VOC Algorithm Tuning failed"));
          return;
        }
        ESP_LOGV(TAG,
                 "Set VOC Algorithm Tuning Parameters: Index Offset: %u Learning Time Offset (hours): %u "
                 "Learning Time Gain (hours): %u Gating Max Duration (minutes): %u STD Initial: %u Gain Factor: %u",
                 tuning_params.index_offset, tuning_params.learning_time_offset_hours,
                 tuning_params.learning_time_gain_hours, tuning_params.gating_max_duration_minutes,
                 tuning_params.std_initial, tuning_params.gain_factor);
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_NOXT;
      break;
    case SetupStates::SM_SETUP_SET_NOXT:
      ESP_LOGV(TAG, "SM_SETUP_SET_NOXT State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->nox_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_NOX_ALGORITHM_TUNING, this->nox_tuning_params_.value())) {
          this->mark_failed(LOG_STR("Set NOx Algorithm Tuning failed"));
          return;
        }
        ESP_LOGV(TAG,
                 "Set NOx Algorithm Tuning Parameters: Index Offset: %u Learning Time Offset (hours): %u "
                 "Gating Max Duration (minutes): %u Gain Factor: %u",
                 tuning_params.index_offset, tuning_params.learning_time_offset_hours,
                 tuning_params.gating_max_duration_minutes, tuning_params.gain_factor);
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_TP;
      break;
    case SetupStates::SM_SETUP_SET_TP:
      ESP_LOGV(TAG, "SM_SETUP_SET_TP State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->temperature_compensation_.has_value()) {
        if (!this->write_temperature_compensation_(this->temperature_compensation_.value())) {
          this->mark_failed(LOG_STR("Set Temperature Compensation failed"));
          return;
        }
        ESP_LOGV(TAG,
                 "Set Temperature Compensation: Offset: %.3f Normalized Offset Slope: %.6f "
                 "    Time Constant: %u",
                 comp.offset / 200.0, comp.normalized_offset_slope / 10000.0, comp.time_constant);
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_CO2ASC;
      break;
    case SetupStates::SM_SETUP_SET_CO2ASC:
      ESP_LOGV(TAG, "SM_SETUP_SET_CO2ASC State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->auto_self_calibration_.has_value()) {
        if (!this->write_command(CMD_CO2_SENSOR_AUTO_SELF_CAL, this->auto_self_calibration_.value() ? 0x01 : 0x00)) {
          this->mark_failed(LOG_STR("Set CO₂ Auto Self Calibration failed"));
          return;
        }
        ESP_LOGV(TAG, "Set CO₂ Auto Self Calibration: %s", TRUEFALSE(this->auto_self_calibration_.value()));
      }
      this->loop_state_ = SetupStates::SM_SETUP_SET_CO2AC;
      break;
    case SetupStates::SM_SETUP_SET_CO2AC:
      ESP_LOGV(TAG, "SM_SETUP_SET_CO2AC State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      if (this->altitude_compensation_.has_value()) {
        if (!this->write_command(CMD_SENSOR_ALTITUDE, this->altitude_compensation_.value())) {
          this->mark_failed(LOG_STR("Set CO₂ Altitude Compensation failed"));
          return;
        }
        ESP_LOGV(TAG, "Set CO₂ Altitude Compensation %s", this->altitude_compensation_);
      }
      this->state_wait_time_ = 1420 - (App.get_loop_component_start_time() - this->stop_time_);
      this->loop_state_ = SetupStates::SM_SETUP_START_MEAS;
      break;
    case SetupStates::SM_SETUP_START_MEAS:
      ESP_LOGV(TAG, "SM_SETUP_START_MEAS State, stop_to_start_delay=1420ms, actual=%ums",
               App.get_loop_component_start_time() - this->stop_time_);
      if (!this->start_measurements_()) {
        this->mark_failed(LOG_STR("Start Measurements failed"));
        return;
      }
      this->state_wait_time_ = 50;
      this->loop_state_ = SetupStates::SM_SETUP_DONE;
      break;
    case SetupStates::SM_SETUP_DONE:
      ESP_LOGV(TAG, "SM_SETUP_DONE State, requested_delay=%ums, actual=%ums", this->state_wait_time_,
               App.get_loop_component_start_time() - this->state_time_);
      this->loop_state_ = SetupStates::SM_IDLE;
      this->command_flag_ &= ~CMD_FLAG_SETUP;
      ESP_LOGD(TAG, "Initialized");
      break;
  }
  this->state_time_ = App.get_loop_component_start_time();
}

void Sen6xComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "SEN6X:\n"
                "  Initialized: %s\n"
                "  Model: %s\n"
                "  Update Interval: %ums\n"
                "  Serial number: %s\n"
                "  Firmware version: %u.%u",
                TRUEFALSE(this->is_initialized()), LOG_STR_ARG(type_to_string(this->type_.value())),
                this->update_interval_, this->serial_number_, this->firmware_major_, this->firmware_minor_);
  if (this->temperature_compensation_.has_value()) {
    TemperatureCompensation comp = this->temperature_compensation_.value();
    ESP_LOGCONFIG(TAG,
                  "  Temperature Compensation:\n"
                  "    Offset (°C): %.3f\n"
                  "    Normalized Offset Slope: %.6f\n"
                  "    Time Constant (seconds): %u",
                  comp.offset / 200.0, comp.normalized_offset_slope / 10000.0, comp.time_constant);
  }
  if (this->temperature_acceleration_.has_value()) {
    TemperatureAcceleration accel = this->temperature_acceleration_.value();
    ESP_LOGCONFIG(TAG,
                  "  Temperature Acceleration:\n"
                  "    T1 (seconds): %.1f\n"
                  "    T2 (seconds): %.1f\n"
                  "    K: %.1f\n"
                  "    P: %.1f",
                  accel.t1 / 10.0, accel.t2 / 10.0, accel.k / 10.0, accel.p / 10.0);
  }
  LOG_SENSOR("  ", "PM  1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM  2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM  4.0", this->pm_4_0_sensor_);
  LOG_SENSOR("  ", "PM 10.0", this->pm_10_0_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "VOC", this->voc_sensor_);
  if (this->store_voc_algorithm_state_.has_value()) {
    char age_buf[32] = "";
    char hex_buf[20];
    char state_buf[32] = "";
    uint32_t diff_seconds = 0;
    if (((this->voc_algorithm_state_status_ == Sen6xVocStatus::RESTORED_TIME) ||
         (this->voc_algorithm_state_status_ == Sen6xVocStatus::TOO_OLD)) &&
        (this->time_source_ != nullptr) && (this->time_source_->timestamp_now() != 0)) {
      diff_seconds = this->time_source_->timestamp_now() - this->voc_algorithm_state_.epoch;
    } else if (this->voc_algorithm_state_status_ == Sen6xVocStatus::RESTORED) {
      diff_seconds = (App.get_loop_component_start_time() - this->voc_algorithm_state_time_) / 1000;
    }
    if (diff_seconds != 0) {
      uint8_t seconds = diff_seconds % 60;
      uint8_t minutes = (diff_seconds / 60) % 60;
      uint32_t hours = diff_seconds / 3600;
      snprintf(age_buf, sizeof(age_buf), "\n      Age: %02d:%02d:%02d", hours, minutes, seconds);
      format_hex_pretty_to(hex_buf, this->voc_algorithm_state_.state, 4, 0);
      snprintf(state_buf, sizeof(state_buf), "\n      State: %s", hex_buf);
    }
    ESP_LOGCONFIG(TAG,
                  "    Store Algorithm State: %s\n"
                  "      Status: %s%s%s",
                  this->time_source_ != nullptr ? "Enabled, time enhanced" : "Enabled",
                  status_to_string(this->voc_algorithm_state_status_), age_buf, state_buf);
  }
  if (this->voc_tuning_params_.has_value()) {
    GasTuning tuning_params = this->voc_tuning_params_.value();
    ESP_LOGCONFIG(TAG,
                  "    Algorithm Tuning Parameters:\n"
                  "      Index Offset: %u\n"
                  "      Learning Time Offset (hours): %u\n"
                  "      Learning Time Gain (hours): %u\n"
                  "      Gating Max Duration (minutes): %u\n"
                  "      STD Initial: %u\n"
                  "      Gain Factor: %u",
                  tuning_params.index_offset, tuning_params.learning_time_offset_hours,
                  tuning_params.learning_time_gain_hours, tuning_params.gating_max_duration_minutes,
                  tuning_params.std_initial, tuning_params.gain_factor);
  }
  LOG_SENSOR("  ", "NOx", this->nox_sensor_);
  if (this->nox_tuning_params_.has_value()) {
    GasTuning tuning_params = this->nox_tuning_params_.value();
    ESP_LOGCONFIG(TAG,
                  "    Algorithm Tuning Parameters:\n"
                  "      Index Offset: %u\n"
                  "      Learning Time Offset (hours): %u\n"
                  "      Gating Max Duration (minutes): %u\n"
                  "      Gain Factor: %u",
                  tuning_params.index_offset, tuning_params.learning_time_offset_hours,
                  tuning_params.gating_max_duration_minutes, tuning_params.gain_factor);
  }
  LOG_SENSOR("  ", "CO₂", this->co2_sensor_);
  if (this->auto_self_calibration_.has_value()) {
    ESP_LOGCONFIG(TAG, "    Automatic self calibration: %s", TRUEFALSE(this->auto_self_calibration_.value()));
  }
  if (this->ambient_pressure_compensation_source_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Ambient Pressure Compensation Source: %s",
                  this->ambient_pressure_compensation_source_->get_name().c_str());
  } else if (this->altitude_compensation_.has_value()) {
    ESP_LOGCONFIG(TAG, "    Altitude Compensation: %d", this->altitude_compensation_.value());
  }
  LOG_SENSOR("  ", "HCHO", this->hcho_sensor_);
}

bool Sen6xComponent::is_initialized() const { return !(this->command_flag_ & CMD_FLAG_SETUP); }

bool Sen6xComponent::start_measurements_() {
  auto result = this->write_command(CMD_START_MEASUREMENTS);
  if (result) {
    if (this->is_initialized()) {
      ESP_LOGD(TAG, "Measurements Enabled");
    }
  }
  return result;
}

bool Sen6xComponent::stop_measurements_() {
  auto result = this->write_command(CMD_STOP_MEASUREMENTS);
  this->stop_time_ = App.get_loop_component_start_time();
  if (result) {
    if (this->is_initialized()) {
      ESP_LOGD(TAG, "Measurements Stopped");
    }
  }
  return result;
}

bool Sen6xComponent::write_tuning_parameters_(uint16_t i2c_command, const GasTuning &tuning) {
  uint16_t params[6];
  params[0] = tuning.index_offset;
  params[1] = tuning.learning_time_offset_hours;
  params[2] = tuning.learning_time_gain_hours;
  params[3] = tuning.gating_max_duration_minutes;
  params[4] = tuning.std_initial;
  params[5] = tuning.gain_factor;
  return this->write_command(i2c_command, params, 6);
}

bool Sen6xComponent::write_temperature_compensation_(const TemperatureCompensation &compensation) {
  uint16_t params[4];
  params[0] = static_cast<uint16_t>(compensation.offset);
  params[1] = static_cast<uint16_t>(compensation.normalized_offset_slope);
  params[2] = compensation.time_constant;
  params[3] = compensation.slot;
  return this->write_command(CMD_TEMPERATURE_COMPENSATION, params, 4);
}

bool Sen6xComponent::write_temperature_acceleration_() {
  uint16_t params[4];
  auto accel_param = this->temperature_acceleration_.value();
  params[0] = accel_param.k;
  params[1] = accel_param.p;
  params[2] = accel_param.t1;
  params[3] = accel_param.t2;
  return this->write_command(CMD_TEMPERATURE_ACCEL_PARAMETERS, params, 4);
}

bool Sen6xComponent::write_ambient_pressure_compensation_(uint16_t pressure_in_hpa) {
  if (abs(this->ambient_pressure_compensation_ - pressure_in_hpa) > 1) {
    this->ambient_pressure_compensation_ = pressure_in_hpa;
    if (!this->write_command(CMD_AMBIENT_PRESSURE, pressure_in_hpa)) {
      return false;
    }
    ESP_LOGD(TAG, "Set Ambient Pressure Compensation: success, %d hPa", pressure_in_hpa);
  }
  return true;
}

void Sen6xComponent::set_type(Sen6xType type) {
  this->type_ = type;
  switch (type) {
    case Sen6xType::SEN62:
      this->measurement_cmd_ = SEN62_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 6;
      break;
    case Sen6xType::SEN63C:
      this->measurement_cmd_ = SEN63C_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 7;
      break;
    case Sen6xType::SEN65:
      this->measurement_cmd_ = SEN65_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 8;
      break;
    case Sen6xType::SEN66:
      this->measurement_cmd_ = SEN66_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 9;
      break;
    case Sen6xType::SEN68:
      this->measurement_cmd_ = SEN68_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 9;
      break;
    case Sen6xType::SEN69C:
      this->measurement_cmd_ = SEN69C_CMD_READ_MEASUREMENT;
      this->measurement_cmd_len_ = 10;
      break;
  }
}

bool Sen6xComponent::activate_heater() {
  this->command_flag_ |= CMD_FLAG_HEATER;
  return true;
}

bool Sen6xComponent::perform_forced_co2_recalibration(uint16_t co2) {
  if (this->co2_sensor_ == nullptr) {
    ESP_LOGE(TAG, "CO₂ sensor does not exist");
    return false;
  }
  this->co2_reference_ = co2;
  this->command_flag_ |= CMD_FLAG_CO2_RECAL;
  return true;
}

bool Sen6xComponent::set_ambient_pressure_compensation(uint16_t pressure_in_hpa) {
  if (this->co2_sensor_ == nullptr) {
    ESP_LOGE(TAG, "CO₂ sensor does not exist");
    return false;
  }
  this->ambient_pressure_ = pressure_in_hpa;
  this->command_flag_ |= CMD_FLAG_CO2_PRESS;
  return true;
}

bool Sen6xComponent::start_fan_cleaning() {
  this->command_flag_ |= CMD_FLAG_FAN_CLEAN;
  return true;
}

bool Sen6xComponent::set_temperature_compensation(float offset, float normalized_offset_slope, uint16_t time_constant,
                                                  uint8_t slot) {
  TemperatureCompensation comp(offset, normalized_offset_slope, time_constant, slot);
  this->temperature_compensation_ = comp;
  this->command_flag_ |= CMD_FLAG_TEMP_COMP;
  return true;
}

}  // namespace sen6x
}  // namespace esphome
