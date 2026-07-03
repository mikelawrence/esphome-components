import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)

from .. import CONF_LD2410S_ID, LD2410SComponent, ld2410s_ns

CONF_RESPONSE_SPEED = "response_speed"

ResponseSpeedSelect = ld2410s_ns.class_("ResponseSpeedSelect", select.Select)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
    cv.Optional(CONF_RESPONSE_SPEED): select.select_schema(
        ResponseSpeedSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:speedometer",
    ),
}


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if response_speed_config := config.get(CONF_RESPONSE_SPEED):
        sel = await select.new_select(response_speed_config, options=["normal", "fast"])
        await cg.register_parented(sel, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_response_speed_select(sel))
