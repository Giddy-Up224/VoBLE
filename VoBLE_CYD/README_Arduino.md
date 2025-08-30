# JK BMS Monitor for ESP32 CYD (Cheap Yellow Display)

This project converts the original Python VoBLE JK BMS monitor to Arduino C++ code that runs on an ESP32 with a CYD (Cheap Yellow Display).

## Hardware Requirements

- ESP32-based CYD (Cheap Yellow Display) with:
  - ILI9341 320x240 TFT display
  - Touch screen support
  - ESP32 with Bluetooth capability
- JK BMS with Bluetooth enabled

## Features

- **Bluetooth Low Energy (BLE) Connection**: Connects to JK BMS via BLE
- **Real-time Data Display**: Shows SOC, voltage, current, and temperature
- **LVGL Graphics**: Modern gauge interface using LVGL library
- **Touch Interface**: Responsive touch screen interface
- **Auto-reconnection**: Automatically reconnects if BLE connection is lost

## Arduino IDE Setup

### Required Libraries

Install these libraries through Arduino Library Manager:

1. **TFT_eSPI** by Bodmer (v2.5.34+)
2. **lvgl** (v8.3.11+)

Note: ESP32 BLE libraries are built-in to the ESP32 Arduino core, no additional BLE library needed.

### TFT_eSPI Configuration

Create or modify `TFT_eSPI/User_Setup.h` with these settings for CYD:

```cpp
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TOUCH_CS 21
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
```

## Configuration

### BMS MAC Address

Update the MAC address in the code to match your JK BMS:

```cpp
static const char* BMS_MAC = "C8:47:80:23:4F:95"; // Replace with your BMS MAC address
```

To find your BMS MAC address:
1. Use a BLE scanner app on your phone
2. Look for a device named "JK-" followed by numbers
3. Note the MAC address

### Pin Configuration

The default pin configuration is for standard CYD boards. If you have a different board, update the pin definitions in `platformio.ini` or create a custom `User_Setup.h` file.

## Building and Uploading

### Using Arduino IDE

1. Copy `JK_BMS_CYD.ino` and `lv_conf.h` to your Arduino sketch folder
2. Install required libraries
3. Configure TFT_eSPI as described above
4. Select "ESP32 Dev Module" as the board
5. Upload the sketch

### Using PlatformIO

1. Use the provided `platformio.ini` file
2. Run `pio run -t upload`

## Usage

1. Power on the ESP32 CYD
2. The device will automatically attempt to connect to the JK BMS
3. Once connected, real-time data will be displayed:
   - SOC gauge (0-100%)
   - Battery voltage
   - Current (positive = discharging, negative = charging)
   - Connection status

## Troubleshooting

### Connection Issues

- Ensure BMS Bluetooth is enabled
- Verify MAC address is correct
- Check that BMS is not connected to another device
- Try power cycling both devices

### Display Issues

- Verify TFT_eSPI configuration matches your hardware
- Check pin connections
- Ensure adequate power supply (5V/2A recommended)

### Memory Issues

- If experiencing crashes, reduce LVGL buffer size in `lv_conf.h`
- Consider using PSRAM-enabled ESP32 modules

## Protocol Details

This implementation uses the JK BMS Bluetooth protocol:

- **Service UUID**: `0000ffe0-0000-1000-8000-00805f9b34fb`
- **Characteristic UUID**: `0000ffe1-0000-1000-8000-00805f9b34fb`
- **Commands**:
  - `0x97`: Device information
  - `0x96`: Device state (SOC, voltage, current, etc.)

## Differences from Original VoBLE

- **Language**: Converted from Python to C++
- **Platform**: Runs on ESP32 instead of PC
- **Graphics**: Uses LVGL instead of NiceGUI/Highcharts
- **BLE Stack**: Uses ESP32 BLE instead of Bleak
- **Display**: Real embedded display instead of web interface

## Contributing

Feel free to contribute improvements:
- Add more BMS data fields
- Improve UI design
- Add configuration via touch interface
- Support for other BMS models

## License

Based on the original VoBLE project. Check original license terms.