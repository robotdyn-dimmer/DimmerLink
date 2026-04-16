# Native Driver Overview — DimmerLink Tasmota

**Driver:** `xdrv_94_dimmerlink.ino` (XDRV_94 / XI2C_100)
**Platforms:** ESP32, ESP8266

## What is the native driver?

The DimmerLink native C driver (`xdrv_94_dimmerlink.ino`) integrates the MCU TRIAC AC dimmer controller into Tasmota as a compiled driver. Unlike the Berry scripted driver, the native driver is compiled directly into the firmware binary and requires no file upload or `autoexec.be` configuration. It activates automatically at boot whenever a compatible DimmerLink device is detected on the I2C bus.

## Key differences from the Berry driver

| Aspect | Native C driver | Berry driver |
|--------|----------------|--------------|
| Activation | Automatic — compiled in, detects I2C at boot | Manual — files uploaded, configured in `autoexec.be` |
| Platform | ESP32 **and** ESP8266 | ESP32 only (Berry not available on ESP8266) |
| Configuration | `user_config_override.h` at compile time | `/dimmerlink.json` at runtime |
| Command prefix | `Dl` (e.g., `DlDim`, `DlCurve`) | `DimmerLink` (e.g., `DimmerLink_Kitchen`) |
| Named instances | Not supported — devices indexed as Dev1, Dev2 | Supported — each device has a label |
| Auto-scan | Full I2C bus scan across all buses (0x08-0x77) | Manual address configuration |

## Key features

- Up to **4 devices**, each with up to **4 channels** (16 channels total)
- **Web UI** brightness sliders and sensor display
- **MQTT telemetry** with per-channel JSON data
- **HTTP API** for remote control via `curl` or automation systems
- Three **dimming curves**: LINEAR, RMS, LOG (per channel)
- Hardware **fade** transitions (0 to 25.5 seconds)
- **Temperature monitoring** and thermal protection reporting
- Automatic **AC frequency** detection (50/60 Hz)

## Requirements

| Category | Details |
|----------|---------|
| **Build tool** | PlatformIO (CLI or VS Code extension) |
| **MCU** | ESP32 (any variant) or ESP8266 |
| **Hardware** | DimmerLink device with I2C mode enabled |
| **Wiring** | SDA, SCL, GND, VCC with 4.7 kOhm pull-ups |

## Driver files

| File | Purpose |
|------|---------|
| `xdrv_94_dimmerlink.ino` | Driver source — place in `tasmota/` directory |
| `user_config_override.h` | Enable `USE_DIMMERLINK` define |

> [!TIP]
> Download the driver source:
> [xdrv_94_dimmerlink.ino on GitHub](https://github.com/robotdyn-dimmer/DimmerLink/blob/main/tasmota/xdrv-driver/xdrv_94_dimmerlink.ino)

## Documentation

| Page | Description |
|------|-------------|
| [Build & Flash](01_build_flash.md) | PlatformIO setup, compilation, flashing via USB and OTA |
| [Commands](02_commands.md) | Full command reference — DlDim, DlCurve, DlFade, DlStatus, DlReset, DlRecalibrate, DlAddress |
| [Web, MQTT & HTTP](03_web_mqtt_http.md) | Web UI sliders, MQTT telemetry JSON, HTTP API endpoints |
| [Advanced Features](04_advanced.md) | Dimming curves explained, fade control, temperature monitoring, multi-device setup, migration from Berry |
| [Reference](05_reference.md) | Troubleshooting, error codes, I2C register map, thermal states |
