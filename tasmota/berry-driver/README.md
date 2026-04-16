# Berry Driver Overview

The DimmerLink Berry driver connects CIU32 TRIAC AC dimmer hardware to Tasmota via I2C. It provides native Tasmota integration with web dashboard controls, console commands, HTTP API, and MQTT telemetry.

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
| Hardware | ESP32 board + CIU32 DimmerLink module(s) |
| I2C wiring | SDA, SCL, GND, VCC with **4.7kΩ pull-up resistors** on SDA and SCL |

> [!WARNING]
> Berry is only available on ESP32. For ESP8266, use the native Tasmota driver (coming soon).

## Documentation

| Page | Description |
|------|-------------|
| [Getting Started](01_getting_started.md) | Installation, first boot, verification |
| [Configuration](02_configuration.md) | JSON config, I2C addresses, multi-device setup |
| [Control Methods](03_control.md) | Web dashboard, console, HTTP API, MQTT |
| [Code Examples](04_examples.md) | Berry scripts, automations, Home Assistant |
| [Reference & Troubleshooting](05_reference.md) | Command table, curves, common issues |

## Download

- [`DimmerLink.be`](DimmerLink.be) — main driver
- [`dimmerlink_loader.be`](dimmerlink_loader.be) — auto-loader
