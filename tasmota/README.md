# Tasmota Integration

DimmerLink integrates with [Tasmota](https://tasmota.github.io/) firmware for ESP32, providing a complete AC dimmer solution with web dashboard, MQTT telemetry, HTTP API, and Home Assistant integration.

## Available Drivers

| Driver | Platform | Description |
|--------|----------|-------------|
| [Berry I2C Driver](berry-driver/README.md) | ESP32 | Full-featured Berry driver with auto-detection, web sliders, MQTT, presets |
| Native Driver | ESP8266/ESP32 | Coming soon — lightweight C driver for resource-constrained devices |

## Berry Driver Features

- **Zero-configuration** — auto-detects DimmerLink devices on I2C bus
- **Web dashboard** — brightness sliders and sensor readouts
- **Multiple control methods** — console, HTTP API, MQTT, Berry scripting
- **Multi-device support** — up to 4 devices on same I2C bus
- **Multi-channel** — 1-4 channels per device
- **Preset scenes** — named brightness levels (night, low, mid, high, full)
- **Dimming curves** — Linear, RMS, Logarithmic
- **Temperature monitoring** — thermal protection alerts (hardware dependent)
- **Home Assistant** — MQTT auto-discovery compatible

> [!NOTE]
> The Berry driver requires **ESP32** with Tasmota firmware that includes Berry support (`tasmota32` or `tasmota32-berry`). For ESP8266, the native driver (coming soon) will be available.

## Quick Start

1. [Getting Started](berry-driver/01_getting_started.md) — install and verify in 5 minutes
2. [Configuration](berry-driver/02_configuration.md) — customize labels, addresses, presets
3. [Control Methods](berry-driver/03_control.md) — web, HTTP, MQTT, console
4. [Code Examples](berry-driver/04_examples.md) — Berry scripts and automations
5. [Reference](berry-driver/05_reference.md) — command table and troubleshooting
