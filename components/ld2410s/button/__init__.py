import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_UPDATE,
    ENTITY_CATEGORY_CONFIG,
)

from .. import CONF_LD2410S_ID, LD2410SComponent, ld2410s_ns

CONF_CAL_START = "cal_start"

CalibrationStartButton = ld2410s_ns.class_("CalibrationStartButton", button.Button)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
    cv.Optional(CONF_CAL_START): button.button_schema(
        CalibrationStartButton,
        device_class=DEVICE_CLASS_UPDATE,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:refresh-auto",
    ),
}


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])

    if cal_start := config.get(CONF_CAL_START):
        b = await button.new_button(cal_start)
        await cg.register_parented(b, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_cal_start_button(b))
