# Command Reference — DimmerLink Native Tasmota Driver

All commands follow standard Tasmota conventions:

- Commands are **case-insensitive**.
- An optional device number suffix (1 to 4) selects which detected device to address. No suffix, or suffix `0`, defaults to device 1.
- Sending a command with **no argument** returns the current state without changing anything.
- All commands return **JSON responses**.

The native driver uses the prefix **`Dl`** for all commands (different from the Berry driver's `DimmerLink` prefix).

---

## DlDim — Set channel brightness

**Syntax:**

```
DlDim[<dev>] [<ch>,]<level>
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |
| `ch` | integer | 1-4 | Channel number (optional; default = 1) |
| `level` | integer | 0-100 | Brightness in percent |

**Examples:**

```
DlDim 50            # Device 1, channel 1 to 50%
DlDim 75            # Device 1, channel 1 to 75%
DlDim 2,30          # Device 1, channel 2 to 30%
DlDim2 100          # Device 2, channel 1 to 100%
DlDim2 3,0          # Device 2, channel 3 to 0% (off)
DlDim               # Query — returns all channel levels for device 1
```

**Response:**

```json
{"DlDim1":{"Ch1":50,"Ch2":30,"Ch3":0,"Ch4":0}}
```

The response always includes all channels for the addressed device.

**Notes:**

- Level `0` turns the channel off. The device stores no memory of the previous level — re-enabling requires sending a new non-zero level.
- Level values above 100 are silently clamped to 100 by the driver.
- If the device returns a non-zero error code after the write, the driver logs it at DEBUG level but does not return an error to the caller.

---

## DlCurve — Set dimming curve

**Syntax:**

```
DlCurve[<dev>] [<ch>,]<curve>
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |
| `ch` | integer | 1-4 | Channel number (optional; default = 1) |
| `curve` | integer | 0-2 | Curve: 0 = LINEAR, 1 = RMS, 2 = LOG |

**Examples:**

```
DlCurve 0           # Device 1, channel 1 to LINEAR
DlCurve 2           # Device 1, channel 1 to LOG
DlCurve 2,1         # Device 1, channel 2 to RMS
DlCurve2 1          # Device 2, channel 1 to RMS
DlCurve             # Query — returns all channel curves for device 1
```

**Response:**

```json
{"DlCurve1":{"Ch1":"LINEAR","Ch2":"LOG","Ch3":"LINEAR","Ch4":"LINEAR"}}
```

**Notes:**

- The curve setting is applied per channel and is independent between channels on the same device.
- Curve values outside 0-2 are rejected; the command has no effect and returns the current state.
- Curve changes take effect on the next brightness command. Setting a curve does not by itself change the output level.

See [Dimming Curves Explained](https://www.rbdimmer.com/docs/dimmerlink-tasmota-xdrv-advanced) for detailed descriptions of each curve and load-type recommendations.

---

## DlFade — Set fade transition time

**Syntax:**

```
DlFade[<dev>] <value>
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |
| `value` | integer | 0-255 | Fade duration in units of 100ms |

**Examples:**

```
DlFade 0            # Instant change (no fade)
DlFade 10           # 1.0 second fade
DlFade 50           # 5.0 second fade
DlFade 255          # 25.5 second fade (maximum)
DlFade2 20          # Device 2: 2.0 second fade
DlFade              # Query — returns current fade value for device 1
```

**Response:**

```json
{"DlFade":0}
```

**Notes:**

- Fade is a **device-level** setting, not per-channel. All channels on a device use the same fade time.
- The fade is implemented in the MCU hardware. Once a brightness change is written over I2C, the device internally ramps to the new level over the configured duration.
- Fade time `0` means immediate change. Fade applies to all brightness writes including those from web UI sliders.

---

## DlStatus — Show full device status

**Syntax:**

```
DlStatus[<dev>]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |

**Examples:**

```
DlStatus            # Status of device 1
DlStatus2           # Status of device 2
```

**Response:**

```json
{
  "DimmerLink1": {
    "Addr": "0x50",
    "Bus": 1,
    "Ready": true,
    "FW": 1,
    "ACFreq": 50,
    "Fade": 10,
    "Ch1": {"Level": 75, "Curve": "LOG"},
    "Ch2": {"Level": 30, "Curve": "RMS"},
    "Ch3": {"Level": 0, "Curve": "LINEAR"},
    "Ch4": {"Level": 0, "Curve": "LINEAR"},
    "Temperature": 42,
    "ThermalState": "NORMAL"
  }
}
```

The `Temperature` and `ThermalState` fields appear only if the MCU firmware has the temperature feature enabled. The `LastError` field appears only if the last I2C operation produced a non-zero error code:

```json
{
  "DimmerLink1": {
    "Addr": "0x50",
    "Bus": 1,
    "Ready": true,
    "FW": 1,
    "ACFreq": 50,
    "Fade": 0,
    "Ch1": {"Level": 0, "Curve": "LINEAR"},
    "LastError": "0xFE"
  }
}
```

---

## DlReset — Software reset

**Syntax:**

```
DlReset[<dev>]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |

**Examples:**

```
DlReset             # Reset device 1
DlReset2            # Reset device 2
```

**Response:**

```json
{"Command":"Done"}
```

**Notes:**

- Writes `0x01` to the COMMAND register (0x01).
- After reset, the device's `ready` flag is set to `false`. It is restored automatically by the next second-tick poll once the device reports READY status.
- The reset reinitializes the MCU firmware, including triggering a new AC frequency calibration cycle. During calibration, brightness commands may be ignored or produce inconsistent results.

---

## DlRecalibrate — Re-calibrate AC frequency

**Syntax:**

```
DlRecalibrate[<dev>]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |

**Examples:**

```
DlRecalibrate       # Recalibrate device 1
DlRecalibrate2      # Recalibrate device 2
```

**Response:**

```json
{"Command":"Done"}
```

**Notes:**

- Writes `0x02` to the COMMAND register.
- Triggers a new AC frequency measurement cycle. Useful if the mains frequency is reporting incorrectly, or after moving the device to a different AC supply.
- The device sets READY = 0 during calibration. Normal operation resumes automatically after calibration completes (typically 1-2 seconds).
- Unlike `DlReset`, recalibration does **not** reset channel levels or curve settings.

---

## DlAddress — Change or query I2C address

**Syntax:**

```
DlAddress[<dev>] [<new_addr>]
```

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `dev` | integer | 1-4 | Device number (optional; default = 1) |
| `new_addr` | hex or decimal | 0x08-0x77 | New I2C address (optional; omit to query) |

**Examples:**

```
DlAddress           # Query current address of device 1
DlAddress 0x51      # Change device 1 address to 0x51
DlAddress2 0x52     # Change device 2 address to 0x52
DlAddress 81        # Change device 1 to decimal 81 (= 0x51)
```

**Response (query):**

```json
{"DlAddress":"0x50"}
```

**Response (after successful change):**

```json
{"DlAddress":"0x51"}
```

**Notes:**

- The address is accepted in hexadecimal (with `0x` or `0X` prefix) or decimal.
- After writing, the driver waits 50ms and reads the VERSION register from the new address to verify the change. If verification fails, the command returns an error, but the device hardware may have already changed — use `DlStatus` to check.
- The new address is stored in the MCU's internal flash and persists through power cycles.
- The old address becomes invalid immediately after the command succeeds.
- Addresses 0x00-0x07 and 0x78-0x7F are rejected as reserved I2C addresses.

---

## Full command table

| Command | Argument | Device | Channel | Returns |
|---------|----------|--------|---------|---------|
| `DlDim[N]` | `[ch,]level` | Optional | Optional | All channel levels |
| `DlCurve[N]` | `[ch,]curve` | Optional | Optional | All channel curves |
| `DlFade[N]` | `value` | Optional | -- | Current fade value |
| `DlStatus[N]` | -- | Optional | -- | Full device status |
| `DlReset[N]` | -- | Optional | -- | Done |
| `DlRecalibrate[N]` | -- | Optional | -- | Done |
| `DlAddress[N]` | `[new_addr]` | Optional | -- | Current or new address |
