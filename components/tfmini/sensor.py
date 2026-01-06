import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor, uart
from esphome.const import (  # CONF_DIRECTION,; ICON_ACCELERATION,; ICON_RESTART,
    CONF_DISTANCE,
    CONF_ID,
    CONF_MODEL,
    CONF_SAMPLE_RATE,
    CONF_SIGNAL_STRENGTH,
    CONF_TEMPERATURE,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_TEMPERATURE,
    ICON_ARROW_EXPAND_VERTICAL,
    ICON_SIGNAL,
    ICON_THERMOMETER,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_CENTIMETER,
)

CODEOWNERS = ["@mikelawrence"]
DEPENDENCIES = ["uart"]

CONF_DIR_SAMPLE_NUM = "dir_sample_num"
CONF_CONFIG_PIN = "config_pin"
CONF_LOW_POWER = "low_power"


tfmini_ns = cg.esphome_ns.namespace("tfmini")
TFminiComponent = tfmini_ns.class_("TFminiComponent", uart.UARTDevice, cg.Component)

TFMINI_S = "TFMINI_S"
TFMINI_PLUS = "TFMINI_PLUS"
TF_LUNA = "TF_LUNA"

TFminiModel = tfmini_ns.enum("TFminiModel")

TFMINI_MODELS = {
    TFMINI_S: TFminiModel.MODEL_TFMINI_S,
    TFMINI_PLUS: TFminiModel.MODEL_TFMINI_PLUS,
    TF_LUNA: TFminiModel.MODEL_TF_LUNA,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TFminiComponent),
        cv.Required(CONF_MODEL): cv.enum(TFMINI_MODELS, upper=True),
        cv.Optional(CONF_SAMPLE_RATE, default=100): cv.int_range(min=1, max=1000),
        cv.Optional(CONF_CONFIG_PIN): cv.All(pins.internal_gpio_output_pin_schema),
        cv.Required(CONF_DISTANCE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CENTIMETER,
            icon=ICON_ARROW_EXPAND_VERTICAL,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SIGNAL_STRENGTH): sensor.sensor_schema(
            icon=ICON_SIGNAL,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon=ICON_THERMOMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_LOW_POWER): cv.boolean,
    }
).extend(uart.UART_DEVICE_SCHEMA)


def final_validate(config):
    model = config[CONF_MODEL]
    if CONF_LOW_POWER in config:
        if model == TFMINI_PLUS:
            raise cv.Invalid(f"Model {model} does not support '{CONF_LOW_POWER}'.")
        if CONF_SAMPLE_RATE not in config or config[CONF_SAMPLE_RATE] > 10:
            raise cv.Invalid("When 'low_power=true', 'sample_rate' <= 10.")
    if model in {TF_LUNA} and config[CONF_SAMPLE_RATE] > 500:
        raise cv.Invalid(f"Model {model} 'sample_rate' <= 500.")
    if CONF_CONFIG_PIN in config and model in {TFMINI_PLUS, TFMINI_S}:
        raise cv.Invalid(f"Model {model} does not support 'config_pin'.")
    schema = uart.final_validate_device_schema(
        "tfmini",
        baud_rate=115200,
        require_tx=True,
        require_rx=True,
        data_bits=8,
        parity=None,
        stop_bits=1,
    )
    schema(config)


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_model(config[CONF_MODEL]))
    if cfg := config.get(CONF_SAMPLE_RATE):
        cg.add(var.set_sample_rate(cfg))
    if cfg := config.get(CONF_CONFIG_PIN):
        cg.add(var.set_config_pin(await cg.gpio_pin_expression(cfg)))
    if cfg := config.get(CONF_LOW_POWER):
        cg.add(var.set_low_power_mode(cfg))

    cg.add(var.set_distance_sensor(await sensor.new_sensor(config[CONF_DISTANCE])))
    if cfg := config.get(CONF_TEMPERATURE):
        cg.add(var.set_temperature_sensor(await sensor.new_sensor(cfg)))
    if cfg := config.get(CONF_SIGNAL_STRENGTH):
        cg.add(var.set_signal_strength_sensor(await sensor.new_sensor(cfg)))
