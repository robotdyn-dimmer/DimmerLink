# Build & Flash — DimmerLink Native Tasmota Driver

## Prerequisites

### Software

| Requirement | Details |
|-------------|---------|
| **PlatformIO** | IDE or CLI. Install from [https://platformio.org](https://platformio.org) or as a VS Code extension |
| **Tasmota source** | Clone from `https://github.com/arendst/Tasmota` or use your existing fork |
| **Python 3** | Required by PlatformIO build system |
| **Git** | For cloning and managing source |

### Hardware

| Requirement | Details |
|-------------|---------|
| **ESP32 or ESP8266 board** | Any ESP32 variant (`tasmota32`, `tasmota32c3`, `tasmota32s3`, etc.) or ESP8266 (`tasmota`, `tasmota-sensors`, etc.) |
| **DimmerLink device** | MCU TRIAC dimmer controller with I2C interface enabled |
| **I2C wiring** | SDA, SCL, GND, VCC connections with appropriate pull-up resistors |
| **USB cable** | For flashing (or OTA update capability if re-flashing) |

### DimmerLink I2C mode

The DimmerLink device ships with UART interface enabled by default. Before connecting to Tasmota's I2C bus, the device must be switched to I2C mode using the UART command `SWITCH_I2C` (0x5B). Refer to the [DimmerLink hardware documentation](../../02_HARDWARE_CONNECTION.md) for this procedure. After switching, the device boots in I2C mode and the change is saved to the device's internal flash.

## Enabling the driver

The driver is gated by two preprocessor guards:

```c
#ifdef USE_I2C
#ifdef USE_DIMMERLINK
```

Both must be active. `USE_I2C` is typically enabled in your board's default configuration. `USE_DIMMERLINK` must be explicitly added.

### Step 1: Create or open `user_config_override.h`

This file is located at:

```
tasmota/user_config_override.h
```

If it does not exist, create it in that location.

### Step 2: Add the define

```c
#ifndef USE_DIMMERLINK
  #define USE_DIMMERLINK
#endif
```

### Step 3: Confirm I2C is enabled

Check that your target environment has `USE_I2C` defined. For both `tasmota32` (ESP32) and `tasmota` (ESP8266) this is enabled by default. If you are building a stripped-down environment (e.g., `tasmota-lite`), verify in `tasmota/tasmota_configurations.h` that `USE_I2C` is present.

### Driver registration

Once both defines are active, the driver self-registers with Tasmota's dispatch table via the `Xdrv94` function and claims I2C slot `XI2C_100` (device ID 100). No other source-level changes are needed.

## Configuration options

Two optional defines control runtime behavior. Place them in `user_config_override.h`.

### `USE_DIMMERLINK_ADDR`

Sets the starting I2C scan address expectation. The driver still scans the full valid I2C address range (0x08 to 0x77) and identifies devices by reading the VERSION register. The default value is for documentation purposes.

```c
// Default: 0x50 (factory default for DimmerLink)
#define USE_DIMMERLINK_ADDR  0x50
```

### `USE_DIMMERLINK_CHANNELS`

Sets the number of channels configured per detected device.

```c
// Default: 4 (maximum)
#define USE_DIMMERLINK_CHANNELS  4
```

Valid range: 1 to 4. Set this to the actual number of load channels physically wired to your DimmerLink hardware.

```c
// Example: single-channel installation
#define USE_DIMMERLINK_CHANNELS  1

// Example: two-channel installation
#define USE_DIMMERLINK_CHANNELS  2
```

### Compile-time limits

These are fixed in the source and cannot be changed via `user_config_override.h`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `DL_MAX_DEVICES` | 4 | Maximum simultaneous devices detected |
| `DL_MAX_CHANNELS` | 4 | Maximum channels per device |
| `DL_FW_VERSION` | 0x01 | Required firmware version fingerprint |

### Full `user_config_override.h` example

```c
// user_config_override.h — DimmerLink native driver configuration

// Enable DimmerLink native I2C driver (requires USE_I2C)
#ifndef USE_DIMMERLINK
  #define USE_DIMMERLINK
#endif

// Default I2C address (informational — driver scans full bus range)
#ifndef USE_DIMMERLINK_ADDR
  #define USE_DIMMERLINK_ADDR  0x50
#endif

// Number of channels per device (1-4)
// Set to match the physical wiring of your DimmerLink hardware
#ifndef USE_DIMMERLINK_CHANNELS
  #define USE_DIMMERLINK_CHANNELS  4
#endif
```

## I2C hardware wiring

### Pin connections

**ESP32 default pins:**

| DimmerLink Pin | ESP32 Pin | Notes |
|---------------|-----------|-------|
| VCC | 3.3V | DimmerLink accepts 1.8V, 3.3V, or 5V — no level converter needed |
| GND | GND | Common ground |
| SDA | GPIO21 (default) | Any GPIO configured as I2C SDA in Tasmota |
| SCL | GPIO22 (default) | Any GPIO configured as I2C SCL in Tasmota |

**ESP8266 default pins:**

| DimmerLink Pin | ESP8266 Pin | Notes |
|---------------|-------------|-------|
| VCC | 3.3V | Same as ESP32 |
| GND | GND | Common ground |
| SDA | GPIO4 (D2) | Default I2C SDA on most ESP8266 boards |
| SCL | GPIO5 (D1) | Default I2C SCL on most ESP8266 boards |

On both platforms, I2C pins can be remapped via the module configuration (`Configuration > Configure Module`).

### Schematic

```
        3.3V
           |
      4.7k | 4.7k
           |     |
MCU        |     |     DimmerLink
SDA  ------+-----+---- SDA
SCL  ------------+---- SCL
GND   ----------------  GND
3.3V  ----------------  VCC
```

### Pull-up resistors

External pull-ups are required. Both ESP32 and ESP8266 have weak internal pull-ups (~45 kOhm) that are insufficient for I2C.

| Wire length | Recommended pull-up value |
|-------------|--------------------------|
| Under 10 cm | 4.7 kOhm |
| 10 to 30 cm | 2.2 to 4.7 kOhm |
| Over 30 cm | 1 to 2.2 kOhm (not recommended; use shorter wires) |

Place the resistors between the SDA/SCL lines and 3.3V. One pair of resistors serves the entire bus — do not add pull-ups per device when multiple devices share the same bus.

### Voltage levels

DimmerLink accepts VCC from 1.8V to 5V and its logic levels match VCC. At 3.3V (standard ESP32/ESP8266 level) no level converter is required.

> [!WARNING]
> Do not connect 5V logic to the MCU's GPIO pins.

### I2C bus speed

The DimmerLink device operates at **100 kHz (Standard Mode)**. Tasmota's default I2C speed is suitable. Do not configure the I2C bus above 100 kHz for this device.

### Second I2C bus (ESP32)

Tasmota supports two I2C buses on ESP32, ESP8266, ESP32-C6, ESP32-H2, ESP32-P4, ESP32-S2 and ESP32-S3. The driver scans both buses automatically. If you have other I2C devices on Bus 1 and wish to isolate DimmerLink on Bus 2, configure the second SDA/SCL pair in Tasmota's module configuration and connect DimmerLink there. No driver-side change is needed.

## Building with PlatformIO

### Build command

```bash
# ESP8266 (standard Tasmota)
pio run -e tasmota

# ESP32 (standard)
pio run -e tasmota32

# ESP32-C3
pio run -e tasmota32c3

# ESP32-S3
pio run -e tasmota32s3
```

Select your target environment in `platformio_override.ini` under `default_envs`.

### Confirming the driver is compiled in

After a successful build, verify the driver is included:

```bash
pio run -e tasmota32 2>&1 | grep -i dimmerlink
```

If the driver compiled in, you will see references to `Xdrv94` or `DimmerLink` symbols in the output. If nothing appears, recheck that `USE_DIMMERLINK` is defined and that `USE_I2C` is active for the selected environment.

### Memory impact

| Resource | Usage |
|----------|-------|
| RAM (heap) | ~40 bytes per device; under 250 bytes for 4 devices at 4 channels |
| Flash (code) | ~4 to 6 KB depending on build configuration |

## Flashing the firmware

### Via PlatformIO (USB)

Connect the board via USB and run:

```bash
# ESP8266
pio run -e tasmota --target upload

# ESP32
pio run -e tasmota32 --target upload
```

PlatformIO auto-detects the serial port. If multiple ports are available, specify explicitly:

```bash
pio run -e tasmota32 --target upload --upload-port COM3
```

> [!NOTE]
> The ESP32 must be in download mode. Most dev boards enter download mode automatically. If yours does not, hold the BOOT button while pressing EN (reset), then release both.

### Via OTA (Tasmota web UI)

If the device is already running Tasmota:

1. Build the firmware (e.g., `pio run -e tasmota32`)
2. The output binary is at `.pio/build/<env>/firmware.bin`
3. In the Tasmota web UI: **Firmware Upgrade > Upgrade by file upload**
4. Select `firmware.bin` and click **Start upgrade**

> [!WARNING]
> OTA may fail with "Not enough space" on ESP32 if the firmware exceeds half the app partition. In that case, flash via USB or use the two-step safeboot OTA method.

### Via esptool.py (manual)

```bash
# ESP32
esptool.py --chip esp32 --port COM3 --baud 921600 \
  write_flash -z 0x0 .pio/build/tasmota32/firmware.bin

# ESP8266
esptool.py --chip esp8266 --port COM3 --baud 921600 \
  write_flash -z 0x0 .pio/build/tasmota/firmware.bin
```

Adjust `--chip`, `--port` and the firmware path for your environment.

## Verification

### Serial log output

Connect a serial terminal (115200 baud, 8N1) and observe the boot log. A successful detection produces:

```
DLK: DimmerLink v1 at 0x50 bus1 (4ch, 50Hz, READY)
```

If no DimmerLink devices are found, no `DLK:` lines appear in the boot log.

### Console verification

After boot, query the device:

```
DlStatus
```

A connected device responds with:

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
    "Ch2": {"Level": 0, "Curve": "LINEAR"},
    "Ch3": {"Level": 0, "Curve": "LINEAR"},
    "Ch4": {"Level": 0, "Curve": "LINEAR"}
  }
}
```

If the command returns `{"Command":"Unknown"}`, the driver is not compiled in.

### I2C scan

The Tasmota built-in I2C scan command confirms the device is on the bus:

```
I2CScan
```

### Web UI confirmation

Navigate to `http://<device-ip>/`. With DimmerLink detected, the page shows brightness sliders and sensor data rows. The absence of sliders indicates no device was detected.
