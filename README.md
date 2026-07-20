# ESPHome-Components

The components in this repository have been separated into individual repositories. Initially I intended to keep all components in one directory to make things easier for me but it became clear to a minor change in one component should not cause a release for all components.

The following separate repositories are now available.

## New Component Repsitories

* [TFMini](https://github.com/mikelawrence/esphome-component-tfmine)
* [STUSB4500](https://github.com/mikelawrence/esphome-component-stusb4500)
* [DFROBOT_C4001](https://github.com/mikelawrence/esphome-component-dfrobot-c4001)
* [LD2410S](https://github.com/mikelawrence/esphome-component-ld2410s)
* [SEN5X](https://github.com/mikelawrence/esphome-component-sen5x)
* [SEN6X](https://github.com/mikelawrence/esphome-component-sen6x)
* [ESP32 RMT PWM](https://github.com/mikelawrence/esphome-component-esp32-rmt-pwm)

Original README include for completeness

------------------------------------------------------------------------------------------------------------------------

## Installation

The preferred method to add your own components to ESPHome is to use [External Components](https://esphome.io/components/external_components.html#external-components-git).

If you want to make all components available use this config.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
```

If you want to make specific components available use this config.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ tfmini ]
```

## Available Components

* [TFMini](#tfmini-external-component)
* [STUSB4500](#stusb4500-external-component)
* [C4001](#c4001-external-component)
* [LD2410S](#ld2410s-external-component)
* [SEN5X](#sen5x-external-component)
* [SEN6X](#sen6x-external-component)
* [ESP32 RMT PWM](#esp32-rmt-pwm-external-component)

## TFMini External Component

These ToF (time of flight) range finder sensors are compact, self-contained range finders. They support both UART and I<sub>2</sub>C communication but this component only supports UART communication.

This component supports the following Benewake LiDAR Range Finder Sensors.

TFMini Plus|TFMini-S| TFLuna
:---------:|:------:|:-----:
<img src="https://en.benewake.com/uploadfiles/2023/03/20230328182059608.png"> | <img src="https://en.benewake.com/uploadfiles/2023/03/20230328182039063.jpg"> | <img src="https://en.benewake.com/uploadfiles/2023/03/20230328182002559.jpg">

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ tfmini ]

uart:
  - id: tfmini_uart
    tx_pin: GPIO25
    rx_pin: GPIO24
    baud_rate: 115200
    data_bits: 8
    parity: NONE
    stop_bits: 1

sensor:
  - platform: tfmini
    model: TFMINI_PLUS
    sample_rate: 10
    low_power: true
    temperature:
      id: tfmini_temperature
      name: "Temperature"
    distance:
      id: tfmini_distance
      internal: true
      name: "TFMini Distance"
      accuracy_decimals: 1
    signal_strength:
      id: tfmini_signal_strength
      name: "TFMini Signal Strength"
      filters:
        - throttle: 1s
```

### Configuration Variables

* **model** (**Required**, string): The model of the Range Finder Sensor. Options are `TFMINI_PLUS` , `TFMINI_S` or `TF_LUNA` .
* **uart_id** (*Optional*, string): Manually specify the ID of the [UART Bus](https://esphome.io/components/uart) if you use multiple UART buses.
* **sample_rate** (*Optional*, integer): The frame rate at which the sensor will output sensor data in samples per sec. For the TFMINI_PLUS and TFMINI_S the range is 1-1000. For the TFLuna the range is 1-500. Note when low_power mode is set to true for the TFMINI_S and the TFLuna model the is significantly lower from 1-10. Default is 100.
* **config_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#config-pin-schema)): This pin when connected will be set high to enable UART mode. (*TF_LUNA only*)
* **low_power** (*Optional*, boolean): Turns on low power mode. This also requires sample_rate to be 10 or less. (*TF_LUNA, TFMini-S only*)

### Sensors

* **distance** (*Optional*): Distance in cm. For the TFMINI_PLUS and TFMINI_S the range is 10-1200cm. For the TFLuna the range is 20-800cm. A distance of 10000cm means the sensor is not receiving enough signal, most likely open air. A distance of 0cm means the sensor is saturated and there is no measurement possible. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
* **signal_strength** (*Optional*): Represents the signal strength with a range of 0-65535. The longer the measurement distance, the lower signal strength will be. The lower the reflectivity is, the lower the signal strength will be. When signal strength is less than 100 detection is unreliable and distance is set to 10000cm. When signal strength is 65535 detection is unreliable and distance is set to 0cm. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
* **temperature** (*Optional*): Internal temperature in °C. It's not clear how useful this sensor because it certainly does not measure room temperature. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

## STUSB4500 External Component

The STUSB4500 is a USB power delivery controller that supports sink up to 100 W (20V, 5A). It has Non-Volatile Memory that can be programmed with your PDO profile so when you connect to a USB-C Power Source it will immediately negotiate your power delivery contract.

<p align="center">
    <img src="https://www.sparkfun.com/media/catalog/product/cache/a793f13fd3d678cea13d28206895ba0c/1/5/15801-SparkFun_Power_Delivery_Board_-_USB-C__Qwiic_-04.jpg" width="30%"><br />
    STUSB4500 board from SparkFun Electronics
</p>

There are 3 PDOs that you can configure with PD1 fixed at 5V. You can vary PD1's current. If you have all three PDOs configured the STUSB4500 will try to negotiate a contract with the PDOs in the following order: PD3, PD2 and PD1. If you don't want PDO1 negotiated then set `power_only_above_5v: true` .

The configuration below the STUSB4500 will try to negotiate 12V @ 3A then 9V @ 3A. If neither are available then it won't negotiate at all.

It also creates a `PD State` and `PD Status` Text Sensor.

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ stusb4500 ]

stusb4500:
  alert_pin: GPIO8
  snk_pdo_numb: 3
  v_snk_pdo2: 9.00V
  i_snk_pdo2: 3.00A
  v_snk_pdo3: 12.00V
  i_snk_pdo3: 3.00A
  power_only_above_5v: true
  usb_comms_capable: true

sensor:
  - platform: stusb4500
    pd_state:
      name: "PD State"

text_sensor:
  - platform: stusb4500
    pd_status:
      name: "PD Status"
```

### Configuration Variables

* **flash_nvm** (*Optional*, boolean): When set to `true` the STUSB4500 NVM will be flashed with the current settings but only if different. To be on the safe side you should not leave this set to `true` . A power cycle is required to renegotiate. Default is `false` .
* **default_nvm** (*Optional*, boolean): When set to `true` the STUSB4500 NVM will be flashed with default settings but only if different. To be on the safe side you should not leave this set to `true` . A power cycle is required to renegotiate. Default is `false` .
* **snk_pdo_numb** (*Optional*, integer): Set the number of PDOs that should be negotiated. Range is 1 - 3. A value of 3 means PDO3, PDO2 and PDO1 can be negotiated. A value of 2 means PDO2 and PDO1 can be negotiated. A value of 1 means only PDO1 can be negotiated. Default is 3.
* **v_snk_pdo2** (*Optional*, float): This is the desired PDO2 voltage. Range is 5.0 to 20.0. Default is 20.0.
* **v_snk_pdo3** (*Optional*, float): This is the desired PDO3 voltage. Range is 5.0 to 20.0. Default is 15.0.
* **i_snk_pdo1** (*Optional*, float): This is the desired PDO1 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.5.
* **i_snk_pdo2** (*Optional*, float): This is the desired PDO2 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.5.
* **i_snk_pdo3** (*Optional*, float): This is the desired PDO3 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.0.
* **shift_vbus_hl1** (*Optional*, percentage): This is the percentage above PDO1 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 10%.
* **shift_vbus_ll1** (*Optional*, percentage): This is the percentage below PDO1 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
* **shift_vbus_hl2** (*Optional*, percentage): This is the percentage above PDO2 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 5%.
* **shift_vbus_ll2** (*Optional*, percentage): This is the percentage below PDO2 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
* **shift_vbus_hl3** (*Optional*, percentage): This is the percentage above PDO3 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 5%.
* **shift_vbus_ll3** (*Optional*, percentage): This is the percentage below PDO3 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
* **vbus_disch_time_to_0v_** (*Optional*, time): Coefficient used to compute V<sub>BUS</sub> discharge time to 0V. Range is 84ms to 1260ms and must be a multiple of 84ms. Default is 756ms.
* **vbus_disch_time_to_pdo_** (*Optional*, time): Coefficient used to compute V<sub>BUS</sub> discharge time when transitioning to lower PDO voltage. Range is 24ms to 360ms and must be a multiple of 24ms. Default is 288ms.
* **vbus_disch_disable** (*Optional*, boolean): When set to `true` the STUSB4500 NVM will stop discharging V<sub>BUS</sub>. Default is `false` .
* **usb_comms_capable** (*Optional*, boolean): When set to `true` the STUSB4500 will tell the connected device that USB communications are possible. Default is `false` .
* **snk_uncons_power** (*Optional*, boolean): When set to `true` the STUSB4500 will tell the connected device external power is available. Default is `false` .
* **req_src_current** (*Optional*, boolean): When set to `true` the STUSB4500 will report the source current from source instead of the sink. Default is `false` .
* **power_ok_cfg** (*Optional*, enumeration): Selects the POWER_OK pins configuration. Options are `CONFIGURATION_1` , `CONFIGURATION_2` or `CONFIGURATION_3` . Default is `CONFIGURATION_2` .
* **gpio_cfg** (*Optional*, enumeration): Selects the GPIO configuration. Options are `SW_CTRL_GPIO` , `ERROR_RECOVERY` , `DEBUG` or `SINK_POWER` . Default is `ERROR_RECOVERY` .
* **power_only_above_5v** (*Optional*, boolean): When set to `true` the STUSB4500 will enable V<sub>BUS</sub> only if PDO2 or PDO3 was negotiated. When `false` V<sub>BUS</sub> is enabled at 5V when connected to a non-PD charger. The available current is unknown. Default is `false` .

### Sensors

* **pd_state** (*Optional*): Current PD State as an integer. Range 0 to 3 where 0 is no PD negotiated. A value of 1, 2 or 3 indicates which PDO was negotiated. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

### Text Sensors

* **pd_status** (*Optional*): Easy to read PD State (e.g. "12V @ 3A" ). All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

### Actions

When the GPIO is configured for software control `gpio_cfg: SW_CTRL_GPIO` these actions control the state of the output.

* **stusb4500.turn_gpio_on** Will pull the GPIO low.
* **stusb4500.turn_gpio_off** Will set the output to High-Z.

Example using automations...

```yaml
on_value_range:
  - below: 5.0
    then:
      - stusb4500.turn_gpio_on: pd_controller
```

Example in lambdas...

```yaml
- lambda: |-
  id(pd_controller).turn_gpio_on();
```

### NVM Configuration

> [!CAUTION]
> When configuring the STUSB4500 for the first time you should realize the default configuration will allow up to 20V on V<sub>BUS</sub>. Make sure your circuit can either support 20V or configure the NVM using a Power Source that will NOT produce 20V like USB 2.0 or a non-PD USB-C charger.

Using the STUSB4500 ESPHome component requires the following steps:

1. Set the configuration variables to match your requirements. The sample configuration above sets PDO3 to 12V @ 3A, PDO2 to 9V @3A and PDO1 is disabled because `power_only_above_5v: true` . This means it will first try to negotiate 12V @ 3A. If unavailable then it will try 9V @ 3A. If is unavailable no PD will be negotiated. Either the `pd_state` or `pd_status` sensor will let you know if there is a problem.
2. Add `flash_nvm: true` to the configuration.
   You should see the following messages in your log.

     ```log
     [00:10:03][C][stusb4500:112]: STUSB4500:
     [00:10:03][E][stusb4500:119]:   NVM has been flashed, power cycle the device to reload NVM
     `

   If you forgot to add `flash_nvm: true` to the configuration you will see an error in the log.

     ```log
     [00:06:27][C][stusb4500:112]: STUSB4500:
     [00:06:27][E][stusb4500:114]:   NVM does not match current settings, you must set flash_nvm: true
     [00:06:27][C][stusb4500:130]:   PDO3 negotiated 9.00V @ 3.00A, 27.00W
     ```

3. Power cycle your board. Make sure `NVM matches settings` is present in the log and that one of your PDOs was negotiated. Configuring the STUSB4500 can be a bit finicky, but keep at it.

   ```log
   [00:00:19][C][stusb4500:112]: STUSB4500:
   [00:00:19][C][stusb4500:116]:   NVM matches settings
   [00:00:19][C][stusb4500:130]:   PDO3 negotiated 12.00V @ 3.00A, 36.00W
   ```

4. The STUSB4500 component does check that the NVM is indeed different before flashing it but it is prudent to remove the `flash_nvm: true` after it is clear the STUSB4500 is working as configured.

## C4001 External Component

> [!NOTE]
> This component is in the process of being added to ESPHome. ESPHome [PR#9327](https://github.com/esphome/esphome/pull/9327).

The DFRobot C4001 (SEN0609 or SEN0610) is a millimeter-wave presence detector. The C4001 millimeter-wave presence sensor has the advantage of being able to detect both static and moving objects. It also has a relatively strong anti-interference ability, making it less susceptible to factors such as temperature changes, variations in ambient light, and environmental noise. Whether a person is sitting, sleeping, or in motion, the sensor can quickly detect their presence.

<p align="center">
    <img src="https://dfimg.dfrobot.com/enshop/SEN0609/SEN0609_Main_01.jpg" width="50%"><br />
    C4001 (SEN0609) 25m mmWave Presence Sensor
</p>

There are two variants:

* SEN0609 has a 100° horizontal and 40° vertical field of view, 16 meter presence detection range and 25 meter motion detection range.
* SEN0610 has a 100° horizontal and 80° vertical field of view, 8 meter presence detection range and 12 meter motion detection range.

> [!NOTE]
> Some settings have different ranges depending on the variant used. This component treats both variants the same, so it is your responsibility to make sure your configuration sets these values appropriately.

The sensor can operate in one of two modes, `Presence` and `Speed and Distance` . In `Presence` mode the sensor provides a singular occupancy output. The presence output once presence is detected will stay on for a period that can be configured. In `Speed and Distance` mode the occupancy binary sensor indicates if a target is being tracked or not. Each time the sensor indicates presence it also outputs target distance, target speed and target energy. In `Speed and Distance` mode all of these parameter update frequently. There are only two settings for this mode, micro_motion_enable switch and threshold_factor number.

The C4001 sensor maintains settings in flash. When powered on these settings are loaded from flash and made operational. To change the configuration of the sensor dial in the setting you need and hit the config_save button. This will tell the sensor to store the new settings in flash and make them operational. You only need to do this once.

More information on the C4001 (SEN0609) sensor is available [here](https://www.dfrobot.com/product-2793.html). Information on the C4001 (SEN0610) sensor is available [here](https://www.dfrobot.com/product-2795.html).

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ dfrobot_c4001 ]

dfrobot_c4001:
  id: mmwave_sensor
  uart_id: mmwave_uart
  mode: PRESENCE
```

### Configuration Variables

* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the `dfrobot_c4001` component. Required if there are multiple DFRobot C4001s configured.

* **uart_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID of the UART Component to use. Required if you have multiple UARTs configured.

* **mode** (*Required*, enumeration): This sets the operation mode of the sensor. Options are `PRESENCE` and `SPEED_AND_DISTANCE`.

### Buttons

The `dfrobot_c4001` button allows you to perform `config save` actions on your DFRobot C4001.

```yaml
button:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    config_save:
      name: Config Save
      entity_category: CONFIG
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **config_save** (*Optional*): When you click this button the current configuration will be saved. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).
* **factory_reset** (*Optional*): Clicking this button will perform a factory reset of the module and all configuration values will go back to default. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).
* **restart** (*Optional*): When you click this button the module will restart and all configuration values will remains as previously set. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).

### Binary Sensors

The `dfrobot_c4001` binary sensor report `config changed` and `occupancy` state information about your DFRobot C4001.

```yaml
binary_sensor:
  - platform: dfrobot_c4001
    config_changed:
      name: Config Changed
    occupancy:
      id: occupancy
      name: Occupancy
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **config_changed** (*Optional*): When `true` the current sensor configuration has been changed but not saved to the sensor. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).
* **occupancy** (*Optional*): In `PRESENCE` mode this indicates presence. In `SPEED_AND_DISTANCE` mode this indicates a target is being tracked. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).

### Numbers

The `dfrobot_c4001` button allows you to perform `config save` actions on your DFRobot C4001.

```yaml
number:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    max_range:
      name: Range Max
    min_range:
      name: Range Min
    trigger_range:
      name: Range Trigger
    hold_sensitivity:
      name: Sensitivity Hold
    trigger_sensitivity:
      name: Sensitivity Trigger
    on_latency:
      name: Latency On
    off_latency:
      name: Latency Off
    inhibit_time:
      name: Inhibit Time
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **min_range** (*Optional*): This is the minimum detection range. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The manual recommends not changing this value. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **max_range** (*Optional*): This is the maximum detection range. Default is 6 meters (m) with a range of 0.6 to 25.0 m. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **trigger_range** (*Optional*): Sets the maximum range at which occupancy can switch to present. The range between max detection range and trigger detection range can NOT cause occupancy to switch to present. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **hold_sensitivity** (*Optional*): The number represents the ease in which the sensor switches to the present state when someone enters the sensing range of the sensor. Default is 7 (no units) with a range of 0 to 9, higher is more sensitive. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **trigger_sensitivity** (*Optional*): This number represents ease of continued presence detection after the sensor switched to the present state. Default is 5 (no units) with a range of 0 to 9, higher is more sensitive. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **on_latency** (*Optional*): This time value is how long presence is detected before switching to the present state. Default is 0.050 (seconds) with a range of 0.0 to 100.0. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **off_latency** (*Optional*): This time value is how long the after the sensor no longer detects presence before switching to the not present state. Default is 15 (seconds) with a range of 0 to 1500. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **inhibit_time** (*Optional*): The dead-time after switching to the not present state before presence can be detected again. Default is 1 (seconds) with a range of 0.1 to 255.0. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **threshold_factor** (*Optional*): The larger the number the larger the object and more motion is required to trigger the sensor to switch to target tracked state. Default is 5 with a range of 0 to 65535. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

### Sensors

The `dfrobot_c4001`  sensors report `software version` and `firmware version` from your DFRobot C4001.

```yaml
sensor:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    software_version:
      name: Software Version
    hardware_version:
      name: Hardware Version
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **target_distance** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates distance to target in meters (m). When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).
* **target_speed** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates target speed in meters per second (m/s). When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).
* **target_energy** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates target energy in no units. When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).

### Text Sensors

The `dfrobot_c4001` text sensors report `software version` and `firmware version` from your DFRobot C4001.

```yaml
text_sensor:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    software_version:
      name: Software Version
    hardware_version:
      name: Hardware Version
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **software_version** (*Optional*): Software Version as reported by the module. All Options from [Text Sensor Component](https://esphome.io/components/sensor/#base-text-sensor-configuration).
* **firmware_version** (*Optional*): Firmware Version as reported by the module. All Options from [Text Sensor Component](https://esphome.io/components/text_sensor/#base-text-sensor-configuration).

### Switches

The `dfrobot_c4001` switch allows you to enable the output LED on your DFRobot C4001.

```yaml
switch:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    led_enable:
      name: Enable LED
```

#### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **led_enable** (*Optional*): When turned on the green LED will flash when the sensor has been started. The blue LED cannot be disabled with this command. All Options from [Switch Component](https://esphome.io/components/switch/#base-switch-configuration).
* **micro_motion_enable** (*Optional*): Turns on micro motion mode. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Switch Component](https://esphome.io/components/switch/#base-switch-configuration).

### Actions

* **dfrobot_c4001.factory_reset** Will perform a factory reset of the module and all configuration values will go back to default. The module will restart with these defaults. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. This is much easier to do with a lambda that accidentally performs a factory reset every second.

Example using automations...

```yaml
button:
  - platform: template
    name: Factory Reset
    on_press:
      - dfrobot_c4001.factory_reset: mmwave_sensor
    entity_category: CONFIG
```

Example in lambdas...

```yaml
- lambda: |-
  id(mmwave_sensor).factory_reset();
```

* **dfrobot_c4001.restart** Will restart the module and all configuration values will remains as previously set.

Example using automations...

```yaml
button:
  - platform: template
    name: Restart
    on_press:
      - dfrobot_c4001.restart: mmwave_sensor
    entity_category: CONFIG
```

Example in lambdas...

```yaml
- lambda: |-
  id(mmwave_sensor).restart();
```

## LD2410S External Component

<p align="center">
    <img src="https://www.hlktech.net/res/_cache/auto/15/1575.png" width="50%"><br />
    Hi-Link LD2410S 8m mmWave Presence Sensor
</p>

This custom component is based on [PR #8486](https://github.com/esphome/esphome/pull/8486) on ESPHome. I've copied it here since I doubt it will be merged into ESPHome. There is risk that it could disappear on ESPHome and it's not setup as an external component in [NovakIrs repository](https://github.com/NovakIrs/esphome/tree/ld2410s). This PR works just fine. Again all credit goes to [NovakIrs](https://github.com/NovakIrs) repository.

I have made changes to this component. Specifically a recent update to the documentation indicates the older serial documentation may be out of date. The new documentation indicates the Threshold command now contains the Trigger and Hold Thresholds for the first 8 gates and the SNR command contains the Trigger and Hold Thresholds for the last 8 gates. The first 8 gates have a rang of 10 to 95dB and the last 8 gates have a range of 5 to 63dB. It's worth noting that the documentation implies the sensor is only good at tracking Moving AND Still targets in the first 8 gates. The last 8 gates may only work for Moving targets and possibly with dimenishing results as you get close to the last gate.

I have also made a lot of changes to make the component match the LD2410 component and to follow ESPHome guidelines. Just couldn't help myself. There are basically no changes to the serial comms and it's associated command parsing.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ ld2410s ]

uart:
  - id: ld2410s_uart
    tx_pin: GPIO17
    rx_pin: GPIO18
    baud_rate: 115200
    parity: NONE
    stop_bits: 1

ld2410s:
  id: ld2410s_radar
  uart_id: ld2410s_uart
```

### Configuration Variables

* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **uart_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID of the UART Component to use. Required if you have multiple UARTs configured.

## Buttons

The `ld2410s` button allows you to perform `calibration start` actions on your LD2410S.

```yaml
button:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    cal_start:
      name: Calibration Start
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **cal_start** (*Optional*): When you click this button the built-in LD2410S Auto-Calibration will start. Be sure to have the room in the idle state, i.e. no people, pets or robot vaccumes running. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).

## Binary Sensors

The `ld2410s` binary sensor `has target` and `calibration running` state information on your LD2410S.

```yaml
binary_sensor:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    has_target:
      name: MMWAVE Presence
    cal_running:
      name: Calibration Running
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **has_target** (*Optional*): When `true` the radar has detected a target either moving or still. This is effectivly presence. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).

* **calibration_running** (*Optional*): When `true` Auto-Calibration is running and `false` when not. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).

> [!NOTE]
> The has_target [Binary_Sensor](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration) above includes the following [Filter](https://https://esphome.io/components/binary_sensor/#binary-sensor-filters).
> `- settle: 1s`
> If you have defined other filters, this default will be overridden; you may of course add it back to your custom filter(s) as above if you wish.
> To remove the default filter for any given sensor instance, add `filters: []` to its configuration.

### Numbers

The `ld2410s` number platform allows you to control the gate distance, no detection delay, reporting frequency and threshold hold and threshold trigger for each gate of your LD2410S.

```yaml
number:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    max_distance:
      name: Max Detect Distance
    min_distance:
      name: Min Detect Distance
    no_delay:
      name: No detect Report Delay
    status_reporting_frequency:
      name: Status Reporting Frequency
    distance_reporting_frequency:
      name: Distance Reporting Frequency
    g0:
      trigger_threshold:
        name: G0 Trigger Threshold
      hold_threshold:
        name: G0 Hold Threshold
    g1:
      trigger_threshold:
        name: G1 Trigger Threshold
      hold_threshold:
        name: G1 Hold Threshold
    g2:
      trigger_threshold:
        name: G2 Trigger Threshold
      hold_threshold:
        name: G2 Hold Threshold
    g3:
      trigger_threshold:
        name: G3 Trigger Threshold
      hold_threshold:
        name: G3 Hold Threshold
    g4:
      trigger_threshold:
        name: G4 Trigger Threshold
      hold_threshold:
        name: G4 Hold Threshold
    g5:
      trigger_threshold:
        name: G5 Trigger Threshold
      hold_threshold:
        name: G5 Hold Threshold
    g6:
      trigger_threshold:
        name: G6 Trigger Threshold
      hold_threshold:
        name: G6 Hold Threshold
    g7:
      trigger_threshold:
        name: G7 Trigger Threshold
      hold_threshold:
        name: G7 Hold Threshold
    g8:
      trigger_threshold:
        name: G8 Trigger Threshold
      hold_threshold:
        name: G8 Hold Threshold
    g9:
      trigger_threshold:
        name: G9 Trigger Threshold
      hold_threshold:
        name: G9 Hold Threshold
    g10:
      trigger_threshold:
        name: G10 Trigger Threshold
      hold_threshold:
        name: G10 Hold Threshold
    g11:
      trigger_threshold:
        name: G11 Trigger Threshold
      hold_threshold:
        name: G11 Hold Threshold
    g12:
      trigger_threshold:
        name: G12 Trigger Threshold
      hold_threshold:
        name: G12 Hold Threshold
    g13:
      trigger_threshold:
        name: G13 Trigger Threshold
      hold_threshold:
        name: G13 Hold Threshold
    g14:
      trigger_threshold:
        name: G14 Trigger Threshold
      hold_threshold:
        name: G14 Hold Threshold
    g15:
      trigger_threshold:
        name: G15 Trigger Threshold
      hold_threshold:
        name: G15 Hold Threshold
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **max_distance_gate** (*Optional*): This is the maximum detection gate. Default is 15 with a range of 0 to 15 in steps of 1. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

* **min_distance_gate** (*Optional*): This is the minimum detection gate. Default is 0 with a range of 0 to 15 in steps of 1. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

* **no_delay** (*Optional*): This is the no detection delay. Basically a delay off function. Default is 10 seconds (s) with a range of 0 to 120s in steps of 1s. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

* **status_reporting_frequency** (*Optional*): This is `has_target` binary sensor reporting frequency. Default is 8.0Hz with a range of 0.5 to 8.0Hz. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

* **distance_reporting_frequency** (*Optional*): This is `target_distance` binary sensor reporting frequency. Default is 8.0Hz with a range of 0.5 to 8.0Hz. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

* **gX** (*Optional*): Thresholds for the Xth gate (X => 0 to 15). The gate sparation is 0.7m. Note the datsheet indicates static detection is effective only in the first 8 gates. The higher gates will still detect motion.

  * **threshold_hold** (*Optional*): This represents the threshold to which a target will no longer be detected for the given gate. Default is gate specific and range is 10 to 95dB for gates 0 - 7 and 5 to 63dB for gate 8 - 15 in steps of 1dB. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

  * **threshold_trigger** (*Optional*): Trigger Threshold detemine when the energy is enough to switch to target detected for the given gate. Default is gate specific and range is 10 to 95dB for gates 0 - 7 and 5 to 63dB for gate 8 - 15 in steps of 1dB. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

### LD2410S Selects

The `ld2410s` select allows you to control the response speed of your LD2410S.

```yaml
select:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    response_speed:
      name: Response Speed
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **response_speed** (*Optional*): This is how quickly the sensor switches to no occupancy.You have two options `Normal` and `Fast`. All Options from [Select Component](https://esphome.io/components/select/#base-select-configuration).

### LD2410S Sensors

The `ld2410s` sensor reports the `target distance`, `calibration progress` and gate energies for each gate of your LD2410S.

```yaml
sensor:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    cal_progress:
      name: Calibration Progress
    g0_energy:
      name: G0 Energy
    g1_energy:
      name: G1 Energy
    g2_energy:
      name: G2 Energy
    g3_energy:
      name: G3 Energy
    g4_energy:
      name: G4 Energy
    g5_energy:
      name: G5 Energy
    g6_energy:
      name: G6 Energy
    g7_energy:
      name: G7 Energy
    g8_energy:
      name: G8 Energy
    g9_energy:
      name: G9 Energy
    g10_energy:
      name: G10 Energy
    g11_energy:
      name: G11 Energy
    g12_energy:
      name: G12 Energy
    g13_energy:
      name: G13 Energy
    g14_energy:
      name: G14 Energy
    g15_energy:
      name: G15 Energy
    target_distance:
      name: Target Distance
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **cal_progress** (*Optional*): This is the current Auto-Calibration progress in perecent (%). All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).

* **gX energy** (*Optional*): Energies for the Xth gate (X => 0 to 15). Range is 0 to 100dB in steps of 1dB. Only operates when Minimal Output is off. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

> [!NOTE]
> Each of the [Sensor Components](https://esphome.io/components/sensor/#base-sensor-configuration) above include the following [Filter](https://esphome.io/components/sensor/#sensor-filters).
> `- throttle_with_priority: 1s`
> If you have defined other filters, this default will be overridden; you may of course add it back to your custom filter(s) as above if you wish.
> To remove the default filter for any given sensor instance, add `filters: []` to its configuration.

* **target_distance** (*Optional*): This sensor indicates current distance to target in meters (m). All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).

### LD2410S Switches

The `ld2410s` switch allows to the turn on `minimal output` on your LD2410S.

```yaml
switch:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    minimal_output:
      name: Minimal Output
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **minimal_output** (*Optional*): When turned on the unit stops sending detailed target parameters like individual gate data and only streams simple presence states over the serial port . All Options from [Switch Component](https://esphome.io/components/switch/#base-switch-configuration).

### LD2410S Text Sensors

The `ld2410s` text sensor reports the `firmware version` of your LD2410S.

```yaml
text_sensor:
  - platform: ld2410s
    ld2410s_id: ld2410s_radar
    fw_version:
      name: Firmware version
```

### Configuration Variables

* **ld2410s_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the ld2410s component. Required if there are multiple LD2410Ss configured.

* **fw_version** (*Optional*): The is the LD2410S firmware version. All Options from [Sensor Component](https://esphome.io/components/text_sensor/#base-text-sensor-configuration).

## SEN5X External Component

<p align="center">
    <img src="https://sensirion.com/_next/image?url=https%3A%2F%2Fsensirion.com%2Fmedia%2Fportfolio%2Fseries%2Fimage%2F6a057318-e34a-4b2c-9303-5ac180312d85.webp&w=2048&q=75" width="50%"><br />
    SEN55 Environmental Sensor
</p>

> [!NOTE]
> ESPHome now has a SEN6X component. They decided to not use my pull request that supported the SEN6X sensors in the SEN5X component after 6 mounths of work. Kinda sucked but it what it is. I ended up supporting the SEN6X component integration but it's not really the way I would have handled it. I doubt I will support ESPHome directly in the future but I will release my components here. This component is here incase y0ou want to use the sen5x compoenent from that PR

This external component adds SEN60, SEN63C, SEN65, SEN66 and SEN68 support to the built-in sen5x component.

> [!NOTE]
> I recommend you use my [sen6x component](#sen6x-external-component) below or the ESPHome built-in sen6x component. This will not be updated frequently.

```yaml
# Load from this repository
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ sen5x ]

# Example SEN66 configuration entry
sensor:
  - platform: sen6x
    id: my_sen66
    type: SEN66
    temperature_compensation:
      offset: 0
      normalized_offset_slope: 0
      time_constant: 0
    temperature_acceleration:
      k: 20
      p: 20
      t1: 100
      t2: 300
    pm_1_0:
      name: "PM <1µm Mass concentration"
    pm_2_5:
      name: "PM <2.5µm Mass concentration"
    pm_4_0:
      name: "PM <4µm Mass concentration"
    pm_10_0:
      name: "PM <10µm Mass concentration"
    temperature:
      name: "Temperature"
    humidity:
      name: "Humidity"
    voc:
      name: "VOC"
      store_algorithm_state: true
      algorithm_tuning:
        index_offset: 100
        learning_time_offset_hours: 12
        learning_time_gain_hours: 12
        gating_max_duration_minutes: 180
        std_initial: 50
        gain_factor: 230
    nox:
      name: "NOx"
      algorithm_tuning:
        index_offset: 1
        learning_time_offset_hours: 12
        gating_max_duration_minutes: 720
        gain_factor: 230
    co2:
      name: "CO₂"
      automatic_self_calibration: false
      ambient_pressure_compensation_source: pressure_hpa
```

### SEN5X Configuration Variables

* **type** (*Required*, enum): The type of the connected sensor. Must be one of the following:
  SEN50, SEN54, SEN55, SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **pm_1_0** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  1μm in size. Readings in µg/m³. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **pm_2_5** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  2.5μm in size. Readings in µg/m³. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **pm_4_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  4μm in size. Readings in µg/m³. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **pm_10_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  10μm in size. Readings in µg/m³. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **temperature** (*Optional*): The information for the Temperature sensor. Only available with SEN54, SEN55,
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **humidity** (*Optional*): The information for the Relative Humidity sensor. Only available with SEN54, SEN55,
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **co2** (*Optional*): The information for the Carbon dioxide (CO₂) sensor. Readings in ppm. Only available with
  SEN63C, SEN66 or SEN69C.

  * **auto_self_calibration** (*Optional*, boolean): True enables automatic CO₂ self calibration.
  False disables automatic CO₂ calibration. Default is `true`.
  * **altitude_compensation** (*Optional*, integer): When set to altitude (in meters), the CO₂ sensor will be
  statically compensated for deviations due to current altitude. See [CO₂ Compensation](#co₂-compensation) section below
  for more information.
  * **ambient_pressure_compensation** (*Optional*, integer): When set to pressure (in hPA), the CO₂ sensor will be
  compensated for deviations due to ambient pressure. See [CO₂ Compensation](#co₂-compensation) section below
  for more information.
  * **ambient_pressure_compensation_source** (*Optional*, [ID](/guides/configuration-types#config-id)):
  Sets an external pressure sensor ID (must report in hPA). This will compensate the CO₂ sensor for deviations
  due to current pressure. This correction is applied with each update of the CO₂ sensor. See
  [CO₂ Compensation](#co₂-compensation) section below for more information.
  * All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **voc** (*Optional*): The information for the VOC Index sensor. Only available with SEN54, SEN55, SEN65, SEN66,
  SEN69 or SEN69C.

  * **algorithm_tuning** (*Optional*): The VOC algorithm can be customized by tuning 6 different parameters.
    For more details see
    [Engineering Guidelines for SEN5x](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf).

    * **index_offset** (*Optional*): VOC index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    * **learning_time_offset_hours** (*Optional*): Time constant to estimate the VOC algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    * **learning_time_gain_hours** (*Optional*): Time constant to estimate the VOC algorithm gain from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hours.
    * **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high VOC index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    * **std_initial** (*Optional*): Initial estimate for standard deviation. Lower value boosts events during
      initial learning period, but may result in larger device-to-device variations.
      Allowed values are in range 10..5000. The default value is 50.
    * **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the VOC index output.
      Allowed values are in range 1..1000. The default value is 230. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **nox** (*Optional*): NOx Index. Only available with SEN55, SEN65, SEN66, SEN69 or SEN69C.

  * **algorithm_tuning** (*Optional*): Like VOC the NOx algorithm can be customized by tuning 5 different parameters.

    * **index_offset** (*Optional*): NOx index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    * **learning_time_offset_hours** (*Optional*): Time constant to estimate the NOx algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    * **learning_time_gain_hours** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    * **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high NOx index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    * **std_initial** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    * **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the NOx index output.
      Allowed values are in range 1..1000. The default value is 230. All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **hcho** (*Optional*): The information for the Formaldehyde (HCHO) sensor. Readings in ppb. Only available with
  SEN68 or SEN69C.
  
  * All options from [Sensor](https://esphome.io/components/sensor/#base-sensor-configuration).

* **store_baseline** (*Optional*, boolean): When set to `true` the VOC algorithm state is saved to flash every
  2 hours. During setup of the sensor the previously saved algorithm state is loaded and the VOC sensor will
  skip the initial learning phase.
  Only available with SEN54, SEN55, SEN65, SEN66, SEN68 or SEN69C.

* **auto_cleaning_interval** (*Optional*, positive int): The periodic fan-cleaning interval in seconds.
  Only available with SEN55, SEN54 OR SEN55.

* **temperature_compensation** (*Optional*, sequence): These parameters allow the user to compensate temperature
  effects of the customer design by applying custom temperature offsets to the ambient temperature. Only available
  with SEN54, SEN55, SEN62, SEN63C, SEN65, SEN66, SEN69 or SEN69C.
  See [Temperature Compensation](#sen5x-temperature-compensation) section below for more information.

  * **offset** (*Optional*, float): Temperature offset, in °C. Defaults to `0`.
  * **normalized_offset_slope** (*Optional*, float): Normalized temperature offset slope. Defaults to `0`.
  * **time_constant** (*Optional*, positive int): Time constant in seconds. Defaults to `0`.
  
* **acceleration_mode** (*Optional*): Allowed value are `low`, `medium` and `high`. Defaults to `low`.
  Only available with SEN54 or SEN55.

  By default, the RH/T acceleration algorithm is optimized for a sensor which is positioned in free air.
  If the sensor is integrated into another device, the ambient RH/T output values might not be optimal
  due to different thermal behavior.

  This parameter can be used to adapt the RH/T acceleration behavior for the actual use-case, leading in an
  improvement of the ambient RH/T output accuracy. There is a limited set of different modes available.
  Medium and high accelerations are particularly indicated for air quality monitors which are subjected to
  large temperature changes. Low acceleration is advised for stationary devices not subject to large
  variations in temperature.

  For more information see
  [Temperature Acceleration and Compensation Instructions for SEN5x.](https://sensirion.com/media/documents/9B9DE2A7/61E957EB/Sensirion_Temperature_Acceleration_and_Compensation_Instructions_SEN.pdf).

* **address** (*Optional*, int): Manually specify the I²C address of the sensor. Defaults to `0x69` for
  SEN5X sensors and `0x6B` for SEN6X sensors.

> [!NOTE]
> This component reports readings as soon as they are available without regard initial accuracy.
> Your configuration should limit reporting of sensor values for a period of time after power-up.
> A good starting point is 5 minutes.
>
> * The PM sensor has a start-up time of 30 seconds.
> * The temperature sensor has a response time of 1 minute with no mention start-up time.
> * The humidity sensor has a response time of 20 seconds with no mention start-up time.
> * The VOC sensor will start detecting events in 1 minute but may take up to 1 hour to meet
>   data sheet specifications.
> * The NOx sensor will start detecting events in 5 minutes but may take up to 6 hours to meet
>   data sheet specifications.
> * The CO₂ sensor has a response time of between 60 and 70 seconds with no mention start-up time.
> * The HCHO sensor has a start-up time of 10 minutes.

### SEN5X Actions

Multiple actions are available with this component and all are mutually exclusive. Actions take time to complete.
While an individual action is running the sensor is otherwise occupied and cannot be accessed. Attempting to run
an action while another action is already running results in a 'Sensor is busy' log warning. Several actions also
require the sensor to be in the idle state with no measurements running.

#### SEN5X Fan Cleaning

Both sensor families support manual running of the fan cleaning cycle by using the
`sen5x.start_fan_autoclean` action. Only available with the SEN54, SEN55, SEN62, SEN63C, SEN65,
SEN66, SEN68 or SEN69C.

{{< anchor "start_fan_autoclean_action" >}}

##### SEN5X `sen5x.start_fan_autoclean` Action

This [action](https://esphome.io/automations/actions/#actions) manually starts fan cleaning. During the fan cleaning
process sensor measurements are paused or stopped, depending on the sensor, while the fan is running at
the elevated rate. The entire fan cleaning sequence takes 12 seconds.

```yaml
on_...:
  then:
    - sen5x.start_fan_autoclean: sen54
```

You can emulate the SEN5X automatic fan cleaning on a SEN6X sensor by calling the `sen5x.start_fan_autoclean:`
action periodically.

For example, to clean the fan every 7 days while the device is on, as recommended by the manufacturer, the
following configuration can be added:

```yaml
interval:
  - interval: 7d
    then:
      - sen5x.start_fan_autoclean: my_sen66
```

#### SEN5X Humidity Sensor Heater

The SEN6X humidity sensor can develop an offset in the humidity reading when exposed to high levels of
humidity for extended periods of time. It supports a heater similar to the one in the SHT4X. The
difference is no automatic mode. Instead you have to trigger `sen5x.activate_heater` action occasionally.

##### SEN5X `activate_heater` Action

This [action](https://esphome.io/automations/actions/#actions) manually starts the heater. First all measurements are
stopped, then the heater is turned on at 200mW for 1s, finally there is a 20 second delay before
reenabling the measurements. This is to ensure the heating effects are gone before temperature measurements
resume. The entire activate heater sequence takes 22 seconds.

```yaml
on_...:
  then:
    - sen5x.activate_heater: my_sen66
```

#### SEN5X CO₂ Calibration

The CO₂ sensor by default has auto-calibration enabled. Auto-calibration will adjust the minimum measurement
over the last week or so to the outdoor average of slightly more than 400 ppm. Auto-calibration assumes that
you are opening the windows at least once a week. If you don't open the windows then over time the CO₂ level
will tend downward.

If you know your minimums are not going to be 400 ppm then you can disable auto-calibration and occasionally
take the sensor outside for 5 minutes and then force a manual CO₂ calibration to the expected outdoor CO₂ level.
Be sure to watch the log output of your device when you perform this action. If the sensor reports an error
during the recalibration process it will be reported in the log.

##### SEN5X `perform_forced_co2_recalibration` Action

This [action](https://esphome.io/automations/actions/#actions) forces a manual calibration on the CO₂ sensor. Measurements
are stopped before issuing the forced co2 recalibrate command to the sensor. The entire perform forced co2
recalibration action takes 2 seconds.

The example below will recalibrate the CO₂ sensor when the "CO₂ Calibrate" button is pressed using the
"CO₂ Calibration Value" number's current value.

```yaml
number:
  - platform: template
    id: co2_forced_cal_value
    name: "CO₂ Calibration Value"
    device_class: carbon_dioxide
    entity_category: CONFIG
    optimistic: true
    max_value: 1200
    min_value: 400
    step: 1
    initial_value: 420
button:
  - platform: template
    name: "CO₂ Calibrate"
    entity_category: CONFIG
    on_press:
      - sen5x.perform_forced_co2_recalibration:
          value: !lambda "return id(co2_forced_cal_value).state;"
          id: sen66_sensor
sensor:
  - platform: sen5x
    type: SEN66
    id: sen66_sensor
    co2:
      name: "CO₂"
```

#### SEN5X CO₂ Compensation

The CO₂ sensor supports pressure/altitude compensation to improve CO₂ accuracy. If a pressure sensor is available
you can dynamically adjust pressure compensation by either adding `ambient_pressure_compensation_source` to your
configuration for automatic updates or you can periodically call the `sen5x.set_ambient_pressure_compensation`
action with the current ambient pressure. You can also statically define `altitude_compensation`.

##### SEN5X Dynamic example with a local sensor

```yaml
sensor:
  - platform: bmp581
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        - lambda: |-
            // convert Pa to hPa (or mBar)
            return x * 0.01;
  - platform: sen5x
    type: SEN69C
    co2:
      name: "CO₂"
      ambient_pressure_compensation_source: pressure_hpa
```

##### SEN5X Dynamic example `set_ambient_pressure_compensation` Action

This [action](https://esphome.io/automations/actions/#actions) updates the current pressure used in CO₂ pressure
compensation. Must be in hPa or mBar. Note: Once `set_ambient_pressure_compensation` is called `altitude_compensation`,
if set in the configuration, will be ignored. Only available with SEN63C, SEN66 or SEN69C.

```yaml
sensor:
  - platform: bmp581
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        - lambda: |-
            // convert Pa to hPa (or mBar)
            return x * 0.01;
    on_value:
      then:
        - lambda: !lambda |-
            id(sen66_sensor).set_ambient_pressure_compensation(x);
  - platform: sen5x
    type: SEN69C
    id: sen66_sensor
    co2:
      name: "CO₂"
```

##### SEN5X Static example with altitude

```yaml
sensor:
  - platform: sen5x
    type: SEN66
    co2:
      name: "CO₂"
      altitude_compensation: 427m
```

### SEN5X Automatic Fan Cleaning

The SEN5X sensors have an automatic fan-cleaning procedure will be triggered periodically following
`auto_cleaning_interval` cleaning interval. This will accelerate the fan to maximum speed for 10 seconds
to blow out the accumulated dust inside the fan.

* Measurement values are not updated while the fan-cleaning is running.
* The default cleaning interval is set to 604,800 seconds (i.e., 168 hours or 1 week).
* The interval can be configured using the Set Automatic Cleaning Interval command.
* Set the interval to 0 to disable the automatic cleaning.
* The cleaning procedure can also be started manually with the `start_fan_autoclean` Action.

### SEN5X NOx and VOC Algorithm Tuning

Both the NOx and VOC sensor support algorithm tuning. These variables are set with the `algorithm_tuning`
configuration under the `voc` and `nox` sensors. For more details see
[Engineering Guidelines for SEN5X](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf)

### SEN5X Temperature Compensation

The SEN54, SEN55, SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C contain an internal temperature
compensation mechanism. The compensated ambient temperature is calculated as follows:

```c++
T_Ambient_Compensated = T_Ambient + (slope*T_Ambient) + offset
```

Where slope and offset are the values set with `temperature_compensation` configuration variables, smoothed
with the specified time constant, also a `temperature_compensation` configuration variable. The time constant
is how fast the slope and offset are applied. After the specified value in seconds, 63% of the new slope and
offset are applied.

More details about the tuning of these parameters for SEN5X sensors are included in the application note:
[Temperature Acceleration and Compensation Instructions for SEN5x](https://sensirion.com/media/documents/9B9DE2A7/61E957EB/Sensirion_Temperature_Acceleration_and_Compensation_Instructions_SEN.pdf).

The SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C support temperature compensation using the same formula
above but with the added feature of up to five slots. At this time only slot 0 is supported. A later update
will correct this issue.

More details about the tuning of these parameters for SEN6X sensors are included in the application note:
[SEN6x – Temperature Acceleration and Compensation Instructions](https://sensirion.com/media/documents/C964FCC8/693FD554/PS_AN_SEN6x_Temperature_Compensation_and_Acceleration_Application_No.pdf).

## SEN6X External Component

<p align="center">
    <img src="https://sensirion.com/_next/image?url=https%3A%2F%2Fsensirion.com%2Fmedia%2Fportfolio%2Fproduct%2Fimage%2Fd2b23157-ec1c-40f7-b89f-e38ad4d4a40f.webp&w=2048&q=75" width="50%"><br />
    SEN66 Environmental Sensor
</p>

This external component adds SEN60, SEN63C, SEN65, SEN66 and SEN68 support to ESPHome.

Temperature compensation is not working for the SEN6x models. Still waiting on the Sensirion Application Note.

```yaml
# Example SEN66 sensor configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ sen5x ]

sensor:
  - platform: sen6x
    id: my_sen66
    type: SEN66
    temperature_compensation:
      offset: 0
      normalized_offset_slope: 0
      time_constant: 0
    temperature_acceleration:
      k: 20
      p: 20
      t1: 100
      t2: 300
    pm_1_0:
      name: "PM <1µm Mass concentration"
    pm_2_5:
      name: "PM <2.5µm Mass concentration"
    pm_4_0:
      name: "PM <4µm Mass concentration"
    pm_10_0:
      name: "PM <10µm Mass concentration"
    temperature:
      name: "Temperature"
    humidity:
      name: "Humidity"
    voc:
      name: "VOC"
      store_algorithm_state: true
      algorithm_tuning:
        index_offset: 100
        learning_time_offset_hours: 12
        learning_time_gain_hours: 12
        gating_max_duration_minutes: 180
        std_initial: 50
        gain_factor: 230
    nox:
      name: "NOx"
      algorithm_tuning:
        index_offset: 1
        learning_time_offset_hours: 12
        gating_max_duration_minutes: 720
        gain_factor: 230
    co2:
      name: "CO₂"
      automatic_self_calibration: false
      ambient_pressure_compensation_source: pressure_hpa
```

### Configuration Variables

* **type** (*Required*, enum): The type of the connected sensor.Provides compile time config validation of sensors. Must be one of the following:
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](/components/sensor#config-sensor).

* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the sen6x component. Required if there are multiple SEN6Xs configured.

* **address** (*Optional*, int): Manually specify the I²C address of the sensor. Defaults to `0x6B`.

* **temperature_acceleration** (*Optional*): This command allows user to set custom temperature acceleration
  parameters. Light is intended for smaller devices or devices with less thermal mass. Strong is the opposite.

  This is a good starting point for these values.

  | Acceleration Level     |   T1    |   T1    |   K    |   P    |
  | -----------------------|:-------:|:-------:|:------:|:------:|
  | Light (IAQM)           |   100   |   300   |   20   |   20   |
  | Middle                 |   100   |   300   |   50   |   20   |
  | Strong (Air Purifier)  |   250   |   300   |  150   |   20   |

  For more information see
  [SEN6x – Temperature Acceleration and Compensation Instructions](https://sensirion.com/media/documents/C964FCC8/693FD554/PS_AN_SEN6x_Temperature_Compensation_and_Acceleration_Application_No.pdf).

  * **k** (*Required*): Filter constant K.
  * **p** (*Required*): Filter constant P.
  * **t1** (*Required*): Time constant T1 in seconds.
  * **t2** (*Required*): Time constant T2 in seconds.

* **temperature_compensation** (*Optional*, sequence): These parameters allow the user to compensate temperature
  effects of the design-in at the customer side by applying custom temperature offsets to the ambient temperature.
  Temperature Compensation is a sequence (called slots) of compensation parameter sets. Up to five
  temperature compensation sets are supported.
  See [Temperature Compensation](#temperature-compensation) section below for more information.

  * **offset** (*Optional*, float): Temperature offset, in °C. Defaults to `0`.
  * **normalized_offset_slope** (*Optional*, float): Normalized temperature offset slope. Defaults to `0`.
  * **time_constant** (*Optional*, positive int): Time constant in seconds. Defaults to `0`.

### SEN6X Sensors

* **pm_1_0** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  1μm in size. Readings in µg/m³. All options from [Sensor](/components/sensor#config-sensor).

* **pm_2_5** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  2.5μm in size. Readings in µg/m³. All options from [Sensor](/components/sensor#config-sensor).

* **pm_4_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  4μm in size. Readings in µg/m³. All options from [Sensor](/components/sensor#config-sensor).

* **pm_10_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  10μm in size. Readings in µg/m³. All options from [Sensor](/components/sensor#config-sensor).

* **temperature** (*Optional*): The information for the Temperature sensor. Only available with
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](/components/sensor#config-sensor).

* **humidity** (*Optional*): The information for the Relative Humidity sensor. Only available with
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C. All options from [Sensor](/components/sensor#config-sensor).

* **co2** (*Optional*): The information for the Carbon dioxide (CO₂) sensor. Readings in ppm. Only available with
  SEN63C, SEN66 or SEN69C.

  * **auto_self_calibration** (*Optional*, boolean): True enables automatic CO₂ self calibration.
  False disables automatic CO₂ calibration. Default is `true`.
  * **altitude_compensation** (*Optional*, integer): When set to altitude (in meters), the CO₂ sensor will be
  statically compensated for deviations due to current altitude. See [CO₂ Compensation](#co₂-compensation) section below
  for more information.
  * **ambient_pressure_compensation_source** (*Optional*, [ID](/guides/configuration-types#config-id)):
  Sets an external pressure sensor ID (must report in hPA). This will compensate the CO₂ sensor for deviations
  due to current pressure. This correction is applied with each update of the CO₂ sensor. See
  [CO₂ Compensation](#co₂-compensation) section below for more information.
  * All options from [Sensor](/components/sensor#config-sensor).

* **voc** (*Optional*): The information for the VOC Index sensor. Only available with SEN65, SEN66, SEN69 or SEN69C.

  * **store_algorithm_state** (*Optional*, boolean): When set to `true` the VOC algorithm state is saved to flash every
    2 hours. During setup of the sensor the previously saved algorithm state is loaded and the VOC sensor will
    skip the initial learning phase.
    Only available with SEN65, SEN66, SEN68 or SEN69C.
  * **algorithm_tuning** (*Optional*): The VOC algorithm can be customized by tuning 6 different parameters.
    For more details see
    [Engineering Guidelines for SEN5X](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf)

    * **index_offset** (*Optional*): VOC index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    * **learning_time_offset_hours** (*Optional*): Time constant to estimate the VOC algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    * **learning_time_gain_hours** (*Optional*): Time constant to estimate the VOC algorithm gain from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hours.
    * **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high VOC index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    * **std_initial** (*Optional*): Initial estimate for standard deviation. Lower value boosts events during
      initial learning period, but may result in larger device-to-device variations.
      Allowed values are in range 10..5000. The default value is 50.
    * **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the VOC index output.
      Allowed values are in range 1..1000. The default value is 230. All options from [Sensor](/components/sensor#config-sensor).

* **nox** (*Optional*): NOx Index. Only available with SEN65, SEN66, SEN69 or SEN69C.

  * **algorithm_tuning** (*Optional*): Like VOC the NOx algorithm can be customized by tuning 5 different parameters.
    For more details see
    [Engineering Guidelines for SEN5X](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf)

    * **index_offset** (*Optional*): NOx index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    * **learning_time_offset_hours** (*Optional*): Time constant to estimate the NOx algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    * **learning_time_gain_hours** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    * **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high NOx index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    * **std_initial** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    * **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the NOx index output.
      Allowed values are in range 1..1000. The default value is 230. All options from [Sensor](/components/sensor#config-sensor).

* **hcho** (*Optional*): The information for the Formaldehyde (HCHO) sensor. Readings in ppb. Only available with
  SEN68 or SEN69C. All options from [Sensor](/components/sensor#config-sensor).

> [!NOTE]
> This component reports readings as soon as they are available without regard initial accuracy.
> Your configuration should limit reporting of sensor values for a period of time after power-up.
> A good starting point is 5 minutes.
>
> * The PM sensor has a start-up time of 30 seconds.
> * The temperature sensor has a response time of 1 minute with no mention start-up time.
> * The humidity sensor has a response time of 20 seconds with no mention start-up time.
> * The VOC sensor will start detecting events in 1 minute but may take up to 1 hour to meet
>   data sheet specifications.
> * The NOx sensor will start detecting events in 5 minutes but may take up to 6 hours to meet
>   data sheet specifications.
> * The CO₂ sensor has a response time of between 60 and 70 seconds with no mention start-up time.
> * The HCHO sensor has a start-up time of 10 minutes.

### Actions

Multiple actions are available with this component and are queued when requested. Queued actions will wait until the sensor is not busy, either from other actions or reading sensor data and the perform the requested actions. Some actions will pause sensor readings while the action is in progress. Trying to queue the same action more than once does nto make sense and the second action will be ignored.

#### Fan Cleaning

The sensor supports a manual fan cleaning cycle by using the
`sen6x.start_fan_cleaning` action.

##### `sen6x.start_fan_cleaning` Action

This [action](/automations/actions#all-actions) manually starts fan cleaning. During the fan cleaning
process sensor measurements are paused or stopped, depending on the sensor, while the fan is running at
the elevated rate. The entire fan cleaning sequence takes 12 seconds.

```yaml
on_...:
  then:
    - sen6x.start_fan_cleaning: sen62
```

The previous generation SEN5X sensor had an optional automatic fan cleaning every 7 days. You can emulate this
automatic fan cleaning by calling the `sen6x.start_fan_cleaning:` action periodically.

For example, to clean the fan every 7 days while the device is on, as recommended by the manufacturer, the
following configuration can be added:

```yaml
interval:
  - interval: 7d
    then:
      - sen6x.start_fan_cleaning: my_sen66
```

#### Humidity Sensor Heater

The SEN6X humidity sensor can develop an offset in the humidity reading when exposed to high levels of
humidity for extended periods of time. It supports a heater similar to the one in the SHT4X. The
difference is no automatic mode. Instead you have to trigger `sen6x.activate_heater` action occasionally.

##### `sen6x.activate_heater` Action

This [action](/automations/actions#all-actions) manually starts the heater. First all measurements are
stopped, then the heater is turned on at 200mW for 1s, finally there is a 20 second delay before
reenabling the measurements. This is to ensure the heating effects are gone before temperature measurements
resume. The entire activate heater sequence takes 22 seconds.

```yaml
on_...:
  then:
    - sen6x.activate_heater: my_sen66
```

#### CO₂ Calibration

The CO₂ sensor by default has auto-calibration enabled. Auto-calibration will adjust the minimum measurement
over the last week or so to the outdoor average of slightly more than 400 ppm. Auto-calibration assumes that
you are opening the windows at least once a week. If you don't open the windows then over time the CO₂ level
will tend downward.

If you know your minimums are not going to be 400 ppm then you can disable auto-calibration and occasionally
take the sensor outside for 5 minutes and then force a manual CO₂ calibration to the expected outdoor CO₂ level.
Be sure to watch the log output of your device when you perform this action. If the sensor reports an error
during the recalibration process it will be reported in the log.

##### `sen6x.perform_forced_co2_recalibration` Action

This [action](/automations/actions#all-actions) forces a manual calibration on the CO₂ sensor. Measurements
are stopped before issuing the forced co2 recalibrate command to the sensor. The entire perform forced co2
recalibration action takes 2 seconds.

The example below will recalibrate the CO₂ sensor when the "CO₂ Calibrate" button is pressed using the
"CO₂ Calibration Value" number's current value.

```yaml
number:
  - platform: template
    id: co2_forced_cal_value
    name: "CO₂ Calibration Value"
    device_class: carbon_dioxide
    entity_category: CONFIG
    optimistic: true
    max_value: 1200
    min_value: 400
    step: 1
    initial_value: 420
button:
  - platform: template
    name: "CO₂ Calibrate"
    entity_category: CONFIG
    on_press:
      - sen6x.perform_forced_co2_recalibration:
          value: !lambda "return id(co2_forced_cal_value).state;"
          id: sen66_sensor
sensor:
  - platform: sen6x
    type: SEN66
    id: sen66_sensor
    co2:
      name: "CO₂"
```

#### CO₂ Compensation

The CO₂ sensor supports pressure/altitude compensation to improve CO₂ accuracy. If a pressure sensor is available
you can dynamically adjust pressure compensation by either adding `ambient_pressure_compensation_source` to your
configuration for automatic updates or you can periodically call the `sen6x.set_ambient_pressure_compensation`
action with the current ambient pressure. You can also statically define `altitude_compensation`.

##### Dynamic example with a local sensor

```yaml
sensor:
  - platform: bmp581
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        - lambda: |-
            // convert Pa to hPa (or mBar)
            return x * 0.01;
  - platform: sen6x
    type: SEN69C
    co2:
      name: "CO₂"
      ambient_pressure_compensation_source: pressure_hpa
```

##### Dynamic example `sen6x.set_ambient_pressure_compensation` Action

This [action](/automations/actions#all-actions) updates the current pressure used in CO₂ pressure compensation.
Must be in hPa or mBar. Note: Once `set_ambient_pressure_compensation` is called `altitude_compensation`, if
set in the configuration, will be ignored. Only available with SEN63C, SEN66 or SEN69C.

```yaml
sensor:
  - platform: bmp581
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        - lambda: |-
            // convert Pa to hPa (or mBar)
            return x * 0.01;
    on_value:
      then:
        - lambda: !lambda |-
            id(sen66_sensor).set_ambient_pressure_compensation(x);
  - platform: sen6x
    type: SEN69C
    id: sen66_sensor
    co2:
      name: "CO₂"
```

##### Static example with altitude

```yaml
sensor:
  - platform: sen6x
    type: SEN66
    co2:
      name: "CO₂"
      altitude_compensation: 427m
```

#### Temperature Compensation

These sensors contain an internal temperature compensation mechanism. The compensated ambient temperature is
calculated as follows:

```c++
T_Ambient_Compensated = T_Ambient + (slope*T_Ambient) + offset
```

Where slope and offset are the values set with `temperature_compensation` configuration variables, smoothed
with the specified time constant, also a `temperature_compensation` configuration variable. The time constant
is how fast the slope and offset are applied. After the specified value in seconds, 63% of the new slope and
offset are applied.

More details about the tuning of these parameters is included in the application note:
[SEN6x – Temperature Acceleration and Compensation Instructions](https://sensirion.com/media/documents/C964FCC8/693FD554/PS_AN_SEN6x_Temperature_Compensation_and_Acceleration_Application_No.pdf).

##### `sen6x.set_temperature_compensation` Action

This [action](/automations/actions#all-actions) give you access to temperature compensation slots 0-4.
This is a dynamic compensation that for example can be used when internal devices are turned on that cause additional heating.

```yaml
button:
  - platform: template
    name: "Set Slot 1 TC"
    on_press:
      - sen6x.set_temperature_compensation:
          id: sen66_sensor
          slot: 1
          offset: -1.2
          normalized_offset_slope: 0
          time_constant: 0
```

## ESP32 RMT PWM External Component

PWM output using the ESP-IDF RMT driver for ESP32 devices.

This component provides a continuous PWM output using an RMT TX channel in infinite loop mode. It exposes a standard ESPHome `output` platform and is suitable for driving loads like fans or LED drivers that expect a hardware PWM signal.

> **Important:** This component has been **tested on ESP32-C3 only**. The implementation uses the generic ESP-IDF RMT driver API and is expected to work on other ESP32 variants with RMT support, but those targets are currently **untested**.

```yaml
# Sample configuration entry example
external_components:
  - source: github://mikelawrence/esphome-components
    components:
      - esp32_rmt_pwm

output:
  - platform: esp32_rmt_pwm
    id: fan_pwm_output
    pin: GPIO4
    frequency: 25kHz
    rmt_symbols: 96

fan:
  - platform: speed
    name: "RMT PWM Fan"
    output: fan_pwm_output
    speed_count: 3
```

### ESP32 RMT PWM Configuration Variables

* **id** (*Required*, ID)
  The ID of the output component. This is used to reference the output from other ESPHome components (lights, fans, custom components, etc.).

* **pin** (*Required*, [Pin Schema](https://esphome.io/guides/configuration-types#config-pin-schema))
  The GPIO pin used for the PWM output signal.

* **frequency** (*Optional*, frequency, default: `25kHz`)
  The PWM frequency.
  Internally, the component:

  * Uses an RMT resolution of 10 MHz (100 ns per tick).
  * Computes the PWM period in ticks as:
    * `period_ticks = resolution_hz / frequency`
  * Validates that:
    * `period_ticks >= 2` (to avoid absurdly high frequencies)
    * `period_ticks <= 32767` (to fit the 15-bit RMT symbol duration field)

  If the requested frequency falls outside these bounds, the component logs an error and marks itself as failed during setup rather than generating an invalid PWM signal.

* **rmt_symbols** (*Optional*, integer, variant-specific default, minimum: `48`)

  Number of RMT symbols allocated for the TX channel (`mem_block_symbols`).

  This value controls how much RMT memory is reserved for this channel. The component itself uses a single symbol for the PWM period, but the allocation size still affects resource usage and plays more of a role if the implementation is later extended. You should override the default if you need to optimize RMT memory usage or are running multiple RMT-based components.
