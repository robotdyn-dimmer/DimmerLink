# Reference & Troubleshooting

## Command Reference

| Command | Description | Example |
|---------|-------------|---------|
| `DimmerLink_<Label> <0-100>` | Set brightness | `DimmerLink_Kitchen 75` |
| `DimmerLink_<Label><N> <0-100>` | Set channel N brightness | `DimmerLink_Lamp2 50` |
| `DimmerLink_<Label>Curve [ch] <0-2>` | Set dimming curve | `DimmerLink_KitchenCurve 2` |
| `DimmerLink_<Label>Fade <0-255>` | Set fade time (x100ms) | `DimmerLink_KitchenFade 10` |
| `DimmerLinkPreset <name>` | Apply preset to all devices | `DimmerLinkPreset night` |
| `Power<N> ON/OFF/TOGGLE` | Virtual relay control | `Power6 ON` |

## Berry Console Utilities

| Command | Description |
|---------|-------------|
| `import DimmerLink; DimmerLink.scan()` | Scan I2C bus for devices |
| `import DimmerLink; DimmerLink.change_addr(0x50, 0x51)` | Change device address |
| `import DimmerLink; DimmerLink.help()` | Show command reference |
| `global._dimmerlink[0].info()` | Show device status |

## Dimming Curves

| Value | Name | Description | Best for |
|-------|------|-------------|----------|
| 0 | LINEAR | Linear phase angle | General purpose, motors |
| 1 | RMS | RMS-compensated (level² mapping) | Incandescent, halogen |
| 2 | LOG | Logarithmic (perceptual) | LEDs, mood lighting |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No slider on main page | Check: `DimmerLink.scan()` finds device? I2C pins configured? |
| Slider doesn't change brightness | Check label doesn't end with digit. Check `Status 8` for DimmerLink data |
| "Command Unknown" in console | Driver not loaded. Check `autoexec.be` contains loader line |
| Device not found after address change | Run `DimmerLink.scan()` to find real address, update `dimmerlink.json` |
| "label ends with digit" warning | Rename label in `dimmerlink.json` to not end with 0-9 |
| Temperature shows "N/A" | MCU firmware does not have FEATURE_TEMPERATURE enabled |
