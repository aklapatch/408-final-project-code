# IAC Project FRDM K64F code
This is the code for our energy project from the IAC (Indiana University's Industrial Audit Commission).

## Notes
To upgrade the mbed-os version, download a new release from their [github](https://github.com/ARMmbed/mbed-os/releases) and then replace the `mbed-os` folder in this folder with  the mbed folder you downloaded.
Make sure that you rename the downloaded folder to `mbed-os`. Also make sure that there is not extra directory level in the mbed-os folder before you get into the source code.

## Building
You also need to have `git` and `python` installed. Python 3.7.4 is used here and the git version is probably not as important.

You need to install the mbed cli to be able to compile and run this embedded program. I installed the windows version of python 3.7.4, and used the command `pip install mbed-cli` to get the cli on my system. You also need to cd into the `mbed-os` directory and run `pip install -r requirements.txt` to get everything you need.

We will be using the GNU ARM embedded toolchain [6-2017-q1-update](https://developer.arm.com/-/media/Files/downloads/gnu-rm/6_1-2017q1/gcc-arm-none-eabi-6-2017-q1-update-win32-zip.zip?revision=f49d2798-8261-4951-b93b-9a1a354e5aca?product=GNU%20Arm%20Embedded%20Toolchain,ZIP,,Windows,6-2017-q1-update). You will need to add that to your PATH (windows) for this to work. 

You can import this repository by using the CLI command: `mbed import REPO-URL`

That will automatically grab the project and basically `git clone` it right in your current directory.

And you might have to fiddle with different versions of python packages. (try using pip to get the newest, that worked for me at one point)

When you have your environment setup and every tool is added to your path, you should be able to run `mbed compile -m k64f -t GCC_ARM --flash` with a FRDM K64F connected, and the program should compile and be flashed to the K64F.

The ESP8266 chip may need firmware of at least v2 to work. There are some instructions/tips in the `getting the ESP8266 to work with the arduino.md` file, but you are on your own as far as that goes. 

Some Arduino instructions for flashing [here](https://www.electronicshub.org/update-flash-esp8266-firmware/).

### Useful docs:
+ [ESP8266 interface code + docs](https://os.mbed.com/teams/ESP8266/code/esp8266-driver/)
