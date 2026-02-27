# DS3231 Real Time Clock (RTC) Component for ESPHome

This is copied from [Nerdiyde](https://github.com/Nerdiyde/DS3231-RTC-component-for-ESPHome) so that I can use it as an external component.

This repository provides a custom ESPHome component for the DS3231 Real Time Clock (RTC) module, including support for its integrated temperature sensor. The DS3231 is a highly accurate I2C RTC with an integrated temperature-compensated crystal oscillator (TCXO) and crystal. It is widely used in embedded and IoT projects for precise timekeeping, even during power outages (with a backup battery).

## Features

- Accurate real-time clock (RTC) integration for ESPHome
- I2C communication
- Automatic time synchronization with ESPHome
- Exposes the DS3231's built-in temperature sensor as a standard ESPHome sensor
- Actions to manually read from or write to the RTC via ESPHome automations

## What is the DS3231?

The DS3231 is a low-cost, extremely accurate I2C real-time clock (RTC) with an integrated temperature-compensated crystal oscillator (TCXO) and crystal. The device incorporates a battery input and maintains accurate timekeeping when main power to the device is interrupted. It also features a built-in digital temperature sensor, which can be read out via I2C.

## Installation

1. **Copy the `ds3231` directory** into your ESPHome project's custom components folder (usually inside `esphome/` or your project root).
2. Make sure your ESPHome device is connected to the DS3231 module via I2C (SDA/SCL).
3. Add the custom component to your ESPHome YAML configuration as shown below.

## Example ESPHome Configuration

```yaml
esphome:
  name: my_ds3231_clock
  platform: ESP8266  # or ESP32
  board: nodemcuv2   # adjust to your board

# Use external component from GitHub
external_components:
  - source: github://your-github-username/DS3231-RTC-component-for-ESPHome
    components: [ ds3231 ]

# Enable I2C bus
i2c:
  sda: GPIO4   # D2 on NodeMCU
  scl: GPIO5   # D1 on NodeMCU
  scan: true
  frequency: 400kHz

# Time component using DS3231
time:
  - platform: ds3231
    id: ds3231_time
    address: 0x68  # Default I2C address
    update_interval: 60s  # How often to read from RTC (default: 60s)
    timezone: Europe/Berlin  # Set your timezone
    
    # Optional: Expose the built-in temperature sensor
    temperature:
      name: "DS3231 Temperature"
      accuracy_decimals: 2

# Optional: Add buttons for manual synchronization
button:
  - platform: template
    name: "Write Time to RTC"
    on_press:
      - ds3231.write_time: ds3231_time
  
  - platform: template
    name: "Read Time from RTC"
    on_press:
      - ds3231.read_time: ds3231_time

# Optional: Automation example - sync RTC on boot
on_boot:
  - priority: -100  # Run after WiFi/time sync
    then:
      - delay: 5s
      - ds3231.write_time: ds3231_time
      - logger.log: "System time written to DS3231 RTC"
```

## Usage

### Time Synchronization
- The component periodically reads the time from the RTC (default: every 60 seconds)
- On boot, the ESP first synchronizes with NTP (if WiFi is available), then you can write that time to the RTC
- If the ESP loses power, it will read the time from the RTC on next boot to restore accurate timekeeping

### Temperature Sensor
- The DS3231 has a built-in temperature sensor (primarily for crystal compensation)
- It will be available as a standard ESPHome sensor entity if configured
- Temperature range: -40°C to +85°C, accuracy: ±3°C
- Note: This sensor reflects the module's internal temperature, not necessarily ambient temperature

### Actions
- `ds3231.write_time`: Writes the current ESP system time to the RTC (useful after NTP sync)
- `ds3231.read_time`: Reads time from the RTC and synchronizes the ESP system clock (happens automatically)

### Update Interval
- Configure `update_interval` to control how often the component reads from the RTC
- Default is 60 seconds - reduce for more frequent updates or increase to save I2C bus bandwidth
- Each update also reads the temperature sensor if configured

## Notes

- The default I2C address for the DS3231 is `0x68`. If your module uses a different address, adjust the configuration accordingly.
- Make sure to connect a backup battery to the DS3231 to retain time during power loss.
- The temperature sensor is primarily intended for internal compensation and may not reflect ambient temperature accurately, but it can be useful for monitoring the module's environment.

## Troubleshooting

- If the component fails to communicate, check your I2C wiring and address.
- Use the ESPHome logs to see detailed error messages from the component.

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## Credits

- Based on the ESPHome external component system.
- DS3231 datasheet: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf