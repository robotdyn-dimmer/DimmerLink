import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import dimmerlink_ns, DimmerLinkHub, CONF_DIMMERLINK_ID

DEPENDENCIES = ["dimmerlink"]

CONF_RESET = "reset"
CONF_RECALIBRATE = "recalibrate"

DimmerLinkResetButton = dimmerlink_ns.class_(
    "DimmerLinkResetButton", button.Button
)
DimmerLinkRecalibrateButton = dimmerlink_ns.class_(
    "DimmerLinkRecalibrateButton", button.Button
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DIMMERLINK_ID): cv.use_id(DimmerLinkHub),
        cv.Optional(CONF_RESET): button.button_schema(
            DimmerLinkResetButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:restart",
        ),
        cv.Optional(CONF_RECALIBRATE): button.button_schema(
            DimmerLinkRecalibrateButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:refresh",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DIMMERLINK_ID])

    if CONF_RESET in config:
        btn = await button.new_button(config[CONF_RESET])
        cg.add(btn.set_parent(hub))

    if CONF_RECALIBRATE in config:
        btn = await button.new_button(config[CONF_RECALIBRATE])
        cg.add(btn.set_parent(hub))
