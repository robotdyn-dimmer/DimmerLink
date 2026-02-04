# DimmerLink — ESPHome Lambda Integration

[← ESPHome Integration](../../README.md)

---

Integration of DimmerLink AC TRIAC Dimmer with Home Assistant via ESPHome using lambda functions.

> [!TIP]
> For a simpler approach without lambda code, see the [External Component](../../components/README.md).

---

## Contents

### Main Sections

| # | Section | Description |
|---|---------|-------------|
| 1 | [Introduction](./01_introduction.md) | Overview, requirements, connection, basic configuration |
| 2 | [Entities](./02_entities.md) | Ready-to-use entities for your configuration |
| 3 | [Examples](./03_examples.md) | Complete working YAML configurations |
| 4 | [Lambda Reference](./04_lambda_reference.md) | I2C registers and lambda functions reference |
| 5 | [Troubleshooting](./Troubleshooting.md) | Home Assistant integration, troubleshooting, roadmap |

---

## Quick Start

### 1. Connection

```text
ESP32          DimmerLink
─────          ──────────
3.3V     →     VCC
GND      →     GND
GPIO21   →     SDA
GPIO22   →     SCL
```

### 2. Minimal Configuration

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

i2c:
  sda: GPIO21
  scl: GPIO22
  id: bus_a

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
    gamma_correct: 1.0
```

### 3. Flash

```bash
esphome run dimmerlink.yaml
```

**30 lines — and the dimmer is in Home Assistant!**

---

## Entity Types

| Entity | Platform | Description |
|--------|----------|-------------|
| **Light** | `monochromatic` | Main dimmer with brightness |
| **Sensor** | `template` | AC frequency, level, version |
| **Binary Sensor** | `template` | Ready status, error flag |
| **Text Sensor** | `template` | Error code (text) |
| **Select** | `template` | Dimming curve selection |
| **Number** | `template` | Direct level control |
| **Button** | `template` | Reset, Recalibrate |

→ [More about Entities](./02_entities.md)

---

## Configuration Examples

| Example | Description | Link |
|---------|-------------|------|
| **Minimal** | Light + basic I2C | [→](./03_examples.md#71-minimal) |
| **Standard** | Light + Sensors + Select | [→](./03_examples.md#72-standard) |
| **Extended** | All entities + diagnostics | [→](./03_examples.md#73-extended) |
| **Multi-device** | 2+ dimmers on one bus | [→](./03_examples.md#74-multi-device) |
| **With Button** | Physical button control | [→](./03_examples.md#75-with-physical-button) |
| **Production** | For production environment | [→](./03_examples.md#76-production) |

→ [All Examples](./03_examples.md)

---

## Register Map (Brief)

| Address | Name | R/W | Description |
|---------|------|-----|-------------|
| `0x00` | STATUS | R | Device status |
| `0x01` | COMMAND | W | Control commands |
| `0x02` | ERROR | R | Last error code |
| `0x10` | DIM0_LEVEL | R/W | Brightness 0-100% |
| `0x11` | DIM0_CURVE | R/W | Curve 0-2 |
| `0x20` | AC_FREQ | R | AC frequency |
| `0x30` | I2C_ADDRESS | R/W | I2C address |

→ [Full Reference](./04_lambda_reference.md#46-reference-tables)

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [External Component](../../../components/README.md) | ESPHome DimmerLink component (no lambdas) |
| [I2C Communication](../../../04_I2C_COMMUNICATION.md) | I2C protocol details |
| [Hardware Connection](../../../02_HARDWARE_CONNECTION.md) | Connection diagrams |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release |

---

[← ESPHome Integration](../../README.md)
