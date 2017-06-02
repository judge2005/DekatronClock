#include <Arduino.h>

#define LED_PIN 13         // Test LED
#define GUIDE_1_PIN 5       // Guide 1 - G1 pin of 2-guide Dekatron
#define GUIDE_2_PIN 6       // Guide 2 - G2 pin of 2-guide Dekatron
#define INDEX_PIN 7       // INDEX_PIN   - NDX input pin. High when glow at K0

/**
 * A class that moves the lit pin forwards or backwards one step
 */
struct Stepper {
	int cathode;

	Stepper() {
		cathode = 0;
	}

	void stepToCathode() // Dekatron Stepper
	{
		if (cathode == 0)	// Main cathode
				{
			digitalWrite(GUIDE_1_PIN, LOW);
			digitalWrite(GUIDE_2_PIN, LOW);
		}
		if (cathode == 1)	// Guide 1 cathode
				{
			digitalWrite(GUIDE_1_PIN, HIGH);
			digitalWrite(GUIDE_2_PIN, LOW);
		}
		if (cathode == 2)	// Guide 2 cathode
				{
			digitalWrite(GUIDE_1_PIN, LOW);
			digitalWrite(GUIDE_2_PIN, HIGH);
		}
	}

	void forward() {
		cathode = (cathode + 1) % 3;
		stepToCathode();
	}

	void back() {
		cathode = (cathode + 5) % 3;
		stepToCathode();
	}
};

/**
 * Lights up the given pins. Pins are specified in an array. So for example
 * you can give an array with three elements, each of which contains the number
 * of a pin to light up. You can change the pins at any time, but not the number
 * of pins.
 *
 * We use it to display a clock, but it could be used to move any (small) number of
 * pins around in any way.
 */
struct PinSet {
	byte *pins;
	int numPins;
	int pinIdx;
	byte currPin;
	byte moveTo;
	boolean set;
	Stepper &stepper;

	unsigned long lastTick;
	unsigned long lastMove;

	PinSet(int numPins, Stepper &stepper) :
		pins(new byte(numPins)),
		numPins(numPins),
		pinIdx(0),
		currPin(0),
		moveTo(0),
		set(false),
		stepper(stepper),
		lastTick(0),
		lastMove(0)
	{
		for (int i=0; i<numPins; i++) {
			pins[i] = 0;
		}
	}

	/**
	 * Move the pin to zero - all pins will be relative to this.
	 */
	void zeroSet() {
		if (!digitalRead(INDEX_PIN)) {
			stepper.forward();
		} else {
			set = true;
		}
	}

	/**
	 * Call periodically with the current number of microseconds.
	 */
	void periodic(unsigned long now) {
		if (now - lastTick >= 80) {	// Do this once per 80 us. Any faster and the dekatron gets confused.
			lastTick = now;

			if (!set) {
				// Set to zero pin
				zeroSet();
			} else {
				// Take the shortest route to where we want to be
				if (currPin < moveTo) {
					currPin++;
					stepper.forward();
				}

				if (currPin > moveTo) {
					currPin--;
					stepper.back();
				}
			}
		}

		// Switch between 'hands' every 7000us - seems like the slowest time with no flicker
		if (now - lastMove >= 7000) {
			lastMove = now;
			pinIdx = (pinIdx + 1) % numPins;
			moveTo = pins[pinIdx];
		}
	}

	/**
	 * Set the pins that should be lit
	 */
	void setPins(byte *pins) {
		for (int i=0; i<numPins; i++) {
			this->pins[i] = pins[i];
		}
	}
};

/**
 * Uses SetPins to implement a clock.
 */
struct Clock {
	Stepper stepper;
	PinSet pinSet;

	unsigned long lastSec;
	unsigned long seconds;

	Clock() : pinSet(3, stepper), lastSec(0), seconds(0) {}

	/**
	 * Set the time in seconds. It should be the number of seconds since midnight,
	 * or it can be a real number of seconds if you have a RTC
	 */
	void set(unsigned long secs) {
		seconds = secs;
		lastSec = secs;
	}

	/**
	 * Should be called periodically with the current number of microseconds
	 */
	void periodic(unsigned long now) {
		pinSet.periodic(now);

		// Use a smaller number to make it count faster!
		if (now - lastSec >= 1000000) {
			lastSec = now;
			seconds++;
			byte pins[3];

			pins[2] = (seconds % 60) / 2;	// Location of seconds hand
			pins[1] = ((seconds / 60) % 60) / 2; // Location of minutes hand
			pins[0] = ((seconds / 60 / 12) % 60) / 2; // Location of hours hand

			pinSet.setPins(pins);
		}
	}
};

Clock clock;

// setup() runs once, at reset, to initialize system
void setup() {
	pinMode(GUIDE_1_PIN, OUTPUT);
	pinMode(GUIDE_2_PIN, OUTPUT);
	pinMode(INDEX_PIN, INPUT);
	pinMode(LED_PIN, OUTPUT);

	// Set to 08:20. Because why not?
	clock.set(8L * 60L * 60L + 20L * 60L);
}

void loop() {
	unsigned long now = micros();

	clock.periodic(now);
}
