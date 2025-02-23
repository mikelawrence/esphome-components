#include "stusb4500.h"

#include <iostream>
#include <sstream>
#include <string>

#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace stusb4500
  {
    static const char *const TAG = "stusb4500";

    void STUSB4500Component::setup()
    {
      float voltage, current;

      ESP_LOGCONFIG(TAG, "Setting up STUSB4500...");
      // setup ALERT input pin (when low something has changed)
      if (this->alert_pin_ != nullptr)
      {
        this->alert_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
        this->alert_pin_->setup();
        // this->alert_pin_->attach_interrupt(this->alert_pin_, gpio::INTERRUPT_FALLING_EDGE);
      }
      // load settings from NVM
      ESP_LOGCONFIG(TAG, "Loading NVM...");
      if (!this->nvm_read_())
      {
        ESP_LOGE(TAG, "No response from STUSB4500");
        this->mark_failed();
        return;
      }
      this->rdo_ = read_rdo_status_(); // get current RDO
      if (this->flash_nvm_)
      {
        ESP_LOGCONFIG(TAG, "Flash NVM requested...");
        if (!this->nvm_compare_())
        {
          ESP_LOGCONFIG(TAG, "NVM doesn't match settings");
          this->nvm_load_default_();
          this->nvm_update_();
          ESP_LOGCONFIG(TAG, "Flashing NVM...");
          if (!this->nvm_write_()) {
            ESP_LOGE(TAG, "No response from STUSB4500");
          }
          this->nvm_flashed_ = true;
          ESP_LOGCONFIG(TAG, "NVM has been flashed, power cycle the device to reload NVM");
          this->mark_failed();
        }
        else
        {
          ESP_LOGCONFIG(TAG, "NVM matches current settings, doing nothing");
        }
      }
      else
      {
        if (!this->nvm_compare_()) {
          ESP_LOGE(TAG, "NVM doesn't match settings");
        }
      }
      this->pd_status_ = this->rdo_.b.Object_Pos;
      if (this->pd_state_sensor_ != nullptr)
      {
        this->pd_state_sensor_->publish_state(this->pd_status_);
      }
      current = this->rdo_.b.Operating_Current / 100.0;
      if (this->pd_status_ > 0)
      {
        // Some form of PD was negotiated
        switch (this->pd_status_)
        {
        case 1:
          voltage = 5.0;
          break;
        case 2:
          voltage = this->v_snk_pdo2_;
          break;
        case 3:
          voltage = this->v_snk_pdo3_;
          break;
        default:
          // everything else is no PDO
          voltage = 0.0;
          current = 0.0;
          break;
        }
        if (this->pd_status_sensor_ != nullptr)
        {
          std::ostringstream pd;
          pd << current << "A @ " << voltage << "V";
          this->pd_status_sensor_->publish_state(pd.str());
        }
      }
      else
      {
        // No PD negotiated
        if (this->pd_status_sensor_ != nullptr)
        {
          this->pd_status_sensor_->publish_state("Unknown");
        }
      }
    }

    void STUSB4500Component::dump_config()
    {
      uint8_t match;

      ESP_LOGCONFIG(TAG, "STUSB4500:");
      if (!this->nvm_compare_()) {
        ESP_LOGE(TAG, "  NVM does not match current settings, you should set flash_nvm: true for one boot");
      } else {
        ESP_LOGCONFIG(TAG, "  NVM matches settings");
      }
      if (this->nvm_flashed_) {
        ESP_LOGE(TAG, "  NVM has been flashed, power cycle the device to reload NVM");
      }
      switch (this->rdo_.b.Object_Pos)
      {
      case 1:
        ESP_LOGCONFIG(TAG, "  PDO1 negotiated 5.00V @ %0.2fA, %fW", this->rdo_.b.Operating_Current / 100.0, 5.0 * this->rdo_.b.Operating_Current / 100.0);
        break;
      case 2:
        ESP_LOGCONFIG(TAG, "  PDO2 negotiated %0.2fV @ %0.2fA, %fW", this->v_snk_pdo2_, this->rdo_.b.Operating_Current / 100.0, this->v_snk_pdo2_ * this->rdo_.b.Operating_Current / 100.0);
        break;
      case 3:
        ESP_LOGCONFIG(TAG, "  PDO3 negotiated %0.2fV @ %0.2fA, %fW", this->v_snk_pdo3_, this->rdo_.b.Operating_Current / 100.0, this->v_snk_pdo3_ * this->rdo_.b.Operating_Current / 100.0);
        break;
      default:
        ESP_LOGE(TAG, "  No PD negotiated");
        break;
      }
      LOG_I2C_DEVICE(this);
      LOG_PIN("  ALERT Pin: ", this->alert_pin_);
      LOG_SENSOR("  ", "PD State:", this->pd_state_sensor_);
      LOG_TEXT_SENSOR("  ", "PD Status:", this->pd_status_sensor_);
    }

    void STUSB4500Component::soft_reset_()
    {
      uint8_t buffer[1];

      buffer[0] = SOFT_RESET_MESSAGE;
      this->write_register(REG_TX_HEADER_LOW, buffer, 1); // Soft Reset

      buffer[0] = SEND_MESSAGE;
      this->write_register(REG_PD_COMMAND_CTRL, buffer, 1); // Send Message
    }

    void STUSB4500Component::read_pdos_()
    {
      uint8_t buffer[3 * 4];

      this->read_register(REG_DPM_PDO, buffer, 3 * 4);
      this->pdos_[0].d32 = convert_little_endian(*reinterpret_cast<uint32_t *>(&buffer[0 * 4]) & ~0xC0700000);
      this->pdos_[1].d32 = convert_little_endian(*reinterpret_cast<uint32_t *>(&buffer[1 * 4]) & ~0xFFF00000);
      this->pdos_[2].d32 = convert_little_endian(*reinterpret_cast<uint32_t *>(&buffer[2 * 4]) & ~0xFFF00000);

      this->read_register(REG_DPM_PDO_NUMB, buffer, 1);
      this->pdo_numb_ = buffer[0];
    }

    SNK_PDO_TypeDef STUSB4500Component::read_pdo_(uint8_t pdo_numb)
    {
      uint8_t buffer[4];

      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      this->read_register(REG_DPM_PDO + (pdo_numb * 4), buffer, 4); // read all 4-bytes in 32-bit word

      return convert_little_endian(*reinterpret_cast<SNK_PDO_TypeDef *>(buffer));
    }

    RDO_REG_STATUS_TypeDef STUSB4500Component::read_rdo_status_()
    {
      uint8_t buffer[4];

      this->read_register(REG_RDO_REG_STATUS, buffer, 4); // read all 4-bytes in 32-bit word

      return convert_little_endian(*reinterpret_cast<RDO_REG_STATUS_TypeDef *>(buffer));
    }

    void STUSB4500Component::write_pdos_()
    {
      uint8_t buffer[3 * 4];

      *reinterpret_cast<uint32_t *>(&buffer[0 * 4]) = convert_little_endian(this->pdos_[0].d32);
      *reinterpret_cast<uint32_t *>(&buffer[1 * 4]) = convert_little_endian(this->pdos_[1].d32);
      *reinterpret_cast<uint32_t *>(&buffer[2 * 4]) = convert_little_endian(this->pdos_[2].d32);
      this->write_register(REG_DPM_PDO, buffer, 3 * 4);
    }

    void STUSB4500Component::write_pdo_(uint8_t pdo_numb, SNK_PDO_TypeDef pdo_data)
    {
      uint8_t buffer[4];

      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      *reinterpret_cast<uint32_t *>(&buffer) = convert_little_endian(pdo_data.d32);

      this->write_register(REG_DPM_PDO + (pdo_numb * 4), buffer, 4); // write all 4-bytes in 32-bit word
    }

    void STUSB4500Component::set_pdo_voltage_(uint8_t pdo_numb, float voltage)
    {
      // this->nvm_set_pdo_voltage_(pdo_numb, voltage);

      if (pdo_numb == 1)
        return;

      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      uint16_t voltageValue = (uint16_t)(voltage * 20.0);

      if (voltageValue < 5 * 20)
        voltageValue = 5 * 20;
      else if (voltageValue > 20 * 20)
        voltageValue = 20 * 20;

      // ESP_LOGD(TAG, "PDO%d Voltage = %d", pdo_numb + 1, voltageValue);

      this->pdos_[pdo_numb].fix.Voltage = voltageValue;
    }

    float STUSB4500Component::get_pdo_voltage_(uint8_t pdo_numb)
    {
      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      float voltage = this->pdos_[pdo_numb].fix.Voltage / 20.0;

      if (voltage < 5.0)
        voltage = 5.0;
      else if (voltage > 20.0)
        voltage = 20.0;

      return voltage;
    }

    void STUSB4500Component::set_pdo_current_(uint8_t pdo_numb, float current)
    {
      // this->nvm_set_pdo_current_(pdo_numb, current);

      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      uint16_t currentValue = (uint16_t)(current * 100.0);

      if (currentValue > 5 * 100)
        currentValue = 5 * 100;

      this->pdos_[pdo_numb].fix.Operational_Current = currentValue;
    }

    float STUSB4500Component::get_pdo_current_(uint8_t pdo_numb)
    {
      if (pdo_numb < 1)
        pdo_numb = 0;
      else if (pdo_numb > 3)
        pdo_numb = 2;
      else
        pdo_numb -= 1;

      float current = this->pdos_[pdo_numb].fix.Operational_Current / 100.0;

      if (current > 5.0)
        current = 5.0;

      return current;
    }

    void STUSB4500Component::set_pdo_number_(uint8_t pdo_numb)
    {
      uint8_t buffer[1];

      if (pdo_numb > 3)
        pdo_numb = 3;
      else if (pdo_numb < 1)
        pdo_numb = 1;

      buffer[0] = pdo_numb;
      this->write_register(REG_DPM_PDO_NUMB, buffer, 1);
    }

    uint8_t STUSB4500Component::get_pdo_number_()
    {
      return this->pdo_numb_;
    }

    uint8_t STUSB4500Component::nvm_compare_()
    {
      uint8_t all_match = true;

      // compare NVM with ESPHome settings
      all_match &= (this->snk_pdo_numb_ == this->nvm_get_pdo_number_());
      all_match &= (this->i_snk_pdo1_ == this->nvm_get_pdo_current_(1)) && (this->shift_vbus_hl1_ == this->nvm_get_upper_limit_percentage_(1)) &&
                   (this->shift_vbus_ll1_ == this->nvm_get_lower_limit_percentage_(1));
      all_match &= (this->v_snk_pdo2_ == this->nvm_get_pdo_voltage_(2)) && (this->i_snk_pdo2_ == this->nvm_get_pdo_current_(2)) &&
                   (this->shift_vbus_hl2_ == this->nvm_get_upper_limit_percentage_(2)) && (this->shift_vbus_ll2_ == this->nvm_get_lower_limit_percentage_(2));
      all_match &= (this->v_snk_pdo3_ == this->nvm_get_pdo_voltage_(3)) && (this->i_snk_pdo3_ == this->nvm_get_pdo_current_(3)) &&
                   (this->shift_vbus_hl3_ == this->nvm_get_upper_limit_percentage_(3)) && (this->shift_vbus_ll3_ == this->nvm_get_lower_limit_percentage_(3));
      all_match &= (this->i_snk_pdo_flex_ == this->nvm_get_flex_current_());
      all_match &= (this->vbus_disch_disable_ == this->nvm_get_vbus_disch_disable_());
      all_match &= (this->vbus_disch_time_to_0v_ == this->nvm_get_vbus_disch_time_to_0v_());
      all_match &= (this->vbus_disch_time_to_pdo_ == this->nvm_get_vbus_disch_time_to_pdo_());
      all_match &= (this->power_ok_cfg_ == this->nvm_get_power_ok_cfg_());
      all_match &= (this->usb_comm_capable_ == this->nvm_get_usb_comm_capable_());
      all_match &= (this->snk_uncons_power_ == this->nvm_get_external_power_());
      all_match &= (this->req_src_current_ == this->nvm_get_req_src_current_());
      all_match &= (this->power_only_above_5v_ == this->nvm_get_power_above_5v_only_());
      return all_match;
    }

    void STUSB4500Component::nvm_update_()
    {
      // update NVM with ESPHome settings
      this->nvm_set_pdo_number_(this->snk_pdo_numb_);
      this->nvm_set_pdo_voltage_(2, this->v_snk_pdo2_);
      this->nvm_set_pdo_voltage_(3, this->v_snk_pdo3_);
      this->nvm_set_pdo_current_(1, this->i_snk_pdo1_);
      this->nvm_set_pdo_current_(2, this->i_snk_pdo2_);
      this->nvm_set_pdo_current_(3, this->i_snk_pdo3_);
      this->nvm_set_flex_current_(this->i_snk_pdo_flex_);
      this->nvm_set_upper_limit_percentage_(1, this->shift_vbus_hl1_);
      this->nvm_set_lower_limit_percentage_(1, this->shift_vbus_ll1_);
      this->nvm_set_upper_limit_percentage_(2, this->shift_vbus_hl2_);
      this->nvm_set_lower_limit_percentage_(2, this->shift_vbus_ll2_);
      this->nvm_set_upper_limit_percentage_(3, this->shift_vbus_hl3_);
      this->nvm_set_lower_limit_percentage_(3, this->shift_vbus_ll3_);
      this->nvm_set_vbus_disch_time_to_0v_(this->vbus_disch_time_to_0v_);
      this->nvm_set_vbus_disch_time_to_pdo_(this->vbus_disch_time_to_pdo_);
      this->nvm_set_vbus_disch_disable_(this->vbus_disch_disable_);
      this->nvm_set_power_ok_cfg_(this->power_ok_cfg_);
      this->nvm_set_usb_comm_capable_(this->usb_comm_capable_);
      this->nvm_set_external_power_(this->snk_uncons_power_);
      this->nvm_set_req_src_current_(this->req_src_current_);
      this->nvm_set_power_above_5v_only_(this->power_only_above_5v_);
    }

    // uint8_t STUSB4500Component::nvm_compare_default_()
    // {
    //   uint8_t default_sector[5][8] =
    //       {{0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00},
    //        {0x10, 0x40, 0x9C, 0x1C, 0xFF, 0x01, 0x3C, 0xDF},
    //        {0x02, 0x40, 0x0F, 0x00, 0x32, 0x00, 0xFC, 0xF1},
    //        {0x00, 0x19, 0x56, 0xAF, 0xF5, 0x35, 0x5F, 0x00},
    //        {0x00, 0x4B, 0x90, 0x21, 0x43, 0x00, 0x40, 0xFB}};

    //   for (uint8_t sector = 0; sector < 5; sector++)
    //     if (this->sector[sector].d64 != convert_little_endian(*reinterpret_cast<uint64_t *>(default_sector[sector])))
    //       return false;

    //   return true;
    // }

    void STUSB4500Component::nvm_load_default_()
    {
      uint8_t default_sector[5][8] =
          {{0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00},
           {0x10, 0x40, 0x9C, 0x1C, 0xFF, 0x01, 0x3C, 0xDF},
           {0x02, 0x40, 0x0F, 0x00, 0x32, 0x00, 0xFC, 0xF1},
           {0x00, 0x19, 0x56, 0xAF, 0xF5, 0x35, 0x5F, 0x00},
           {0x00, 0x4B, 0x90, 0x21, 0x43, 0x00, 0x40, 0xFB}};

      for (uint8_t sector = 0; sector < 5; sector++)
        this->sector[sector].d64 = convert_little_endian(*reinterpret_cast<uint64_t *>(default_sector[sector]));
    }

    uint8_t STUSB4500Component::nvm_read_()
    {
      uint8_t buffer[8];

      // Enter Read Mode
      buffer[0] = FTP_CUST_PASSWORD;
      if (this->write_register(REG_FTP_CUST_PASSWORD, buffer, 1))
      { // unlock NVM
        ESP_LOGE(TAG, "  NVM Read Error 1");
        return false;
      }
      buffer[0] = 0x00;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1))
      { // NVM in reset
        return false;
      }
      delayMicroseconds(100); // wait long enough for reset finish

      // Read Sectors
      for (uint8_t sector = 0; sector < 5; sector++)
      {
        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N;
        if (this->write_register(REG_FTP_CTRL_0, buffer, 1))
        { // NVM out of reset
          return false;
        }
        buffer[0] = (READ & FTP_CUST_OPCODE);
        if (this->write_register(REG_FTP_CTRL_1, buffer, 1))
        { // set Read Sectors Opcode
          return false;
        }
        buffer[0] = (sector & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        if (this->write_register(REG_FTP_CTRL_0, buffer, 1))
        { // load Opcode
          return false;
        }
        if (!this->nvm_wait_())
        { // wait until operation is complete
          return false;
        }
        if (this->read_register(REG_RW_BUFFER, buffer, 8))
        { // read all 8 bytes in current sector
          return false;
        }
        this->sector[sector].d64 = convert_little_endian(*reinterpret_cast<uint64_t *>(buffer)); // convert to proper byte order
      }

      // Exit Test Mode
      buffer[0] = FTP_CUST_RST_N;
      buffer[1] = 0x00;
      return !this->write_register(REG_FTP_CTRL_0, buffer, 2);
    }

    uint8_t STUSB4500Component::nvm_write_()
    {
      uint8_t buffer[8];
      // ESP_LOGD(TAG, "Writing NVM");

      // Enter Write Mode
      buffer[0] = FTP_CUST_PASSWORD;
      if (this->write_register(REG_FTP_CUST_PASSWORD, buffer, 1)) // unlock NVM
        return false;
      buffer[0] = 0x00;
      if (this->write_register(REG_RW_BUFFER, buffer, 1)) // must be clear for Partial Erase
        return false;
      buffer[0] = 0x00;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // NVM in reset
        return false;
      delayMicroseconds(10); // wait long enough for reset finish
      buffer[0] = (FTP_CUST_PWR | FTP_CUST_RST_N);
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // NVM out of reset
        return false;

      // Erase all sectors
      buffer[0] = ((0x1F << 3) & FTP_CUST_SER) | (WRITE_SER & FTP_CUST_OPCODE);
      if (this->write_register(REG_FTP_CTRL_1, buffer, 1)) // set all Sectors and Write SER opcode
        return false;
      buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // load Opcode
        return false;
      if (!this->nvm_wait_()) // wait until operation is complete
        return false;
      buffer[0] = (SOFT_PROG_SECTOR & FTP_CUST_OPCODE);
      if (this->write_register(REG_FTP_CTRL_1, buffer, 1)) // set Soft Program Sector opcode
        return false;
      buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // load Opcode
        return false;
      if (!this->nvm_wait_()) // wait until operation is complete
        return false;
      buffer[0] = (ERASE_SECTOR & FTP_CUST_OPCODE);
      if (this->write_register(REG_FTP_CTRL_1, buffer, 1)) // set Erase Sectors opcode
        return false;
      buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // load Opcode
        return false;
      if (!this->nvm_wait_()) // wait until operation is complete
        return false;

      // Write Sectors
      for (uint8_t sector = 0; sector < 5; sector++)
      {
        *reinterpret_cast<uint64_t *>(buffer) = convert_little_endian(this->sector[sector].d64); // convert to proper byte order
        if (this->write_register(REG_RW_BUFFER, buffer, 8))                                      // write all 8 bytes in current sector
          return false;
        // ESP_LOGD(TAG, "Sector[%d] = %02X %02X %02X %02X %02X %02X %02X %02X", sector, buffer[0], buffer[1], buffer[2],
        //          buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

        buffer[0] = (FTP_CUST_PWR | FTP_CUST_RST_N);
        if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // NVM out of reset
          return false;
        buffer[0] = (WRITE_PL & FTP_CUST_OPCODE);
        if (this->write_register(REG_FTP_CTRL_1, buffer, 1)) // set Write PL opcode
          return false;
        buffer[0] = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // load Opcode
          return false;
        if (!this->nvm_wait_()) // wait until operation is complete
          return false;
        buffer[0] = (PROG_SECTOR & FTP_CUST_OPCODE);
        if (this->write_register(REG_FTP_CTRL_1, buffer, 1)) // set Program Sector opcode
          return false;
        buffer[0] = (sector & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        if (this->write_register(REG_FTP_CTRL_0, buffer, 1)) // load Opcode
          return false;
        if (!this->nvm_wait_()) // wait until operation is complete
          return false;
      }

      // Exit Test Mode
      buffer[0] = FTP_CUST_RST_N;
      buffer[1] = 0x00;
      if (this->write_register(REG_FTP_CTRL_0, buffer, 2)) // clear registers
        return false;
      buffer[0] = 0x00;
      return !this->write_register(REG_FTP_CUST_PASSWORD, buffer, 1); // unlock NVM
      return true;
    }

    uint8_t STUSB4500Component::nvm_wait_()
    {
      uint8_t buffer[1];

      // FTP_CUST_REQ bit is cleared by NVM controller when the operation is finished
      do
      {
        if (this->read_register(REG_FTP_CTRL_0, buffer, 1))
          return false;
      } while (buffer[0] & FTP_CUST_REQ);

      return true;
    }

    uint8_t STUSB4500Component::nvm_get_pdo_number_()
    {
      return this->sector[3].bank3.DPM_Snk_PDO_Numb;
    }

    float STUSB4500Component::nvm_get_pdo_voltage_(uint8_t pdo_numb)
    {
      switch (pdo_numb)
      {
      case 1: // PDO 1
        return 5.0;
      case 2: // PDO 2
        return this->sector[4].bank4.Snk_PDO_Flex1_V / 20.0;
      default: // PDO 3
        return this->sector[4].bank4.Snk_PDO_Flex2_V / 20.0;
      }
    }

    float STUSB4500Component::nvm_get_pdo_current_(uint8_t pdo_numb)
    {
      uint8_t currentValue;
      switch (pdo_numb)
      {
      case 1:                                                // PDO 1
        currentValue = this->sector[3].bank3.LUT_Snk_PDO1_I; // this->sector[3][2] >> 4;
        break;
      case 2:                                                // PDO 2
        currentValue = this->sector[3].bank3.LUT_Snk_PDO2_I; // this->sector[3][4] & 0x0F;
        break;
      default:                                               // PDO 3
        currentValue = this->sector[3].bank3.LUT_Snk_PDO3_I; // this->sector[3][5] >> 4;
        break;
      }
      if (currentValue == 0)
        return 0.0;
      else if (currentValue < 11)
        return currentValue * 0.25 + 0.25;
      else
        return currentValue * 0.50 - 2.50;
    }

    float STUSB4500Component::nvm_get_upper_limit_percentage_(uint8_t pdo_numb)
    {
      // returns a fraction from 0.01 to 0.15, or 1 to 15% before conversion
      switch (pdo_numb)
      {
      case 1: // PDO 1
        return (float)this->sector[3].bank3.Snk_HL1 / 100.0;
      case 2: // PDO 2
        return (float)this->sector[3].bank3.Snk_HL2 / 100.0;
      default: // PDO 3
        return (float)this->sector[3].bank3.Snk_HL3 / 100.0;
      }
    }

    float STUSB4500Component::nvm_get_lower_limit_percentage_(uint8_t pdo_numb)
    {
      // returns a fraction from 0.01 to 0.15, or 1 to 15% before conversion
      switch (pdo_numb)
      {
      case 1: // PDO 1
        return (float)this->sector[3].bank3.Snk_LL1 / 100.0;
      case 2: // PDO 2
        return (float)this->sector[3].bank3.Snk_LL2 / 100.0;
      default: // PDO 3
        return (float)this->sector[3].bank3.Snk_LL3 / 100.0;
      }
    }

    float STUSB4500Component::nvm_get_flex_current_()
    {
      return (float)(this->sector[4].bank4.Snk_PDO_Flex_I) / 100.0;
    }

    uint16_t STUSB4500Component::nvm_get_vbus_disch_time_to_0v_()
    {
      // return a value in ms with a range of 84ms to 1260ms
      return (uint16_t)this->sector[1].bank1.Vbus_Disch_Time_To_0V * 84;
    }

    uint16_t STUSB4500Component::nvm_get_vbus_disch_time_to_pdo_()
    {
      // returns a value in ms with a range of 24ms to 360ms,
      return (uint16_t)this->sector[1].bank1.Vbus_Disch_Time_To_PDO * 24;
    }

    uint8_t STUSB4500Component::nvm_get_vbus_disch_disable_()
    {
      return this->sector[1].bank1.Vbus_Dchg_Mask;
    }

    uint8_t STUSB4500Component::nvm_get_external_power_(void)
    {
      return this->sector[3].bank3.Snk_Uncons_Power;
    }

    uint8_t STUSB4500Component::nvm_get_usb_comm_capable_(void)
    {
      return this->sector[3].bank3.USB_Comm_Capable;
    }

    PowerOkConfig STUSB4500Component::nvm_get_power_ok_cfg_(void)
    {
      uint8_t value = this->sector[4].bank4.Power_OK_Cfg;

      if (value == 3)
        return CONFIGURATION_3;
      else if (value == 2)
        return CONFIGURATION_2;
      else
        return CONFIGURATION_1;
    }

    uint8_t STUSB4500Component::nvm_get_gpio_ctrl_(void)
    {
      return this->sector[1].bank1.GPIO_Cfg;
    }

    uint8_t STUSB4500Component::nvm_get_power_above_5v_only_(void)
    {
      return this->sector[4].bank4.Power_Only_Above_5V;
    }

    uint8_t STUSB4500Component::nvm_get_req_src_current_(void)
    {
      return this->sector[4].bank4.Req_Src_Current;
    }

    void STUSB4500Component::nvm_set_pdo_number_(uint8_t pdoNumber)
    {
      uint8_t pdoValue;

      if (pdoNumber < 1)
        pdoValue = 1;
      else if (pdoNumber > 3)
        pdoValue = 3;
      else
        pdoValue = pdoNumber;

      this->sector[3].bank3.DPM_Snk_PDO_Numb = pdoValue;
    }

    void STUSB4500Component::nvm_set_pdo_voltage_(uint8_t pdo_numb, float voltage)
    {
      uint16_t voltageValue = uint16_t(voltage * 20.0);
      if (voltageValue < 5 * 20)
        voltageValue = 5 * 20;
      else if (voltageValue > 20 * 20)
        voltageValue = 20 * 20;

      switch (pdo_numb)
      {
      case 1: // PDO 1 voltage is fixed
        break;
      case 2: // PDO 2
        this->sector[4].bank4.Snk_PDO_Flex1_V = voltageValue;
        break;
      default: // PDO 3
        this->sector[4].bank4.Snk_PDO_Flex2_V = voltageValue;
        break;
      }
    }

    void STUSB4500Component::nvm_set_pdo_current_(uint8_t pdo_numb, float current)
    {
      // valid currents are as follows:
      // 0.00A, 0.50A, 0.75A, 1.00A, 1.25A, 1.50A, 1.75A,
      // 2.00A, 2.25A, 2.50A, 2.75A, 3.00A, 3.50A, and 4.00A
      uint8_t currentValue;

      if (current < 0.5)
        currentValue = 0;
      else if (current <= 3.0)
        currentValue = ((uint8_t)(current / 0.25)) - 1;
      else
        currentValue = ((uint8_t)(current / 0.5)) + 5;

      switch (pdo_numb)
      {
      case 1: // PDO 1
        this->sector[3].bank3.LUT_Snk_PDO1_I = currentValue;
        break;
      case 2: // PDO 2
        this->sector[3].bank3.LUT_Snk_PDO2_I = currentValue;
        break;
      default: // PDO 3
        this->sector[3].bank3.LUT_Snk_PDO3_I = currentValue;
        break;
      }
    }

    void STUSB4500Component::nvm_set_flex_current_(float current)
    {
      if (current < 0.0)
        current = 0.0;
      else if (current > 5.0)
        current = 5.0;
      // uint16_t currentValue = current * 100.0;

      this->sector[4].bank4.Snk_PDO_Flex_I = current * 100;
    }

    void STUSB4500Component::nvm_set_upper_limit_percentage_(uint8_t pdo_numb, float percent)
    {
      // percent is a fraction from 0.01 to 0.15, or 1 to 15% after conversion
      // percent will be forced to a whole percent number in the set
      // {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
      uint8_t percentValue = (uint8_t)(percent * 100.0);

      if (percentValue > 15)
        percentValue = 15;
      else if (percentValue < 1)
        percentValue = 1;

      switch (pdo_numb)
      {
      case 1: // PDO 1
        this->sector[3].bank3.Snk_HL1 = percentValue;
        break;
      case 2: // PDO 2
        this->sector[3].bank3.Snk_HL2 = percentValue;
        break;
      default: // PDO 3
        this->sector[3].bank3.Snk_HL3 = percentValue;
        break;
      }
    }

    void STUSB4500Component::nvm_set_lower_limit_percentage_(uint8_t pdo_numb, float percent)
    {
      // percent is a fraction from 0.01 to 0.15, or 1 to 15% after conversion
      // percent will be forced to a whole percent number in the set
      // {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
      uint8_t percentValue = (uint8_t)(percent * 100.0);

      if (percentValue > 15)
        percentValue = 15;
      else if (percentValue < 1)
        percentValue = 1;

      switch (pdo_numb)
      {
      case 1: // PDO 1
        this->sector[3].bank3.Snk_LL1 = percentValue;
        break;
      case 2: // PDO 2
        this->sector[3].bank3.Snk_LL2 = percentValue;
        break;
      default: // PDO 3
        this->sector[3].bank3.Snk_LL3 = percentValue;
        break;
      }
    }

    void STUSB4500Component::nvm_set_vbus_disch_time_to_pdo_(uint16_t time)
    {
      // time is a value in ms with a range of 24ms to 360ms,
      // time will be divided by 24ms ending with a range of 1 - 15
      time /= 24;
      if (time > 15)
        time = 15;
      else if (time < 1)
        time = 1;

      this->sector[1].bank1.Vbus_Disch_Time_To_PDO = time;
    }

    void STUSB4500Component::nvm_set_vbus_disch_time_to_0v_(uint16_t time)
    {
      // time is a value in ms with a range of 84ms to 1260ms,
      // time will be divided by 84ms ending with a range of 1 - 15
      time /= 84;
      if (time > 15)
        time = 15;
      else if (time < 1)
        time = 1;

      this->sector[1].bank1.Vbus_Disch_Time_To_0V = time;
    }

    void STUSB4500Component::nvm_set_vbus_disch_disable_(uint8_t value)
    {
      // value is true/false
      if (value != 0)
        value = 1;

      this->sector[1].bank1.Vbus_Dchg_Mask = value;
    }

    void STUSB4500Component::nvm_set_external_power_(uint8_t value)
    {
      if (value != 0)
        value = 1;

      this->sector[3].bank3.Snk_Uncons_Power = value;
    }

    void STUSB4500Component::nvm_set_usb_comm_capable_(uint8_t value)
    {
      if (value != 0)
        value = 1;

      this->sector[3].bank3.USB_Comm_Capable = value;
    }

    void STUSB4500Component::nvm_set_power_ok_cfg_(PowerOkConfig value)
    {
      uint8_t new_value;
      if (value == CONFIGURATION_1)
        new_value = 0;
      else if (value == CONFIGURATION_2)
        new_value = 2;
      else
        new_value = 3;

      this->sector[4].bank4.Power_OK_Cfg = new_value;
    }

    void STUSB4500Component::nvm_set_gpio_ctrl_(uint8_t value)
    {
      if (value > 3)
        value = 3;

      this->sector[1].bank1.GPIO_Cfg = value;
    }

    void STUSB4500Component::nvm_set_power_above_5v_only_(uint8_t value)
    {
      if (value != 0)
        value = 1;

      this->sector[4].bank4.Power_Only_Above_5V = value;
    }

    void STUSB4500Component::nvm_set_req_src_current_(uint8_t value)
    {
      if (value != 0)
        value = 1;

      this->sector[4].bank4.Req_Src_Current = value;
    }

  } // namespace stusb4500
} // namespace esphome
