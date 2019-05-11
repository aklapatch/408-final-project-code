# IAC Project FRDM K64F code
This is the code for our energy project from the IAC (Indiana University's Industrial Audit Commission).

## Building
You need to install the mbed cli to be able to compile and run this embedded program. Documentation for the mbed cli can be found here: [https://os.mbed.com/docs/mbed-os/v5.12/tools/installation-and-setup.html](https://os.mbed.com/docs/mbed-os/v5.12/tools/installation-and-setup.html)

The mbed CLI prefers ARM_GCC 6.x.x. This compiler version will be used: [download link](https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-win32.zip?revision=d8809bf7-a431-49ee-98d5-0475d839f8f1?product=GNU%20Arm%20Embedded%20Toolchain,ZIP,,Windows,6-2017-q2-update).

You also need to have `git` and `python` installed. Python 3.7.3 is used here and the git version is probably not as important.

You can import this repository by using the CLI command: `mbed import https://github.iu.edu/aklapatc/IAC-Project`

That will automatically grab the project and basically `git clone` it right in your current directory.

When you have your environment setup and every tool is added to your path, you should be able to run `mbed compile -m k64f -t GCC_ARM --flash` with a FRDM K64F connected, and the program should compile and be flashed to the K64F.

### Useful docs:
+ [ESP8266 interface code + docs](https://os.mbed.com/teams/ESP8266/code/esp8266-driver/)