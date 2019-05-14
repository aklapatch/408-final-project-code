# IAC Project FRDM K64F code
This is the code for our energy project from the IAC (Indiana University's Industrial Audit Commission).

## Building
You need to install the mbed cli to be able to compile and run this embedded program. Documentation for the mbed cli can be found here: [https://os.mbed.com/docs/mbed-os/v5.12/tools/installation-and-setup.html](https://os.mbed.com/docs/mbed-os/v5.12/tools/installation-and-setup.html)

The mbed CLI prefers ARM_GCC 6.x.x. This compiler version will be used: [download link](https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-win32.zip?revision=d8809bf7-a431-49ee-98d5-0475d839f8f1?product=GNU%20Arm%20Embedded%20Toolchain,ZIP,,Windows,6-2017-q2-update).

You also need to have `git` and `python` installed. Python 3.7.3 is used here and the git version is probably not as important.

You can import this repository by using the CLI command: `mbed import https://github.iu.edu/aklapatc/IAC-Project`

That will automatically grab the project and basically `git clone` it right in your current directory.

When you have your environment setup and every tool is added to your path, you should be able to run `mbed compile -m k64f -t GCC_ARM --flash` with a FRDM K64F connected, and the program should compile and be flashed to the K64F.

The ESP8266 chip needs to have firmware of at least v2 to work. Upgrade instructions are [here](https://os.mbed.com/users/sarahmarshy/notebook/esp8266-v2-firmware-update-/).

The above paragraph but with Arduino instructions [here](https://www.electronicshub.org/update-flash-esp8266-firmware/).

Make sure that the baud rate in Device manager for the Arduino's serial port is 115200, and that you pulse the RST pin with 3.3 V every time you ground the GPIO/IOG pin to switch to and from firmware upload mode. Also make sure that no other device is using that Serial Port either (like the Arduino Serial Monitor).

Here are the pin connections that I used for the arduino to update the ESP8266 firmware.

| Arduino | ESP8266 |
|---------|---------|
| 3.3V | 3V3 , EN |
| GND | IO0 (GPIO), GND,  RST (through push button) |
| RX (pin 0) | RX |
| TX (pin 1) | TX |

This is how the ESP8266 should be connected to function (theoretically):
| SensorBoard | ESP8266 |
|----------|-----------|
| Red Wires (3.3V) | EN, 3V3 |
| Green | RX |
| Yellow | TX |
| Black | GND |

### Useful docs:
+ [ESP8266 interface code + docs](https://os.mbed.com/teams/ESP8266/code/esp8266-driver/)