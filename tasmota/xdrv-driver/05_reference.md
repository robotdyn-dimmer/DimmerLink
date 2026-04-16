# Reference & Troubleshooting — DimmerLink Native Tasmota Driver

## Troubleshooting

### Device not detected at boot

**Symptom:** No `DLK:` lines in the serial log. `DlStatus` returns `{"Command":"Unknown"}` or `{"Command":"Error"}`.

| Check | Resolution |
|-------|------------|
| I2C mode | DimmerLink ships with UART enabled. Switch to I2C mode using the UART `SWITCH_I2C` (0x5B) command before connecting to the I2C bus. |
| Physical wiring | Verify SDA, SCL, GND, and VCC are connected. SDA and SCL connect directly (not crossed), unlike UART. |
| Pull-up resistors | Add 4.7 kOhm resistors from SDA to 3.3V and SCL to 3.3V. The ESP32/ESP8266 internal pull-ups are insufficient. |
| I2C pins in Tasmota | Confirm GPIO pins are assigned as I2C SDA and I2C SCL in `Configuration > Configure Module`. |
| Device power-up time | The device requires ~2 seconds after power-up for AC frequency calibration. Restart Tasmota if it booted before the device was ready. |
| `USE_DIMMERLINK` not defined | Confirm the define is in `user_config_override.h` and the firmware was rebuilt. |
| `USE_I2C` missing | Add `#define USE_I2C` to `user_config_override.h` if building a stripped environment. |
| Firmware version mismatch | The driver requires register 0x03 to return exactly `0x01`. Different firmware versions are skipped. |

### Device detected but commands ignored

**Symptom:** `DlStatus` returns data but `DlDim` has no visible effect.

| Check | Resolution |
|-------|------------|
| Device READY state | Check `DlStatus` — if `"Ready": false`, the device is still calibrating. Wait and retry. |
| Error code in status | Check for `LastError` field. `0xFE` = parameter out of range. `0xFC` = EEPROM write error. |
| Thermal shutdown | If `ThermalState` is `SHUTDOWN`, outputs are disabled. Allow the device to cool. |
| Load wiring | Confirm the load is correctly connected to the TRIAC module. |

### I2C errors or intermittent communication

**Symptom:** Device detected but `DlStatus` shows stale data or errors appear in the log.

| Check | Resolution |
|-------|------------|
| Wire length | Keep I2C wires under 30 cm. Longer wires require stronger pull-ups or I2C buffers. |
| Pull-up value | Shorter wires with strong pull-ups (2.2 kOhm) improve signal integrity. Do not use values below 1 kOhm. |
| Power supply | Add a 100nF decoupling capacitor between VCC and GND close to the DimmerLink device. |
| Address conflict | Use `I2CScan` to check for devices sharing the same address. |
| GPIO conflicts | Do not place SDA or SCL on GPIO pins used for other purposes (ADC, touch, strapping pins). |

### Calibration takes too long

**Symptom:** Device shows `"Ready": false` for an extended period.

| Check | Resolution |
|-------|------------|
| AC connection | The MCU detects AC frequency from the mains. If no AC load is connected, calibration cannot complete. |
| Frequency detection | Run `DlStatus` — if `ACFreq` shows `0`, the device has not detected AC frequency. |
| Force recalibration | Send `DlRecalibrate` to restart the calibration cycle. |

### Address change fails

**Symptom:** `DlAddress 0x51` returns an error, or device disappears.

| Check | Resolution |
|-------|------------|
| Verification timeout | The driver reads VERSION from the new address after 50ms. If the device is slow, verification may fail even though the address changed. Power cycle and restart Tasmota. |
| Conflict at new address | If another device occupies the target address, collisions will occur. Scan the bus first with `I2CScan`. |
| Finding the device | Run `I2CScan` or restart Tasmota (which rescans) to discover the actual current address. |

### Temperature not showing

**Symptom:** No temperature row in sensor display. No `Temp` or `Thermal` in MQTT.

| Check | Resolution |
|-------|------------|
| Firmware feature | The MCU firmware must include `FEATURE_TEMPERATURE`. Check with the device supplier. |
| Register value | At boot, the driver reads register 0x40. Value `0xFF` means no sensor. Any other value means the sensor is present. |

### Build issues

| Symptom | Resolution |
|---------|------------|
| `USE_DIMMERLINK` undefined error | Check `user_config_override.h` is in `tasmota/` directory, not the project root. |
| Driver not in binary | `USE_I2C` not enabled for the environment. |
| `XI2C_100` conflict warning | Check `I2CDEVICES.md` — slot 100 is reserved for DimmerLink. |
| Linker error on `I2cRead8` / `I2cWrite8` | `USE_I2C` is missing. |

---

## Error codes

| Code | Name | Meaning |
|------|------|---------|
| 0x00 | OK | No error |
| 0xF9 | ERR_SYNTAX | Invalid register address |
| 0xFC | ERR_NOT_READY | EEPROM write error (flash write failed) |
| 0xFD | ERR_INDEX | Invalid dimmer channel index |
| 0xFE | ERR_PARAM | Parameter value out of range (level > 100, curve > 2, address outside 0x08-0x77) |

Error codes are read from device register 0x02 after write operations. The `DlStatus` command displays the last error code if non-zero as `"LastError":"0xFE"`. Error codes clear automatically when a subsequent successful operation is completed.

---

## I2C register map

| Address | Register | Access | Values |
|---------|----------|--------|--------|
| 0x00 | STATUS | R | Bit 0 = READY, Bit 1 = ERROR |
| 0x01 | COMMAND | W | 0x00=NOP, 0x01=RESET, 0x02=RECALIBRATE |
| 0x02 | ERROR | R | 0x00=OK, 0xF9=SYNTAX, 0xFC=NOT_READY, 0xFD=INDEX, 0xFE=PARAM |
| 0x03 | VERSION | R | 0x01 (firmware version fingerprint) |
| 0x10 | DIM0_LEVEL | R/W | 0-100 |
| 0x11 | DIM0_CURVE | R/W | 0=LINEAR, 1=RMS, 2=LOG |
| 0x12 | DIM1_LEVEL | R/W | 0-100 |
| 0x13 | DIM1_CURVE | R/W | 0=LINEAR, 1=RMS, 2=LOG |
| 0x14 | DIM2_LEVEL | R/W | 0-100 |
| 0x15 | DIM2_CURVE | R/W | 0=LINEAR, 1=RMS, 2=LOG |
| 0x16 | DIM3_LEVEL | R/W | 0-100 |
| 0x17 | DIM3_CURVE | R/W | 0=LINEAR, 1=RMS, 2=LOG |
| 0x18 | FADE | R/W | 0-255 (x100ms) |
| 0x20 | AC_FREQ | R | 50 or 60 |
| 0x21 | AC_PERIOD_L | R | Half-wave period low byte |
| 0x22 | AC_PERIOD_H | R | Half-wave period high byte |
| 0x23 | CALIBRATION | R | 0=busy, 1=complete |
| 0x30 | I2C_ADDRESS | R/W | 0x08-0x77 |
| 0x40 | TEMP_CURRENT | R | raw-50 = Celsius; 0xFF = no sensor |
| 0x41 | TEMP_STATE | R | 0-5 (thermal state) |

### I2C read quirk (dummy read requirement)

The MCU firmware requires a dummy read before each real read to correctly set the internal register pointer. The driver handles this transparently:

```c
uint8_t DL_Read8(uint8_t addr, uint8_t reg, uint8_t bus) {
    I2cRead8(addr, reg, bus);           // Dummy: sets register pointer
    return I2cRead8(addr, reg, bus);    // Real read
}
```

This behavior is a hardware requirement of the MCU and must be preserved in any code that communicates with the device directly.

---

## Thermal state reference

| State | Register value | Typical temperature | Behavior |
|-------|---------------|---------------------|---------|
| NORMAL | 0 | Below ~60 C | Full operation |
| WARNING | 1 | ~60-70 C | Monitoring only; no output change |
| DERATE | 2 | ~70-80 C | MCU reduces output gradually |
| CRITICAL | 3 | ~80-85 C | MCU reduces output aggressively |
| SHUTDOWN | 4 | Above ~85 C | All outputs disabled |
| FAULT | 5 | N/A | Temperature sensor fault detected |

> [!NOTE]
> Thresholds are firmware-defined and may vary by hardware revision. The values above are indicative only.
