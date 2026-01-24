# I2C Communication

Detailed description of the I2C interface for controlling DimmerLink.

---

## Connection Parameters

| Parameter | Value |
|-----------|-------|
| Mode | Slave |
| Address | **0x50** (7-bit) |
| Speed | 100 kHz (Standard Mode) |
| Pull-up | 4.7kΩ on SDA and SCL |

> **ℹ️ Note:** I2C interface is available immediately after power-up.

---

## Register Model

DimmerLink uses a register-based access model — read and write operations by register addresses.

### Register Map

| Address | Name | R/W | Description |
|---------|------|-----|-------------|
| **0x00** | STATUS | R | Device status |
| **0x01** | COMMAND | W | Control commands |
| **0x02** | ERROR | R | Last error code |
| **0x03** | VERSION | R | Firmware version |
| **0x10** | DIM0_LEVEL | R/W | Dimmer 0 brightness (0-100%) |
| **0x11** | DIM0_CURVE | R/W | Dimmer 0 curve |
| **0x20** | AC_FREQ | R | Mains frequency (50/60 Hz) |
| **0x21** | AC_PERIOD_L | R | Mains period, low byte |
| **0x22** | AC_PERIOD_H | R | Mains period, high byte |
| **0x23** | CALIBRATION | R | Calibration status |
| **0x30** | I2C_ADDRESS | R/W | Current I2C address (0x08-0x77) |

---

## Register Descriptions

### STATUS (0x00) — Device Status

| Bit | Name | Description |
|-----|------|-------------|
| 0 | READY | 1 = Device is ready |
| 1 | ERROR | 1 = Last operation failed |
| 2-7 | — | Reserved |

### COMMAND (0x01) — Control Commands

| Value | Command | Description |
|-------|---------|-------------|
| 0x00 | NOP | No operation |
| 0x01 | RESET | Software reset |
| 0x02 | RECALIBRATE | Re-calibrate frequency |
| 0x03 | SWITCH_UART | Switch interface to UART |

### ERROR (0x02) — Error Code

| Code | Name | Description |
|------|------|-------------|
| 0x00 | OK | No errors |
| 0xF9 | ERR_SYNTAX | Invalid register address |
| 0xFC | ERR_NOT_READY | EEPROM write error |
| 0xFD | ERR_INDEX | Invalid dimmer index |
| 0xFE | ERR_PARAM | Invalid parameter value |

### DIM0_LEVEL (0x10) — Brightness

- **Read:** returns current brightness (0-100)
- **Write:** sets brightness (0-100)

| Value | Brightness |
|-------|------------|
| 0 | Off |
| 50 | 50% |
| 100 | Full brightness |

### DIM0_CURVE (0x11) — Dimming Curve

| Value | Curve | Application |
|-------|-------|-------------|
| 0 | LINEAR | Universal |
| 1 | RMS | Incandescent, halogen |
| 2 | LOG | LED (matches eye perception) |

### AC_FREQ (0x20) — Mains Frequency

- **Read:** 50 or 60 (Hz)

### I2C_ADDRESS (0x30) — Device I2C Address

- **Read:** returns current I2C address
- **Write:** sets new I2C address

| Parameter | Value |
|-----------|-------|
| Range | 0x08 - 0x77 |
| Default | 0x50 |

> **⚠️ Important:** After writing a new address, the device **immediately** responds on the new address. The old address no longer works. The new address is saved to EEPROM.

**Reserved addresses (not allowed):**
- 0x00-0x07: General Call, START byte, CBUS, HS-mode
- 0x78-0x7F: 10-bit addressing, Device ID

### Changing I2C Address

**Arduino:**
```cpp
// Change address from 0x50 to 0x51
void changeAddress(uint8_t newAddr) {
    Wire.beginTransmission(0x50);  // Current address
    Wire.write(0x30);               // I2C_ADDRESS register
    Wire.write(newAddr);            // New address
    Wire.endTransmission();

    // Device now responds on newAddr!
}
```

**Raspberry Pi (CLI):**
```bash
# Change address to 0x51
i2cset -y 1 0x50 0x30 0x51
# Device is now at address 0x51
i2cdetect -y 1  # Verify
```

> ⚠️ **Warning:** After changing the address, the device **immediately** stops responding on the old address!

---

### Switching to UART

If you need to switch from I2C to UART:

**Arduino:**
```cpp
Wire.beginTransmission(0x50);
Wire.write(0x01);   // COMMAND register
Wire.write(0x03);   // SWITCH_UART
Wire.endTransmission();
// I2C no longer works, use UART
```

**Raspberry Pi:**
```bash
i2cset -y 1 0x50 0x01 0x03
# Now control via UART only
```

### AC_PERIOD_L/H (0x21, 0x22) — Mains Period

Half-wave period in microseconds (16-bit value):
```
period_us = (AC_PERIOD_H << 8) | AC_PERIOD_L
```

| Frequency | Expected Period |
|-----------|-----------------|
| 50 Hz | ~10000 µs |
| 60 Hz | ~8333 µs |

> 📘 For most applications, the AC_FREQ (0x20) register is sufficient.

### CALIBRATION (0x23) — Calibration Status

| Value | Status |
|-------|--------|
| 0 | Calibration in progress |
| 1 | Calibration complete |


---


## I2C Operations

### Writing to a Register

```
START → [Address+W] → ACK → [Register] → ACK → [Data] → ACK → STOP
```

Example — set brightness to 50%:
```
START → 0xA0 → ACK → 0x10 → ACK → 0x32 → ACK → STOP
```

### Reading from a Register

```
START → [Address+W] → ACK → [Register] → ACK →
RESTART → [Address+R] → ACK → [Data] → NACK → STOP
```

Example — read brightness:
```
START → 0xA0 → ACK → 0x10 → ACK →
RESTART → 0xA1 → ACK → [data] → NACK → STOP
```

**Addresses:**
- Write: 0xA0 (0x50 << 1)
- Read: 0xA1 (0x50 << 1 | 1)

---

## Code Examples

### Arduino (Wire.h)

```cpp
#include <Wire.h>

#define DIMMER_ADDR   0x50
#define REG_STATUS    0x00
#define REG_ERROR     0x02
#define REG_LEVEL     0x10
#define REG_CURVE     0x11
#define REG_FREQ      0x20

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (isReady()) {
        Serial.println("DimmerLink ready!");
        Serial.print("Mains frequency: ");
        Serial.print(getFrequency());
        Serial.println(" Hz");
    }
}

// Check device readiness
bool isReady() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_STATUS);
    Wire.endTransmission(false);

    Wire.requestFrom(DIMMER_ADDR, 1);
    if (Wire.available()) {
        return (Wire.read() & 0x01) != 0;
    }
    return false;
}

// Set brightness (0-100%)
bool setLevel(uint8_t level) {
    if (level > 100) return false;

    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_LEVEL);
    Wire.write(level);
    return Wire.endTransmission() == 0;
}

// Get current brightness
int getLevel() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_LEVEL);
    Wire.endTransmission(false);

    Wire.requestFrom(DIMMER_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

// Set curve (0=LINEAR, 1=RMS, 2=LOG)
bool setCurve(uint8_t curve) {
    if (curve > 2) return false;

    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_CURVE);
    Wire.write(curve);
    return Wire.endTransmission() == 0;
}

// Get curve type
int getCurve() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_CURVE);
    Wire.endTransmission(false);

    Wire.requestFrom(DIMMER_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

// Get mains frequency
int getFrequency() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_FREQ);
    Wire.endTransmission(false);

    Wire.requestFrom(DIMMER_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

// Get error code
uint8_t getError() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_ERROR);
    Wire.endTransmission(false);

    Wire.requestFrom(DIMMER_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

void loop() {
    // Smooth brightness change
    for (int level = 0; level <= 100; level += 10) {
        setLevel(level);
        Serial.print("Brightness: ");
        Serial.print(level);
        Serial.println("%");
        delay(500);
    }

    delay(1000);

    for (int level = 100; level >= 0; level -= 10) {
        setLevel(level);
        delay(500);
    }

    delay(1000);
}
```

### Python (smbus2) — Raspberry Pi

```python
from smbus2 import SMBus
import time

class DimmerLink:
    ADDR = 0x50

    # Registers
    REG_STATUS = 0x00
    REG_COMMAND = 0x01
    REG_ERROR = 0x02
    REG_VERSION = 0x03
    REG_LEVEL = 0x10
    REG_CURVE = 0x11
    REG_FREQ = 0x20

    # Curves
    CURVE_LINEAR = 0
    CURVE_RMS = 1
    CURVE_LOG = 2

    def __init__(self, bus_number=1):
        self.bus = SMBus(bus_number)

    def is_ready(self):
        """Check if device is ready"""
        status = self.bus.read_byte_data(self.ADDR, self.REG_STATUS)
        return (status & 0x01) != 0

    def get_version(self):
        """Get firmware version"""
        return self.bus.read_byte_data(self.ADDR, self.REG_VERSION)

    def set_level(self, level):
        """Set brightness 0-100%"""
        if not 0 <= level <= 100:
            raise ValueError("Level must be 0-100")
        self.bus.write_byte_data(self.ADDR, self.REG_LEVEL, level)

    def get_level(self):
        """Get current brightness"""
        return self.bus.read_byte_data(self.ADDR, self.REG_LEVEL)

    def set_curve(self, curve):
        """Set curve: 0=LINEAR, 1=RMS, 2=LOG"""
        if curve not in [0, 1, 2]:
            raise ValueError("Curve must be 0, 1, or 2")
        self.bus.write_byte_data(self.ADDR, self.REG_CURVE, curve)

    def get_curve(self):
        """Get curve type"""
        return self.bus.read_byte_data(self.ADDR, self.REG_CURVE)

    def get_frequency(self):
        """Get mains frequency (50 or 60 Hz)"""
        return self.bus.read_byte_data(self.ADDR, self.REG_FREQ)

    def get_error(self):
        """Get last error code"""
        return self.bus.read_byte_data(self.ADDR, self.REG_ERROR)

    def reset(self):
        """Software reset"""
        self.bus.write_byte_data(self.ADDR, self.REG_COMMAND, 0x01)

    def close(self):
        self.bus.close()


# Usage example
if __name__ == "__main__":
    dimmer = DimmerLink()

    if dimmer.is_ready():
        print(f"Firmware version: {dimmer.get_version()}")
        print(f"Mains frequency: {dimmer.get_frequency()} Hz")

        # Smooth brightness change
        for level in range(0, 101, 10):
            dimmer.set_level(level)
            print(f"Brightness: {level}%")
            time.sleep(0.5)

        # Set RMS curve for incandescent bulbs
        dimmer.set_curve(DimmerLink.CURVE_RMS)
        print("Curve: RMS")

    dimmer.close()
```

### MicroPython (ESP32, Raspberry Pi Pico)

```python
from machine import I2C, Pin
import time

class DimmerLink:
    ADDR = 0x50

    # Registers
    REG_STATUS = 0x00
    REG_COMMAND = 0x01
    REG_ERROR = 0x02
    REG_LEVEL = 0x10
    REG_CURVE = 0x11
    REG_FREQ = 0x20

    def __init__(self, i2c_id=0, scl_pin=22, sda_pin=21):
        """
        i2c_id: I2C bus number (usually 0)
        scl_pin, sda_pin: I2C pins
            - ESP32: scl=22, sda=21
            - Raspberry Pi Pico: scl=5, sda=4
        """
        self.i2c = I2C(i2c_id, scl=Pin(scl_pin), sda=Pin(sda_pin), freq=100000)

    def is_ready(self):
        """Check if device is ready"""
        data = self.i2c.readfrom_mem(self.ADDR, self.REG_STATUS, 1)
        return (data[0] & 0x01) != 0

    def set_level(self, level):
        """Set brightness 0-100%"""
        self.i2c.writeto_mem(self.ADDR, self.REG_LEVEL, bytes([level]))

    def get_level(self):
        """Get current brightness"""
        data = self.i2c.readfrom_mem(self.ADDR, self.REG_LEVEL, 1)
        return data[0]

    def set_curve(self, curve):
        """Set curve: 0=LINEAR, 1=RMS, 2=LOG"""
        self.i2c.writeto_mem(self.ADDR, self.REG_CURVE, bytes([curve]))

    def get_curve(self):
        """Get curve type"""
        data = self.i2c.readfrom_mem(self.ADDR, self.REG_CURVE, 1)
        return data[0]

    def get_frequency(self):
        """Get mains frequency"""
        data = self.i2c.readfrom_mem(self.ADDR, self.REG_FREQ, 1)
        return data[0]


# Usage example
dimmer = DimmerLink()

if dimmer.is_ready():
    print(f"Mains frequency: {dimmer.get_frequency()} Hz")

    while True:
        for level in range(0, 101, 10):
            dimmer.set_level(level)
            print(f"Brightness: {level}%")
            time.sleep(0.5)

        for level in range(100, -1, -10):
            dimmer.set_level(level)
            time.sleep(0.5)
```

### CircuitPython (Adafruit)

```python
import board
import busio
import time

class DimmerLink:
    ADDR = 0x50
    REG_LEVEL = 0x10
    REG_CURVE = 0x11
    REG_FREQ = 0x20

    def __init__(self):
        self.i2c = busio.I2C(board.SCL, board.SDA, frequency=100000)
        # Wait with timeout
        timeout = 100
        while not self.i2c.try_lock():
            timeout -= 1
            if timeout == 0:
                raise RuntimeError("I2C bus is busy")
            time.sleep(0.01)
            pass

    def set_level(self, level):
        """Set brightness 0-100%"""
        self.i2c.writeto(self.ADDR, bytes([self.REG_LEVEL, level]))

    def get_level(self):
        """Get current brightness"""
        result = bytearray(1)
        self.i2c.writeto_then_readfrom(self.ADDR, bytes([self.REG_LEVEL]), result)
        return result[0]

    def get_frequency(self):
        """Get mains frequency"""
        result = bytearray(1)
        self.i2c.writeto_then_readfrom(self.ADDR, bytes([self.REG_FREQ]), result)
        return result[0]

    def close(self):
        self.i2c.unlock()


# Usage example
dimmer = DimmerLink()

print(f"Mains frequency: {dimmer.get_frequency()} Hz")

for level in range(0, 101, 10):
    dimmer.set_level(level)
    print(f"Brightness: {level}%")
    time.sleep(0.5)

dimmer.close()
```

---

## Comparison with UART

| Aspect | I2C | UART |
|--------|-----|------|
| **Advantages** | Register access, simpler code | Works with bridges |
| **Write example** | `write_byte_data(0x50, 0x10, 50)` | `write([0x02, 0x53, 0x00, 50])` |
| **Read example** | `read_byte_data(0x50, 0x10)` | Send command, read response |
| **Error code** | Read from ERROR register | Returned as response |

---

## Debugging

### Finding the Device

**Raspberry Pi:**
```bash
i2cdetect -y 1
```

Expected output — address `50` in the table.

**Arduino:**
```cpp
void scanI2C() {
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device found: 0x");
            Serial.println(addr, HEX);
        }
    }
}
```

### Common Problems

| Problem | Cause | Solution |
|---------|-------|----------|
| Device not found | No pull-up | Add 4.7kΩ on SDA and SCL |
| Device not found | Wrong address | Check 0x50 (or changed address) |
| ERROR = 0xFE | Invalid parameter | level > 100, curve > 2 or address outside 0x08-0x77 |
| Unstable connection | Long wires | Shorten wires or reduce pull-up value |
| No response after address change | Using old address | Reconnect using the new address |

### Command Line Read/Write (Linux)

```bash
# Read brightness
i2cget -y 1 0x50 0x10

# Set brightness to 50%
i2cset -y 1 0x50 0x10 0x32

# Read mains frequency
i2cget -y 1 0x50 0x20
```

---

## What's Next?

- [UART Interface](03_UART_COMMUNICATION.md) — alternative control method
- [Raspberry Pi](05_SINGLE_BOARD_COMPUTERS.md) — more on single board computers
- [Code Examples](examples/) — ready-to-use scripts
- [FAQ](07_FAQ_TROUBLESHOOTING.md) — troubleshooting
