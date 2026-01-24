"""
DimmerLink - UART Example (MicroPython)

For ESP32, Raspberry Pi Pico and other MicroPython boards.

Wiring ESP32:
    ESP32          DimmerLink
    GPIO17 (TX)  → RX   (crossed!)
    GPIO16 (RX)  → TX   (crossed!)
    3.3V         → VCC
    GND          → GND

Wiring Raspberry Pi Pico:
    Pico         DimmerLink
    GP0 (TX)   → RX   (crossed!)
    GP1 (RX)   → TX   (crossed!)
    3V3        → VCC
    GND        → GND

⚠️ UART is slower than I2C — don't send commands more than 5-10 times/sec!
   For smooth transitions we recommend I2C (see i2c_example.py)

Documentation: https://rbdimmer.com/docs/
"""

from machine import UART, Pin
import time

# UART commands (all start with 0x02)
CMD_START       = 0x02    # Start byte (required!)
CMD_SET         = 0x53    # 'S' - set brightness
CMD_GET         = 0x47    # 'G' - get brightness
CMD_CURVE       = 0x43    # 'C' - set curve
CMD_GETCURVE    = 0x51    # 'Q' - get curve
CMD_FREQ        = 0x52    # 'R' - get mains frequency
CMD_RESET       = 0x58    # 'X' - device reset
CMD_SWITCH_I2C  = 0x5B    # '[' - switch to I2C

# Response codes
RESP_OK         = 0x00    # Success
RESP_ERR_SYNTAX = 0xF9    # Invalid command format
RESP_ERR_EEPROM = 0xFC    # EEPROM write error
RESP_ERR_INDEX  = 0xFD    # Invalid dimmer index
RESP_ERR_PARAM  = 0xFE    # Invalid parameter

# Curve types
CURVE_LINEAR = 0
CURVE_RMS    = 1
CURVE_LOG    = 2

# Error descriptions
ERROR_MESSAGES = {
    RESP_OK: "OK",
    RESP_ERR_SYNTAX: "Invalid command format",
    RESP_ERR_EEPROM: "EEPROM write error",
    RESP_ERR_INDEX: "Invalid dimmer index (use 0)",
    RESP_ERR_PARAM: "Invalid parameter value",
}


class DimmerLink:
    """Class for controlling DimmerLink via UART"""

    def __init__(self, uart_id=1, tx_pin=17, rx_pin=16, baudrate=115200):
        """
        Initialize UART connection

        Args:
            uart_id: UART number (ESP32: 1 or 2, Pico: 0 or 1)
            tx_pin: GPIO for TX (ESP32: 17, Pico: 0)
            rx_pin: GPIO for RX (ESP32: 16, Pico: 1)
            baudrate: Speed (always 115200 for DimmerLink)
        """
        self.uart = UART(uart_id, baudrate=baudrate, tx=Pin(tx_pin), rx=Pin(rx_pin))
        print(f"UART{uart_id} initialized: TX=GPIO{tx_pin}, RX=GPIO{rx_pin}, {baudrate} baud")

    def _clear_buffer(self):
        """Clear input buffer from old data"""
        while self.uart.any():
            self.uart.read()

    def _read_response(self, expected_bytes=1, timeout_ms=100):
        """
        Read response with timeout

        Args:
            expected_bytes: Expected number of bytes
            timeout_ms: Timeout in milliseconds

        Returns:
            bytes or None
        """
        start = time.ticks_ms()
        while self.uart.any() < expected_bytes:
            if time.ticks_diff(time.ticks_ms(), start) > timeout_ms:
                return None
            time.sleep_ms(1)
        return self.uart.read(expected_bytes)

    def _print_error(self, code):
        """Print error description"""
        msg = ERROR_MESSAGES.get(code, f"Unknown error 0x{code:02X}")
        print(f"Error: {msg}")

    def set_level(self, level):
        """
        Set brightness

        Args:
            level: Brightness 0-100%

        Returns:
            bool: True if successful

        Note:
            ⚠️ Don't call more than 5-10 times per second!
        """
        if not 0 <= level <= 100:
            print(f"Error: level must be 0-100, got {level}")
            return False

        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_SET, 0x00, level])
        self.uart.write(cmd)

        resp = self._read_response(1)
        if resp is None:
            print("Error: No response (check TX→RX connection)")
            return False

        if resp[0] != RESP_OK:
            self._print_error(resp[0])
            return False

        return True

    def get_level(self):
        """
        Get current brightness

        Returns:
            int: Brightness 0-100%, or None on error
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_GET, 0x00])
        self.uart.write(cmd)

        resp = self._read_response(2)
        if resp is None:
            print("Error: No response")
            return None

        if resp[0] == RESP_OK:
            return resp[1]
        else:
            self._print_error(resp[0])
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

        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_CURVE, 0x00, curve_type])
        self.uart.write(cmd)

        resp = self._read_response(1)
        if resp is None:
            print("Error: No response")
            return False

        if resp[0] != RESP_OK:
            self._print_error(resp[0])
            return False

        return True

    def get_curve(self):
        """
        Get curve type

        Returns:
            int: 0=LINEAR, 1=RMS, 2=LOG, or None on error
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_GETCURVE, 0x00])
        self.uart.write(cmd)

        resp = self._read_response(2)
        if resp is None:
            return None

        if resp[0] == RESP_OK:
            return resp[1]
        return None

    def get_frequency(self):
        """
        Get mains frequency

        Returns:
            int: 50 or 60 Hz, or None on error
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_FREQ])
        self.uart.write(cmd)

        resp = self._read_response(2)
        if resp is None:
            return None

        if resp[0] == RESP_OK:
            return resp[1]
        return None

    def reset(self):
        """
        Software device reset

        ⚠️ After reset, device will reboot!
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_RESET])
        self.uart.write(cmd)
        print("Device reset command sent")

    def switch_to_i2c(self):
        """
        Switch interface to I2C

        ⚠️ After switching, UART will no longer work!
           Use I2C at address 0x50.
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_SWITCH_I2C])
        self.uart.write(cmd)

        resp = self._read_response(1)
        if resp and resp[0] == RESP_OK:
            print("Switched to I2C mode")
            print("UART is now disabled, use I2C at address 0x50")
            return True
        return False


# =============================================================
# Usage example
# =============================================================

def main():
    print("=" * 40)
    print("DimmerLink UART Example (MicroPython)")
    print("=" * 40)

    # ---------------------------------------------------------
    # Configuration for your board (uncomment as needed):
    # ---------------------------------------------------------

    # ESP32: UART1, TX=GPIO17, RX=GPIO16
    dimmer = DimmerLink(uart_id=1, tx_pin=17, rx_pin=16)

    # Raspberry Pi Pico: UART0, TX=GP0, RX=GP1
    # dimmer = DimmerLink(uart_id=0, tx_pin=0, rx_pin=1)

    # ---------------------------------------------------------

    print()

    # Connection check
    freq = dimmer.get_frequency()
    if freq:
        print(f"Device ready! AC frequency: {freq} Hz")
    else:
        print("Device not responding!")
        print("  Check:")
        print("  - TX -> RX (crossed!)")
        print("  - RX -> TX (crossed!)")
        print("  - Baud rate: 115200")
        return

    # Current level
    level = dimmer.get_level()
    print(f"Current level: {level}%")
    print()

    # === Demo: Step brightness change ===
    print("--- Demo: Step brightness (UART is slower than I2C) ---")
    print("Minimum 200-300ms between commands!\n")

    print("Increasing brightness...")
    for lvl in range(0, 101, 10):
        if dimmer.set_level(lvl):
            print(f"  Level: {lvl}%")
        time.sleep_ms(300)  # Minimum 200-300 ms between commands

    time.sleep(1)

    print("\nDecreasing brightness...")
    for lvl in range(100, -1, -10):
        if dimmer.set_level(lvl):
            print(f"  Level: {lvl}%")
        time.sleep_ms(300)

    # Check reading
    print("\n--- Reading current level ---")
    current = dimmer.get_level()
    if current is not None:
        print(f"Current level: {current}%")

    # Return to 0
    dimmer.set_level(0)

    print("\nDone!")
    print("\nTip: For smooth fading, use I2C interface (i2c_example.py)")


if __name__ == "__main__":
    main()
