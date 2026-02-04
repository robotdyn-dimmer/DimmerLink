import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID, CONF_GAMMA_CORRECT, CONF_DEFAULT_TRANSITION_LENGTH

from . import dimmerlink_ns, DimmerLinkHub, CONF_DIMMERLINK_ID

DEPENDENCIES = ["dimmerlink"]

DimmerLinkLight = dimmerlink_ns.class_("DimmerLinkLight", light.LightOutput)

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(DimmerLinkLight),
        cv.Required(CONF_DIMMERLINK_ID): cv.use_id(DimmerLinkHub),
        cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
        cv.Optional(
            CONF_DEFAULT_TRANSITION_LENGTH, default="1s"
        ): cv.positive_time_period_milliseconds,
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DIMMERLINK_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    cg.add(var.set_parent(hub))
