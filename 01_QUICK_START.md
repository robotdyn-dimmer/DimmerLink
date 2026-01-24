# Quick Start

This guide will help you get DimmerLink up and running in minutes.

---

## What You'll Need

1. **DimmerLink** — controller board
2. **Dimmer** — TRIAC module with external control support
3. **Microcontroller or SBC** — Arduino, ESP32, Raspberry Pi, etc.
4. **Jumper wires**
5. **Lamp** for testing (incandescent or dimmable LED)

---

## Step 1: Choose an Interface

| Feature | UART | I2C |
|---------|------|-----|
| Wiring | TX/RX crossed | SDA/SCL direct |
| Code complexity | Command packets | Register access |
| **Recommendation** | For advanced users | **For beginners** |

> **💡 Tip:** We recommend starting with I2C — simpler code, easier debugging.

---

## Step 2: Wiring

### DimmerLink Connectors

**Input (to your project):**

| Pin | Function |
|-----|----------|
| VCC | Power 3.3V |
| GND | Ground |
| TX/SDA | UART TX or I2C SDA |
| RX/SCL | UART RX or I2C SCL |

**Output (to dimmer module):**

| Pin | Function |
|-----|----------|
| VCC | Power |
| GND | Ground |
| Z-C | Zero-Cross signal |
| Dim | TRIAC control |

### Connection Diagram

<!-- TODO: Add connection diagram image -->

```
[Your Project] ←→ [DimmerLink] ←→ [Dimmer] ←→ [Mains + Lamp]
```

**Detailed wiring diagrams for different boards:** [Hardware Connection](02_HARDWARE_CONNECTION.md)

**Detailed dimmer and load connection diagrams (lamps, heaters):** [TODO: Website link]

---

## Step 3: Upload Code

### Option A: I2C (Recommended)

**Arduino:**

```cpp
#include <Wire.h>

#define DIMMER_ADDR 0x50
#define REG_LEVEL   0x10

void setup() {
    Wire.begin();
}

void loop() {
    // Smooth brightness change
    for (int level = 0; level <= 100; level += 10) {
        setLevel(level);
        delay(500);
    }
    for (int level = 100; level >= 0; level -= 10) {
        setLevel(level);
        delay(500);
    }
}

void setLevel(uint8_t level) {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_LEVEL);
    Wire.write(level);
    Wire.endTransmission();
}
```

**MicroPython (ESP32, Raspberry Pi Pico):**

```python
from machine import I2C, Pin
import time

# ESP32: scl=22, sda=21
# Raspberry Pi Pico: scl=5, sda=4
i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=100000)
DIMMER_ADDR = 0x50
REG_LEVEL = 0x10

def set_level(level):
    i2c.writeto_mem(DIMMER_ADDR, REG_LEVEL, bytes([level]))

# Smooth brightness change
while True:
    for level in range(0, 101, 10):
        set_level(level)
        time.sleep(0.5)
```

**Python (Raspberry Pi):**

```python
from smbus2 import SMBus
import time

bus = SMBus(1)
DIMMER_ADDR = 0x50
REG_LEVEL = 0x10

def set_level(level):
    bus.write_byte_data(DIMMER_ADDR, REG_LEVEL, level)

# Set brightness to 50%
set_level(50)
```

---

### Option B: UART

**Arduino:**

```cpp
// Use Serial1 (or SoftwareSerial for Uno)
#define DIMMER_SERIAL Serial1

void setup() {
    DIMMER_SERIAL.begin(115200);
    checkConnection();
}

void loop() {
    setLevel(50);  // 50%
    delay(2000);
    setLevel(100); // 100%
    delay(2000);
}

void setLevel(uint8_t level) {
    uint8_t cmd[] = {0x02, 0x53, 0x00, level};
    DIMMER_SERIAL.write(cmd, 4);

    // Wait for response
    delay(10);
    if (DIMMER_SERIAL.available()) {
        uint8_t response = DIMMER_SERIAL.read();
        // 0x00 = OK
    }
}

// Connection check — request mains frequency
void checkConnection() {
    uint8_t cmd[] = {0x02, 0x52};
    Serial1.write(cmd, 2);

    delay(50);
    if (Serial1.available() >= 2) {
        uint8_t status = Serial1.read();
        uint8_t freq = Serial1.read();
        if (status == 0x00) {
            Serial.print("OK! Mains frequency: ");
            Serial.print(freq);
            Serial.println(" Hz");
        }
    }
}
```

**Python:**

```python
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)

def set_level(level):
    cmd = bytes([0x02, 0x53, 0x00, level])
    ser.write(cmd)
    response = ser.read(1)
    return len(response) > 0 and response[0] == 0x00

# Set brightness to 50%
if set_level(50):
    print("OK")
else:
    print("Error")
```

---

## Step 4: Verify Operation

1. **Upload the code** to your microcontroller
2. **Power up** DimmerLink
3. **Observe** — the lamp should change brightness

---

## Connection Verification

### I2C — Device Scan

**Arduino:**

```cpp
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.println("Scanning for I2C devices...");

    Wire.beginTransmission(0x50);
    if (Wire.endTransmission() == 0) {
        Serial.println("DimmerLink found at 0x50");
    } else {
        Serial.println("Device not found!");
    }
}

void loop() {}
```

**Raspberry Pi (command line):**

```bash
# Install if not present:
sudo apt install i2c-tools

# Scan for devices:
i2cdetect -y 1
```

Expected output — `50` at the intersection of row 5 and column 0.

### UART — Response Check

Send the mains frequency request command:

```
HEX: 02 52
```

Expected response:
- `00 32` — OK, frequency 50 Hz
- `00 3C` — OK, frequency 60 Hz

---

## Not Working?

| Problem | Solution |
|---------|----------|
| No response | Check wiring and power |
| Error 0xFC | EEPROM write error |
| I2C doesn't see device | Check connections and pull-up resistors |
| Incorrect brightness | Check logic levels (3.3V/5V) |

**More details:** [FAQ & Troubleshooting](07_FAQ_TROUBLESHOOTING.md)

---

## What's Next?

- [Wiring for Different Boards](02_HARDWARE_CONNECTION.md)
- [All UART Commands](03_UART_COMMUNICATION.md)
- [All I2C Registers](04_I2C_COMMUNICATION.md)
- [Code Examples](examples/)
