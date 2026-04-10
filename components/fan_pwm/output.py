import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_FREQUENCY, CONF_ID, CONF_PIN

AUTO_LOAD = ["output"]

fan_pwm_ns = cg.esphome_ns.namespace("fan_pwm")
FanPWMOutput = fan_pwm_ns.class_("FanPWMOutput", output.FloatOutput, cg.Component)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(FanPWMOutput),
        cv.Required(CONF_PIN): cv.int_range(min=0, max=21),
        cv.Optional(CONF_FREQUENCY, default=25000): cv.int_range(min=100, max=500000),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)

    cg.add(var.set_pin(config[CONF_PIN]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
