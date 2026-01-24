"""
DimmerLink - I2C Example (MicroPython)

For ESP32, Raspberry Pi Pico and other MicroPython boards.

Wiring ESP32:
    ESP32          DimmerLink
    GPIO21 (SDA) → SDA
    GPIO22 (SCL) → SCL
    3.3V         → VCC
    GND          → GND

Wiring Raspberry Pi Pico:
    Pico         DimmerLink
    GP4 (SDA)  → SDA
    GP5 (SCL)  → SCL
    3V3        → VCC
    GND        → GND

Pull-up resistors: 4.7kΩ on SDA and SCL to VCC
(Pico doesn't have built-in pull-ups — external ones are required!)

Documentation: https://rbdimmer.com/docs/
"""

from machine import I2C, Pin
import time

# I2C address of DimmerLink (default 0x50, can be changed)
DIMMER_ADDR = 0x50

# Registers
REG_STATUS   = 0x00   # Device status (R)
REG_COMMAND  = 0x01   # Control commands (W)
REG_ERROR    = 0x02   # Last error code (R)
REG_LEVEL    = 0x10   # Brightness 0-100% (R/W)
REG_CURVE    = 0x11   # Dimming curve (R/W)
REG_FREQ     = 0x20   # Mains frequency Hz (R)
REG_I2C_ADDR = 0x30   # Device I2C address (R/W)

# Dimming curve types
CURVE_LINEAR = 0      # Linear (universal)
CURVE_RMS    = 1      # RMS (incandescent, halogen)
CURVE_LOG    = 2      # Logarithmic (LED)


class DimmerLink:
    """Class for controlling DimmerLink via I2C"""

    def __init__(self, i2c_id=0, scl_pin=22, sda_pin=21, freq=100000, addr=DIMMER_ADDR):
        """
        Initialize I2C connection

        Args:
            i2c_id: I2C bus number (0 or 1)
            scl_pin: GPIO for SCL (ESP32: 22, Pico: 5)
            sda_pin: GPIO for SDA (ESP32: 21, Pico: 4)
            freq: I2C frequency (default 100kHz)
            addr: Device I2C address (default 0x50)
        """
        self.i2c = I2C(i2c_id, scl=Pin(scl_pin), sda=Pin(sda_pin), freq=freq)
        self.addr = addr

        # Check device presence on bus
        devices = self.i2c.scan()
        if self.addr not in devices:
            print(f"Warning: Device 0x{self.addr:02X} not found!")
            print(f"   Found devices: {[hex(d) for d in devices]}")
            print("   Check:")
            print("   - SDA -> SDA, SCL -> SCL (not crossed!)")
            print("   - Pull-up resistors 4.7k ohm")
            print("   - Power supply VCC and GND")
        else:
            print(f"DimmerLink found at 0x{self.addr:02X}")

    def set_level(self, level):
        """
        Set brightness

        Args:
            level: Brightness 0-100%

        Returns:
            bool: True if successful

        Note:
            I2C is fast — can be called frequently for smooth transitions
        """
        if not 0 <= level <= 100:
            print(f"Error: level must be 0-100, got {level}")
            return False

        try:
            self.i2c.writeto_mem(self.addr, REG_LEVEL, bytes([level]))
            return True
        except OSError as e:
            print(f"I2C Error: {e}")
            return False

    def get_level(self):
        """
        Get current brightness

        Returns:
            int: Brightness 0-100%, or None on error
        """
        try:
            data = self.i2c.readfrom_mem(self.addr, REG_LEVEL, 1)
            return data[0]
        except OSError as e:
            print(f"I2C Error: {e}")
            return None

    def set_curve(self, curve_type):
        """
        Set dimming curve

        Args:
            curve_type: CURVE_LINEAR (0), CURVE_RMS (1), CURVE_LOG (2)

        Returns:
            bool: True if successful
        """
        if curve_type not in (0, 1, 2):
            print(f"Error: curve must be 0, 1, or 2, got {curve_type}")
            return False

        try:
            self.i2c.writeto_mem(self.addr, REG_CURVE, bytes([curve_type]))
            return True
        except OSError as e:
            print(f"I2C Error: {e}")
            return False

    def get_curve(self):
        """
        Get curve type

        Returns:
            int: 0=LINEAR, 1=RMS, 2=LOG, or None on error
        """
        try:
            data = self.i2c.readfrom_mem(self.addr, REG_CURVE, 1)
            return data[0]
        except OSError as e:
            print(f"I2C Error: {e}")
            return None

    def get_frequency(self):
        """
        Get mains frequency

        Returns:
            int: 50 or 60 Hz, or None on error
        """
        try:
            data = self.i2c.readfrom_mem(self.addr, REG_FREQ, 1)
            return data[0]
        except OSError as e:
            print(f"I2C Error: {e}")
            return None

    def get_error(self):
        """
        Get last error code

        Returns:
            int: Error code (0x00 = OK)
        """
        try:
            data = self.i2c.readfrom_mem(self.addr, REG_ERROR, 1)
            return data[0]
        except OSError:
            return None

    def fade_to(self, target, duration_ms=1000):
        """
        Smooth brightness change to target level

        Args:
            target: Target brightness 0-100%
            duration_ms: Transition time in milliseconds
        """
        current = self.get_level()
        if current is None:
            print("Error: cannot read current level")
            return

        if not 0 <= target <= 100:
            print(f"Error: target must be 0-100, got {target}")
            return

        steps = abs(target - current)
        if steps == 0:
            return

        delay_ms = max(1, duration_ms // steps)
        direction = 1 if target > current else -1

        level = current
        while True:
            self.set_level(level)
            if level == target:
                break
            level += direction
            time.sleep_ms(delay_ms)


def smooth_fade(dimmer, start, end, duration_ms=2000):
    """
    Smooth brightness change between two levels

    Args:
        dimmer: DimmerLink instance
        start: Start brightness (0-100)
        end: End brightness (0-100)
        duration_ms: Transition time in milliseconds
    """
    if not 0 <= start <= 100 or not 0 <= end <= 100:
        print("Error: start and end must be 0-100")
        return

    steps = abs(end - start)
    if steps == 0:
        return

    delay_ms = max(1, duration_ms // steps)
    direction = 1 if end > start else -1

    level = start
    while True:
        dimmer.set_level(level)
        if level == end:
            break
        level += direction
        time.sleep_ms(delay_ms)


# =============================================================
# Usage example
# =============================================================

def main():
    print("=" * 40)
    print("DimmerLink I2C Example (MicroPython)")
    print("=" * 40)

    # ---------------------------------------------------------
    # Configuration for your board (uncomment as needed):
    # ---------------------------------------------------------

    # ESP32: I2C0, SCL=GPIO22, SDA=GPIO21
    dimmer = DimmerLink(i2c_id=0, scl_pin=22, sda_pin=21)

    # Raspberry Pi Pico: I2C0, SCL=GP5, SDA=GP4
    # dimmer = DimmerLink(i2c_id=0, scl_pin=5, sda_pin=4)

    # ---------------------------------------------------------

    # Connection check
    freq = dimmer.get_frequency()
    if freq:
        print(f"AC frequency: {freq} Hz")
    else:
        print("Cannot read frequency, check connection!")
        return

    # Current curve
    curve_names = {0: "LINEAR", 1: "RMS", 2: "LOG"}
    curve = dimmer.get_curve()
    print(f"Current curve: {curve_names.get(curve, 'Unknown')}")
    print()

    # === Demo 1: Smooth transitions ===
    print("--- Demo 1: Smooth fade (I2C is fast!) ---")

    print("Fading up 0% -> 100%...")
    smooth_fade(dimmer, 0, 100, duration_ms=2000)

    time.sleep(1)

    print("Fading down 100% -> 0%...")
    smooth_fade(dimmer, 100, 0, duration_ms=2000)

    time.sleep(1)

    # === Demo 2: Curve comparison ===
    print("\n--- Demo 2: Curve comparison at 50% ---")
    print("(Observe brightness difference)")

    dimmer.set_level(50)

    for curve_id in (CURVE_LINEAR, CURVE_RMS, CURVE_LOG):
        dimmer.set_curve(curve_id)
        print(f"  {curve_names[curve_id]} curve")
        time.sleep(2)

    # Return to initial settings
    dimmer.set_curve(CURVE_LINEAR)
    dimmer.set_level(0)

    print("\nDone!")


if __name__ == "__main__":
    main()
