# DS3231 Real Time Clock (RTC) Component for ESPHome

This is copied from [Miloit](https://github.com/miloit) so that I can use it as an external component.

``` yaml
ds3231:
  id: my_ds3231
  address: 0x68
  time_id: sntp_time
  update_interval: 1s
  temperature:
    name: "DS3231 Temperature"
  time:
    name: "DS3231 Time"
~~~