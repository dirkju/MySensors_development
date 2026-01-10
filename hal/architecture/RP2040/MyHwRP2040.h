/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 * Hardware Abstraction Layer (HAL) for Raspberry Pi RP2040 microcontroller
 * Supports Raspberry Pi Pico and other RP2040-based boards
 * Requires Earle Philhower's arduino-pico core: https://github.com/earlephilhower/arduino-pico
 */

#ifndef MyHwRP2040_h
#define MyHwRP2040_h

#include <Arduino.h>
#include <EEPROM.h>
#include <hardware/watchdog.h>
#include <hardware/regs/rosc.h>
#include <pico/unique_id.h>
#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <pico/bootrom.h>

#ifdef __cplusplus
#include <SPI.h>
#endif

// Define architecture
#ifndef ARDUINO_ARCH_RP2040
#error This HAL is for RP2040 architecture only
#endif

// AVR compatibility macros (RP2040 doesn't have PROGMEM)
#ifndef printf_P
#define printf_P printf
#endif

// MySensors types (define if not already defined by MyHwHAL.h)
#ifndef unique_id_t
typedef uint8_t unique_id_t[16];
#endif

// EEPROM size (arduino-pico uses last 4KB of flash)
#ifndef EEPROM_SIZE
#define EEPROM_SIZE (4096)
#endif

// Serial device (USB CDC by default)
#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif

#ifndef MY_BAUD_RATE
#define MY_BAUD_RATE (115200)
#endif

// SPI settings
#ifndef MY_RP2040_SPI_DEVICE
#define MY_RP2040_SPI_DEVICE SPI
#endif

// Default SPI pins for Raspberry Pi Pico (SPI0)
// MISO: GPIO 16, MOSI: GPIO 19, SCK: GPIO 18
#define hwSPI MY_RP2040_SPI_DEVICE

// Disable software SPI (RP2040 has fast hardware SPI)
#ifndef MY_SOFTSPI
#define MY_DISABLED_SERIAL_SPI
#endif

/**
 * @brief Digital I/O macros (use Arduino core functions)
 */
#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __mode) pinMode(__pin, __mode)

/**
 * @brief Timing macros
 */
#define hwMillis() millis()
#define hwMicros() micros()

/**
 * @brief Random number generation flag
 * RP2040 has hardware ROSC (Ring Oscillator) for true random numbers
 */
#define MY_HW_HAS_GETENTROPY

/**
 * @brief Critical section macro for atomic operations
 * Saves and restores interrupt state
 */
#define MY_CRITICAL_SECTION \
	for (uint32_t __irq_state = save_and_disable_interrupts(), __once = 1; \
	     __once; \
	     __once = 0, restore_interrupts(__irq_state))

/**
 * @brief Sleep wake codes (define if not already defined by MySensorsCore.h)
 */
#ifndef MY_WAKE_UP_BY_TIMER
#define MY_WAKE_UP_BY_TIMER ((int8_t)-1)
#endif
#ifndef MY_SLEEP_NOT_POSSIBLE
#define MY_SLEEP_NOT_POSSIBLE ((int8_t)-2)
#endif

/**
 * @brief Interrupt handling
 */
#define MY_HWRP2040_INTERRUPT_NUM_TO_GPIO(__num) (__num)
#define MY_HWRP2040_GPIO_TO_INTERRUPT_NUM(__gpio) (__gpio)

// Interrupt modes (Arduino compatible)
#ifndef CHANGE
#define CHANGE 1
#endif
#ifndef FALLING
#define FALLING 2
#endif
#ifndef RISING
#define RISING 3
#endif

// Forward declarations
bool hwInit(void);

// Digital I/O (already defined as macros above)

// Timing (already defined as macros above)

// Configuration storage (EEPROM emulation)
/**
 * @brief Read a byte from emulated EEPROM
 * @param addr EEPROM address (0 to EEPROM_SIZE-1)
 * @return Value at address
 */
uint8_t hwReadConfig(const int addr);

/**
 * @brief Write a byte to emulated EEPROM
 * @param addr EEPROM address (0 to EEPROM_SIZE-1)
 * @param value Value to write
 */
void hwWriteConfig(const int addr, uint8_t value);

/**
 * @brief Read a block from emulated EEPROM
 * @param buf Destination buffer
 * @param addr EEPROM start address
 * @param length Number of bytes to read
 */
void hwReadConfigBlock(void *buf, void *addr, size_t length);

/**
 * @brief Write a block to emulated EEPROM
 * @param buf Source buffer
 * @param addr EEPROM start address
 * @param length Number of bytes to write
 */
void hwWriteConfigBlock(void *buf, void *addr, size_t length);

// System control
/**
 * @brief Reset watchdog timer
 */
void hwWatchdogReset(void);

/**
 * @brief Perform system reboot
 */
void hwReboot(void);

/**
 * @brief Initialize random number generator with hardware entropy
 */
void hwRandomNumberInit(void);

// Hardware information
/**
 * @brief Get unique board ID
 * @param uniqueID Pointer to 16-byte unique ID structure
 * @return true if successful
 */
bool hwUniqueID(unique_id_t *uniqueID);

/**
 * @brief Get system voltage (VSYS) in millivolts
 * @return Voltage in mV (0 if not supported)
 */
uint16_t hwCPUVoltage(void);

/**
 * @brief Get CPU frequency in 0.1 MHz units
 * @return Frequency (e.g., 1330 = 133.0 MHz)
 */
uint16_t hwCPUFrequency(void);

/**
 * @brief Get CPU die temperature
 * @return Temperature in Celsius (-127 if not supported)
 */
int8_t hwCPUTemperature(void);

/**
 * @brief Get free RAM in bytes
 * @return Free memory in bytes
 */
uint16_t hwFreeMem(void);

// Sleep functions (Phase 1: stubs, Phase 2: implementation)
/**
 * @brief Sleep with timer wake (Phase 1: returns MY_SLEEP_NOT_POSSIBLE)
 * @param ms Sleep duration in milliseconds
 * @return Wake source or error code
 */
int8_t hwSleep(uint32_t ms);

/**
 * @brief Sleep with interrupt and timer wake (Phase 1: returns MY_SLEEP_NOT_POSSIBLE)
 * @param interrupt GPIO pin number
 * @param mode Interrupt mode (CHANGE, FALLING, RISING)
 * @param ms Sleep duration in milliseconds (0 = indefinite)
 * @return Wake source (interrupt number or MY_WAKE_UP_BY_TIMER)
 */
int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms);

/**
 * @brief Sleep with two interrupts and timer wake (Phase 1: returns MY_SLEEP_NOT_POSSIBLE)
 * @param interrupt1 First GPIO pin number
 * @param mode1 First interrupt mode
 * @param interrupt2 Second GPIO pin number
 * @param mode2 Second interrupt mode
 * @param ms Sleep duration in milliseconds (0 = indefinite)
 * @return Wake source (interrupt number or MY_WAKE_UP_BY_TIMER)
 */
int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms);

/**
 * @brief Get remaining sleep time (Phase 1: stub returns 0)
 * @return Remaining sleep time in milliseconds
 */
uint32_t hwGetSleepRemaining(void);

// Optional: Hardware entropy (used by random number generator)
#ifdef MY_HW_HAS_GETENTROPY
/**
 * @brief Fill buffer with hardware random bytes
 * @param buffer Destination buffer
 * @param length Number of bytes to generate
 * @return Number of bytes generated, or -1 on error
 */
ssize_t hwGetentropy(void *buffer, size_t length);
#endif

#endif // MyHwRP2040_h
