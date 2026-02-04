import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import dimmerlink_ns, DimmerLinkHub, CONF_DIMMERLINK_ID

DEPENDENCIES = ["dimmerlink"]

CONF_READY = "ready"
CONF_ERROR = "error"
CONF_CALIBRATION_DONE = "calibration_done"

DimmerLinkBinarySensor = dimmerlink_ns.class_(
    "DimmerLinkBinarySensor", cg.PollingComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DimmerLinkBinarySensor),
            cv.Required(CONF_DIMMERLINK_ID): cv.use_id(DimmerLinkHub),
            cv.Optional(CONF_READY): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
            ),
            cv.Optional(CONF_ERROR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
            cv.Optional(CONF_CALIBRATION_DONE): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DIMMERLINK_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_parent(hub))

    if CONF_READY in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_READY])
        cg.add(var.set_ready_sensor(sens))

    if CONF_ERROR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ERROR])
        cg.add(var.set_error_sensor(sens))

    if CONF_CALIBRATION_DONE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CALIBRATION_DONE])
        cg.add(var.set_calibration_done_sensor(sens))
