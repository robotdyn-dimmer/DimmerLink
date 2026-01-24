# FAQ & Troubleshooting

Frequently asked questions and solutions for common problems when working with DimmerLink.

---

## Table of Contents

- [Connection Problems](#connection-problems)
- [Protocol Errors](#protocol-errors)
- [Dimming Issues](#dimming-issues)
- [Choosing a Dimming Curve](#choosing-a-dimming-curve)
- [Error Codes](#error-codes)

---

## Connection Problems

### No Response from Device

**Symptoms:** I send a command, but there's no response.

**Possible causes and solutions:**

| Cause | Solution |
|-------|----------|
| Missing start byte (UART) | Add `0x02` at the beginning of the command |
| Wrong baud rate (UART) | Set **115200 baud** |
| TX/RX swapped | Check crossed wiring: TX→RX, RX→TX |
| Interface in different mode | Device in I2C mode? Switch to UART |
| Wrong address (I2C) | Default address is **0x50** (may be changed) |
| No pull-up resistors (I2C) | Add 4.7kΩ resistors on SDA and SCL |
| No power | Check VCC and GND |

**I2C Diagnostics:**
```bash
# Linux/Raspberry Pi
i2cdetect -y 1
```
If the device is working, address `50` will appear in the table.

**UART Diagnostics:**
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
ser.write(bytes([0x02, 0x52]))  # Request frequency
print(ser.read(2).hex())  # Expect "0032" or "003c"
```

---

### I2C Doesn't See Device

**Symptoms:** `i2cdetect` shows empty table, address 0x50 not displayed.

**Solutions:**

1. **Check pull-up resistors**
   - Raspberry Pi: built-in 1.8kΩ usually sufficient
   - Other boards: add external 4.7kΩ

2. **Check wiring**
   ```
   SDA → SDA (not crossed!)
   SCL → SCL
   ```

3. **Check wire length**
   - I2C: maximum 30 cm without buffers
   - For longer distances use UART

4. **Check if I2C is enabled**
   ```bash
   # Raspberry Pi
   sudo raspi-config
   # Interface Options → I2C → Enable
   ```

5. **Check bus number**
   ```bash
   ls /dev/i2c-*
   # May be i2c-0 or i2c-1
   i2cdetect -y 0  # try different bus
   ```

---

### Permission Denied (Linux)

**Symptoms:** `PermissionError: [Errno 13] Permission denied: '/dev/ttyUSB0'`

**Solution:**
```bash
# Add user to groups
sudo usermod -a -G dialout,i2c $USER

# Log out and back in, or reboot
logout
```

---

### Device Not Responding After Interface Switch

**Symptoms:** After SWITCH_I2C command (0x5B) or SWITCH_UART (via I2C), device doesn't respond.

**Cause:** This is normal behavior. After switching interfaces, the old interface is disabled.

**Solutions:**

1. **After UART → I2C:**
   - Connect to device via I2C
   - Default address: 0x50 (or previously configured)
   - Check: `i2cdetect -y 1`

2. **After I2C → UART:**
   - Connect to device via UART
   - Speed: 115200, 8N1

3. **If you changed the I2C address:**
   - Scan the bus: `i2cdetect -y 1`
   - Device will be at the new address

**Example switching UART → I2C:**
```python
import serial
from smbus2 import SMBus

# 1. Send switch command via UART
ser = serial.Serial('COM3', 115200, timeout=1)
ser.write(bytes([0x02, 0x5B]))  # SWITCH_I2C
resp = ser.read(1)
if resp and resp[0] == 0x00:
    print("Switched to I2C")
ser.close()

# 2. Now work via I2C
bus = SMBus(1)
freq = bus.read_byte_data(0x50, 0x20)
print(f"Mains frequency: {freq} Hz")
```

---

## Protocol Errors

### Error 0xFC — EEPROM Write Error

**Cause:** Failed to save settings to non-volatile memory.

**Possible causes:**
- Flash memory problem
- Power supply voltage too low

**Solution:**
- Check power supply voltage (minimum 2.7V for Flash write)
- Retry the command

---

### Error 0xF9 — Syntax Error

**Cause:** Unknown command or invalid format.

**Check:**

1. **Start byte** — command must begin with `0x02`
2. **Command code** — use only valid codes:
   - `0x53` (SET)
   - `0x47` (GET)
   - `0x43` (CURVE)
   - `0x51` (GETCURVE)
   - `0x52` (FREQ)
   - `0x58` (RESET)
   - `0x5B` (SWITCH_I2C)

**Example of correct command:**
```
Correct:   02 53 00 32  (SET brightness 50%)
Incorrect: 53 00 32     (no start byte)
Incorrect: 02 99 00     (unknown command 0x99)
```

---

### Error 0xFD — Invalid Index

**Cause:** Non-existent dimmer index specified.

**Solution:** Use index `0x00`. Current version supports only one dimmer.

```
Correct:   02 53 00 32  (dimmer 0)
Incorrect: 02 53 05 32  (dimmer 5 doesn't exist)
```

---

### Error 0xFE — Invalid Parameter

**Cause:** Parameter value outside allowed range.

**Check:**
- **Brightness:** 0-100 (not 0-255!)
- **Curve:** 0, 1, or 2
- **I2C address:** 0x08-0x77

```
Correct:   02 53 00 64  (brightness 100%)
Incorrect: 02 53 00 FF  (255 > 100)

Correct:   02 43 00 02  (LOG curve)
Incorrect: 02 43 00 05  (curve 5 doesn't exist)
```

---

## Dimming Issues

### Lamp Doesn't Light at Low Brightness

**Symptoms:** Incandescent lamp doesn't light below 25% brightness.

**Cause:** This is normal behavior for incandescent lamps. At low power levels, filament heating is too low for visible light.

**Solution:** Use brightness levels of 25% and above for incandescent lamps.

---

### Brightness Doesn't Match Expectations

**Symptoms:** 50% looks like 20% or 80%.

**Cause:** Wrong dimming curve selected.

**Solution:** Choose curve matching your load type (see section below).

---

### GET and SET Format

Both **SET** and **GET** work in the same format:

- **SET** accepts percent: 0-100
- **GET** returns percent: 0-100

```python
dimmer.set_level(50)       # Set 50%
level = dimmer.get_level() # Get 50
```

---

### UART vs I2C — Which to Choose?

**UART limitations:**
- Recommended interval between commands: **10-20 ms**
- More frequent commands may cause buffer data loss
- UART is a relatively slow interface

**Recommendation:** If you need frequent brightness changes (smooth transitions, animations), use **I2C** — it's significantly faster.

```python
# UART: no more than 3-5 commands/sec
for level in range(0, 101, 10):
    ser.write(bytes([0x02, 0x53, 0x00, level]))
    time.sleep(0.3)  # Minimum 200-300 ms between commands

# I2C: can be much faster
for level in range(0, 101):
    bus.write_byte_data(0x50, 0x10, level)
    time.sleep(0.01)  # 10 ms — smooth transition
```

---

## Choosing a Dimming Curve

### Which Curve to Choose?

| Curve | Code | Best for | Feature |
|-------|------|----------|---------|
| **LINEAR** | 0 | Universal | Linear power relationship |
| **RMS** | 1 | Incandescent lamps | Linear brightness for incandescent |
| **LOG** | 2 | LED lamps | Matches eye perception |

### LINEAR (default)

```
Power
100% │         ╱
     │       ╱
 50% │     ╱
     │   ╱
  0% │ ╱
     └──────────────
       0%   50%  100%
          Level
```

- Power linearly depends on level
- Universal option
- Suitable for most loads

### RMS

```
Brightness
100% │         ╱
     │       ╱
 50% │     ╱
     │   ╱
  0% │ ╱
     └──────────────
       0%   50%  100%
          Level
```

- Compensates for incandescent lamp non-linearity
- 50% level = 50% perceived brightness
- Ideal for incandescent lamps

### LOG (Logarithmic)

```
Brightness
100% │              ╱
     │           ╱
 50% │       ╱
     │   ╱
  0% │╱
     └──────────────
       0%   50%  100%
          Level
```

- Accounts for logarithmic eye perception
- Smooth brightness changes at low levels
- Recommended for LED lamps

### Recommendations

| Load Type | Recommended Curve |
|-----------|-------------------|
| LED (dimmable) | LOG (2) |
| Incandescent lamp | RMS (1) |
| Heater | LINEAR (0) |
| Motor (fan) | LINEAR (0) |
| Don't know | Try all three |

> **⚠️ Important:** Gas discharge lamps (halogen, fluorescent, HID, sodium) are **not supported** — they require high voltage for ignition and maintaining the discharge.

**How to change curve:**
```python
# UART
ser.write(bytes([0x02, 0x43, 0x00, 2]))  # LOG

# I2C
bus.write_byte_data(0x50, 0x11, 2)  # LOG
```

---

## Error Codes

### Summary Table

| Code | Name | Description | Solution |
|------|------|-------------|----------|
| 0x00 | OK | Success | — |
| 0xF9 | ERR_SYNTAX | Invalid format | Check START byte and command code |
| 0xFC | ERR_NOT_READY | EEPROM write error | Check power, retry |
| 0xFD | ERR_INDEX | Invalid index | Use index 0 |
| 0xFE | ERR_PARAM | Invalid parameter | Check value ranges |

### Error Handling Example

**Python:**
```python
ERROR_CODES = {
    0x00: "OK",
    0xF9: "Invalid command syntax",
    0xFC: "EEPROM write error",
    0xFD: "Invalid dimmer index",
    0xFE: "Invalid parameter value"
}

def check_response(resp):
    if not resp:
        return "No response"
    code = resp[0]
    return ERROR_CODES.get(code, f"Unknown error: 0x{code:02X}")
```

**Arduino:**
```cpp
const char* getErrorMessage(uint8_t code) {
    switch (code) {
        case 0x00: return "OK";
        case 0xF9: return "Syntax error";
        case 0xFC: return "EEPROM write error";
        case 0xFD: return "Invalid index";
        case 0xFE: return "Invalid parameter";
        default: return "Unknown error";
    }
}
```

---

## Compatible Loads

### Supported

| Load Type | Support | Notes |
|-----------|---------|-------|
| Incandescent lamp | ✓ | Minimum 25% for visible light |
| Halogen lamp | ✓ | Works like incandescent |
| LED (dimmable) | ✓ | Only with "dimmable" marking |
| Heater | ✓ | Resistive load |
| Motor (fan) | ✓ | Universal/brush motors |

> ⚠️ **Important about motors:**
> - ✓ Universal brush motors (vacuums, mixers, drills)
> - ✗ Induction/asynchronous motors (most fans, pumps) — will hum and overheat!
> If unsure about motor type — don't use dimming.

### Not Supported

| Load Type | Reason |
|-----------|--------|
| Fluorescent lamps | Gas discharge, require ballast |
| HID, sodium lamps | Gas discharge, high voltage |
| LED without "dimmable" | Not designed for dimming |
| Transformer LED drivers | May overheat or fail |

---

## Lamp Flickers

**Symptoms:** Lamp blinks or flickers during operation.

**Possible causes and solutions:**

| Cause | Solution |
|-------|----------|
| LED lamp not "dimmable" | Replace with dimmable LED |
| LED power too low | Add minimum load (incandescent lamp in parallel) |
| Power line interference | Add RC snubber to dimmer output |
| Poor contact | Check all connections |

> 💡 **Note:** DimmerLink eliminates software flickering. If flickering persists — the cause is in the load or wiring.

---

## General Tips

### Checklist Before Starting

**DimmerLink:**
- [ ] Power connected (VCC and GND)
- [ ] TX/RX connected crossed (for UART)
- [ ] SDA/SCL connected directly (for I2C)
- [ ] Pull-up resistors installed (for I2C)
- [ ] Device in correct mode (UART or I2C)

**Dimmer:**
- [ ] Z-C and Dim connected to DimmerLink
- [ ] Dimmer connected to AC mains (CAUTION!)
- [ ] Load power doesn't exceed dimmer rating
- [ ] Load is compatible (see table above)

### Minimal Functionality Test

**UART:**
```python
import serial

ser = serial.Serial('COM3', 115200, timeout=1)  # Or '/dev/ttyUSB0'

# Request mains frequency
ser.write(bytes([0x02, 0x52]))
resp = ser.read(2)

if len(resp) == 2 and resp[0] == 0x00:
    print(f"Device working! Mains frequency: {resp[1]} Hz")
else:
    print(f"Error: {resp.hex() if resp else 'no response'}")
```

**I2C:**
```python
from smbus2 import SMBus

try:
    bus = SMBus(1)
    freq = bus.read_byte_data(0x50, 0x20)
    print(f"✓ Device working! Mains frequency: {freq} Hz")
except OSError as e:
    print(f"✗ Connection error: {e}")
    print("Check:")
    print("  - SDA/SCL wiring")
    print("  - Pull-up resistors")
    print("  - Is I2C enabled: sudo raspi-config")
```

---

## What's Next?

- [Quick Start](01_QUICK_START.md) — start with a simple example
- [UART Commands](03_UART_COMMUNICATION.md) — full command list
- [I2C Registers](04_I2C_COMMUNICATION.md) — register map
- [Code Examples](examples/) — ready-to-use scripts
