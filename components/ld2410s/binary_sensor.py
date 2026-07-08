import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_HAS_TARGET,
    CONF_ID,
    DEVICE_CLASS_OCCUPANCY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_ACCOUNT,
)

from . import CONF_LD2410S_ID, LD2410SComponent

DEPENDENCIES = ["ld2410s"]

CONF_CAL_RUNNING = "cal_running"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
    cv.Optional(CONF_CAL_RUNNING): binary_sensor.binary_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:exclamation",
    ),
    cv.Optional(CONF_HAS_TARGET): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_OCCUPANCY,
        filters=[{"settle": cv.TimePeriod(milliseconds=1000)}],
        icon=ICON_ACCOUNT,
    ),
}


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if has_target_config := config.get(CONF_HAS_TARGET):
        sens = await binary_sensor.new_binary_sensor(has_target_config)
        cg.add(ld2410s_component.set_has_target_binary_sensor(sens))
    if cal_running_config := config.get(CONF_CAL_RUNNING):
        sens = await binary_sensor.new_binary_sensor(cal_running_config)
        cg.add(ld2410s_component.set_cal_running_binary_sensor(sens))
