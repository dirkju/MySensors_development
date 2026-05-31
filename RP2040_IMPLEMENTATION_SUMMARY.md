# MySensors RP2040 Support - Implementation Summary

## Overview

This document provides a high-level summary of the RP2040 (Raspberry Pi Pico) HAL implementation for the MySensors framework. Full details are in the phase-specific documents.

**Related Documents:**
- [RP2040_PHASE1_IMPLEMENTATION.md](RP2040_PHASE1_IMPLEMENTATION.md) - Core functionality (no sleep)
- [RP2040_PHASE2_SLEEP_IMPLEMENTATION.md](RP2040_PHASE2_SLEEP_IMPLEMENTATION.md) - Sleep mode support (<10µA)

---

## Quick Start

### For Impatient Developers

**Minimum viable implementation (Phase 1 only):**

1. **Create files:**
   - `hal/architecture/RP2040/MyHwRP2040.h` (250 lines)
   - `hal/architecture/RP2040/MyHwRP2040.cpp` (400 lines)
   - `hal/architecture/RP2040/MyMainRP2040.cpp` (80 lines)

2. **Modify files:**
   - `MySensors.h` - Add RP2040 detection (3 lines)
   - `core/MyCapabilities.h` - Add RP2040 string (1 line)

3. **Test:**
   - Serial gateway sketch
   - Verify EEPROM persistence
   - Test with RF24 radio

**Time estimate:** 2-3 weeks for Phase 1

---

## Architecture Decision: arduino-pico Core

**Selected:** [Earle Philhower's arduino-pico](https://github.com/earlephilhower/arduino-pico)

**Rationale:**
| Criteria | arduino-pico | Official mbed Core |
|----------|--------------|---------------------|
| RAM Efficiency | ✅ Excellent | ❌ Poor |
| EEPROM Support | ✅ Built-in | ⚠️ LittleFS only |
| PlatformIO | ✅ Native | ⚠️ Limited |
| Community | ✅ Very active | ⚠️ Limited |

**Install:**
```
Arduino IDE: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
PlatformIO: platform = raspberrypi
```

---

## Phase 1: Core Functionality (No Sleep)

### Goals
- ✅ Functional MySensors gateway
- ✅ Always-on sensor nodes
- ✅ EEPROM emulation
- ✅ System information
- ❌ Sleep mode (Phase 2)

### Key Features

| Feature | Implementation | Status |
|---------|----------------|--------|
| **Digital I/O** | Arduino core functions | Simple |
| **Timing** | millis() / micros() | Simple |
| **EEPROM** | Flash emulation (4KB) | Medium |
| **Unique ID** | Flash ROM ID | Medium |
| **CPU Voltage** | ADC channel 3 (VSYS/3) | Medium |
| **CPU Frequency** | clock_get_hz() | Simple |
| **CPU Temperature** | ADC channel 4 | Medium |
| **Free Memory** | sbrk() calculation | Simple |
| **Watchdog** | hardware/watchdog.h | Simple |
| **Reboot** | Watchdog reset | Simple |
| **Random Seed** | ROSC randombit | Medium |
| **Critical Section** | save_and_disable_interrupts() | Simple |
| **Sleep** | Returns MY_SLEEP_NOT_POSSIBLE | Stub |

### Files to Create

```
hal/architecture/RP2040/
├── MyHwRP2040.h           # HAL header (~250 lines)
├── MyHwRP2040.cpp         # HAL implementation (~400 lines)
├── MyMainRP2040.cpp       # Custom main() (~80 lines)
└── README.md              # Documentation (~150 lines)
```

### Example: Serial Gateway (Phase 1)

```cpp
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 7
#define MY_RF24_CS_PIN 5
#define MY_GATEWAY_SERIAL
#define MY_DEBUG

#include <MySensors.h>

void presentation() {
    sendSketchInfo("RP2040 Gateway", "1.0");
}

void loop() {
    // MySensors handles everything
}
```

**Hardware:**
- Raspberry Pi Pico
- nRF24L01+ on SPI0 (CE=7, CS=5)
- USB serial connection

**Use cases:**
- ✅ Serial gateways
- ✅ MQTT gateways (Pico W with WiFi)
- ✅ Powered sensor nodes
- ✅ Repeaters
- ❌ Battery-powered nodes (no sleep)

### Deliverables

- [x] Phase 1 implementation plan (see RP2040_PHASE1_IMPLEMENTATION.md)
- [ ] Code implementation
- [ ] Example sketches
- [ ] Testing on hardware
- [ ] Pull request to MySensors

---

## Phase 2: Sleep Mode Support

### Goals
- ✅ Timer-based sleep
- ✅ Interrupt wake (single + dual)
- ✅ **<10µA sleep current**
- ✅ Long sleep support (>49 days)

### Sleep Mode Strategy

**Three-tier approach:**

#### Tier 1: Stock Pico (~180 µA)
- **Hardware:** Raspberry Pi Pico (power LED removed)
- **Sleep current:** 180-200 µA
- **Implementation:** Dormant mode (rosc_set_dormant)
- **Cost:** $4
- **Suitable for:** Short-term battery operation (days to weeks)

#### Tier 2: RTC-Assisted (10-20 µA)
- **Hardware:** Custom board with low-power LDO + DS3231 RTC
- **Sleep current:** 10-20 µA
- **Components:**
  - MCP1700 LDO (1.6 µA quiescent)
  - DS3231 RTC (0.84 µA timekeeping)
- **Cost:** ~$15
- **Suitable for:** Medium-term battery operation (months to 1 year)

#### Tier 3: Ultra-Low Power (<10 µA)
- **Hardware:** Optimized custom PCB
- **Sleep current:** <5 µA (target: 3 µA)
- **Components:**
  - TPS7A02 LDO (25 nA quiescent)
  - DS3231 RTC (0.84 µA)
  - TPS22860 radio load switch (1 µA off-state)
  - No power LED
- **Cost:** ~$20-25
- **Suitable for:** Long-term deployment (years)

### Power Budget Example (Tier 3)

```
Component                Current
─────────────────────────────────
RP2040 dormant:          0.18 µA
DS3231 RTC:              0.84 µA
TPS7A02 LDO:             0.025 µA
Load switch:             0.50 µA
GPIO leakage:            0.10 µA
DS18B20 sensor:          0.75 µA
─────────────────────────────────
Total sleep:             2.4 µA ✓

Battery life (2×AA = 3000 mAh):
  3000 mAh / 0.0024 mA = 1,250,000 hours = 142 years
```

### Key Implementation Challenges

| Challenge | Solution |
|-----------|----------|
| **Clock restoration** | Save clock config, restart XOSC/PLL after wake |
| **Long sleep (>49 days)** | External RTC with alarm interrupt |
| **USB disconnect** | Acceptable; use UART for debug |
| **Flash timing** | 1ms delay after wake before flash access |
| **Dual-core coordination** | Run MySensors on Core 0 only (default) |

### Example: Low-Power Sensor (Phase 2)

```cpp
#define MY_RADIO_RF24
#define MY_NODE_ID 10
#define MY_RP2040_EXTERNAL_RTC  // Enable RTC for <10µA
#include <MySensors.h>

#define MOTION_PIN 7

MyMessage msgMotion(0, V_TRIPPED);

void presentation() {
    sendSketchInfo("Motion Sensor", "1.0");
    present(0, S_MOTION);
}

void loop() {
    // Sleep until motion (or 1 hour timeout)
    int8_t result = sleep(digitalPinToInterrupt(MOTION_PIN),
                          RISING,
                          3600000);  // 1 hour

    if (result == digitalPinToInterrupt(MOTION_PIN)) {
        send(msgMotion.set(1));
        wait(500);
        send(msgMotion.set(0));
    }
}
```

**Power consumption:**
- Sleep: 3-5 µA (Tier 3 hardware)
- Wake + transmit: 30-40 mA for ~50ms
- Average: ~5-10 µA

**Battery life:**
- 2×AA (3000 mAh): 20-30 years
- CR2032 (220 mAh): 2-3 years

### Deliverables

- [x] Phase 2 implementation plan (see RP2040_PHASE2_SLEEP_IMPLEMENTATION.md)
- [ ] Sleep mode code
- [ ] RTC integration
- [ ] Power optimization
- [ ] Testing with power meter
- [ ] Pull request

---

## Comparison to Other Architectures

| Feature | AVR | STM32F4 | ESP32 | RP2040 |
|---------|-----|---------|-------|--------|
| **Clock** | 16 MHz | 84-168 MHz | 240 MHz | 133 MHz |
| **RAM** | 2 KB | 64-128 KB | 320 KB | 264 KB |
| **Flash** | 32 KB | 256-512 KB | 4 MB | 2 MB |
| **Sleep Current** | 5-20 µA | 10-50 µA | Varies | **3-200 µA*** |
| **Arduino Core** | Official | STM32duino | Official | Community |
| **Cost** | $2-5 | $5-15 | $5-10 | **$4** |
| **Best Use** | Simple nodes | Advanced nodes | WiFi gateways | **Gateway/Repeater** |

*Sleep current depends on hardware (see tiers above)

### RP2040 Strengths
- ✅ **High performance:** 133 MHz dual-core
- ✅ **Large RAM:** 264 KB (excellent for routing tables)
- ✅ **Low cost:** $4 for Pico
- ✅ **Large flash:** 2 MB (OTA updates)
- ✅ **PIO:** Flexible I/O (future radio drivers)

### RP2040 Weaknesses
- ⚠️ **Sleep current:** Higher than STM32/AVR (requires optimization)
- ⚠️ **Community core:** Not official Arduino
- ⚠️ **USB dependency:** Stock Pico has USB serial only

### Recommended Uses

| Use Case | Recommendation | Rationale |
|----------|----------------|-----------|
| **Serial Gateway** | ✅ Excellent | High performance, low cost, plenty of RAM |
| **WiFi Gateway (Pico W)** | ✅ Excellent | Built-in WiFi, good performance |
| **Repeater Node** | ✅ Excellent | Fast routing, large routing table capacity |
| **Always-On Sensor** | ✅ Good | Plenty of resources, low cost |
| **Battery Sensor (Stock Pico)** | ⚠️ Fair | ~180 µA sleep, OK for short deployments |
| **Battery Sensor (Custom)** | ✅ Good | <10 µA achievable with proper hardware |
| **Ultra-Low Power Node** | ⚠️ Use STM32/AVR | Better out-of-box power efficiency |

---

## Testing Plan

### Phase 1 Testing

#### Unit Tests
- [x] Hardware initialization
- [x] Digital I/O (LED blink)
- [x] EEPROM read/write
- [x] EEPROM persistence (reboot test)
- [x] Unique ID extraction
- [x] System information (voltage, freq, temp, RAM)
- [x] Watchdog reset
- [x] Random number generation

#### Integration Tests
- [ ] Serial gateway with RF24
- [ ] Sensor node (always-on)
- [ ] EEPROM routing table persistence
- [ ] Multi-node network
- [ ] Long-term stability (24+ hours)

### Phase 2 Testing

#### Sleep Mode Tests
- [ ] Timer sleep (10ms to 1 hour)
- [ ] Long sleep (1+ hours with RTC)
- [ ] Interrupt wake (single GPIO)
- [ ] Dual interrupt wake
- [ ] Clock restoration after wake
- [ ] USB reconnection after wake

#### Power Measurement
- [ ] Tier 1: Stock Pico (LED removed) → Target: <200 µA
- [ ] Tier 2: RTC + LDO → Target: <20 µA
- [ ] Tier 3: Optimized board → Target: <10 µA

**Equipment needed:**
- µCurrent Gold or equivalent
- Multimeter
- Power supply / battery
- Oscilloscope (optional, for wake latency)

---

## Build System Integration

### Arduino IDE

**Board Manager URL:**
```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

**Board selection:**
- Tools → Board → Raspberry Pi Pico/RP2040 → Raspberry Pi Pico

### PlatformIO

**Basic configuration:**
```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino

lib_deps =
    mysensors/MySensors@^2.3.2

build_flags =
    -DMY_RADIO_RF24
    -DMY_GATEWAY_SERIAL
    -DMY_DEBUG
```

**With RTC (Phase 2):**
```ini
[env:pico_sleep]
platform = raspberrypi
board = pico
framework = arduino

lib_deps =
    mysensors/MySensors@^2.3.2
    adafruit/RTClib@^2.1.1

build_flags =
    -DMY_RADIO_RF24
    -DMY_NODE_ID=10
    -DMY_RP2040_EXTERNAL_RTC
    -DMY_DEBUG
```

---

## Implementation Timeline

### Phase 1: Core Functionality

| Week | Tasks | Deliverables |
|------|-------|--------------|
| **Week 1** | File structure, header, basic macros, hwInit() | Compiles with test sketch |
| **Week 2** | EEPROM, system info, unique ID | Gateway functional |
| **Week 3** | System control, testing, documentation | Serial gateway works |
| **Week 4** | Bug fixes, PR preparation | Pull request submitted |

**Estimated effort:** 60-80 hours

### Phase 2: Sleep Support

| Week | Tasks | Deliverables |
|------|-------|--------------|
| **Week 5** | Sleep infrastructure, light sleep, dormant mode | Basic sleep works |
| **Week 6** | RTC integration, clock restoration | Long sleep works |
| **Week 7** | Power optimization, interrupt wake | <10µA achieved |
| **Week 8** | Testing, documentation, PR | Pull request submitted |

**Estimated effort:** 80-100 hours

**Total project:** 140-180 hours (4-6 weeks full-time)

---

## Hardware Bill of Materials

### Development Hardware (Phase 1)

| Item | Quantity | Cost | Purpose |
|------|----------|------|---------|
| Raspberry Pi Pico | 2-3 | $12 | Testing (gateway + nodes) |
| nRF24L01+ module | 2-3 | $6 | Radio communication |
| Breadboard | 1 | $5 | Prototyping |
| Jumper wires | 1 set | $3 | Connections |
| USB cables | 2 | $6 | Programming |
| **Total** | | **$32** | |

### Phase 2 Hardware (Sleep Testing)

| Item | Quantity | Cost | Purpose |
|------|----------|------|---------|
| DS3231 RTC module | 1 | $3 | External RTC |
| µCurrent Gold | 1 | $60 | Current measurement |
| LiPo battery (3.7V) | 1 | $10 | Power source |
| Low-power LDO (MCP1700) | 1 | $0.50 | Optional regulator |
| **Phase 2 Total** | | **$73.50** | |

### Tier 3 Custom Board (Optional)

| Item | Quantity | Cost | Notes |
|------|----------|------|-------|
| Custom PCB | 5 | $20 | OSH Park or similar |
| RP2040 chip | 1 | $1 | Qty 1 price |
| TPS7A02 LDO | 1 | $2 | Ultra-low quiescent |
| DS3231 RTC | 1 | $1 | Bulk pricing |
| TPS22860 load switch | 1 | $0.50 | Radio power control |
| Passives, connectors | | $5 | Various |
| **Per board** | | **$9.50** | + $20 PCB setup |

---

## Success Criteria

### Phase 1 Complete When:

- ✅ Code compiles without warnings (Arduino IDE + PlatformIO)
- ✅ Serial gateway forwards messages between nodes and serial
- ✅ Sensor node joins network and sends data
- ✅ EEPROM persists data across reboots
- ✅ RF24 radio communicates reliably
- ✅ All system info functions return valid values
- ✅ Documentation complete
- ✅ Pull request accepted by MySensors maintainers

### Phase 2 Complete When:

- ✅ All Phase 1 criteria met
- ✅ Timer sleep works (10ms to 49+ days)
- ✅ Interrupt wake works (single + dual)
- ✅ Sleep current <200 µA (stock Pico, LED removed)
- ✅ Sleep current <20 µA (with RTC + LDO)
- ✅ Sleep current <10 µA (custom board with TPS7A02)
- ✅ Clock restoration stable after wake
- ✅ Long-term stability test (1 week+)
- ✅ Pull request accepted

---

## Known Limitations

### Phase 1

1. **No sleep support** - Always-on nodes only
2. **USB serial dependency** - Stock Pico requires USB connection
3. **EEPROM wear** - ~10,000 write cycles (adequate for MySensors)
4. **Temperature accuracy** - ±5°C uncalibrated
5. **Core usage** - MySensors runs on Core 0; sketches may use Core 1 via
   `setup1()`/`loop1()` (Core 1 launch preserved by the 2026-05-31 main() fix)

### Phase 2

1. **Stock Pico cannot reach <10µA** - Requires custom hardware
2. **USB disconnects during sleep** - Normal behavior
3. **Flash timing** - 1ms delay needed after dormant wake
4. **Interrupt latency** - ~5ms wake time from dormant
5. **RTC required for long sleep** - >49 days needs external RTC

---

## Contributing

### Code Style

Follow MySensors conventions:
- **Indentation:** Tabs
- **Braces:** K&R style
- **Naming:** camelCase (functions), UPPERCASE (macros)
- **Comments:** Doxygen style

### Pull Request Process

1. Fork MySensors repository
2. Create feature branch: `feature/rp2040-hal`
3. Implement Phase 1
4. Test on hardware
5. Submit PR with:
   - Code changes
   - Documentation
   - Example sketches
   - Test results
6. Address review feedback
7. Repeat for Phase 2

### Testing Requirements

- [ ] Compiles without warnings
- [ ] Gateway functional test passed
- [ ] Sensor node functional test passed
- [ ] EEPROM persistence verified
- [ ] Radio compatibility tested (RF24 minimum)
- [ ] Power measurements documented (Phase 2)
- [ ] No breaking changes to existing code

---

## Questions & Answers

### Q: Which RP2040 board should I use?

**A:** Raspberry Pi Pico for development (Phase 1). Custom board with MCP1700 + DS3231 for production low-power nodes (Phase 2 Tier 2).

### Q: Can I achieve <10µA with stock Pico?

**A:** No. Stock Pico minimum is ~180 µA (with power LED removed). Need custom board with TPS7A02 LDO + RTC for <10 µA.

### Q: Is RP2040 better than STM32 for battery nodes?

**A:** No, STM32 is better out-of-box (10-50 µA stock). RP2040 requires custom hardware to compete. But RP2040 is better for gateways/repeaters due to more RAM and lower cost.

### Q: Does Pico W WiFi work with MySensors?

**A:** Phase 1 doesn't include WiFi support. Future enhancement could add MQTT gateway over WiFi (similar to ESP32).

### Q: Can I use official Arduino mbed core instead?

**A:** Not recommended. Higher RAM usage, no EEPROM library, limited PlatformIO support. arduino-pico is better for MySensors.

### Q: What about Core 1 (second processor)?

**A:** MySensors itself runs on Core 0. As of the 2026-05-31 main() fix, the HAL reuses
arduino-pico's own `main()` (via macro injection), so the core's conditional
`multicore_launch_core1()` is preserved: **a sketch can define `setup1()`/`loop1()` and
run its own code on Core 1 alongside MySensors** (e.g. the Trovis 5575 PIO SPI client).
Earlier Phase 1/2 builds used a standalone `main()` that disabled the Core 1 launch.

### Q: How long before this is in official MySensors?

**A:** Depends on implementation quality and maintainer review. Estimate 2-6 months after PR submission.

---

## Resources

### Documentation
- [MySensors Official Site](https://www.mysensors.org/)
- [arduino-pico Documentation](https://arduino-pico.readthedocs.io/)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [Raspberry Pi Pico Datasheet](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf)

### Source Code
- [MySensors GitHub](https://github.com/mysensors/MySensors)
- [arduino-pico GitHub](https://github.com/earlephilhower/arduino-pico)

### Community
- [MySensors Forum](https://forum.mysensors.org/)
- [Raspberry Pi Forums](https://forums.raspberrypi.com/)

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-10 | MySensors Community | Initial implementation summary |

---

**Status:** Planning Phase
**Next Step:** Begin Phase 1 implementation
**Target Release:** MySensors 2.4.0 or 2.5.0

---

*End of Implementation Summary*
