# WT32-ETH01 PlatformIO Project

## Board Information
- **Board:** Waveshare WT32-ETH01
- **MCU:** ESP32-WROVER (with PSRAM)
- **Ethernet PHY:** IP101
- **Framework:** Arduino (espressif32)

## Pinout
| Function      | GPIO |
|--------------|------|
| ETH CS       | 5    |
| ETH INT      | 16   |
| ETH RESET    | 21   |
| SPI MOSI     | 23   |
| SPI MISO     | 19   |
| SPI SCLK     | 18   |

## Build & Upload
```bash
# Build the project
pio run

# Upload to board
pio run -t upload

# Monitor serial output
pio device monitor
```

## Ethernet Usage
Uncomment the `Ethernet.begin()` call in `src/main.cpp` and configure as needed.
