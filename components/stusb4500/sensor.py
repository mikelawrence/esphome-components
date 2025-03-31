import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome import core
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    DEVICE_CLASS_EMPTY,
    STATE_CLASS_MEASUREMENT,
)

from . import HUB_CHILD_SCHEMA, CONF_STUSB4500_HUB_ID

CONF_PD_STATE = "pd_state"
ICON_USBC = "mdi:usb-c-port"

DEPENDENCIES = ["stusb4500"]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_PD_STATE): sensor.sensor_schema(
                icon=ICON_USBC,
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
                device_class=DEVICE_CLASS_EMPTY,
            ),
        }
    )
    .extend(HUB_CHILD_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    stusb4500_hub = await cg.get_variable(config[CONF_STUSB4500_HUB_ID])

    if pd_state_config := config.get(CONF_PD_STATE):
        sens = await sensor.new_sensor(pd_state_config)
        cg.add(stusb4500_hub.set_pd_state_sensor(sens))
