#include "sen6x.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace sen6x {

static const char *const TAG = "sen6x";

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
    case Sen6xType::SEN69C:
      return LOG_STR("SEN69C");
    default:
      return LOG_STR("UNKNOWN");
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
  // the sensor needs 100 ms after power up before i2c bus communication can be established
  this->set_timeout(100, [this]() { this->internal_setup_(SetupStates::SM_START); });
}

void Sen6xComponent::internal_setup_(SetupStates state) {
  switch (state) {
    case SetupStates::SM_START:
      // Check if measurement is ready before reading the value
      if (!this->write_command(CMD_GET_DATA_READY_STATUS)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_START_1); });
      return;
    case SetupStates::SM_START_1:
      uint16_t raw_read_status;
      if (!this->read_data(raw_read_status)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      // In order to query the device periodic measurement must be ceased
      if (raw_read_status) {
        ESP_LOGV(TAG, "Stopping periodic measurement");
        if (!this->stop_measurements_()) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
        // stop measurement was issued
        this->start_time_ = App.get_loop_component_start_time() - 20;
      } else {
        // stop measurement was not issued
        this->start_time_ = App.get_loop_component_start_time() - 1400;
      }
      // even if waiting for stop measurement we can still issue other commands (20ms is fine)
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_GET_SN); });
      return;
    case SetupStates::SM_GET_SN:
      // we will need to measure stop measurements elapsed time
      if (!this->write_command(CMD_GET_SERIAL_NUMBER)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_GET_SN_1); });
      return;
    case SetupStates::SM_GET_SN_1: {
      uint16_t raw_serial_number[8];
      // Serial numbers are currently only 16 chars long, same on label, this could change
      if (!this->read_data(raw_serial_number, 8)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      // *serial_number is not null terminated, snprintf takes care of this
      const char *serial_number = sensirion_convert_to_string_in_place(raw_serial_number, 8);
      snprintf(this->serial_number_, sizeof(this->serial_number_), "%s", serial_number);
      ESP_LOGV(TAG, "Read Serial Number: %s", this->serial_number_);

      if (!this->write_command(CMD_GET_PRODUCT_NAME)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_GET_PN); });
      return;
    }
    case SetupStates::SM_GET_PN: {
      uint16_t raw_product_name[4];
      // 8 chars is enough room for the at most 7 chars plus null
      if (!this->read_data(raw_product_name, 4)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
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
          this->mark_failed(LOG_STR("Product Name failed"));
          return;
        }
      }
      ESP_LOGV(TAG, "Read Product Name: %.32s", product_name);

      if (!this->write_command(CMD_GET_FIRMWARE_VERSION)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_GET_FW); });
      return;
    }
    case SetupStates::SM_GET_FW: {
      uint16_t firmware;
      if (!this->read_data(&firmware, 1)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->firmware_minor_ = firmware & 0xFF;
      this->firmware_major_ = firmware >> 8;
      ESP_LOGV(TAG, "Read Firmware version: %u.%u", this->firmware_major_, this->firmware_minor_);
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_VOCB); });
      return;
    }
    case SetupStates::SM_SET_VOCB:
      if (this->store_voc_algorithm_state_.has_value() && this->store_voc_algorithm_state_.value()) {
        // Hash with serial number. Serial numbers are unique, so multiple sensors can be used without conflict
        uint32_t hash = fnv1a_hash(this->serial_number_);
        // algorithm state is actually uint8_t[8] but uint16_t[4] is the way this data is received from the sensor
        this->pref_ = global_preferences->make_preference<uint16_t[4]>(hash, true);
        this->start_time_ = App.get_loop_component_start_time();
        if (this->pref_.load(&this->voc_algorithm_state_)) {
          if (!this->write_command(CMD_VOC_ALGORITHM_STATE, this->voc_algorithm_state_, 4)) {
            ESP_LOGE(TAG, "VOC Algorithm State write to sensor failed");
            this->voc_algorithm_error_ = true;
          } else {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
            char hex_buf[5 * 4];
            format_hex_pretty_to(hex_buf, this->baseline_state_, 4, 0);
            ESP_LOGV(TAG, "VOC Algorithm State loaded: %s", hex_buf);
#endif
            return;
          }
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_ACCEL); });
      return;
    case SetupStates::SM_SET_ACCEL:
      if (this->temperature_acceleration_.has_value()) {
        if (!this->write_temperature_acceleration_()) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_VOCT); });
      return;
    case SetupStates::SM_SET_VOCT:
      if (this->voc_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_VOC_ALGORITHM_TUNING, this->voc_tuning_params_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_NOXT); });
      return;
    case SetupStates::SM_SET_NOXT:
      if (this->nox_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_NOX_ALGORITHM_TUNING, this->nox_tuning_params_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_TP); });
      return;
    case SetupStates::SM_SET_TP:
      if (this->temperature_compensation_.has_value()) {
        if (!this->write_temperature_compensation_(this->temperature_compensation_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_CO2ASC); });
      return;
    case SetupStates::SM_SET_CO2ASC:
      if (this->auto_self_calibration_.has_value()) {
        if (!this->write_command(CMD_CO2_SENSOR_AUTO_SELF_CAL, this->auto_self_calibration_.value() ? 0x01 : 0x00)) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SetupStates::SM_SET_CO2AC); });
      return;
    case SetupStates::SM_SET_CO2AC:
      if (this->altitude_compensation_.has_value()) {
        if (!this->write_command(CMD_SENSOR_ALTITUDE, this->altitude_compensation_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed();
          return;
        }
      }
      auto delay = if App.get_loop_component_start_time() - this->start_time_ + 1400;
      ESP_LOGE(TAG, "Delay :%ums", delay);
      this->set_timeout(delay, [this]() { this->internal_setup_(SetupStates::SM_START_MEAS); });
      return;
    case SetupStates::SM_START_MEAS:
      if (!this->start_measurements_()) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed();
        return;
      }
      this->set_timeout(50, [this]() { this->internal_setup_(SetupStates::SM_DONE); });
      return;
    case SetupStates::SM_DONE:
      this->initialized_ = true;
      ESP_LOGD(TAG, "Initialized");
      ESP_LOGE(TAG, "End time :%ums", App.get_loop_component_start_time());
      return;
  }
}

void Sen6xComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "SEN6X:\n"
                "  Initialized: %s\n"
                "  Model: %s\n"
                "  Update Interval: %ums\n"
                "  Serial number: %s\n"
                "  Firmware version: %u.%u",
                TRUEFALSE(this->initialized_), LOG_STR_ARG(type_to_string(this->type_.value())), this->update_interval_,
                this->serial_number_, this->firmware_major_, this->firmware_minor_);
  if (this->temperature_compensation_.has_value()) {
    TemperatureCompensation comp = this->temperature_compensation_.value();
    ESP_LOGCONFIG(TAG,
                  "  Temperature Compensation:\n"
                  "    Offset: %.3f\n"
                  "    Normalized Offset Slope: %.6f\n"
                  "    Time Constant: %u",
                  comp.offset / 200.0, comp.normalized_offset_slope / 10000.0, comp.time_constant);
  }
  if (this->temperature_acceleration_.has_value()) {
    TemperatureAcceleration accel = this->temperature_acceleration_.value();
    ESP_LOGCONFIG(TAG,
                  "  Temperature Acceleration:\n"
                  "    T1: %.1f\n"
                  "    T2: %.1f\n"
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
    char hex_buf[5 * 4];
    format_hex_pretty_to(hex_buf, this->voc_algorithm_state_, 4, 0);
    ESP_LOGCONFIG(TAG,
                  "    Store Algorithm State: %s\n"
                  "      Error: %s\n"
                  "      State: %s\n",
                  TRUEFALSE(this->store_voc_algorithm_state_.value()), TRUEFALSE(this->voc_algorithm_error_), hex_buf);
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
    ESP_LOGCONFIG(TAG, "    Automatic self calibration: %s", ONOFF(this->auto_self_calibration_.value()));
  }
  if (this->ambient_pressure_compensation_source_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Ambient Pressure Compensation Source: %s",
                  this->ambient_pressure_compensation_source_->get_name().c_str());
  } else if (this->altitude_compensation_.has_value()) {
    ESP_LOGCONFIG(TAG, "    Altitude Compensation: %d", this->altitude_compensation_.value());
  }
  LOG_SENSOR("  ", "HCHO", this->hcho_sensor_);
}

void Sen6xComponent::update() {
  if (!this->initialized_ || !this->running_ || this->busy_)
    return;
  uint16_t cmd;
  uint8_t length;
  this->updating_ = true;
  switch (this->type_.value()) {
    case Sen6xType::SEN62:
      cmd = SEN62_CMD_READ_MEASUREMENT;
      length = 6;
      break;
    case Sen6xType::SEN63C:
      cmd = SEN63C_CMD_READ_MEASUREMENT;
      length = 7;
      break;
    case Sen6xType::SEN65:
      cmd = SEN65_CMD_READ_MEASUREMENT;
      length = 8;
      break;
    case Sen6xType::SEN66:
      cmd = SEN66_CMD_READ_MEASUREMENT;
      length = 9;
      break;
    case Sen6xType::SEN68:
      cmd = SEN68_CMD_READ_MEASUREMENT;
      length = 9;
      break;
    case Sen6xType::SEN69C:
      cmd = SEN69C_CMD_READ_MEASUREMENT;
      length = 10;
      break;
    default:
      ESP_LOGE(TAG, "Unsupported model");
      this->status_set_warning();
      return;
  }
  if (!this->write_command(cmd)) {
    ESP_LOGE(TAG, "Write Read Measurement command failed");
    this->status_set_warning();
    this->updating_ = false;
    return;
  }
  this->set_timeout(20, [this, length]() {
    uint16_t measurements[10];
    if (!this->read_data(measurements, length)) {
      ESP_LOGV(TAG, "Read Read Measurement data failed");
      this->status_set_warning();
      this->updating_ = false;
      return;
    }
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

    if (this->store_voc_algorithm_state_.has_value() && this->store_voc_algorithm_state_.value() &&
        (App.get_loop_component_start_time() - this->start_time_) >= SHORTEST_BASELINE_STORE_INTERVAL) {
      this->start_time_ = App.get_loop_component_start_time();
      if (!this->write_command(CMD_VOC_ALGORITHM_STATE)) {
        ESP_LOGV(TAG, "Write VOC Algorithm State command failed");
        this->status_clear_warning();
        this->voc_algorithm_error_ = true;
        this->updating_ = false;
      } else {
        this->set_timeout(20, [this]() {
          if (!this->read_data(this->voc_algorithm_state_, 4)) {
            ESP_LOGV(TAG, "Read VOC Algorithm State data failed");
            this->voc_algorithm_error_ = true;
          } else {
            if (!this->pref_.save(&this->voc_algorithm_state_)) {
              ESP_LOGV(TAG, "Store VOC Algorithm State failed");
              this->voc_algorithm_error_ = true;
            } else {
              ESP_LOGV(TAG, "VOC Algorithm State saved");
              this->voc_algorithm_error_ = false;
            }
            this->status_clear_warning();
          }
          this->updating_ = false;
        });
      }
    } else {
      if (this->ambient_pressure_compensation_source_ != nullptr) {
        float pressure = this->ambient_pressure_compensation_source_->state;
        if (!std::isnan(pressure)) {
          uint16_t new_ambient_pressure = static_cast<uint16_t>(pressure);
          if (!write_ambient_pressure_compensation_(new_ambient_pressure)) {
            ESP_LOGV(TAG, "Write Ambient Pressure Compensation command failed");
            this->status_set_warning();
            this->updating_ = false;
            return;
          }
        }
      }
    }
  });
}

bool Sen6xComponent::has_co2_() const {
  return (this->type_.value() == Sen6xType::SEN63C || this->type_.value() == Sen6xType::SEN66 ||
          this->type_.value() == Sen6xType::SEN69C);
}

bool Sen6xComponent::start_measurements_() {
  auto result = this->write_command(CMD_START_MEASUREMENTS);
  if (result) {
    this->running_ = true;
    if (this->initialized_) {
      ESP_LOGD(TAG, "Measurements Enabled");
    }
  }
  return result;
}

bool Sen6xComponent::stop_measurements_() {
  auto result = this->write_command(CMD_STOP_MEASUREMENTS);
  if (result) {
    this->running_ = false;
    if (this->initialized_) {
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
    ESP_LOGD(TAG, "Ambient Pressure Compensation updated, %d hPa", pressure_in_hpa);
  }
  return true;
}

// Will return false if the sensor is busy or not initialized or does not support CO2
bool Sen6xComponent::set_ambient_pressure_compensation(uint16_t pressure_in_hpa) {
  if (!has_co2_()) {
    ESP_LOGE(TAG, "Set Ambient Pressure Compensation is not supported");
    return false;
  }
  if (this->busy_ || !this->initialized_) {
    ESP_LOGW(TAG, "Ambient Pressure Compensation aborted, sensor is busy");
    return false;  // only one action at a time and must be initialized
  }
  this->busy_ = true;
  this->set_timeout(75, [this, pressure_in_hpa]() {  // let any update in progress finish
    if (!write_ambient_pressure_compensation_(pressure_in_hpa)) {
      ESP_LOGE(TAG, "Ambient Pressure Compensation failed");
      this->busy_ = false;
    } else {
      this->set_timeout(20, [this]() { this->busy_ = false; });
    }
  });
  return true;
}

// Will return false if the sensor is busy or not initialized
bool Sen6xComponent::start_fan_cleaning() {
  if (this->busy_ || !this->initialized_) {
    ESP_LOGW(TAG, "Fan Autoclean aborted, sensor is busy");
    return false;
  }
  this->busy_ = true;
  this->set_timeout(75, [this]() {
    ESP_LOGD(TAG, "Fan Autoclean started (12s)");
    // measurements must be stopped
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, "Fan Autoclean failed");
      this->busy_ = false;
    } else {
      this->set_timeout(1400, [this]() {
        if (!this->write_command(CMD_START_CLEANING_FAN)) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->start_measurements_();
          ESP_LOGE(TAG, "Fan Autoclean failed");
          this->set_timeout(50, [this]() { this->busy_ = false; });
        } else {
          this->set_timeout(10000, [this]() {
            if (!this->start_measurements_()) {
              ESP_LOGE(TAG, "Fan Autoclean failed");
              this->busy_ = false;
            } else {
              ESP_LOGD(TAG, "Fan Autoclean finished");
              this->set_timeout(50, [this]() { this->busy_ = false; });
            }
          });
        }
      });
    }
  });
  return true;
}

// Will return false if the sensor is busy or not initialized
bool Sen6xComponent::activate_heater() {
  if (this->busy_ || !this->initialized_) {
    ESP_LOGW(TAG, "Activate Heater aborted, sensor is busy");
    return false;
  }
  this->busy_ = true;
  this->set_timeout(75, [this]() {
    ESP_LOGD(TAG, "Activate Heater started (22s)");
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, "Activate Heater failed");
      this->busy_ = false;
      return;
    }
    this->set_timeout(1400, [this]() {
      if (!this->write_command(CMD_ACTIVATE_SHT_HEATER)) {
        ESP_LOGE(TAG, "Activate Heater failed");
        this->start_measurements_();
        this->set_timeout(50, [this]() { this->busy_ = false; });
      } else {
        this->set_timeout(20000, [this]() {
          if (!this->start_measurements_()) {
            ESP_LOGE(TAG, "Activate Heater failed");
          } else {
            ESP_LOGD(TAG, "Activate Heater finished");
          }
          this->busy_ = false;
          this->set_timeout(50, [this]() { this->busy_ = false; });
        });
      }
    });
  });
  return true;
}

// Will return false if the sensor is busy or not initialized or does not support CO2
bool Sen6xComponent::perform_forced_co2_recalibration(uint16_t co2) {
  if (!this->has_co2_()) {
    ESP_LOGE(TAG, "Forced CO₂ Recalibration is not supported");
    return false;
  }
  if (this->busy_ || !this->initialized_) {
    ESP_LOGW(TAG, "Forced CO₂ Recalibration aborted, sensor is busy");
    return false;
    ;
  }
  this->busy_ = true;
  this->set_timeout(75, [this, co2]() {
    ESP_LOGD(TAG, "Forced CO₂ Recalibration started, co2=%d", co2);
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, "Forced CO₂ Recalibration failed");
      this->busy_ = false;
    } else {
      this->set_timeout(1400, [this, co2]() {
        if (!this->write_command(CMD_PERFORM_FORCED_CO2_RECAL, co2)) {
          this->start_measurements_();
          ESP_LOGE(TAG, "Forced CO₂ Recalibration failed");
          this->set_timeout(50, [this]() { this->busy_ = false; });
        } else {
          this->set_timeout(500, [this]() {
            uint16_t correction = 0;
            if (!this->read_data(correction)) {
              ESP_LOGE(TAG, "Forced CO₂ Recalibration failed");
            } else {
              if (correction == 0xFFFF) {
                ESP_LOGE(TAG, "Forced CO₂ Recalibration failed");  // sensor reported failure
              } else {
                ESP_LOGD(TAG, "Forced CO₂ Recalibration finished, corr=%d", static_cast<int32_t>(correction) - 0x8000);
              }
            }
            if (!this->start_measurements_()) {
              ESP_LOGE(TAG, "Forced CO₂ Recalibration failed");
            }
            this->set_timeout(50, [this]() { this->busy_ = false; });
          });
        }
      });
    }
  });
  return true;
}

// Will return false if the sensor is busy or not initialized
bool Sen6xComponent::set_temperature_compensation(float offset, float normalized_offset_slope, uint16_t time_constant,
                                                  uint8_t slot) {
  TemperatureCompensation comp(offset, normalized_offset_slope, time_constant, slot);
  this->temperature_compensation_ = comp;
  if (!this->initialized_) {
    return false;  // setup will apply this temperature compensation
  }
  if (this->busy_) {
    ESP_LOGW(TAG, "Set Temperature Compensation aborted, sensor is busy");
    return false;
  }
  this->busy_ = true;
  this->set_timeout(75, [this, comp, offset, normalized_offset_slope, time_constant, slot]() {
    ESP_LOGD(
        TAG,
        "Set Temperature Compensation updated, offset=%.3f, normalized_offset_slope=%.6f, time_constant=%u, slot=%u",
        offset, normalized_offset_slope, time_constant, slot);
    if (!this->write_temperature_compensation_(comp)) {
      ESP_LOGE(TAG, "Set Temperature Compensation failed");
    }
    this->set_timeout(50, [this]() { this->busy_ = false; });
  });
  return true;
}

}  // namespace sen6x
}  // namespace esphome