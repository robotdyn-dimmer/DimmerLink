# Code Examples

## Example 1: Simple brightness control from Berry

```berry
# Set brightness directly from any Berry script
import DimmerLink

# Access the first loaded DimmerLink instance
var dim = global._dimmerlink[0]

# Set brightness
dim.set_level(0, 75)     # channel 0 (first channel), 75%

# Read current level
print(dim.levels[0])     # prints: 75

# Turn off
dim.set_level(0, 0)
```

## Example 2: Timed dimming with Berry timer

```berry
# Gradually dim from current level to 0 over 10 seconds
var dim = global._dimmerlink[0]
var target = 0
var current = dim.levels[0]
var steps = 20
var step_size = (current - target) / steps
var step_delay = 500  # ms

var step_count = 0
def dim_step()
  step_count += 1
  var new_level = current - int(step_size * step_count)
  if new_level < target  new_level = target  end
  dim.set_level(0, new_level)
  if step_count < steps
    tasmota.set_timer(step_delay, dim_step)
  end
end

dim_step()
```

## Example 3: React to a button press

```berry
# Toggle DimmerLink on button press using Tasmota rules
# In Tasmota console:
# Rule1 ON Button1#state DO br global._dimmerlink[0].set_level(0, global._dimmerlink[0].levels[0] > 0 ? 0 : 100) ENDON
# Rule1 1

# Or in Berry:
tasmota.add_rule("Button1#state",
  def ()
    var dim = global._dimmerlink[0]
    if dim.levels[0] > 0
      dim.set_level(0, 0)
    else
      dim.set_level(0, 100)
    end
    dim._sync_power()
  end
)
```

## Example 4: Scheduled brightness changes

```berry
# Set brightness based on time of day
# Add to autoexec.be after the dimmerlink_loader line

tasmota.add_cron("0 0 7 * * *", def ()
  # 7:00 AM — full brightness
  tasmota.cmd("DimmerLinkPreset full")
end, "morning")

tasmota.add_cron("0 0 21 * * *", def ()
  # 9:00 PM — dim for evening
  tasmota.cmd("DimmerLinkPreset low")
end, "evening")

tasmota.add_cron("0 0 23 * * *", def ()
  # 11:00 PM — night mode
  tasmota.cmd("DimmerLinkPreset night")
end, "night")
```

## Example 5: MQTT-triggered scene control

```berry
# Subscribe to a custom MQTT topic for scene control
tasmota.add_rule("MqttReceived#scene",
  def (value)
    if value == "movie"
      # Dim to 10%, LOG curve for LED
      for inst : global._dimmerlink
        for ch : 0 .. inst.channels - 1
          inst.set_level(ch, 10)
        end
        inst._sync_power()
      end
      tasmota.cmd("DimmerLink_TVCurve 2")  # LOG curve
    elif value == "dinner"
      tasmota.cmd("DimmerLinkPreset mid")
      tasmota.cmd("DimmerLink_KitchenCurve 1")  # RMS for warm light
    elif value == "off"
      tasmota.cmd("DimmerLinkPreset night")
    end
  end
)

# Trigger from MQTT:
# Topic: cmnd/<topic>/scene
# Payload: movie
```

## Example 6: Custom HTTP endpoint

```berry
# Add a custom web endpoint for external integration
import webserver

webserver.on("/dimmerlink/api", def (req)
  import json
  var result = {}
  for inst : global._dimmerlink
    result[inst.label] = {
      "addr": format("0x%02X", inst.addr),
      "levels": inst.levels,
      "power": inst.power,
      "ready": inst.ready
    }
  end
  webserver.content_response(json.dump(result))
end)

# Access: http://<ip>/dimmerlink/api
# Returns: {"Kitchen":{"addr":"0x50","levels":[75],"power":true,"ready":true}}
```

## Example 7: Temperature-based derating alert

```berry
# Monitor temperature and send MQTT alert if thermal protection activates
# (Requires MCU firmware with FEATURE_TEMPERATURE)

tasmota.add_cron("0 */1 * * * *", def ()
  for inst : global._dimmerlink
    if inst.has_temp
      var temp = inst.get_temp()
      var state = inst.get_temp_state_name()
      if state != "NORMAL" && state != nil
        var msg = format('{"alert":"thermal","device":"%s","temp":%d,"state":"%s"}', inst.label, temp, state)
        tasmota.publish("tele/dimmerlink/alert", msg)
      end
    end
  end
end, "temp_monitor")
```

## Example 8: Manual setup without auto-loader

If you prefer full manual control instead of using the auto-loader:

```berry
# In preinit.be (runs before autoexec.be)
import DimmerLink

# Create instances explicitly
var kitchen = DimmerLink(0x50, "Kitchen", 1)
var bedroom = DimmerLink(0x51, "Bedroom", 2)

# Store references for use in other scripts
global.kitchen_dimmer = kitchen
global.bedroom_dimmer = bedroom
```

Then in other Berry scripts:
```berry
global.kitchen_dimmer.set_level(0, 50)
global.bedroom_dimmer.set_level(1, 30)  # channel 2
```

## Example 9: Integration with Home Assistant via MQTT

Tasmota auto-discovery works with Home Assistant. The DimmerLink virtual relay appears as a switch. For dimmer control, publish from HA:

```yaml
# Home Assistant configuration.yaml
mqtt:
  light:
    - name: "Kitchen Dimmer"
      command_topic: "cmnd/tasmota_XXXX/DimmerLink_Kitchen"
      state_topic: "tele/tasmota_XXXX/SENSOR"
      state_value_template: "{{ value_json.Kitchen.Level }}"
      brightness_command_topic: "cmnd/tasmota_XXXX/DimmerLink_Kitchen"
      brightness_state_topic: "tele/tasmota_XXXX/SENSOR"
      brightness_value_template: "{{ value_json.Kitchen.Level }}"
      brightness_scale: 100
      on_command_type: "brightness"
```
