# Single Board Computers

Connecting DimmerLink to Raspberry Pi, Orange Pi, Banana Pi and other SBCs.

---

## Overview

All single board computers operate on **3.3V logic** — direct connection to DimmerLink without level converters.

| SBC | I2C | UART | OS |
|-----|-----|------|-----|
| Raspberry Pi 3/4/5 | ✓ | ✓ | Raspberry Pi OS |
| Orange Pi | ✓ | ✓ | Armbian |
| Banana Pi | ✓ | ✓ | Armbian |

---

## Raspberry Pi

### Wiring

| Raspberry Pi | DimmerLink | Function |
|--------------|------------|----------|
| Pin 1 (3.3V) | VCC | Power |
| Pin 6 (GND) | GND | Ground |
| Pin 3 (GPIO2) | SDA | I2C Data |
| Pin 5 (GPIO3) | SCL | I2C Clock |

For UART:

| Raspberry Pi | DimmerLink | Function |
|--------------|------------|----------|
| Pin 1 (3.3V) | VCC | Power |
| Pin 6 (GND) | GND | Ground |
| Pin 8 (GPIO14) | RX | UART TX → RX |
| Pin 10 (GPIO15) | TX | UART RX ← TX |

### Enabling Interfaces

```bash
sudo raspi-config
```

- **I2C:** Interface Options → I2C → Enable
- **UART:** Interface Options → Serial Port → Enable

Reboot after changes:
```bash
sudo reboot
```

### Checking I2C

```bash
# Install utilities (if not installed)
sudo apt install i2c-tools

# Scan I2C bus for devices
i2cdetect -y 1
```

Expected output — address `50` in the table:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

### Control via CLI

**I2C:**
```bash
# Set brightness to 50%
i2cset -y 1 0x50 0x10 0x32

# 💡 Tip: 0x32 in HEX = 50 in decimal. You can use decimal numbers: i2cset -y 1 0x50 0x10 50

# Read current brightness
i2cget -y 1 0x50 0x10

# Read mains frequency
i2cget -y 1 0x50 0x20
```

**UART:**
```bash
# Configure port
stty -F /dev/serial0 115200 cs8 -cstopb -parenb

# Set brightness to 50% (requires xxd or printf)
printf '\x02\x53\x00\x32' > /dev/serial0

# Or using a Python one-liner
python3 -c "import serial; s=serial.Serial('/dev/serial0', 115200); s.write(bytes([0x02,0x53,0x00,50])); print(s.read(1).hex())"
```

### Python + I2C (smbus2)

```bash
# Install library
pip install smbus2
```

```python
from smbus2 import SMBus
import time

DIMMER_ADDR = 0x50
REG_LEVEL = 0x10
REG_CURVE = 0x11
REG_FREQ = 0x20

bus = SMBus(1)

# Read mains frequency
freq = bus.read_byte_data(DIMMER_ADDR, REG_FREQ)
print(f"Mains frequency: {freq} Hz")

# Set brightness to 50%
bus.write_byte_data(DIMMER_ADDR, REG_LEVEL, 50)
print("Brightness set: 50%")

# Read current brightness
level = bus.read_byte_data(DIMMER_ADDR, REG_LEVEL)
print(f"Current brightness: {level}%")

bus.close()
```

### Python + UART (pyserial)

```bash
# Install library
pip install pyserial
```

```python
import serial
import time

ser = serial.Serial('/dev/serial0', 115200, timeout=0.1)

# Set brightness to 50%
ser.write(bytes([0x02, 0x53, 0x00, 50]))
resp = ser.read(1)
if resp and resp[0] == 0x00:
    print("Brightness set: 50%")

# Get mains frequency
ser.write(bytes([0x02, 0x52]))
resp = ser.read(2)
if len(resp) == 2 and resp[0] == 0x00:
    print(f"Mains frequency: {resp[1]} Hz")

ser.close()
```

### Auto-start Script (systemd)

Create file `/etc/systemd/system/dimmer.service`:

```ini
[Unit]
Description=Dimmer Controller Service
After=multi-user.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/pi/dimmer_control.py
Restart=on-failure
User=pi

[Install]
WantedBy=multi-user.target
```

> ⚠️ **Important:** Replace `/home/pi/dimmer_control.py` with the path to your script.

Activation:
```bash
sudo systemctl daemon-reload
sudo systemctl enable dimmer.service
sudo systemctl start dimmer.service
```

---

## Orange Pi

### Features

- Most models are GPIO-compatible with Raspberry Pi
- OS: Armbian (recommended)
- I2C/UART utilities are similar to Raspberry Pi

### Enabling I2C

In Armbian:
```bash
sudo armbian-config
# System → Hardware → enable i2c
```

Or via overlays in `/boot/armbianEnv.txt`:
```
overlays=i2c0
```

> 📘 Orange Pi typically runs Armbian. [Armbian installation instructions](https://www.armbian.com/download/)

### Wiring (Orange Pi Zero)

| Orange Pi Zero | DimmerLink |
|----------------|------------|
| Pin 1 (3.3V) | VCC |
| Pin 6 (GND) | GND |
| Pin 3 (PA12/SDA) | SDA |
| Pin 5 (PA11/SCL) | SCL |

### Verification

```bash
# Scan for devices
i2cdetect -y 0   # May be i2c-0 instead of i2c-1
```

### Python Code

Code is similar to Raspberry Pi, but bus number may differ:

```python
from smbus2 import SMBus

# Orange Pi may use bus 0
bus = SMBus(0)  # or SMBus(1) — depends on model
```

---

## Banana Pi

### Features

- GPIO pinout compatible with Raspberry Pi
- OS: Armbian, BPI-WiringPi

### Wiring (Banana Pi M2)

| Banana Pi M2 | DimmerLink |
|--------------|------------|
| Pin 1 (3.3V) | VCC |
| Pin 6 (GND) | GND |
| Pin 3 (GPIO2) | SDA |
| Pin 5 (GPIO3) | SCL |

### Enabling I2C

```bash
sudo armbian-config
# System → Hardware → enable i2c
```

---

## General Recommendations

### Pull-up Resistors for I2C

| Board | Built-in Pull-up | Recommendation |
|-------|------------------|----------------|
| Raspberry Pi | 1.8kΩ | Usually sufficient |
| Orange Pi | Varies by model | Check, add 4.7kΩ |
| Banana Pi | Varies by model | Check, add 4.7kΩ |
| Pico | None | Add 4.7kΩ |

### Wire Length

- **I2C:** up to 30 cm without issues
- **UART:** up to 1-2 meters

### Power Supply

- DimmerLink draws minimal current
- Power from the 3.3V pin of the SBC is usually sufficient
- If operation is unstable, use a separate power supply

### Debugging

1. **I2C not working:**
   - Check `i2cdetect` — is address 0x50 visible
   - Check pull-up resistors
   - Make sure I2C is enabled in the system

2. **UART not working:**
   - Check TX↔RX (crossed wiring)
   - Make sure UART is enabled
   - Check access permissions for `/dev/serial0`

3. **Permission denied error:**
   ```bash
   sudo usermod -a -G i2c,dialout $USER
   # Log out and back in after this
   ```

---

## What's Next?

- [Advanced Usage](06_ADVANCED_USAGE.md) — USB-UART, wireless modules
- [FAQ](07_FAQ_TROUBLESHOOTING.md) — troubleshooting
- [Code Examples](examples/) — ready-to-use scripts
