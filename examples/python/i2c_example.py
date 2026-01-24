#!/usr/bin/env python3
"""
DimmerLink - I2C Example (Python + smbus2)

For Raspberry Pi and other single board computers with Linux.

Installation:
    pip install smbus2

Enable I2C on Raspberry Pi:
    sudo raspi-config → Interface Options → I2C → Enable
    sudo reboot

Check connection:
    sudo apt install i2c-tools
    i2cdetect -y 1
    # Should show device at address 0x50

Wiring:
    Raspberry Pi     DimmerLink
    Pin 1 (3.3V)   → VCC
    Pin 6 (GND)    → GND
    Pin 3 (SDA)    → SDA  (not crossed!)
    Pin 5 (SCL)    → SCL  (not crossed!)

Pull-up resistors: Raspberry Pi has built-in 1.8kΩ — usually sufficient.

Documentation: https://rbdimmer.com/docs/
"""

from smbus2 import SMBus
import time
import sys

# I2C address of DimmerLink (default 0x50, can be changed)
DIMMER_ADDR = 0x50

# Registers
REG_STATUS   = 0x00   # Device status (R)
REG_COMMAND  = 0x01   # Control commands (W)
REG_ERROR    = 0x02   # Last error code (R)
REG_VERSION  = 0x03   # Firmware version (R)
REG_LEVEL    = 0x10   # Brightness 0-100% (R/W)
REG_CURVE    = 0x11   # Dimming curve (R/W)
REG_FREQ     = 0x20   # Mains frequency Hz (R)
REG_I2C_ADDR = 0x30   # Device I2C address (R/W)

# Dimming curve types
CURVE_LINEAR = 0      # Linear (universal)
CURVE_RMS    = 1      # RMS (incandescent, halogen)
CURVE_LOG    = 2      # Logarithmic (LED)

# Curve names for output
CURVE_NAMES = {
    CURVE_LINEAR: "LINEAR",
    CURVE_RMS: "RMS",
    CURVE_LOG: "LOG"
}


class DimmerLink:
    """Class for controlling DimmerLink via I2C (smbus2)"""

    def __init__(self, bus_number=1, addr=DIMMER_ADDR):
        """
        Initialize I2C connection

        Args:
            bus_number: I2C bus number (usually 1 for Raspberry Pi)
            addr: Device I2C address (default 0x50)

        Raises:
            OSError: If unable to open I2C bus
        """
        self.addr = addr
        try:
            self.bus = SMBus(bus_number)
        except OSError as e:
            print(f"Error opening I2C bus {bus_number}: {e}")
            print("\nCheck:")
            print("  - I2C enabled: sudo raspi-config → Interface Options → I2C")
            print("  - User in i2c group: sudo usermod -a -G i2c $USER")
            print("  - Then logout/login or reboot")
            raise

        # Check device presence
        try:
            self.bus.read_byte(self.addr)
            print(f"DimmerLink found at address 0x{self.addr:02X}")
        except OSError:
            print(f"Warning: Device 0x{self.addr:02X} not responding")
            print("   Run: i2cdetect -y 1")

    def set_level(self, level):
        """
        Set brightness

        Args:
            level: Brightness 0-100%

        Raises:
            ValueError: If level not in range 0-100

        Note:
            I2C is fast — can be called frequently for smooth transitions
        """
        if not 0 <= level <= 100:
            raise ValueError(f"Level must be 0-100, got {level}")

        self.bus.write_byte_data(self.addr, REG_LEVEL, level)

    def get_level(self):
        """
        Get current brightness

        Returns:
            int: Brightness 0-100%
        """
        return self.bus.read_byte_data(self.addr, REG_LEVEL)

    def set_curve(self, curve_type):
        """
        Set dimming curve

        Args:
            curve_type: CURVE_LINEAR (0), CURVE_RMS (1), CURVE_LOG (2)

        Raises:
            ValueError: If curve_type not 0, 1 or 2
        """
        if curve_type not in (0, 1, 2):
            raise ValueError(f"Curve type must be 0, 1, or 2, got {curve_type}")

        self.bus.write_byte_data(self.addr, REG_CURVE, curve_type)

    def get_curve(self):
        """
        Get curve type

        Returns:
            int: 0=LINEAR, 1=RMS, 2=LOG
        """
        return self.bus.read_byte_data(self.addr, REG_CURVE)

    def get_frequency(self):
        """
        Get mains frequency

        Returns:
            int: 50 or 60 Hz
        """
        return self.bus.read_byte_data(self.addr, REG_FREQ)

    def get_version(self):
        """
        Get firmware version

        Returns:
            int: Version number (e.g., 1 = v1.0)
        """
        return self.bus.read_byte_data(self.addr, REG_VERSION)

    def get_error(self):
        """
        Get last error code

        Returns:
            int: Error code (0x00 = OK)
        """
        return self.bus.read_byte_data(self.addr, REG_ERROR)

    def reset(self):
        """
        Software device reset

        ⚠️ After reset, device will reboot (~3 sec)
        """
        self.bus.write_byte_data(self.addr, REG_COMMAND, 0x01)
        print("Device reset, wait 3 seconds...")

    def switch_to_uart(self):
        """
        Switch interface to UART

        ⚠️ After switching, I2C will no longer work!
        """
        self.bus.write_byte_data(self.addr, REG_COMMAND, 0x03)
        print("Switched to UART mode (115200, 8N1)")

    def change_address(self, new_addr):
        """
        Change device I2C address

        Args:
            new_addr: New address (0x08-0x77)

        ⚠️ After change, device immediately responds on new address!
        """
        if not 0x08 <= new_addr <= 0x77:
            raise ValueError(f"Address must be 0x08-0x77, got 0x{new_addr:02X}")

        self.bus.write_byte_data(self.addr, REG_I2C_ADDR, new_addr)
        print(f"Address changed from 0x{self.addr:02X} to 0x{new_addr:02X}")
        self.addr = new_addr

    def fade_to(self, target, duration=1.0):
        """
        Smooth brightness change to target level

        Args:
            target: Target brightness 0-100%
            duration: Transition time in seconds
        """
        if not 0 <= target <= 100:
            raise ValueError(f"Target must be 0-100, got {target}")

        current = self.get_level()
        steps = abs(target - current)

        if steps == 0:
            return

        delay = duration / steps
        direction = 1 if target > current else -1

        level = current
        while True:
            self.set_level(level)
            if level == target:
                break
            level += direction
            time.sleep(delay)

    def close(self):
        """Close I2C connection"""
        self.bus.close()

    def __enter__(self):
        """Support for context manager (with statement)"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Automatic close on exit from with"""
        self.close()


def smooth_fade(dimmer, start, end, duration=2.0):
    """
    Smooth brightness change between two levels

    Args:
        dimmer: DimmerLink instance
        start: Start brightness (0-100)
        end: End brightness (0-100)
        duration: Transition time in seconds
    """
    if not 0 <= start <= 100 or not 0 <= end <= 100:
        raise ValueError("Start and end must be 0-100")

    steps = abs(end - start)
    if steps == 0:
        return

    delay = duration / steps
    direction = 1 if end > start else -1

    level = start
    while True:
        dimmer.set_level(level)
        if level == end:
            break
        level += direction
        time.sleep(delay)


# =============================================================
# Usage example
# =============================================================

def main():
    print("=" * 50)
    print("DimmerLink I2C Example (Python + smbus2)")
    print("=" * 50)
    print()

    # Using context manager for automatic close
    try:
        with DimmerLink(bus_number=1) as dimmer:

            # Device info
            freq = dimmer.get_frequency()
            version = dimmer.get_version()
            curve = dimmer.get_curve()

            print(f"Firmware version: {version}")
            print(f"AC frequency: {freq} Hz")
            print(f"Current curve: {CURVE_NAMES.get(curve, 'Unknown')}")
            print()

            # === Demo 1: Smooth transitions ===
            print("--- Demo 1: Smooth fade (I2C is fast!) ---")

            print("Fading up 0% -> 100%...")
            smooth_fade(dimmer, 0, 100, duration=2.0)

            time.sleep(0.5)

            print("Fading down 100% -> 0%...")
            smooth_fade(dimmer, 100, 0, duration=2.0)

            time.sleep(1)

            # === Demo 2: Curve comparison ===
            print("\n--- Demo 2: Curve comparison at 50% ---")
            print("(Observe brightness difference)")

            dimmer.set_level(50)

            for curve_id in (CURVE_LINEAR, CURVE_RMS, CURVE_LOG):
                dimmer.set_curve(curve_id)
                print(f"  {CURVE_NAMES[curve_id]} curve")
                time.sleep(2)

            # Return to initial settings
            dimmer.set_curve(CURVE_LINEAR)
            dimmer.set_level(0)

            print("\nDone!")

    except OSError as e:
        print(f"\nI2C Error: {e}")
        print("\nTroubleshooting:")
        print("  1. Check wiring: SDA→SDA, SCL→SCL (not crossed!)")
        print("  2. Check power: VCC and GND connected")
        print("  3. Run: i2cdetect -y 1")
        print("  4. Check permissions: sudo usermod -a -G i2c $USER")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(0)


if __name__ == "__main__":
    main()
