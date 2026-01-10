# MySensors HAL for RP2040

This Hardware Abstraction Layer (HAL) provides MySensors support for Raspberry Pi RP2040 microcontroller-based boards, including:

- Raspberry Pi Pico
- Raspberry Pi Pico W
- Adafruit Feather RP2040
- Seeed XIAO RP2040
- Sparkfun RP2040 boards
- Generic RP2040 boards

## Requirements

### Arduino Core

This HAL requires **Earle Philhower's arduino-pico core**:
- Repository: https://github.com/earlephilhower/arduino-pico
- Board Manager URL: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

**Why arduino-pico?**
- Better RAM efficiency than official mbed-based core
- Built-in EEPROM emulation (4KB flash storage)
- Excellent PlatformIO support
- Active community and regular updates

### Hardware

- RP2040-based board (Raspberry Pi Pico or compatible)
- Radio module (nRF24L01+, RFM69, RFM95, etc.)
- USB cable for programming and serial communication

## Installation

### Arduino IDE

1. Add board manager URL:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

2. Install the board:
   - Tools → Board → Boards Manager
   - Search for "Raspberry Pi Pico/RP2040"
   - Install "Raspberry Pi Pico/RP2040" by Earle Philhower

3. Select your board:
   - Tools → Board → Raspberry Pi Pico/RP2040 → Raspberry Pi Pico

4. Install MySensors library:
   - Sketch → Include Library → Manage Libraries
   - Search for "MySensors"
   - Install latest version (2.4.0+)

### PlatformIO

Add to `platformio.ini`:

```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino

lib_deps =
    mysensors/MySensors@^2.3.2
```

## Pin Mapping

### Default SPI Pins (SPI0)

| Signal | GPIO | Pin# | Notes |
|--------|------|------|-------|
| MISO | GPIO 16 | Pin 21 | SPI0 RX |
| MOSI | GPIO 19 | Pin 25 | SPI0 TX |
| SCK | GPIO 18 | Pin 24 | SPI0 SCK |

### Recommended Radio Pins

| Radio Signal | Recommended GPIO | Notes |
|--------------|------------------|-------|
| CE (nRF24) | GPIO 7 | Chip Enable |
| CSN/CS | GPIO 5 | Chip Select |
| IRQ/INT | GPIO 7 | Interrupt (RFM69/95) |
| RST | GPIO 6 | Reset (RFM69/95) |

### Built-in LED

| Component | GPIO | Notes |
|-----------|------|-------|
| LED | GPIO 25 | Built-in LED on Pico |

## Features

### Implemented (Phase 1)

- ✅ Digital I/O via Arduino functions
- ✅ Hardware SPI support
- ✅ EEPROM emulation (4KB flash storage)
- ✅ Unique board ID from flash ROM
- ✅ CPU voltage measurement (VSYS via ADC)
- ✅ CPU frequency reporting
- ✅ CPU temperature sensor (die temperature)
- ✅ Free RAM calculation
- ✅ Hardware watchdog support
- ✅ System reboot function
- ✅ Hardware random number generation (ROSC)
- ✅ Critical sections (atomic operations)

### Not Implemented (Phase 1)

- ❌ Sleep mode support (returns `MY_SLEEP_NOT_POSSIBLE`)
- ❌ WiFi gateway (Pico W support planned for future)

Sleep mode will be added in Phase 2 with <10µA target.

## Example Sketches

### Serial Gateway

```cpp
// Enable debug
#define MY_DEBUG

// Select radio
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 7
#define MY_RF24_CS_PIN 5

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#include <MySensors.h>

void setup() {
    // MySensors handles initialization
}

void presentation() {
    sendSketchInfo("RP2040 Gateway", "1.0");
}

void loop() {
    // MySensors handles gateway operations
}
```

### Temperature Sensor Node

```cpp
#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_NODE_ID 10

#include <MySensors.h>

#define CHILD_ID_TEMP 0

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void presentation() {
    sendSketchInfo("RP2040 Temp Sensor", "1.0");
    present(CHILD_ID_TEMP, S_TEMP);
}

void loop() {
    // Read RP2040 die temperature
    int8_t temp = hwCPUTemperature();

    // Send to gateway
    send(msgTemp.set(temp));

    // Wait 60 seconds (no sleep in Phase 1)
    wait(60000);
}
```

## Configuration

### Serial Device

By default, the HAL uses USB CDC (`Serial`). To use UART:

```cpp
#define MY_SERIALDEVICE Serial1  // UART0 on GPIO 0/1
#define MY_BAUD_RATE 115200
```

### EEPROM Size

Default is 4KB (maximum for arduino-pico):

```cpp
#define EEPROM_SIZE 4096  // Do not exceed 4096
```

### SPI Device

Default is hardware SPI0:

```cpp
#define MY_RP2040_SPI_DEVICE SPI  // Can use SPI1 if needed
```

## Limitations

### Phase 1 Limitations

1. **No sleep support**: All sleep functions return `MY_SLEEP_NOT_POSSIBLE`. Nodes operate in always-on mode consuming ~30mA.

2. **USB serial dependency**: Stock Pico requires USB connection for serial communication. Use UART serial for standalone operation.

3. **EEPROM wear**: Flash-based EEPROM has ~10,000 write cycles per sector. MySensors minimizes writes (only on routing changes).

4. **Temperature accuracy**: Die temperature sensor is uncalibrated (±5°C). Use external sensor for precision measurements.

5. **Single core**: MySensors runs on Core 0. Core 1 is unused (available for future enhancements).

### Hardware Limitations

- Stock Raspberry Pi Pico cannot achieve <10µA sleep current (requires custom board with low-power LDO)
- Power LED consumes 2mA (can be removed for battery operation)

## Performance

### Memory

- **RAM**: 264 KB total
- **Typical usage (gateway)**: ~40 KB used, ~220 KB free
- **Typical usage (node)**: ~25 KB used, ~235 KB free

### Speed

- **CPU**: 133 MHz (dual-core ARM Cortex-M0+)
- **SPI**: Up to 62.5 MHz (radio modules typically use 1-10 MHz)
- **Boot time**: ~1-2 seconds

### Power Consumption

- **Active (no radio)**: ~30 mA @ 133 MHz
- **Active (with radio TX)**: ~40-50 mA
- **Sleep (Phase 1)**: Not supported (always-on)
- **Sleep (Phase 2 planned)**: <10 µA with external RTC and custom hardware

## Troubleshooting

### Board not detected

1. Install arduino-pico core via Boards Manager
2. Hold BOOTSEL button while plugging in USB
3. Board appears as USB mass storage device
4. Select correct board in Tools → Board menu

### Serial output not appearing

1. Verify USB cable supports data (not charge-only)
2. Check `Serial.begin()` is called (automatic in `hwInit()`)
3. For gateways, code waits up to 5 seconds for USB serial connection
4. Try different USB port

### EEPROM values not persisting

1. Verify `EEPROM.begin()` called in `hwInit()` ✓ (automatic)
2. Ensure `EEPROM.commit()` called after writes ✓ (automatic in `hwWriteConfig()`)
3. Check power supply stability during writes
4. Verify not exceeding 4KB EEPROM size

### Radio not communicating

1. Verify pin connections (CE, CSN/CS, SPI pins)
2. Check 3.3V power supply (stable, adequate current)
3. Enable verbose radio debug: `#define MY_DEBUG_VERBOSE_RF24`
4. Try different radio module (hardware fault)
5. Reduce SPI speed if wiring is long

## Roadmap

### Phase 2 (Planned)

- Low-power sleep modes
  - Light sleep: ~390 µA
  - Dormant mode: ~180 µA
  - RTC-assisted: <10 µA (requires external RTC + custom hardware)
- Timer-based wake (up to 49+ days with RTC)
- GPIO interrupt wake
- Dual interrupt wake
- Long sleep support

### Future Enhancements

- WiFi gateway support (Pico W)
- Dual-core optimization (offload radio to Core 1)
- PIO-based radio drivers
- Hardware crypto acceleration
- OTA firmware updates

## Technical Details

### Architecture Detection

The HAL automatically detects RP2040 architecture via:

```cpp
#if defined(ARDUINO_ARCH_RP2040)
```

### Unique ID Source

The 16-byte unique ID is derived from:
- First 8 bytes: RP2040 flash ROM chip ID (via `pico_get_unique_board_id()`)
- Last 8 bytes: Zero-padded

This provides adequate uniqueness for MySensors networks (one flash chip model typically has one ID pattern).

### Random Number Generation

Hardware entropy from:
1. **ROSC (Ring Oscillator)**: `rosc_hw->randombit` provides true random bits
2. **ADC noise**: Temperature sensor ADC adds additional entropy

Quality: Cryptographically random (suitable for security applications)

### System Information Functions

| Function | Source | Accuracy |
|----------|--------|----------|
| `hwCPUVoltage()` | ADC channel 3 (VSYS/3) | ±5% |
| `hwCPUFrequency()` | `clock_get_hz(clk_sys)` | Exact |
| `hwCPUTemperature()` | ADC channel 4 (die temp) | ±5°C |
| `hwFreeMem()` | `sbrk()` heap/stack calculation | Approximate |

## Contributing

Pull requests welcome! Please follow MySensors coding style:
- Tabs for indentation
- K&R brace style
- Doxygen comments
- Test on hardware before submitting

## Support

- MySensors Forum: https://forum.mysensors.org/
- MySensors GitHub: https://github.com/mysensors/MySensors
- arduino-pico Issues: https://github.com/earlephilhower/arduino-pico/issues

## License

This HAL is part of the MySensors library and is licensed under the GNU General Public License v2.0.

## Acknowledgments

- MySensors team for the excellent framework
- Earle Philhower for the arduino-pico core
- Raspberry Pi Foundation for the RP2040 microcontroller
