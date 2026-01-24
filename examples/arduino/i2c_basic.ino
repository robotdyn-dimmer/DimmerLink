/**
 * DimmerLink - I2C Basic Example
 *
 * Simple example of dimmer control via I2C.
 * I2C is recommended for local control - faster and more stable than UART.
 *
 * Wiring Arduino Uno/Nano:
 *   Arduino    DimmerLink
 *   A4 (SDA) → SDA
 *   A5 (SCL) → SCL
 *   5V       → VCC
 *   GND      → GND
 *
 * Wiring ESP32:
 *   ESP32      DimmerLink
 *   GPIO21   → SDA
 *   GPIO22   → SCL
 *   3.3V     → VCC
 *   GND      → GND
 *
 * Pull-up resistors: 4.7kΩ on SDA and SCL to VCC
 * (Arduino has built-in weak pull-ups, but external ones are recommended)
 *
 * Documentation: https://rbdimmer.com/docs/
 */

#include <Wire.h>

// I2C address of DimmerLink (default 0x50, can be changed)
#define DIMMER_ADDR  0x50

// Registers
#define REG_STATUS   0x00   // Device status (R)
#define REG_COMMAND  0x01   // Control commands (W)
#define REG_ERROR    0x02   // Last error code (R)
#define REG_LEVEL    0x10   // Brightness 0-100% (R/W)
#define REG_CURVE    0x11   // Dimming curve (R/W)
#define REG_FREQ     0x20   // Mains frequency Hz (R)

// Dimming curve types
#define CURVE_LINEAR 0      // Linear (universal)
#define CURVE_RMS    1      // RMS (incandescent, halogen)
#define CURVE_LOG    2      // Logarithmic (LED)

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }  // Wait for Serial for Leonardo/ESP32

    Wire.begin();

    // For ESP32 you can specify different pins:
    // Wire.begin(21, 22);  // SDA, SCL

    Serial.println("=================================");
    Serial.println("DimmerLink I2C Example");
    Serial.println("=================================");

    // Connection check - request mains frequency
    int freq = getFrequency();
    if (freq > 0) {
        Serial.print("Device ready! AC frequency: ");
        Serial.print(freq);
        Serial.println(" Hz");
    } else {
        Serial.println("Device not responding!");
        Serial.println("  Check connections:");
        Serial.println("  - SDA -> SDA (not crossed!)");
        Serial.println("  - SCL -> SCL");
        Serial.println("  - Pull-up resistors 4.7k ohm");
    }

    Serial.println();
}

/**
 * Set brightness (0-100%)
 *
 * I2C is fast - can be called frequently for smooth transitions.
 * Returns true if command was sent successfully.
 */
bool setLevel(uint8_t level) {
    if (level > 100) {
        Serial.println("Error: level must be 0-100");
        return false;
    }

    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_LEVEL);
    Wire.write(level);
    return (Wire.endTransmission() == 0);
}

/**
 * Get current brightness
 * Returns 0-100% or -1 on error
 */
int getLevel() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_LEVEL);
    if (Wire.endTransmission(false) != 0) {
        return -1;
    }

    Wire.requestFrom((uint8_t)DIMMER_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

/**
 * Set dimming curve
 *
 * @param curve: CURVE_LINEAR (0), CURVE_RMS (1), CURVE_LOG (2)
 *
 * LINEAR - universal, linear power
 * RMS    - for incandescent lamps (linear brightness)
 * LOG    - for LED (matches eye perception)
 */
bool setCurve(uint8_t curve) {
    if (curve > 2) {
        Serial.println("Error: curve must be 0, 1, or 2");
        return false;
    }

    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_CURVE);
    Wire.write(curve);
    return (Wire.endTransmission() == 0);
}

/**
 * Get curve type
 * Returns 0-2 or -1 on error
 */
int getCurve() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_CURVE);
    if (Wire.endTransmission(false) != 0) {
        return -1;
    }

    Wire.requestFrom((uint8_t)DIMMER_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

/**
 * Get mains frequency
 * Returns 50 or 60 Hz, or -1 on error
 */
int getFrequency() {
    Wire.beginTransmission(DIMMER_ADDR);
    Wire.write(REG_FREQ);
    if (Wire.endTransmission(false) != 0) {
        return -1;
    }

    Wire.requestFrom((uint8_t)DIMMER_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

/**
 * Smooth brightness change (fade)
 *
 * @param from:     start level (0-100)
 * @param to:       end level (0-100)
 * @param duration: transition time in ms
 */
void fadeTo(uint8_t from, uint8_t to, unsigned int duration) {
    int steps = abs(to - from);
    if (steps == 0) return;

    int stepDelay = duration / steps;
    int direction = (to > from) ? 1 : -1;

    for (int level = from; level != to; level += direction) {
        setLevel(level);
        delay(stepDelay);
    }
    setLevel(to);
}

void loop() {
    // === Demo 1: Smooth brightness change ===
    Serial.println("Demo 1: Smooth fade (I2C is fast!)");

    Serial.println("  Fading up 0% -> 100%...");
    fadeTo(0, 100, 2000);  // 2 seconds

    delay(500);

    Serial.println("  Fading down 100% -> 0%...");
    fadeTo(100, 0, 2000);

    delay(1000);

    // === Demo 2: Compare curves at 50% ===
    Serial.println("\nDemo 2: Comparing curves at 50%");
    Serial.println("  (observe brightness difference)");

    setCurve(CURVE_LINEAR);
    setLevel(50);
    Serial.println("  LINEAR curve - 50%");
    delay(2000);

    setCurve(CURVE_RMS);
    setLevel(50);
    Serial.println("  RMS curve - 50% (brighter for incandescent)");
    delay(2000);

    setCurve(CURVE_LOG);
    setLevel(50);
    Serial.println("  LOG curve - 50% (natural for LED)");
    delay(2000);

    // Return to LINEAR
    setCurve(CURVE_LINEAR);
    setLevel(0);

    Serial.println("\n--- Cycle complete, restarting... ---\n");
    delay(2000);
}
