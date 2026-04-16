# Berry Driver Overview

The DimmerLink Berry driver connects MCU TRIAC AC dimmer hardware to Tasmota via I2C. It provides native Tasmota integration with web dashboard controls, console commands, HTTP API, and MQTT telemetry.

## Driver Files

| File | Purpose |
|------|---------|
| `DimmerLink.be` | Main driver — I2C communication, commands, web UI, telemetry |
| `dimmerlink_loader.be` | Auto-loader — reads config, creates instances, registers presets |
| `dimmerlink.json` | Device configuration — auto-generated on first boot |

## Requirements

| Component | Details |
|-----------|---------|
| Tasmota firmware | `tasmota32` or `tasmota32-berry` (ESP32 with Berry support) |
| Hardware | ESP32 board + MCU DimmerLink module(s) |
| I2C wiring | SDA, SCL, GND, VCC with **4.7kΩ pull-up resistors** on SDA and SCL |

> [!WARNING]
> Berry is only available on ESP32. For ESP8266, use the native Tasmota driver (coming soon).

## Documentation

| Page | Description |
|------|-------------|
| [Getting Started](https://www.rbdimmer.com/docs/dimmerlink-tasmota-berry-start) | Installation, first boot, verification |
| [Configuration](https://www.rbdimmer.com/docs/dimmerlink-tasmota-berry-config) | JSON config, I2C addresses, multi-device setup |
| [Control Methods](https://www.rbdimmer.com/docs/dimmerlink-tasmota-berry-control) | Web dashboard, console, HTTP API, MQTT |
| [Code Examples](https://www.rbdimmer.com/docs/dimmerlink-tasmota-berry-examples) | Berry scripts, automations, Home Assistant |
| [Reference & Troubleshooting](https://www.rbdimmer.com/docs/dimmerlink-tasmota-berry-reference) | Command table, curves, common issues |

## Download

- [`DimmerLink.be`](https://github.com/robotdyn-dimmer/DimmerLink/blob/main/tasmota/berry-driver/DimmerLink.be) — main driver
- [`dimmerlink_loader.be`](https://github.com/robotdyn-dimmer/DimmerLink/blob/main/tasmota/berry-driver/dimmerlink_loader.be) — auto-loader
