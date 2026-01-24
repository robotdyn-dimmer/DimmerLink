/**
 * DimmerLink - UART Basic Example
 *
 * Example of dimmer control via UART.
 * UART is convenient for remote control and working through bridges (USB, WiFi, LoRa).
 *
 * ⚠️ IMPORTANT for Arduino Uno/Nano:
 * SoftwareSerial at 115200 baud may be UNSTABLE!
 * If communication errors occur, we recommend:
 *   - Using Arduino Mega/Due with hardware Serial1
 *   - Or using I2C interface (see i2c_basic.ino)
 *
 * Wiring (SoftwareSerial):
 *   Arduino    DimmerLink
 *   Pin 10   → RX  (Arduino TX → DimmerLink RX)
 *   Pin 11   → TX  (Arduino RX ← DimmerLink TX)
 *   5V       → VCC
 *   GND      → GND
 *
 * Wiring Arduino Mega (Serial1):
 *   Arduino    DimmerLink
 *   TX1 (18) → RX
 *   RX1 (19) → TX
 *   5V       → VCC
 *   GND      → GND
 *
 * Documentation: https://rbdimmer.com/docs/
 */

// ============================================================
// SELECT ONE OPTION:
// ============================================================

// Option 1: SoftwareSerial for Arduino Uno/Nano
// ⚠️ May be unstable at 115200!
#define USE_SOFTWARE_SERIAL

// Option 2: Hardware Serial1 for Mega/Due/ESP32
// Uncomment line below and comment out USE_SOFTWARE_SERIAL
// #define USE_HARDWARE_SERIAL

// ============================================================

#ifdef USE_SOFTWARE_SERIAL
    #include <SoftwareSerial.h>
    SoftwareSerial dimmerSerial(10, 11);  // RX, TX
#else
    #define dimmerSerial Serial1
#endif

// UART commands (all start with 0x02)
#define CMD_START       0x02    // Start byte (required!)
#define CMD_SET         0x53    // 'S' - set brightness
#define CMD_GET         0x47    // 'G' - get brightness
#define CMD_CURVE       0x43    // 'C' - set curve
#define CMD_GETCURVE    0x51    // 'Q' - get curve
#define CMD_FREQ        0x52    // 'R' - get mains frequency
#define CMD_RESET       0x58    // 'X' - device reset
#define CMD_SWITCH_I2C  0x5B    // '[' - switch to I2C

// Response codes
#define RESP_OK         0x00    // Success
#define RESP_ERR_SYNTAX 0xF9    // Invalid command format
#define RESP_ERR_EEPROM 0xFC    // EEPROM write error
#define RESP_ERR_INDEX  0xFD    // Invalid dimmer index
#define RESP_ERR_PARAM  0xFE    // Invalid parameter

// Curve types
#define CURVE_LINEAR    0
#define CURVE_RMS       1
#define CURVE_LOG       2

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

    dimmerSerial.begin(115200);

    Serial.println("=================================");
    Serial.println("DimmerLink UART Example");
    Serial.println("=================================");

    #ifdef USE_SOFTWARE_SERIAL
    Serial.println("Warning: Using SoftwareSerial (may be unstable)");
    Serial.println("    Consider I2C for better reliability");
    #else
    Serial.println("Using Hardware Serial1");
    #endif

    Serial.println();

    // Small delay for stabilization
    delay(100);

    // Clear buffer
    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    // Connection check - request mains frequency
    int freq = getFrequency();
    if (freq > 0) {
        Serial.print("Device ready! AC frequency: ");
        Serial.print(freq);
        Serial.println(" Hz");
    } else {
        Serial.println("Device not responding!");
        Serial.println("  Check connections:");
        Serial.println("  - TX -> RX (crossed!)");
        Serial.println("  - RX -> TX (crossed!)");
        Serial.println("  - Baud rate: 115200");
    }

    Serial.println();
}

/**
 * Set brightness (0-100%)
 *
 * ⚠️ UART limitation: don't call more than 5-10 times per second!
 * For smooth transitions use I2C.
 *
 * Command format: 0x02 0x53 IDX LEVEL
 */
bool setLevel(uint8_t level) {
    if (level > 100) {
        Serial.println("Error: level must be 0-100");
        return false;
    }

    // Clear input buffer
    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    uint8_t cmd[] = {CMD_START, CMD_SET, 0x00, level};
    dimmerSerial.write(cmd, 4);

    // Wait for response (1 byte)
    delay(50);
    if (dimmerSerial.available()) {
        uint8_t resp = dimmerSerial.read();
        if (resp != RESP_OK) {
            printError(resp);
            return false;
        }
        return true;
    }

    Serial.println("Error: No response");
    return false;
}

/**
 * Get current brightness
 *
 * Command format: 0x02 0x47 IDX
 * Response format: STATUS LEVEL
 *
 * Returns 0-100% or -1 on error
 */
int getLevel() {
    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    uint8_t cmd[] = {CMD_START, CMD_GET, 0x00};
    dimmerSerial.write(cmd, 3);

    delay(50);
    if (dimmerSerial.available() >= 2) {
        uint8_t status = dimmerSerial.read();
        uint8_t level = dimmerSerial.read();
        if (status == RESP_OK) {
            return level;
        }
        printError(status);
    }
    return -1;
}

/**
 * Set dimming curve
 *
 * @param curve: CURVE_LINEAR (0), CURVE_RMS (1), CURVE_LOG (2)
 *
 * Command format: 0x02 0x43 IDX CURVE
 */
bool setCurve(uint8_t curve) {
    if (curve > 2) {
        Serial.println("Error: curve must be 0, 1, or 2");
        return false;
    }

    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    uint8_t cmd[] = {CMD_START, CMD_CURVE, 0x00, curve};
    dimmerSerial.write(cmd, 4);

    delay(50);
    if (dimmerSerial.available()) {
        uint8_t resp = dimmerSerial.read();
        if (resp != RESP_OK) {
            printError(resp);
            return false;
        }
        return true;
    }
    return false;
}

/**
 * Get curve type
 *
 * Command format: 0x02 0x51 IDX
 * Response format: STATUS CURVE
 */
int getCurve() {
    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    uint8_t cmd[] = {CMD_START, CMD_GETCURVE, 0x00};
    dimmerSerial.write(cmd, 3);

    delay(50);
    if (dimmerSerial.available() >= 2) {
        uint8_t status = dimmerSerial.read();
        uint8_t curve = dimmerSerial.read();
        if (status == RESP_OK) {
            return curve;
        }
    }
    return -1;
}

/**
 * Get mains frequency
 *
 * Command format: 0x02 0x52
 * Response format: STATUS FREQ
 *
 * Returns 50 or 60 Hz, or -1 on error
 */
int getFrequency() {
    while (dimmerSerial.available()) {
        dimmerSerial.read();
    }

    uint8_t cmd[] = {CMD_START, CMD_FREQ};
    dimmerSerial.write(cmd, 2);

    delay(50);
    if (dimmerSerial.available() >= 2) {
        uint8_t status = dimmerSerial.read();
        uint8_t freq = dimmerSerial.read();
        if (status == RESP_OK) {
            return freq;
        }
    }
    return -1;
}

/**
 * Print error description
 */
void printError(uint8_t code) {
    Serial.print("Error 0x");
    Serial.print(code, HEX);
    Serial.print(": ");

    switch (code) {
        case RESP_OK:
            Serial.println("OK");
            break;
        case RESP_ERR_SYNTAX:
            Serial.println("Invalid command format");
            break;
        case RESP_ERR_EEPROM:
            Serial.println("EEPROM write error");
            break;
        case RESP_ERR_INDEX:
            Serial.println("Invalid dimmer index (use 0)");
            break;
        case RESP_ERR_PARAM:
            Serial.println("Invalid parameter value");
            break;
        default:
            Serial.println("Unknown error");
            break;
    }
}

void loop() {
    // === Demo: Step brightness change ===
    // UART is slower than I2C, so use larger steps

    Serial.println("Increasing brightness (step 10%)...");
    for (int level = 0; level <= 100; level += 10) {
        if (setLevel(level)) {
            Serial.print("  Level: ");
            Serial.print(level);
            Serial.println("%");
        }
        delay(300);  // Minimum 200-300 ms between UART commands
    }

    delay(1000);

    Serial.println("Decreasing brightness (step 10%)...");
    for (int level = 100; level >= 0; level -= 10) {
        if (setLevel(level)) {
            Serial.print("  Level: ");
            Serial.print(level);
            Serial.println("%");
        }
        delay(300);
    }

    delay(1000);

    // Read current level
    int currentLevel = getLevel();
    if (currentLevel >= 0) {
        Serial.print("Current level (read back): ");
        Serial.print(currentLevel);
        Serial.println("%");
    }

    Serial.println("\n--- Cycle complete, restarting... ---\n");
    delay(2000);
}
