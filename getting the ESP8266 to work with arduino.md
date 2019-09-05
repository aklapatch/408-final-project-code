# PIN layout
- esp8266 -> Arduino
- VCC -> 3.3v
- REST (reset) -> push button to GND (not shorted unless the button is pressed)
- RX -> RX (ch 0)
- TX -> TX (ch 0)
- GND -> GND
- CH_PD -> 3.3v

# windows settings
device manager Serial port (ch340) baud rate is set to 9600, and the arduino one is set to 115200

# arduino pins/settings
Res (reset pin) is grounded so the arduino acts as a passthrough

Arduino is flashed with a blank firware.

# ESP8266 chip stats
26Mhz crystal freq
8Mbit (1Mbyte) memory chip

## extra flashing instructions
- ground the gpio0 pin 
- pulse the rst pin with gnd to get it into that mode
-  Use the README to find the flashing addresses
- SET SPI MODE TO DOUT, not QIO
- you NEED to disconnect gpio0 before flashing.

to get the flash locations for each of the .bin files.
You need to use the 1.6.2 version for the 8Mb chip since it does not have the 1024x1024 map, it needs to use the .1024 files in the 512x512 folder.


##  Happenings
Tried flashing (had to get a bigger power supply for the arduino (5v 3a) and it seems to work

you can see the boot log by using 74880 buad, so use that to monitor boot modes

Boot mode should be 1,6 in the boot log to flash.
Use a button to ground gpio0.

Try flashing immediately after resetting, and without any other program using the serial port. I have had better luck that way, but in reality it is just finicky
maybe because of connections or power. Just keep trying.

