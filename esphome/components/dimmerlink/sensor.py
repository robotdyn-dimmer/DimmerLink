import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_HERTZ,
    UNIT_PERCENT,
    UNIT_EMPTY,
    DEVICE_CLASS_FREQUENCY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_CHIP,
)

from . import dimmerlink_ns, DimmerLinkHub, CONF_DIMMERLINK_ID

DEPENDENCIES = ["dimmerlink"]

CONF_AC_FREQUENCY = "ac_frequency"
CONF_LEVEL = "level"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_AC_PERIOD = "ac_period"

ICON_SINE_WAVE = "mdi:sine-wave"
ICON_BRIGHTNESS = "mdi:brightness-percent"
ICON_TIMER = "mdi:timer-outline"

DimmerLinkSensor = dimmerlink_ns.class_(
    "DimmerLinkSensor", cg.PollingComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DimmerLinkSensor),
            cv.Required(CONF_DIMMERLINK_ID): cv.use_id(DimmerLinkHub),
            cv.Optional(CONF_AC_FREQUENCY): sensor.sensor_schema(
                unit_of_measurement=UNIT_HERTZ,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_FREQUENCY,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_SINE_WAVE,
            ),
            cv.Optional(CONF_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS,
            ),
            cv.Optional(CONF_FIRMWARE_VERSION): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
                icon=ICON_CHIP,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_AC_PERIOD): sensor.sensor_schema(
                unit_of_measurement="μs",
                accuracy_decimals=0,
                icon=ICON_TIMER,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DIMMERLINK_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_parent(hub))

    if CONF_AC_FREQUENCY in config:
        sens = await sensor.new_sensor(config[CONF_AC_FREQUENCY])
        cg.add(var.set_ac_frequency_sensor(sens))

    if CONF_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_LEVEL])
        cg.add(var.set_level_sensor(sens))

    if CONF_FIRMWARE_VERSION in config:
        sens = await sensor.new_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(var.set_firmware_version_sensor(sens))

    if CONF_AC_PERIOD in config:
        sens = await sensor.new_sensor(config[CONF_AC_PERIOD])
        cg.add(var.set_ac_period_sensor(sens))
