import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import dimmerlink_ns, DimmerLinkHub, CONF_DIMMERLINK_ID

DEPENDENCIES = ["dimmerlink"]

CONF_CURVE = "curve"

DimmerLinkCurveSelect = dimmerlink_ns.class_(
    "DimmerLinkCurveSelect", select.Select, cg.Component
)

CURVE_OPTIONS = ["LINEAR", "RMS", "LOG"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DIMMERLINK_ID): cv.use_id(DimmerLinkHub),
        cv.Optional(CONF_CURVE): select.select_schema(
            DimmerLinkCurveSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:chart-bell-curve",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DIMMERLINK_ID])

    if CONF_CURVE in config:
        sel = await select.new_select(config[CONF_CURVE], options=CURVE_OPTIONS)
        await cg.register_component(sel, config[CONF_CURVE])
        cg.add(sel.set_parent(hub))
