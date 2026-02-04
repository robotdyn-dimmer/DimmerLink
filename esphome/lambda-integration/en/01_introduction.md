[← Home](../../../README.md) | [Contents](./README.md) | [Next: Entities →](./02_entities.md)

---

# ESPHome Integration — Introduction

Integration of DimmerLink with Home Assistant via ESPHome.

---

## Contents

- [1. Overview](#1-overview)
- [2. Requirements](#2-requirements)
- [3. Connection](#3-connection)
- [4. Basic Configuration](#4-basic-configuration)

**Additional Sections:**

- [Lambda Reference](./04_lambda_reference.md) — I2C and registers reference
- [Ready Entities](./02_entities.md) — copy-paste code blocks
- [Complete Examples](./03_examples.md) — working configurations

---

## 1. Overview

### What is ESPHome?

[ESPHome](https://esphome.io/) is a system for creating ESP8266/ESP32 firmware using simple YAML configurations. ESPHome devices automatically integrate with Home Assistant.

### Why ESPHome + DimmerLink?

```text
┌─────────────────┐     I2C      ┌─────────────┐     WiFi     ┌─────────────────┐
│   DimmerLink    │◄────────────►│    ESP32    │◄────────────►│  Home Assistant │
│  (AC Dimmer)    │              │  (ESPHome)  │              │                 │
└─────────────────┘              └─────────────┘              └─────────────────┘
```

**Benefits:**

| Feature | Description |
|---------|-------------|
| **HA Integration** | Dimmer appears as a Light entity |
| **Automations** | Use in Home Assistant scenarios |
| **OTA Updates** | Wireless firmware updates |
| **Local Control** | Works without internet |
| **Low Latency** | Direct connection, no cloud |

### Solution Architecture

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
   AC Dimmer
      │
      │ AC 110/220V
      ▼
    Lamp
```

### Integration Approach

DimmerLink does not have a ready-made component in ESPHome. We use **lambda functions** — embedded C++ code in YAML configuration.

**Why lambda?**

- No external libraries required
- Full control over I2C communication
- Works with any ESPHome version
- Easy to adapt for new registers

**How it looks:**

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);
```

---

## 2. Requirements

### Hardware Requirements

| Component | Requirement | Note |
|-----------|-------------|------|
| **DimmerLink** | v1.0+ | In I2C mode |
| **ESP32** | Any variant | ESP32, ESP32-C3, ESP32-S2, ESP32-S3 |
| **AC Dimmer** | TRIAC module | With Z-C and DIM inputs |
| **Load** | Dimmable | Incandescent bulbs, LED (dimmable) |

### Supported ESP32 Boards

| Board | Default I2C | Note |
|-------|-------------|------|
| ESP32 DevKit | GPIO21/GPIO22 | Recommended |
| ESP32-C3 | GPIO8/GPIO9 | Compact |
| ESP32-S2 | GPIO8/GPIO9 | USB-native |
| ESP32-S3 | GPIO8/GPIO9 | Dual-core |
| ESP32-WROOM | GPIO21/GPIO22 | Classic |
| NodeMCU-32S | GPIO21/GPIO22 | With USB |

> [!WARNING]
> **ESP8266** is also supported, but ESP32 is recommended for stable I2C operation.

### Software Requirements

| Component | Version | Where to get |
|-----------|---------|--------------|
| **ESPHome** | 2023.1+ | Home Assistant Add-on or CLI |
| **Home Assistant** | 2023.1+ | [home-assistant.io](https://home-assistant.io) |
| **Python** | 3.9+ | Only for ESPHome CLI |

### Checking ESPHome Version

**In Home Assistant:**
Settings → Add-ons → ESPHome → Info

**CLI:**

```bash
esphome version
```

### Before You Begin

- [ ] DimmerLink switched to I2C mode (UART command: `02 5B`)
- [ ] ESP32 board available
- [ ] ESPHome installed
- [ ] WiFi network available
- [ ] Home Assistant running (optional)

---

## 3. Connection

### Connection Diagram

```text
┌─────────────┐         ┌─────────────────┐         ┌─────────────┐
│   ESP32     │         │   DimmerLink    │         │  AC Dimmer  │
│             │         │                 │         │             │
│  3.3V ──────┼────────►│ VCC             │         │             │
│  GND ───────┼────────►│ GND         VCC ├────────►│ VCC         │
│  GPIO21 ────┼────────►│ SDA         GND ├────────►│ GND         │
│  GPIO22 ────┼────────►│ SCL         Z-C ├────────►│ Z-C         │
│             │         │             DIM ├────────►│ DIM         │
└─────────────┘         └─────────────────┘         └─────────────┘
```

### Connection Table

| ESP32 | DimmerLink | Function |
|-------|------------|----------|
| 3.3V | VCC | Power |
| GND | GND | Ground |
| GPIO21 | SDA | I2C Data |
| GPIO22 | SCL | I2C Clock |

| DimmerLink | AC Dimmer | Function |
|------------|-----------|----------|
| VCC | VCC | Power |
| GND | GND | Ground |
| Z-C | Z-C | Zero-Cross |
| DIM | DIM | Gate |

### Pull-up Resistors

ESP32 has internal pull-ups, but external resistors are recommended for reliable operation:

| Wire Length | Resistors |
|-------------|-----------|
| < 10 cm | Can work without external |
| 10-30 cm | 4.7 kΩ recommended |
| > 30 cm | 2.2-4.7 kΩ required |

### Switching DimmerLink to I2C Mode

DimmerLink ships in UART mode. I2C is required for ESPHome.

**Via UART (before connecting to ESP32):**

```python
import serial
ser = serial.Serial('COM3', 115200)
ser.write(bytes([0x02, 0x5B]))  # SWITCH_I2C
# Response: 0x00 = OK
```

**Or via terminal (HEX mode):**

```text
Send: 02 5B
Response: 00
```

After switching, DimmerLink saves the mode to EEPROM.

### Verifying Connection

After connecting and flashing ESP32, logs should show:

```text
[I][i2c.arduino:XX]: Found i2c device at address 0x50
```

> [!NOTE]
> **Detailed connection documentation:** [Hardware Connection](../../../02_HARDWARE_CONNECTION.md)

---

## 4. Basic Configuration

### Step 1: Create Project

**In Home Assistant (Add-on):**

1. Settings → Add-ons → ESPHome
2. "+ NEW DEVICE"
3. Name: `dimmerlink`
4. Select board type: ESP32

**CLI:**

```bash
esphome wizard dimmerlink.yaml
```

### Step 2: Basic YAML

Create file `dimmerlink.yaml`:

```yaml
# ============================================================
# DimmerLink + ESPHome — Basic Configuration
# ============================================================

substitutions:
  device_name: "dimmerlink"
  friendly_name: "DimmerLink"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev
  framework:
    type: arduino

# Logging
logger:
  level: INFO

# Home Assistant API
api:
  encryption:
    key: !secret api_key

# OTA updates
ota:
  platform: esphome
  password: !secret ota_password

# WiFi
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  # Fallback access point
  ap:
    ssid: "${device_name}-AP"
    password: "12345678"

captive_portal:

# ============================================================
# I2C Bus
# ============================================================
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz
  id: bus_a

# ============================================================
# Output — Communication with DimmerLink
# ============================================================
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          // Convert 0.0-1.0 → 0-100%
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;

          // Write to DIM0_LEVEL register (0x10)
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);

# ============================================================
# Light — What Home Assistant sees
# ============================================================
light:
  - platform: monochromatic
    name: "Light"
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF
```

### Step 3: secrets.yaml File

Create `secrets.yaml` in the same folder:

```yaml
wifi_ssid: "YourNetwork"
wifi_password: "YourPassword"
api_key: "generate_via_esphome"
ota_password: "your_ota_password"
```

> [!TIP]
> **Generating api_key:** ESPHome automatically generates a key when creating a project. Or use: `esphome wizard`.

### Step 4: Compile and Upload

**In Home Assistant:**

1. Click "INSTALL" on the device card
2. Select "Plug into this computer" (first time via USB)
3. Wait for upload to complete

**CLI:**

```bash
# First upload via USB
esphome run dimmerlink.yaml

# Subsequent updates — over the air
esphome run dimmerlink.yaml --device dimmerlink.local
```

### Step 5: Verification

**In ESPHome logs:**

```text
[I][i2c.arduino:XX]: Found i2c device at address 0x50
[I][app:XX]: ESPHome version 2024.x.x compiled on ...
[I][wifi:XX]: WiFi Connected!
[I][api:XX]: Home Assistant connected!
```

**In Home Assistant:**

1. Settings → Devices & Services
2. Find "ESPHome" → "DimmerLink"
3. Light entity should appear

### Step 6: Testing

**In Home Assistant:**

1. Open the Light card
2. Move the brightness slider
3. Lamp should change brightness

**Via Developer Tools:**

```yaml
service: light.turn_on
target:
  entity_id: light.dimmerlink_light
data:
  brightness_pct: 50
```

---

## What's Next?

### Extending Functionality

After successful basic configuration, add:

| Feature | Section |
|---------|---------|
| Dimming curve selection | [Ready Entities → Select](./02_entities.md#66-select--curve-selection) |
| AC frequency monitoring | [Ready Entities → Sensor](./02_entities.md#63-sensor--sensors) |
| Device status | [Ready Entities → Binary Sensor](./02_entities.md#64-binary-sensor--binary-sensors) |
| Physical button | [Examples → With Button](./03_examples.md#75-with-physical-button) |
| Multiple dimmers | [Examples → Multi-device](./03_examples.md#74-multi-device) |

### Recommended Configuration

For most users, we recommend the **Standard Example**:

→ [03_examples.md → 7.2 Standard](./03_examples.md#72-standard)

Includes:

- Light with smooth transitions
- Curve selection (LINEAR / RMS / LOG)
- AC frequency sensor
- Ready status
- Restart button

### Reference Materials

| Document | Content |
|----------|---------|
| [Lambda Reference](./04_lambda_reference.md) | All registers, command codes, lambda examples |
| [Ready Entities](./02_entities.md) | Copy-paste blocks for each entity type |
| [Complete Examples](./03_examples.md) | 6 ready configurations |

---

## Quick Start (TL;DR)

```yaml
# Minimum for operation — copy and modify WiFi

substitutions:
  device_name: "dimmerlink"

esphome:
  name: ${device_name}

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

**30 lines — and the dimmer is in Home Assistant!**

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release |

---

[← Home](../../../README.md) | [Contents](./README.md) | [Next: Entities →](./02_entities.md)
