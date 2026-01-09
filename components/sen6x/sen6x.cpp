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
static const uint16_t SEN63_CMD_READ_MEASUREMENT = 0x0471;
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

static const int8_t SEN6X_INDEX_SCALE_FACTOR = 10;                            // used for VOC and NOx index values
static const int8_t SEN6X_MIN_INDEX_VALUE = 1 * SEN6X_INDEX_SCALE_FACTOR;     // must be adjusted by the scale factor
static const int16_t SEN6X_MAX_INDEX_VALUE = 500 * SEN6X_INDEX_SCALE_FACTOR;  // must be adjusted by the scale factor

static inline const char *model_to_str(Sen6xType model) {
  switch (model) {
    case SEN62:
      return "SEN62";
    case SEN63C:
      return "SEN63C";
    case SEN65:
      return "SEN65";
    case SEN66:
      return "SEN66";
    case SEN68:
      return "SEN68";
    case SEN69C:
      return "SEN69C";
    default:
      return "UNKNOWN MODEL";
  }
}

static inline std::string convert_to_string(uint16_t array[], uint8_t length) {
  for (int i = 0; i < length; i++) {
    array[i] = convert_big_endian(array[i]);
  }
  std::string new_string = reinterpret_cast<const char *>(array);
  return new_string;
}

void Sen6xComponent::setup() {
  this->set_timeout(100, [this]() { this->internal_setup_(SEN6X_SM_START); });
}

void Sen6xComponent::internal_setup_(SetupStates state) {
  uint16_t string_number[16] = {0};
  switch (state) {
    case SEN6X_SM_START:
      // Check if measurement is ready before reading the value
      if (!this->write_command(CMD_GET_DATA_READY_STATUS)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_START_1); });
      break;
    case SEN6X_SM_START_1:
      uint16_t raw_read_status;
      if (!this->read_data(raw_read_status)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      // In order to query the device periodic measurement must be ceased
      if (raw_read_status) {
        ESP_LOGV(TAG, "Stopping periodic measurement");
        if (!this->stop_measurements_()) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(1400, [this]() { this->internal_setup_(SEN6X_SM_GET_SN); });
      } else {
        this->internal_setup_(SEN6X_SM_GET_SN);
      }
      break;
    case SEN6X_SM_GET_SN:
      if (!this->get_register(CMD_GET_SERIAL_NUMBER, string_number, 16, 20)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      this->serial_number_ = convert_to_string(string_number, 16);
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_GET_PN); });
      break;
    case SEN6X_SM_GET_PN:
      if (!this->get_register(CMD_GET_PRODUCT_NAME, string_number, 16, 20)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      this->product_name_ = convert_to_string(string_number, 16);
      if (this->product_name_ != model_to_str(this->model_.value())) {
        this->mark_failed(LOG_STR("Product Name mismatch"));
        return;
      }
      ESP_LOGV(TAG, "Product Name %s", this->product_name_.c_str());
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_GET_FW); });
      break;
    case SEN6X_SM_GET_FW:
      uint16_t firmware;
      if (!this->get_register(CMD_GET_FIRMWARE_VERSION, &firmware, 1, 20)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      this->firmware_minor_ = firmware & 0xFF;
      this->firmware_major_ = firmware >> 8;
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_VOCB); });
      break;
    case SEN6X_SM_SET_VOCB:
      if (this->voc_sensor_ && this->store_voc_baseline_) {
        // Hash with config hash, version, and serial number, ensures the baseline storage is cleared after OTA
        // Serial numbers are unique to each sensor, so multiple sensors can be used without conflict
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 1, 0)
        uint32_t hash = fnv1a_hash_extend(App.get_config_version_hash(), this->serial_number_);
#else
        uint32_t hash = fnv1_hash(App.get_compilation_time_ref() + this->serial_number_);
#endif
        this->pref_ = global_preferences->make_preference<Sen6xBaselines>(hash, true);
        if (this->pref_.load(&this->voc_baselines_storage_)) {
          ESP_LOGV(TAG, "Loaded VOC baseline state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, this->voc_baselines_storage_.state1);
        }
        // Initialize storage timestamp
        this->seconds_since_last_store_ = 0;
        if (this->voc_baselines_storage_.state0 > 0 && this->voc_baselines_storage_.state1 > 0) {
          ESP_LOGV(TAG, "Restoring VOC baseline from save state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, this->voc_baselines_storage_.state1);
          uint16_t states[4];
          states[0] = voc_baselines_storage_.state0 >> 16;
          states[1] = voc_baselines_storage_.state0 & 0xFFFF;
          states[2] = voc_baselines_storage_.state1 >> 16;
          states[3] = voc_baselines_storage_.state1 & 0xFFFF;
          if (!this->write_command(CMD_VOC_ALGORITHM_STATE, states, 4)) {
            ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
            this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          }
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_VOCT); });
      break;
    case SEN6X_SM_SET_ACCEL:
      if (this->temperature_acceleration_.has_value()) {
        if (!this->write_temperature_acceleration_()) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_VOCT); });
      } else {
        this->internal_setup_(SEN6X_SM_SET_VOCT);
      }
      break;
    case SEN6X_SM_SET_VOCT:
      if (this->voc_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_VOC_ALGORITHM_TUNING, this->voc_tuning_params_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_NOXT); });
      } else {
        this->internal_setup_(SEN6X_SM_SET_NOXT);
      }
      break;
    case SEN6X_SM_SET_NOXT:
      if (this->nox_tuning_params_.has_value()) {
        if (!this->write_tuning_parameters_(CMD_NOX_ALGORITHM_TUNING, this->nox_tuning_params_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_TP); });
      } else {
        this->internal_setup_(SEN6X_SM_SET_TP);
      }
      break;
    case SEN6X_SM_SET_TP:
      if (this->temperature_compensation_.has_value()) {
        if (!this->write_temperature_compensation_(this->temperature_compensation_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_CO2ASC); });
      } else {
        this->internal_setup_(SEN6X_SM_SET_CO2ASC);
      }
      break;
    case SEN6X_SM_SET_CO2ASC:
      if (this->co2_auto_calibrate_.has_value()) {
        if (!this->write_command(CMD_CO2_SENSOR_AUTO_SELF_CAL, this->co2_auto_calibrate_.value() ? 0x01 : 0x00)) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SET_CO2AC); });
      } else {
        this->internal_setup_(SEN6X_SM_SET_CO2AC);
      }
      break;
    case SEN6X_SM_SET_CO2AC:
      if (this->co2_altitude_compensation_.has_value()) {
        if (!this->write_command(CMD_SENSOR_ALTITUDE, this->co2_altitude_compensation_.value())) {
          ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
          this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_SENSOR_CHECK); });
      } else {
        this->internal_setup_(SEN6X_SM_SENSOR_CHECK);
      }
      break;
    case SEN6X_SM_SENSOR_CHECK:
      if (this->co2_ambient_pressure_source_ == nullptr) {
        // if ambient pressure was updated then send it to the sensor
        if (this->co2_ambient_pressure_ != 0) {
          this->write_co2_ambient_pressure_compensation_(this->co2_ambient_pressure_);
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(SEN6X_SM_START_MEAS); });
      break;
    case SEN6X_SM_START_MEAS:
      if (!this->start_measurements_()) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
        return;
      }
      this->set_timeout(50, [this]() { this->internal_setup_(SEN6X_SM_DONE); });
      break;
    case SEN6X_SM_DONE:
      this->initialized_ = true;
      ESP_LOGD(TAG, "Initialized");
      break;
  }
}

void Sen6xComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "SEN6X:\n"
                "  Initialized: %s\n"
                "  Model: %s\n"
                "  Update Interval: %ums\n"
                "  Serial number: %s\n"
                "  Firmware version: %u.%u\n",
                TRUEFALSE(this->initialized_), model_to_str(this->model_.value()), this->update_interval_,
                this->serial_number_.c_str(), this->firmware_major_, this->firmware_minor_);
  LOG_SENSOR("  ", "PM  1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM  2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM  4.0", this->pm_4_0_sensor_);
  LOG_SENSOR("  ", "PM 10.0", this->pm_10_0_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "VOC", this->voc_sensor_);
  LOG_SENSOR("  ", "NOx", this->nox_sensor_);
  LOG_SENSOR("  ", "CO₂", this->co2_sensor_);
  if (this->co2_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Automatic self calibration: %s", ONOFF(this->co2_auto_calibrate_.value()));
    if (this->co2_ambient_pressure_source_ != nullptr) {
      ESP_LOGCONFIG(TAG, "    Dynamic ambient pressure compensation using sensor '%s'",
                    this->co2_ambient_pressure_source_->get_name().c_str());
    } else if (this->this->co2_ambient_pressure_.has_value()) {
      ESP_LOGCONFIG(TAG, "    Pressure compensation: %dm", this->this->co2_ambient_pressure_.value());
    } else if (this->co2_altitude_compensation_.has_value()) {
      ESP_LOGCONFIG(TAG, "    Altitude compensation: %dm", this->co2_altitude_compensation_.value());
    }
    LOG_SENSOR("  ", "HCHO", this->hcho_sensor_);
  }

  void Sen6xComponent::update() {
    if (!this->initialized_) {
      return;
    }
    // Store baselines after defined interval or if the diff between current and stored baseline becomes too much
    if (this->store_voc_baseline_ && this->seconds_since_last_store_ > SHORTEST_BASELINE_STORE_INTERVAL) {
      if (this->write_command(CMD_VOC_ALGORITHM_STATE)) {
        // run it a bit later to avoid adding a delay here
        this->set_timeout(550, [this]() {
          uint16_t states[4];
          if (this->read_data(states, 4)) {
            uint32_t state0 = states[0] << 16 | states[1];
            uint32_t state1 = states[2] << 16 | states[3];
            if ((uint32_t) std::abs(static_cast<int32_t>(this->voc_baselines_storage_.state0 - state0)) >
                    MAXIMUM_STORAGE_DIFF ||
                (uint32_t) std::abs(static_cast<int32_t>(this->voc_baselines_storage_.state1 - state1)) >
                    MAXIMUM_STORAGE_DIFF) {
              this->seconds_since_last_store_ = 0;
              this->voc_baselines_storage_.state0 = state0;
              this->voc_baselines_storage_.state1 = state1;

              if (this->pref_.save(&this->voc_baselines_storage_)) {
                ESP_LOGI(TAG, "Stored VOC baseline state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                         this->voc_baselines_storage_.state0, this->voc_baselines_storage_.state1);
              } else {
                ESP_LOGE(TAG, "Could not store VOC baselines");
              }
            }
          }
        });
      }
    }
    // do not read measured values if we are not running
    if (!this->running_) {
      return;
    }
    uint16_t cmd;
    uint8_t length;
    switch (this->model_.value()) {
      case SEN62:
        cmd = SEN62_CMD_READ_MEASUREMENT;
        length = 6;
        break;
      case SEN63C:
        cmd = SEN63_CMD_READ_MEASUREMENT;
        length = 7;
        break;
      case SEN65:
        cmd = SEN65_CMD_READ_MEASUREMENT;
        length = 8;
        break;
      case SEN66:
        cmd = SEN66_CMD_READ_MEASUREMENT;
        length = 9;
        break;
      case SEN68:
        cmd = SEN68_CMD_READ_MEASUREMENT;
        length = 9;
        break;
      case SEN69C:
        cmd = SEN69C_CMD_READ_MEASUREMENT;
        length = 10;
        break;
      default:
        ESP_LOGE(TAG, "Unsupported model");
        this->status_set_warning();
        return;
    }
    if (!this->write_command(cmd)) {
      this->status_set_warning();
      ESP_LOGD(TAG, ESP_LOG_MSG_COMM_FAIL);
      return;
    }
    this->set_timeout(20, [this, length]() {
      uint16_t measurements[10];
      if (!this->read_data(measurements, length)) {
        ESP_LOGV(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->status_set_warning();
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
      if (this->humidity_sensor_ != nullptr) {
        float humidity = static_cast<int16_t>(measurements[4]) / 100.0f;
        if (measurements[4] == INT16_MAX) {
          humidity = NAN;
        }
        ESP_LOGV(TAG, "humidity = 0x%.4x", measurements[4]);
        this->humidity_sensor_->publish_state(humidity);
      }
      if (this->temperature_sensor_ != nullptr) {
        float temperature = static_cast<int16_t>(measurements[5]) / 200.0f;
        if (measurements[5] == INT16_MAX) {
          temperature = NAN;
        }
        ESP_LOGV(TAG, "temperature = 0x%.4x", measurements[5]);
        this->temperature_sensor_->publish_state(temperature);
      }
      if (this->voc_sensor_ != nullptr) {
        ESP_LOGV(TAG, "voc = 0x%.4x", measurements[6]);
        int16_t voc_idx = static_cast<int16_t>(measurements[6]);
        float voc = (voc_idx < SEN6X_MIN_INDEX_VALUE || voc_idx > SEN6X_MAX_INDEX_VALUE) ? NAN : voc_idx / 10.0f;
        this->voc_sensor_->publish_state(voc);
      }
      if (this->nox_sensor_ != nullptr) {
        ESP_LOGV(TAG, "nox = 0x%.4x", measurements[7]);
        int16_t nox_idx = static_cast<int16_t>(measurements[7]);
        float nox = (nox_idx < SEN6X_MIN_INDEX_VALUE || nox_idx > SEN6X_MAX_INDEX_VALUE) ? NAN : nox_idx / 10.0f;
        this->nox_sensor_->publish_state(nox);
      }
      if (this->co2_sensor_ != nullptr) {
        if (this->model_.value() == SEN63C || this->model_.value() == SEN69C) {
          int16_t value = static_cast<int16_t>(measurements[6]);  // SEN63C reports as signed
          if (this->model_.value() == SEN69C) {
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
      if (this->co2_ambient_pressure_source_ != nullptr) {
        float pressure = this->co2_ambient_pressure_source_->state;
        if (!std::isnan(pressure)) {
          this->action_set_ambient_pressure_compensation(pressure);
        }
      }
      this->status_clear_warning();
    });
  }

  bool Sen6xComponent::start_measurements_() {
    auto result = this->write_command(CMD_START_MEASUREMENTS);
    if (!result) {
      ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    } else {
      this->running_ = true;
      ESP_LOGD(TAG, "Measurements Enabled");
    }
    return result;
  }

  bool Sen6xComponent::stop_measurements_() {
    auto result = this->write_command(CMD_STOP_MEASUREMENTS);
    if (!result) {
      ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    } else {
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
    auto result = this->write_command(i2c_command, params, 6);
    if (!result) {
      ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    }
    return result;
  }

  bool Sen6xComponent::write_temperature_compensation_(const TemperatureCompensation &compensation) {
    uint16_t params[4];
    params[0] = static_cast<uint16_t>(compensation.offset);
    params[1] = static_cast<uint16_t>(compensation.normalized_offset_slope);
    params[2] = compensation.time_constant;
    params[3] = compensation.slot;
    auto result = this->write_command(CMD_TEMPERATURE_COMPENSATION, params, 4);
    if (!result) {
      ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    }
    return result;
  }

  bool SEN5XComponent::write_temperature_acceleration_() {
    uint16_t params[4];
    if (this->temperature_acceleration_.has_value()) {
      auto accel_param = this->temperature_acceleration_.value();
      params[0] = accel_param.k;
      params[1] = accel_param.p;
      params[2] = accel_param.t1;
      params[3] = accel_param.t2;
      auto result = this->write_command(SEN6X_CMD_TEMPERATURE_ACCEL_PARAMETERS, params, 4);
      if (!result) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        return false;
      }
    }
    return true;
  }

  bool Sen6xComponent::write_co2_ambient_pressure_compensation_(uint16_t pressure_in_hpa) {
    auto result = this->write_command(CMD_CO2_SENSOR_AUTO_SELF_CAL, this->co2_auto_calibrate_.value() ? 0x01 : 0x00);
    if (!result) {
      ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    }
    return result;
  }

  bool Sen6xComponent::set_ambient_pressure_compensation(float pressure_in_hpa) {
    if (this->model_.value() == SEN63C || this->model_.value() == SEN66 || this->model_.value() == SEN69C) {
      uint16_t new_ambient_pressure = static_cast<uint16_t>(pressure_in_hpa);
      if (!this->initialized_) {
        this->co2_ambient_pressure_ = new_ambient_pressure;
        return false;
      }
      // Only send pressure value if it has changed since last update
      if (new_ambient_pressure != this->co2_ambient_pressure_) {
        write_co2_ambient_pressure_compensation_(new_ambient_pressure);
        this->co2_ambient_pressure_ = new_ambient_pressure;
        this->set_timeout(20, []() {});
      }
      return true;
    } else {
      ESP_LOGE(TAG, "Set Ambient Pressure Compensation is not supported");
      return false;
    }
  }

  bool Sen6xComponent::start_fan_cleaning() {
    if (this->busy_) {
      ESP_LOGW(TAG, "Sensor is busy");
      return false;
    }
    ESP_LOGD(TAG, "Fan cleaning started (12s)");
    this->busy_ = true;  // prevent actions from stomping on each other
    // measurements must be stopped first
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, "Fan cleaning failed");
      this->busy_ = false;
      return false;
    }
    this->set_timeout(1400, [this]() {
      if (!this->write_command(CMD_START_CLEANING_FAN)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->start_measurements_();
        ESP_LOGE(TAG, "Fan cleaning failed");
        this->set_timeout(50, [this]() { this->busy_ = false; });
      } else {
        this->set_timeout(10000, [this]() {
          if (!this->start_measurements_()) {
            ESP_LOGE(TAG, "Fan cleaning failed");
            this->busy_ = false;
            return;
          }
          ESP_LOGD(TAG, "Fan cleaning finished");
          this->set_timeout(50, [this]() { this->busy_ = false; });
        });
      }
    });
    return true;
  }

  bool Sen6xComponent::action_activate_heater() {
    if (this->busy_) {
      ESP_LOGW(TAG, "Sensor is busy");
      return false;
    }
    ESP_LOGD(TAG, "Heater started (22s)");
    this->busy_ = true;  // prevent actions from stomping on each other
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, "Heater failed");
      this->busy_ = false;
      return false;
    }
    this->set_timeout(1400, [this]() {
      if (!this->write_command(CMD_ACTIVATE_SHT_HEATER)) {
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        this->start_measurements_();
        this->set_timeout(50, [this]() { this->busy_ = false; });
      } else {
        this->set_timeout(20000, [this]() {
          if (!this->start_measurements_()) {
            this->busy_ = false;
            ESP_LOGE(TAG, "Heater failed");
          } else {
            ESP_LOGD(TAG, "Heater finished");  // more than 10s after start
            this->set_timeout(50, [this]() { this->busy_ = false; });
          }
        });
      }
    });
    return true;
  }

  bool Sen6xComponent::action_perform_forced_co2_calibration(uint16_t co2) {
    if (this->model_.value() == SEN63C || this->model_.value() == SEN66 || this->model_.value() == SEN69C) {
      if (this->busy_) {
        ESP_LOGW(TAG, "Sensor is busy");
        return false;
      }
      ESP_LOGD(TAG, "Forced CO₂ recalibration, target co2=%d", co2);
      this->busy_ = true;  // prevent actions from stomping on each other
      if (!this->stop_measurements_()) {
        ESP_LOGE(TAG, "Forced CO₂ recalibration failed");
        this->busy_ = false;
        return false;
      }
      this->set_timeout(1400, [this, co2]() {
        if (!this->write_command(CMD_PERFORM_FORCED_CO2_RECAL, co2)) {
          this->start_measurements_();
          ESP_LOGE(TAG, "Forced CO₂ recalibration failed");
          this->set_timeout(50, [this]() { this->busy_ = false; });
        } else {
          this->set_timeout(500, [this]() {
            uint16_t frc = 0;
            if (!this->read_data(frc)) {
              ESP_LOGE(TAG, "Forced CO₂ recalibration failed");
            } else {
              if (frc == 0xFFFF) {
                ESP_LOGE(TAG, "Forced CO₂ recalibration failed");
              } else {
                ESP_LOGD(TAG, "Forced CO₂ recalibration finished, frc=%+d", static_cast<int32_t>(frc) - 0x8000);
              }
            }
            if (!this->start_measurements_()) {
              ESP_LOGE(TAG, "Forced CO₂ recalibration failed");
            }
            this->set_timeout(50, [this]() { this->busy_ = false; });
          });
        }
      });
      return true;
    } else {
      ESP_LOGE(TAG, "Forced CO₂ recalibration is not supported");
      return false;
    }
  }

  bool SEN5XComponent::set_temperature_compensation(float offset, float normalized_offset_slope, uint16_t time_constant,
                                                    uint8_t slot) {
    TemperatureCompensation compensation(offset, normalized_offset_slope, time_constant, slot);
    if (!this->initialized_) {
      this->temperature_compensation_ = compensation;
      return false;
    }
    if (this->busy_) {
      ESP_LOGW(TAG, "Sensor is busy");
      return false;
    }
    ESP_LOGD(TAG, "Set Temperature Compensation, offset=%f, normalized_offset_slope=%f, time_constant=%d, slot=%d",
             offset, normalized_offset_slope, time_constant, slot);
    this->busy_ = true;  // prevent actions from stomping on each other
    if (!this->write_temperature_compensation_(compensation)) {
      ESP_LOGE(TAG, "Set Temperature Compensation failed");
    }
    this->set_timeout(50, [this]() { this->busy_ = false; });
    return true;
  }

}  // namespace sen6x
}  // namespace esphome
