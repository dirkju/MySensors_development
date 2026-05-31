/**
 * RP2040 dual-core + MySensors smoke test (Phase 3 / T1).
 *
 * Proves the macro-injection MyMainRP2040.cpp fix:
 *   - MySensors initialises on Core 0 (with RFM95 configured)
 *   - Core 1 is launched (setup1/loop1), so a counter incremented on Core 1
 *     keeps advancing — printed from Core 0
 *   - boot does not hang with no radio attached (MY_TRANSPORT_WAIT_READY_MS)
 *
 * This file is intentionally named src/main.cpp: it also verifies that the
 * `#include <main.cpp>` inside the HAL resolves to the arduino-pico CORE main,
 * not this sketch (no infinite include recursion).
 */

// ---- MySensors configuration ----
#define MY_DEBUG
#define MY_RADIO_RFM95
#define MY_NODE_ID 116

// Do not block boot if the radio/uplink is not ready (no module attached yet).
#define MY_TRANSPORT_WAIT_READY_MS 5000

// Radio on hardware SPI1 (default pins GP10 SCK / GP11 MOSI / GP12 MISO),
// clear of Trovis SPI (GP2-5), UART (GP0/1) and LED (GP16).
#define MY_RP2040_SPI_DEVICE SPI1
#define MY_RFM95_CS_PIN  13
#define MY_RFM95_IRQ_PIN 14
#define MY_RFM95_IRQ_NUM 14
#define MY_RFM95_RST_PIN 15

#include <MySensors.h>

// Incremented only on Core 1 to prove the second core is alive.
volatile uint32_t core1Ticks = 0;

void presentation()
{
	sendSketchInfo("RP2040 DualCore Test", "1.0");
}

void setup()
{
	// MySensors has already initialised by the time setup() runs.
	Serial.println(F("[Core0] setup() done - MySensors up"));
}

void loop()
{
	static uint32_t last = 0;
	const uint32_t now = millis();
	if (now - last >= 2000) {
		last = now;
		// If core1Ticks keeps climbing, Core 1 is running under MySensors.
		Serial.print(F("[Core0] up="));
		Serial.print(now / 1000);
		Serial.print(F("s  core1Ticks="));
		Serial.println(core1Ticks);
	}
}

void setup1()
{
	// Runs on Core 1 (launched by arduino-pico's main, preserved by the
	// macro-injection HAL).
}

void loop1()
{
	core1Ticks++;
	delay(1);
}
