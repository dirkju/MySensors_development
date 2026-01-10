/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
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
 * Main function for RP2040 architecture
 * Integrates MySensors with arduino-pico core
 */

#include <Arduino.h>

// Declare MySensors functions (defined in MySensors library)
extern void _begin();
extern void _process();

// Declare sketch functions (weak default implementations in MyASM.S)
// Sketches can override these with strong definitions
extern void setup(void);
extern void loop(void);

/**
 * @brief Main entry point
 *
 * Replaces arduino-pico's main() to integrate MySensors framework
 */
int main(void)
{
	// arduino-pico core initialization happens before main()
	// (USB, clocks, etc. are already initialized)

	// Initialize MySensors (which calls setup() internally if it exists)
	_begin();

	// Main loop
	for (;;) {
		// Process MySensors
		_process();

		// Call sketch loop() function (if defined)
		if (loop) {
			loop();
		}

		// Yield to allow background tasks (USB, etc.)
		if (serialEventRun) {
			serialEventRun();
		}
	}

	return 0;
}
