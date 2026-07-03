import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_CHIP,
)

from . import CONF_LD2410S_ID, LD2410SComponent

DEPENDENCIES = ["ld2410s"]

CONF_ENERGY_VALUES = "energy_values"
CONF_FW_VERSION = "fw_version"
CONF_THRESHOLD_TRIGGERS = "gate_trig_threshold"
CONF_THRESHOLD_HOLDS = "hold_threshold"
CONF_THRESHOLD_SNRS = "threshold_snrs"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
    cv.Optional(CONF_FW_VERSION): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=ICON_CHIP,
    ),
}


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if fw_version_config := config.get(CONF_FW_VERSION):
        sens = await text_sensor.new_text_sensor(fw_version_config)
        cg.add(ld2410s_component.set_fw_version_text_sensor(sens))
