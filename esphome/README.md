# DimmerLink — ESPHome Integration

Integration of DimmerLink AC TRIAC Dimmer with Home Assistant via ESPHome.

---

## Two Integration Methods

DimmerLink supports two approaches for ESPHome integration:

### 1. External Component (Recommended)

A ready-to-use ESPHome component with clean YAML configuration. No C++ or lambda code required.

```yaml
external_components:
  - source: github://robotdyn-dimmer/DimmerLink@main
    components: [dimmerlink]
    refresh: 1d
    path: esphome/components

dimmerlink:
  id: dimmer1
  address: 0x50

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Dimmer"
```

**Best for:** Most users. Simple YAML, automatic I2C communication, built-in sensors, selects, and buttons.

> [Full Component Documentation](./components/README.md) | [Example YAML](./components/example.yaml)

### 2. Lambda Integration (Advanced)

Direct I2C register access using ESPHome lambda functions. Provides full control over communication protocol.

```yaml
output:
  - platform: template
    id: dimmer_out
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);

light:
  - platform: monochromatic
    name: "Dimmer"
    output: dimmer_out
```

**Best for:** Advanced users who need custom logic, non-standard configurations, or want to understand the I2C protocol in detail.

> [Full Lambda Documentation](./lambda-integration/en/README.md) | [Lambda Reference](./lambda-integration/en/04_lambda_reference.md)

---

## Comparison

| Feature | External Component | Lambda Integration |
|---------|-------------------|-------------------|
| Setup complexity | Simple YAML | Requires C++ lambdas |
| Lines of code | ~30 lines | ~100+ lines |
| Sensors & status | Built-in | Manual implementation |
| Dimming curve select | Built-in | Manual implementation |
| Reset / Recalibrate | Built-in buttons | Manual commands |
| Multiple devices | Native support | Manual per-device |
| Customization | Standard options | Full flexibility |
| I2C error handling | Automatic | Manual |

---

## Architecture

```text
Home Assistant
      │
      │ WiFi (Native API)
      ▼
   ESP32 + ESPHome
      │
      │ I2C (100 kHz)
      ▼
  DimmerLink
      │
      │ Zero-Cross + Gate
      ▼
   AC Dimmer (TRIAC)
      │
      │ AC 110/220V
      ▼
    Lamp
```

---

## Quick Start

### Wiring

```text
ESP32          DimmerLink
─────          ──────────
3.3V     →     VCC
GND      →     GND
GPIO21   →     SDA
GPIO22   →     SCL
```

### Minimal Configuration (External Component)

```yaml
esphome:
  name: dimmerlink

esp32:
  board: esp32dev

logger:
api:
ota:

wifi:
  ssid: "YOUR_WIFI"
  password: "YOUR_PASSWORD"

external_components:
  - source: github://robotdyn-dimmer/DimmerLink@main
    components: [dimmerlink]
    refresh: 1d
    path: esphome/components

i2c:
  sda: GPIO21
  scl: GPIO22

dimmerlink:
  id: dimmer1

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Dimmer"
    gamma_correct: 1.0
```

---

## Entity Types (External Component)

| Entity | Platform | Description |
|--------|----------|-------------|
| **Light** | `dimmerlink` | Brightness control (0-100%) |
| **Sensor** | `dimmerlink` | AC frequency, brightness level, firmware version |
| **Binary Sensor** | `dimmerlink` | Ready status, error flag, calibration status |
| **Select** | `dimmerlink` | Dimming curve (LINEAR / RMS / LOG) |
| **Button** | `dimmerlink` | Reset, Recalibrate commands |

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [I2C Communication](../04_I2C_COMMUNICATION.md) | I2C protocol and register map |
| [Hardware Connection](../02_HARDWARE_CONNECTION.md) | Connection diagrams |
| [Quick Start Guide](../01_QUICK_START.md) | Getting started with DimmerLink |
| [FAQ & Troubleshooting](../07_FAQ_TROUBLESHOOTING.md) | Common issues and solutions |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release: lambda integration documentation |
| 1.1 | 2026-02 | Added external component |
