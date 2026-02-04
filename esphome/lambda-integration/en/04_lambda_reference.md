[<- Back: Examples](./03_examples.md) | [Contents](./README.md) | [Next: Troubleshooting ->](./Troubleshooting.md)

---

# ESPHome Integration — Lambda Reference

Complete reference for working with DimmerLink through ESPHome lambda.

> [!NOTE]
> **Document in progress.** DimmerLink is actively evolving — new registers and functions will be added to this reference.

---

## Contents

- [4.1 I2C Basics in ESPHome](#41-i2c-basics-in-esphome)
- [4.2 Control Registers (0x00-0x0F)](#42-control-registers-0x00-0x0f)
- [4.3 Dimmer Registers (0x10-0x1F)](#43-dimmer-registers-0x10-0x1f)
- [4.4 System Registers (0x20-0x2F)](#44-system-registers-0x20-0x2f)
- [4.5 Configuration Registers (0x30-0x3F)](#45-configuration-registers-0x30-0x3f)
- [4.6 Reference Tables](#46-reference-tables)

---

## 4.1 I2C Basics in ESPHome

### I2C Bus Configuration

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz
  id: bus_a
```

| Parameter | Description | Recommendation |
|-----------|-------------|----------------|
| `sda` | Data pin | GPIO21 (ESP32 default) |
| `scl` | Clock pin | GPIO22 (ESP32 default) |
| `scan` | Scan bus on startup | `true` for debugging |
| `frequency` | Bus frequency | `100kHz` (DimmerLink Standard Mode) |
| `id` | Bus identifier | Required for lambda |

---

### Constants and Device Address

```yaml
substitutions:
  # DimmerLink I2C address (default)
  dimmerlink_addr: "0x50"
```

For use in lambda:

```cpp
// Device address
const uint8_t DIMMERLINK_ADDR = 0x50;
```

---

### Basic I2C Operations

#### Writing to a Register

```cpp
// Template: write a single byte to a register
uint8_t reg = 0x10;       // Register address
uint8_t value = 50;       // Value to write
uint8_t data[2] = {reg, value};
id(bus_a).write(DIMMERLINK_ADDR, data, 2, true);
```

**`write()` Parameters:**
- `address` — I2C device address
- `data` — pointer to data
- `length` — number of bytes
- `stop` — send STOP condition (`true` usually)

#### Reading from a Register

```cpp
// Template: read a single byte from a register
uint8_t reg = 0x10;
uint8_t value = 0;

// 1. Send register address (without STOP)
auto err = id(bus_a).write(DIMMERLINK_ADDR, &reg, 1, false);
if (err != i2c::ERROR_OK) {
    ESP_LOGW("dimmerlink", "Write error: %d", err);
    return {};  // or error handling
}

// 2. Read data
err = id(bus_a).read(DIMMERLINK_ADDR, &value, 1);
if (err != i2c::ERROR_OK) {
    ESP_LOGW("dimmerlink", "Read error: %d", err);
    return {};
}

// value now contains the read value
```

#### Reading Multiple Bytes

```cpp
// Reading 2 bytes (e.g., 16-bit value)
uint8_t reg = 0x21;
uint8_t buffer[2] = {0, 0};

id(bus_a).write(DIMMERLINK_ADDR, &reg, 1, false);
id(bus_a).read(DIMMERLINK_ADDR, buffer, 2);

uint16_t value = (buffer[1] << 8) | buffer[0];  // Little-endian
```

---

### Error Handling

```cpp
#include "esphome/components/i2c/i2c.h"

// ESPHome I2C error codes
// i2c::ERROR_OK          = 0  — success
// i2c::ERROR_TIMEOUT     = 1  — timeout
// i2c::ERROR_NOT_ACKNOWLEDGED = 2  — NACK
// i2c::ERROR_DATA_TOO_LARGE   = 3  — too much data
// i2c::ERROR_UNKNOWN     = 4  — unknown error

auto err = id(bus_a).write(DIMMERLINK_ADDR, data, 2, true);

if (err == i2c::ERROR_OK) {
    ESP_LOGD("dimmerlink", "Success");
} else if (err == i2c::ERROR_NOT_ACKNOWLEDGED) {
    ESP_LOGW("dimmerlink", "Device not responding (NACK)");
} else {
    ESP_LOGE("dimmerlink", "I2C error: %d", err);
}
```

---

### Logging

```cpp
// ESPHome logging levels
ESP_LOGV("tag", "Verbose: %d", value);   // Very detailed
ESP_LOGD("tag", "Debug: %d", value);     // Debug
ESP_LOGI("tag", "Info: %d", value);      // Information
ESP_LOGW("tag", "Warning: %d", value);   // Warning
ESP_LOGE("tag", "Error: %d", value);     // Error
```

---

### Helper Functions (Global)

For convenience, you can define helper functions:

```yaml
esphome:
  includes:
    - dimmerlink_helpers.h
```

File `dimmerlink_helpers.h` (in the configuration folder):

```cpp
#pragma once

#include "esphome.h"

namespace dimmerlink {

const uint8_t ADDR = 0x50;

// Registers
namespace reg {
    const uint8_t STATUS      = 0x00;
    const uint8_t COMMAND     = 0x01;
    const uint8_t ERROR       = 0x02;
    const uint8_t VERSION     = 0x03;
    const uint8_t DIM0_LEVEL  = 0x10;
    const uint8_t DIM0_CURVE  = 0x11;
    const uint8_t AC_FREQ     = 0x20;
    const uint8_t AC_PERIOD_L = 0x21;
    const uint8_t AC_PERIOD_H = 0x22;
    const uint8_t CALIBRATION = 0x23;
    const uint8_t I2C_ADDRESS = 0x30;
}

// Commands
namespace cmd {
    const uint8_t NOP         = 0x00;
    const uint8_t RESET       = 0x01;
    const uint8_t RECALIBRATE = 0x02;
    const uint8_t SWITCH_UART = 0x03;
}

// Curves
namespace curve {
    const uint8_t LINEAR = 0;
    const uint8_t RMS    = 1;
    const uint8_t LOG    = 2;
}

// DimmerLink errors
namespace error {
    const uint8_t OK         = 0x00;
    const uint8_t SYNTAX     = 0xF9;
    const uint8_t NOT_READY  = 0xFC;
    const uint8_t INDEX      = 0xFD;
    const uint8_t PARAM      = 0xFE;
}

}  // namespace dimmerlink
```

Usage in lambda:

```cpp
uint8_t data[2] = {dimmerlink::reg::DIM0_LEVEL, 50};
id(bus_a).write(dimmerlink::ADDR, data, 2, true);
```

> [!TIP]
> Using the helper file is optional. All examples below work without it.

---

## 4.2 Control Registers (0x00-0x0F)

Device control and status registers.

### STATUS (0x00) — Device Status

| Parameter | Value |
|-----------|-------|
| **Address** | `0x00` |
| **Access** | Read-only |
| **Size** | 1 byte |

**Bit Structure:**

| Bit | Name | Description |
|-----|------|-------------|
| 0 | READY | 1 = Device ready (calibration complete) |
| 1 | ERROR | 1 = Last operation ended with error |
| 2-7 | — | Reserved |

**Lambda — reading status:**

```cpp
// Read full status byte
uint8_t reg = 0x00;
uint8_t status = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
        bool ready = status & 0x01;
        bool error = status & 0x02;

        ESP_LOGI("dimmerlink", "Status: ready=%d, error=%d", ready, error);
    }
}
```

**ESPHome Entity — Binary Sensor:**

```yaml
binary_sensor:
  # Device ready status
  - platform: template
    name: "DimmerLink Ready"
    id: dimmerlink_ready
    icon: "mdi:check-circle"
    entity_category: "diagnostic"
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x01) != 0;
        }
      }
      return {};

  # Error flag
  - platform: template
    name: "DimmerLink Error Flag"
    id: dimmerlink_error_flag
    icon: "mdi:alert-circle"
    entity_category: "diagnostic"
    device_class: problem
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x02) != 0;
        }
      }
      return {};
```

---

### COMMAND (0x01) — Control Commands

| Parameter | Value |
|-----------|-------|
| **Address** | `0x01` |
| **Access** | Write-only |
| **Size** | 1 byte |

**Available Commands:**

| Value | Command | Description | Execution Time |
|-------|---------|-------------|----------------|
| `0x00` | NOP | No operation | — |
| `0x01` | RESET | Software reset | ~3 sec (reboot) |
| `0x02` | RECALIBRATE | Recalibrate AC frequency | ~200 ms |
| `0x03` | SWITCH_UART | Switch to UART mode | Immediately |

**Lambda — sending command:**

```cpp
// General command sending template
void send_command(uint8_t cmd) {
    uint8_t data[2] = {0x01, cmd};
    id(bus_a).write(0x50, data, 2, true);
}
```

```cpp
// RESET — software reset
uint8_t data[2] = {0x01, 0x01};
id(bus_a).write(0x50, data, 2, true);
ESP_LOGW("dimmerlink", "Reset command sent, device will reboot");
```

```cpp
// RECALIBRATE — frequency recalibration
uint8_t data[2] = {0x01, 0x02};
id(bus_a).write(0x50, data, 2, true);
ESP_LOGI("dimmerlink", "Recalibration started");
```

```cpp
// SWITCH_UART — switch to UART
// After this command I2C no longer works!
uint8_t data[2] = {0x01, 0x03};
id(bus_a).write(0x50, data, 2, true);
ESP_LOGW("dimmerlink", "Switched to UART mode, I2C disabled");
```

> [!WARNING]
> After the `SWITCH_UART` command, the device stops responding via I2C!

**ESPHome Entity — Button:**

```yaml
button:
  # Software reset
  - platform: template
    name: "DimmerLink Reset"
    id: dimmerlink_reset
    icon: "mdi:restart"
    entity_category: "config"
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x01};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGW("dimmerlink", "Reset command sent");
      - delay: 3s
      - logger.log:
          level: INFO
          format: "DimmerLink should be back online"

  # Recalibration
  - platform: template
    name: "DimmerLink Recalibrate"
    id: dimmerlink_recalibrate
    icon: "mdi:refresh"
    entity_category: "config"
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x02};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Recalibration started");
      - delay: 500ms
      - component.update: ac_frequency
```

---

### ERROR (0x02) — Last Error Code

| Parameter | Value |
|-----------|-------|
| **Address** | `0x02` |
| **Access** | Read-only |
| **Size** | 1 byte |

**Error Codes:**

| Code | Name | Description |
|------|------|-------------|
| `0x00` | OK | No error |
| `0xF9` | ERR_SYNTAX | Invalid register address |
| `0xFC` | ERR_NOT_READY | Device not ready (not calibrated or FLASH error) |
| `0xFD` | ERR_INDEX | Invalid dimmer index |
| `0xFE` | ERR_PARAM | Invalid parameter value |

**Lambda — reading error:**

```cpp
uint8_t reg = 0x02;
uint8_t error_code = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &error_code, 1) == i2c::ERROR_OK) {
        switch (error_code) {
            case 0x00: ESP_LOGD("dimmerlink", "No error"); break;
            case 0xF9: ESP_LOGW("dimmerlink", "ERR_SYNTAX"); break;
            case 0xFC: ESP_LOGW("dimmerlink", "ERR_NOT_READY"); break;
            case 0xFD: ESP_LOGW("dimmerlink", "ERR_INDEX"); break;
            case 0xFE: ESP_LOGW("dimmerlink", "ERR_PARAM"); break;
            default:   ESP_LOGE("dimmerlink", "Unknown error: 0x%02X", error_code);
        }
    }
}
```

**ESPHome Entity — Text Sensor:**

```yaml
text_sensor:
  - platform: template
    name: "DimmerLink Last Error"
    id: dimmerlink_last_error
    icon: "mdi:alert"
    entity_category: "diagnostic"
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error_code = 0;

      if (id(bus_a).write(0x50, &reg, 1, false) != i2c::ERROR_OK) {
        return {"I2C Error"};
      }
      if (id(bus_a).read(0x50, &error_code, 1) != i2c::ERROR_OK) {
        return {"Read Error"};
      }

      switch (error_code) {
        case 0x00: return {"OK"};
        case 0xF9: return {"ERR_SYNTAX"};
        case 0xFC: return {"ERR_NOT_READY"};
        case 0xFD: return {"ERR_INDEX"};
        case 0xFE: return {"ERR_PARAM"};
        default:   return {"UNKNOWN (0x" + format_hex_pretty(&error_code, 1) + ")"};
      }
```

**ESPHome Entity — Sensor (numeric code):**

```yaml
sensor:
  - platform: template
    name: "DimmerLink Error Code"
    id: dimmerlink_error_code
    icon: "mdi:alert-octagon"
    entity_category: "diagnostic"
    accuracy_decimals: 0
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error_code = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &error_code, 1) == i2c::ERROR_OK) {
          return (float)error_code;
        }
      }
      return {};
```

---

### VERSION (0x03) — Firmware Version

| Parameter | Value |
|-----------|-------|
| **Address** | `0x03` |
| **Access** | Read-only |
| **Size** | 1 byte |

**Version Format:**
- `0x01` = v1.0
- `0x02` = v2.0
- `0x10` = v1.0 (alternative format, if major.minor)

**Lambda — reading version:**

```cpp
uint8_t reg = 0x03;
uint8_t version = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &version, 1) == i2c::ERROR_OK) {
        ESP_LOGI("dimmerlink", "Firmware version: %d", version);
    }
}
```

**ESPHome Entity — Sensor:**

```yaml
sensor:
  - platform: template
    name: "DimmerLink Firmware Version"
    id: dimmerlink_version
    icon: "mdi:chip"
    entity_category: "diagnostic"
    accuracy_decimals: 0
    update_interval: never  # Read once on startup
    lambda: |-
      uint8_t reg = 0x03;
      uint8_t version = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &version, 1) == i2c::ERROR_OK) {
          return (float)version;
        }
      }
      return {};

# Read on startup
esphome:
  on_boot:
    priority: -100
    then:
      - component.update: dimmerlink_version
```

---

## 4.3 Dimmer Registers (0x10-0x1F)

Dimmer channel control registers.

> [!NOTE]
> **Current version:** 1 channel (DIM0) is supported. DIM1-DIM3 registers are reserved for future versions.

### DIM0_LEVEL (0x10) — Brightness Level

| Parameter | Value |
|-----------|-------|
| **Address** | `0x10` |
| **Access** | Read/Write |
| **Size** | 1 byte |
| **Range** | 0-100 (%) |

**Values:**
- `0` — off (TRIAC does not open)
- `1-99` — dimming (phase-cut)
- `100` — full brightness (minimum delay)

> [!WARNING]
> Writing a value > 100 will return error `ERR_PARAM (0xFE)`.

**Lambda — writing level:**

```cpp
// Set brightness 0-100%
uint8_t level = 50;  // 50%

if (level > 100) level = 100;  // Overflow protection

uint8_t data[2] = {0x10, level};
auto err = id(bus_a).write(0x50, data, 2, true);

if (err == i2c::ERROR_OK) {
    ESP_LOGD("dimmerlink", "Level set to %d%%", level);
} else {
    ESP_LOGW("dimmerlink", "Failed to set level: %d", err);
}
```

**Lambda — reading level:**

```cpp
uint8_t reg = 0x10;
uint8_t level = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &level, 1) == i2c::ERROR_OK) {
        ESP_LOGD("dimmerlink", "Current level: %d%%", level);
        return (float)level;
    }
}
return {};  // Error
```

**ESPHome Entity — Output (basis for Light):**

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          // state: 0.0 - 1.0, convert to 0-100
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;

          uint8_t data[2] = {0x10, level};
          auto err = id(bus_a).write(0x50, data, 2, true);

          if (err == i2c::ERROR_OK) {
              ESP_LOGD("dimmerlink", "Set level: %d%%", level);
          } else {
              ESP_LOGW("dimmerlink", "Write error: %d", err);
          }
```

**ESPHome Entity — Light:**

```yaml
light:
  - platform: monochromatic
    name: "Dimmer"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0  # DimmerLink has its own curves
```

**ESPHome Entity — Number (direct control):**

```yaml
number:
  - platform: template
    name: "Dimmer Level"
    id: dimmer_level_number
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    min_value: 0
    max_value: 100
    step: 1
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t level = (uint8_t)x;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Level set to %d%%", level);
```

**ESPHome Entity — Sensor (readback):**

```yaml
sensor:
  - platform: template
    name: "Dimmer Level Readback"
    id: dimmer_level_sensor
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    accuracy_decimals: 0
    update_interval: 5s
    lambda: |-
      uint8_t reg = 0x10;
      uint8_t level = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &level, 1) == i2c::ERROR_OK) {
          return (float)level;
        }
      }
      return {};
```

---

### DIM0_CURVE (0x11) — Dimming Curve

| Parameter | Value |
|-----------|-------|
| **Address** | `0x11` |
| **Access** | Read/Write |
| **Size** | 1 byte |
| **Range** | 0-2 |

**Curve Types:**

| Value | Name | Description | Application |
|-------|------|-------------|-------------|
| `0` | LINEAR | Linear dependency | Universal |
| `1` | RMS | Quadratic (RMS) | Incandescent lamps, halogen |
| `2` | LOG | Logarithmic | LED (matches eye perception) |

> [!NOTE]
> Changing the curve immediately affects brightness at the current level.

**Lambda — setting curve:**

```cpp
// Set curve
// 0 = LINEAR, 1 = RMS, 2 = LOG
uint8_t curve = 1;  // RMS

if (curve > 2) {
    ESP_LOGW("dimmerlink", "Invalid curve: %d", curve);
    return;
}

uint8_t data[2] = {0x11, curve};
id(bus_a).write(0x50, data, 2, true);
ESP_LOGI("dimmerlink", "Curve set to %d", curve);
```

**Lambda — reading curve:**

```cpp
uint8_t reg = 0x11;
uint8_t curve = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &curve, 1) == i2c::ERROR_OK) {
        const char* names[] = {"LINEAR", "RMS", "LOG"};
        if (curve <= 2) {
            ESP_LOGI("dimmerlink", "Current curve: %s", names[curve]);
        }
        return (float)curve;
    }
}
return {};
```

**ESPHome Entity — Select:**

```yaml
select:
  - platform: template
    name: "Dimming Curve"
    id: dimming_curve
    icon: "mdi:chart-bell-curve"
    options:
      - "LINEAR"
      - "RMS"
      - "LOG"
    initial_option: "LINEAR"
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;

          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Curve set to: %s (%d)", x.c_str(), curve);

# Read current curve on startup
esphome:
  on_boot:
    priority: -100
    then:
      - lambda: |-
          uint8_t reg = 0x11;
          uint8_t curve = 0;
          if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
            if (id(bus_a).read(0x50, &curve, 1) == i2c::ERROR_OK) {
              auto call = id(dimming_curve).make_call();
              switch (curve) {
                case 0: call.set_option("LINEAR"); break;
                case 1: call.set_option("RMS"); break;
                case 2: call.set_option("LOG"); break;
              }
              call.perform();
            }
          }
```

**ESPHome Entity — Sensor (numeric value):**

```yaml
sensor:
  - platform: template
    name: "Dimming Curve Value"
    id: dimming_curve_value
    icon: "mdi:chart-bell-curve"
    entity_category: "diagnostic"
    accuracy_decimals: 0
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x11;
      uint8_t curve = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &curve, 1) == i2c::ERROR_OK) {
          return (float)curve;
        }
      }
      return {};
```

---

### Reserved Registers (0x12-0x1F)

| Address | Name | Description |
|---------|------|-------------|
| `0x12` | DIM1_LEVEL | Channel 1 — reserved |
| `0x13` | DIM1_CURVE | Channel 1 — reserved |
| `0x14` | DIM2_LEVEL | Channel 2 — reserved |
| `0x15` | DIM2_CURVE | Channel 2 — reserved |
| `0x16` | DIM3_LEVEL | Channel 3 — reserved |
| `0x17` | DIM3_CURVE | Channel 3 — reserved |
| `0x18-0x1F` | — | Reserved |

> [!NOTE]
> Future versions of DimmerLink may support up to 4 dimming channels.

---

## 4.4 System Registers (0x20-0x2F)

System information and calibration parameters.

### AC_FREQ (0x20) — AC Frequency

| Parameter | Value |
|-----------|-------|
| **Address** | `0x20` |
| **Access** | Read-only |
| **Size** | 1 byte |
| **Values** | 50 or 60 (Hz) |

**Lambda — reading frequency:**

```cpp
uint8_t reg = 0x20;
uint8_t freq = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &freq, 1) == i2c::ERROR_OK) {
        if (freq == 50 || freq == 60) {
            ESP_LOGI("dimmerlink", "AC Frequency: %d Hz", freq);
            return (float)freq;
        } else {
            ESP_LOGW("dimmerlink", "Invalid frequency: %d", freq);
        }
    }
}
return {};
```

**ESPHome Entity — Sensor:**

```yaml
sensor:
  - platform: template
    name: "AC Frequency"
    id: ac_frequency
    icon: "mdi:sine-wave"
    unit_of_measurement: "Hz"
    accuracy_decimals: 0
    device_class: frequency
    state_class: measurement
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x20;
      uint8_t freq = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &freq, 1) == i2c::ERROR_OK) {
          if (freq == 50 || freq == 60) {
            return (float)freq;
          }
        }
      }
      return {};
```

---

### AC_PERIOD_L / AC_PERIOD_H (0x21-0x22) — Half-Wave Period

| Parameter | Value |
|-----------|-------|
| **Address** | `0x21` (L), `0x22` (H) |
| **Access** | Read-only |
| **Size** | 2 bytes (16-bit) |
| **Units** | microseconds (us) |

**Expected Values:**
- 50 Hz: ~10000 us (9000-11000)
- 60 Hz: ~8333 us (7500-9000)

**Lambda — reading period:**

```cpp
uint8_t reg = 0x21;
uint8_t buffer[2] = {0, 0};

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, buffer, 2) == i2c::ERROR_OK) {
        uint16_t period_us = (buffer[1] << 8) | buffer[0];  // Little-endian
        ESP_LOGI("dimmerlink", "AC Half-period: %d us", period_us);
        return (float)period_us;
    }
}
return {};
```

**ESPHome Entity — Sensor:**

```yaml
sensor:
  - platform: template
    name: "AC Half-Period"
    id: ac_period
    icon: "mdi:timer-outline"
    unit_of_measurement: "us"
    accuracy_decimals: 0
    entity_category: "diagnostic"
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x21;
      uint8_t buffer[2] = {0, 0};
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, buffer, 2) == i2c::ERROR_OK) {
          uint16_t period = (buffer[1] << 8) | buffer[0];
          return (float)period;
        }
      }
      return {};
```

---

### CALIBRATION (0x23) — Calibration Status

| Parameter | Value |
|-----------|-------|
| **Address** | `0x23` |
| **Access** | Read-only |
| **Size** | 1 byte |

**Values:**
- `0` — Calibration in progress
- `1` — Calibration complete

**Lambda — checking calibration:**

```cpp
uint8_t reg = 0x23;
uint8_t cal_status = 0;

if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(0x50, &cal_status, 1) == i2c::ERROR_OK) {
        bool calibrated = (cal_status == 1);
        ESP_LOGI("dimmerlink", "Calibration: %s", calibrated ? "done" : "in progress");
        return calibrated;
    }
}
return {};
```

**ESPHome Entity — Binary Sensor:**

```yaml
binary_sensor:
  - platform: template
    name: "DimmerLink Calibrated"
    id: dimmerlink_calibrated
    icon: "mdi:tune"
    entity_category: "diagnostic"
    lambda: |-
      uint8_t reg = 0x23;
      uint8_t status = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
          return status == 1;
        }
      }
      return {};
```

---

## 4.5 Configuration Registers (0x30-0x3F)

Device configuration registers.

### I2C_ADDRESS (0x30) — Device I2C Address

| Parameter | Value |
|-----------|-------|
| **Address** | `0x30` |
| **Access** | Read/Write |
| **Size** | 1 byte |
| **Range** | 0x08-0x77 |
| **Default** | 0x50 |

> [!IMPORTANT]
> After writing a new address, the device **immediately** starts responding at the new address. The old address stops working. The change is saved to EEPROM.

**Prohibited Addresses:**
- `0x00-0x07` — reserved by I2C
- `0x78-0x7F` — reserved by I2C

**Lambda — reading current address:**

```cpp
// Assuming the address is known (default 0x50)
uint8_t current_addr = 0x50;
uint8_t reg = 0x30;
uint8_t addr = 0;

if (id(bus_a).write(current_addr, &reg, 1, false) == i2c::ERROR_OK) {
    if (id(bus_a).read(current_addr, &addr, 1) == i2c::ERROR_OK) {
        ESP_LOGI("dimmerlink", "Current I2C address: 0x%02X", addr);
    }
}
```

**Lambda — changing address:**

```cpp
// After this operation the device will only respond at the new address!
uint8_t old_addr = 0x50;
uint8_t new_addr = 0x51;

// Range check
if (new_addr < 0x08 || new_addr > 0x77) {
    ESP_LOGE("dimmerlink", "Invalid address: 0x%02X (must be 0x08-0x77)", new_addr);
    return;
}

uint8_t data[2] = {0x30, new_addr};
auto err = id(bus_a).write(old_addr, data, 2, true);

if (err == i2c::ERROR_OK) {
    ESP_LOGW("dimmerlink", "Address changed from 0x%02X to 0x%02X", old_addr, new_addr);
    ESP_LOGW("dimmerlink", "Device now responds ONLY at new address!");
} else {
    ESP_LOGE("dimmerlink", "Failed to change address: %d", err);
}
```

> [!WARNING]
> After changing the address, you need to update your ESPHome configuration!

**ESPHome Entity — Number (for changing address):**

```yaml
# Dangerous operation! After changing you need to reconfigure everything.
number:
  - platform: template
    name: "DimmerLink I2C Address"
    id: dimmerlink_i2c_address
    icon: "mdi:identifier"
    entity_category: "config"
    min_value: 8      # 0x08
    max_value: 119    # 0x77
    step: 1
    mode: box
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t new_addr = (uint8_t)x;

          if (new_addr < 0x08 || new_addr > 0x77) {
              ESP_LOGE("dimmerlink", "Invalid address: 0x%02X", new_addr);
              return;
          }

          uint8_t data[2] = {0x30, new_addr};
          // Use current address (default 0x50)
          id(bus_a).write(0x50, data, 2, true);

          ESP_LOGW("dimmerlink", "Address changed to 0x%02X", new_addr);
          ESP_LOGW("dimmerlink", "UPDATE YOUR CONFIG AND REBOOT!");
```

**ESPHome — Multiple Device Support:**

```yaml
substitutions:
  dimmer1_addr: "0x50"
  dimmer2_addr: "0x51"

output:
  - platform: template
    id: dimmer1_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer1_addr}, data, 2, true);

  - platform: template
    id: dimmer2_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer2_addr}, data, 2, true);

light:
  - platform: monochromatic
    name: "Dimmer 1"
    output: dimmer1_output

  - platform: monochromatic
    name: "Dimmer 2"
    output: dimmer2_output
```

---

## 4.6 Reference Tables

### Complete Register Map

| Address | Name | R/W | Size | Range | Description |
|---------|------|-----|------|-------|-------------|
| `0x00` | STATUS | R | 1 | bitmap | Device status |
| `0x01` | COMMAND | W | 1 | 0-3 | Control commands |
| `0x02` | ERROR | R | 1 | enum | Last error code |
| `0x03` | VERSION | R | 1 | — | Firmware version |
| `0x04-0x0F` | — | — | — | — | Reserved |
| `0x10` | DIM0_LEVEL | R/W | 1 | 0-100 | Channel 0 brightness (%) |
| `0x11` | DIM0_CURVE | R/W | 1 | 0-2 | Channel 0 curve |
| `0x12-0x1F` | DIMx | — | — | — | Reserved (channels 1-3) |
| `0x20` | AC_FREQ | R | 1 | 50/60 | AC frequency (Hz) |
| `0x21` | AC_PERIOD_L | R | 1 | — | Period, low byte (us) |
| `0x22` | AC_PERIOD_H | R | 1 | — | Period, high byte |
| `0x23` | CALIBRATION | R | 1 | 0-1 | Calibration status |
| `0x24-0x2F` | — | — | — | — | Reserved |
| `0x30` | I2C_ADDRESS | R/W | 1 | 0x08-0x77 | Device I2C address |
| `0x31-0x3F` | — | — | — | — | Reserved |

### Command Codes (Register 0x01)

| Code | Command | Description |
|------|---------|-------------|
| `0x00` | NOP | No operation |
| `0x01` | RESET | Software reset (~3 sec) |
| `0x02` | RECALIBRATE | Frequency recalibration (~200 ms) |
| `0x03` | SWITCH_UART | Switch to UART mode |

### Error Codes (Register 0x02)

| Code | Name | Description |
|------|------|-------------|
| `0x00` | OK | No error |
| `0xF9` | ERR_SYNTAX | Invalid register address |
| `0xFC` | ERR_NOT_READY | Device not ready (not calibrated or FLASH error) |
| `0xFD` | ERR_INDEX | Invalid channel index |
| `0xFE` | ERR_PARAM | Invalid parameter value |

### Curve Types (Register 0x11)

| Code | Name | Formula | Application |
|------|------|---------|-------------|
| `0` | LINEAR | out = in | Universal |
| `1` | RMS | out = in^2 | Incandescent lamps |
| `2` | LOG | out = log(in) | LED lamps |

---

## Changelog

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial version |

---

[<- Back: Examples](./03_examples.md) | [Contents](./README.md) | [Next: Troubleshooting ->](./Troubleshooting.md)
