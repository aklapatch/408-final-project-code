# updating the boot loader and firmward for the K64F
This [page](https://os.mbed.com/blog/entry/DAPLink-bootloader-update/) is helpful.

1. Download the DAPLink Bootloader image from the previously mentioned helpful page.
2. hold down the reset button on your K64F board while pluggging it inot a computer usb port (make sure you are connectiing the OpenSDA2 port on the board to the computer)

If you are on Windows, disable the storage service (services.msc) to prevent asynchronous data transfers

3. copy the bootload .bin file over to the drive labeled: BOOTLOADER
4. after the copy is done, unplug the K64F and plug it in again, but without holding down the reset button
5. A new drive should come up called MAINTENANCE.
6. Download the DAPLink firmware from [here](https://github.com/ARMmbed/DAPLink/releases). I used v0246 when I last did this. (and it worked)
7. copy the v0246 firmware over the the drive called MAINTENANCE 
8. the MAINTENANCE directory should go away and be replaced by a directory called   DAPLINK . If the name is different, that is fine, as long as a new drive comes back

Remember to enable the Storage Service again if you are on Windows

