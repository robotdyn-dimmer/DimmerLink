# DimmerLink auto-loader — reads /dimmerlink.json
import DimmerLink
import json

var cfg
try
  var f = open('/dimmerlink.json', 'r')
  cfg = json.load(f.read())
  f.close()
except ..
  cfg = nil
end

# Auto-scan if no config
if cfg == nil
  var devs = []
  var letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  for bus_obj : [global.wire1, global.wire2]
    if bus_obj == nil  continue  end
    try
      if !bus_obj.enabled()  continue  end
      for addr : bus_obj.scan()
        bus_obj.read(addr, 0x03, 1)
        if bus_obj.read(addr, 0x03, 1) == 0x01
          var idx = size(devs)
          var lbl = (idx < size(letters))
            ? "Dimmer" + letters[idx .. idx]
            : format("Dimmer%s", letters[(idx % 26) .. (idx % 26)])
          devs.push({"addr": format("0x%02X", addr), "label": lbl, "channels": 1})
        end
      end
    except ..
    end
  end
  if size(devs) == 0  devs.push({"addr": "0x50", "label": "DimmerLink", "channels": 1})  end
  cfg = {"devices": devs, "presets": {"night": 10, "low": 25, "mid": 50, "high": 75, "full": 100}}
  var f = open('/dimmerlink.json', 'w')
  f.write(json.dump(cfg))
  f.close()
end

# Cleanup old instances if reloading
if global._dimmerlink
  for inst : global._dimmerlink
    inst.deinit()
  end
end
if global._dimmerlink_presets
  tasmota.remove_cmd("DimmerLinkPreset")
end

# Create instances
global._dimmerlink = []
for dev : cfg["devices"]
  var inst = DimmerLink(int(dev.find("addr", "0x50")), str(dev.find("label", "DimmerLink")), int(dev.find("channels", 1)))
  if inst.wire  global._dimmerlink.push(inst)  end
end

# Presets
global._dimmerlink_presets = cfg.find("presets", {})
if size(global._dimmerlink_presets) > 0
  tasmota.add_cmd("DimmerLinkPreset",
    def (cmd, idx, payload, pj)
      import json as j
      var p = global._dimmerlink_presets
      var name = string.tolower(payload)
      if p.contains(name)
        var val = int(p[name])
        for inst : global._dimmerlink
          for ch : 0 .. inst.channels - 1  inst.set_level(ch, val)  end
          inst._sync_power()
        end
        tasmota.resp_cmnd(format('{"DimmerLinkPreset":"%s","Level":%d}', name, val))
      else
        var keys = []
        for k : p.keys()  keys.push(k)  end
        tasmota.resp_cmnd(format('{"DimmerLinkPreset":"unknown","Available":%s}', j.dump(keys)))
      end
    end)
end

print(format("DimmerLink: %d device(s)", size(global._dimmerlink)))
