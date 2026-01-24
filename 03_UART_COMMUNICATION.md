# UART Communication

Detailed description of the UART protocol for controlling DimmerLink.

---

## Connection Parameters

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None (N) |
| Stop Bits | 1 |
| Format | **8N1** |

> **ℹ️ Note:** UART interface is available immediately after power-up.

---

## Command Format

All commands start with the start byte `0x02` (STX — Start of Text):

```
┌──────────┬──────────┬──────────┬──────────┐
│  START   │   CMD    │   ARG1   │   ARG2   │
│   0x02   │  1 byte  │ optional │ optional │
└──────────┴──────────┴──────────┴──────────┘
```

> ⚠️ **Important:** Without the start byte `0x02`, the command will not be processed!

---

## Command Table

| Command | Code | Format | Description |
|---------|------|--------|-------------|
| **SET** | 0x53 ('S') | `02 53 IDX LEVEL` | Set brightness |
| **GET** | 0x47 ('G') | `02 47 IDX` | Get brightness |
| **CURVE** | 0x43 ('C') | `02 43 IDX TYPE` | Set dimming curve |
| **GETCURVE** | 0x51 ('Q') | `02 51 IDX` | Get curve type |
| **FREQ** | 0x52 ('R') | `02 52` | Get mains frequency |
| **RESET** | 0x58 ('X') | `02 58` | Software reset |
| **SWITCH_I2C** | 0x5B ('[') | `02 5B` | Switch to I2C |

### Parameters

- **IDX** — dimmer index (0-7, current version supports only 0. Values 1-7 reserved for future multi-channel versions)
- **LEVEL** — brightness 0-100 (percent)
- **TYPE** — curve type: 0=LINEAR, 1=RMS, 2=LOG

---

## Response Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | OK | Command executed successfully |
| 0xF9 | ERR_SYNTAX | Invalid format or unknown command |
| 0xFC | ERR_NOT_READY | EEPROM write error |
| 0xFD | ERR_INDEX | Invalid dimmer index |
| 0xFE | ERR_PARAM | Invalid parameter value |

---

## Command Descriptions

### SET — Set Brightness

**Format:** `02 53 IDX LEVEL`

```
Example: 02 53 00 32  → Set dimmer 0 to 50%
Response: 00          → OK
```

| LEVEL | Brightness |
|-------|------------|
| 0x00 (0) | 0% — off |
| 0x32 (50) | 50% |
| 0x64 (100) | 100% — full brightness |

### GET — Get Current Brightness

**Format:** `02 47 IDX`

```
Example: 02 47 00     → Request dimmer 0 brightness
Response: 00 32       → OK, level 50%
```

> **📝 Note:** GET returns value in percent (0-100), same as SET.

### CURVE — Set Dimming Curve

**Format:** `02 43 IDX TYPE`

| TYPE | Curve | Application |
|------|-------|-------------|
| 0 | LINEAR | Universal |
| 1 | RMS | Incandescent, halogen |
| 2 | LOG | LED (matches eye perception) |

```
Example: 02 43 00 01  → Set RMS curve for dimmer 0
Response: 00          → OK
```

### GETCURVE — Get Curve Type

**Format:** `02 51 IDX`

```
Example: 02 51 00     → Request dimmer 0 curve type
Response: 00 00       → OK, type LINEAR
```

### FREQ — Get Mains Frequency

**Format:** `02 52`

```
Example: 02 52        → Request frequency
Response: 00 32       → OK, 50 Hz (0x32 = 50)
Or:      00 3C       → OK, 60 Hz (0x3C = 60)
```

### RESET — Software Reset

**Format:** `02 58`

```
Example: 02 58        → Reset device
(no response — device reboots)
```

### SWITCH_I2C — Switch to I2C

**Format:** `02 5B`

```
Example: 02 5B        → Switch interface to I2C
Response: 00          → OK (last UART response)
```

After successful execution, UART is disabled, device switches to I2C mode. Further control only via I2C at address 0x50 (or configured address).

> **📝 Note:** Mode is saved in EEPROM and restored after reboot.

---

## Code Examples

### Arduino

```cpp
// Use Serial1 for boards with multiple UARTs (Mega, Due, ESP32)
// Or SoftwareSerial for Uno/Nano

// For Arduino Mega, Due, ESP32 — use Serial1, Serial2
// For Arduino Uno/Nano — use SoftwareSerial (see example below)
#define DIMMER_SERIAL Serial1

void setup() {
    DIMMER_SERIAL.begin(115200);
}

// Set brightness (0-100%)
bool setLevel(uint8_t level) {
    uint8_t cmd[] = {0x02, 0x53, 0x00, level};
    DIMMER_SERIAL.write(cmd, 4);

    delay(10);
    if (DIMMER_SERIAL.available()) {
        return DIMMER_SERIAL.read() == 0x00;
    }
    return false;
}

// Get current brightness (returns 0-100%)
int getLevel() {
    uint8_t cmd[] = {0x02, 0x47, 0x00};
    DIMMER_SERIAL.write(cmd, 3);

    delay(10);
    if (DIMMER_SERIAL.available() >= 2) {
        uint8_t status = DIMMER_SERIAL.read();
        uint8_t level = DIMMER_SERIAL.read();
        if (status == 0x00) {
            return level;
        }
    }
    return -1;
}

// Set curve (0=LINEAR, 1=RMS, 2=LOG)
bool setCurve(uint8_t curve) {
    uint8_t cmd[] = {0x02, 0x43, 0x00, curve};
    DIMMER_SERIAL.write(cmd, 4);

    delay(10);
    if (DIMMER_SERIAL.available()) {
        return DIMMER_SERIAL.read() == 0x00;
    }
    return false;
}

// Get mains frequency (50 or 60 Hz)
int getFrequency() {
    uint8_t cmd[] = {0x02, 0x52};
    DIMMER_SERIAL.write(cmd, 2);

    delay(10);
    if (DIMMER_SERIAL.available() >= 2) {
        uint8_t status = DIMMER_SERIAL.read();
        uint8_t freq = DIMMER_SERIAL.read();
        if (status == 0x00) {
            return freq;
        }
    }
    return -1;
}

void loop() {
    setLevel(50);   // 50%
    delay(2000);
    setLevel(100);  // 100%
    delay(2000);
}
```

### Arduino with SoftwareSerial (for Uno/Nano)

```cpp
#include <SoftwareSerial.h>

SoftwareSerial dimmerSerial(10, 11);  // RX, TX

void setup() {
    Serial.begin(115200);
    dimmerSerial.begin(115200);

    Serial.println("DimmerLink ready");
}

bool setLevel(uint8_t level) {
    uint8_t cmd[] = {0x02, 0x53, 0x00, level};
    dimmerSerial.write(cmd, 4);

    delay(10);
    if (dimmerSerial.available()) {
        return dimmerSerial.read() == 0x00;
    }
    return false;
}

void loop() {
    if (setLevel(50)) {
        Serial.println("Set to 50%: OK");
    } else {
        Serial.println("Set to 50%: ERROR");
    }
    delay(3000);
}
```

> ⚠️ **Note:** SoftwareSerial on Arduino Uno/Nano may be unstable at 115200 baud. If you experience communication errors, we recommend using I2C interface or a board with hardware UART (Arduino Mega, ESP32).

### Python (pyserial)

```python
import serial
import time

class DimmerLink:
    def __init__(self, port, baudrate=115200):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=0.1)
        except serial.SerialException as e:
            print(f"Connection error to {port}: {e}")
            print("Check:")
            print("  - Is USB-UART adapter connected?")
            print("  - Correct port? (Windows: COM3, Linux: /dev/ttyUSB0)")
            raise

    def set_level(self, level):
        """Set brightness 0-100%"""
        cmd = bytes([0x02, 0x53, 0x00, level])
        self.ser.write(cmd)
        resp = self.ser.read(1)
        return len(resp) > 0 and resp[0] == 0x00

    def get_level(self):
        """Get brightness 0-100%"""
        cmd = bytes([0x02, 0x47, 0x00])
        self.ser.write(cmd)
        resp = self.ser.read(2)
        if len(resp) == 2 and resp[0] == 0x00:
            return resp[1]
        return None

    def set_curve(self, curve_type):
        """Set curve: 0=LINEAR, 1=RMS, 2=LOG"""
        cmd = bytes([0x02, 0x43, 0x00, curve_type])
        self.ser.write(cmd)
        resp = self.ser.read(1)
        return len(resp) > 0 and resp[0] == 0x00

    def get_frequency(self):
        """Get mains frequency (50 or 60 Hz)"""
        cmd = bytes([0x02, 0x52])
        self.ser.write(cmd)
        resp = self.ser.read(2)
        if len(resp) == 2 and resp[0] == 0x00:
            return resp[1]
        return None

    def close(self):
        self.ser.close()


# Usage example
if __name__ == "__main__":
    # Windows: 'COM3', Linux: '/dev/ttyUSB0'
    dimmer = DimmerLink('/dev/ttyUSB0')

    print(f"Mains frequency: {dimmer.get_frequency()} Hz")

    # Smooth brightness change
    for level in range(0, 101, 10):
        if dimmer.set_level(level):
            print(f"Brightness: {level}%")
        time.sleep(0.5)

    dimmer.close()
```

### MicroPython (ESP32, Raspberry Pi Pico)

```python
from machine import UART, Pin
import time

class DimmerLink:
    def __init__(self, uart_id=1, tx_pin=17, rx_pin=16):
        self.uart = UART(uart_id, baudrate=115200, tx=Pin(tx_pin), rx=Pin(rx_pin))

    def set_level(self, level):
        """Set brightness 0-100%"""
        cmd = bytes([0x02, 0x53, 0x00, level])
        self.uart.write(cmd)
        time.sleep_ms(10)
        if self.uart.any():
            return self.uart.read(1)[0] == 0x00
        return False

    def get_level(self):
        """Get brightness 0-100%"""
        cmd = bytes([0x02, 0x47, 0x00])
        self.uart.write(cmd)
        time.sleep_ms(10)
        if self.uart.any() >= 2:
            resp = self.uart.read(2)
            if resp[0] == 0x00:
                return resp[1]
        return None

    def set_curve(self, curve_type):
        """Set curve: 0=LINEAR, 1=RMS, 2=LOG"""
        cmd = bytes([0x02, 0x43, 0x00, curve_type])
        self.uart.write(cmd)
        time.sleep_ms(10)
        if self.uart.any():
            return self.uart.read(1)[0] == 0x00
        return False

    def get_frequency(self):
        """Get mains frequency"""
        cmd = bytes([0x02, 0x52])
        self.uart.write(cmd)
        time.sleep_ms(10)
        if self.uart.any() >= 2:
            resp = self.uart.read(2)
            if resp[0] == 0x00:
                return resp[1]
        return None


# Usage example
dimmer = DimmerLink()

print(f"Mains frequency: {dimmer.get_frequency()} Hz")

while True:
    for level in range(0, 101, 10):
        dimmer.set_level(level)
        print(f"Brightness: {level}%")
        time.sleep(0.5)
```

---

## For Those Unfamiliar with HEX

HEX (hexadecimal) is a way of writing numbers.

| Decimal | HEX | Note |
|---------|-----|------|
| 0 | 0x00 | Zero |
| 50 | 0x32 | Fifty |
| 100 | 0x64 | One hundred |
| 255 | 0xFF | Maximum for 1 byte |

### How to Convert a Number to HEX

**Python:**
```python
level = 50
hex_value = hex(level)  # '0x32'
print(f"50 in HEX = {hex_value}")
```

**Arduino:**
```cpp
int level = 50;
Serial.print("50 in HEX = 0x");
Serial.println(level, HEX);  // Prints "32"
```

### Brightness HEX Cheat Sheet

| Brightness | HEX | SET Command |
|------------|-----|-------------|
| 0% (off) | 0x00 | `02 53 00 00` |
| 10% | 0x0A | `02 53 00 0A` |
| 25% | 0x19 | `02 53 00 19` |
| 50% | 0x32 | `02 53 00 32` |
| 75% | 0x4B | `02 53 00 4B` |
| 100% | 0x64 | `02 53 00 64` |

### Helper Function for Building Commands

**Python:**
```python
def make_set_command(level_percent):
    """Create SET command from percent"""
    return bytes([0x02, 0x53, 0x00, level_percent])

# Usage
cmd = make_set_command(75)  # 75%
print(f"Command: {cmd.hex()}")  # Prints: 0253004b
```

**Arduino:**
```cpp
void sendSetCommand(uint8_t level) {
    uint8_t cmd[] = {0x02, 0x53, 0x00, level};
    Serial1.write(cmd, 4);
}

// Usage
sendSetCommand(75);  // 75%
```

---

## Debugging

### Connection Test

Send the frequency request command:
```
TX: 02 52
RX: 00 32  (OK, 50 Hz)
```

### Common Errors

| Problem | Cause | Solution |
|---------|-------|----------|
| No response | Missing START byte | Add 0x02 at the beginning |
| No response | Wrong baud rate | Check 115200 |
| No response | Interface in I2C mode | Switch back to UART |
| 0xF9 | Unknown command | Check command code |
| 0xFC | EEPROM write error | Retry command |
| 0xFE | Invalid parameter | level > 100 or curve > 2 |

### Terminal Programs

For debugging you can use:
- **Windows**: RealTerm (HEX mode), SSCOM
- **Linux**: `picocom`, `minicom`
- **Cross-platform**: PuTTY, CoolTerm

---

## What's Next?

- [I2C Interface](04_I2C_COMMUNICATION.md) — alternative control method
- [Code Examples](examples/) — ready-to-use scripts
- [FAQ](07_FAQ_TROUBLESHOOTING.md) — troubleshooting
