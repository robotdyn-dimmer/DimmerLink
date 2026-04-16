# Advanced Features — DimmerLink Native Tasmota Driver

## Dimming curves explained

The dimming curve determines how the brightness percentage maps to the actual TRIAC firing angle and therefore the power delivered to the load. Three curves are available, each suited to different load types.

### LINEAR (value 0)

Phase angle is proportional to the requested brightness level. The relationship between the command value and electrical power is linear.

```
Power delivered
100% |                    /
     |                  /
 50% |              /
     |           /
  0% |        /
     +------------------
     0%     50%     100%
          Level set
```

**Use cases:** General-purpose loads, resistive heaters, brush motors, any load where electrical power should track the set value directly. Suitable as a starting point when the optimal curve is unknown.

**Not recommended for:** LED lamps or incandescent lamps where perceived brightness is important.

### RMS (value 1)

The phase angle is adjusted so that the RMS voltage (and therefore RMS power) is proportional to the set level.

```
Perceived brightness (incandescent)
100% |                    /
     |                  /
 50% |              /
     |           /
  0% |        /
     +------------------
     0%     50%     100%
          Level set
```

**Use cases:** Incandescent lamps and halogen lamps. With the RMS curve, a setting of 50 produces approximately half the perceived brightness of 100.

**Not recommended for:** LED lamps — dimmable LED drivers regulate current internally and the relationship between phase angle and light output is more complex.

### LOG (value 2)

The phase angle follows a logarithmic curve designed to produce perceptually uniform brightness steps. The human eye responds logarithmically to light intensity (Weber-Fechner law), so equal increments produce approximately equal perceptual changes.

```
Perceived brightness (LED)
100% |                         /
     |                    /
 50% |               /
     |         /
  0% |  /
     +------------------
     0%     50%     100%
          Level set
```

**Use cases:** Dimmable LED lamps, mood lighting, any application where smooth dimming at low levels is important. Prevents the common complaint that LEDs jump from "barely on" to "very bright" over a small range.

**Not recommended for:** Resistive or inductive loads where power tracking (not perceived brightness) is the goal.

### Selecting the right curve

| Load | Recommended curve | Command |
|------|------------------|---------|
| Dimmable LED | LOG | `DlCurve 2` |
| Incandescent lamp | RMS | `DlCurve 1` |
| Halogen lamp | RMS | `DlCurve 1` |
| Resistive heater | LINEAR | `DlCurve 0` |
| Brush motor (fan, mixer) | LINEAR | `DlCurve 0` |
| Unknown | Try all three | -- |

The curve can be changed at any time without affecting the current brightness level. The new curve applies from the next brightness-change command onward.

---

## Fade control

### How fade works

The fade time controls how long the MCU hardware takes to transition from the current brightness level to a newly commanded level. The fade is implemented entirely in the MCU firmware — once a new level is written over I2C, the device ramps internally without further interaction from Tasmota.

### Time calculation

```
Fade time (seconds) = value * 0.1
```

| Value | Fade time |
|-------|-----------|
| 0 | Instant (no fade) |
| 1 | 100ms |
| 5 | 500ms |
| 10 | 1.0 second |
| 20 | 2.0 seconds |
| 50 | 5.0 seconds |
| 100 | 10.0 seconds |
| 255 | 25.5 seconds (maximum) |

### Fade is per-device, not per-channel

All channels on a single device share the same fade time. To have different fade speeds on different channels, they must be on separate devices.

### Using fade with web UI sliders

The fade setting affects all level-change commands, including those from web UI sliders. With a non-zero fade value, moving the slider sends a new target level and the device smoothly transitions. Moving the slider again before the fade completes starts a new transition from the current position.

### Setting and querying fade

```
DlFade 10       # Set device 1 fade to 1.0 second
DlFade2 0       # Set device 2 fade to instant
DlFade          # Query device 1 current fade value
```

---

## Temperature monitoring

### Feature availability

Temperature monitoring is available when the MCU firmware is compiled with `FEATURE_TEMPERATURE`. The driver detects this automatically at boot by reading the `TEMP_CURRENT` register (0x40). If the register returns `0xFF`, temperature is not available and all temperature fields are suppressed.

### Temperature reading

Raw temperature is stored in register 0x40 with an offset of 50:

```
Temperature (Celsius) = register_value - 50
```

This allows a range from -50 C (register value 0) to +205 C (register value 255). Normal operating range is 20 C to 100 C.

### Thermal states

The MCU firmware manages thermal protection autonomously. Six states are defined:

| State | Value | Description | Driver action |
|-------|-------|-------------|--------------|
| NORMAL | 0 | Temperature below warning threshold | None |
| WARNING | 1 | Temperature above warning threshold | Logged at DEBUG level |
| DERATE | 2 | Active output reduction to lower temperature | None — hardware manages output |
| CRITICAL | 3 | Aggressive output reduction | Logged at ERROR level |
| SHUTDOWN | 4 | All outputs disabled for safety | Logged at ERROR level |
| FAULT | 5 | Temperature sensor fault | Logged at ERROR level |

States CRITICAL, SHUTDOWN, and FAULT trigger an ERROR-level log entry:

```
DLK: Dev 0x50 thermal CRITICAL
```

### Thermal protection is hardware-managed

The MCU handles derating and shutdown autonomously. Tasmota cannot override thermal protection — if the device is in SHUTDOWN state, writing a brightness value has no effect until the device cools and exits shutdown.

### Monitoring via MQTT

With temperature enabled, each SENSOR message includes the current temperature and state:

```json
{
  "DimmerLink1": {
    "Addr": "0x50",
    "ACFreq": 50,
    "Fade": 0,
    "Ch1": {"Level": 75, "Curve": "LOG"},
    "Temp": 42,
    "Thermal": "NORMAL"
  }
}
```

Use Tasmota rules or an external system to respond to thermal state changes.

---

## Multi-device addressing

The driver supports up to four DimmerLink devices on the same I2C bus (or spread across two buses). Devices are numbered 1 through 4 in the order detected during the boot scan — ascending by address (0x08 to 0x77).

### Device numbering

| I2C Address | Detected Order | Device Number |
|-------------|---------------|---------------|
| 0x50 | 1st found | Device 1 (suffix `1` or no suffix) |
| 0x51 | 2nd found | Device 2 (suffix `2`) |
| 0x52 | 3rd found | Device 3 (suffix `3`) |

### Assigning unique addresses

All DimmerLink devices ship with I2C address `0x50`. To use multiple devices, each must have a unique address assigned **one at a time** before connecting them together.

**Step-by-step for three devices:**

```
Step 1: Connect Device A only (boots at 0x50)
        Console: DlAddress 0x51
        Result: Device A is now at 0x51

Step 2: Power off. Connect Device B only (boots at 0x50)
        Console: DlAddress 0x52
        Result: Device B is now at 0x52

Step 3: Power off. Connect Device C (keep at 0x50)
        Power on all three together.
        Three devices detected: 0x50, 0x51, 0x52
```

### Address verification

The driver verifies the address change by re-reading the VERSION register from the new address:

```c
if (DL_ValidRead8(&ver, new_addr, DL_REG_VERSION, dev->bus) && ver == DL_FW_VERSION) {
    dev->addr = new_addr;
    // success
}
```

If verification fails, the command returns an error. The physical device may have already changed — run `DlStatus` to confirm.

### Valid address range

Addresses must be in the range `0x08` to `0x77` (inclusive). Reserved addresses 0x00-0x07 and 0x78-0x7F are rejected.

### I2C address conflict notes

The default address `0x50` conflicts with common EEPROM/FRAM devices (FM24CXX, AT24Cxx, 24C256, 24LC256). The driver identifies DimmerLink devices specifically by firmware version byte at register 0x03, so non-DimmerLink devices at 0x50 are silently skipped. However, to avoid spurious bus transactions, change the DimmerLink address if EEPROM devices share the bus.

> [!TIP]
> Addresses in the range `0x10` to `0x4F` are typically free of conflicts in standard Tasmota builds.

### Scan order across buses

If two I2C buses are active, the scan processes Bus 0 before Bus 1. A device on Bus 0 at address 0x51 will be found before a device on Bus 1 at address 0x50.

---

## Migration from Berry driver

If you have been using the Berry `DimmerLink.be` driver and are switching to the native driver, note the following differences.

### What changes

| Aspect | Berry driver | Native driver |
|--------|-------------|---------------|
| Command prefix | `DimmerLink`, `DimmerLink_Kitchen`, etc. | `Dl` |
| Device selection | By label (name) | By detected order (number suffix) |
| Channel addressing | Trailing digit: `DimmerLink_Kitchen2` | `ch,level` syntax: `DlDim 2,50` |
| Curve command | `DimmerLink_KitchenCurve 2` | `DlCurve 2` |
| Fade command | `DimmerLink_KitchenFade 10` | `DlFade 10` |
| Virtual relay | Yes — `Power<N> ON/OFF` | Not provided |
| Presets | `DimmerLinkPreset night` | Not provided |
| Bus scanner | `DimmerLink.scan()` in Berry console | Not provided; use `I2CScan` |
| Named instances | Yes — labels in `dimmerlink.json` | Not provided — devices numbered |
| Configuration | `/dimmerlink.json` on filesystem | `user_config_override.h` at compile time |
| MQTT telemetry key | Device label (e.g., `"Kitchen"`) | `DimmerLink1`, `DimmerLink2`, etc. |

### Migration steps

1. **Flash** the new firmware that includes `USE_DIMMERLINK`.
2. **Remove Berry driver files** from the filesystem (`DimmerLink.be`, `dimmerlink_loader.be`) or remove the `load('dimmerlink_loader.be')` line from `autoexec.be`. The Berry driver and native driver can coexist but will both respond to the same device and generate duplicate telemetry.
3. **Update MQTT automations** — change `DimmerLink_<Label>` command names to `DlDim`, `DlCurve`, `DlFade`.
4. **Update MQTT subscriptions** — change named device keys (e.g., `"Kitchen"`) to numbered keys (`"DimmerLink1"`, `"DimmerLink2"`).
5. **Update HTTP API calls** — change `DimmerLink_Kitchen%2075` to `DlDim%2075` (or `DlDim2%2075` for the second device).
6. **Virtual relays** — if you relied on `Power<N>` to toggle devices, implement equivalent logic using Tasmota rules and `DlDim` commands.
7. **Presets** — if you used `DimmerLinkPreset`, create equivalent Tasmota rules or Berry scripts that issue `DlDim` commands.

> [!WARNING]
> Running both drivers simultaneously is not recommended. Both will detect the same devices and respond to their respective commands but do not share state. Telemetry will include entries from both drivers, which may cause parsing issues in downstream systems.
