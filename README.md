# ESPHome-Components

The preferred method to add your own components to ESPHome is to use [External Components](https://esphome.io/components/external_components.html#external-components-git).

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/esphome/esphome
      ref: dev
    components: [ rtttl, dfplayer ]
```

## TFMini Range Finding Sensor

These ToF (time of flight) range finder sensors are compact, self-contained range finders. They support both UART and I<sub>2</sub>C communication but this component only supports UART communication.

This component supports the following Benewake LiDAR Range Finder Sensors.

TFMini Plus|TFMini-S| TFLuna
:---------:|:------:|:-----:
<img src="https://en.benewake.com/uploadfiles/2023/03/20230328182059608.png"> | <img src="https://en.benewake.com/uploadfiles/2023/03/20230328182039063.jpg"> | <img src="https://en.benewake.com/uploadfiles/2023/03/20230328182002559.jpg">

```yaml
# Sample configuration entry example
sensor:
  - platform: tfmini
    model: TFMINI_PLUS
    sample_rate: 10
    low_power: true
    version:
      id: tfmini_version
      name: "TFMini Firmware Version"
    signal_strength:
      id: tfmini_signal_strength
      name: "Signal"
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
## Configuration Variables
+ **model** (**Required**, string): The model of the Range Finder Sensor. Options are ```TFMINI_PLUS```, ```TFMINI_S``` or ```TF_LUNA```.
+ **uart_id** (*Optional*, string): Manually specify the ID of the [UART Bus](https://esphome.io/components/uart) if you use multiple UART buses.
+ **sample_rate** (*Optional*, integer): The frame rate at which the sensor will output sensor data in samples per sec. For the TFMINI_PLUS and TFMINI_S the range is 1-1000. For the TFLuna the range is 1-500. Note when low_power mode is set to true for the TFMINI_S and the TFLuna model this range is 1-10. Default is 100. 
+ **config_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#config-pin-schema)): This pin when connected will be set high to enable UART mode. (*TF_LUNA only*) 
+ **low_power** (*Optional*, boolean): Turns on low power mode. This also requires sample_rate to be 10 or less. (*TF_LUNA, TFMini-S only*) 

## Sensors
+ **distance** (*Optional*): Distance in cm. For the TFMINI_PLUS and TFMINI_S the range is 10-1200cm. For the TFLuna the range is 20-800cm. A distance of 10000cm means the sensor is not receiving enough signal, most likely open air. A distance of 0cm means the sensor is saturated and there is no measurement possible. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)
+ **signal_strength** (*Optional*): Represents the signal strength with a range of 0-65535. The longer the measurement distance, the lower signal strength will be. The lower the reflectivity is, the lower the signal strength will be. When signal strength is less than 100 detection is unreliable and distance is set to 10000cm. When signal strength is 65535 detection is unreliable and distance is set to 0cm. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)
+ **temperature** (*Optional*): Internal temperature in °C. It's not clear how useful this sensor because it certainly does not measure room temperature. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
+ **version** (*Optional*): Firmware version of Range Finder Sensor. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)

## STUSB4500 USB-C PD controller

The STUSB4500 is a USB power delivery controller that supports sink up to 100 W (20V, 5A). It has Non-Volatile Memory that can be programmed with your PDO profile so when you connect to a USB-C Power Source it will immediately negotiate your power delivery contract.

<p align="center">
    <img src="https://www.sparkfun.com/media/catalog/product/cache/a793f13fd3d678cea13d28206895ba0c/1/5/15801-SparkFun_Power_Delivery_Board_-_USB-C__Qwiic_-04.jpg" width="30%"><br />
    STUSB4500 board from SparkFun Electronics
</p>

```yaml
# Sample configuration entry example
sensor:
  - platform: tfmini
    model: TFMINI_PLUS
    sample_rate: 10
    low_power: true
    version:
      id: tfmini_version
      name: "TFMini Firmware Version"
    signal_strength:
      id: tfmini_signal_strength
      name: "Signal"
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
## Configuration Variables
+ **model** (**Required**, string): The model of the Range Finder Sensor. Options are ```TFMINI_PLUS```, ```TFMINI_S``` or ```TF_LUNA```.
+ **uart_id** (*Optional*, string): Manually specify the ID of the [UART Bus](https://esphome.io/components/uart) if you use multiple UART buses.
+ **sample_rate** (*Optional*, integer): The frame rate at which the sensor will output sensor data in samples per sec. For the TFMINI_PLUS and TFMINI_S the range is 1-1000. For the TFLuna the range is 1-500. Note when low_power mode is set to true for the TFMINI_S and the TFLuna model this range is 1-10. Default is 100. 
+ **config_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#config-pin-schema)): This pin when connected will be set high to enable UART mode. (*TF_LUNA only*) 
+ **low_power** (*Optional*, boolean): Turns on low power mode. This also requires sample_rate to be 10 or less. (*TF_LUNA, TFMini-S only*) 

## Sensors
+ **distance** (*Optional*): Distance in cm. For the TFMINI_PLUS and TFMINI_S the range is 10-1200cm. For the TFLuna the range is 20-800cm. A distance of 10000cm means the sensor is not receiving enough signal, most likely open air. A distance of 0cm means the sensor is saturated and there is no measurement possible. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)
+ **signal_strength** (*Optional*): Represents the signal strength with a range of 0-65535. The longer the measurement distance, the lower signal strength will be. The lower the reflectivity is, the lower the signal strength will be. When signal strength is less than 100 detection is unreliable and distance is set to 10000cm. When signal strength is 65535 detection is unreliable and distance is set to 0cm. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)
+ **temperature** (*Optional*): Internal temperature in °C. It's not clear how useful this sensor because it certainly does not measure room temperature. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
+ **version** (*Optional*): Firmware version of Range Finder Sensor. All Options from [Sensor](https://esphome.io/components/sensor/#config-sensor)

