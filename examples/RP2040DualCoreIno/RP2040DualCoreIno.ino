/**
 * RP2040 dual-core + MySensors smoke test (Phase 3 / T1) — Arduino IDE / arduino-cli
 * variant of examples/RP2040DualCore (PlatformIO). Same purpose: verify the
 * macro-injection MyMainRP2040.cpp fix compiles under the Arduino toolchain and
 * preserves the Core 1 launch (setup1/loop1) alongside MySensors on Core 0.
 *
 * No radio module required (MY_TRANSPORT_WAIT_READY_MS bounds the boot wait).
 */

#define MY_DEBUG
#define MY_RADIO_RFM95
#define MY_NODE_ID 116
#define MY_TRANSPORT_WAIT_READY_MS 5000

#define MY_RP2040_SPI_DEVICE SPI1
#define MY_RFM95_CS_PIN  13
#define MY_RFM95_IRQ_PIN 14
#define MY_RFM95_IRQ_NUM 14
#define MY_RFM95_RST_PIN 15

#include <MySensors.h>

volatile uint32_t core1Ticks = 0;

void presentation()
{
	sendSketchInfo("RP2040 DualCore Test", "1.0");
}

void setup()
{
	Serial.println(F("[Core0] setup() done - MySensors up"));
}

void loop()
{
	static uint32_t last = 0;
	const uint32_t now = millis();
	if (now - last >= 2000) {
		last = now;
		Serial.print(F("[Core0] up="));
		Serial.print(now / 1000);
		Serial.print(F("s  core1Ticks="));
		Serial.println(core1Ticks);
	}
}

void setup1()
{
}

void loop1()
{
	core1Ticks++;
	delay(1);
}
