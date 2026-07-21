# Waveshare ESP32-S3 AMOLED 2.06" Wardriver

WiFi & BLE wardriving firmware for the Waveshare ESP32-S3 Touch AMOLED 2.06" display.

## Features

✅ **Real-time WiFi Scanning**
- Displays all nearby WiFi networks on the AMOLED screen
- Shows signal strength (RSSI) visualization by channel
- Identifies open networks with `*` marker
- Channel noise analysis

✅ **SD Card Logging** (Phase 1)
- Logs all discovered networks to CSV file
- WiGLE.net compatible format
- Auto-creates `wardriver_logs/` directory on SD card

✅ **Battery Efficient**
- Power-saving sleep modes
- Backlight dimming support
- ~2-3 hours active scanning on typical battery

✅ **Works Out-of-the-Box**
- Uses Waveshare's official Arduino_GFX library
- Optimized for ESP32-S3 Touch AMOLED 2.06
- No external library hunts

## Hardware Requirements

- **Waveshare ESP32-S3 Touch AMOLED 2.06"** (240×536 ST7789 display)
- USB-C cable for flashing
- Micro SD card (optional, for logging)
- Battery or USB power

## Software Setup

### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP32 Board Support
In Arduino IDE:
- **File** → **Preferences**
- Paste this URL in "Additional Boards Manager URLs":
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- **Tools** → **Board** → **Boards Manager**
- Search for "ESP32"
- Install the latest version (currently v3.x.x)

### 3. Install Required Libraries
**Sketch** → **Include Library** → **Manage Libraries**

Search for and install:
- `Arduino_GFX_Library` by MoonOnOur (latest version)
- `SD` by Arduino (built-in, usually)

### 4. Select Board Settings
- **Board:** ESP32-S3 (or "Generic ESP32-S3")
- **USB CDC On Boot:** Enable
- **CPU Frequency:** 240 MHz
- **Flash Size:** 16 MB
- **Partition Scheme:** Default
- **Upload Speed:** 921600
- **Port:** (select your COM port)

### 5. Flash the Firmware
- Open `wardriver.ino` in Arduino IDE
- Click **Upload** (or Ctrl+U)
- Wait for compile + flash (~30 seconds)

## Usage

1. **Power on** the device
2. WiFi analyzer starts automatically
3. **Real-time display** shows networks per channel with signal strength visualization
4. **Statistics** at bottom show lowest-noise WiFi channels
5. **SD logging** (if enabled) saves to `/wardriver_logs/scan_YYYYMMDD_HHMMSS.csv`

### CSV Log Format (WiGLE compatible)
```
BSSID,SSID,Channel,RSSI,Encryption,FirstSeen
AA:BB:CC:DD:EE:FF,"Network Name",6,-65,WPA2,2026-07-21T14:30:00Z
```

## Roadmap

- [x] Phase 1: WiFi scanning + display
- [x] Phase 1: SD card CSV logging
- [ ] Phase 2: BLE device scanning
- [ ] Phase 2: GPS integration (NEO-6M module)
- [ ] Phase 2: UI menu system (start/stop/settings)
- [ ] Phase 3: WiGLE.net direct upload

## Architecture

```
wardriver.ino              ← Main loop, WiFi scanning
sd_logger.h                ← SD card file operations
wifi_analyzer.h            ← WiFi display rendering
```

## Troubleshooting

### Display shows garbage or doesn't initialize
- Verify board selection: **Tools** → **Board** → should show "ESP32-S3"
- Try upload speed 115200 if 921600 fails
- Check USB cable (must support data, not just power)

### No SD card detected
- Format micro SD as FAT32 first
- Insert firmly until click
- Check serial output: `Serial.begin(115200)` prints debug messages

### WiFi scan shows no networks
- Ensure WiFi router is powered on
- Try moving closer to router
- Antenna placement matters—keep device flat and away from metal

## Credits

- **Base code:** Waveshare ESP32-S3 AMOLED examples
- **Display library:** [Arduino_GFX by MoonOnOur](https://github.com/moononournation/Arduino_GFX_Library)
- **WiFi Analyzer concept:** Public domain wireless scanning examples

## License

MIT License - See LICENSE file

## Contributing

Fork, modify, and submit PRs! Areas we want help with:
- BLE scanning
- GPS integration
- UI menu system
- Power optimization
- WiGLE.net API integration

---

**Happy wardriving! 📡**
