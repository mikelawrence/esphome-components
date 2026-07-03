import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    ENTITY_CATEGORY_CONFIG,
    ICON_PULSE,
    ICON_TIMELAPSE,
    UNIT_DECIBEL,
    UNIT_HERTZ,
    UNIT_SECOND,
)

from .. import CONF_LD2410S_ID, LD2410SComponent, ld2410s_ns

MaxDetectGateNumber = ld2410s_ns.class_("MaxDetectGateNumber", number.Number)
MinDetectGateNumber = ld2410s_ns.class_("MinDetectGateNumber", number.Number)
DelayNumber = ld2410s_ns.class_("DelayNumber", number.Number)
StatusReportingFreqNumber = ld2410s_ns.class_(
    "StatusReportingFreqNumber", number.Number
)
DistReportingFreqNumber = ld2410s_ns.class_(
    "DistReportingFreqNumber", number.Number
)

ThresholdTriggerNumber = ld2410s_ns.class_(
    "ThresholdTriggerNumber", number.Number
)
ThresholdHoldNumber = ld2410s_ns.class_(
    "ThresholdHoldNumber", number.Number
)

CONF_DISTANCE_REPORTING_FREQUENCY = "distance_reporting_frequency"
CONF_MAX_DETECT_GATE = "max_detect_gate"
CONF_MIN_DETECT_GATE = "min_detect_gate"
CONF_NO_DELAY = "no_delay"
CONF_STATUS_REPORTING_FREQUENCY = "status_reporting_frequency"
CONF_THRESHOLD_TRIGGER = "trigger_threshold"
CONF_THRESHOLD_HOLD = "hold_threshold"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
        cv.Optional(CONF_MAX_DETECT_GATE): number.number_schema(
            MaxDetectGateNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-collapse-right",
        ),
        cv.Optional(CONF_MIN_DETECT_GATE): number.number_schema(
            MinDetectGateNumber,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:arrow-collapse-left",
        ),
        cv.Optional(CONF_NO_DELAY): number.number_schema(
            DelayNumber,
            device_class=DEVICE_CLASS_DURATION,
            entity_category=ENTITY_CATEGORY_CONFIG,
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMELAPSE,
        ),
        cv.Optional(CONF_STATUS_REPORTING_FREQUENCY): number.number_schema(
            StatusReportingFreqNumber,
            device_class=DEVICE_CLASS_FREQUENCY,
            entity_category=ENTITY_CATEGORY_CONFIG,
            unit_of_measurement=UNIT_HERTZ,
            icon=ICON_PULSE,
        ),
        cv.Optional(CONF_DISTANCE_REPORTING_FREQUENCY): number.number_schema(
            DistReportingFreqNumber,
            device_class=DEVICE_CLASS_FREQUENCY,
            entity_category=ENTITY_CATEGORY_CONFIG,
            unit_of_measurement=UNIT_HERTZ,
            icon=ICON_PULSE,
        ),
    }
)

CONFIG_SCHEMA = CONFIG_SCHEMA.extend(
    {
        cv.Optional(f"g{x}"): cv.Schema(
            {
                cv.Optional(CONF_THRESHOLD_TRIGGER): number.number_schema(
                    ThresholdTriggerNumber,
                    device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    unit_of_measurement=UNIT_DECIBEL,
                    icon="mdi:pencil",
                ),
                cv.Optional(CONF_THRESHOLD_HOLD): number.number_schema(
                    ThresholdHoldNumber,
                    device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    unit_of_measurement=UNIT_DECIBEL,
                    icon="mdi:pencil",
                ),
            }
        )
        for x in range(16)
    }
)


async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if max_detect_gate_config := config.get(CONF_MAX_DETECT_GATE):
        n = await number.new_number(
            max_detect_gate_config, min_value=0, max_value=15, step=1
        )
        await cg.register_parented(n, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_max_detect_gate_number(n))
    if min_detect_gate_config := config.get(CONF_MIN_DETECT_GATE):
        n = await number.new_number(
            min_detect_gate_config, min_value=0, max_value=15, step=1
        )
        await cg.register_parented(n, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_min_detect_gate_number(n))
    if no_delay_config := config.get(CONF_NO_DELAY):
        n = await number.new_number(no_delay_config, min_value=1, max_value=120, step=1)
        await cg.register_parented(n, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_no_delay_number(n))
    if status_reporting_freq_config := config.get(CONF_STATUS_REPORTING_FREQUENCY):
        n = await number.new_number(
            status_reporting_freq_config, min_value=0.5, max_value=8, step=0.5
        )
        await cg.register_parented(n, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_status_reporting_freq_number(n))
    if distance_reporting_freq_config := config.get(CONF_DISTANCE_REPORTING_FREQUENCY):
        n = await number.new_number(
            distance_reporting_freq_config, min_value=0.5, max_value=8, step=0.5
        )
        await cg.register_parented(n, config[CONF_LD2410S_ID])
        cg.add(ld2410s_component.set_distance_reporting_freq_number(n))
    for x in range(16):
        min = 10 if x < 8 else 5
        max = 95 if x < 8 else 63
        if gate_conf := config.get(f"g{x}"):
            trigger_conf = gate_conf.get(CONF_THRESHOLD_TRIGGER)
            n = cg.new_Pvariable(trigger_conf[CONF_ID], x)
            await number.register_number(
                n, trigger_conf, min_value=min, max_value=max, step=1
            )
            await cg.register_parented(n, config[CONF_LD2410S_ID])
            cg.add(ld2410s_component.set_gate_trig_threshold_number(x, n))

            hold_conf = gate_conf.get(CONF_THRESHOLD_HOLD)
            n = cg.new_Pvariable(hold_conf[CONF_ID], x)
            await number.register_number(
                n, hold_conf, min_value=min, max_value=max, step=1
            )
            await cg.register_parented(n, config[CONF_LD2410S_ID])
            cg.add(ld2410s_component.set_gate_hold_threshold_number(x, n))
