import logging

from esphome import pins
import esphome.codegen as cg
from esphome.components import esp32, esp32_rmt, output
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import CONF_FREQUENCY, CONF_ID, CONF_PIN, CONF_RMT_SYMBOLS

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@mikelawrence"]
DEPENDENCIES = ["esp32"]

rmt_pwm_ns = cg.esphome_ns.namespace("esp32_rmt_pwm")
RMTPWMOutput = rmt_pwm_ns.class_("ESP32RMTPWMOutput", output.FloatOutput, cg.Component)

CONFIG_SCHEMA = cv.All(
    esp32.only_on_variant(
        unsupported=list(esp32_rmt.VARIANTS_NO_RMT),
        msg_prefix="ESP32 RMT PWM",
    ),
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(RMTPWMOutput),
            cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_FREQUENCY, default="25kHz"): cv.frequency,
            cv.SplitDefault(
                CONF_RMT_SYMBOLS,
                esp32=192,
                esp32_c3=96,
                esp32_c5=96,
                esp32_c6=96,
                esp32_h2=96,
                esp32_p4=192,
                esp32_s2=192,
                esp32_s3=192,
            ): cv.int_range(min=48),
        }
    ).extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    # Re-enable ESP-IDF's RMT driver (excluded by default to save compile time)
    include_builtin_idf_component("esp_driver_rmt")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])

    cg.add(var.set_pin(pin))
    cg.add(var.set_frequency(int(config[CONF_FREQUENCY])))
    cg.add(var.set_rmt_symbols(config[CONF_RMT_SYMBOLS]))
