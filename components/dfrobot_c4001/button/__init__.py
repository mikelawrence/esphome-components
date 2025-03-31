import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import (
    CONF_FACTORY_RESET,
    CONF_RESTART,
    DEVICE_CLASS_UPDATE,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_RESTART,
    ICON_RESTART_ALERT,
)

from .. import HUB_CHILD_SCHEMA, CONF_DFROBOT_C4001_HUB_ID, dfrobot_c4001_ns


CONF_CONFIG_SAVE = "config_save"

ConfigSaveButton = dfrobot_c4001_ns.class_("ConfigSaveButton", button.Button)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_CONFIG_SAVE): button.button_schema(
                ConfigSaveButton,
                device_class=DEVICE_CLASS_UPDATE,
                entity_category=ENTITY_CATEGORY_CONFIG,
                # icon=ICON_RESTART_ALERT,
            ),
        }
    )
    .extend(HUB_CHILD_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    sens0609_hub = await cg.get_variable(config[CONF_DFROBOT_C4001_HUB_ID])
    if config_save := config.get(CONF_CONFIG_SAVE):
        b = await button.new_button(config_save)
        await cg.register_parented(b, config[CONF_DFROBOT_C4001_HUB_ID])
        cg.add(sens0609_hub.set_config_save_button(b))
