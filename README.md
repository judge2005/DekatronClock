# Dekatron Clock
Implements an analog clock using an arduino shield at https://threeneurons.wordpress.com/arduino/dekatron-shield-kit/. It assumes you are using an A101 dekatron.

Video at https://youtu.be/NQM1M31fNoU. It is not as flickery in real life!

To control the time and the animations, you will need a rotary encoder with a built-in switch. I use the [pec11r-4015f-s0024](https://www.digikey.com/product-detail/en/bourns-inc/PEC11R-4015F-S0024/PEC11R-4015F-S0024-ND/4499668). Others may or may not work with the code. The suggested filter circuit on the datasheet doesn't work for me. Use [this one](https://ccrma.stanford.edu/workshops/mid2005/sensors/encoder.html) instead. Use a pull-down resistor on the switch. The code assumes it is high when closed.

The clock will also use a DS3231 real-time clock if one is present. This is available from [Adafruit as a module](https://www.adafruit.com/product/3013).

The code relies on the Adafruit [RTCLib library](https://github.com/adafruit/RTClib). If you don't want to install that library, and you aren't going to use the DS3231 RTC module, you can just comment out the first line of the .ino file.
