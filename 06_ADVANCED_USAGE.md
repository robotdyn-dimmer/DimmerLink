# Advanced Usage

Non-standard ways to connect and control DimmerLink.

---

## USB-UART Adapters

Control DimmerLink from a computer via USB-UART adapter.

### Popular Adapters

| Chip | Driver | Notes |
|------|--------|-------|
| CH340/CH341 | Often built into OS | Cheap, common |
| CP2102/CP2104 | Silicon Labs | Stable |
| FT232RL | FTDI | Professional |
| PL2303 | Prolific | Outdated, driver issues |

### Wiring

| USB-UART | DimmerLink |
|----------|------------|
| VCC (3.3V or 5V) | VCC |
| GND | GND |
| TXD | RX |
| RXD | TX |

> **ℹ️ Note:** DimmerLink supports 1.8V, 3.3V and 5V logic levels — use whichever voltage your adapter provides.

### Drivers

**Windows:**
- CH340: usually installs automatically, or download from manufacturer's website
- CP2102: [Silicon Labs VCP Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- FTDI: [FTDI VCP Driver](https://ftdichip.com/drivers/vcp-drivers/)

**Linux:**
- Drivers are usually already in the kernel
- Device will appear as `/dev/ttyUSB0` or `/dev/ttyACM0`

**macOS:**
- CH340: may require driver from manufacturer
- CP2102/FTDI: built into the system

### Control from PC (Python)

```python
import serial
import time

# Windows: 'COM3', Linux: '/dev/ttyUSB0', macOS: '/dev/tty.usbserial-*'
ser = serial.Serial('COM3', 115200, timeout=0.1)

def set_level(level):
    ser.write(bytes([0x02, 0x53, 0x00, level]))
    resp = ser.read(1)
    return len(resp) > 0 and resp[0] == 0x00

def get_frequency():
    ser.write(bytes([0x02, 0x52]))
    resp = ser.read(2)
    if len(resp) == 2 and resp[0] == 0x00:
        return resp[1]
    return None

# Usage
print(f"Mains frequency: {get_frequency()} Hz")
set_level(50)
print("Brightness: 50%")

ser.close()
```

### Terminal Programs

For debugging and testing:

| Program | Platform | HEX Mode |
|---------|----------|----------|
| RealTerm | Windows | Yes |
| SSCOM | Windows | Yes |
| CoolTerm | Windows/Mac/Linux | Yes |
| PuTTY | Windows/Linux | No (text only) |
| picocom | Linux | No |

**Example in RealTerm:**
1. Port → select your COM port
2. Baud: 115200
3. Send → "Send Numbers" tab
4. Enter: `02 53 00 32` (HEX)
5. Click "Send Numbers"

---

## WiFi-UART (ESP-01)

Wireless control via ESP-01 or ESP8266.

### Diagram

```
[Computer/Phone] ←WiFi→ [ESP-01] ←UART→ [DimmerLink] → [Dimmer]
```

### ESP-01 Wiring

| ESP-01 | DimmerLink |
|--------|------------|
| VCC | VCC (3.3V) |
| GND | GND |
| TX | RX |
| RX | TX |

### ESP-01 Firmware (Arduino IDE)

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

ESP8266WebServer server(80);

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    server.on("/set", handleSet);
    server.on("/get", handleGet);
    server.begin();
}

void handleSet() {
    if (server.hasArg("level")) {
        int level = server.arg("level").toInt();
        if (level >= 0 && level <= 100) {
            uint8_t cmd[] = {0x02, 0x53, 0x00, (uint8_t)level};
            Serial.write(cmd, 4);

            delay(10);
            if (Serial.available()) {
                uint8_t resp = Serial.read();
                server.send(200, "text/plain", resp == 0x00 ? "OK" : "ERROR");
            } else {
                server.send(500, "text/plain", "NO_RESPONSE");
            }
        } else {
            server.send(400, "text/plain", "INVALID_LEVEL");
        }
    } else {
        server.send(400, "text/plain", "MISSING_LEVEL");
    }
}

void handleGet() {
    uint8_t cmd[] = {0x02, 0x47, 0x00};
    Serial.write(cmd, 3);

    delay(10);
    if (Serial.available() >= 2) {
        uint8_t status = Serial.read();
        uint8_t level = Serial.read();  // Already in percent 0-100!
        if (status == 0x00) {
            server.send(200, "text/plain", String(level));
        } else {
            server.send(500, "text/plain", "ERROR");
        }
    } else {
        server.send(500, "text/plain", "NO_RESPONSE");
    }
}

void loop() {
    server.handleClient();
}
```

### Usage

```bash
# Set brightness to 50%
curl "http://192.168.1.100/set?level=50"

# Get current brightness
curl "http://192.168.1.100/get"
```

---

## Bluetooth (HC-05/HC-06)

Control from smartphone or computer via Bluetooth.

### Wiring

| HC-05/HC-06 | DimmerLink |
|-------------|------------|
| VCC | VCC (3.3V or 5V) |
| GND | GND |
| TXD | RX |
| RXD | TX |

> **📝 Note:** HC-05 defaults to 9600 baud. Reconfigure to 115200 via AT commands.

### Configuring HC-05 (AT Commands)

1. Connect HC-05 in AT mode (hold button while powering on)
2. Open terminal at 38400 baud
3. Enter:
   ```
   AT+UART=115200,0,0
   AT+NAME=Dimmer
   AT+PSWD=1234
   ```

### Android App

Use any Bluetooth Serial app:
- Serial Bluetooth Terminal
- Bluetooth Electronics

Send HEX commands directly.

---

## LoRa Modules

Long-range control via LoRa (up to several kilometers).

### Diagram

```
[Controller + LoRa TX] ~~~radio~~~ [LoRa RX + DimmerLink]
```

> ⚠️ **E32 Setup:** E32 modules require pre-configuration via [RF Setting](http://www.ebyte.com/en/downpdf.aspx?id=132) — speed, channel, address. Default: 9600 baud — needs to be changed to 115200 for DimmerLink, or use an MCU as a bridge for speed conversion.

### Popular Modules

- E32 (SX1278) — simple UART interface
- Ra-02 — requires SPI library
- RFM95 — for LoRaWAN

### E32-TTL-100 Wiring

| E32 | DimmerLink |
|-----|------------|
| VCC | VCC (3.3V or 5V) |
| GND | GND |
| TXD | RX |
| RXD | TX |

### Considerations

- **Latency:** 50-200 ms depending on settings
- **Bandwidth:** limited (1-50 kbps)
- **Reliability:** use acknowledgment and retries

### Example (Transmitter)

```cpp
// Arduino + E32 (transmitter)
void sendCommand(uint8_t* cmd, int len) {
    Serial1.write(cmd, len);  // Send via LoRa
}

void loop() {
    // Set brightness to 50%
    uint8_t cmd[] = {0x02, 0x53, 0x00, 0x32};
    sendCommand(cmd, 4);
    delay(1000);
}
```

---

## GSM/GPRS Modules

Remote control via SMS or internet.

### Popular Modules

- SIM800L — compact, 2G
- SIM900 — classic
- SIM7600 — 4G LTE

### Diagram

```
[Server/Phone] ←GSM→ [SIM800L + MCU] ←UART→ [DimmerLink]
```

### SIM800L Wiring

| SIM800L | Arduino/ESP |
|---------|-------------|
| VCC | 4V (separate power!) |
| GND | GND |
| TXD | RX |
| RXD | TX |

> **📝 Note:** SIM800L requires stable 3.7-4.2V power with up to 2A current during transmission.

### Control via SMS

```cpp
#include <SoftwareSerial.h>

SoftwareSerial gsm(7, 8);  // RX, TX for SIM800L
SoftwareSerial dimmer(10, 11);  // RX, TX for DimmerLink

void setup() {
    gsm.begin(9600);
    dimmer.begin(115200);

    // Configure SIM800L for SMS
    gsm.println("AT+CMGF=1");  // Text mode
    delay(100);
    gsm.println("AT+CNMI=2,2,0,0,0");  // SMS notifications
    delay(100);
}

void loop() {
    if (gsm.available()) {
        String message = gsm.readString();

        // Parse SMS "SET 50"
        if (message.indexOf("SET ") >= 0) {
            int idx = message.indexOf("SET ") + 4;
            int level = message.substring(idx).toInt();

            if (level >= 0 && level <= 100) {
                uint8_t cmd[] = {0x02, 0x53, 0x00, (uint8_t)level};
                dimmer.write(cmd, 4);
            }
        }
    }
}
```

---

## Wireless Communication Considerations

### Latency

| Connection Type | Typical Latency |
|-----------------|-----------------|
| USB-UART | < 1 ms |
| WiFi (local network) | 5-50 ms |
| Bluetooth | 10-50 ms |
| LoRa | 50-500 ms |
| GSM (SMS) | 1-10 sec |
| GSM (GPRS) | 100-500 ms |

### Buffering

With wireless communication, data may be buffered. Recommendations:

1. **Send commands as a whole** — don't split into individual bytes
2. **Add delay** between commands (50-100 ms)
3. **Wait for acknowledgment** before the next command

### Reliability

For critical applications:

1. **Check the response** — command succeeded only when receiving `0x00`
2. **Retry on error** — 2-3 attempts with delay
3. **Timeout** — if no response within 1-2 seconds, retry

```python
def reliable_set_level(ser, level, retries=3):
    for attempt in range(retries):
        ser.write(bytes([0x02, 0x53, 0x00, level]))
        ser.flush()

        resp = ser.read(1)
        if resp and resp[0] == 0x00:
            return True

        time.sleep(0.1)

    return False
```

---

## I2C Limitations Through Bridges

I2C is **not suitable** for wireless communication due to:
- Strict timing requirements (clock stretching)
- Lack of buffering in the protocol
- Need for bidirectional synchronous communication

**Solution:** For wireless control, use **UART**.

If you have I2C connection and need wireless access — add an MCU (Arduino/ESP) as a bridge:
```
[WiFi/BT] → [ESP32 (UART)] → [DimmerLink (I2C)]
```

---

## What's Next?

- [FAQ](07_FAQ_TROUBLESHOOTING.md) — troubleshooting
- [UART Commands](03_UART_COMMUNICATION.md) — full command list
- [Code Examples](examples/) — ready-to-use scripts
