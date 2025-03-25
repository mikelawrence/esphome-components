import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome import core
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import CONF_STUSB4500_ID, STUSB4500Component

CONF_PD_STATUS = "pd_status"
ICON_USBC = "mdi:usb-c-port"

DEPENDENCIES = ["stusb4500"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_STUSB4500_ID): cv.use_id(STUSB4500Component),
        cv.Optional(CONF_PD_STATUS): text_sensor.text_sensor_schema(
            icon=ICON_USBC,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    stusb4500_component = await cg.get_variable(config[CONF_STUSB4500_ID])

    if pd_status_config := config.get(CONF_PD_STATUS):
        sens = await text_sensor.new_text_sensor(pd_status_config)
        cg.add(stusb4500_component.set_pd_status_text_sensor(sens))
