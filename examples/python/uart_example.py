#!/usr/bin/env python3
"""
DimmerLink - UART Example (Python + pyserial)

For Raspberry Pi, Windows, Linux, macOS.

Installation:
    pip install pyserial

Connection via USB-UART adapter (CH340, CP2102, FT232):
    Adapter          DimmerLink
    TX             → RX   (crossed!)
    RX             → TX   (crossed!)
    VCC (3.3V/5V)  → VCC
    GND            → GND

Connection on Raspberry Pi (built-in UART):
    Raspberry Pi     DimmerLink
    Pin 8  (TX)    → RX   (crossed!)
    Pin 10 (RX)    → TX   (crossed!)
    Pin 1  (3.3V)  → VCC
    Pin 6  (GND)   → GND

    Enable UART on Pi:
    sudo raspi-config → Interface Options → Serial Port → Enable

⚠️ UART is slower than I2C — don't send commands more than 5-10 times/sec!
   For smooth transitions we recommend I2C (see i2c_example.py)

Documentation: https://rbdimmer.com/docs/
"""

import serial
import serial.tools.list_ports
import time
import sys

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

# Error descriptions
ERROR_MESSAGES = {
    RESP_OK: "OK",
    RESP_ERR_SYNTAX: "Invalid command format (check START byte 0x02)",
    RESP_ERR_EEPROM: "EEPROM write error",
    RESP_ERR_INDEX: "Invalid dimmer index (use 0)",
    RESP_ERR_PARAM: "Invalid parameter value (level 0-100, curve 0-2)",
}

# Curve types
CURVE_LINEAR = 0
CURVE_RMS    = 1
CURVE_LOG    = 2

CURVE_NAMES = {
    CURVE_LINEAR: "LINEAR",
    CURVE_RMS: "RMS",
    CURVE_LOG: "LOG"
}


class DimmerLink:
    """Class for controlling DimmerLink via UART (pyserial)"""

    def __init__(self, port, baudrate=115200, timeout=0.5):
        """
        Initialize UART connection

        Args:
            port: COM port
                  Windows: 'COM3', 'COM4', ...
                  Linux: '/dev/ttyUSB0', '/dev/ttyACM0', '/dev/serial0'
                  macOS: '/dev/tty.usbserial-...'
            baudrate: Speed (always 115200 for DimmerLink)
            timeout: Read timeout in seconds

        Raises:
            serial.SerialException: If unable to open port
        """
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
            print(f"Connected to {port} at {baudrate} baud")
        except serial.SerialException as e:
            print(f"Error opening {port}: {e}")
            print("\nAvailable ports:")
            for p in serial.tools.list_ports.comports():
                print(f"  - {p.device}: {p.description}")
            raise

        # Clear buffers
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def _clear_buffer(self):
        """Clear input buffer from old data"""
        self.ser.reset_input_buffer()

    def _get_error_message(self, code):
        """Get error description by code"""
        return ERROR_MESSAGES.get(code, f"Unknown error 0x{code:02X}")

    def set_level(self, level):
        """
        Set brightness

        Args:
            level: Brightness 0-100%

        Returns:
            bool: True if successful

        Raises:
            ValueError: If level not in range 0-100

        Note:
            ⚠️ Don't call more than 5-10 times per second!
        """
        if not 0 <= level <= 100:
            raise ValueError(f"Level must be 0-100, got {level}")

        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_SET, 0x00, level])
        self.ser.write(cmd)

        resp = self.ser.read(1)
        if len(resp) == 0:
            print("Error: No response (check TX→RX connection)")
            return False

        if resp[0] != RESP_OK:
            print(f"Error: {self._get_error_message(resp[0])}")
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
        self.ser.write(cmd)

        resp = self.ser.read(2)
        if len(resp) < 2:
            return None

        if resp[0] == RESP_OK:
            return resp[1]

        print(f"Error: {self._get_error_message(resp[0])}")
        return None

    def set_curve(self, curve_type):
        """
        Set dimming curve

        Args:
            curve_type: CURVE_LINEAR (0), CURVE_RMS (1), CURVE_LOG (2)

        Returns:
            bool: True if successful

        Raises:
            ValueError: If curve_type not 0, 1 or 2
        """
        if curve_type not in (0, 1, 2):
            raise ValueError(f"Curve type must be 0, 1, or 2, got {curve_type}")

        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_CURVE, 0x00, curve_type])
        self.ser.write(cmd)

        resp = self.ser.read(1)
        if len(resp) == 0:
            return False

        if resp[0] != RESP_OK:
            print(f"Error: {self._get_error_message(resp[0])}")
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
        self.ser.write(cmd)

        resp = self.ser.read(2)
        if len(resp) == 2 and resp[0] == RESP_OK:
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
        self.ser.write(cmd)

        resp = self.ser.read(2)
        if len(resp) == 2 and resp[0] == RESP_OK:
            return resp[1]
        return None

    def reset(self):
        """
        Software device reset

        ⚠️ After reset, device will reboot (~3 sec)
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_RESET])
        self.ser.write(cmd)
        print("Device reset command sent, wait 3 seconds...")

    def switch_to_i2c(self):
        """
        Switch interface to I2C

        Returns:
            bool: True if successful

        ⚠️ After switching, UART will no longer work!
           Use I2C at address 0x50.
        """
        self._clear_buffer()
        cmd = bytes([CMD_START, CMD_SWITCH_I2C])
        self.ser.write(cmd)

        resp = self.ser.read(1)
        if len(resp) > 0 and resp[0] == RESP_OK:
            print("Switched to I2C mode")
            print("  UART is now disabled")
            print("  Use I2C at address 0x50")
            return True

        print("Failed to switch to I2C")
        return False

    def close(self):
        """Close connection"""
        self.ser.close()

    def __enter__(self):
        """Support for context manager (with statement)"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Automatic close on exit from with"""
        self.close()


def list_ports():
    """Show list of available ports"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found")
        return []

    print("Available serial ports:")
    for p in ports:
        print(f"  {p.device}: {p.description}")
    return [p.device for p in ports]


def auto_detect_port():
    """
    Attempt automatic port detection

    Returns:
        str: Port name or None
    """
    ports = serial.tools.list_ports.comports()

    # Search by known VID/PID (CH340, CP2102, FT232)
    known_adapters = [
        (0x1A86, 0x7523, "CH340"),      # CH340
        (0x10C4, 0xEA60, "CP2102"),     # CP2102
        (0x0403, 0x6001, "FT232"),      # FT232RL
        (0x0403, 0x6015, "FT231X"),     # FT231X
    ]

    for port in ports:
        for vid, pid, name in known_adapters:
            if port.vid == vid and port.pid == pid:
                print(f"Auto-detected {name} on {port.device}")
                return port.device

    # Fallback: first USB-Serial port
    for port in ports:
        if "USB" in port.device.upper() or "ACM" in port.device.upper():
            print(f"Found USB serial: {port.device}")
            return port.device

    # Raspberry Pi built-in UART
    if sys.platform.startswith('linux'):
        import os
        if os.path.exists('/dev/serial0'):
            print("Found Raspberry Pi UART: /dev/serial0")
            return '/dev/serial0'

    return None


# =============================================================
# Usage example
# =============================================================

def main():
    print("=" * 50)
    print("DimmerLink UART Example (Python + pyserial)")
    print("=" * 50)
    print()

    # Port detection
    port = auto_detect_port()

    if port is None:
        print("No suitable port found\n")
        list_ports()
        print("\nSpecify port manually:")
        print("  Windows: python uart_example.py COM3")
        print("  Linux:   python uart_example.py /dev/ttyUSB0")
        sys.exit(1)

    # Can override via command line argument
    if len(sys.argv) > 1:
        port = sys.argv[1]
        print(f"Using port from argument: {port}")

    print()

    try:
        with DimmerLink(port) as dimmer:

            # Connection check
            freq = dimmer.get_frequency()
            if freq is None:
                print("\nDevice not responding!")
                print("  Check:")
                print("  - TX → RX (crossed!)")
                print("  - RX → TX (crossed!)")
                print("  - Baud rate: 115200")
                sys.exit(1)

            curve = dimmer.get_curve()

            print(f"AC frequency: {freq} Hz")
            print(f"Current curve: {CURVE_NAMES.get(curve, 'Unknown')}")
            print()

            # === Demo: Step brightness change ===
            print("--- Demo: Step brightness (UART is slower than I2C) ---")
            print("Minimum 200-300ms between commands!\n")

            print("Increasing brightness...")
            for level in range(0, 101, 10):
                if dimmer.set_level(level):
                    print(f"  Level: {level}%")
                time.sleep(0.3)  # Minimum 200-300 ms between commands

            time.sleep(1)

            print("\nDecreasing brightness...")
            for level in range(100, -1, -10):
                if dimmer.set_level(level):
                    print(f"  Level: {level}%")
                time.sleep(0.3)

            # Check reading
            print("\n--- Reading current level ---")
            current = dimmer.get_level()
            if current is not None:
                print(f"Current level: {current}%")

            # Curve demo
            print("\n--- Testing curves at 50% ---")
            dimmer.set_level(50)

            for curve_id in (CURVE_LINEAR, CURVE_RMS, CURVE_LOG):
                if dimmer.set_curve(curve_id):
                    print(f"  {CURVE_NAMES[curve_id]} curve")
                time.sleep(1)

            # Return to initial settings
            dimmer.set_curve(CURVE_LINEAR)
            dimmer.set_level(0)

            print("\nDone!")
            print("\nTip: For smooth fading, use I2C interface (i2c_example.py)")

    except serial.SerialException as e:
        print(f"\nSerial Error: {e}")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(0)


if __name__ == "__main__":
    main()
