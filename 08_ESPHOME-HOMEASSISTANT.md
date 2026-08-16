# DimmerLink — ESPHome Integration

**Flicker-free AC dimming for Home Assistant.** A dedicated timing controller that eliminates the #1 problem with TRIAC dimmers.

---

## ❌ The Problem

Using ESPHome's `ac_dimmer` component?

- 💡 **Flickering** at low brightness levels
- ⚡ **WiFi conflicts** — interrupts break timing
- 🔥 **ESP8266 unstable** — not enough resources for timing-critical code
- 📝 **Complex tuning** — timing depends on load type and mains quality

> *"The lamp has an irregular flicker"* — [ESPHome Issue #4131](https://github.com/esphome/issues/issues/4131)

## ✅ The Solution

**DimmerLink** — a compact module with dedicated Cortex-M0+ that handles ALL timing-critical operations: zero-cross detection, phase angle calculation, and TRIAC gate control with microsecond precision.

Your ESP just sends: *"Set brightness to 50%"* via I2C. That's it.

- ✅ **Zero flickering** — hardware-level timing, no software jitter
- ✅ **3 lines of YAML** — no interrupts, no libraries, no conflicts
- ✅ **Works on any ESP** — ESP8266, ESP32, ESP32-S2/S3/C3
- ✅ **Auto 50/60 Hz** — automatic mains frequency detection
- ✅ **3 dimming curves** — Linear, RMS, Logarithmic

---

## DimmerLink vs ESPHome ac_dimmer

| | ac_dimmer | **DimmerLink** |
|---|-----------|----------------|
| Flickering | ⚠️ Common at low levels ([#4131](https://github.com/esphome/issues/issues/4131)) | ✅ **Never** — hardware timing |
| ESP8266 support | ⚠️ Unstable (interrupt conflicts) | ✅ **Stable** — no interrupts needed |
| CPU usage | High (timing-critical ISR) | **< 1%** (just I2C writes) |
| WiFi interference | Yes (interrupts vs WiFi stack) | **None** |
| Setup complexity | Medium (tuning required) | **Simple YAML** |
| Dimming curves | Software only | **3 hardware curves** |
| AC frequency | Manual configuration | **Auto-detect 50/60 Hz** |
| Additional hardware | $0 | [$1.99 module](https://www.rbdimmer.com/shop/dimmerlink-controller-uart-i2c-interface-for-ac-dimmers-48?utm_source=github&utm_medium=referral&utm_campaign=dimmerlink) |

> **When to use `ac_dimmer`:** If it already works well for your setup — keep using it!
>
> **When to use DimmerLink:** Flickering issues, ESP8266, multiple dimmers, or you want plug-and-play simplicity.

---

![DimmerLink](https://github.com/robotdyn-dimmer/DimmerLink/raw/main/images/DimLink_pic.jpeg)

- **Product Page**: [DImmerLink on rbdimmer.com](https://www.rbdimmer.com/shop/dimmerlink-controller-uart-i2c-interface-for-ac-dimmers-48?utm_source=github&utm_medium=referral&utm_campaign=dimmerlink)
- **AliExpress Page**: [DImmerLink on AliExpress](https://aliexpress.com/item/1005011583805008.html)

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

dimmerlink:
  id: dimmer1
  address: 0x50

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Dimmer"
```

**Best for:** Most users. Simple YAML, automatic I2C communication, built-in sensors, selects, and buttons.

> [Full Component Documentation](./components/README.md)

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

> [Full Lambda Documentation](./esphome/lambda-integration/en/README.md) | [Lambda Reference](./esphome/lambda-integration/en/04_lambda_reference.md)

---

## Comparison of Integration Methods

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
| [I2C Communication](./04_I2C_COMMUNICATION.md) | I2C protocol and register map |
| [Hardware Connection](./02_HARDWARE_CONNECTION.md) | Connection diagrams |
| [Quick Start Guide](./01_QUICK_START.md) | Getting started with DimmerLink |
| [FAQ & Troubleshooting](./07_FAQ_TROUBLESHOOTING.md) | Common issues and solutions |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release: lambda integration documentation |
| 1.1 | 2026-02 | Added external component |
