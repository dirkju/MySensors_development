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
 * Hardware Abstraction Layer (HAL) implementation for Raspberry Pi RP2040
 */

#include "MyHwRP2040.h"

// External C function for heap pointer
extern "C" char *sbrk(int incr);

/**
 * @brief Initialize hardware
 * @return true if initialization successful
 */
bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	// Initialize USB CDC serial
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);

#if defined(MY_GATEWAY_SERIAL)
	// Wait for USB CDC connection (gateway mode)
	// Timeout after 5 seconds to allow headless operation
	const unsigned long start = hwMillis();
	while (!MY_SERIALDEVICE && (hwMillis() - start < 5000)) {
		// Wait for USB serial or timeout
	}
#endif // MY_GATEWAY_SERIAL
#endif // MY_DISABLED_SERIAL

	// Initialize EEPROM emulation
	// arduino-pico EEPROM library uses last 4KB of flash
	EEPROM.begin(EEPROM_SIZE);

	// Note: Watchdog is not enabled by default
	// User can enable via watchdog_enable() if desired
	// Example: watchdog_enable(8300, false); // 8.3 second timeout

	return true;
}

// ================================
// Configuration Storage (EEPROM)
// ================================

uint8_t hwReadConfig(const int addr)
{
	// EEPROM.begin() already called in hwInit()
	return EEPROM.read(addr);
}

void hwWriteConfig(const int addr, uint8_t value)
{
	// Only write if value changed (reduces flash wear)
	if (EEPROM.read(addr) != value) {
		EEPROM.write(addr, value);
		EEPROM.commit();  // Must commit to persist to flash
	}
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(addr);

	for (size_t i = 0; i < length; i++) {
		*dst++ = EEPROM.read(offs + i);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	const uint8_t *src = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(addr);
	bool changed = false;

	// Write all bytes, tracking if any changed
	for (size_t i = 0; i < length; i++) {
		const uint8_t value = *src++;
		if (EEPROM.read(offs + i) != value) {
			EEPROM.write(offs + i, value);
			changed = true;
		}
	}

	// Only commit if something changed (reduces flash wear)
	if (changed) {
		EEPROM.commit();
	}
}

// ================================
// System Control
// ================================

void hwWatchdogReset(void)
{
#if defined(MY_HW_WATCHDOG)
	watchdog_update();
#endif
}

void hwReboot(void)
{
	// Use watchdog for clean reset
	watchdog_enable(1, true);  // 1ms timeout, reset enabled
	while (true) {
		tight_loop_contents();  // Wait for watchdog reset
	}
}

void hwRandomNumberInit(void)
{
	uint32_t seed = 0;

	// Method 1: Use ROSC (Ring Oscillator) randombit for true randomness
	// RP2040 has a hardware TRNG via the ring oscillator
	for (int i = 0; i < 32; i++) {
		const uint32_t random_bit = rosc_hw->randombit;
		seed = (seed << 1) | random_bit;
	}

	// Method 2: Add ADC noise for additional entropy
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);  // Temperature sensor has noise
	const uint16_t adc_noise = adc_read();
	seed ^= adc_noise;
	adc_set_temp_sensor_enabled(false);

	// Seed Arduino random number generator
	randomSeed(seed);
}

#ifdef MY_HW_HAS_GETENTROPY
ssize_t hwGetentropy(void *buffer, size_t length)
{
	if (buffer == NULL || length == 0) {
		return -1;
	}

	uint8_t *buf = static_cast<uint8_t *>(buffer);

	// Generate random bytes using ROSC hardware TRNG
	for (size_t i = 0; i < length; i++) {
		uint8_t random_byte = 0;
		// Generate 8 random bits
		for (int bit = 0; bit < 8; bit++) {
			random_byte = (random_byte << 1) | (rosc_hw->randombit & 1);
		}
		buf[i] = random_byte;
	}

	return static_cast<ssize_t>(length);
}
#endif // MY_HW_HAS_GETENTROPY

// ================================
// Hardware Information
// ================================

bool hwUniqueID(unique_id_t *uniqueID)
{
	if (uniqueID == NULL) {
		return false;
	}

	pico_unique_board_id_t board_id;
	pico_get_unique_board_id(&board_id);

	// board_id.id is 8 bytes (flash ROM ID)
	// Copy to first 8 bytes of uniqueID
	memcpy(uniqueID, board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES);

	// Pad remaining bytes with zeros (uniqueID is already a pointer to array)
	memset((uint8_t*)uniqueID + PICO_UNIQUE_BOARD_ID_SIZE_BYTES, 0,
	       sizeof(unique_id_t) - PICO_UNIQUE_BOARD_ID_SIZE_BYTES);

	return true;
}

uint16_t hwCPUVoltage(void)
{
	// RP2040 has VSYS/3 divider on ADC channel 3
	// VSYS is the main system voltage (USB 5V or VSYS pin)

	adc_init();
	adc_set_temp_sensor_enabled(false);
	adc_select_input(3);  // ADC3 = VSYS/3

	// Take multiple readings for accuracy
	uint32_t sum = 0;
	for (int i = 0; i < 8; i++) {
		sum += adc_read();
		delayMicroseconds(100);
	}
	const uint16_t raw = sum / 8;

	// Convert to voltage:
	// ADC is 12-bit (0-4095)
	// Reference voltage is 3.3V
	// VSYS/3 means multiply by 3
	// voltage_mv = (raw * 3300 * 3) / 4096
	const uint32_t voltage_mv = (raw * 9900UL) / 4096;

	return static_cast<uint16_t>(voltage_mv);
}

uint16_t hwCPUFrequency(void)
{
	// Get system clock frequency in Hz
	const uint32_t freq_hz = clock_get_hz(clk_sys);

	// Convert to 0.1 MHz units
	// 133 MHz = 1330 units
	return static_cast<uint16_t>(freq_hz / 100000UL);
}

int8_t hwCPUTemperature(void)
{
	// Enable temperature sensor
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);  // ADC4 = temperature sensor

	// CRITICAL: Allow sensor to settle (arduino-pico uses 1ms)
	// Without this delay, readings can be very inaccurate (off by 20-30°C)
	delay(1);

	// Take multiple readings for accuracy
	uint32_t sum = 0;
	for (int i = 0; i < 8; i++) {
		sum += adc_read();
		delayMicroseconds(100);
	}
	const uint16_t raw = sum / 8;

	adc_set_temp_sensor_enabled(false);

	// Convert to temperature using RP2040 datasheet formula
	// T = 27 - (ADC_voltage - 0.706) / 0.001721
	// ADC_voltage = (raw * 3.3) / 4096
	const float voltage = (raw * 3.3f) / 4096.0f;
	const float temp_c = 27.0f - ((voltage - 0.706f) / 0.001721f);

	return static_cast<int8_t>(temp_c);
}

uint16_t hwFreeMem(void)
{
	char top;
	// sbrk(0) returns current heap end
	// &top is approximate stack pointer
	// Free memory is between heap and stack
	return &top - reinterpret_cast<char *>(sbrk(0));
}

// ================================
// Sleep Functions (Phase 1: Stubs)
// ================================
// Phase 1: All sleep functions return MY_SLEEP_NOT_POSSIBLE
// Phase 2: Full implementation with <10µA target

int8_t hwSleep(uint32_t ms)
{
	(void)ms;  // Unused in Phase 1
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	(void)interrupt;  // Unused in Phase 1
	(void)mode;       // Unused in Phase 1
	(void)ms;         // Unused in Phase 1
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms)
{
	(void)interrupt1;  // Unused in Phase 1
	(void)mode1;       // Unused in Phase 1
	(void)interrupt2;  // Unused in Phase 1
	(void)mode2;       // Unused in Phase 1
	(void)ms;          // Unused in Phase 1
	return MY_SLEEP_NOT_POSSIBLE;
}

uint32_t hwGetSleepRemaining(void)
{
	// Phase 1: Sleep not implemented yet, return 0
	return 0;
}
