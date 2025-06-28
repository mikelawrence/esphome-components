#include "sen5x.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace sen5x {

static const char *const TAG = "sen5x";
static const char *const ESP_LOG_MSG_CO2_CAL_FAIL = "Perform Forced CO₂ Calibration failed";
static const char *const ESP_LOG_MSG_ACT_SHT_HEATER_FAIL = "Activate SHT Heater failed";
static const char *const ESP_LOG_MSG_FAN_CLEAN_FAIL = "Fan Cleaning failed";

static const uint16_t SEN5X_CMD_READ_MEASUREMENT = 0x03C4;
static const uint16_t SEN60_CMD_READ_MEASUREMENT = 0xEC05;
static const uint16_t SEN63_CMD_READ_MEASUREMENT = 0x0471;
static const uint16_t SEN65_CMD_READ_MEASUREMENT = 0x0446;
static const uint16_t SEN66_CMD_READ_MEASUREMENT = 0x0300;
static const uint16_t SEN68_CMD_READ_MEASUREMENT = 0x0467;
static const uint16_t SEN60_CMD_RESET = 0x3F8D;
static const uint16_t SEN60_CMD_START_MEASUREMENTS = 0x2152;
static const uint16_t SEN60_CMD_STOP_MEASUREMENTS = 0x3F86;
static const uint16_t SEN60_CMD_GET_DATA_READY_STATUS = 0xE4B8;
static const uint16_t SEN60_CMD_START_CLEANING_FAN = 0x3730;
static const uint16_t SEN60_CMD_GET_SERIAL_NUMBER = 0x3682;
static const uint16_t SEN60_CMD_READ_DEVICE_STATUS = 0xD206;

static const uint16_t SEN6X_CMD_TEMPERATURE_ACCEL_PARAMETERS = 0x6100;
static const uint16_t SEN6X_CMD_PERFORM_FORCED_CO2_RECAL = 0x6707;
static const uint16_t SEN6X_CMD_CO2_SENSOR_AUTO_SELF_CAL = 0x6711;
static const uint16_t SEN6X_CMD_AMBIENT_PRESSURE = 0x6720;
static const uint16_t SEN6X_CMD_SENSOR_ALTITUDE = 0x6736;
static const uint16_t SEN6X_CMD_ACTIVATE_SHT_HEATER = 0x6765;
static const uint16_t SEN6X_CMD_READ_DEVICE_STATUS = 0xD206;
static const uint16_t SEN6X_CMD_READ_AND_CLEAR_DEVICE_STATUS = 0xD210;

static const uint16_t SEN5X_CMD_START_MEASUREMENTS_RHT_ONLY = 0x0037;
static const uint16_t SEN5X_CMD_RHT_ACCELERATION_MODE = 0x60F7;
static const uint16_t SEN5X_CMD_AUTO_CLEANING_INTERVAL = 0x8004;

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

static const int8_t SEN5X_INDEX_SCALE_FACTOR = 10;                            // used for VOC and NOx index values
static const int8_t SEN5X_MIN_INDEX_VALUE = 1 * SEN5X_INDEX_SCALE_FACTOR;     // must be adjusted by the scale factor
static const int16_t SEN5X_MAX_INDEX_VALUE = 500 * SEN5X_INDEX_SCALE_FACTOR;  // must be adjusted by the scale factor

static const char *model_to_str(Sen5xType model) {
  switch (model) {
    case SEN50:
      return "SEN50";
    case SEN54:
      return "SEN54";
    case SEN55:
      return "SEN55";
    case SEN60:
      return "SEN60";
    case SEN63C:
      return "SEN63C";
    case SEN65:
      return "SEN65";
    case SEN66:
      return "SEN66";
    case SEN68:
      return "SEN68";
    default:
      return "UNKNOWN MODEL";
  }
}
static const Sen5xType str_to_model(const char *product_name) {
  if (product_name == "SEN50") {
    return SEN50;
  } else if (product_name == "SEN54") {
    return SEN54;
  } else if (product_name == "SEN55") {
    return SEN55;
  } else if (product_name == "SEN60") {
    return SEN60;
  } else if (product_name == "SEN63C") {
    return SEN63C;
  } else if (product_name == "SEN65") {
    return SEN65;
  } else if (product_name == "SEN66") {
    return SEN66;
  } else if (product_name == "SEN68") {
    return SEN68;
  } else {
    return UNKNOWN_MODEL;
  }
}

void SEN5XComponent::setup() { this->internal_setup_(0); }

void SEN5XComponent::internal_setup_(uint8_t state) {
  uint16_t string_number[16] = {0};
  switch (state) {
    case 0:
      ESP_LOGCONFIG(TAG, "Setting up sen5x...");
      // the sensor needs 1000 ms to enter the idle state
      this->set_timeout(1000, [this]() { this->internal_setup_(1); });
      break;
    case 1:
      // Check if measurement is ready before reading the value
      if (!this->write_command(CMD_GET_DATA_READY_STATUS)) {
        ESP_LOGE(TAG, "Failed to write data ready status command");
        this->error_code_ = COMMUNICATION_FAILED;
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(2); });
      break;
    case 2:
      uint16_t raw_read_status;
      if (!this->read_data(raw_read_status)) {
        ESP_LOGE(TAG, "Failed to read data ready status");
        this->error_code_ = COMMUNICATION_FAILED;
        this->mark_failed();
        return;
      }
      uint32_t stop_measurement_delay;
      // In order to query the device periodic measurement must be ceased
      if (raw_read_status) {
        ESP_LOGV(TAG, "Sensor has data available, stopping periodic measurement");
        if (!this->stop_measurements_()) {
          ESP_LOGE(TAG, "Failed to stop measurements");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        if (this->is_sen6x()) {
          stop_measurement_delay = 1000;
        } else {
          stop_measurement_delay = 200;
        }
      } else {
        stop_measurement_delay = 0;
      }
      this->set_timeout(stop_measurement_delay, [this]() { this->internal_setup_(3); });
      break;
    case 3:
      if (!this->get_register(CMD_GET_SERIAL_NUMBER, string_number, 16, 20)) {
        ESP_LOGE(TAG, "Failed to read serial number");
        this->error_code_ = SERIAL_NUMBER_IDENTIFICATION_FAILED;
        this->mark_failed();
        return;
      }
      this->serial_number_ = convert_to_string_(string_number, 16);
      ESP_LOGV(TAG, "Serial number %s", this->serial_number_.c_str());
      this->set_timeout(20, [this]() { this->internal_setup_(4); });
      break;
    case 4:
      if (!this->get_register(CMD_GET_PRODUCT_NAME, string_number, 16, 20)) {
        ESP_LOGE(TAG, "Failed to read product name");
        this->error_code_ = PRODUCT_NAME_FAILED;
        this->mark_failed();
        return;
      }
      this->product_name_ = convert_to_string_(string_number, 16);
      if (this->product_name_ == "") {
        // If blank the user must set the model parameter in the configuration
        // original PR indicated the SEN66 did not report Product Name, mine do
        // just in case I added the model configuration parameter so the user can specify the model
        if (!this->model_.has_value()) {
          ESP_LOGE(TAG, "Sensor Product Name is blank, add 'model' to configuration");
          this->error_code_ = PRODUCT_NAME_FAILED;
          this->mark_failed();
          return;
        } else {
          // update blank product name from model parameter
          this->product_name_ = model_to_str(this->model_.value());
        }
      } else if (!this->model_.has_value()) {
        // model is not defined, get it from product name
        this->model_.value() = str_to_model(this->product_name_.c_str());
        if (this->model_.value() == UNKNOWN_MODEL) {
          ESP_LOGE(TAG, "Product Name from sensor is not a known sensor");
          this->error_code_ = PRODUCT_NAME_FAILED;
          this->mark_failed();
          return;
        }
      } else {
        // product name and model specified in config, they must match
        if (this->product_name_ != model_to_str(this->model_.value())) {
          ESP_LOGE(TAG, "Sensor Product Name does not match 'model' in configuration");
          this->error_code_ = PRODUCT_NAME_FAILED;
          this->mark_failed();
          return;
        }
      }
      ESP_LOGV(TAG, "Product Name %s", this->product_name_.c_str());
      this->set_timeout(20, [this]() { this->internal_setup_(5); });
      break;
    case 5:
      uint16_t firmware;
      if (!this->get_register(CMD_GET_FIRMWARE_VERSION, &firmware, 1, 20)) {
        ESP_LOGE(TAG, "Failed to read firmware version");
        this->error_code_ = FIRMWARE_FAILED;
        this->mark_failed();
        return;
      }
      if (this->is_sen6x()) {
        this->firmware_minor_ = firmware && 0xFF;
        this->firmware_major_ = firmware >> 8;
        ESP_LOGD(TAG, "Firmware version %u.%u", this->firmware_major_, this->firmware_minor_);
      } else {
        this->firmware_major_ = firmware >> 8;
        this->firmware_minor_ = 0xFF;  // not defined
        ESP_LOGD(TAG, "Firmware version %u", this->firmware_major_);
      }
      this->set_timeout(20, [this]() { this->internal_setup_(6); });
      break;
    case 6:
      if (this->voc_sensor_ && this->store_baseline_) {
        uint32_t combined_serial =
            encode_uint24(this->serial_number_[0], this->serial_number_[1], this->serial_number_[2]);
        // Hash with compilation time and serial number, this ensures the baseline storage is cleared after OTA
        // Serial numbers are unique to each sensor, so multiple sensors can be used without conflict
        uint32_t hash = fnv1_hash(App.get_compilation_time() + this->serial_number_);
        this->pref_ = global_preferences->make_preference<Sen5xBaselines>(hash, true);

        if (this->pref_.load(&this->voc_baselines_storage_)) {
          ESP_LOGI(TAG, "Loaded VOC baseline state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
        }

        // Initialize storage timestamp
        this->seconds_since_last_store_ = 0;

        if (this->voc_baselines_storage_.state0 > 0 && this->voc_baselines_storage_.state1 > 0) {
          ESP_LOGI(TAG, "Setting VOC baseline from save state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
          uint16_t states[4];

          states[0] = voc_baselines_storage_.state0 >> 16;
          states[1] = voc_baselines_storage_.state0 & 0xFFFF;
          states[2] = voc_baselines_storage_.state1 >> 16;
          states[3] = voc_baselines_storage_.state1 & 0xFFFF;

          if (!this->write_command(CMD_VOC_ALGORITHM_STATE, states, 4)) {
            ESP_LOGE(TAG, "Failed to set VOC baseline from saved state");
          }
        }
      }
      this->set_timeout(20, [this]() { this->internal_setup_(7); });
      break;
    case 7:
      bool result;
      if (this->auto_cleaning_interval_.has_value()) {
        if (this->is_sen6x()) {
          ESP_LOGE(TAG, "Automatic Cleaning Interval is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!write_command(SEN5X_CMD_AUTO_CLEANING_INTERVAL, this->auto_cleaning_interval_.value())) {
          ESP_LOGE(TAG, "Failed to set Automatic Cleaning Interval");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
      } else {
        this->internal_setup_(8);
      }
      break;
    case 8:
      if (this->acceleration_mode_.has_value()) {
        if (this->is_sen6x()) {
          ESP_LOGE(TAG, "RH/T Acceleration Mode is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_command(SEN5X_CMD_RHT_ACCELERATION_MODE, this->acceleration_mode_.value())) {
          ESP_LOGE(TAG, "Failed to set RH/T Acceleration Mode");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(9); });
      } else {
        this->internal_setup_(9);
      }
      break;
    case 9:
      if (this->voc_tuning_params_.has_value()) {
        if (this->model_ == SEN50 || this->model_ == SEN60 || this->model_ == SEN63C) {
          ESP_LOGE(TAG, "VOC Algorithm Tuning Parameters is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_tuning_parameters_(CMD_VOC_ALGORITHM_TUNING, this->voc_tuning_params_.value())) {
          ESP_LOGE(TAG, "Failed to set VOC Algorithm Tuning Parameters");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(10); });
      } else {
        this->internal_setup_(10);
      }
      break;
    case 10:
      if (this->nox_tuning_params_.has_value()) {
        if (this->model_ == SEN50 || this->model_ == SEN60 || this->model_ == SEN63C) {
          ESP_LOGE(TAG, "NOx Algorithm Tuning Parameters is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_tuning_parameters_(CMD_NOX_ALGORITHM_TUNING, this->nox_tuning_params_.value())) {
          ESP_LOGE(TAG, "Failed to set VOC Algorithm Tuning Parameters");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(11); });
      } else {
        this->internal_setup_(11);
      }
      break;
    case 11:
      if (this->temperature_compensation_.has_value()) {
        if (this->model_ == SEN50 || this->is_sen6x()) {
          ESP_LOGE(TAG, "Temperature Compensation Parameters are not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_temperature_compensation_(this->temperature_compensation_.value())) {
          ESP_LOGE(TAG, "Failed to set Temperature Compensation Parameters");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(12); });
      } else {
        this->internal_setup_(12);
      }
      break;
    case 12:
      if (this->co2_auto_calibrate_.has_value()) {
        if ((this->model_.value() == SEN50 || this->model_.value() == SEN54 || this->model_.value() == SEN55 ||
             this->model_.value() == SEN60 || this->model_.value() == SEN65 || this->model_.value() == SEN68)) {
          ESP_LOGE(TAG, "CO₂ Sensor Auto Self Calibrate Parameter is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_command(SEN6X_CMD_CO2_SENSOR_AUTO_SELF_CAL, this->co2_auto_calibrate_.value() ? 0x01 : 0x00)) {
          ESP_LOGE(TAG, "Failed to set CO₂ Sensor Automatic Self Calibration");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(13); });
      } else {
        this->internal_setup_(13);
      }
      break;
    case 13:
      if (this->co2_altitude_compensation_.has_value()) {
        if ((this->model_.value() == SEN50 || this->model_.value() == SEN54 || this->model_.value() == SEN55 ||
             this->model_.value() == SEN60 || this->model_.value() == SEN65 || this->model_.value() == SEN68)) {
          ESP_LOGE(TAG, "CO₂ Altitude Compensation Parameter is not supported");
          this->error_code_ = UNSUPPORTED_CONF;
          this->mark_failed();
          return;
        }
        if (!this->write_command(SEN6X_CMD_CO2_SENSOR_AUTO_SELF_CAL, this->co2_auto_calibrate_.value() ? 0x01 : 0x00)) {
          ESP_LOGE(TAG, "Failed to set CO₂ Sensor Automatic Self Calibration");
          this->error_code_ = COMMUNICATION_FAILED;
          this->mark_failed();
          return;
        }
        this->set_timeout(20, [this]() { this->internal_setup_(14); });
      } else {
        this->internal_setup_(14);
      }
      break;
    case 14:
      if (this->humidity_sensor_ && (this->model_.value() == SEN50 || this->model_.value() == SEN60)) {
        ESP_LOGE(TAG, "Humidity Sensor is not supported");
        this->humidity_sensor_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      } else if (this->temperature_sensor_ && (this->model_.value() == SEN50 || this->model_.value() == SEN60)) {
        ESP_LOGE(TAG, "Temperature Sensor is not supported");
        this->temperature_sensor_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      } else if (this->voc_sensor_ &&
                 (this->model_.value() == SEN50 || this->model_.value() == SEN60 || this->model_.value() == SEN63C)) {
        ESP_LOGE(TAG, "VOC Sensor is not supported");
        this->voc_sensor_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      } else if (this->nox_sensor_ &&
                 (this->model_.value() == SEN50 || this->model_.value() == SEN60 || this->model_.value() == SEN63C)) {
        ESP_LOGE(TAG, "NOx Sensor is not supported");
        this->nox_sensor_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      } else if (this->co2_sensor_ &&
                 (this->model_.value() == SEN50 || this->model_.value() == SEN54 || this->model_.value() == SEN55 ||
                  this->model_.value() == SEN60 || this->model_.value() == SEN65 || this->model_.value() == SEN68)) {
        ESP_LOGE(TAG, "CO₂ Sensor is not supported");
        this->nox_sensor_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      } else if (this->co2_ambient_pressure_source_ &&
                 (this->model_.value() == SEN50 || this->model_.value() == SEN54 || this->model_.value() == SEN55 ||
                  this->model_.value() == SEN60 || this->model_.value() == SEN65 || this->model_.value() == SEN68)) {
        ESP_LOGE(TAG, "Ambient Pressure Compensation Source is not supported");
        this->co2_ambient_pressure_source_ = nullptr;  // mark as not used
        this->error_code_ = UNSUPPORTED_CONF;
        this->mark_failed();
        return;
      }
      // if ambient pressure was already updated then send it to the sensor
      if (this->co2_ambient_pressure_source_ && (this->co2_ambient_pressure_ != 0)) {
        this->update_co2_ambient_pressure_compensation_(this->co2_ambient_pressure_);
      }
      this->set_timeout(2000, [this]() { this->internal_setup_(15); });
      break;
    case 15:
      // Finally start sensor measurements
      if (!this->start_measurements_()) {
        this->error_code_ = MEASUREMENT_INIT_FAILED;
        this->mark_failed();
        return;
      }
      this->set_timeout(20, [this]() { this->internal_setup_(16); });
      break;
    case 16:
      initialized_ = true;
      ESP_LOGD(TAG, "Sensor initialized");
      break;
  }
}

void SEN5XComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "sen5x:");
  if (this->is_failed()) {
    switch (this->error_code_) {
      case COMMUNICATION_FAILED:
        ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
        break;
      case MEASUREMENT_INIT_FAILED:
        ESP_LOGE(TAG, "  Measurement Initialization failed!");
        break;
      case SERIAL_NUMBER_IDENTIFICATION_FAILED:
        ESP_LOGE(TAG, "  Unable to read sensor serial id");
        break;
      case PRODUCT_NAME_FAILED:
        ESP_LOGE(TAG, "  Product name issue");
        break;
      case FIRMWARE_FAILED:
        ESP_LOGE(TAG, "  Unable to read sensor firmware version");
        break;
      case UNSUPPORTED_CONF:
        ESP_LOGE(TAG, "  Unsupported configuration");
        break;
      default:
        ESP_LOGE(TAG, "  Unknown setup error!");
        break;
    }
  }
  LOG_I2C_DEVICE(this);
  if (this->model_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Model: %s", model_to_str(this->model_.value()));
  }
  ESP_LOGCONFIG(TAG, "  Product Name: %s", this->product_name_.c_str());
  ESP_LOGCONFIG(TAG, "  Serial number: %s", this->serial_number_.c_str());
  if (this->is_sen6x()) {
    ESP_LOGCONFIG(TAG, "  Firmware version: %u.%u", this->firmware_major_, this->firmware_minor_);
  } else {
    ESP_LOGCONFIG(TAG, "  Firmware version: %u", this->firmware_major_);
  }
  if (this->auto_cleaning_interval_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Auto cleaning interval %" PRId32 " seconds", auto_cleaning_interval_.value());
  }
  if (this->acceleration_mode_.has_value()) {
    switch (this->acceleration_mode_.value()) {
      case LOW_ACCELERATION:
        ESP_LOGCONFIG(TAG, "  Low RH/T acceleration mode");
        break;
      case MEDIUM_ACCELERATION:
        ESP_LOGCONFIG(TAG, "  Medium RH/T acceleration mode");
        break;
      case HIGH_ACCELERATION:
        ESP_LOGCONFIG(TAG, "  High RH/T acceleration mode");
        break;
    }
  }
  LOG_UPDATE_INTERVAL(this);
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
    ESP_LOGCONFIG(TAG, "    Automatic self calibration: %s", co2_auto_calibrate_ ? "On" : "Off");
    if (this->co2_ambient_pressure_source_ != nullptr) {
      ESP_LOGCONFIG(TAG, "    Dynamic ambient pressure compensation using sensor '%s'",
                    this->co2_ambient_pressure_source_->get_name().c_str());
    } else {
      if (this->co2_altitude_compensation_.has_value()) {
        ESP_LOGCONFIG(TAG, "    Altitude compensation: %dm", this->co2_altitude_compensation_);
      }
    }
  }
  LOG_SENSOR("  ", "HCHO", this->hcho_sensor_);
}

void SEN5XComponent::update() {
  if (!initialized_) {
    return;
  }

  // Store baselines after defined interval or if the difference between current and stored baseline becomes too
  // much
  if (this->store_baseline_ && this->seconds_since_last_store_ > SHORTEST_BASELINE_STORE_INTERVAL) {
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
              ESP_LOGI(TAG, "Stored VOC baseline state0: 0x%04" PRIX32 " ,state1: 0x%04" PRIX32,
                       this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
            } else {
              ESP_LOGW(TAG, "Could not store VOC baselines");
            }
          }
        }
      });
    }
  }
  uint16_t cmd;
  uint8_t length;
  switch (this->model_.value()) {
    case SEN50:
    case SEN54:
    case SEN55:
      cmd = SEN5X_CMD_READ_MEASUREMENT;
      length = 9;
      break;
    case SEN60:
      cmd = SEN60_CMD_READ_MEASUREMENT;
      length = 4;
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
  }

  if (!this->write_command(cmd)) {
    this->status_set_warning();
    ESP_LOGD(TAG, "Failed to write Measurement Command (last_error=%d)", this->last_error_);
    return;
  }

  this->set_timeout(20, [this, length]() {
    uint16_t measurements[9];

    if (!this->read_data(measurements, length)) {
      this->status_set_warning();
      ESP_LOGD(TAG, "read data error (%d)", this->last_error_);
      return;
    }

    // supported by all
    ESP_LOGVV(TAG, "pm_1_0 = 0x%.4x", measurements[0]);
    float pm_1_0 = measurements[0] == UINT16_MAX ? NAN : measurements[0] / 10.0f;
    if (this->pm_1_0_sensor_ != nullptr) {
      this->pm_1_0_sensor_->publish_state(pm_1_0);
    }

    // supported by all
    ESP_LOGVV(TAG, "pm_2_5 = 0x%.4x", measurements[1]);
    float pm_2_5 = measurements[1] == UINT16_MAX ? NAN : measurements[1] / 10.0f;
    if (this->pm_2_5_sensor_ != nullptr) {
      this->pm_2_5_sensor_->publish_state(pm_2_5);
    }

    // supported by all
    ESP_LOGVV(TAG, "pm_4_0 = 0x%.4x", measurements[2]);
    float pm_4_0 = measurements[2] == UINT16_MAX ? NAN : measurements[2] / 10.0f;
    if (this->pm_4_0_sensor_ != nullptr) {
      this->pm_4_0_sensor_->publish_state(pm_4_0);
    }

    // supported by all
    ESP_LOGVV(TAG, "pm_10_0 = 0x%.4x", measurements[3]);
    float pm_10_0 = measurements[3] == UINT16_MAX ? NAN : measurements[3] / 10.0f;
    if (this->pm_10_0_sensor_ != nullptr) {
      this->pm_10_0_sensor_->publish_state(pm_10_0);
    }

    if (this->model_.value() == SEN54 || this->model_.value() == SEN55 || this->model_.value() == SEN63C ||
        this->model_.value() == SEN65 || this->model_.value() == SEN66 || this->model_.value() == SEN68) {
      ESP_LOGVV(TAG, "humidity = 0x%.4x", measurements[4]);
      float humidity = measurements[4] == INT16_MAX ? NAN : static_cast<int16_t>(measurements[4]) / 100.0f;
      if (this->humidity_sensor_ != nullptr) {
        this->humidity_sensor_->publish_state(humidity);
      }

      ESP_LOGVV(TAG, "temperature = 0x%.4x", measurements[5]);
      float temperature = measurements[5] == INT16_MAX ? NAN : static_cast<int16_t>(measurements[5]) / 200.0f;
      if (this->temperature_sensor_ != nullptr) {
        this->temperature_sensor_->publish_state(temperature);
      }
    }

    if (this->model_.value() == SEN54 || this->model_.value() == SEN55 || this->model_.value() == SEN65 ||
        this->model_.value() == SEN66 || this->model_.value() == SEN68) {
      ESP_LOGVV(TAG, "voc = 0x%.4x", measurements[6]);
      int16_t voc_idx = static_cast<int16_t>(measurements[6]);
      float voc = (voc_idx < SEN5X_MIN_INDEX_VALUE || voc_idx > SEN5X_MAX_INDEX_VALUE)
                      ? NAN
                      : static_cast<float>(voc_idx) / 10.0f;
      if (this->voc_sensor_ != nullptr) {
        this->voc_sensor_->publish_state(voc);
      }
    }

    if (this->model_.value() == SEN55 || this->model_.value() == SEN65 || this->model_.value() == SEN66 ||
        this->model_.value() == SEN68) {
      ESP_LOGVV(TAG, "nox = 0x%.4x", measurements[7]);
      int16_t nox_idx = static_cast<int16_t>(measurements[7]);
      float nox = (nox_idx < SEN5X_MIN_INDEX_VALUE || nox_idx > SEN5X_MAX_INDEX_VALUE)
                      ? NAN
                      : static_cast<float>(nox_idx) / 10.0f;
      if (this->nox_sensor_ != nullptr) {
        this->nox_sensor_->publish_state(nox);
      }
    }

    if (this->model_.value() == SEN63C || this->model_.value() == SEN66) {
      uint16_t measurement = measurements[6];
      if (this->model_.value() == SEN66) {
        measurement = measurements[8];
      }
      ESP_LOGVV(TAG, "co2 = 0x%.4x", co2);
      float co2 = measurement == UINT16_MAX ? NAN : measurement / 1.0f;
      if (this->co2_sensor_ != nullptr) {
        this->co2_sensor_->publish_state(co2);
      }
    }

    if (this->model_.value() == SEN68) {
      ESP_LOGVV(TAG, "HCHO = 0x%.4x", measurements[8]);
      float hcho = measurements[8] == UINT16_MAX ? NAN : measurements[8] / 10.0f;
      if (this->hcho_sensor_ != nullptr) {
        this->hcho_sensor_->publish_state(hcho);
      }
    }

    if (this->co2_ambient_pressure_source_ != nullptr) {
      float pressure = this->co2_ambient_pressure_source_->state;
      if (!std::isnan(pressure)) {
        set_ambient_pressure_compensation(pressure);
      }
    }

    this->status_clear_warning();
  });
}

bool SEN5XComponent::start_measurements_() {
  uint16_t cmd;
  switch (this->model_.value()) {
    case SEN50:
    case SEN54:
    case SEN55:
      cmd = SEN5X_CMD_START_MEASUREMENTS_RHT_ONLY;
      if (this->pm_1_0_sensor_ || this->pm_2_5_sensor_ || this->pm_4_0_sensor_ || this->pm_10_0_sensor_) {
        // if any of the gas sensors are active we need a full measurement
        cmd = CMD_START_MEASUREMENTS;
      }
      break;
    case SEN60:
      cmd = SEN60_CMD_START_MEASUREMENTS;
      break;
    case SEN63C:
    case SEN65:
    case SEN66:
    case SEN68:
      cmd = CMD_START_MEASUREMENTS;
      break;
  }

  auto result = this->write_command(cmd);
  if (!result) {
    ESP_LOGE(TAG, "I2C Write error Start Measurements (error=%d)", this->last_error_);
  } else {
    this->running_ = true;
  }
  return result;
}

bool SEN5XComponent::stop_measurements_() {
  uint16_t cmd;
  if (this->model_.value() == SEN60) {
    cmd = SEN60_CMD_STOP_MEASUREMENTS;
  } else {
    cmd = CMD_STOP_MEASUREMENTS;
  }
  auto result = this->write_command(cmd);
  if (!result) {
    ESP_LOGE(TAG, "I2C Write error Stop Measurements (error=%d)", this->last_error_);
  } else {
    this->running_ = false;
  }
  return result;
}

std::string SEN5XComponent::convert_to_string_(uint16_t array[], uint8_t length) {
  std::string new_string;
  const uint16_t *current_int = array;
  char current_char;
  // 2 ASCII bytes are encoded in an int
  do {
    // first char
    current_char = *current_int >> 8;
    if (current_char) {
      new_string.push_back(current_char);
      // second char
      current_char = *current_int & 0xFF;
      if (current_char) {
        new_string.push_back(current_char);
      }
    }
    current_int++;
  } while (current_char && --length);
  return new_string;
}
bool SEN5XComponent::write_tuning_parameters_(uint16_t i2c_command, const GasTuning &tuning) {
  uint16_t params[6];
  params[0] = tuning.index_offset;
  params[1] = tuning.learning_time_offset_hours;
  params[2] = tuning.learning_time_gain_hours;
  params[3] = tuning.gating_max_duration_minutes;
  params[4] = tuning.std_initial;
  params[5] = tuning.gain_factor;
  auto result = this->write_command(i2c_command, params, 6);
  if (!result) {
    ESP_LOGE(TAG, "I2C Write error Tuning Parameters (error=%d)", this->last_error_);
  }
  return result;
}

bool SEN5XComponent::write_temperature_compensation_(const TemperatureCompensation &compensation) {
  uint16_t params[3];
  params[0] = compensation.offset;
  params[1] = compensation.normalized_offset_slope;
  params[2] = compensation.time_constant;
  auto result = this->write_command(CMD_TEMPERATURE_COMPENSATION, params, 3);
  if (!result) {
    ESP_LOGE(TAG, "Write error Temperature Compensation (error=%d)", this->last_error_);
  }
  return result;
}

bool SEN5XComponent::update_co2_ambient_pressure_compensation_(uint16_t pressure_in_hpa) {
  auto result =
      this->write_command(SEN6X_CMD_CO2_SENSOR_AUTO_SELF_CAL, this->co2_auto_calibrate_.value() ? 0x01 : 0x00);
  if (!result) {
    ESP_LOGE(TAG, "Write error Auto Self Calibration (error=%d)", this->last_error_);
  }
  return result;
}

bool SEN5XComponent::set_ambient_pressure_compensation(float pressure_in_hpa) {
  if (this->model_.value() == SEN63C || this->model_.value() == SEN66) {
    uint16_t new_ambient_pressure = (uint16_t) pressure_in_hpa;
    if (!this->initialized_) {
      this->co2_ambient_pressure_ = new_ambient_pressure;
      return false;
    }
    // Only send pressure value if it has changed since last update
    if (new_ambient_pressure != this->co2_ambient_pressure_) {
      update_co2_ambient_pressure_compensation_(new_ambient_pressure);
      this->co2_ambient_pressure_ = new_ambient_pressure;
      this->set_timeout(20, [this]() {});
    } else {
      ESP_LOGD(TAG, "Ambient Pressure compensation skipped - no change required");
    }
    return true;
  } else {
    ESP_LOGE(TAG, "Set Ambient Pressure Compensation is not supported");
    return false;
  }
}

bool SEN5XComponent::is_sen6x() {
  switch (this->model_.value()) {
    case SEN60:
    case SEN63C:
    case SEN65:
    case SEN66:
    case SEN68:
      return true;
    default:
      return false;
  }
}

bool SEN5XComponent::start_fan_cleaning() {
  ESP_LOGV(TAG, "Fan Cleaning started");
  this->initialized_ = false;  // prevent update from trying to read the sensors
  // measurements must be stopped first for certain devices
  if (this->is_sen6x() && !this->stop_measurements_()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_FAN_CLEAN_FAIL);
    this->initialized_ = true;
    return false;
  }
  this->set_timeout(1000, [this]() {
    uint16_t cmd;
    if (this->model_.value() == SEN60) {
      cmd = SEN60_CMD_START_CLEANING_FAN;
    } else {
      cmd = CMD_START_CLEANING_FAN;
    }
    if (!this->write_command(cmd)) {
      ESP_LOGE(TAG, "I2C Write error Start Cleaning Fan (error=%d)", this->last_error_);
      if (!this->running_) {
        this->start_measurements_();
        this->set_timeout(50, [this]() {
          ESP_LOGE(TAG, ESP_LOG_MSG_FAN_CLEAN_FAIL);
          this->initialized_ = true;
        });
      }
    } else {
      this->set_timeout(10000, [this]() {
        if (!this->running_ && !this->start_measurements_()) {
          ESP_LOGE(TAG, ESP_LOG_MSG_FAN_CLEAN_FAIL);
        } else {
          ESP_LOGD(TAG, "Fan Cleaning complete");
          this->initialized_ = true;
        }
        this->set_timeout(50, [this]() { this->initialized_ = true; });
      });
    }
  });
  return true;
}

bool SEN5XComponent::activate_heater() {
  if (this->model_.value() == SEN63C || this->model_.value() == SEN65 || this->model_.value() == SEN66 ||
      this->model_.value() == SEN68) {
    ESP_LOGV(TAG, "Activate SHT Heater started");
    this->initialized_ = false;  // prevent update from trying to read the sensors
    // measurements must be stopped first for certain devices
    if (this->is_sen6x() && !this->stop_measurements_()) {
      ESP_LOGE(TAG, ESP_LOG_MSG_ACT_SHT_HEATER_FAIL);
      this->initialized_ = true;
      return false;
    }
    this->set_timeout(1000, [this]() {
      if (!this->write_command(SEN6X_CMD_ACTIVATE_SHT_HEATER)) {
        ESP_LOGE(TAG, "I2C Write error Activate SHT Heater (error=%d)", this->last_error_);
        if (!this->running_) {
          this->start_measurements_();
          this->set_timeout(1300, [this]() {
            ESP_LOGE(TAG, ESP_LOG_MSG_ACT_SHT_HEATER_FAIL);
            this->initialized_ = true;
          });
        }
      } else {
        this->set_timeout(20000, [this]() {
          if (!this->running_ && !this->start_measurements_()) {
            ESP_LOGE(TAG, ESP_LOG_MSG_ACT_SHT_HEATER_FAIL);
          } else {
            ESP_LOGD(TAG, "Activate SHT Heater complete");
            this->initialized_ = true;
          }
          this->set_timeout(50, [this]() { this->initialized_ = true; });
        });
      }
    });
    return true;
  } else {
    ESP_LOGE(TAG, "Activate SHT Heater is not supported");
    return false;
  }
}

bool SEN5XComponent::perform_forced_co2_calibration(uint16_t co2) {
  if (this->model_.value() == SEN63C || this->model_.value() == SEN66) {
    ESP_LOGE(TAG, "Perform Forced CO₂ Calibration started, target co2=%d", co2);
    this->initialized_ = false;  // prevent update from trying to read the sensors
    // measurements must be stopped first
    if (!this->stop_measurements_()) {
      ESP_LOGE(TAG, ESP_LOG_MSG_CO2_CAL_FAIL);
      this->initialized_ = true;
      return false;
    }
    this->set_timeout(1000, [this, co2]() {
      if (!this->write_command(SEN6X_CMD_PERFORM_FORCED_CO2_RECAL, co2)) {
        ESP_LOGE(TAG, "I2C Write error Perform Forced CO₂ Recalibration (error=%d)", this->last_error_);
        if (!this->running_) {
          this->start_measurements_();
          ESP_LOGE(TAG, ESP_LOG_MSG_CO2_CAL_FAIL);
        }
      } else {
        this->set_timeout(500, [this]() {
          uint16_t correction = 0;
          if (!this->read_data(correction)) {
            this->start_measurements_();
            ESP_LOGE(TAG, ESP_LOG_MSG_CO2_CAL_FAIL);
          } else {
            if (correction == 0xFFFF) {
              this->start_measurements_();
              ESP_LOGE(TAG, "Perform Forced CO₂ Calibration command reports failure");
            } else {
              if (!this->start_measurements_()) {
                ESP_LOGE(TAG, ESP_LOG_MSG_CO2_CAL_FAIL);
              } else {
                ESP_LOGD(TAG, "Perform Forced CO₂ Calibration complete");
              }
            }
          }
        });
      }
      this->set_timeout(50, [this]() { this->initialized_ = true; });
    });
    return true;
  } else {
    ESP_LOGE(TAG, "Perform Forced CO₂ Calibration is not supported");
    return false;
  }
}
}  // namespace sen5x
}  // namespace esphome