[<- Entities](./02_entities.md) | [Contents](./README.md) | [Next: Lambda Reference ->](./04_lambda_reference.md)

---

# ESPHome Integration - Complete Examples

Ready-to-use configurations for copying. Choose the appropriate option and adapt to your needs.

---

## Contents

- [7.1 Minimal](#71-minimal) - Light only, 50 lines
- [7.2 Standard](#72-standard) - Light + sensors + curve
- [7.3 Extended](#73-extended) - full diagnostics
- [7.4 Multi-device](#74-multi-device) - multiple dimmers
- [7.5 With Physical Button](#75-with-physical-button) - local control
- [7.6 Production](#76-production) - for real-world use

---

## 7.1 Minimal

The simplest option. Dimmer in Home Assistant only.

**What you get:**

- Dimmable light in HA
- Smooth transitions
- No diagnostics
- No curve selection

```yaml
# ============================================================
# DimmerLink Minimal Example
# ============================================================
# Light only - minimum code, maximum simplicity
# ============================================================

substitutions:
  device_name: "dimmerlink"
  friendly_name: "Dimmer"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
api:
  encryption:
    key: !secret api_key

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "${device_name}-AP"
    password: "12345678"

captive_portal:

# --- I2C Bus ---
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  id: bus_a

# --- Output ---
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

# --- Light ---
light:
  - platform: monochromatic
    name: "Light"
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF
```

**secrets.yaml:**

```yaml
wifi_ssid: "YOUR_WIFI"
wifi_password: "YOUR_PASSWORD"
api_key: "YOUR_API_KEY"
ota_password: "YOUR_OTA_PASSWORD"
```

---

## 7.2 Standard

Recommended option for most users.

**What you get:**

- Dimmable light
- Dimming curve selection
- AC frequency
- Current level (readback)
- Device status
- Restart button

```yaml
# ============================================================
# DimmerLink Standard Example
# ============================================================
# Recommended configuration for most use cases
# ============================================================

substitutions:
  device_name: "dimmerlink"
  friendly_name: "DimmerLink"
  dimmerlink_addr: "0x50"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}
  on_boot:
    priority: -100
    then:
      - delay: 2s
      - lambda: |-
          // Sync curve on startup
          uint8_t reg = 0x11;
          uint8_t curve = 0;
          if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
            if (id(bus_a).read(${dimmerlink_addr}, &curve, 1) == i2c::ERROR_OK) {
              auto call = id(dimming_curve).make_call();
              switch (curve) {
                case 0: call.set_option("LINEAR"); break;
                case 1: call.set_option("RMS"); break;
                case 2: call.set_option("LOG"); break;
              }
              call.perform();
              ESP_LOGI("dimmerlink", "Synced curve: %d", curve);
            }
          }
      - component.update: ac_frequency

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
  level: INFO

api:
  encryption:
    key: !secret api_key

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
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
# Output + Light
# ============================================================
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

light:
  - platform: monochromatic
    name: "Light"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF

# ============================================================
# Sensors
# ============================================================
sensor:
  # AC frequency
  - platform: template
    name: "AC Frequency"
    id: ac_frequency
    icon: "mdi:sine-wave"
    unit_of_measurement: "Hz"
    accuracy_decimals: 0
    device_class: frequency
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x20;
      uint8_t freq = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &freq, 1) == i2c::ERROR_OK) {
          if (freq == 50 || freq == 60) return (float)freq;
        }
      }
      return {};

  # Current level
  - platform: template
    name: "Level"
    id: dimmer_level
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    accuracy_decimals: 0
    update_interval: 5s
    lambda: |-
      uint8_t reg = 0x10;
      uint8_t level = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &level, 1) == i2c::ERROR_OK) {
          return (float)level;
        }
      }
      return {};

  # WiFi signal
  - platform: wifi_signal
    name: "WiFi Signal"
    update_interval: 60s
    entity_category: diagnostic

# ============================================================
# Binary Sensors
# ============================================================
binary_sensor:
  # Device ready
  - platform: template
    name: "Ready"
    id: dimmerlink_ready
    icon: "mdi:check-circle"
    device_class: running
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x01) != 0;
        }
      }
      return {};

  # Connection status
  - platform: status
    name: "Status"
    entity_category: diagnostic

# ============================================================
# Select - Curve selection
# ============================================================
select:
  - platform: template
    name: "Curve"
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
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          ESP_LOGI("dimmerlink", "Curve: %s", x.c_str());

# ============================================================
# Buttons
# ============================================================
button:
  # Restart ESP
  - platform: restart
    name: "Restart ESP"
    icon: "mdi:restart"
    entity_category: config
```

---

## 7.3 Extended

Full diagnostics and all capabilities.

**What you get:**

- Everything from standard
- Firmware version
- Error codes
- Calibration status
- AC period
- Reset / Recalibrate / Identify buttons
- Text descriptions

```yaml
# ============================================================
# DimmerLink Extended Example
# ============================================================
# Full diagnostics and all capabilities
# ============================================================

substitutions:
  device_name: "dimmerlink"
  friendly_name: "DimmerLink"
  dimmerlink_addr: "0x50"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}
  on_boot:
    priority: -100
    then:
      - delay: 2s
      - lambda: |-
          ESP_LOGI("dimmerlink", "=== Initializing DimmerLink ===");

          // Check readiness
          uint8_t reg = 0x00;
          uint8_t status = 0;
          id(bus_a).write(${dimmerlink_addr}, &reg, 1, false);
          id(bus_a).read(${dimmerlink_addr}, &status, 1);

          if (!(status & 0x01)) {
            ESP_LOGW("dimmerlink", "Device not ready!");
            return;
          }

          // Version
          reg = 0x03;
          uint8_t version = 0;
          id(bus_a).write(${dimmerlink_addr}, &reg, 1, false);
          id(bus_a).read(${dimmerlink_addr}, &version, 1);
          ESP_LOGI("dimmerlink", "Firmware: v%d", version);

          // Frequency
          reg = 0x20;
          uint8_t freq = 0;
          id(bus_a).write(${dimmerlink_addr}, &reg, 1, false);
          id(bus_a).read(${dimmerlink_addr}, &freq, 1);
          ESP_LOGI("dimmerlink", "AC: %d Hz", freq);

          // Sync curve
          reg = 0x11;
          uint8_t curve = 0;
          id(bus_a).write(${dimmerlink_addr}, &reg, 1, false);
          id(bus_a).read(${dimmerlink_addr}, &curve, 1);

          auto call = id(dimming_curve).make_call();
          switch (curve) {
            case 0: call.set_option("LINEAR"); break;
            case 1: call.set_option("RMS"); break;
            case 2: call.set_option("LOG"); break;
          }
          call.perform();

          ESP_LOGI("dimmerlink", "=== Ready ===");

      - component.update: ac_frequency
      - component.update: firmware_version
      - component.update: ac_period

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
  level: DEBUG

api:
  encryption:
    key: !secret api_key

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
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
# Output + Light
# ============================================================
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          auto err = id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          if (err == i2c::ERROR_OK) {
            ESP_LOGD("dimmerlink", "Level: %d%%", level);
          } else {
            ESP_LOGW("dimmerlink", "Write error: %d", err);
          }

light:
  - platform: monochromatic
    name: "Light"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF
    effects:
      - pulse:
          name: "Pulse"
          transition_length: 1s
          update_interval: 2s

# ============================================================
# Sensors
# ============================================================
sensor:
  # AC frequency
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
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &freq, 1) == i2c::ERROR_OK) {
          if (freq == 50 || freq == 60) return (float)freq;
        }
      }
      return {};

  # Current level
  - platform: template
    name: "Level"
    id: dimmer_level
    icon: "mdi:brightness-percent"
    unit_of_measurement: "%"
    accuracy_decimals: 0
    state_class: measurement
    update_interval: 5s
    lambda: |-
      uint8_t reg = 0x10;
      uint8_t level = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &level, 1) == i2c::ERROR_OK) {
          return (float)level;
        }
      }
      return {};

  # AC period (us)
  - platform: template
    name: "AC Period"
    id: ac_period
    icon: "mdi:timer-outline"
    unit_of_measurement: "us"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x21;
      uint8_t buffer[2] = {0, 0};
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, buffer, 2) == i2c::ERROR_OK) {
          uint16_t period = (buffer[1] << 8) | buffer[0];
          return (float)period;
        }
      }
      return {};

  # Firmware version
  - platform: template
    name: "Firmware Version"
    id: firmware_version
    icon: "mdi:chip"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: never
    lambda: |-
      uint8_t reg = 0x03;
      uint8_t version = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &version, 1) == i2c::ERROR_OK) {
          return (float)version;
        }
      }
      return {};

  # Error code
  - platform: template
    name: "Error Code"
    id: error_code
    icon: "mdi:alert-octagon"
    accuracy_decimals: 0
    entity_category: diagnostic
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &error, 1) == i2c::ERROR_OK) {
          return (float)error;
        }
      }
      return {};

  # WiFi
  - platform: wifi_signal
    name: "WiFi Signal"
    update_interval: 60s
    entity_category: diagnostic

  # Uptime
  - platform: uptime
    name: "Uptime"
    entity_category: diagnostic

# ============================================================
# Binary Sensors
# ============================================================
binary_sensor:
  # Device ready
  - platform: template
    name: "Ready"
    id: dimmerlink_ready
    icon: "mdi:check-circle"
    device_class: running
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x01) != 0;
        }
      }
      return {};

  # Error flag
  - platform: template
    name: "Error Flag"
    id: dimmerlink_error
    icon: "mdi:alert-circle"
    device_class: problem
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x00;
      uint8_t status = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &status, 1) == i2c::ERROR_OK) {
          return (status & 0x02) != 0;
        }
      }
      return {};

  # Calibration
  - platform: template
    name: "Calibrated"
    id: dimmerlink_calibrated
    icon: "mdi:tune"
    entity_category: diagnostic
    lambda: |-
      uint8_t reg = 0x23;
      uint8_t status = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &status, 1) == i2c::ERROR_OK) {
          return status == 1;
        }
      }
      return {};

  # Connection status
  - platform: status
    name: "Status"
    entity_category: diagnostic

# ============================================================
# Text Sensors
# ============================================================
text_sensor:
  # Error description
  - platform: template
    name: "Last Error"
    id: last_error_text
    icon: "mdi:alert"
    entity_category: diagnostic
    update_interval: 10s
    lambda: |-
      uint8_t reg = 0x02;
      uint8_t error = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) != i2c::ERROR_OK) return {"I2C Error"};
      if (id(bus_a).read(${dimmerlink_addr}, &error, 1) != i2c::ERROR_OK) return {"Read Error"};
      switch (error) {
        case 0x00: return {"OK"};
        case 0xF9: return {"ERR_SYNTAX"};
        case 0xFC: return {"ERR_NOT_READY"};
        case 0xFD: return {"ERR_INDEX"};
        case 0xFE: return {"ERR_PARAM"};
        default:   return {"UNKNOWN"};
      }

  # Current curve (text)
  - platform: template
    name: "Current Curve"
    id: current_curve_text
    icon: "mdi:chart-bell-curve"
    entity_category: diagnostic
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x11;
      uint8_t curve = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) != i2c::ERROR_OK) return {"I2C Error"};
      if (id(bus_a).read(${dimmerlink_addr}, &curve, 1) != i2c::ERROR_OK) return {"Read Error"};
      switch (curve) {
        case 0: return {"LINEAR"};
        case 1: return {"RMS"};
        case 2: return {"LOG"};
        default: return {"UNKNOWN"};
      }

  # ESPHome version
  - platform: version
    name: "ESPHome Version"
    entity_category: diagnostic

  # WiFi info
  - platform: wifi_info
    ip_address:
      name: "IP Address"
      entity_category: diagnostic
    ssid:
      name: "WiFi SSID"
      entity_category: diagnostic

# ============================================================
# Select
# ============================================================
select:
  - platform: template
    name: "Curve"
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
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          ESP_LOGI("dimmerlink", "Curve: %s", x.c_str());
      - component.update: current_curve_text

# ============================================================
# Buttons
# ============================================================
button:
  # Reset DimmerLink
  - platform: template
    name: "Reset DimmerLink"
    id: dimmer_reset
    icon: "mdi:restart"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x01};
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          ESP_LOGW("dimmerlink", "Reset command sent");
      - delay: 3s
      - component.update: ac_frequency
      - component.update: firmware_version

  # Recalibrate
  - platform: template
    name: "Recalibrate"
    id: dimmer_recalibrate
    icon: "mdi:refresh"
    entity_category: config
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x02};
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);
          ESP_LOGI("dimmerlink", "Recalibration started");
      - delay: 500ms
      - component.update: ac_frequency
      - component.update: ac_period

  # Identify (blink)
  - platform: template
    name: "Identify"
    id: dimmer_identify
    icon: "mdi:lightbulb-alert"
    on_press:
      - repeat:
          count: 3
          then:
            - lambda: |-
                uint8_t on[2] = {0x10, 100};
                id(bus_a).write(${dimmerlink_addr}, on, 2, true);
            - delay: 300ms
            - lambda: |-
                uint8_t off[2] = {0x10, 0};
                id(bus_a).write(${dimmerlink_addr}, off, 2, true);
            - delay: 300ms

  # Restart ESP
  - platform: restart
    name: "Restart ESP"
    icon: "mdi:restart-alert"
    entity_category: config
```

---

## 7.4 Multi-device

Multiple DimmerLink devices on a single I2C bus.

**Requirements:**

- Each DimmerLink must have a unique address
- Change the address via register 0x30 before connecting

```yaml
# ============================================================
# DimmerLink Multi-Device Example
# ============================================================
# Three dimmers on a single I2C bus
# Addresses: 0x50, 0x51, 0x52
# ============================================================

substitutions:
  device_name: "dimmerlink-multi"
  friendly_name: "DimmerLink Multi"

  # Dimmer addresses
  dimmer1_addr: "0x50"
  dimmer2_addr: "0x51"
  dimmer3_addr: "0x52"

  # Names
  dimmer1_name: "Living Room"
  dimmer2_name: "Bedroom"
  dimmer3_name: "Kitchen"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
api:
  encryption:
    key: !secret api_key

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "${device_name}-AP"

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
# Outputs
# ============================================================
output:
  # Dimmer 1
  - platform: template
    id: dimmer1_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer1_addr}, data, 2, true);

  # Dimmer 2
  - platform: template
    id: dimmer2_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer2_addr}, data, 2, true);

  # Dimmer 3
  - platform: template
    id: dimmer3_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;
          uint8_t data[2] = {0x10, level};
          id(bus_a).write(${dimmer3_addr}, data, 2, true);

# ============================================================
# Lights
# ============================================================
light:
  - platform: monochromatic
    name: "${dimmer1_name}"
    id: light1
    output: dimmer1_output
    default_transition_length: 1s
    gamma_correct: 1.0

  - platform: monochromatic
    name: "${dimmer2_name}"
    id: light2
    output: dimmer2_output
    default_transition_length: 1s
    gamma_correct: 1.0

  - platform: monochromatic
    name: "${dimmer3_name}"
    id: light3
    output: dimmer3_output
    default_transition_length: 1s
    gamma_correct: 1.0

# ============================================================
# Curve Selects
# ============================================================
select:
  - platform: template
    name: "${dimmer1_name} Curve"
    id: curve1
    icon: "mdi:chart-bell-curve"
    options: ["LINEAR", "RMS", "LOG"]
    initial_option: "LINEAR"
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;
          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(${dimmer1_addr}, data, 2, true);

  - platform: template
    name: "${dimmer2_name} Curve"
    id: curve2
    icon: "mdi:chart-bell-curve"
    options: ["LINEAR", "RMS", "LOG"]
    initial_option: "LINEAR"
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;
          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(${dimmer2_addr}, data, 2, true);

  - platform: template
    name: "${dimmer3_name} Curve"
    id: curve3
    icon: "mdi:chart-bell-curve"
    options: ["LINEAR", "RMS", "LOG"]
    initial_option: "LINEAR"
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;
          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(${dimmer3_addr}, data, 2, true);

# ============================================================
# Sensors (once, frequency is the same)
# ============================================================
sensor:
  - platform: template
    name: "AC Frequency"
    icon: "mdi:sine-wave"
    unit_of_measurement: "Hz"
    accuracy_decimals: 0
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x20;
      uint8_t freq = 0;
      if (id(bus_a).write(${dimmer1_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmer1_addr}, &freq, 1) == i2c::ERROR_OK) {
          if (freq == 50 || freq == 60) return (float)freq;
        }
      }
      return {};

# ============================================================
# Buttons
# ============================================================
button:
  - platform: restart
    name: "Restart ESP"
    entity_category: config

  # All Off
  - platform: template
    name: "Turn Off All"
    icon: "mdi:lightbulb-off"
    on_press:
      - light.turn_off: light1
      - light.turn_off: light2
      - light.turn_off: light3

  # All On
  - platform: template
    name: "Turn On All"
    icon: "mdi:lightbulb-on"
    on_press:
      - light.turn_on:
          id: light1
          brightness: 100%
      - light.turn_on:
          id: light2
          brightness: 100%
      - light.turn_on:
          id: light3
          brightness: 100%
```

---

## 7.5 With Physical Button

Local control without Home Assistant.

```yaml
# ============================================================
# DimmerLink with Physical Button
# ============================================================
# GPIO0 - button (short press = on/off, long press = brightness)
# ============================================================

substitutions:
  device_name: "dimmerlink-button"
  friendly_name: "DimmerLink Button"
  dimmerlink_addr: "0x50"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
api:
  encryption:
    key: !secret api_key

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "${device_name}-AP"

captive_portal:

# ============================================================
# Globals
# ============================================================
globals:
  - id: dim_direction
    type: int
    initial_value: '1'

  - id: last_brightness
    type: float
    initial_value: '0.5'
    restore_value: true

# ============================================================
# I2C Bus
# ============================================================
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  id: bus_a

# ============================================================
# Output + Light
# ============================================================
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

light:
  - platform: monochromatic
    name: "Light"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 500ms
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF
    on_turn_on:
      - globals.set:
          id: last_brightness
          value: !lambda 'return id(dimmer_light).current_values.get_brightness();'
    on_turn_off:
      - globals.set:
          id: dim_direction
          value: '1'

# ============================================================
# Physical Button (GPIO0)
# ============================================================
binary_sensor:
  - platform: gpio
    pin:
      number: GPIO0
      mode: INPUT_PULLUP
      inverted: true
    id: physical_button
    name: "Button"

    # Short press - on/off
    on_click:
      min_length: 50ms
      max_length: 500ms
      then:
        - light.toggle: dimmer_light

    # Long press - brightness adjustment
    on_press:
      then:
        - delay: 500ms
        - while:
            condition:
              binary_sensor.is_on: physical_button
            then:
              - light.dim_relative:
                  id: dimmer_light
                  relative_brightness: !lambda 'return id(dim_direction) * 0.05;'
                  transition_length: 100ms
              - delay: 100ms

    on_release:
      then:
        - lambda: |-
            // Change direction for next time
            id(dim_direction) = -id(dim_direction);

  # Status
  - platform: status
    name: "Status"
    entity_category: diagnostic

# ============================================================
# Sensors
# ============================================================
sensor:
  - platform: template
    name: "AC Frequency"
    icon: "mdi:sine-wave"
    unit_of_measurement: "Hz"
    update_interval: 60s
    lambda: |-
      uint8_t reg = 0x20;
      uint8_t freq = 0;
      if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
        if (id(bus_a).read(${dimmerlink_addr}, &freq, 1) == i2c::ERROR_OK) {
          if (freq == 50 || freq == 60) return (float)freq;
        }
      }
      return {};

# ============================================================
# Select
# ============================================================
select:
  - platform: template
    name: "Curve"
    id: dimming_curve
    options: ["LINEAR", "RMS", "LOG"]
    initial_option: "LINEAR"
    optimistic: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;
          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);

# ============================================================
# Buttons
# ============================================================
button:
  - platform: restart
    name: "Restart"
    entity_category: config
```

**Button logic:**

- Short press (< 500ms) - turn on/off
- Long press (> 500ms) - smooth brightness adjustment
- Release - change direction (brighter <-> dimmer)

---

## 7.6 Production

Optimized configuration for real-world use.

**Features:**

- Minimal logging
- Protection against frequent writes
- Watchdog
- Safe mode

```yaml
# ============================================================
# DimmerLink Production Example
# ============================================================
# Optimized for stable operation
# ============================================================

substitutions:
  device_name: "dimmer-living-room"
  friendly_name: "Living Room Dimmer"
  dimmerlink_addr: "0x50"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}
  name_add_mac_suffix: false

  on_boot:
    priority: -100
    then:
      - delay: 3s
      - lambda: |-
          uint8_t reg = 0x11;
          uint8_t curve = 0;
          if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) == i2c::ERROR_OK) {
            if (id(bus_a).read(${dimmerlink_addr}, &curve, 1) == i2c::ERROR_OK) {
              auto call = id(dimming_curve).make_call();
              switch (curve) {
                case 0: call.set_option("LINEAR"); break;
                case 1: call.set_option("RMS"); break;
                case 2: call.set_option("LOG"); break;
              }
              call.perform();
            }
          }

esp32:
  board: esp32dev
  framework:
    type: arduino

# Minimal logging
logger:
  level: WARN
  logs:
    component: ERROR

# API with timeout
api:
  encryption:
    key: !secret api_key
  reboot_timeout: 15min

# OTA with password
ota:
  platform: esphome
  password: !secret ota_password
  safe_mode: true
  num_attempts: 5

# WiFi with fallback
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  fast_connect: true
  power_save_mode: LIGHT

  # Fallback access point
  ap:
    ssid: "${device_name}"
    password: !secret ap_password

captive_portal:

# Watchdog
interval:
  - interval: 5min
    then:
      - lambda: |-
          // Periodic check of DimmerLink communication
          uint8_t reg = 0x00;
          uint8_t status = 0;
          if (id(bus_a).write(${dimmerlink_addr}, &reg, 1, false) != i2c::ERROR_OK) {
            ESP_LOGW("watchdog", "DimmerLink communication error");
          }

# ============================================================
# I2C
# ============================================================
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: false  # Disable scan in production
  frequency: 100kHz
  id: bus_a

# ============================================================
# Globals - cache to reduce writes
# ============================================================
globals:
  - id: cached_level
    type: uint8_t
    initial_value: '0'
    restore_value: true

# ============================================================
# Output with protection against frequent writes
# ============================================================
output:
  - platform: template
    id: dimmerlink_output
    type: float
    write_action:
      - lambda: |-
          uint8_t level = (uint8_t)(state * 100.0f);
          if (level > 100) level = 100;

          // Only write if changed
          if (level != id(cached_level)) {
            uint8_t data[2] = {0x10, level};
            if (id(bus_a).write(${dimmerlink_addr}, data, 2, true) == i2c::ERROR_OK) {
              id(cached_level) = level;
            }
          }

# ============================================================
# Light
# ============================================================
light:
  - platform: monochromatic
    name: "Light"
    id: dimmer_light
    output: dimmerlink_output
    default_transition_length: 1s
    gamma_correct: 1.0
    restore_mode: RESTORE_DEFAULT_OFF

# ============================================================
# Sensors (minimal set)
# ============================================================
sensor:
  # WiFi for diagnostics
  - platform: wifi_signal
    name: "WiFi"
    update_interval: 300s
    entity_category: diagnostic

binary_sensor:
  # Connection status
  - platform: status
    name: "Status"
    entity_category: diagnostic

# ============================================================
# Select
# ============================================================
select:
  - platform: template
    name: "Curve"
    id: dimming_curve
    icon: "mdi:chart-bell-curve"
    options: ["LINEAR", "RMS", "LOG"]
    initial_option: "RMS"  # Default RMS for lamps
    optimistic: true
    restore_value: true
    set_action:
      - lambda: |-
          uint8_t curve = 0;
          if (x == "RMS") curve = 1;
          else if (x == "LOG") curve = 2;
          uint8_t data[2] = {0x11, curve};
          id(bus_a).write(${dimmerlink_addr}, data, 2, true);

# ============================================================
# Buttons
# ============================================================
button:
  - platform: restart
    name: "Restart"
    entity_category: config
    disabled_by_default: true
```

---

## Examples Summary Table

| Example | Lines | Entities | Purpose |
|---------|-------|----------|---------|
| Minimal | ~50 | 1 Light | Quick start |
| Standard | ~150 | 6 | Typical use |
| Extended | ~350 | 20+ | Full diagnostics |
| Multi-device | ~200 | 9 | Multiple dimmers |
| With button | ~180 | 5 | Local control |
| Production | ~170 | 4 | Stable operation |

---

## Recommendations for Choosing

```text
New to ESPHome?
  |-> Minimal (7.1)

Want to see status and change curve?
  |-> Standard (7.2)

Need full diagnostics?
  |-> Extended (7.3)

Multiple dimmers?
  |-> Multi-device (7.4)

Need a physical button?
  |-> With button (7.5)

For permanent installation?
  |-> Production (7.6)
```

---

## Change History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial version |

---

[<- Entities](./02_entities.md) | [Contents](./README.md) | [Next: Lambda Reference ->](./04_lambda_reference.md)
