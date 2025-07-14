import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_MODE, DEVICE_CLASS_SWITCH, ENTITY_CATEGORY_CONFIG
import esphome.final_validate as fv

from .. import CONF_DFROBOT_C4001_ID, HUB_CHILD_SCHEMA, dfrobot_c4001_ns

DEPENDENCIES = ["dfrobot_c4001"]


CONF_LED_ENABLE = "led_enable"
CONF_MICRO_MOTION_ENABLE = "micro_motion_enable"
ICON_LED = "mdi:led-off"
ICON_MICROSCOPE = "mdi:microscope"


LedSwitch = dfrobot_c4001_ns.class_("LedSwitch", switch.Switch)
MicroMotionSwitch = dfrobot_c4001_ns.class_("MicroMotionSwitch", switch.Switch)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_LED_ENABLE): switch.switch_schema(
                LedSwitch,
                device_class=DEVICE_CLASS_SWITCH,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon=ICON_LED,
            ),
            cv.Optional(CONF_MICRO_MOTION_ENABLE): switch.switch_schema(
                MicroMotionSwitch,
                device_class=DEVICE_CLASS_SWITCH,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon=ICON_LED,
                default_restore_mode="RESTORE_DEFAULT_OFF",
            ),
        }
    )
    .extend(HUB_CHILD_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


def _final_validate(config):
    full_config = fv.full_config.get()
    hub_path = full_config.get_path_for_id(config[CONF_DFROBOT_C4001_ID])[:-1]
    hub_conf = full_config.get_config_for_path(hub_path)
    MODE = hub_conf.get(CONF_MODE)
    if MODE == "PRESENCE":
        if CONF_MICRO_MOTION_ENABLE in config:
            raise cv.Invalid(
                f"When 'mode' is set to {MODE}, {CONF_MICRO_MOTION_ENABLE} number is not allowed."
            )


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    sens0609_hub = await cg.get_variable(config[CONF_DFROBOT_C4001_ID])

    if led_enable := config.get(CONF_LED_ENABLE):
        s = await switch.new_switch(led_enable)
        await cg.register_parented(s, config[CONF_DFROBOT_C4001_ID])
        cg.add(sens0609_hub.set_led_enable_switch(s))
    if micro_motion_enable := config.get(CONF_MICRO_MOTION_ENABLE):
        s = await switch.new_switch(micro_motion_enable)
        await cg.register_parented(s, config[CONF_DFROBOT_C4001_ID])
        cg.add(sens0609_hub.set_micro_motion_enable_switch(s))
