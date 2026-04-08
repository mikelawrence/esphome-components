import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, output
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_RED,
    CONF_GREEN,
    CONF_BLUE,
    CONF_COLD_WHITE,
    CONF_WARM_WHITE,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_CONSTANT_BRIGHTNESS,
    CONF_COLOR_INTERLOCK,
)

CONF_MAX_POWER = "max_power"
CONF_WEIGHT_RED = "weight_red"
CONF_WEIGHT_GREEN = "weight_green"
CONF_WEIGHT_BLUE = "weight_blue"
CONF_WEIGHT_COLD_WHITE = "weight_cold_white"
CONF_WEIGHT_WARM_WHITE = "weight_warm_white"

rgbww_power_limited_ns = cg.esphome_ns.namespace("rgbww_power_limited")
RGBWWPowerLimitedLight = rgbww_power_limited_ns.class_(
    "RGBWWPowerLimitedLight", light.LightOutput, cg.Component
)

CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(RGBWWPowerLimitedLight),
            cv.Required(CONF_RED): cv.use_id(output.FloatOutput),
            cv.Required(CONF_GREEN): cv.use_id(output.FloatOutput),
            cv.Required(CONF_BLUE): cv.use_id(output.FloatOutput),
            cv.Required(CONF_COLD_WHITE): cv.use_id(output.FloatOutput),
            cv.Required(CONF_WARM_WHITE): cv.use_id(output.FloatOutput),
            cv.Required(CONF_COLD_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Required(CONF_WARM_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
            cv.Optional(CONF_COLOR_INTERLOCK, default=False): cv.boolean,
            cv.Optional(CONF_MAX_POWER, default=1.0): cv.float_range(min=0.01, max=1.0),
            cv.Optional(CONF_WEIGHT_RED, default=1.0): cv.positive_float,
            cv.Optional(CONF_WEIGHT_GREEN, default=1.0): cv.positive_float,
            cv.Optional(CONF_WEIGHT_BLUE, default=1.0): cv.positive_float,
            cv.Optional(CONF_WEIGHT_COLD_WHITE, default=1.0): cv.positive_float,
            cv.Optional(CONF_WEIGHT_WARM_WHITE, default=1.0): cv.positive_float,
        }
    ).extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    for conf_key, setter in [
        (CONF_RED, "set_red"),
        (CONF_GREEN, "set_green"),
        (CONF_BLUE, "set_blue"),
        (CONF_COLD_WHITE, "set_cold_white"),
        (CONF_WARM_WHITE, "set_warm_white"),
    ]:
        out = await cg.get_variable(config[conf_key])
        cg.add(getattr(var, setter)(out))

    cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
    cg.add(var.set_color_interlock(config[CONF_COLOR_INTERLOCK]))
    cg.add(var.set_max_power(config[CONF_MAX_POWER]))
    cg.add(var.set_weight_red(config[CONF_WEIGHT_RED]))
    cg.add(var.set_weight_green(config[CONF_WEIGHT_GREEN]))
    cg.add(var.set_weight_blue(config[CONF_WEIGHT_BLUE]))
    cg.add(var.set_weight_cold_white(config[CONF_WEIGHT_COLD_WHITE]))
    cg.add(var.set_weight_warm_white(config[CONF_WEIGHT_WARM_WHITE]))
