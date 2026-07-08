import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_MOTION_SENSOR,
    UNIT_CENTIMETER,
    UNIT_DECIBEL,
    UNIT_PERCENT,
)

from . import CONF_LD2410S_ID, LD2410SComponent

DEPENDENCIES = ["ld2410s"]

CONF_CAL_PROGRESS = "cal_progress"
CONF_TARGET_DISTANCE = "target_distance"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
        cv.Optional(CONF_CAL_PROGRESS): sensor.sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            unit_of_measurement=UNIT_PERCENT,
            icon="mdi:percent",
        ),
        cv.Optional(CONF_TARGET_DISTANCE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DISTANCE,
            filters=[
                {
                    "timeout": {
                        "timeout": cv.TimePeriod(milliseconds=1000),
                        "value": "last",
                    }
                },
                {"throttle_with_priority": cv.TimePeriod(milliseconds=1000)},
            ],
            unit_of_measurement=UNIT_CENTIMETER,
            icon="mdi:arrow-left-right",
        ),
    }
)

CONFIG_SCHEMA = CONFIG_SCHEMA.extend(
    {
        cv.Optional(f"g{x}_energy"): sensor.sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            filters=[
                {
                    "timeout": {
                        "timeout": cv.TimePeriod(milliseconds=1000),
                        "value": "last",
                    }
                },
                {"throttle_with_priority": cv.TimePeriod(milliseconds=1000)},
            ],
            icon=ICON_MOTION_SENSOR,
            unit_of_measurement=UNIT_DECIBEL,
        )
        for x in range(16)
    }
)


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if cal_progress_config := config.get(CONF_CAL_PROGRESS):
        sens = await sensor.new_sensor(cal_progress_config)
        cg.add(ld2410s_component.set_cal_progress_sensor(sens))
    if target_distance_config := config.get(CONF_TARGET_DISTANCE):
        sens = await sensor.new_sensor(target_distance_config)
        cg.add(ld2410s_component.set_target_distance_sensor(sens))
    for x in range(16):
        if energy_config := config.get(f"g{x}_energy"):
            sens = await sensor.new_sensor(energy_config)
            cg.add(ld2410s_component.set_gate_energy_sensor(x, sens))
