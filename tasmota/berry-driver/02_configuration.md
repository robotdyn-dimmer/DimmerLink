# Configuration

## Configuration File (dimmerlink.json)

The configuration file is stored on the ESP32 filesystem at `/dimmerlink.json`. You can edit it via **Consoles > Manage File System > Edit**.

### Minimal config (single device, 1 channel)

```json
{
  "devices": [
    {"addr": "0x50", "label": "Dimmer", "channels": 1}
  ],
  "presets": {
    "night": 10,
    "low": 25,
    "mid": 50,
    "high": 75,
    "full": 100
  }
}
```

### Multi-channel config (1 device, 2 channels)

```json
{
  "devices": [
    {"addr": "0x50", "label": "Lamp", "channels": 2}
  ],
  "presets": {
    "night": 10,
    "full": 100
  }
}
```

This creates two sliders: **Lamp Ch1** and **Lamp Ch2**, controlled by `DimmerLink_Lamp1` and `DimmerLink_Lamp2` commands.

### Multiple devices on the same I2C bus

```json
{
  "devices": [
    {"addr": "0x50", "label": "Kitchen", "channels": 1},
    {"addr": "0x51", "label": "Bedroom", "channels": 1},
    {"addr": "0x52", "label": "Hall", "channels": 2}
  ],
  "presets": {
    "night": 10,
    "evening": 40,
    "full": 100
  }
}
```

Each device gets its own slider, commands, and telemetry entry.

### Configuration parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `addr` | string | Yes | `"0x50"` | I2C address in hex format |
| `label` | string | Yes | `"DimmerLink"` | Device name (used in commands and web UI) |
| `channels` | integer | No | `1` | Number of dimmer channels (1-4) |
| `presets` | object | No | (see above) | Named brightness levels |

### Label naming rules

Labels are used in Tasmota command names. Due to how Tasmota parses commands:

> [!WARNING]
> Labels must NOT end with a digit. Tasmota interprets trailing digits as a command index number.

| Label | OK? | Why |
|-------|-----|-----|
| `Kitchen` | Yes | No trailing digit |
| `DimmerA` | Yes | Ends with letter |
| `Dimmer1` | **No** | Tasmota reads "1" as command index, not part of the name |
| `Room2` | **No** | Same issue — "2" becomes the index |
| `Lamp_North` | Yes | Underscore and letter are fine |

If you use a label ending with a digit, the driver will print a warning at boot:
```
DimmerLink WARNING: label 'Room2' ends with digit — commands will not work. Rename it.
```

## Setting Up I2C Addresses

Every DimmerLink device ships with the default I2C address **0x50**. To use multiple devices on the same bus, each must have a unique address.

### Checking what's on the bus

Open **Consoles > Berry Scripting Console** and run:

```berry
import DimmerLink
DimmerLink.scan()
```

Output:
```
I2C bus 0:
  0x50 - DimmerLink v1 (READY, 50Hz, level=0%)
Found 1 DimmerLink device(s)
```

### Changing a device's address

> [!IMPORTANT]
> Connect only ONE device at a time when changing addresses.

1. Connect the device you want to reconfigure
2. In Berry console, run:

```berry
import DimmerLink
DimmerLink.change_addr(0x50, 0x51)
```

Output:
```
DimmerLink at 0x50: v1, READY
Writing new address 0x51... OK
Verifying 0x51... OK
Address changed: 0x50 -> 0x51
>>> Update dimmerlink.json: change addr to 0x51
```

3. The address change is **immediate** and saved to the device's internal flash
4. Power off, connect the next device, power on, repeat if needed
5. Update `/dimmerlink.json` with the new addresses
6. Restart Tasmota

### Step-by-step: setting up 3 devices

```
1. Connect Device A only → it responds at 0x50 (default)
   Berry console: DimmerLink.change_addr(0x50, 0x51)
   → Device A is now at 0x51

2. Power off. Connect Device B only → it responds at 0x50
   Berry console: DimmerLink.change_addr(0x50, 0x52)
   → Device B is now at 0x52

3. Power off. Connect Device C (keep at default 0x50)

4. Power off. Connect all three devices together.

5. Edit dimmerlink.json:
   {
     "devices": [
       {"addr": "0x50", "label": "DeviceC", "channels": 1},
       {"addr": "0x51", "label": "DeviceA", "channels": 1},
       {"addr": "0x52", "label": "DeviceB", "channels": 1}
     ]
   }

6. Restart Tasmota.

7. Verify: DimmerLink.scan()
   0x50 - DimmerLink v1 (READY, 50Hz)
   0x51 - DimmerLink v1 (READY, 50Hz)
   0x52 - DimmerLink v1 (READY, 50Hz)
   Found 3 DimmerLink device(s)
```

### Forgot which address a device uses?

```berry
import DimmerLink
DimmerLink.scan()
```

This always shows the real addresses on the bus, regardless of what's in the config file.

### Getting device info

If the driver is already loaded, you can inspect a running instance:

```berry
global._dimmerlink[0].info()
```

Output:
```
DimmerLink "Kitchen" at 0x50 (bus 0)
  Channels:  1
  Levels:    Ch1=50%
  Power:     ON (relay 6)
  Curve:     Ch1=LINEAR
  Fade:      0 (0.0s)
  AC freq:   50 Hz
  Firmware:  v1
  Ready:     Yes
  preinit:   DimmerLink(0x50, "Kitchen", 1)
```

## Tasmota Web Configuration

The DimmerLink driver uses the standard Tasmota web interface. No special configuration pages are needed.

<p align="center"><img src="../../images/DimmerLink-I2Cconfig.png" width="50%" /></p>

<p align="center"><img src="../../images/DimmerLink-filesystem.png" width="50%" /></p>
