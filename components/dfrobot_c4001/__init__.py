from esphome import automation
from esphome.automation import maybe_simple_id
import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODE

DEPENDENCIES = ["uart"]

CODEOWNERS = ["@mikelawrence"]
MULTI_CONF = True

CONF_DFROBOT_C4001_ID = "dfrobot_c4001_id"
MODE = ""

dfrobot_c4001_ns = cg.esphome_ns.namespace("dfrobot_c4001")
DFRobotC4001Hub = dfrobot_c4001_ns.class_(
    "DFRobotC4001Hub", cg.Component, uart.UARTDevice
)
ModeConfig = dfrobot_c4001_ns.enum("ModeConfig")

CONF_MODE_SELECTS = {
    "PRESENCE": ModeConfig.MODE_PRESENCE,
    "SPEED_AND_DISTANCE": ModeConfig.MODE_SPEED_AND_DISTANCE,
}


# Actions
DFRobotC4001FactoryResetAction = dfrobot_c4001_ns.class_(
    "DFRobotC4001FactoryResetAction", automation.Action
)

DFRobotC4001RestartAction = dfrobot_c4001_ns.class_(
    "DFRobotC4001RestartAction", automation.Action
)

HUB_CHILD_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DFROBOT_C4001_ID): cv.use_id(DFRobotC4001Hub),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DFRobotC4001Hub),
            cv.Required(CONF_MODE): cv.enum(CONF_MODE_SELECTS, upper=True, space="_"),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "dfrobot_c4001",
    require_tx=True,
    require_rx=True,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_MODE in config:
        cg.add(var.set_mode(config[CONF_MODE]))


@automation.register_action(
    "dfrobot_c4001.factory_reset",
    DFRobotC4001FactoryResetAction,
    maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(DFRobotC4001Hub),
        }
    ),
)
async def dfrobot_c4001_factory_reset_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "dfrobot_c4001.restart",
    DFRobotC4001RestartAction,
    maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(DFRobotC4001Hub),
        }
    ),
)
async def dfrobot_c4001_restart_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
