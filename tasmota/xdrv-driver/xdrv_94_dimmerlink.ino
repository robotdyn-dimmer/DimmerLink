/*
  xdrv_94_dimmerlink.ino - DimmerLink AC TRIAC dimmer support for Tasmota

  SPDX-FileCopyrightText: 2026 DimmerLink Project

  SPDX-License-Identifier: GPL-3.0-only
*/

#ifdef USE_I2C
#ifdef USE_DIMMERLINK
/*********************************************************************************************\
 * DimmerLink - CIU32 AC TRIAC Dimmer Controller
 *
 * I2C multi-channel dimmer with up to 4 channels per device, up to 4 devices on same bus.
 * Features: brightness 0-100%, 3 dimming curves (LINEAR/RMS/LOG), fade control,
 *           AC frequency detection (50/60Hz), temperature monitoring, thermal protection.
 *
 * I2C Address: 0x50 (default), configurable 0x08-0x77
 *
 * Commands:
 *   DlDim[<dev>] [<ch>,]<level>       - Set brightness 0-100 (dev=1-4, ch=1-4)
 *   DlCurve[<dev>] [<ch>,]<curve>     - Set curve 0=LINEAR, 1=RMS, 2=LOG
 *   DlFade[<dev>] <value>             - Set fade time 0-255 (x100ms)
 *   DlStatus[<dev>]                   - Show full device status
 *   DlReset[<dev>]                    - Software reset
 *   DlRecalibrate[<dev>]              - Re-calibrate AC frequency
 *   DlAddress[<dev>] <new_addr>       - Change I2C address (hex 0x08-0x77)
\*********************************************************************************************/

#define XDRV_94                    94
#define XI2C_100                   100   // See I2CDEVICES.md

#define DL_MAX_DEVICES             4
#define DL_MAX_CHANNELS            4

#ifndef USE_DIMMERLINK_ADDR
  #define USE_DIMMERLINK_ADDR      0x50
#endif
#ifndef USE_DIMMERLINK_CHANNELS
  #define USE_DIMMERLINK_CHANNELS  4
#endif

// Register map
#define DL_REG_STATUS              0x00
#define DL_REG_COMMAND             0x01
#define DL_REG_ERROR               0x02
#define DL_REG_VERSION             0x03
#define DL_REG_DIM_BASE            0x10  // +N*2 = LEVEL, +N*2+1 = CURVE
#define DL_REG_FADE                0x18
#define DL_REG_AC_FREQ             0x20
#define DL_REG_AC_PERIOD_L         0x21
#define DL_REG_AC_PERIOD_H         0x22
#define DL_REG_CALIBRATION         0x23
#define DL_REG_I2C_ADDR            0x30
#define DL_REG_TEMP_CURRENT        0x40
#define DL_REG_TEMP_STATE          0x41

// Status register bits
#define DL_STATUS_READY            0x01
#define DL_STATUS_ERROR            0x02

// Commands
#define DL_CMD_NOP                 0x00
#define DL_CMD_RESET               0x01
#define DL_CMD_RECALIBRATE         0x02

// Firmware version fingerprint
#define DL_FW_VERSION              0x01

// Dimming curves
#define DL_CURVE_LINEAR            0
#define DL_CURVE_RMS               1
#define DL_CURVE_LOG               2

// Error codes
#define DL_ERR_OK                  0x00
#define DL_ERR_SYNTAX              0xF9
#define DL_ERR_NOT_READY           0xFC
#define DL_ERR_INDEX               0xFD
#define DL_ERR_PARAM               0xFE

// Thermal states
#define DL_TEMP_NORMAL             0
#define DL_TEMP_WARNING            1
#define DL_TEMP_DERATE             2
#define DL_TEMP_CRITICAL           3
#define DL_TEMP_SHUTDOWN           4
#define DL_TEMP_FAULT              5

/*********************************************************************************************/

struct DlChannel {
  uint8_t level;               // 0-100 brightness
  uint8_t curve;               // 0=LINEAR, 1=RMS, 2=LOG
};

struct DlDevice {
  uint8_t addr;                // I2C address
  uint8_t bus;                 // I2C bus index
  uint8_t channels;            // Number of active channels (1-4)
  uint8_t version;             // Firmware version
  uint8_t ac_freq;             // Detected AC frequency (50 or 60)
  uint8_t fade_time;           // Fade time 0-255 (x100ms)
  uint8_t temp_raw;            // Temperature register raw value
  uint8_t temp_state;          // Thermal protection state
  uint8_t last_error;          // Last error code
  bool has_temp;               // Temperature sensor available
  bool ready;                  // Device ready flag
  bool power;                  // Virtual relay state (ON/OFF)
  uint8_t relay_offset;        // Index of first relay in TasmotaGlobal.devices_present
  uint8_t last_level[DL_MAX_CHANNELS]; // Last non-zero level per channel (for Power ON restore)
  uint8_t web_level[DL_MAX_CHANNELS];  // Last level sent to web slider
  DlChannel ch[DL_MAX_CHANNELS];
};

struct {
  DlDevice *dev[DL_MAX_DEVICES];
  uint8_t count;               // Number of detected devices
} *Dl = nullptr;

// No PROGMEM — direct access needed for format strings; total ~70 bytes RAM, acceptable
const char kDlCurveNames[][8] = { "LINEAR", "RMS", "LOG" };
const char kDlTempStates[][9] = { "NORMAL", "WARNING", "DERATE", "CRITICAL", "SHUTDOWN", "FAULT" };

/*********************************************************************************************\
 * I2C Helpers - CIU32 requires dummy read to set register pointer
\*********************************************************************************************/

uint8_t DL_Read8(uint8_t addr, uint8_t reg, uint8_t bus) {
  I2cRead8(addr, reg, bus);                    // Dummy read: sets register pointer
  return I2cRead8(addr, reg, bus);             // Real read
}

bool DL_ValidRead8(uint8_t *data, uint8_t addr, uint8_t reg, uint8_t bus) {
  I2cRead8(addr, reg, bus);                    // Dummy read
  return I2cValidRead8(data, addr, reg, bus);  // Real read with validation
}

bool DL_Write8(uint8_t addr, uint8_t reg, uint8_t val, uint8_t bus) {
  return I2cWrite8(addr, reg, val, bus);
}

/*********************************************************************************************\
 * Device access helpers - use indices to avoid Arduino auto-prototype issues
\*********************************************************************************************/

// Get 0-based device index from XdrvMailbox.index (1-based, 0 defaults to 1)
int32_t DL_DevIdx(void) {
  uint32_t idx = XdrvMailbox.index;
  if (0 == idx) { idx = 1; }
  if (idx > Dl->count) { return -1; }
  return idx - 1;
}

int DL_GetTemp(uint8_t d) {
  if (!Dl->dev[d]->has_temp || Dl->dev[d]->temp_raw == 0xFF) { return -999; }
  return (int)Dl->dev[d]->temp_raw - 50;
}

/*********************************************************************************************\
 * Core operations - all use device index (0-based)
\*********************************************************************************************/

void DL_SetLevel(uint8_t d, uint8_t ch, uint8_t level) {
  if (!Dl->dev[d]->ready) { return; }
  if (ch >= Dl->dev[d]->channels) { return; }
  if (level > 100) { level = 100; }
  DL_Write8(Dl->dev[d]->addr, DL_REG_DIM_BASE + ch * 2, level, Dl->dev[d]->bus);
  Dl->dev[d]->ch[ch].level = level;
  uint8_t err;
  if (DL_ValidRead8(&err, Dl->dev[d]->addr, DL_REG_ERROR, Dl->dev[d]->bus) && err != DL_ERR_OK) {
    Dl->dev[d]->last_error = err;
    AddLog(LOG_LEVEL_DEBUG, PSTR("DLK: Dev 0x%02X ch%d set level %d err 0x%02X"), Dl->dev[d]->addr, ch, level, err);
  }
}

void DL_SetCurve(uint8_t d, uint8_t ch, uint8_t curve) {
  if (!Dl->dev[d]->ready) { return; }
  if (ch >= Dl->dev[d]->channels) { return; }
  if (curve > DL_CURVE_LOG) { return; }
  DL_Write8(Dl->dev[d]->addr, DL_REG_DIM_BASE + ch * 2 + 1, curve, Dl->dev[d]->bus);
  Dl->dev[d]->ch[ch].curve = curve;
}

void DL_SetFade(uint8_t d, uint8_t fade) {
  if (!Dl->dev[d]->ready) { return; }
  DL_Write8(Dl->dev[d]->addr, DL_REG_FADE, fade, Dl->dev[d]->bus);
  Dl->dev[d]->fade_time = fade;
}

void DL_SendCmd(uint8_t d, uint8_t cmd) {
  DL_Write8(Dl->dev[d]->addr, DL_REG_COMMAND, cmd, Dl->dev[d]->bus);
}

/*********************************************************************************************\
 * Virtual Power Relay - sync Tasmota Power state with channel levels
\*********************************************************************************************/

// Check if any channel is on, update Tasmota Power state accordingly
void DL_SyncPower(uint8_t d) {
  DlDevice *dev = Dl->dev[d];
  bool any_on = false;
  for (uint8_t ch = 0; ch < dev->channels; ch++) {
    if (dev->ch[ch].level > 0) {
      any_on = true;
      dev->last_level[ch] = dev->ch[ch].level;  // Remember last non-zero level
    }
  }
  if (any_on != dev->power) {
    dev->power = any_on;
    ExecuteCommandPower(dev->relay_offset + 1, any_on ? POWER_ON : POWER_OFF, SRC_SWITCH);
  }
}

// Handle Power ON/OFF from Tasmota (FUNC_SET_DEVICE_POWER)
void DL_SetPower(void) {
  power_t rpower = XdrvMailbox.index;
  for (uint8_t d = 0; d < Dl->count; d++) {
    DlDevice *dev = Dl->dev[d];
    bool new_power = bitRead(rpower, dev->relay_offset);
    if (new_power == dev->power) { continue; }
    dev->power = new_power;
    for (uint8_t ch = 0; ch < dev->channels; ch++) {
      if (dev->power) {
        uint8_t lvl = dev->last_level[ch] > 0 ? dev->last_level[ch] : 100;
        DL_SetLevel(d, ch, lvl);
      } else {
        DL_SetLevel(d, ch, 0);
      }
    }
  }
}

/*********************************************************************************************\
 * Initialization and detection
\*********************************************************************************************/

void DL_InitDevice(uint8_t d) {
  DlDevice *dev = Dl->dev[d];
  for (uint8_t ch = 0; ch < dev->channels; ch++) {
    uint8_t val;
    if (DL_ValidRead8(&val, dev->addr, DL_REG_DIM_BASE + ch * 2, dev->bus)) {
      dev->ch[ch].level = (val <= 100) ? val : 0;
    }
    if (DL_ValidRead8(&val, dev->addr, DL_REG_DIM_BASE + ch * 2 + 1, dev->bus)) {
      dev->ch[ch].curve = (val <= DL_CURVE_LOG) ? val : DL_CURVE_LINEAR;
    }
    dev->web_level[ch] = dev->ch[ch].level;
  }
  uint8_t val;
  if (DL_ValidRead8(&val, dev->addr, DL_REG_FADE, dev->bus)) {
    dev->fade_time = val;
  }
  if (DL_ValidRead8(&val, dev->addr, DL_REG_AC_FREQ, dev->bus)) {
    dev->ac_freq = val;
  }
  if (DL_ValidRead8(&val, dev->addr, DL_REG_TEMP_CURRENT, dev->bus)) {
    dev->has_temp = (val != 0xFF);
    dev->temp_raw = val;
  }
  if (dev->has_temp) {
    if (DL_ValidRead8(&val, dev->addr, DL_REG_TEMP_STATE, dev->bus)) {
      dev->temp_state = val;
    }
  }
}

void DL_Detect(void) {
  for (uint8_t bus = 0; bus < MAX_I2C; bus++) {
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      if (Dl && Dl->count >= DL_MAX_DEVICES) { break; }
      if (!I2cSetDevice(addr, bus)) { continue; }

      uint8_t ver;
      if (!DL_ValidRead8(&ver, addr, DL_REG_VERSION, bus)) { continue; }
      if (ver != DL_FW_VERSION) { continue; }

      uint8_t status;
      if (!DL_ValidRead8(&status, addr, DL_REG_STATUS, bus)) { continue; }

      if (!Dl) {
        Dl = (decltype(Dl))calloc(sizeof(*Dl), 1);
        if (!Dl) { return; }
      }

      DlDevice *dev = (DlDevice*)calloc(sizeof(DlDevice), 1);
      if (!dev) { continue; }

      dev->addr = addr;
      dev->bus = bus;
      dev->version = ver;
      dev->ready = (status & DL_STATUS_READY) != 0;
      dev->channels = USE_DIMMERLINK_CHANNELS;

      Dl->dev[Dl->count] = dev;
      DL_InitDevice(Dl->count);

      // Register virtual relay for this device
      dev->relay_offset = TasmotaGlobal.devices_present;
      UpdateDevicesPresent(1);

      // Set initial last_level for Power ON restore; power starts false
      // so first DL_SyncPower() call will set the correct Tasmota Power state
      for (uint8_t ch = 0; ch < dev->channels; ch++) {
        dev->last_level[ch] = (dev->ch[ch].level > 0) ? dev->ch[ch].level : 100;
      }
      dev->power = false;  // Will be synced on first EverySecond or DlDim

      Dl->count++;

      I2cSetActiveFound(addr, "DimmerLink", bus);
      AddLog(LOG_LEVEL_INFO, PSTR("DLK: DimmerLink v%d at 0x%02X bus%d (%dch, %dHz, relay%d, %s)"),
        ver, addr, bus + 1, dev->channels, dev->ac_freq, dev->relay_offset + 1,
        dev->ready ? PSTR("READY") : PSTR("calibrating"));
    }
  }
}

/*********************************************************************************************\
 * Periodic polling
\*********************************************************************************************/

void DL_EverySecond(void) {
  for (uint8_t d = 0; d < Dl->count; d++) {
    DlDevice *dev = Dl->dev[d];
    uint8_t status;
    if (DL_ValidRead8(&status, dev->addr, DL_REG_STATUS, dev->bus)) {
      dev->ready = (status & DL_STATUS_READY) != 0;
    }
    if (dev->has_temp) {
      uint8_t val;
      if (DL_ValidRead8(&val, dev->addr, DL_REG_TEMP_CURRENT, dev->bus)) {
        dev->temp_raw = val;
      }
      if (DL_ValidRead8(&val, dev->addr, DL_REG_TEMP_STATE, dev->bus)) {
        dev->temp_state = val;
        if (val >= DL_TEMP_CRITICAL) {
          AddLog(LOG_LEVEL_ERROR, PSTR("DLK: Dev 0x%02X thermal %s"),
            dev->addr, kDlTempStates[val <= DL_TEMP_FAULT ? val : DL_TEMP_FAULT]);
        }
      }
    }
    for (uint8_t ch = 0; ch < dev->channels; ch++) {
      uint8_t val;
      if (DL_ValidRead8(&val, dev->addr, DL_REG_DIM_BASE + ch * 2, dev->bus)) {
        if (val <= 100) { dev->ch[ch].level = val; }
      }
    }
    // Sync virtual relay state with actual channel levels
    DL_SyncPower(d);
  }
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

const char kDlCommands[] PROGMEM = "Dl|"  // Prefix
  "Dim|Curve|Fade|Status|Reset|Recalibrate|Address";

void (* const DlCommands[])(void) PROGMEM = {
  &CmndDlDim, &CmndDlCurve, &CmndDlFade, &CmndDlStatus,
  &CmndDlReset, &CmndDlRecalibrate, &CmndDlAddress };

void CmndDlDim(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DlDevice *dev = Dl->dev[d];

  if (XdrvMailbox.data_len > 0) {
    int32_t ch_in = 0;
    int32_t level_in;
    char *comma = strchr(XdrvMailbox.data, ',');
    if (comma) {
      *comma = '\0';
      ch_in = strtol(XdrvMailbox.data, nullptr, 10);
      level_in = strtol(comma + 1, nullptr, 10);
      if (ch_in < 1 || ch_in > dev->channels) { ResponseCmndError(); return; }
      ch_in--;  // 1-based to 0-based
    } else {
      level_in = strtol(XdrvMailbox.data, nullptr, 10);
    }
    if (level_in < 0 || level_in > 100) { ResponseCmndError(); return; }
    DL_SetLevel(d, ch_in, level_in);
    DL_SyncPower(d);
  }
  Response_P(PSTR("{\"DlDim%d\":{"), d + 1);
  for (uint8_t i = 0; i < dev->channels; i++) {
    ResponseAppend_P(PSTR("%s\"Ch%d\":%d"), i ? "," : "", i + 1, dev->ch[i].level);
  }
  ResponseJsonEndEnd();
}

void CmndDlCurve(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DlDevice *dev = Dl->dev[d];

  if (XdrvMailbox.data_len > 0) {
    int32_t ch_in = 0;
    int32_t curve_in;
    char *comma = strchr(XdrvMailbox.data, ',');
    if (comma) {
      *comma = '\0';
      ch_in = strtol(XdrvMailbox.data, nullptr, 10);
      curve_in = strtol(comma + 1, nullptr, 10);
      if (ch_in < 1 || ch_in > dev->channels) { ResponseCmndError(); return; }
      ch_in--;
    } else {
      curve_in = strtol(XdrvMailbox.data, nullptr, 10);
    }
    if (curve_in < 0 || curve_in > DL_CURVE_LOG) { ResponseCmndError(); return; }
    DL_SetCurve(d, ch_in, curve_in);
  }
  Response_P(PSTR("{\"DlCurve%d\":{"), d + 1);
  for (uint8_t i = 0; i < dev->channels; i++) {
    ResponseAppend_P(PSTR("%s\"Ch%d\":\"%s\""), i ? "," : "", i + 1,
      (dev->ch[i].curve <= DL_CURVE_LOG) ? kDlCurveNames[dev->ch[i].curve] : "?");
  }
  ResponseJsonEndEnd();
}

void CmndDlFade(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }

  if ((XdrvMailbox.payload >= 0) && (XdrvMailbox.payload <= 255)) {
    DL_SetFade(d, XdrvMailbox.payload);
  }
  ResponseCmndNumber(Dl->dev[d]->fade_time);
}

void CmndDlStatus(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DlDevice *dev = Dl->dev[d];

  Response_P(PSTR("{\"DimmerLink%d\":{\"Addr\":\"0x%02X\",\"Bus\":%d,\"Ready\":%s,\"FW\":%d,\"ACFreq\":%d,\"Fade\":%d"),
    d + 1, dev->addr, dev->bus + 1,
    dev->ready ? PSTR("true") : PSTR("false"),
    dev->version, dev->ac_freq, dev->fade_time);

  for (uint8_t ch = 0; ch < dev->channels; ch++) {
    ResponseAppend_P(PSTR(",\"Ch%d\":{\"Level\":%d,\"Curve\":\"%s\"}"),
      ch + 1, dev->ch[ch].level,
      (dev->ch[ch].curve <= DL_CURVE_LOG) ? kDlCurveNames[dev->ch[ch].curve] : "?");
  }

  if (dev->has_temp) {
    int temp_c = DL_GetTemp(d);
    ResponseAppend_P(PSTR(",\"Temperature\":%d,\"ThermalState\":\"%s\""),
      temp_c, (dev->temp_state <= DL_TEMP_FAULT) ? kDlTempStates[dev->temp_state] : "UNKNOWN");
  }

  if (dev->last_error != DL_ERR_OK) {
    ResponseAppend_P(PSTR(",\"LastError\":\"0x%02X\""), dev->last_error);
  }

  ResponseJsonEndEnd();
}

void CmndDlReset(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DL_SendCmd(d, DL_CMD_RESET);
  Dl->dev[d]->ready = false;
  ResponseCmndDone();
}

void CmndDlRecalibrate(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DL_SendCmd(d, DL_CMD_RECALIBRATE);
  Dl->dev[d]->ready = false;
  ResponseCmndDone();
}

void CmndDlAddress(void) {
  int32_t d = DL_DevIdx();
  if (d < 0) { ResponseCmndError(); return; }
  DlDevice *dev = Dl->dev[d];

  if (XdrvMailbox.data_len > 0) {
    uint32_t new_addr;
    if (XdrvMailbox.data[0] == '0' && (XdrvMailbox.data[1] == 'x' || XdrvMailbox.data[1] == 'X')) {
      new_addr = strtoul(XdrvMailbox.data, nullptr, 16);
    } else {
      new_addr = atoi(XdrvMailbox.data);
    }

    if (new_addr >= 0x08 && new_addr <= 0x77 && new_addr != dev->addr) {
      uint8_t old_addr = dev->addr;
      DL_Write8(dev->addr, DL_REG_I2C_ADDR, new_addr, dev->bus);
      delay(50);
      uint8_t ver;
      if (DL_ValidRead8(&ver, new_addr, DL_REG_VERSION, dev->bus) && ver == DL_FW_VERSION) {
        I2cResetActive(old_addr, dev->bus);          // Release old address
        dev->addr = new_addr;
        I2cSetActiveFound(new_addr, "DimmerLink", dev->bus);  // Register new address
        AddLog(LOG_LEVEL_INFO, PSTR("DLK: Address changed 0x%02X -> 0x%02X"), old_addr, new_addr);
        Response_P(PSTR("{\"DlAddress\":\"0x%02X\"}"), new_addr);
        return;
      } else {
        AddLog(LOG_LEVEL_ERROR, PSTR("DLK: Address change failed at 0x%02X"), new_addr);
        ResponseCmndError();
        return;
      }
    }
  }
  Response_P(PSTR("{\"DlAddress\":\"0x%02X\"}"), dev->addr);
}

/*********************************************************************************************\
 * Telemetry
\*********************************************************************************************/

void DL_Show(bool json) {
  for (uint8_t d = 0; d < Dl->count; d++) {
    DlDevice *dev = Dl->dev[d];
    if (json) {
      ResponseAppend_P(PSTR(",\"DimmerLink%d\":{\"Addr\":\"0x%02X\",\"ACFreq\":%d,\"Fade\":%d"),
        d + 1, dev->addr, dev->ac_freq, dev->fade_time);
      for (uint8_t ch = 0; ch < dev->channels; ch++) {
        ResponseAppend_P(PSTR(",\"Ch%d\":{\"Level\":%d,\"Curve\":\"%s\"}"),
          ch + 1, dev->ch[ch].level,
          (dev->ch[ch].curve <= DL_CURVE_LOG) ? kDlCurveNames[dev->ch[ch].curve] : "?");
      }
      if (dev->has_temp) {
        int temp_c = DL_GetTemp(d);
        ResponseAppend_P(PSTR(",\"Temp\":%d,\"Thermal\":\"%s\""),
          temp_c, (dev->temp_state <= DL_TEMP_FAULT) ? kDlTempStates[dev->temp_state] : "?");
      }
      ResponseJsonEnd();
#ifdef USE_WEBSERVER
    } else {
      char label[20];
      snprintf_P(label, sizeof(label), PSTR("DimmerLink%d"), d + 1);
      for (uint8_t ch = 0; ch < dev->channels; ch++) {
        WSContentSend_PD(PSTR("{s}%s Ch%d{m}%d%% (%s){e}"),
          label, ch + 1, dev->ch[ch].level,
          (dev->ch[ch].curve <= DL_CURVE_LOG) ? kDlCurveNames[dev->ch[ch].curve] : "?");
      }
      WSContentSend_PD(PSTR("{s}%s AC{m}%d Hz{e}"), label, dev->ac_freq);
      WSContentSend_PD(PSTR("{s}%s Fade{m}%d (%d.%ds){e}"), label, dev->fade_time, dev->fade_time / 10, dev->fade_time % 10);
      if (dev->has_temp) {
        int temp_c = DL_GetTemp(d);
        const char *thermal = (dev->temp_state <= DL_TEMP_FAULT) ? kDlTempStates[dev->temp_state] : "?";
        WSContentSend_PD(PSTR("{s}%s Temp{m}%d " D_UNIT_CELSIUS " (%s){e}"), label, temp_c, thermal);
      }
      DL_WebUpdateSliders(d);
#endif  // USE_WEBSERVER
    }
  }
}

/*********************************************************************************************\
 * Web interface
\*********************************************************************************************/

#ifdef USE_WEBSERVER

const char HTTP_MSG_SLIDER_DL[] PROGMEM =
  "<tr>"
  "<td style='width:15%%'><span style='font-size:11px'>%s</span></td>"
  "<td style='width:85%%'><div class='r' style='background-image:linear-gradient(to right,#000,#FFF);'>"
  "<input id='i94%d' type='range' min='0' max='100' value='%d' onchange='lc(\"i\",94%d,value)'></div></td>"
  "</tr>";

void DL_WebAddMainSlider(void) {
  WSContentSend_P(HTTP_TABLE100);
  char slabel[24];
  for (uint8_t d = 0; d < Dl->count; d++) {
    DlDevice *dev = Dl->dev[d];
    for (uint8_t ch = 0; ch < dev->channels; ch++) {
      uint8_t sid = d * DL_MAX_CHANNELS + ch;
      dev->web_level[ch] = dev->ch[ch].level;
      if (Dl->count > 1) {
        snprintf_P(slabel, sizeof(slabel), PSTR("DL%d Ch%d"), d + 1, ch + 1);
      } else {
        snprintf_P(slabel, sizeof(slabel), PSTR("DL Ch%d"), ch + 1);
      }
      WSContentSend_P(HTTP_MSG_SLIDER_DL,
        slabel,
        sid,
        dev->ch[ch].level,
        sid
      );
    }
  }
  WSContentSend_P(PSTR("</table>"));
}

void DL_WebUpdateSliders(uint8_t d) {
  DlDevice *dev = Dl->dev[d];
  bool need_update = false;
  for (uint8_t ch = 0; ch < dev->channels; ch++) {
    if (dev->ch[ch].level != dev->web_level[ch]) {
      need_update = true;
      break;
    }
  }
  if (!need_update) { return; }

  WSContentSend_P(PSTR("</table>"));
  WSContentSend_P(HTTP_MSG_EXEC_JAVASCRIPT);
  for (uint8_t ch = 0; ch < dev->channels; ch++) {
    if (dev->ch[ch].level != dev->web_level[ch]) {
      uint8_t sid = d * DL_MAX_CHANNELS + ch;
      if (WebUpdateSliderTime()) {
        dev->web_level[ch] = dev->ch[ch].level;
      }
      WSContentSend_P(PSTR("eb('i94%d').value=%d;"), sid, dev->ch[ch].level);
    }
  }
  WSContentSend_P(PSTR("\">{t}"));
}

void DL_WebGetArg(void) {
  char tmp[8];
  char svalue[32];
  char webindex[8];

  for (uint8_t d = 0; d < Dl->count; d++) {
    for (uint8_t ch = 0; ch < Dl->dev[d]->channels; ch++) {
      uint8_t sid = d * DL_MAX_CHANNELS + ch;
      snprintf_P(webindex, sizeof(webindex), PSTR("i94%d"), sid);
      WebGetArg(webindex, tmp, sizeof(tmp));
      if (strlen(tmp)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("DlDim%d %d,%s"), d + 1, ch + 1, tmp);
        ExecuteWebCommand(svalue);
      }
    }
  }
}

#endif  // USE_WEBSERVER

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv94(uint32_t function) {
  if (!I2cEnabled(XI2C_100)) { return false; }

  bool result = false;

  if (FUNC_INIT == function) {
    DL_Detect();
  }
  else if (Dl) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        DL_EverySecond();
        break;
      case FUNC_JSON_APPEND:
        DL_Show(true);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        DL_Show(false);
        break;
      case FUNC_WEB_ADD_MAIN_BUTTON:
        DL_WebAddMainSlider();
        break;
      case FUNC_WEB_GET_ARG:
        DL_WebGetArg();
        break;
#endif  // USE_WEBSERVER
      case FUNC_SET_DEVICE_POWER:
        DL_SetPower();
        result = true;
        break;
      case FUNC_COMMAND:
        result = DecodeCommand(kDlCommands, DlCommands);
        break;
      case FUNC_ACTIVE:
        result = true;
        break;
    }
  }
  return result;
}

#endif  // USE_DIMMERLINK
#endif  // USE_I2C
