[<- Back: Lambda Reference](./04_lambda_reference.md) | [Contents](./README.md)

---

# ESPHome Integration — Home Assistant, Troubleshooting, Next Steps

Integration with Home Assistant, troubleshooting, and development plans.

---

## Contents

- [5.1 Home Assistant Integration](#51-home-assistant-integration)
- [5.2 Troubleshooting](#52-troubleshooting)
- [5.3 Next Steps](#53-next-steps)

---

## 5.1 Home Assistant Integration

### Automatic Discovery

After flashing the ESP32, the device automatically appears in Home Assistant.

**Verification:**

1. Settings -> Devices & Services
2. Find the "New devices discovered" notification
3. Or: ESPHome -> select the device

### Entities in Home Assistant

When using the [Standard Example](./03_examples.md#72-standard), the following are created:

| Entity | Type | Description |
|--------|------|-------------|
| `light.dimmerlink_light` | Light | Main dimmer |
| `sensor.dimmerlink_ac_frequency` | Sensor | Mains frequency (50/60 Hz) |
| `sensor.dimmerlink_level` | Sensor | Current level (%) |
| `select.dimmerlink_curve` | Select | Dimming curve |
| `binary_sensor.dimmerlink_ready` | Binary Sensor | Ready status |
| `button.dimmerlink_restart_esp` | Button | ESP restart |

### Lovelace Card

#### Simple Card

```yaml
type: light
entity: light.dimmerlink_light
name: Living Room
```

#### Extended Card

```yaml
type: vertical-stack
cards:
  - type: light
    entity: light.dimmerlink_light
    name: Dimmer

  - type: entities
    entities:
      - entity: select.dimmerlink_curve
        name: Curve
      - entity: sensor.dimmerlink_ac_frequency
        name: Mains Frequency
      - entity: binary_sensor.dimmerlink_ready
        name: Status
```

#### Card with Slider (custom:slider-entity-row)

```yaml
type: entities
entities:
  - entity: light.dimmerlink_light
    type: custom:slider-entity-row
    name: Brightness
    toggle: true
```

> [!TIP]
> Requires installation of [slider-entity-row](https://github.com/thomasloven/lovelace-slider-entity-row) via HACS.

### Automations

#### Turn On by Motion Sensor

```yaml
automation:
  - alias: "Light on Motion"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor
        to: "on"
    condition:
      - condition: sun
        after: sunset
    action:
      - service: light.turn_on
        target:
          entity_id: light.dimmerlink_light
        data:
          brightness_pct: 70
          transition: 2
```

#### Turn Off After 5 Minutes

```yaml
automation:
  - alias: "Auto Turn Off"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor
        to: "off"
        for:
          minutes: 5
    action:
      - service: light.turn_off
        target:
          entity_id: light.dimmerlink_light
        data:
          transition: 5
```

#### Night Mode (Dimmed Light)

```yaml
automation:
  - alias: "Night Mode"
    trigger:
      - platform: time
        at: "23:00:00"
    condition:
      - condition: state
        entity_id: light.dimmerlink_light
        state: "on"
    action:
      - service: light.turn_on
        target:
          entity_id: light.dimmerlink_light
        data:
          brightness_pct: 20
          transition: 10
```

#### Change Curve by Load Type

```yaml
automation:
  - alias: "Curve for LED"
    trigger:
      - platform: state
        entity_id: input_select.lamp_type
        to: "LED"
    action:
      - service: select.select_option
        target:
          entity_id: select.dimmerlink_curve
        data:
          option: "LOG"

  - alias: "Curve for Incandescent"
    trigger:
      - platform: state
        entity_id: input_select.lamp_type
        to: "Incandescent"
    action:
      - service: select.select_option
        target:
          entity_id: select.dimmerlink_curve
        data:
          option: "RMS"
```

### Scripts

#### Gradual Morning Turn On

```yaml
script:
  morning_light:
    alias: "Morning Light"
    sequence:
      - service: light.turn_on
        target:
          entity_id: light.dimmerlink_light
        data:
          brightness_pct: 10
      - delay:
          seconds: 30
      - service: light.turn_on
        target:
          entity_id: light.dimmerlink_light
        data:
          brightness_pct: 50
          transition: 120  # 2 minutes
```

#### Presence Simulation

```yaml
script:
  presence_simulation:
    alias: "Presence Simulation"
    sequence:
      - repeat:
          count: 10
          sequence:
            - service: light.turn_on
              target:
                entity_id: light.dimmerlink_light
              data:
                brightness_pct: "{{ range(30, 80) | random }}"
            - delay:
                minutes: "{{ range(5, 20) | random }}"
            - service: light.turn_off
              target:
                entity_id: light.dimmerlink_light
            - delay:
                minutes: "{{ range(10, 30) | random }}"
```

### Scenes

```yaml
scene:
  - name: "Movie"
    entities:
      light.dimmerlink_light:
        state: on
        brightness_pct: 15

  - name: "Reading"
    entities:
      light.dimmerlink_light:
        state: on
        brightness_pct: 80

  - name: "Romantic"
    entities:
      light.dimmerlink_light:
        state: on
        brightness_pct: 30
```

### Groups

```yaml
light:
  - platform: group
    name: "All Dimmers"
    entities:
      - light.dimmerlink_living_room
      - light.dimmerlink_bedroom
      - light.dimmerlink_kitchen
```

### Template Sensor for Power

If lamp power is known:

```yaml
sensor:
  - platform: template
    sensors:
      dimmer_power:
        friendly_name: "Dimmer Power"
        unit_of_measurement: "W"
        value_template: >
          {% set brightness = state_attr('light.dimmerlink_light', 'brightness') | default(0) %}
          {% set max_power = 100 %}
          {{ (brightness / 255 * max_power) | round(1) }}
```

### History and Statistics

Add to `configuration.yaml`:

```yaml
recorder:
  include:
    entities:
      - light.dimmerlink_light
      - sensor.dimmerlink_level
      - sensor.dimmerlink_ac_frequency

history:
  include:
    entities:
      - light.dimmerlink_light
```

---

## 5.2 Troubleshooting

### I2C Issues

#### Device Not Found (0x50)

**Symptom:** Logs don't show "Found i2c device at address 0x50"

**Check:**

| Check | Solution |
|-------|----------|
| DimmerLink in I2C mode? | Switch via UART: `02 5B` |
| Power connected? | VCC -> 3.3V, GND -> GND |
| Wires correct? | SDA -> SDA, SCL -> SCL (not crossed!) |
| Pull-up resistors? | Add 4.7k ohm on SDA and SCL to 3.3V |
| Wires short enough? | I2C works up to 30 cm |

**Diagnostics:**

```yaml
# Add to configuration for debugging
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true  # Enable scanning
  id: bus_a

logger:
  level: DEBUG
  logs:
    i2c: DEBUG
```

#### Intermittent I2C Errors

**Symptom:** "I2C communication failed" from time to time

**Solutions:**

1. **Reduce frequency:**

   ```yaml
   i2c:
     frequency: 50kHz  # Instead of 100kHz
   ```

2. **Add delays:**

   ```yaml
   output:
     - platform: template
       write_action:
         - delay: 1ms  # Before writing
         - lambda: |-
             // ... code
   ```

3. **Check power supply** — unstable power causes I2C errors

### WiFi Issues

#### ESP32 Doesn't Connect to WiFi

**Check:**

- Correct SSID and password in `secrets.yaml`
- WiFi on 2.4 GHz (ESP32 doesn't support 5 GHz)
- Router not blocking new devices

**Solution — Fallback AP:**

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  ap:
    ssid: "dimmerlink-fallback"
    password: "12345678"

captive_portal:
```

If it doesn't connect to WiFi — it will create an access point for configuration.

#### Frequent Disconnections

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  fast_connect: true
  power_save_mode: NONE  # Disable power saving
```

### Home Assistant Issues

#### Device Doesn't Appear in HA

1. Check that ESPHome addon is running
2. Check api key in configuration
3. Restart Home Assistant
4. Add manually: Settings -> Integrations -> Add -> ESPHome

#### "Entity unavailable"

**Causes:**
- ESP32 offline
- WiFi issues
- API key mismatch

**Check:**

```bash
ping dimmerlink.local
```

### Dimming Issues

#### Lamp Flickers

**Causes and Solutions:**

| Cause | Solution |
|-------|----------|
| LED not dimmable | Use lamps marked "dimmable" |
| Low load | Minimum 10-25W for stable operation |
| Wrong curve | Try LOG for LED |
| Mains interference | Add RC snubber to dimmer output |

#### Lamp Doesn't Turn Off Completely

DimmerLink completely turns off the TRIAC at level=0. If the lamp still glows:

- Problem in dimmer or wiring
- Some LED drivers have leakage

#### Brightness Range Too Narrow

Try different curves:

```yaml
# In Home Assistant Developer Tools
service: select.select_option
target:
  entity_id: select.dimmerlink_curve
data:
  option: "RMS"  # or "LOG"
```

### ESPHome Issues

#### Compilation Error

**"lambda function not allowed"**

Check indentation in YAML — lambda must have correct indentation:

```yaml
# Correct
write_action:
  - lambda: |-
      uint8_t data[2] = {0x10, 50};
      id(bus_a).write(0x50, data, 2, true);

# Incorrect (missing "|-")
write_action:
  - lambda:
      uint8_t data[2] = {0x10, 50};
```

**"id not found"**

Make sure `id: bus_a` is declared in the `i2c:` section.

#### OTA Not Working

1. Check that ESP32 is on the same network
2. Use IP instead of hostname:

   ```bash
   esphome run dimmerlink.yaml --device 192.168.1.100
   ```

### Diagnostic Commands

#### I2C Check from ESPHome

Add a diagnostic button:

```yaml
button:
  - platform: template
    name: "I2C Diagnostic"
    on_press:
      - lambda: |-
          ESP_LOGI("diag", "=== I2C Diagnostic ===");

          // Check connection
          uint8_t reg = 0x00;
          uint8_t status = 0;
          auto err = id(bus_a).write(0x50, &reg, 1, false);
          ESP_LOGI("diag", "Write result: %d", err);

          err = id(bus_a).read(0x50, &status, 1);
          ESP_LOGI("diag", "Read result: %d, status: 0x%02X", err, status);

          // Read version
          reg = 0x03;
          uint8_t version = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &version, 1);
          ESP_LOGI("diag", "Firmware: %d", version);

          // Read frequency
          reg = 0x20;
          uint8_t freq = 0;
          id(bus_a).write(0x50, &reg, 1, false);
          id(bus_a).read(0x50, &freq, 1);
          ESP_LOGI("diag", "AC Frequency: %d Hz", freq);

          ESP_LOGI("diag", "=== Done ===");
```

#### Logging All Operations

```yaml
logger:
  level: DEBUG
  logs:
    i2c: DEBUG
    api: DEBUG
    sensor: DEBUG
```

### Factory Reset

#### ESP32

1. Erase flash:

   ```bash
   esptool.py --port COM3 erase_flash
   ```

2. Flash firmware again

#### DimmerLink

Send RESET command:

```yaml
button:
  - platform: template
    name: "Factory Reset DimmerLink"
    on_press:
      - lambda: |-
          uint8_t data[2] = {0x01, 0x01};
          id(bus_a).write(0x50, data, 2, true);
      - delay: 3s
```

### Error Codes Table

| Code | Name | Cause | Solution |
|------|------|-------|----------|
| 0x00 | OK | — | — |
| 0xF9 | ERR_SYNTAX | Invalid register | Check register address |
| 0xFC | ERR_NOT_READY | Not calibrated or FLASH error | Wait 2 sec after power on |
| 0xFD | ERR_INDEX | Invalid dimmer index | Use 0 |
| 0xFE | ERR_PARAM | Invalid parameter | level <= 100, curve <= 2 |

---

## 5.3 Next Steps

### Current Status

**DimmerLink v1.0:**
- I2C interface
- UART interface
- Basic dimming (0-100%)
- Three curves (LINEAR, RMS, LOG)
- Auto-detection of mains frequency
- I2C address change

**ESPHome Integration v1.0:**
- Light entity
- All sensors
- Curve selection
- Diagnostics
- Multi-device

### Development Plans

#### DimmerLink v2.0 (In Development)

| Feature | Status | Description |
|---------|--------|-------------|
| Thermal protection | In development | NTC monitoring, PWM fan |
| Smooth FADE | In development | Hardware fade by command |
| Multi-channel | Planned | Up to 4 channels on one module |
| EEPROM settings | Planned | Saving level and curve |

#### ESPHome Integration v2.0

| Feature | Status | Description |
|---------|--------|-------------|
| Native component | Available | [External Component](../../../components/README.md) |
| Thermal monitoring | In development | Temperature sensor |
| Fan control | In development | Cooling management |
| Energy monitoring | Planned | Integration with ACS712/CT |

### Native ESPHome Component

A native ESPHome external component is now available. It provides clean YAML configuration without lambda code:

```yaml
external_components:
  - source: github://robotdyn-dimmer/DimmerLink@main
    components: [dimmerlink]

dimmerlink:
  id: dimmer1
  address: 0x50

light:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    name: "Light"

sensor:
  - platform: dimmerlink
    dimmerlink_id: dimmer1
    ac_frequency:
      name: "AC Frequency"
```

> [!TIP]
> See the [External Component Documentation](../../../components/README.md) for full configuration reference.

### Extended Projects

#### Solar Router

Using DimmerLink to route excess solar energy:

```text
+---------------+     +---------------+     +---------------+
| Solar Panel   |---->| CT Sensor     |---->|   ESP32       |
+---------------+     +---------------+     |  + ESPHome    |
                                            |               |
+---------------+     +---------------+     |               |
|   Heater      |<----|  DimmerLink   |<----|               |
+---------------+     +---------------+     +---------------+
```

#### Multi-room System

```yaml
# Central controller for the whole house
substitutions:
  # Dimmer addresses
  living_room: "0x50"
  bedroom: "0x51"
  kitchen: "0x52"
  bathroom: "0x53"

# 4 dimmers on one ESP32
```

#### Smart Alarm Clock

```yaml
# Gradual light turn on for waking up
automation:
  - alias: "Sunrise Alarm"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - service: light.turn_on
        target:
          entity_id: light.bedroom_dimmer
        data:
          brightness_pct: 1
      - repeat:
          count: 30
          sequence:
            - delay: 60
            - service: light.turn_on
              target:
                entity_id: light.bedroom_dimmer
              data:
                brightness_pct: "{{ repeat.index * 3 }}"
                transition: 60
```

### Feedback

**Found a bug or have a suggestion?**

- GitHub Issues: [DimmerLink Repository](https://github.com/robotdyn-dimmer/DimmerLink/issues)
- Documentation: [rbdimmer.com/docs](https://rbdimmer.com/docs)

**Want to help with development?**

- Testing new versions
- Documentation and translations
- Usage examples

### Useful Links

| Resource | Link |
|----------|------|
| **DimmerLink Documentation** | [rbdimmer.com/docs](https://rbdimmer.com/docs) |
| **ESPHome** | [esphome.io](https://esphome.io) |
| **Home Assistant** | [home-assistant.io](https://home-assistant.io) |
| **ESPHome I2C** | [esphome.io/components/i2c](https://esphome.io/components/i2c.html) |
| **ESPHome Light** | [esphome.io/components/light](https://esphome.io/components/light/index.html) |
| **HACS** | [hacs.xyz](https://hacs.xyz) |

---

## Change History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial version |

---

[<- Back: Lambda Reference](./04_lambda_reference.md) | [Contents](./README.md)
