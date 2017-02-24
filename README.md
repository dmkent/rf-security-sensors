# RF receive sketch for old security sensors

This sketch is intended to be hooked up to the RF receiver that was part
of an old Zeus security system. The system had two wireless PIRs and two
wireless door sensors.

I have not been able to find much online about the system. The most detail
is [this manual (in French)](http://www.selectronic.fr/media/pdf/119926.pdf) which
is of limited use. My system is no longer operational and looking at the board
a blown capacitor explains why...

TODO: add some photos of board

So, I've desoldered the RF receiver sub-board and now have it hooked up
to an Adafruit HUZZAH ESP8266 breakout board (via a voltage divider to
step logic down to 3V).

TODO: circuit diagram

After much trial and error with various iterations I've managed to work out
enough of the signal encoding to identify signals from the door sensors.

A transmission appears to be a simple on-off-keying (OOK) 8-byte word along
with a synchronisation signal.

The various signals appear to be (as consecutive pulse lengths in microseconds):

* Sync start: 17000, 1300
* Sync (repeated 11 times): 360, 435
* Sync end: 360, 3890
* Resync start: 720, 14775
* Bit zero: 730, 430
* Bit one: 360, 805
* End: >200000

A transmission then looks something like:

TODO: flow diagram

1. Sync start
2. 11 sync pairs
3. Sync end
4. 64 bits data (128 pulses)
5. 1 parity bit (2 pulses)

The signal will then either terminate with a long pulse or:

1. Resync start
2. 11 sync pairs
3. Sync end
4. 64 bits data (128 pulses)
5. 1 parity bit (2 pulses)

Then repeat as needed.

Next thing is to hook up the decoded data to be sent via MQTT to Home Assistant.
