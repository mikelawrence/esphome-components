# ESPHome-Components

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

## ESPHome TFMini External Component

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
+ **model** (**Required**, string): The model of the Range Finder Sensor. Options are ```TFMINI_PLUS```, ```TFMINI_S``` or ```TF_LUNA```.
+ **uart_id** (*Optional*, string): Manually specify the ID of the [UART Bus](https://esphome.io/components/uart) if you use multiple UART buses.
+ **sample_rate** (*Optional*, integer): The frame rate at which the sensor will output sensor data in samples per sec. For the TFMINI_PLUS and TFMINI_S the range is 1-1000. For the TFLuna the range is 1-500. Note when low_power mode is set to true for the TFMINI_S and the TFLuna model the is significantly lower from 1-10. Default is 100. 
+ **config_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#config-pin-schema)): This pin when connected will be set high to enable UART mode. (*TF_LUNA only*) 
+ **low_power** (*Optional*, boolean): Turns on low power mode. This also requires sample_rate to be 10 or less. (*TF_LUNA, TFMini-S only*) 

### Sensors
+ **distance** (*Optional*): Distance in cm. For the TFMINI_PLUS and TFMINI_S the range is 10-1200cm. For the TFLuna the range is 20-800cm. A distance of 10000cm means the sensor is not receiving enough signal, most likely open air. A distance of 0cm means the sensor is saturated and there is no measurement possible. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
+ **signal_strength** (*Optional*): Represents the signal strength with a range of 0-65535. The longer the measurement distance, the lower signal strength will be. The lower the reflectivity is, the lower the signal strength will be. When signal strength is less than 100 detection is unreliable and distance is set to 10000cm. When signal strength is 65535 detection is unreliable and distance is set to 0cm. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
+ **temperature** (*Optional*): Internal temperature in °C. It's not clear how useful this sensor because it certainly does not measure room temperature. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

## ESPHome STUSB4500 External Component

The STUSB4500 is a USB power delivery controller that supports sink up to 100 W (20V, 5A). It has Non-Volatile Memory that can be programmed with your PDO profile so when you connect to a USB-C Power Source it will immediately negotiate your power delivery contract.

<p align="center">
    <img src="https://www.sparkfun.com/media/catalog/product/cache/a793f13fd3d678cea13d28206895ba0c/1/5/15801-SparkFun_Power_Delivery_Board_-_USB-C__Qwiic_-04.jpg" width="30%"><br />
    STUSB4500 board from SparkFun Electronics
</p>

There are 3 PDOs that you can configure with PD1 fixed at 5V. You can vary PD1's current. If you have all three PDOs configured the STUSB4500 will try to negotiate a contract with the PDOs in the following order: PD3, PD2 and PD1. If you don't want PDO1 negotiated then set ```power_only_above_5v: true```.

The configuration below the STUSB4500 will try to negotiate 12V @ 3A then 9V @ 3A. If neither are available then it won't negotiate at all. 

It also creates a ```PD State``` and ```PD Status``` Text Sensor.

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
+ **flash_nvm** (*Optional*, boolean): When set to ```true``` the STUSB4500 NVM will be flashed with the current settings but only if different. To be on the safe side you should not leave this set to ```true```. A power cycle is required to renegotiate. Default is ```false```.
+ **default_nvm** (*Optional*, boolean): When set to ```true``` the STUSB4500 NVM will be flashed with default settings but only if different. To be on the safe side you should not leave this set to ```true```. A power cycle is required to renegotiate. Default is ```false```.
+ **snk_pdo_numb** (*Optional*, integer): Set the number of PDOs that should be negotiated. Range is 1 - 3. A value of 3 means PDO3, PDO2 and PDO1 can be negotiated. A value of 2 means PDO2 and PDO1 can be negotiated. A value of 1 means only PDO1 can be negotiated. Default is 3.
+ **v_snk_pdo2** (*Optional*, float): This is the desired PDO2 voltage. Range is 5.0 to 20.0. Default is 20.0.
+ **v_snk_pdo3** (*Optional*, float): This is the desired PDO3 voltage. Range is 5.0 to 20.0. Default is 15.0.
+ **i_snk_pdo1** (*Optional*, float): This is the desired PDO1 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.5.
+ **i_snk_pdo2** (*Optional*, float): This is the desired PDO2 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.5.
+ **i_snk_pdo3** (*Optional*, float): This is the desired PDO3 current. Range is 0.0 to 5.0. 5.0A is only available with 20V. For all other voltages the maximum current is 3.0A. Default is 1.0.
+ **shift_vbus_hl1** (*Optional*, percentage): This is the percentage above PDO1 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 10%.
+ **shift_vbus_ll1** (*Optional*, percentage): This is the percentage below PDO1 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
+ **shift_vbus_hl2** (*Optional*, percentage): This is the percentage above PDO2 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 5%.
+ **shift_vbus_ll2** (*Optional*, percentage): This is the percentage below PDO2 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
+ **shift_vbus_hl3** (*Optional*, percentage): This is the percentage above PDO3 voltage when the Over Voltage Lock Out occurs. Range is 1% to 15%. Default is 5%.
+ **shift_vbus_ll3** (*Optional*, percentage): This is the percentage below PDO3 voltage when the Under Voltage Lock Out occurs. Range is 1% to 15%. Default is 15%.
+ **vbus_disch_time_to_0v_** (*Optional*, time): Coefficient used to compute V<sub>BUS</sub> discharge time to 0V. Range is 84ms to 1260ms and must be a multiple of 84ms. Default is 756ms.
+ **vbus_disch_time_to_pdo_** (*Optional*, time): Coefficient used to compute V<sub>BUS</sub> discharge time when transitioning to lower PDO voltage. Range is 24ms to 360ms and must be a multiple of 24ms. Default is 288ms.
+ **vbus_disch_disable** (*Optional*, boolean): When set to ```true``` the STUSB4500 NVM will stop discharging V<sub>BUS</sub>. Default is ```false```.
+ **usb_comms_capable** (*Optional*, boolean): When set to ```true``` the STUSB4500 will tell the connected device that USB communications are possible. Default is ```false```.
+ **snk_uncons_power** (*Optional*, boolean): When set to ```true``` the STUSB4500 will tell the connected device external power is available. Default is ```false```.
+ **req_src_current** (*Optional*, boolean): When set to ```true``` the STUSB4500 will report the source current from source instead of the sink. Default is ```false```.
+ **power_ok_cfg** (*Optional*, enumeration): Selects the POWER_OK pins configuration. Options are ```CONFIGURATION_1```, ```CONFIGURATION_2``` or ```CONFIGURATION_3```. Default is ```CONFIGURATION_2```.
+ **gpio_cfg** (*Optional*, enumeration): Selects the GPIO configuration. Options are ```SW_CTRL_GPIO```, ```ERROR_RECOVERY```, ```DEBUG``` or ```SINK_POWER```. Default is ```ERROR_RECOVERY```.
+ **power_only_above_5v** (*Optional*, boolean): When set to ```true``` the STUSB4500 will enable V<sub>BUS</sub> only if PDO2 or PDO3 was negotiated. When ```false``` V<sub>BUS</sub> is enabled at 5V when connected to a non-PD charger. The available current is unknown. Default is ```false```.

### Sensors
+ **pd_state** (*Optional*): Current PD State as an integer. Range 0 to 3 where 0 is no PD negotiated. A value of 1, 2 or 3 indicates which PDO was negotiated. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

### Text Sensors
+ **pd_status** (*Optional*): Easy to read PD State (e.g. "12V @ 3A" ). All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).

### Actions
When the GPIO is configured for software control ```gpio_cfg: SW_CTRL_GPIO ``` these actions control the state of the output. 

+ **stusb4500.turn_gpio_on** Will pull the GPIO low.
+ **stusb4500.turn_gpio_off** Will set the output to High-Z.

Example using automations...
```
on_value_range:
  - below: 5.0
    then:
      - stusb4500.turn_gpio_on: pd_controller
```
Example in lambdas...
```
- lambda: |-
  id(pd_controller).turn_gpio_on();
```

### NVM Configuration

> [!CAUTION]
> When configuring the STUSB4500 for the first time you should realize the default configuration will allow up to 20V on V<sub>BUS</sub>. Make sure your circuit can either support 20V or configure the NVM using a Power Source that will NOT produce 20V like USB 2.0 or a non-PD USB-C charger.

Using the STUSB4500 ESPHome component requires the following steps:

1. Set the configuration variables to match your requirements. The sample configuration above sets PDO3 to 12V @ 3A, PDO2 to 9V @3A and PDO1 is disabled because ```power_only_above_5v: true```. This means it will first try to negotiate 12V @ 3A. If unavailable then it will try 9V @ 3A. If is unavailable no PD will be negotiated. Either the ```pd_state``` or ```pd_status``` sensor will let you know if there is a problem.
2. Add ```flash_nvm: true``` to the configuration.
   You should see the following messages in your log.
     ```log
     [00:10:03][C][stusb4500:112]: STUSB4500:
     [00:10:03][E][stusb4500:119]:   NVM has been flashed, power cycle the device to reload NVM
     ```
   If you forgot to add ```flash_nvm: true``` to the configuration you will see an error in the log.
     ```log
     [00:06:27][C][stusb4500:112]: STUSB4500:
     [00:06:27][E][stusb4500:114]:   NVM does not match current settings, you must set flash_nvm: true
     [00:06:27][C][stusb4500:130]:   PDO3 negotiated 9.00V @ 3.00A, 27.00W
     ```
3. Power cycle your board. Make sure ```NVM matches settings``` is present in the log and that one of your PDOs was negotiated. Configuring the STUSB4500 can be a bit finicky, but keep at it.
   ```log
   [00:00:19][C][stusb4500:112]: STUSB4500:
   [00:00:19][C][stusb4500:116]:   NVM matches settings
   [00:00:19][C][stusb4500:130]:   PDO3 negotiated 12.00V @ 3.00A, 36.00W
   ```
4. The STUSB4500 component does check that the NVM is indeed different before flashing it but it is prudent to remove the ```flash_nvm: true``` after it is clear the STUSB4500 is working as configured.


## ESPHome C4001 External Component

> [!NOTE]
> This component is in the process of being added to ESPHome. ESPHome [PR#9327](https://github.com/esphome/esphome/pull/9327).


The DFRobot C4001 (SEN0609 or SEN0610) is a millimeter-wave presence detector. The C4001 millimeter-wave presence sensor has the advantage of being able to detect both static and moving objects. It also has a relatively strong anti-interference ability, making it less susceptible to factors such as temperature changes, variations in ambient light, and environmental noise. Whether a person is sitting, sleeping, or in motion, the sensor can quickly detect their presence.

<p align="center">
    <img src="https://dfimg.dfrobot.com/enshop/SEN0609/SEN0609_Main_01.jpg" width="50%"><br />
    C4001 (SEN0609) 25m mmWave Presence Sensor
</p>

There are two variants:

+ SEN0609 has a 100° horizontal and 40° vertical field of view, 16 meter presence detection range and 25 meter motion detection range.
+ SEN0610 has a 100° horizontal and 80° vertical field of view, 8 meter presence detection range and 12 meter motion detection range.

> [!NOTE]
> Some settings have different ranges depending on the variant used. This component treats both variants the same, so it is your responsibility to make sure your configuration sets these values appropriately.

The sensor can operate in one of two modes, ```Presence``` and ```Speed and Distance```. In ```Presence``` mode the sensor provides a singular occupancy output. The presence output once presence is detected will stay on for a period that can be configured. In ```Speed and Distance``` mode the occupancy binary sensor indicates if a target is being tracked or not. Each time the sensor indicates presence it also outputs target distance, target speed and target energy. In ```Speed and Distance``` mode all of these parameter update frequently. There are only two settings for this mode, micro_motion_enable switch and threshold_factor number.

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

binary_sensor:
  - platform: dfrobot_c4001
    config_changed:
      name: Config Changed
    occupancy:
      id: occupancy_uart
      name: Occupancy via UART
      
button:
  - platform: dfrobot_c4001
    config_save:
      name: Config Save
      entity_category: CONFIG

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

switch:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    led_enable:
      name: Enable LED

text_sensor:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    software_version:
      name: Software Version
    hardware_version:
      name: Hardware Version

```

### Configuration Variables
+ **mode** (*Required*, enumeration): This sets the operation mode of the sensor. Options are ```PRESENCE``` and ```SPEED_AND_DISTANCE```.

### Buttons
+ **config_save** (*Optional*): When you click this button the current configuration will be saved. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. All Options from [Button Component](https://esphome.io/components/button/index.html#base-button-configuration).
+ **factory_reset** (*Optional*): Clicking this button will perform a factory reset of the module and all configuration values will go back to default. All Options from [Button Component](https://esphome.io/components/button/index.html#base-button-configuration).
+ **restart** (*Optional*): When you click this button the module will restart and all configuration values will remains as previously set. All Options from [Button Component](https://esphome.io/components/button/index.html#base-button-configuration).

### Binary Sensors
+ **config_changed** (*Optional*): When ```true``` the current sensor configuration has been changed but not saved to the sensor. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).
+ **occupancy** (*Optional*): In ```PRESENCE``` mode this indicates presence. In ```SPEED_AND_DISTANCE``` mode this indicates a target is being tracked. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).

### Numbers
+ **min_range** (*Optional*): This is the minimum detection range. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The manual recommends not changing this value. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **max_range** (*Optional*): This is the maximum detection range. Default is 6 meters (m) with a range of 0.6 to 25.0 m. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **trigger_range** (*Optional*): Sets the maximum range at which occupancy can switch to present. The range between max detection range and trigger detection range can NOT cause occupancy to switch to present. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **hold_sensitivity** (*Optional*): The number represents the ease in which the sensor switches to the present state when someone enters the sensing range of the sensor. Default is 7 (no units) with a range of 0 to 9, higher is more sensitive. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **trigger_sensitivity** (*Optional*): This number represents ease of continued presence detection after the sensor switched to the present state. Default is 5 (no units) with a range of 0 to 9, higher is more sensitive. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **on_latency** (*Optional*): This time value is how long presence is detected before switching to the present state. Default is 0.050 (seconds) with a range of 0.0 to 100.0. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **off_latency** (*Optional*): This time value is how long the after the sensor no longer detects presence before switching to the not present state. Default is 15 (seconds) with a range of 0 to 1500. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **inhibit_time** (*Optional*): The dead-time after switching to the not present state before presence can be detected again. Default is 1 (seconds) with a range of 0.1 to 255.0. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```PRESENCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
+ **threshold_factor** (*Optional*): The larger the number the larger the object and more motion is required to trigger the sensor to switch to target tracked state. Default is 5 with a range of 0 to 65535. The ```config_save``` button must be clicked to save the sensor configuration to flash and make operational. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

### Sensors
+ **target_distance** (*Optional*): When **occupancy** binary sensor is ```true``` this sensor indicates distance to target in meters (m). When **occupancy** binary sensor is ```false``` this sensor switches to 0.0 indicating invalid data. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/index.html#base-sensor-configuration). 
+ **target_speed** (*Optional*): When **occupancy** binary sensor is ```true``` this sensor indicates target speed in meters per second (m/s). When **occupancy** binary sensor is ```false``` this sensor switches to 0.0 indicating invalid data. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/index.html#base-sensor-configuration).
+ **target_energy** (*Optional*): When **occupancy** binary sensor is ```true``` this sensor indicates target energy in no units. When **occupancy** binary sensor is ```false``` this sensor switches to 0.0 indicating invalid data. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/index.html#base-sensor-configuration).

### Switches
+ **led_enable** (*Optional*): When turned on the green LED will flash when the sensor has been started. The blue LED cannot be disabled with this command. All Options from [Switch Component](https://esphome.io/components/switch/index.html#base-switch-configuration).
+ **micro_motion_enable** (*Optional*): Turns on micro motion mode. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Switch Component](https://esphome.io/components/switch/index.html#base-switch-configuration).

### Sensors
+ **target_distance** (*Optional*): When **occupancy** binary sensor is ```true``` this sensor indicates distance to target in meters (m). When **occupancy** binary sensor is ```false``` this sensor switches to 0.0 indicating invalid data. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/index.html#base-sensor-configuration). 
+ **target_speed** (*Optional*): When **occupancy** binary sensor is ```true``` this sensor indicates target speed in meters per second (m/s). When **occupancy** binary sensor is ```false``` this sensor switches to 0.0 indicating invalid data. Available only in ```SPEED_AND_DISTANCE``` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/index.html#base-sensor-configuration).

### Actions
+ **dfrobot_c4001.factory_reset** Will perform a factory reset of the module and all configuration values will go back to default. The module will restart with these defaults. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. This is much easier to do with a lambda that accidentally performs a factory reset every second.

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
```
- lambda: |-
  id(mmwave_sensor).factory_reset();
```

+ **dfrobot_c4001.restart** Will restart the module and all configuration values will remains as previously set.

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
```
- lambda: |-
  id(mmwave_sensor).restart();
```

## ESPHome LD2450 External Component

This custom component adds some functionality to the built-in LD2450 Component. Only the differences are listed below.

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ ld2450 ]

ld2450:
  id: ld2450_radar
  flip_x_axis: true
number:
  - platform: ld2450
    ld2450_id: ld2450_radar
    installation_angle:
      name: Installation Angle

```

### Configuration Variables
+ **flip_x_axis** (*Optional*, boolean): When the DL2450 is mounted upside down you can set this to ```true``` to flip the X axis.

### Numbers
+ **installation_angle** (*Optional*): Allows you to change the installation angle in °. Makes it easy to use when installed in a corner. Default is 0° with a range of ±45°. All Options from [Number](https://esphome.io/components/sensor/#config-number).

## SEN5X External Component


> [!NOTE]
> A Pull Request to change the sen5x component in ESPHome is in progress. ESPHome [PR#9254](https://github.com/esphome/esphome/pull/9254).


This external component adds SEN60, SEN63C, SEN65, SEN66 and SEN68 support to the built-in sen5x component. This component extends [PR #8318](https://github.com/esphome/esphome/pull/8318). Only the differences from sen5x component are listed below.

Temperature compensation is not working for the SEN6x models. Still waiting on the Sensirion Application Note.

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components
    components: [ sen5x ]

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
    set_action:
      - delay: 1s

button:
  - platform: template
    name: "Start Fan Auto Clean"
    entity_category: CONFIG
    on_press:
      - sen5x.start_fan_autoclean: sen66_sensor
  - platform: template
    name: "Activate SHT Heater"
    entity_category: CONFIG
    on_press:
      - sen5x.activate_heater: sen66_sensor
  - platform: template
    name: "CO₂ Calibrate"
    entity_category: CONFIG
    on_press:
      - sen5x.perform_forced_co2_calibration:
          value: !lambda |-
            float value = id(co2_forced_cal_value).state;
            return value;
          id: sen66_sensor

sensor:
  - platform: sen5x
    id: sen66_sensor
    i2c_id: i2c2_bus
    address: 0x6B
    update_interval: 1s
    model: SEN66
    store_baseline: true
    temperature:
      name: "Temperature"
      id: temperature
    humidity:
      name: "Humidity"
      id: humidity
    pm_1_0:
      name: "PM <1µm Weight concentration"
      id: pm_1_0
    pm_2_5:
      name: "PM <2.5µm Weight concentration"
      id: pm_2_5
    pm_4_0:
      name: "PM <4µm Weight concentration"
      id: pm_4_0
    pm_10_0:
      name: "PM <10µm Weight concentration"
      id: pm_10_0
    co2:
      name: "CO₂"
      auto_self_calibration: false
    nox:
      name: "NOx"
    voc:
      name: "VOC"
```
### Configuration Variables
+ **model** (**Optional**, string): The model of the Sensirion SEN5X or SEN6X sensor. Options are ```SEN50```, ```SEN54```, ```SEN55```, ```SEN60```, ```SEN63C```, ```SEN65```, ```SEN66``` or ```SEN68```. Use this if the model cannot be read from the sensor. There were reports of a blank model string on a SEN66 sensor.
+ **auto_cleaning_interval** (**Optional**, string): The interval in seconds of the periodic fan-cleaning. Only the SEN50, SEN55 and SEN56 models support automatic fan cleaning.

### Sensors
+ **co2** (*Optional*): The Carbon Dioxide (CO₂) level in ppm. Only the SEN63C and SEN66 models have a CO₂ sensor. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
  - **auto_self_calibration** (*Optional*, boolean): Enables automatic self-calibration (ASC) for the CO₂ sensor. Default is ```true```.
  - **altitude_compensation** (*Optional*, int): Enable compensating deviations due to current altitude (in metres). Notice: setting altitude_compensation is ignored if ambient_pressure_compensation is set.
  - **ambient_pressure_compensation_source** (*Optional*, ID): Set an external pressure sensor ID used for ambient pressure compensation. The pressure sensor must report pressure in hPa. The correction is applied before updating the state of the CO₂ sensor.
+ **hcho** (*Optional*): The Formaldehyde (HCHO) level in ppb. Only the SEN68 model has a HCHO sensor. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).


### Actions

#### sen5x.perform_forced_co2_calibration Action
This action manually calibrates the CO₂ sensor to the provided value in ppm. Let the CO₂ sensor operate normally for at least 3 minutes before performing a forced calibration.

```
on_...:
  then:
    - sen5x.perform_forced_calibration:
        # Global Monthly Mean CO₂
        # https://gml.noaa.gov/ccgg/trends/global.html
        value: 426
        id: my_sen66
```

#### sen5x.set_ambient_pressure_hpa Action
This action sets the current pressure (hPa or mbar). Check the datasheet for CO₂ pressure compensation. You can also just set the ID of your pressure sensor with ```ambient_pressure_compensation_source```. Only the SEN63C and SEN66 models have a CO₂ sensor. 

```
sensor:
  - platform: pressure_sensor
    on_value:
      then:
        - lambda: !lambda |-
            // convert pressure to hPa before sending to sen5x sensor
            id(my_sen66)->set_ambient_pressure_compensation(x / 100.0);"
```

#### sen5x.activate_heater Action
This action turns the humidity sensor's heater on for 1s at 200mW. This action only works for the SEN63C, SEN65, SEN66 and SEN68 models. Unlike the SHT4X there is no further configuration nor a way to setup automatic heating interval. I may need to add code to emulate the automatic nature built-in to the SHT4X for the heater.

```
on_value:
  then:
    - lambda: !lambda "id(my_sen66)->activate_heater();"
```
