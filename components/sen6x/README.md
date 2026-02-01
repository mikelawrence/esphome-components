# sen6x Component

The `sen6x` sensor platform allows you to use Sensirion
[SEN6X Series](https://sensirion.com/sen6x-air-quality-sensor-platform)
Environmental sensors with ESPHome.

The [I²C Bus](https://esphome.io/components/i2c) is required in your configuration for this sensor to work.

<img src="https://sensirion.com/_next/image?url=https%3A%2F%2Fsensirion.com%2Fmedia%2Fportfolio%2Fseries%2Fimage%2Fa3c23126-f3f9-47a8-98e0-2b8dbda727a5.webp&w=1920&q=75">

```yaml
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

## Configuration variables

- **type** (*Required*, enum): The type of the connected sensor. Must be one of the following:
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C.

- **temperature_compensation** (*Optional*, sequence): These parameters allow the user to compensate temperature
  effects of the customer design by applying custom temperature offsets to the ambient temperature. Only available
  with SEN62, SEN63C, SEN65, SEN66, SEN69 or SEN69C.
  See [Temperature Compensation and Acceleration](#temperature-and-acceleration-compensation) section below for more information.

  - **offset** (*Optional*, float): Temperature offset, in °C. Defaults to `0`.
  - **normalized_offset_slope** (*Optional*, float): Normalized temperature offset slope. Defaults to `0`.
  - **time_constant** (*Optional*, positive int): Time constant in seconds. Defaults to `0`.

- **temperature_acceleration** (*Optional*): This command allows user to set custom temperature acceleration
  parameters. Only available with SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C.

  - **k** (*Optional*): Filter constant K.
  - **p** (*Optional*): Filter constant P.
  - **t1** (*Optional*): Time constant T1 in seconds.
  - **t2** (*Optional*): Time constant T2 in seconds.

  This is a good starting point for these values.

  | Acceleration Level     |   T1    |   T1    |   K    |   P    |
  | -----------------------|:-------:|:-------:|:------:|:------:|
  | Light (IAQM)           |   100   |   300   |   20   |   20   |
  | Middle                 |   100   |   300   |   50   |   20   |
  | Strong (Air Purifier)  |   250   |   300   |  150   |   20   |

  For more information see
  [Temperature Compensation and Acceleration](#temperature-and-acceleration-compensation) section below.

- **pm_1_0** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  1μm in size. Readings in µg/m³.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **pm_2_5** (*Optional*): The information for the **Mass Concentration** sensor for fine particles up to
  2.5μm in size. Readings in µg/m³.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **pm_4_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  4μm in size. Readings in µg/m³.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **pm_10_0** (*Optional*): The information for the **Mass Concentration** sensor for coarse particles up to
  10μm in size. Readings in µg/m³.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **temperature** (*Optional*): The information for the Temperature sensor. Only available with SEN62, SEN63C, 
  SEN65, SEN66, SEN68 or SEN69C.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **humidity** (*Optional*): The information for the Relative Humidity sensor. Only available with SEN54, SEN55,
  SEN62, SEN63C, SEN65, SEN66, SEN68 or SEN69C.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **co2** (*Optional*): The information for the Carbon dioxide (CO₂) sensor. Readings in ppm. Only available with
  SEN63C, SEN66 or SEN69C.

  - **auto_self_calibration** (*Optional*, boolean): True enables automatic CO₂ self calibration.
  False disables automatic CO₂ calibration. Default is `true`.
  - **altitude_compensation** (*Optional*, integer): When set to altitude (in meters), the CO₂ sensor will be
  statically compensated for deviations due to current altitude. See [CO₂ Compensation](#co-compensation) section below
  for more information.
  - **ambient_pressure_compensation_source** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#config-id)):
  Sets an external pressure sensor ID (must report in hPA). This will compensate the CO₂ sensor for deviations
  due to current pressure. This correction is applied with each update of the CO₂ sensor. See
  [CO₂ Compensation](#co-compensation) section below for more information.
  - All options from [Sensor](https://esphome.io/components/sensor).

- **voc** (*Optional*): The information for the VOC Index sensor. Only available with SEN65, SEN66, SEN69 or SEN69C.

  - **store_algorithm_state** (*Optional*, boolean): When set to `true` the VOC algorithm state is saved to flash every
    2 hours. During setup of the sensor the previously saved algorithm state is loaded and the VOC sensor will
    skip the initial learning phase.
    Only available with SEN65, SEN66, SEN68 or SEN69C.
  - **algorithm_tuning** (*Optional*): The VOC algorithm can be customized by tuning 6 different parameters.
    For more details see
    [Engineering Guidelines for SEN5x](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf).

    - **index_offset** (*Optional*): VOC index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    - **learning_time_offset_hours** (*Optional*): Time constant to estimate the VOC algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    - **learning_time_gain_hours** (*Optional*): Time constant to estimate the VOC algorithm gain from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hours.
    - **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high VOC index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    - **std_initial** (*Optional*): Initial estimate for standard deviation. Lower value boosts events during
      initial learning period, but may result in larger device-to-device variations.
      Allowed values are in range 10..5000. The default value is 50.
    - **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the VOC index output.
      Allowed values are in range 1..1000. The default value is 230.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **nox** (*Optional*): NOx Index. Only available with SEN65, SEN66, SEN69 or SEN69C.

  - **algorithm_tuning** (*Optional*): Like VOC the NOx algorithm can be customized by tuning 5 different parameters.
    For more details see
    [Engineering Guidelines for SEN5x](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf).

    - **index_offset** (*Optional*): NOx index representing typical (average) conditions.
      Allowed values are in range 1..250. The default value is 100.
    - **learning_time_offset_hours** (*Optional*): Time constant to estimate the NOx algorithm offset from the
      history in hours. Past events will be forgotten after about twice the learning time.
      Allowed values are in range 1..1000. The default value is 12 hour.
    - **learning_time_gain_hours** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    - **gating_max_duration_minutes** (*Optional*): Maximum duration of gating in minutes (freeze of estimator
      during high NOx index signal). Zero disables the gating. Allowed values are in range 0..3000.
      The default value is 180 minutes.
    - **std_initial** (*Optional*): Here for completeness, but this is ignored by NOx Algorithm Tuning.
    - **gain_factor** (*Optional*): Gain factor to amplify or to attenuate the NOx index output.
      Allowed values are in range 1..1000. The default value is 230.

  - All options from [Sensor](https://esphome.io/components/sensor).

- **hcho** (*Optional*): The information for the Formaldehyde (HCHO) sensor. Readings in ppb. Only available with
  SEN68 or SEN69C.
  
  - All options from [Sensor](https://esphome.io/components/sensor).

- **address** (*Optional*, int): Manually specify the I²C address of the sensor. Defaults to `0x6B`.

> [!NOTE]
> This component reports readings as soon as they are available without regard initial accuracy.
> Your configuration should limit reporting of sensor values for a period of time after power-up.
> A good starting point is 5 minutes.
>
> - The PM sensor has a start-up time of 30 seconds.
> - The temperature sensor has a response time of 1 minute with no mention start-up time.
> - The humidity sensor has a response time of 20 seconds with no mention start-up time.
> - The VOC sensor will start detecting events in 1 minute but may take up to 1 hour to meet
>   data sheet specifications.
> - The NOx sensor will start detecting events in 5 minutes but may take up to 6 hours to meet
>   data sheet specifications.
> - The CO₂ sensor has a response time of between 60 and 70 seconds with no mention start-up time.
> - The HCHO sensor has a start-up time of 10 minutes.

## Actions

Several [actions](https://esphome.io/automations/actions/) are available with this component. All are
mutually exclusive and take time to complete. While an action is running the sensor is otherwise occupied
and cannot be accessed. Attempting to run another action while another is already running results in a
'Sensor is busy' log warning. Some actions will stop measurements before executing. All actions are also
available as Lambda calls.

### Fan Cleaning

This sensor supports manual running of the fan cleaning cycle by using `start_fan_cleaning`. During the fan
cleaning process sensor measurements are stopped. The entire fan cleaning sequence takes 12 seconds.

Note: The Lambda call returns false when the sensor is busy.

#### `start_fan_cleaning`

```yaml
button:
  - platform: template
    name: "Start Fan Cleaning Action"
    entity_category: CONFIG
    on_press: # Action
      sen6x.start_fan_cleaning: sen66_sensor
  - platform: template
    name: "Start Fan Cleaning Template"
    entity_category: CONFIG
    on_press: # Lambda call 
      lambda: !lambda |-
        id(sen66_sensor).start_fan_cleaning();
sensor:
  - platform: sen6x
    type: SEN66
    id: sen66_sensor
    humidity:
      humidity: "Humidity"
 ```

You can emulate the SEN5X automatic fan cleaning on this sensor by calling `start_fan_cleaning` periodically.

For example, to clean the fan every 7 days while the device is on, as recommended by the manufacturer, the
following configuration can be added:

```yaml
sensor:
  - platform: sen6x
    type: SEN66
    id: sen66_sensor
    pm_1_0:
      name: "PM <1µm Mass concentration"
interval:
  - interval: 7d
    then: # Action
      - sen6x.start_fan_cleaning: sen66_sensor
  - interval: 7d
    then: # Lambda call
      lambda: !lambda |-
        id(sen66_sensor).start_fan_cleaning();
```

You should use only one of the intervals in the above yaml.

### Humidity Sensor Heater

The SEN6X humidity sensor can develop an offset in the humidity reading when exposed to high levels of
humidity for extended periods of time. It supports a heater similar to the one in the SHT4X. The
difference is no automatic mode. Instead you have to call `activate_heater` occasionally.

#### `activate_heater`

This action manually starts the heater. First measurements are stopped, then the heater is turned on
at 200mW for 1s, finally there is a 20 second delay before reenabling the measurements. This is to
ensure the heating effects are gone before temperature measurements resume. The entire activate
heater sequence takes 22 seconds.

```yaml
button:
  - platform: template
    name: "Activate Heater Action"
    entity_category: CONFIG
    on_press:
      sen6x.activate_heater: sen66_sensor
  - platform: template
    name: "Activate Heater Template"
    entity_category: CONFIG
    on_press:
      lambda: !lambda |-
        id(sen66_sensor).activate_heater();
sensor:
  - platform: sen6x
    type: SEN66
    id: sen66_sensor
    humidity:
      humidity: "Humidity"
```

### CO₂ Re-calibration

The CO₂ sensor by default has auto-calibration enabled. Auto-calibration will adjust the minimum measurement
over the last week or so to the outdoor average of slightly more than 400 ppm. Auto-calibration assumes that
you are opening the windows at least once a week. If you don't open the windows then over time the CO₂ level
will tend downward.

If you know your minimums are not going to be 400 ppm then you can disable auto-calibration and occasionally
take the sensor outside for 5 minutes and then force a manual CO₂ calibration to the expected outdoor CO₂ level.
Be sure to watch the log output of your device when you perform this action. If the sensor reports an error
during the recalibration process it will be reported in the log.

#### `perform_forced_co2_recalibration`

This [action](https://esphome.io/automations/actions/) forces a manual calibration on the CO₂ sensor. Measurements
are stopped before issuing the forced co2 recalibrate command to the sensor. The entire perform forced co2
recalibration action takes 2 seconds.

The example below will recalibrate the CO₂ sensor when either button is pressed using the "CO₂ Calibration Value"
number's current value.

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
    name: "CO₂ Calibrate Action"
    entity_category: CONFIG
    on_press:
      sen6x.perform_forced_co2_recalibration:
        value: !lambda "return id(co2_forced_cal_value).state;"
        id: sen66_sensor
  - platform: template
    name: "CO₂ Calibrate Template"
    entity_category: CONFIG
    on_press:
      lambda: !lambda |-
        id(sen66_sensor).perform_forced_co2_recalibration(id(co2_forced_cal_value).state);
sensor:
  - platform: sen6x
    type: SEN66
    id: sen66_sensor
    co2:
      name: "CO₂"
```

### CO₂ Compensation

The CO₂ sensor supports pressure/altitude compensation to improve CO₂ accuracy. If a pressure sensor is available
you can dynamically adjust pressure compensation by either adding `ambient_pressure_compensation_source` to your
configuration for automatic updates or you can periodically call the `sen6x.set_ambient_pressure_compensation`
action with the current ambient pressure. You can also statically define `altitude_compensation`.

#### Dynamic example with a local sensor

```yaml
sensor:
  - platform: bmp581
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        lambda: |-
          // convert Pa to hPa (or mBar)
          return x * 0.01;
  - platform: sen6x
    type: SEN69C
    co2:
      name: "CO₂"
      ambient_pressure_compensation_source: pressure_hpa
```

#### Dynamic example `set_ambient_pressure_compensation`

This [action](https://esphome.io/automations/actions/) updates the current pressure used in CO₂ pressure compensation.
Must be in hPa or mBar. Note: Once `set_ambient_pressure_compensation` is called `altitude_compensation`, if
set in the configuration, will be ignored. Only available with SEN63C, SEN66 or SEN69C.

```yaml
sensor:
  - platform: bmp581_i2c
    id: bmp581_sensor
    pressure:
      id: pressure_hpa
      filters:
        lambda: |-
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

#### Static example with altitude

```yaml
sensor:
  - platform: sen6x
    type: SEN66
    co2:
      name: "CO₂"
      altitude_compensation: 427m
```

## Temperature and Acceleration Compensation

The SEN6X sensors contain an internal temperature compensation mechanism. The compensated ambient temperature
is calculated as follows:

``` c++
T_Ambient_Compensated = T_Ambient + (slope*T_Ambient) + offset
```

Where slope and offset are the values set with `temperature_compensation` configuration variables, smoothed
with the specified time constant, also a `temperature_compensation` configuration variable. The time constant
is how fast the slope and offset are applied. After the specified value in seconds, 63% of the new slope and
offset are applied. Temperature Compensation features of up to five slots. These five slots contribute additively
to temperature compensation and allow the user to compensate for devices that can be turned on/off like a WiFi
module or a cooling fan.

More details about the tuning of these parameters for SEN6X sensors are included in the application note:
[SEN6x – Temperature Acceleration and Compensation Instructions](https://sensirion.com/media/documents/C964FCC8/69709EC3/PS_AN_SEN6x_Temperature_Compensation_and_Acceleration_Application_No.pdf).

## NOx and VOC Algorithm Tuning

Both the NOx and VOC sensor support algorithm tuning. These variables are set with the `algorithm_tuning`
configuration under the `voc` and `nox` sensors. For more details see
[Engineering Guidelines for SEN5X](https://sensirion.com/media/documents/25AB572C/62B463AA/Sensirion_Engineering_Guidelines_SEN5x.pdf)
