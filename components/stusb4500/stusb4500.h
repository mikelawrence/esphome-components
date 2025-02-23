#pragma once

// #include <vector>
#include "esphome/core/hal.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/i2c/i2c.h"

// SNK_PDO
typedef union
{
  uint32_t d32;
  struct
  {
    uint16_t Operational_Current : 10;       // Bits 09-00
    uint16_t Voltage : 10;                   // Bits 19-10
    uint16_t Reserved_22_20 : 3;             // Bits 22-20
    uint16_t Fast_Role_Req_cur : 2;          // Bits 24-23, must be set to 0 in 2.0
    uint16_t Dual_Role_Data : 1;             // Bit     25
    uint16_t USB_Communications_Capable : 1; // Bit     26
    uint16_t Unconstrained_Power : 1;        // Bit     27
    uint16_t Higher_Capability : 1;          // Bit     28
    uint16_t Dual_Role_Power : 1;            // Bit     29
    uint16_t Fixed : 2;                      // Bits 31-30

  } __attribute__((packed)) fix;
} SNK_PDO_TypeDef;

// SRC PDO
typedef union
{
  uint32_t d32;
  struct
  {
    // Table 6-9 Fixed Supply PDO - Source
    uint16_t Max_Operating_Current : 10; // Bits 09-00
    uint16_t Voltage : 10;               // Bits 19-10
    uint16_t PeakCurrent : 2;            // Bits 21-20
    uint16_t Reserved : 3;               // Bits 24-22
    uint16_t DataRoleSwap : 1;           // Bit  25
    uint16_t Communication : 1;          // Bit  26
    uint16_t ExternalyPowered : 1;       // Bit  27
    uint16_t SuspendSuported : 1;        // Bit  28
    uint16_t DualRolePower : 1;          // Bit  29
    uint16_t FixedSupply : 2;            // Bits 31-30
  } __attribute__((packed)) fix;
  struct
  {
    // Table 6-11 Variable Supply (non-Battery) PDO - Source
    uint16_t Operating_Current : 10; // Bits 09-00
    uint16_t Min_Voltage : 10;       // Bits 19-10
    uint16_t Max_Voltage : 10;       // Bits 29-20
    uint16_t VariableSupply : 2;     // Bits 31-30
  } __attribute__((packed)) var;
  struct
  {
    // Table 6-12 Battery Supply PDO - Source
    uint16_t Operating_Power : 10; // Bits 09-00
    uint16_t Min_Voltage : 10;     // Bits 19-10
    uint16_t Max_Voltage : 10;     // Bits 29-20
    uint16_t Battery : 2;          // Bits 31-30
  } __attribute__((packed)) bat;

} SRC_PDO_TypeDef;

// SRC RDO 
typedef union
{
  uint32_t d32;
  struct
  {
    uint16_t Max_Current : 10;       // Bits 09-00
    uint16_t Operating_Current : 10; // Bits 19-10
    uint16_t reserved_22_20 : 3;     // Bits 22-20
    uint16_t UnchunkedMess_sup : 1;  // Bit     23
    uint16_t UsbSuspend : 1;         // Bit     24
    uint16_t UsbComCap : 1;          // Bit     25
    uint16_t CapaMismatch : 1;       // Bit     26
    uint16_t GiveBack : 1;           // Bit     27
    uint16_t Object_Pos : 3;         // Bits 30-28
    uint16_t reserved_31 : 1;        // Bits    31

  } __attribute__((packed)) b;
} RDO_REG_STATUS_TypeDef;

typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t Phy_Status_AL : 1;          // Bit 0
    uint8_t Prt_Status_AL : 1;          // Bit 1
    uint8_t Reserved_2 : 1;             // Bit 2
    uint8_t PD_TypeC_Status_AL : 1;     // Bit 3
    uint8_t HW_Fault_Status_AL : 1;     // Bit 4
    uint8_t Monitoring_Status_AL : 1;   // Bit 5
    uint8_t CC_Detection_Status_AL : 1; // Bit 6
    uint8_t Hard_Reset_AL : 1;          // Bit 7
  } __attribute__((packed)) b;
} ALERT_STATUS_TypeDef;

typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t Phy_Status_AL_Mask : 1;          // Bit 0
    uint8_t Prt_Status_AL_Mask : 1;          // Bit 1
    uint8_t Reserved_2 : 1;                  // Bit 2
    uint8_t PD_TypeC_Status_AL_Mask : 1;     // Bit 3
    uint8_t HW_Fault_Status_AL_Mask : 1;     // Bit 4
    uint8_t Monitoring_Status_AL_Mask : 1;   // Bit 5
    uint8_t CC_Detection_Status_AL_Mask : 1; // Bit 6
    uint8_t Hard_Reset_AL_Mask : 1;          // Bit 7
  } __attribute__((packed)) b;
} ALERT_STATUS_MASK_TypeDef;

// NVM Union
typedef union
{
  uint64_t d64;
  uint8_t d8[8];
  struct
  {
    uint16_t Vendor_ID : 16;             // Bits 15-00
    uint16_t Product_ID : 16;            // Bits 31-16
    uint16_t BDC_Device_ID_ID : 16;      // Bits 47-32
    uint16_t Port_Role : 8;              // Bits 55-48
    uint16_t Device_Power_Role_Ctrl : 8; // Bits 63-56
  } __attribute__((packed)) bank0;
  struct
  {
    uint16_t Reserved_03_00 : 4;         // Bits 03-00
    uint16_t GPIO_Cfg : 2;               // Bits 05-04, GPIO_CFG
    uint16_t Reserved_12_06 : 7;         // Bits 12-06
    uint16_t Vbus_Dchg_Mask : 1;         // Bit     13, VBUS_DISCH_DISABLE
    uint16_t Reserved_15_14 : 2;         // Bits 15-14
    uint16_t Vbus_Disch_Time_To_PDO : 4; // Bits 19-16, VBUS_DISCH_TIME_TO_PDO
    uint16_t Vbus_Disch_Time_To_0V : 4;  // Bits 23-20, VBUS_DISCH_TIME_TO_0V
    uint16_t Reserved_63_25 : 7;         // Bits 63-24
  } __attribute__((packed)) bank1;
  struct
  {
    uint64_t Reserved_63_00 : 8; // Bits 63-00
  } __attribute__((packed)) bank2;
  struct
  {
    uint16_t Reserved_15_00 : 16;  // Bits 15-00
    uint16_t USB_Comm_Capable : 1; // Bit     16, USB_COMM_CAPABLE
    uint16_t DPM_Snk_PDO_Numb : 2; // Bits 18-17, SNK_PDO_NUMB
    uint16_t Snk_Uncons_Power : 1; // Bit     19, SNK_UNCONS_POWER
    uint16_t LUT_Snk_PDO1_I : 4;   // Bits 23-20, I_SNK_PDO1
    uint16_t Snk_LL1 : 4;          // Bits 27-24, SHIFT_VBUS_LL1
    uint16_t Snk_HL1 : 4;          // Bits 31-28, SHIFT_VBUS_HL1
    uint16_t LUT_Snk_PDO2_I : 4;   // Bits 35-32, I_SNK_PDO2
    uint16_t Snk_LL2 : 4;          // Bits 39-36, SHIFT_VBUS_LL2
    uint16_t Snk_HL2 : 4;          // Bits 43-40, SHIFT_VBUS_HL2
    uint16_t LUT_Snk_PDO3_I : 4;   // Bits 47-44, I_SNK_PDO3
    uint16_t Snk_LL3 : 4;          // Bits 51-48, SHIFT_VBUS_LL3
    uint16_t Snk_HL3 : 4;          // Bits 55-52, SHIFT_VBUS_HL3
    uint16_t Reserved_63_56 : 8;   // Bits 63-56
  } __attribute__((packed)) bank3;
  struct
  {
    uint8_t Reserved_05_00 : 6;      // Bits 05-00
    uint16_t Snk_PDO_Flex1_V : 10;   // Bits 15-06, V_SNK_PDO2
    uint16_t Snk_PDO_Flex2_V : 10;   // Bits 25-16, V_SNK_PDO3
    uint16_t Snk_PDO_Flex_I : 10;    // Bits 35-26, I_SNK_PDO_FLEX
    uint8_t Reserved_36 : 1;         // Bit     36
    uint8_t Power_OK_Cfg : 2;        // Bits 38-37, POWER_OK_CFG
    uint32_t Reserved_50_39 : 12;    // Bits 50-39
    uint8_t Power_Only_Above_5V : 1; // Bit     51, POWER_ONLY_ABOVE_5V
    uint8_t Req_Src_Current : 1;     // Bit     52, REQ_SRC_CURRENT
    uint8_t Reserved_55_53 : 3;      // Bits 55-53
    uint8_t Alert_Status_1_Mask : 8; // Bits 63-56
  } __attribute__((packed)) bank4;
} NVM_TypeDef;

enum PowerOkConfig
{
  CONFIGURATION_1,
  CONFIGURATION_2,
  CONFIGURATION_3,
};

namespace esphome
{
  namespace stusb4500
  {
    class STUSB4500Component : public i2c::I2CDevice, public Component
    {
    public:
      void set_pd_state_sensor(sensor::Sensor *pd_state_sensor) { this->pd_state_sensor_ = pd_state_sensor; };
      void set_pd_status_sensor(text_sensor::TextSensor *pd_status_sensor) { this->pd_status_sensor_ = pd_status_sensor; };

      void set_alert_pin(InternalGPIOPin *pin) { this->alert_pin_ = pin; };
      void set_flash_nvm(bool nvm) { this->flash_nvm_ = nvm; };
      void set_default_nvm(bool nvm) { this->default_nvm_ = nvm; };
      void set_snk_pdo_numb(uint8_t num) { this->snk_pdo_numb_ = num; };
      void set_v_snk_pdo2(float voltage) { this->v_snk_pdo2_ = voltage; };
      void set_v_snk_pdo3(float voltage) { this->v_snk_pdo3_ = voltage; };
      void set_i_snk_pdo1(float current) { this->i_snk_pdo1_ = current; };
      void set_i_snk_pdo2(float current) { this->i_snk_pdo2_ = current; };
      void set_i_snk_pdo3(float current) { this->i_snk_pdo3_ = current; };
      void set_i_snk_pdo_flex(float current) { this->i_snk_pdo_flex_ = current; };
      void set_shift_vbus_hl1(float percentage) { this->shift_vbus_hl1_ = percentage; };
      void set_shift_vbus_ll1(float percentage) { this->shift_vbus_ll1_ = percentage; };
      void set_shift_vbus_hl2(float percentage) { this->shift_vbus_hl2_ = percentage; };
      void set_shift_vbus_ll2(float percentage) { this->shift_vbus_ll2_ = percentage; };
      void set_shift_vbus_hl3(float percentage) { this->shift_vbus_hl3_ = percentage; };
      void set_shift_vbus_ll3(float percentage) { this->shift_vbus_ll3_ = percentage; };
      void set_vbus_disch_time_to_0v(uint32_t milliseconds) { this->vbus_disch_time_to_0v_ = milliseconds; };
      void set_vbus_disch_time_to_pdo(uint32_t milliseconds) { this->vbus_disch_time_to_pdo_ = milliseconds; };
      void set_vbus_disch_disable(bool disable) { this->vbus_disch_disable_ = disable; };
      void set_power_ok_cfg(PowerOkConfig value) { this->power_ok_cfg_ = value; };
      void set_usb_comm_capable(bool capable) { this->usb_comm_capable_ = capable; };
      void set_snk_uncons_power(bool unconstrained) { this->snk_uncons_power_ = unconstrained; };
      void set_req_src_current(bool require) { this->req_src_current_ = require; };
      void set_power_only_above_5v(bool above) { this->power_only_above_5v_ = above; };

      // ========== INTERNAL METHODS ==========
      void setup() override;
      // void update() override;
      // void loop() override;
      void dump_config() override;
      // float get_setup_priority() const override { return setup_priority::BUS; }

    protected:
      const uint8_t REG_DEFAULT = 0xFF;
      const uint8_t REG_FTP_CUST_PASSWORD = 0x95;
      const uint8_t FTP_CUST_PASSWORD = 0x47;
      const uint8_t REG_FTP_CTRL_0 = 0x96;
      const uint8_t FTP_CUST_PWR = 0x80;
      const uint8_t FTP_CUST_RST_N = 0x40;
      const uint8_t FTP_CUST_REQ = 0x10;
      const uint8_t FTP_CUST_SECT = 0x07;
      const uint8_t REG_FTP_CTRL_1 = 0x97;
      const uint8_t FTP_CUST_SER = 0xF8;
      const uint8_t FTP_CUST_OPCODE = 0x07;
      const uint8_t READ = 0x00;
      const uint8_t WRITE_PL = 0x01;
      const uint8_t WRITE_SER = 0x02;
      const uint8_t ERASE_SECTOR = 0x05;
      const uint8_t PROG_SECTOR = 0x06;
      const uint8_t SOFT_PROG_SECTOR = 0x07;
      const uint8_t SECTOR_0 = 0x01;
      const uint8_t SECTOR_1 = 0x02;
      const uint8_t SECTOR_2 = 0x04;
      const uint8_t SECTOR_3 = 0x08;
      const uint8_t SECTOR_4 = 0x10;
      
      const uint8_t REG_DPM_PDO_NUMB = 0x70;
      const uint8_t REG_DPM_PDO = 0x85;
      const uint8_t REG_RDO_REG_STATUS = 0x91;
      const uint8_t REG_RW_BUFFER = 0x53;
      const uint8_t REG_TX_HEADER_LOW = 0x51;
      
      const uint8_t REG_ALERT_STATUS_1 = 0x0B;
      const uint8_t REG_ALERT_STATUS_MASK = 0x0C;
      const uint8_t REG_PORT_STATUS_TRANS = 0x0D;
      const uint8_t REG_PORT_STATUS = 0x0E;
      const uint8_t REG_TYPEC_MONITORING_STATUS_0 = 0x0F;
      const uint8_t REG_TYPEC_MONITORING_STATUS_1 = 0x10;
      const uint8_t REG_CC_STATUS = 0x11;
      const uint8_t REG_CC_HW_FAULT_STATUS_0 = 0x12;
      const uint8_t REG_CC_HW_FAULT_STATUS_1 = 0x13;
      const uint8_t REG_PD_TYPEC_STATUS = 0x14;
      const uint8_t REG_REG_TYPE_C_STATUS = 0x15;
      const uint8_t REG_PRT_STATUS = 0x16;
      const uint8_t REG_PHY_STATUS = 0x17;
      const uint8_t REG_CC_CAPABILITY_CTRL = 0x18;
      const uint8_t REG_PRT_TX_CTRL = 0x19;
      const uint8_t REG_PD_COMMAND_CTRL = 0x1A;
      const uint8_t SOFT_RESET_MESSAGE = 0x0D;
      const uint8_t SEND_MESSAGE = 0x26;
      const uint8_t REG_DEV_CTRL = 0x1D;

      NVM_TypeDef sector[5];
      uint8_t pd_status_{255};

      // Config variables
      bool flash_nvm_{false};
      bool default_nvm_{false};
      uint8_t snk_pdo_numb_{3};
      float v_snk_pdo2_{15.0};
      float v_snk_pdo3_{20.0};
      float i_snk_pdo1_{1.5};
      float i_snk_pdo2_{1.5};
      float i_snk_pdo3_{1.0};
      float i_snk_pdo_flex_{2.0};
      float shift_vbus_hl1_{0.10};
      float shift_vbus_ll1_{0.15};
      float shift_vbus_hl2_{0.05};
      float shift_vbus_ll2_{0.15};
      float shift_vbus_hl3_{0.05};
      float shift_vbus_ll3_{0.15};
      uint16_t vbus_disch_time_to_0v_{756};
      uint16_t vbus_disch_time_to_pdo_{288};
      bool vbus_disch_disable_{false};
      PowerOkConfig power_ok_cfg_{CONFIGURATION_2};
      bool usb_comm_capable_{false};
      bool snk_uncons_power_{false};
      bool req_src_current_{false};
      bool power_only_above_5v_{false};

      // Pins
      InternalGPIOPin *alert_pin_{nullptr};

      // Sensors
      sensor::Sensor *pd_state_sensor_{nullptr};
      text_sensor::TextSensor *pd_status_sensor_{nullptr};

      // local variables
      SNK_PDO_TypeDef pdos_[3];
      RDO_REG_STATUS_TypeDef rdo_;
      uint8_t pdo_numb_{3};
      uint8_t nvm_flashed_{false};

      // member functions
      void write_pdos_();
      void write_pdo_(uint8_t pdo_numb, SNK_PDO_TypeDef pdo_data);
      void read_pdos_();
      SNK_PDO_TypeDef read_pdo_(uint8_t pdo_numb);
      RDO_REG_STATUS_TypeDef read_rdo_status_();
      void set_pdo_voltage_(uint8_t pdo_numb, float voltage);
      float get_pdo_voltage_(uint8_t pdo_numb);
      void set_pdo_current_(uint8_t pdo_numb, float current);
      float get_pdo_current_(uint8_t pdo_numb);
      void set_pdo_number_(uint8_t pdo_numb);
      uint8_t get_pdo_number_();
      void soft_reset_();
      void dump_snk_pdo(uint8_t pdo_numb);

      // NVM functions
      // void dump_nvm_();
      // void nvm_get_default_(NVM_TypeDef *default);
      uint8_t nvm_compare_();
      // uint8_t nvm_compare_default_();
      void nvm_load_default_();
      void nvm_update_();
      uint8_t nvm_read_();
      uint8_t nvm_write_();
      uint8_t nvm_wait_();
      uint8_t nvm_get_pdo_number_();
      float nvm_get_pdo_voltage_(uint8_t pdo_numb);
      float nvm_get_pdo_current_(uint8_t pdo_numb);
      float nvm_get_upper_limit_percentage_(uint8_t pdo_numb);
      float nvm_get_lower_limit_percentage_(uint8_t pdo_numb);
      float nvm_get_flex_current_();
      uint16_t nvm_get_vbus_disch_time_to_0v_();
      uint16_t nvm_get_vbus_disch_time_to_pdo_();
      uint8_t nvm_get_vbus_disch_disable_();
      PowerOkConfig nvm_get_power_ok_cfg_();
      uint8_t nvm_get_external_power_(void);
      uint8_t nvm_get_usb_comm_capable_(void);
      uint8_t nvm_get_config_ok_gpio_(void);
      uint8_t nvm_get_gpio_ctrl_(void);
      uint8_t nvm_get_power_above_5v_only_(void);
      uint8_t nvm_get_req_src_current_(void);
      void nvm_set_pdo_number_(uint8_t pdo_numb);
      void nvm_set_pdo_voltage_(uint8_t pdo_numb, float voltage);
      void nvm_set_pdo_current_(uint8_t pdo_numb, float current);
      void nvm_set_upper_limit_percentage_(uint8_t pdo_numb, float percent);
      void nvm_set_lower_limit_percentage_(uint8_t pdo_numb, float percent);
      void nvm_set_flex_current_(float current);
      void nvm_set_vbus_disch_time_to_0v_(uint16_t value);
      void nvm_set_vbus_disch_time_to_pdo_(uint16_t value);
      void nvm_set_vbus_disch_disable_(uint8_t value);
      void nvm_set_power_ok_cfg_(PowerOkConfig value);
      void nvm_set_external_power_(uint8_t value);
      void nvm_set_usb_comm_capable_(uint8_t value);
      void nvm_set_config_ok_gpio_(uint8_t value);
      void nvm_set_gpio_ctrl_(uint8_t value);
      void nvm_set_power_above_5v_only_(uint8_t value);
      void nvm_set_req_src_current_(uint8_t value);
    };

  } // namespace stusb4500
} // namespace esphome
