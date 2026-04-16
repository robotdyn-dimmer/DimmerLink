# Control Methods

## Web Dashboard

The main Tasmota page (http://device-ip/) shows:

**Brightness sliders** — one per channel. Move the slider and release to set brightness instantly.

**Sensor section** — displays per-device status:
- Level (%)
- Power (ON/OFF)
- AC Frequency (Hz)
- Firmware version
- Temperature and thermal state (if supported by hardware)
- Ready status

<p align="center"><img src="../../images/DimmerLink-dashboard1.png" width="50%" /></p>

## Serial Console / Tasmota Console

All commands work in:
- **Consoles > Console** (web)
- Serial terminal (USB connection)

### Brightness commands

```
DimmerLink_Kitchen 75       # Set Kitchen to 75%
DimmerLink_Kitchen 0        # Turn off (sets power OFF)
DimmerLink_Kitchen 100      # Full brightness

# Multi-channel device:
DimmerLink_Lamp1 80         # Channel 1 to 80%
DimmerLink_Lamp2 30         # Channel 2 to 30%
```

Note: `DimmerLink_Kitchen` is the same as `DimmerLink_Kitchen1` (Tasmota convention: no trailing digit = digit 1).

### Dimming curve

```
DimmerLink_KitchenCurve 0     # LINEAR — general purpose
DimmerLink_KitchenCurve 1     # RMS — incandescent/halogen lamps
DimmerLink_KitchenCurve 2     # LOG — LED dimmers (perceptual brightness)

# Multi-channel: specify channel first
DimmerLink_LampCurve 1 2      # Channel 1 → LOG
DimmerLink_LampCurve 2 1      # Channel 2 → RMS
```

### Fade time

```
DimmerLink_KitchenFade 0      # Instant change
DimmerLink_KitchenFade 10     # 1 second transition (10 × 100ms)
DimmerLink_KitchenFade 50     # 5 second transition
DimmerLink_KitchenFade 255    # 25.5 seconds (maximum)
```

### Presets

```
DimmerLinkPreset night        # All devices to 10%
DimmerLinkPreset low          # All devices to 25%
DimmerLinkPreset mid          # All devices to 50%
DimmerLinkPreset high         # All devices to 75%
DimmerLinkPreset full         # All devices to 100%
```

Presets are defined in `dimmerlink.json` and can be customized.

### Power control

Each DimmerLink device registers a virtual relay in Tasmota:

```
Power6 ON                     # Turn on (restores last brightness)
Power6 OFF                    # Turn off (remembers brightness for next ON)
Power6 TOGGLE                 # Toggle
```

The relay number depends on how many other relays exist. Check the web UI or use `Status 0` to see the relay mapping.

## HTTP API

All commands are accessible via HTTP GET:

```
http://<ip>/cm?cmnd=DimmerLink_Kitchen 75
http://<ip>/cm?cmnd=DimmerLink_KitchenCurve 2
http://<ip>/cm?cmnd=DimmerLink_KitchenFade 10
http://<ip>/cm?cmnd=DimmerLinkPreset night
http://<ip>/cm?cmnd=Power6 ON
```

URL-encoded format (for use in scripts):

```
http://<ip>/cm?cmnd=DimmerLink_Kitchen%2075
http://<ip>/cm?cmnd=DimmerLinkPreset%20night
```

### Response format

```json
{"DimmerLink_Kitchen1":75}
{"DimmerLink_KitchenCurve":{"Ch":1,"Curve":"LOG"}}
{"DimmerLink_KitchenFade":10}
{"DimmerLinkPreset":"night","Level":10}
```

## MQTT

### Sending commands

```
Topic: cmnd/<topic>/DimmerLink_Kitchen
Payload: 75

Topic: cmnd/<topic>/DimmerLinkPreset
Payload: night
```

### Receiving responses

```
Topic: stat/<topic>/RESULT
Payload: {"DimmerLink_Kitchen1":75}
```

### Telemetry (automatic)

Published periodically to `tele/<topic>/SENSOR`:

```json
{
  "Time": "2026-04-16T08:07:55",
  "ESP32": {"Temperature": 55.0},
  "Kitchen": {
    "Addr": "0x50",
    "Power": "ON",
    "Ready": true,
    "Level": 75
  }
}
```

Multi-channel telemetry uses `Levels` array:

```json
{
  "Lamp": {
    "Addr": "0x51",
    "Power": "ON",
    "Ready": true,
    "Levels": [80, 30]
  }
}
```

With temperature sensor (if hardware supports it):

```json
{
  "Kitchen": {
    "Addr": "0x50",
    "Power": "ON",
    "Ready": true,
    "Level": 75,
    "Temp": 42,
    "ThermalState": "NORMAL"
  }
}
```

### Status commands

```
Status 8    → includes DimmerLink data in StatusSNS
Status 0    → full device status including relay mapping
```
