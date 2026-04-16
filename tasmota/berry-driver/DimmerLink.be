# DimmerLink Berry I2C driver for Tasmota
# MCU TRIAC Dimmer Controller — up to 4 channels, multiple devices
# https://github.com/robotdyn-dimmer/DimmerLink
# -------------------------------------------------------------------------
#
# Usage in preinit.be:
#   import DimmerLink
#   dim1 = DimmerLink(0x50, "Kitchen", 1)    # 1 channel at 0x50
#   dim2 = DimmerLink(0x51, "Bedroom", 2)    # 2 channels at 0x51
#
# Utility (Berry console):
#   DimmerLink.scan()                  # find all DimmerLink devices on I2C
#   DimmerLink.change_addr(0x50,0x51)  # change device address
#   DimmerLink.help()                  # show usage
#   dim1.info()                        # show instance status
#
# Commands (console / HTTP / MQTT):
#   DimmerLink <0-100>               # channel 1 (= DimmerLink1)
#   DimmerLink1 <0-100>              # channel 1
#   DimmerLink2 <0-100>              # channel 2
#   DimmerLink_Kitchen <0-100>       # channel 1 of "Kitchen"
#   DimmerLink_Kitchen2 <0-100>      # channel 2 of "Kitchen"
#   DimmerLinkCurve [ch] <0-2>       # curve: 0=LINEAR, 1=RMS, 2=LOG
#   DimmerLinkFade <0-255>           # fade time (x100ms)
#   Power<N> ON/OFF                  # virtual relay
#
# -------------------------------------------------------------------------

class DimmerLink : I2C_Driver
  # I2C register map (MCU I2C_TECHNICAL_SPECIFICATION.md)
  static var ADDR_DEFAULT = 0x50
  static var REG_STATUS   = 0x00
  static var REG_CMD      = 0x01
  static var REG_ERROR    = 0x02
  static var REG_VER      = 0x03
  static var REG_DIM_BASE = 0x10   # +N*2=LEVEL, +N*2+1=CURVE
  static var REG_FADE     = 0x18
  static var REG_FREQ     = 0x20
  static var REG_CALIB    = 0x23
  static var REG_ADDR     = 0x30
  static var REG_TEMP_CUR = 0x40
  static var REG_TEMP_ST  = 0x41
  # version fingerprint
  static var FW_VERSION   = 0x01
  static var CURVE_NAMES  = ["LINEAR", "RMS", "LOG"]

  var channels
  var levels
  var power
  var relay_idx
  var ready
  var label
  var cmd_prefix
  var has_temp

  # ─────────────────────────────────────────────
  # Override read8/write8: MCU requires a dummy
  # read after setting the register pointer.
  # Both guard against nil wire (device disconnected).
  # ─────────────────────────────────────────────
  def read8(reg)
    if !self.wire  return nil  end
    self.wire.read(self.addr, reg, 1)   # dummy: sets register pointer
    return self.wire.read(self.addr, reg, 1)  # real read
  end

  def write8(reg, val)
    if !self.wire  return nil  end
    return self.wire.write(self.addr, reg, val, 1)
  end

  # ─────────────────────────────────────────────
  # Constructor
  # ─────────────────────────────────────────────
  # Sanitize label: strip HTML chars, warn if ends with digit
  static def _sanitize_label(label)
    import string
    # strip HTML metacharacters: < > & " '
    label = string.tr(label, '<>&"\'', '')
    if size(label) == 0  label = "DimmerLink"  end
    # warn if ends with digit (Tasmota parses trailing digits as idx)
    var last = label[size(label) - 1 ..]
    if string.find(last, '0') >= 0 || string.find(last, '1') >= 0 ||
       string.find(last, '2') >= 0 || string.find(last, '3') >= 0 ||
       string.find(last, '4') >= 0 || string.find(last, '5') >= 0 ||
       string.find(last, '6') >= 0 || string.find(last, '7') >= 0 ||
       string.find(last, '8') >= 0 || string.find(last, '9') >= 0
      print(format("DimmerLink WARNING: label '%s' ends with digit — commands will not work. Rename it.", label))
    end
    return label
  end

  def init(addr, label, channels)
    if addr == nil      addr = self.ADDR_DEFAULT  end
    if label == nil     label = "DimmerLink"       end
    if channels == nil  channels = 1               end
    if channels < 1     channels = 1               end
    if channels > 4     channels = 4               end

    label = self._sanitize_label(label)
    self.label = label
    self.channels = channels
    self.has_temp = false
    self.cmd_prefix = ""
    # init all fields to safe defaults before wire check
    self.levels = []
    for i : 0 .. self.channels - 1
      self.levels.push(0)
    end
    self.power = false
    self.ready = false
    self.relay_idx = -1

    # command prefix: "DimmerLink" or "DimmerLink_Name"
    if label == "DimmerLink"
      self.cmd_prefix = "DimmerLink"
    else
      self.cmd_prefix = "DimmerLink_" + label
    end

    super(self).init(label, addr)

    if !self.wire  return  end

    # claim a virtual relay
    self.relay_idx = tasmota.global.devices_present
    tasmota.global.devices_present += 1

    # register commands
    # Tasmota parses trailing digits as idx: DimmerLink_Mock2 → cmd=DimmerLink_Mock idx=2
    tasmota.add_cmd(self.cmd_prefix,
      / cmd, idx, payload, json -> self.cmd_dimmer(idx, payload))
    tasmota.add_cmd(self.cmd_prefix + "Curve",
      / cmd, idx, payload, json -> self.cmd_curve(payload))
    tasmota.add_cmd(self.cmd_prefix + "Fade",
      / cmd, idx, payload, json -> self.cmd_fade(payload))

    tasmota.add_driver(self)

    # read initial state from device
    for ch : 0 .. self.channels - 1
      var cur = self.read8(self.REG_DIM_BASE + ch * 2)
      if cur != nil && cur > 0 && cur <= 100
        self.levels[ch] = cur
        self.power = true
      end
    end

    # detect temperature feature (read TEMP_CURRENT, 0xFF = no sensor)
    var t = self.read8(self.REG_TEMP_CUR)
    if t != nil && t != 0xFF
      self.has_temp = true
    end
  end

  # ─────────────────────────────────────────────
  # Static utility: scan I2C buses for DimmerLink
  # ─────────────────────────────────────────────
  static def scan()
    var found = 0
    for bus_obj : [global.wire1, global.wire2]
      if bus_obj == nil || !bus_obj.enabled()  continue  end
      var bus_num = bus_obj.bus
      var addrs = bus_obj.scan()
      print(format("I2C bus %d:", bus_num))
      for addr : addrs
        # MCU needs dummy read before real read
        bus_obj.read(addr, 0x03, 1)
        var ver = bus_obj.read(addr, 0x03, 1)   # VERSION register
        if ver == 0x01
          bus_obj.read(addr, 0x00, 1)
          var status = bus_obj.read(addr, 0x00, 1)
          bus_obj.read(addr, 0x20, 1)
          var freq = bus_obj.read(addr, 0x20, 1)
          var ready = (status != nil) && (status & 0x01) != 0
          bus_obj.read(addr, 0x10, 1)
          var level = bus_obj.read(addr, 0x10, 1)
          print(format("  0x%02X - DimmerLink v%d (%s, %dHz, level=%d%%)",
            addr, ver,
            ready ? "READY" : "calibrating",
            freq != nil ? freq : 0,
            level != nil ? level : 0))
          found += 1
        end
      end
    end
    if found == 0
      print("No DimmerLink devices found")
    else
      print(format("Found %d DimmerLink device(s)", found))
    end
  end

  # ─────────────────────────────────────────────
  # Static utility: change device I2C address
  # ─────────────────────────────────────────────
  static def change_addr(from_addr, to_addr)
    if to_addr < 0x08 || to_addr > 0x77
      print(format("Error: address must be 0x08-0x77, got 0x%02X", to_addr))
      return
    end
    # find device at from_addr
    var w = tasmota.wire_scan(from_addr)
    if !w
      print(format("Error: no device found at 0x%02X", from_addr))
      return
    end
    # verify it's a DimmerLink (dummy + real read)
    w.read(from_addr, 0x03, 1)
    var ver = w.read(from_addr, 0x03, 1)
    if ver != 0x01
      print(format("Error: device at 0x%02X is not a DimmerLink (version=0x%02X)", from_addr, ver))
      return
    end
    w.read(from_addr, 0x00, 1)
    var status = w.read(from_addr, 0x00, 1)
    var ready = (status != nil) && (status & 0x01) != 0
    print(format("DimmerLink at 0x%02X: v%d, %s", from_addr, ver, ready ? "READY" : "calibrating"))

    # write new address
    print(format("Writing new address 0x%02X...", to_addr))
    w.write(from_addr, 0x30, to_addr, 1)

    # brief delay for address change to take effect
    tasmota.delay(50)

    # verify at new address
    if w.detect(to_addr)
      print(format("Verifying 0x%02X... OK", to_addr))
      print(format("Address changed: 0x%02X -> 0x%02X", from_addr, to_addr))
      print(format(">>> Update preinit.be: DimmerLink(0x%02X, ...)", to_addr))
    else
      # check error at old address
      var err = w.read(from_addr, 0x02, 1)
      if err != nil && err != 0
        print(format("Error: address change failed (error 0x%02X)", err))
      else
        print("Warning: device not responding at new address, may need restart")
      end
    end
  end

  # ─────────────────────────────────────────────
  # Static utility: print help
  # ─────────────────────────────────────────────
  static def help()
    print("DimmerLink Berry I2C driver")
    print("─────────────────────────────────────────")
    print("Setup:")
    print("  DimmerLink.scan()                 - find devices on I2C bus")
    print("  DimmerLink.change_addr(0x50,0x51) - change device address")
    print("  dim = DimmerLink(0x50,\"Name\",ch)  - create instance")
    print("  dim.info()                        - show status + preinit line")
    print("Commands:")
    print("  DimmerLink <0-100>                - set channel 1 (=DimmerLink1)")
    print("  DimmerLink1 <0-100>               - set channel 1")
    print("  DimmerLink2 <0-100>               - set channel 2")
    print("  DimmerLink_Name1 <0-100>          - set channel 1 by name")
    print("  DimmerLinkCurve [ch] <0-2>        - 0=LINEAR 1=RMS 2=LOG")
    print("  DimmerLinkFade <0-255>            - fade time (x100ms)")
    print("  Power<N> ON/OFF                   - virtual relay")
  end

  # ─────────────────────────────────────────────
  # Instance: info — full status + preinit line
  # ─────────────────────────────────────────────
  def info()
    if !self.wire
      print(format('DimmerLink "%s" — NOT CONNECTED', self.label))
      return
    end
    print(format('DimmerLink "%s" at 0x%02X (bus %d)', self.label, self.addr, self.wire.bus))

    # levels per channel
    var s_levels = ""
    for ch : 0 .. self.channels - 1
      if ch > 0  s_levels += " "  end
      s_levels += format("Ch%d=%d%%", ch + 1, self.levels[ch])
    end
    print(format("  Channels:  %d", self.channels))
    print(format("  Levels:    %s", s_levels))
    print(format("  Power:     %s (relay %d)", self.power ? "ON" : "OFF", self.relay_idx + 1))

    # curves per channel
    var curve_names = self.CURVE_NAMES
    var s_curves = ""
    for ch : 0 .. self.channels - 1
      var c = self.read8(self.REG_DIM_BASE + ch * 2 + 1)
      if ch > 0  s_curves += " "  end
      if c != nil && c <= 2
        s_curves += format("Ch%d=%s", ch + 1, curve_names[c])
      else
        s_curves += format("Ch%d=?", ch + 1)
      end
    end
    print(format("  Curve:     %s", s_curves))

    # fade
    var fade = self.read8(self.REG_FADE)
    if fade != nil
      print(format("  Fade:      %d (%.1fs)", fade, fade * 0.1))
    end

    # AC freq
    var freq = self.read8(self.REG_FREQ)
    if freq != nil
      print(format("  AC freq:   %d Hz", freq))
    end

    # firmware
    var ver = self.read8(self.REG_VER)
    if ver != nil
      print(format("  Firmware:  v%d", ver))
    end

    # temperature
    if self.has_temp
      var temp = self.get_temp()
      var tstate = self.get_temp_state_name()
      if temp != nil
        print(format("  Temp:      %d°C (%s)", temp, tstate))
      end
    end

    print(format("  Ready:     %s", self.ready ? "Yes" : "No"))
    print(format('  preinit:   DimmerLink(0x%02X, "%s", %d)', self.addr, self.label, self.channels))
  end

  # ─────────────────────────────────────────────
  # Core: set brightness on device
  # ─────────────────────────────────────────────
  def set_level(ch, level)
    if ch < 0 || ch >= self.channels  return  end
    if level < 0    level = 0    end
    if level > 100  level = 100  end
    self.levels[ch] = level
    self.write8(self.REG_DIM_BASE + ch * 2, level)
  end

  def check_ready()
    if !self.wire  return false  end
    var status = self.read8(self.REG_STATUS)
    self.ready = status != nil && (status & 0x01) != 0
    return self.ready
  end

  def get_temp()
    if !self.has_temp  return nil  end
    var raw = self.read8(self.REG_TEMP_CUR)
    if raw == nil  return nil  end
    return raw - 50
  end

  def get_temp_state_name()
    if !self.has_temp  return nil  end
    var state = self.read8(self.REG_TEMP_ST)
    var names = ["NORMAL", "WARNING", "DERATE", "CRITICAL", "SHUTDOWN", "FAULT"]
    if state != nil && state < size(names)
      return names[state]
    end
    return "UNKNOWN"
  end

  # ─────────────────────────────────────────────
  # Power handler — Tasmota virtual relay
  # ─────────────────────────────────────────────
  def set_power_handler(cmd, idx)
    var ps = tasmota.get_power()
    if self.relay_idx < 0 || self.relay_idx >= size(ps)  return  end
    var new_power = ps[self.relay_idx]
    if new_power == self.power  return  end
    self.power = new_power
    for ch : 0 .. self.channels - 1
      if self.power
        var lvl = self.levels[ch] > 0 ? self.levels[ch] : 100
        self.set_level(ch, lvl)
      else
        self.write8(self.REG_DIM_BASE + ch * 2, 0)
      end
    end
  end

  # ─────────────────────────────────────────────
  # Command: DimmerLink[_Name]<N> <0-100>
  # Tasmota convention: trailing digit = idx (1-based)
  #   DimmerLink_Mock 50   → idx=1, channel 1
  #   DimmerLink_Mock1 50  → idx=1, channel 1
  #   DimmerLink_Mock2 50  → idx=2, channel 2
  # ─────────────────────────────────────────────
  def cmd_dimmer(idx, payload)
    import string
    payload = string.tr(payload, ' ', '')
    if size(payload) == 0 || string.find(payload, '-') > 0
      tasmota.resp_cmnd_failed()
      return
    end
    var val = int(payload)
    if val < 0 || val > 100
      tasmota.resp_cmnd_failed()
      return
    end
    var ch = idx - 1   # idx is 1-based
    if ch < 0 || ch >= self.channels
      tasmota.resp_cmnd_failed()
      return
    end
    self.set_level(ch, val)
    self._sync_power()
    tasmota.resp_cmnd(format('{"%s%d":%d}', self.cmd_prefix, idx, self.levels[ch]))
  end

  # ─────────────────────────────────────────────
  # Command: DimmerLink[_Name]Curve [ch] <0-2>
  # ─────────────────────────────────────────────
  def cmd_curve(payload)
    var parts = string.split(payload, ' ')
    var ch = 0
    var val
    if size(parts) >= 2
      ch = int(parts[0]) - 1
      val = int(parts[1])
    else
      val = int(parts[0])
    end
    if ch < 0 || ch >= self.channels || val < 0 || val > 2
      tasmota.resp_cmnd_failed()
      return
    end
    self.write8(self.REG_DIM_BASE + ch * 2 + 1, val)
    tasmota.resp_cmnd(format('{"%sCurve":{"Ch":%d,"Curve":"%s"}}',
      self.cmd_prefix, ch + 1, self.CURVE_NAMES[val]))
  end

  # ─────────────────────────────────────────────
  # Command: DimmerLink[_Name]Fade <0-255>
  # ─────────────────────────────────────────────
  def cmd_fade(payload)
    var val = int(payload)
    if val < 0 || val > 255
      tasmota.resp_cmnd_failed()
      return
    end
    self.write8(self.REG_FADE, val)
    tasmota.resp_cmnd(format('{"%sFade":%d}', self.cmd_prefix, val))
  end

  # ─────────────────────────────────────────────
  # Internal: sync virtual relay with channel levels
  # ─────────────────────────────────────────────
  def _sync_power()
    var any_on = false
    for ch : 0 .. self.channels - 1
      if self.levels[ch] > 0  any_on = true  end
    end
    if any_on && !self.power
      tasmota.set_power(self.relay_idx, true)
      self.power = true
    elif !any_on && self.power
      tasmota.set_power(self.relay_idx, false)
      self.power = false
    end
  end

  def _levels_json()
    var s = "["
    for i : 0 .. self.channels - 1
      if i > 0  s += ","  end
      s += str(self.levels[i])
    end
    return s + "]"
  end

  # ─────────────────────────────────────────────
  # Tasmota hook: periodic check
  # ─────────────────────────────────────────────
  def every_second()
    if !self.wire  return  end
    if !self.ready
      self.check_ready()
    end
  end

  # ─────────────────────────────────────────────
  # Tasmota hook: web UI — slider per channel
  # ─────────────────────────────────────────────
  def web_add_main_button()
    import webserver
    for ch : 0 .. self.channels - 1
      var ch_num = ch + 1
      # Build command: "DimmerLink_Name<N>" where N is channel number
      # Tasmota parses trailing digits as idx, so we use the base prefix
      # and append channel number. For single channel, just use prefix (idx=1 default).
      var cmd_str
      if self.channels > 1
        cmd_str = format("%s%d", self.cmd_prefix, ch_num)
      else
        cmd_str = self.cmd_prefix
      end
      var ch_label
      if self.channels > 1
        ch_label = format("%s Ch%d", self.label, ch_num)
      else
        ch_label = self.label
      end
      webserver.content_send(format(
        '<div style="text-align:center;margin:6px 0">'
          '<b>%s</b><br>'
          '<input type="range" min="0" max="100" value="%d" '
            'oninput="this.nextElementSibling.textContent=this.value+\'%%\'" '
            'onchange="fetch(\'/cm?cmnd=%s%%20\'+this.value)" '
            'style="width:70%%">'
          '<span>%d%%</span>'
        '</div>',
        ch_label,
        self.levels[ch],
        cmd_str,
        self.levels[ch]))
    end
  end

  # ─────────────────────────────────────────────
  # Tasmota hook: web sensor section
  # ─────────────────────────────────────────────
  def web_sensor()
    if !self.wire  return  end
    var p = self.label
    for ch : 0 .. self.channels - 1
      if self.channels > 1
        tasmota.web_send(format("{s}%s Ch%d{m}%d%%{e}", p, ch + 1, self.levels[ch]))
      else
        tasmota.web_send(format("{s}%s Level{m}%d%%{e}", p, self.levels[ch]))
      end
    end
    tasmota.web_send(format("{s}%s Power{m}%s{e}", p, self.power ? "ON" : "OFF"))
    var freq = self.read8(self.REG_FREQ)
    if freq != nil
      tasmota.web_send(format("{s}%s AC Freq{m}%d Hz{e}", p, freq))
    end
    var ver = self.read8(self.REG_VER)
    if ver != nil
      tasmota.web_send(format("{s}%s Firmware{m}v%d{e}", p, ver))
    end
    if self.has_temp
      var temp = self.get_temp()
      var tstate = self.get_temp_state_name()
      if temp != nil
        tasmota.web_send(format("{s}%s Temp{m}%d&deg;C (%s){e}", p, temp, tstate))
      end
    end
    tasmota.web_send(format("{s}%s Ready{m}%s{e}", p, self.ready ? "Yes" : "No"))
  end

  # ─────────────────────────────────────────────
  # Tasmota hook: MQTT JSON telemetry
  # ─────────────────────────────────────────────
  def json_append()
    if !self.wire  return  end
    var json = format(',"%s":{"Addr":"0x%02X","Power":"%s","Ready":%s',
      self.label, self.addr,
      self.power ? "ON" : "OFF",
      self.ready ? "true" : "false")
    if self.channels == 1
      json += format(',"Level":%d', self.levels[0])
    else
      json += ',"Levels":' + self._levels_json()
    end
    if self.has_temp
      var temp = self.get_temp()
      var tstate = self.get_temp_state_name()
      if temp != nil
        json += format(',"Temp":%d,"ThermalState":"%s"', temp, tstate)
      end
    end
    json += '}'
    tasmota.response_append(json)
  end

  # ─────────────────────────────────────────────
  # Cleanup: unregister commands and driver
  # ─────────────────────────────────────────────
  def deinit()
    tasmota.remove_cmd(self.cmd_prefix)
    tasmota.remove_cmd(self.cmd_prefix + "Curve")
    tasmota.remove_cmd(self.cmd_prefix + "Fade")
    tasmota.remove_driver(self)
  end

end

return DimmerLink
