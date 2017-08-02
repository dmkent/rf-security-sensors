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
to:

* A Saleae logic analyser and
* (soon) an Arduino Pro Mini compatible

TODO: circuit diagram

This sketch uses the decoder in https://github.com/dmkent/zeus-rf-core to
attempt to decode the signal from the reciever in real-time on the Arduino.
