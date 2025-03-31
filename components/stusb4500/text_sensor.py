import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome import core
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import HUB_CHILD_SCHEMA, CONF_STUSB4500_HUB_ID

CONF_PD_STATUS = "pd_status"
ICON_USBC = "mdi:usb-c-port"

DEPENDENCIES = ["stusb4500"]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_PD_STATUS): text_sensor.text_sensor_schema(
                icon=ICON_USBC,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            )
        }
    )
    .extend(HUB_CHILD_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    stusb4500_hub = await cg.get_variable(config[CONF_STUSB4500_HUB_ID])

    if pd_status_config := config.get(CONF_PD_STATUS):
        sens = await text_sensor.new_text_sensor(pd_status_config)
        cg.add(stusb4500_hub.set_pd_status_text_sensor(sens))
