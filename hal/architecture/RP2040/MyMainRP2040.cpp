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
 * Main function for RP2040 architecture (Earle Philhower's arduino-pico core).
 *
 * Rather than defining our own main() — which would shadow the core's archived
 * main.o and therefore drop the core's native initialisation (system clock, USB
 * CDC / Serial, and crucially the conditional second-core launch
 * multicore_launch_core1() that arduino-pico performs when a sketch defines
 * setup1()/loop1()) — we reuse the core's own main() and inject the MySensors
 * stack via macro substitution. This mirrors the established ESP8266 HAL
 * (MyMainESP8266.cpp).
 *
 * The preprocessor only substitutes the whole tokens `setup` and `loop`:
 *   - `setup()` in the core main becomes `_begin()` (MySensors startup, which
 *     internally calls the sketch's setup()).
 *   - `loop()`  in the core main becomes `_my_sensors_loop()` (_process() plus
 *     the sketch's loop()).
 * The tokens `setup1`, `loop1`, and `__loop` are distinct and are left
 * untouched, so arduino-pico's dual-core launch logic is fully preserved.
 */

// Declare MySensors functions (defined in the MySensors library)
extern void _begin(void);
extern void _process(void);

// Helper that the core main's loop() call is redirected to: run the MySensors
// transport state machine, then the sketch's loop().
inline void _my_sensors_loop(void)
{
	// Process incoming data / transport state machine
	_process();
	// Call of loop() in the Arduino sketch
	loop();
}

/*
 * Inject the MySensors calls to _begin() and _process() into the arduino-pico
 * core main (cores/rp2040/main.cpp). This keeps all native initialisation,
 * including the second-core launch, intact.
 */

// Start up MySensors library including call of setup() in the Arduino sketch
#define setup _begin
// Helper function to _process() and call of loop() in the Arduino sketch
#define loop _my_sensors_loop

// Pull in arduino-pico's own core main (cores/rp2040/main.cpp).
//
// NOTE: the core file is literally named `main.cpp`, which collides with the
// usual PlatformIO sketch at `src/main.cpp` (PlatformIO puts both `src` and the
// core dir on the include path, and `src` is searched first). A bare
// `#include <main.cpp>` therefore wrongly re-includes the sketch and explodes
// with redefinition errors. The `../rp2040/` prefix disambiguates: it only
// resolves to a real file from the core's own include dir
// (`.../cores/rp2040/../rp2040/main.cpp`), never from `src` or any other -I dir.
#include <../rp2040/main.cpp>

// Tidy up injection defines
#undef loop
#undef setup
