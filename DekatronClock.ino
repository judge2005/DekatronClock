#include <Arduino.h>

#define LED_PIN 13         // Test LED
#define GUIDE_1_PIN 5       // Guide 1 - G1 pin of 2-guide Dekatron
#define GUIDE_2_PIN 6       // Guide 2 - G2 pin of 2-guide Dekatron
#define INDEX_PIN 7       // INDEX_PIN   - NDX input pin. High when glow at K0

#define pinButton 10
#define encoderA 8
#define encoderB 9

struct Button {
	byte buttonPin;

	int lastButtonValue = 0;
	unsigned long changeTime = 0;

	Button(byte pin) : buttonPin(pin) {
	}

	boolean state() {
		int buttonValue = digitalRead(buttonPin);

		unsigned long now = millis();

		// Has to stay at that value for 50ms
		if (buttonValue != lastButtonValue) {
			if (now - changeTime > 50)
				lastButtonValue = buttonValue;

			changeTime = now;
		}

		return lastButtonValue;
	}

	boolean wasPressed = false;

	boolean clicked() {
		boolean nowPressed = state();

		if (!wasPressed && nowPressed) {
			wasPressed = true;
			return true;
		}

		wasPressed = nowPressed;

		return false;
	}
};

struct RotaryEncoder {
	byte pinA;
	byte pinB;
	int lastModeA;
	int lastModeB;
	int curModeA;
	int curModeB;
	int curPos;
	int lastPos;

	RotaryEncoder(byte aPin, byte bPin) :
		pinA(aPin),
		pinB(bPin),
		lastModeA(LOW),
		lastModeB(LOW),
		curModeA(LOW),
		curModeB(LOW),
		curPos(0),
		lastPos(0)
	{
	}

	int getRotation() {
		// read the current state of the current encoder's pins
		curModeA = digitalRead(pinA);
		curModeB = digitalRead(pinB);

		if ((lastModeA == LOW) && (curModeA == HIGH)) {
			if (curModeB == LOW) {
				curPos--;
			} else {
				curPos++;
			}
		}
		lastModeA = curModeA;
		int rotation = curPos - lastPos;
		lastPos = curPos;

		return rotation;
	}
};

int BinarySearch (byte a[], int low, int high, int key)
{
    int mid;

    if (low == high)
        return low;

    mid = low + ((high - low) / 2);

    if (key > a[mid])
        return BinarySearch (a, mid + 1, high, key);
    else if (key < a[mid])
        return BinarySearch (a, low, mid, key);

    return mid;
}

void BinaryInsertionSort (byte a[], unsigned int b[], int n)
{
    int ins, i;
    byte tmpA;
    unsigned int tmpB;

    for (i = 1; i < n; i++) {
        ins = BinarySearch (a, 0, i, a[i]);
        if (ins < i) {
            tmpB = b[i];
            memmove (b + ins + 1, b + ins, sizeof (unsigned int) * (i - ins));
            b[ins] = tmpB;

            tmpA = a[i];
            memmove (a + ins + 1, a + ins, sizeof (byte) * (i - ins));
            a[ins] = tmpA;
        }
    }
}

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
		cli();
		if (cathode == 0)	// Main cathode
				{
			PORTD &= B10011111;
		}
		if (cathode == 1)	// Guide 1 cathode
				{
			PORTD |= B00100000;
			PORTD &= B10111111;
		}
		if (cathode == 2)	// Guide 2 cathode
				{
			PORTD &= B11011111;
			PORTD |= B01000000;
		}
		sei();
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
	unsigned int *lingers;
	int numPins;
	int maxPins;
	int pinIdx;
	byte currPin;
	byte moveTo;
	boolean set;
	Stepper &stepper;

	unsigned long lastTick;
	unsigned long lastMove;

	PinSet(int maxPins, Stepper &stepper) :
		pins(new byte[maxPins]),
		lingers(new unsigned int[maxPins]),
		numPins(0),
		maxPins(maxPins),
		pinIdx(0),
		currPin(0),
		moveTo(0),
		set(false),
		stepper(stepper),
		lastTick(0),
		lastMove(0)
	{
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
			} else if (currPin != moveTo) {
#define CLOCKWISE

#ifdef SHORTEST
				int delta = moveTo - currPin;
				char step = delta > 0 ? 1 : -1;
				if (abs(delta) > 15) {
					step = -step;
				}

				if (step < 0) {
					stepper.back();
				}

				if (step > 0) {
					stepper.forward();
				}

				currPin = (currPin + step + 30) % 30;
#endif

#ifdef CLOCKWISE
				// Always go clockwise
				stepper.forward();

				currPin = (currPin + 1) % 30;
#endif
			}
		}

		// Switch between 'hands' every 7000us - seems like the slowest time with no flicker
		if (now - lastMove >= lingers[pinIdx]) {
			lastMove = now;
			pinIdx = (pinIdx + 1) % numPins;
			moveTo = pins[pinIdx];
		}
	}

	/**
	 * Set the pins that should be lit and how bright they should be
	 */
	void setData(int numPins, byte *pins, unsigned int *lingers) {
		this->numPins = numPins;

		memcpy(this->lingers, lingers, sizeof (unsigned int) * numPins);
		memcpy(this->pins, pins, sizeof (byte) * numPins);

		BinaryInsertionSort(this->pins, this->lingers, numPins);
	}

	/**
	 * Set how long each pin will be lit
	 */
	void setLingers(unsigned int *lingers) {
		for (int i=0; i<numPins; i++) {
			this->lingers[i] = lingers[i];
		}
	}
};

/**
 * Uses SetPins to implement a clock, with animations
 */
struct AnimatedClock {
	Stepper stepper;
	PinSet pinSet;

	enum Mode {
		plain,
		trainBoth,
		trainClockwise,
		trainAnticlockwise,
		pulseBoth,
		pulseClockwise,
		pulseAnticlockwise,
		SIZE
	};

	unsigned long lastSec;
	unsigned long seconds;
	unsigned long lastPulse;
	byte inc;
	Mode mode;
	unsigned int lingers[9] = {
			14000,
			3500,
			3500,

			1500,
			1500,
			1500,
			1500,
			1500,
			1500
	};

	AnimatedClock() :
		pinSet(3 + 6, stepper),
		lastSec(0),
		seconds(0),
		lastPulse(0),
		inc(0),
		mode(plain)
	{
		pinSet.setLingers(lingers);
	}

	/**
	 * Set the time in seconds. It should be the number of seconds since midnight,
	 * or it can be a real number of seconds if you have a RTC
	 */
	void set(unsigned long secs) {
		seconds = secs;
		lastSec = secs;
	}

	void incrementMinutes() {
		seconds += 60;
		lastSec = seconds;
	}

	void decrementMinutes() {
		seconds -= 60;
		lastSec = seconds;
	}

	void incrementMode() {
		mode = (Mode)((((int)mode)+1) % Mode::SIZE);
	}

	/**
	 * Should be called periodically with the current number of microseconds
	 */
	void periodic(unsigned long now) {
		pinSet.periodic(now);

		if (now - lastPulse >= 100000) {
			int numPins = 3;
			lastPulse = now;

			if (now - lastSec >= 1000000) {
				lastSec = now;
				seconds++;
			}

			byte pins[3 + 6];

			pins[0] = ((seconds / 60 / 12) % 60) / 2; // Location of hours hand
			pins[1] = (seconds % 60) / 2;	// Location of seconds hand
			pins[2] = ((seconds / 60) % 60) / 2; // Location of minutes hand

			if (mode != plain) {
				if (mode < pulseBoth) {
					byte origin = pins[2];
					byte destination = pins[0];

					char clockwiseDistance = destination - origin;
					if (clockwiseDistance < 0) {
						clockwiseDistance = 30 + clockwiseDistance;
					}

					for (int i=0; i<6; i++) {
						byte distance = i * 5 + 1 + inc;
						if (distance <= clockwiseDistance) {
							if (mode == trainBoth || mode == trainClockwise) {
								pins[numPins++] = (origin + distance) % 30;
							}
						} else {
							if (mode == trainBoth || mode == trainAnticlockwise) {
								pins[numPins++] = (30 + origin - (5 - i) * 5 - 1 - inc) % 30;
							}
						}
					}

					inc = (inc + 1) % 5;
				} else {
					byte origin = pins[2];
					byte destination = pins[0];

					char clockwiseDistance = destination - origin;
					if (clockwiseDistance < 0) {
						clockwiseDistance = 30 + clockwiseDistance;
					}

					char antiClockwiseDistance = origin - destination;
					if (antiClockwiseDistance < 0) {
						antiClockwiseDistance = 30 + antiClockwiseDistance;
					}

					byte distance = inc + 1;
					if (distance <= clockwiseDistance) {
						if (mode == pulseBoth || mode == pulseClockwise) {
							pins[numPins++] = (origin + distance) % 30;
						}
					}

					if (distance <= antiClockwiseDistance){
						if (mode == pulseBoth || mode == pulseAnticlockwise) {
							pins[numPins++] = (30 + origin - distance) % 30;
						}
					}

					inc = (inc + 1) % 30;
				}
			}

			pinSet.setData(numPins, pins, lingers);
		}
	}
};

AnimatedClock clock;
Button button(pinButton);
RotaryEncoder encoder(encoderA, encoderB);

// setup() runs once, at reset, to initialize system
void setup() {
	pinMode(GUIDE_1_PIN, OUTPUT);
	pinMode(GUIDE_2_PIN, OUTPUT);
	pinMode(INDEX_PIN, INPUT);
	pinMode(LED_PIN, OUTPUT);

	pinMode(pinButton, INPUT_PULLUP);
	pinMode(encoderA, INPUT_PULLUP);
	pinMode(encoderB, INPUT_PULLUP);

	// Set to 08:20. Because why not?
	clock.set(8L * 60L * 60L + 20L * 60L);
}

void loop() {
	unsigned long now = micros();

	clock.periodic(now);

	if (button.clicked()) {
		clock.incrementMode();
	}

	int rotation = encoder.getRotation();
	if (rotation > 0) {
		clock.incrementMinutes();
	}

	if (rotation < 0) {
		clock.decrementMinutes();
	}
}
