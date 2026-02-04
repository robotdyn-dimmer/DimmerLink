[← Introduction](./01_introduction.md) | [Contents](./README.md) | [Next: Examples →](./03_examples.md)

---

# ESPHome Integration — Ready Entities

Ready-to-use code blocks to copy into your ESPHome configuration.

> [!TIP]
> **How to use:** Copy the needed blocks into your YAML file. All entities are independent — you can use any combination.

---

## Contents

- [Basic Configuration](#basic-configuration)
- [6.1 Light — Dimmable Light](#61-light--dimmable-light)
- [6.2 Output — Output for Light](#62-output--output-for-light)
- [6.3 Sensor — Sensors](#63-sensor--sensors)
- [6.4 Binary Sensor — Binary Sensors](#64-binary-sensor--binary-sensors)
- [6.5 Text Sensor — Text Sensors](#65-text-sensor--text-sensors)
- [6.6 Select — Curve Selection](#66-select--curve-selection)
- [6.7 Number — Numeric Control](#67-number--numeric-control)
- [6.8 Button — Control Buttons](#68-button--control-buttons)
- [6.9 Globals — Global Variables](#69-globals--global-variables)
- [6.10 Boot Initialization](#610-boot-initialization)

---

## Basic Configuration

Required part for all entities.

### Minimal

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz
  id: bus_a
```

### With substitutions (recommended)

```yaml
substitutions:
  device_name: "dimmerlink"
  friendly_name: "DimmerLink"
  dimmerlink_addr: "0x50"

i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz
  id: bus_a
```

> [!TIP]
> **Substitutions** allow easily changing parameters in one place. Use `${dimmerlink_addr}` in lambda instead of `0x50`.

---

## 6.1 Light — Dimmable Light

Main way to integrate with Home Assistant.

### Minimal

```yaml
light:
  - platform: monochromatic
    name: "Dimmer"
    output: dimmerlink_output
```

### Standard

```yaml
light:
  - platform: monochromatic
    name: "${friendly_name} Light"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
```

### Extended

```yaml
light:
  - platform: monochromatic
    name: "${friendly_name} Light"
    id: dimmer_light
    output: dimmerlink_output

    # Transitions
    default_transition_length: 1s

    # No gamma correction (DimmerLink has its own curves)
    gamma_correct: 1.0

    # Restore state after reboot
    restore_mode: RESTORE_DEFAULT_OFF

    # Effects (optional)
    effects:
      - pulse:
          name: "Pulse"
          transition_length: 1s
          update_interval: 2s
      - strobe:
          name: "Strobe"
          colors:
            - state: true
              duration: 500ms
            - state: false
              duration: 500ms
```

**Parameters:**

| Parameter | Description | Default |
|-----------|-------------|---------|
| `output` | Output component ID | Required |
| `default_transition_length` | Smooth transition time | 1s |
| `gamma_correct` | Gamma correction | 2.8 (set to 1.0) |
| `restore_mode` | Restore on startup | RESTORE_DEFAULT_OFF |

> [!IMPORTANT]
> **gamma_correct: 1.0** — important! DimmerLink has its own dimming curves. ESPHome gamma correction will interfere.

---

## 6.2 Output — Output for Light

Link between Light and I2C.

### Minimal

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);
```

### Standard (with logging)

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          // Convert 0.0-1.0 → 0-100%
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;

          // Write to DIM0_LEVEL register
          uint8_t data[2] = {0x10, level};
          auto err = id(bus_a).write(0x50, data, 2, true);

          if (err == i2c::ERROR_OK) {
              ESP_LOGD("dimmerlink", "Level: %d%%", level);
          } else {
              ESP_LOGW("dimmerlink", "Write error: %d", err);
          }
```

### With substitutions

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          ESP_LOGD("dimmerlink", "Level: %d%%", level);
```

### Multi-device (multiple dimmers)

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
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer1_addr}, data, 2, true);

  - platform: template
    id: dimmer2_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
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

## 6.3 Sensor — Sensors

Numeric values for monitoring.

### AC Frequency

```yaml
sensor:
  - platform: template
    name: "${friendly_name} AC Frequency"
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

### Dimmer Level — Current level (readback)

```yaml
sensor:
  - platform: template
    name: "${friendly_name} Level"
    id: dimmer_level
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    accuracy_decimals: 0
    state_class: measurement
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

### AC Period — Half-wave period

```yaml
sensor:
  - platform: template
    name: "${friendly_name} AC Period"
    id: ac_period
    icon: "mdi:timer-outline"
    unit_of_measurement: "μs"
    accuracy_decimals: 0
    entity_category: diagnostic
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

### Firmware Version

```yaml
sensor:
  - platform: template
    name: "${friendly_name} Firmware"
    id: firmware_version
    icon: "mdi:chip"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: never
    lambda: |-
      uint8_t reg = 0x03;
      uint8_t version = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &version, 1) == i2c::ERROR_OK) {
          return (float)version;
        }
      }
      return {};
```

### Error Code

```yaml
sensor:
  - platform: template
    name: "${friendly_name} Error Code"
    id: error_code
    icon: "mdi:alert-octagon"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &error, 1) == i2c::ERROR_OK) {
          return (float)error;
        }
      }
      return {};
```

### Curve Value — Current curve (numeric)

```yaml
sensor:
  - platform: template
    name: "${friendly_name} Curve Value"
    id: curve_value
    icon: "mdi:chart-bell-curve"
    accuracy_decimals: 0
    entity_category: diagnostic
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

### All sensors (copy whole block)

```yaml
sensor:
  # AC frequency
  - platform: template
    name: "${friendly_name} AC Frequency"
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
          if (freq == 50 || freq == 60) return (float)freq;
        }
      }
      return {};

  # Current level
  - platform: template
    name: "${friendly_name} Level"
    id: dimmer_level
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    accuracy_decimals: 0
    state_class: measurement
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

  # Firmware version
  - platform: template
    name: "${friendly_name} Firmware"
    id: firmware_version
    icon: "mdi:chip"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: never
    lambda: |-
      uint8_t reg = 0x03;
      uint8_t version = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &version, 1) == i2c::ERROR_OK) {
          return (float)version;
        }
      }
      return {};

  # WiFi signal (for diagnostics)
  - platform: wifi_signal
    name: "${friendly_name} WiFi Signal"
    update_interval: 60s
```

---

## 6.4 Binary Sensor — Binary Sensors

On/off states.

### Ready — Device ready

```yaml
binary_sensor:
  - platform: template
    name: "${friendly_name} Ready"
    id: dimmerlink_ready
    icon: "mdi:check-circle"
    device_class: running
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x01) != 0;
        }
      }
      return {};
```

### Error Flag

```yaml
binary_sensor:
  - platform: template
    name: "${friendly_name} Error"
    id: dimmerlink_error
    icon: "mdi:alert-circle"
    device_class: problem
    entity_category: diagnostic
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

### Calibrated — Calibration complete

```yaml
binary_sensor:
  - platform: template
    name: "${friendly_name} Calibrated"
    id: dimmerlink_calibrated
    icon: "mdi:tune"
    entity_category: diagnostic
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

### All binary sensors

```yaml
binary_sensor:
  # Device ready
  - platform: template
    name: "${friendly_name} Ready"
    id: dimmerlink_ready
    icon: "mdi:check-circle"
    device_class: running
    entity_category: diagnostic
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
    name: "${friendly_name} Error"
    id: dimmerlink_error
    icon: "mdi:alert-circle"
    device_class: problem
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(0x50, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x02) != 0;
        }
      }
      return {};

  # Calibration complete
  - platform: template
    name: "${friendly_name} Calibrated"
    id: dimmerlink_calibrated
    icon: "mdi:tune"
    entity_category: diagnostic
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

## 6.5 Text Sensor — Text Sensors

Text values for display.

### Last Error — Error description

```yaml
text_sensor:
  - platform: template
    name: "${friendly_name} Last Error"
    id: last_error_text
    icon: "mdi:alert"
    entity_category: diagnostic
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error = 0;

      if (id(bus_a).write(0x50, &reg, 1, false) != i2c::ERROR_OK) {
        return {"I2C Error"};
      }
      if (id(bus_a).read(0x50, &error, 1) != i2c::ERROR_OK) {
        return {"Read Error"};
      }

      switch (error) {
        case 0x00: return {"OK"};
        case 0xF9: return {"ERR_SYNTAX"};
        case 0xFC: return {"ERR_NOT_READY"};
        case 0xFD: return {"ERR_INDEX"};
        case 0xFE: return {"ERR_PARAM"};
        default:   return {"UNKNOWN"};
      }
```

> [!NOTE]
> **Error codes:**
> - `0x00` OK — Success
> - `0xF9` ERR_SYNTAX — Invalid register address
> - `0xFC` ERR_NOT_READY — Device not ready (not calibrated or FLASH error)
> - `0xFD` ERR_INDEX — Invalid dimmer index
> - `0xFE` ERR_PARAM — Invalid parameter (level>100, curve>2)

### Current Curve (text)

```yaml
text_sensor:
  - platform: template
    name: "${friendly_name} Current Curve"
    id: current_curve_text
    icon: "mdi:chart-bell-curve"
    entity_category: diagnostic
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x11;
      uint8_t curve = 0;

      if (id(bus_a).write(0x50, &reg, 1, false) != i2c::ERROR_OK) {
        return {"I2C Error"};
      }
      if (id(bus_a).read(0x50, &curve, 1) != i2c::ERROR_OK) {
        return {"Read Error"};
      }

      switch (curve) {
        case 0: return {"LINEAR"};
        case 1: return {"RMS"};
        case 2: return {"LOG"};
        default: return {"UNKNOWN"};
      }
```

### Device Info

```yaml
text_sensor:
  - platform: template
    name: "${friendly_name} Info"
    id: device_info
    icon: "mdi:information"
    entity_category: diagnostic
    update_interval: never
    lambda: |-
      uint8_t reg_ver = 0x03;
      uint8_t reg_freq = 0x20;
      uint8_t version = 0;
      uint8_t freq = 0;

      id(bus_a).write(0x50, &reg_ver, 1, false);
      id(bus_a).read(0x50, &version, 1);

      id(bus_a).write(0x50, &reg_freq, 1, false);
      id(bus_a).read(0x50, &freq, 1);

      char buf[32];
      snprintf(buf, sizeof(buf), "FW:%d, %dHz", version, freq);
      return {buf};
```

### All text sensors

```yaml
text_sensor:
  # Error description
  - platform: template
    name: "${friendly_name} Last Error"
    id: last_error_text
    icon: "mdi:alert"
    entity_category: diagnostic
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) != i2c::ERROR_OK) return {"I2C Error"};
      if (id(bus_a).read(0x50, &error, 1) != i2c::ERROR_OK) return {"Read Error"};
      switch (error) {
        case 0x00: return {"OK"};
        case 0xF9: return {"ERR_SYNTAX"};
        case 0xFC: return {"ERR_NOT_READY"};
        case 0xFD: return {"ERR_INDEX"};
        case 0xFE: return {"ERR_PARAM"};
        default:   return {"UNKNOWN"};
      }

  # Current curve
  - platform: template
    name: "${friendly_name} Curve"
    id: current_curve_text
    icon: "mdi:chart-bell-curve"
    entity_category: diagnostic
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x11;
      uint8_t curve = 0;
      if (id(bus_a).write(0x50, &reg, 1, false) != i2c::ERROR_OK) return {"I2C Error"};
      if (id(bus_a).read(0x50, &curve, 1) != i2c::ERROR_OK) return {"Read Error"};
      switch (curve) {
        case 0: return {"LINEAR"};
        case 1: return {"RMS"};
        case 2: return {"LOG"};
        default: return {"UNKNOWN"};
      }

  # ESPHome version
  - platform: version
    name: "${friendly_name} ESPHome Version"
```

---

## 6.6 Select — Curve Selection

Dropdown list for dimming curve selection.

### Standard

```yaml
select:
  - platform: template
    name: "${friendly_name} Curve"
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
          ESP_LOGI("dimmerlink", "Curve: %s", x.c_str());
```

### With reading current value on boot

```yaml
select:
  - platform: template
    name: "${friendly_name} Curve"
    id: dimming_curve
    icon: "mdi:chart-bell-curve"
    options:
      - "LINEAR"
      - "RMS"
      - "LOG"
    initial_option: "LINEAR"
    optimistic: true
    restore_value: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;

          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Curve: %s", x.c_str());

# Sync on boot
esphome:
  on_boot:
    priority: -100
    then:
      - delay: 2s
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

---

## 6.7 Number — Numeric Control

Direct level control without Light entity.

### Level — Brightness level

```yaml
number:
  - platform: template
    name: "${friendly_name} Level"
    id: dimmer_level_number
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    min_value: 0
    max_value: 100
    step: 1
    mode: slider
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t level = (uint8_t)x;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Level: %d%%", level);
```

### I2C Address — Change address

```yaml
number:
  - platform: template
    name: "${friendly_name} I2C Address"
    id: i2c_address_number
    icon: "mdi:identifier"
    entity_category: config
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
          id(bus_a).write(0x50, data, 2, true);

          ESP_LOGW("dimmerlink", "Address changed to 0x%02X!", new_addr);
          ESP_LOGW("dimmerlink", "Update config and reboot!");
```

> [!WARNING]
> **I2C Address:** After changing the address, you need to update `${dimmerlink_addr}` in configuration and reflash ESP32.

---

## 6.8 Button — Control Buttons

Actions on press.

### Reset — Software reset

```yaml
button:
  - platform: template
    name: "${friendly_name} Reset"
    id: dimmer_reset
    icon: "mdi:restart"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x01};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGW("dimmerlink", "Reset command sent");
      - delay: 3s
      - logger.log:
          level: INFO
          format: "DimmerLink should be online"
```

### Recalibrate

```yaml
button:
  - platform: template
    name: "${friendly_name} Recalibrate"
    id: dimmer_recalibrate
    icon: "mdi:refresh"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x02};
          id(bus_a).write(0x50, data, 2, true);
          ESP_LOGI("dimmerlink", "Recalibration started");
      - delay: 500ms
      - component.update: ac_frequency
```

### ESP32 Restart

```yaml
button:
  - platform: restart
    name: "${friendly_name} ESP Restart"
    icon: "mdi:restart-alert"
    entity_category: config
```

### Identify — Identification (blink)

```yaml
button:
  - platform: template
    name: "${friendly_name} Identify"
    id: dimmer_identify
    icon: "mdi:lightbulb-alert"
    on_press:
      - repeat:
          count: 3
          then:
            - lambda: |-
                uint8_t on[2] = {0x10, 100};
                id(bus_a).write(0x50, on, 2, true);
            - delay: 300ms
            - lambda: |-
                uint8_t off[2] = {0x10, 0};
                id(bus_a).write(0x50, off, 2, true);
            - delay: 300ms
```

### All buttons

```yaml
button:
  # Reset DimmerLink
  - platform: template
    name: "${friendly_name} Reset"
    id: dimmer_reset
    icon: "mdi:restart"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x01};
          id(bus_a).write(0x50, data, 2, true);
      - delay: 3s

  # Recalibrate
  - platform: template
    name: "${friendly_name} Recalibrate"
    id: dimmer_recalibrate
    icon: "mdi:refresh"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x02};
          id(bus_a).write(0x50, data, 2, true);
      - delay: 500ms

  # ESP32 restart
  - platform: restart
    name: "${friendly_name} ESP Restart"
    entity_category: config

  # Identify (blink)
  - platform: template
    name: "${friendly_name} Identify"
    id: dimmer_identify
    icon: "mdi:lightbulb-alert"
    on_press:
      - repeat:
          count: 3
          then:
            - lambda: |-
                uint8_t on[2] = {0x10, 100};
                id(bus_a).write(0x50, on, 2, true);
            - delay: 300ms
            - lambda: |-
                uint8_t off[2] = {0x10, 0};
                id(bus_a).write(0x50, off, 2, true);
            - delay: 300ms
```

---

## 6.9 Globals — Global Variables

For storing state between lambda calls.

```yaml
globals:
  # Current level (cache)
  - id: cached_level
    type: uint8_t
    initial_value: '0'
    restore_value: true

  # Current curve
  - id: cached_curve
    type: uint8_t
    initial_value: '0'
    restore_value: true

  # Initialization flag
  - id: initialized
    type: bool
    initial_value: 'false'
```

**Usage:**

```yaml
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;

          // Only send if value changed
          if (level != id(cached_level)) {
            uint8_t data[2] = {0x10, level};
            id(bus_a).write(0x50, data, 2, true);
            id(cached_level) = level;
          }
```

---

## 6.10 Boot Initialization

Reading DimmerLink state on ESP32 boot.

### Basic

```yaml
esphome:
  on_boot:
    priority: -100
    then:
      - delay: 2s  # Wait for DimmerLink calibration
      - lambda: |-
          ESP_LOGI("dimmerlink", "Reading initial state...");

          // Check ready status
          uint8_t reg_status = 0x00;
          uint8_t status = 0;
          id(bus_a).write(0x50, &reg_status, 1, false);
          id(bus_a).read(0x50, &status, 1);

          if (status & 0x01) {
            ESP_LOGI("dimmerlink", "Device ready");
          } else {
            ESP_LOGW("dimmerlink", "Device not ready!");
          }
```

### Full (with all entities sync)

```yaml
esphome:
  on_boot:
    priority: -100
    then:
      - delay: 2s
      - lambda: |-
          ESP_LOGI("dimmerlink", "=== Initializing DimmerLink ===");

          // 1. Check ready status
          uint8_t reg = 0x00;
          uint8_t status = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &status, 1);

          if (!(status & 0x01)) {
            ESP_LOGW("dimmerlink", "Device not ready, waiting...");
            return;
          }

          // 2. Read version
          reg = 0x03;
          uint8_t version = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &version, 1);
          ESP_LOGI("dimmerlink", "Firmware: %d", version);

          // 3. Read frequency
          reg = 0x20;
          uint8_t freq = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &freq, 1);
          ESP_LOGI("dimmerlink", "AC Frequency: %d Hz", freq);

          // 4. Read current curve and sync Select
          reg = 0x11;
          uint8_t curve = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &curve, 1);

          auto call = id(dimming_curve).make_call();
          switch (curve) {
            case 0: call.set_option("LINEAR"); break;
            case 1: call.set_option("RMS"); break;
            case 2: call.set_option("LOG"); break;
          }
          call.perform();
          ESP_LOGI("dimmerlink", "Curve: %d", curve);

          // 5. Read current level
          reg = 0x10;
          uint8_t level = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &level, 1);
          ESP_LOGI("dimmerlink", "Level: %d%%", level);

          ESP_LOGI("dimmerlink", "=== Initialization complete ===");

      # Update all sensors
      - component.update: ac_frequency
      - component.update: firmware_version
```

---

## entity_category Table

| Category | Display in HA | When to use |
|----------|---------------|-------------|
| (empty) | Main interface | Light, main sensors |
| `diagnostic` | Diagnostics | Version, status, errors |
| `config` | Configuration | Buttons, settings |

---

## Quick Start

Minimal working configuration — copy and it works:

```yaml
substitutions:
  device_name: "dimmerlink"
  friendly_name: "DimmerLink"

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
  scan: true
  id: bus_a

output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(0x50, data, 2, true);

light:
  - platform: monochromatic
    name: "${friendly_name}"
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release |

---

[← Introduction](./01_introduction.md) | [Contents](./README.md) | [Next: Examples →](./03_examples.md)
