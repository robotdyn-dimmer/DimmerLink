# DimmerLink ESPHome Component

ESPHome external component for **DimmerLink** — AC TRIAC dimmer controller with I2C interface.

## Features

- Smooth brightness control (0-100%)
- Multiple dimming curves: LINEAR, RMS, LOG
- Real-time status monitoring
- AC frequency detection (50/60 Hz)
- Device reset and recalibration commands
- Full Home Assistant integration

## Requirements

- ESPHome 2024.1.0 or newer
- ESP32 or ESP8266 board
- DimmerLink controller connected via I2C

## Installation

Add to your ESPHome configuration:

```yaml
external_components:
  - source: github://robotdyn-dimmer/DimmerLink@main
    components: [dimmerlink]
```

For local development:

```yaml
external_components:
  - source: components
    components: [dimmerlink]
```

## Wiring

| DimmerLink | ESP32 | ESP8266 |
|------------|-------|---------|
| SDA | GPIO21 | GPIO4 |
| SCL | GPIO22 | GPIO5 |
| GND | GND | GND |
| VCC | 3.3V | 3.3V |

## Quick Start

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22

dimmerlink:
  id: dimmer1
  address: 0x50

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Living Room Light"
```

---

## Configuration Reference

### Hub Configuration

The `dimmerlink` hub establishes I2C communication with the device.

```yaml
dimmerlink:
  id: dimmer1
  address: 0x50  # Optional, default: 0x50
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | *Required* | Hub identifier |
| `address` | hex | `0x50` | I2C address (0x08-0x77) |

---

### Light Platform

Controls dimmer brightness.

```yaml
light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Dimmer"
    default_transition_length: 1s
    gamma_correct: 1.0
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `dimmerlink_id` | ID | *Required* | Reference to hub |
| `name` | string | *Required* | Entity name |
| `default_transition_length` | time | `1s` | Fade duration |
| `gamma_correct` | float | `1.0` | Gamma correction (1.0 = disabled) |

---

### Sensor Platform

Monitors device parameters.

```yaml
sensor:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    update_interval: 60s
    ac_frequency:
      name: "AC Frequency"
    level:
      name: "Brightness Level"
    firmware_version:
      name: "Firmware"
    ac_period:
      name: "AC Period"
```

| Sensor | Unit | Description |
|--------|------|-------------|
| `ac_frequency` | Hz | AC mains frequency (50 or 60) |
| `level` | % | Current brightness (0-100) |
| `firmware_version` | - | Device firmware version |
| `ac_period` | μs | AC half-period in microseconds |

---

### Binary Sensor Platform

Device status indicators.

```yaml
binary_sensor:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    update_interval: 5s
    ready:
      name: "Ready"
    error:
      name: "Error"
    calibration_done:
      name: "Calibration Done"
```

| Sensor | Description |
|--------|-------------|
| `ready` | Device is ready for commands |
| `error` | Error condition detected |
| `calibration_done` | Zero-crossing calibration complete |

---

### Select Platform

Dimming curve selection.

```yaml
select:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    curve:
      name: "Dimming Curve"
```

**Available curves:**

| Curve | Description |
|-------|-------------|
| `LINEAR` | Linear brightness control |
| `RMS` | RMS-based power control (constant power) |
| `LOG` | Logarithmic curve (perceived brightness) |

---

### Button Platform

Device control commands.

```yaml
button:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    reset:
      name: "Reset"
    recalibrate:
      name: "Recalibrate"
```

| Button | Description |
|--------|-------------|
| `reset` | Restart the DimmerLink device (~3 sec) |
| `recalibrate` | Recalibrate AC zero-crossing (~200 ms) |

---

## Complete Example

```yaml
esphome:
  name: dimmerlink-demo
  friendly_name: DimmerLink Demo

esp32:
  board: esp32dev

logger:
api:
  encryption:
    key: !secret api_key
ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "DimmerLink-Fallback"

captive_portal:
web_server:

external_components:
  - source: github://robotdyn-dimmer/DimmerLink@main
    components: [dimmerlink]

i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz

dimmerlink:
  id: dimmer1
  address: 0x50

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Light"
    default_transition_length: 500ms

sensor:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    ac_frequency:
      name: "AC Frequency"
    level:
      name: "Brightness"
    firmware_version:
      name: "Firmware"

binary_sensor:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    ready:
      name: "Ready"
    error:
      name: "Error"

select:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    curve:
      name: "Curve"

button:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    reset:
      name: "Reset"
    recalibrate:
      name: "Recalibrate"
```

---

## Multiple Devices

Connect multiple dimmers on the same I2C bus:

```yaml
dimmerlink:
  - id: dimmer_living
    address: 0x50
  - id: dimmer_bedroom
    address: 0x51
  - id: dimmer_kitchen
    address: 0x52

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer_living
    name: "Living Room"
  - platform: dimmerlink
    dimmerlink_id: dimmer_bedroom
    name: "Bedroom"
  - platform: dimmerlink
    dimmerlink_id: dimmer_kitchen
    name: "Kitchen"
```

---

## Troubleshooting

### Device Not Found

1. Check I2C wiring (SDA, SCL, GND, VCC)
2. Enable I2C scan:
   ```yaml
   i2c:
     scan: true
   ```
3. Verify device address in ESPHome logs

### AC Frequency Shows "Unknown"

- Wait for calibration to complete (~2 seconds after power-up)
- Check `calibration_done` binary sensor
- Press "Recalibrate" button

### Light Not Responding

1. Check `ready` status (should be "Connected")
2. Check `error` status (should be "OK")
3. Verify I2C frequency is 100kHz or lower:
   ```yaml
   i2c:
     frequency: 100kHz
   ```

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | OK | No error |
| 0xF9 | SYNTAX | Command syntax error |
| 0xFC | NOT_READY | Device not calibrated or FLASH error |
| 0xFD | INDEX | Invalid channel index |
| 0xFE | PARAM | Invalid parameter value |

---

## Technical Specifications

- **I2C Address:** 0x50 (default), configurable 0x08-0x77
- **I2C Speed:** 100 kHz (Standard Mode)
- **Brightness Range:** 0-100%
- **Startup Delay:** 2 seconds (for calibration)
- **Supported AC:** 50Hz / 60Hz (auto-detect)

---

## License

MIT License - see [LICENSE](../esphome/LICENSE)

## Links

- [DimmerLink Hardware](https://rbdimmer.com)
- [ESPHome Documentation](https://esphome.io)
- [Home Assistant](https://www.home-assistant.io)
