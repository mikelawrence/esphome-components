import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation
from esphome.components import i2c, time, sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

# Configuration constants
CONF_TEMPERATURE = "temperature"
CONF_UPDATE_INTERVAL = "update_interval"
ds3231_ns = cg.esphome_ns.namespace("ds3231")
DS3231Component = ds3231_ns.class_("DS3231Component", time.RealTimeClock, i2c.I2CDevice)
WriteAction = ds3231_ns.class_("WriteAction", automation.Action)
ReadAction = ds3231_ns.class_("ReadAction", automation.Action)

CONFIG_SCHEMA = time.TIME_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(DS3231Component),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            accuracy_decimals=2,
            device_class="temperature",
            state_class="measurement",
        ),
        cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.update_interval,
    }
).extend(i2c.i2c_device_schema(0x68))

@automation.register_action(
    "ds3231.write_time",
    WriteAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(DS3231Component),
        }
    ),
)
async def ds3231_write_time_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var

@automation.register_action(
    "ds3231.read_time",
    ReadAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(DS3231Component),
        }
    ),
)
async def ds3231_read_time_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await time.register_time(var, config)

    # Configure temperature sensor if specified
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
    
    # Set update interval for periodic readings
    if CONF_UPDATE_INTERVAL in config:
        cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))