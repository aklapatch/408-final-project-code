/// \file
/// \brief Contains the logic and control flow for the entire program.

#include "BoardConfig.h"
#include "Networking.h"
#include "OfflineLogging.h"
#include "debugging.h"
#include "mbed.h"
#include <cmath>

#include "BlockDevice.h"

#include "ATCmdParser.h"
#include "UARTSerial.h"

// This will take the system's default block device
BlockDevice *bd = BlockDevice::get_default_instance();

#include "FATFileSystem.h"
FATFileSystem fs("sd");

using namespace std;

/// The watchdog timer goes off after PollingInterval*WATCHDOGCOEFF seconds
#define WATCHDOGCOEFF (5)

// for the watchdog timer, we will have a timeout that goes off
// and resets the program. This function will be detached and reattached
// throughout the life of the program to keep from resetting all the time
/// Resets timeout by detaching first, and then attaching the NVIC_SystemReset()
/// function to the timeout with the new_delay
void resetWatchdog(Timeout &timeout, float new_delay) {
    timeout.detach();
    timeout.attach(&NVIC_SystemReset, new_delay);
}

int main() {

    // interval for the sensor polling
    float PollingInterval = 5.0f;

    Timeout watchdog;

    // name of the file where data is stored
    const char BackupFileName[] = "/sd/PortReadings.dat";

    Timer PollingTimer; // Timer that controlls when polling happens

    // Try to mount the filesystem
    printf("Mounting the filesystem... ");
    fflush(stdout);
    int err = fs.mount(bd);
    printf("%s\n", (err ? "Fail :(" : "OK"));
    printf("\r\n");
    if (err) {
        // Reformat if we can't mount the filesystem
        // this should only happen on the first boot
        printf("No filesystem found, formatting... ");
        fflush(stdout);
        err = fs.reformat(bd);
        printf("%s\n", (err ? "Fail :(" : "OK"));
        if (err) {
            error("error: %s (%d)\n", strerror(-err), err);
        }
        printf(
            "There is not config file since the drive was just formatted\r\n");
        printf("Exiting\r\n");
        return -1;
    }

    // data is gathered from these ports/sensor pins
    AnalogIn Port[] = {PTB2,  PTB3, PTB10, PTB11, PTC11,
                       PTC10, PTC2, PTC0,  PTC9,  PTC8};
    const char *config_file = "/sd/IAC_Config_File.txt";

    bool OfflineMode = false; // indicates whether to actually send data or not

    /// how many times to try connecting to the wifi before giving up
    int wifi_tries = WIFITRIES;

    UARTSerial *_serial = new UARTSerial(PTC17, PTC16, 115200);
    ATCmdParser *_parser = new ATCmdParser(_serial);

    _parser->debug_on(1);
    _parser->set_delimiter("\r\n");
    _parser->set_timeout(3000);

    printf("\r\nReading board settings from %s\r\n", config_file);
    BoardSpecs Specs = readSDCard("/sd/IAC_Config_File.txt");
    // wait_us() is not deprecated, but wait() is
    wait_us(1000000);

    // if (!checkESPWiFiConnection(_parser))
    if (startESP(_parser) != NETWORKSUCCESS) {

        printf("\r\n ESP Chip was not initialized, entering offline mode\r\n");
        OfflineMode = true;
    }

    // if there is no database tableName, or it is all spaces, then exit
    if (Specs.DatabaseTableName == "" || Specs.DatabaseTableName == " ") {
        printf(
            "\r\n No Database Table Name Specified, Entering offline mode\r\n");
        OfflineMode = true;
    }

    // there needs to be a remote ip and remote directory specified too
    if (Specs.RemoteDir == " " || Specs.RemoteDir == "") {
        OfflineMode = true;

        printf("\r\n No Remote directory specified, Entering offline mode\r\n");
    }

    if (Specs.RemoteIP == " " || Specs.RemoteIP == "") {
        OfflineMode = true;

        printf(
            "\r\n No Remote IP address specified, Entering offline mode\r\n");
    }

    if (Specs.RemotePort == 0) {
        OfflineMode = true;

        printf("\r\n No Remote port specified, Entering offline mode\r\n");
    }

    if (Specs.HostName == "" || Specs.HostName == " ") {
        OfflineMode = true;

        printf("\r\n No Remote Hostname found, Entering offline mode\r\n");
    }

    int wifi_err = NETWORKSUCCESS;
    if (!OfflineMode) {
        if (!checkESPWiFiConnection(_parser)) {
            printf("trying to connect to %s\r\n", Specs.NetworkSSID.c_str());
            wifi_err = connectESPWiFi(_parser, Specs);
        }

        if (wifi_err != NETWORKSUCCESS) {
            printf("\r\n failed to connect to %s. Error code = %d \r\n",
                   Specs.NetworkSSID.c_str(), wifi_err);
            wifi_tries -= 1;
        } else {
            printf(" connected to %s\r\n", Specs.NetworkSSID.c_str());
        }
    }

    // get the number of ports for the loop
    const size_t NumPorts = Specs.Ports.size();

    while (true) {

        // Read all of the ports
        for (size_t i = 0; i < NumPorts; ++i) {

            // only reads the port if a port is connected
            if (Specs.Ports[i].Multiplier != 0.0f) {

                // read the port
                Specs.Ports[i].Value =
                    Port[i].read() * Specs.Ports[i].Multiplier;

                // set error indicator if the sample is out of range
                if (Specs.Ports[i].Value > Specs.Ports[i].RangeCeiling) {
                    Specs.Ports[i].Value = HUGE_VAL;
                    printf("\r\nPort value exceeded valid sample value range, "
                           "assigning "
                           "error value\r\n");
                } else if (Specs.Ports[i].Value < Specs.Ports[i].RangeFloor) {
                    Specs.Ports[i].Value = -HUGE_VAL;
                    printf("\r\nPort value is under the valid sample range, "
                           "assigning "
                           "error value\r\n");
                }
                // print data
                printf("\r\n%s's value = %f\r\n", Specs.Ports[i].Name.c_str(),
                       Specs.Ports[i].Value);

                resetWatchdog(watchdog, PollingInterval * WATCHDOGCOEFF);
            }
        }

        // data will be transmitted while this timer is below the
        // PollingInterval
        PollingTimer.start();

        // only try to send data if the wifi chip is working
        if (!OfflineMode) {

            // try to connect to wifi again if you are not connected now
            if (!checkESPWiFiConnection(_parser)) {

                printf("Trying to connect to %s \r\n",
                       Specs.NetworkSSID.c_str());
                wifi_err = connectESPWiFi(_parser, Specs);

                if (wifi_err != NETWORKSUCCESS) {
                    printf("Connection attempt failed error = %d\r\n",
                           wifi_err);

                    wifi_tries -= 1;
                    if (wifi_tries <= 0) {

                        printf("Wifi connection failed %d times, activating "
                               "offline mode\r\n",
                               WIFITRIES);
                        OfflineMode = true;
                    }

                } else {
                    printf("Connected to %s \r\n", Specs.NetworkSSID.c_str());
                    wifi_tries = WIFITRIES;
                }
            }

            // if the board is connected to the network, send data to the
            // database
            if (checkESPWiFiConnection(_parser)) {

                // send backed up data while waiting for the polling rate to
                // expire
                while ((PollingTimer.read() <= PollingInterval) &&
                       checkForBackupFile(BackupFileName)) {

                    printf("\r\n Sending backed up data to the database. \r\n");
                    float tmp = -1.0f;
                    wifi_err =
                        sendBackupDataTCP(_parser, Specs, BackupFileName, tmp);

                    if (tmp != -1.0f && tmp > 0.0f) {
                        PollingInterval = tmp;
                        printf("Sample interval is now %f\r\n",
                               PollingInterval);
                    }

                    if (wifi_err != NETWORKSUCCESS) {
                        printf("\r\n Failed to transmit backed up data to the "
                               "Database \r\n");
                        printf("Error code = %d\r\n", wifi_err);
                        break; // stop transmitting if data transmission failed.

                    } else { // delete data entry if data was sent
                        deleteDataEntry(Specs, BackupFileName);
                    }
                }

                if (!checkForBackupFile(BackupFileName)) {
                    printf("\r\n Sending the last port reading to the database "
                           "\r\n");
                    float tmp = -1;
                    wifi_err = sendBulkDataTCP(_parser, Specs, tmp);

                    if (tmp != -1.0f && tmp > 0.0f) {
                        PollingInterval = tmp;
                        printf("Sample interval is now %f\r\n",
                               PollingInterval);
                    }
                    if (wifi_err != NETWORKSUCCESS) {

                        printf(
                            "Could not send data to database, error = %d\r\n",

                            wifi_err);

                        dumpSensorDataToFile(Specs, BackupFileName);
                    }
                } else {
                    dumpSensorDataToFile(Specs, BackupFileName);
                }

            } else { // back up data if you are not connected
                dumpSensorDataToFile(Specs, BackupFileName);
                printf("\r\n Backed up Active Port data\r\n");
            }

        } else { // in offline mode, just dump data to file
            printf("\r\nIn offline mode. Dumping data to file.\r\n");
            dumpSensorDataToFile(Specs, BackupFileName);
        }

        // wait until the Polling rate is up before reading again.
        while (PollingTimer.read() <= PollingInterval) {
        }

        // Reset Timer
        PollingTimer.stop();
        PollingTimer.reset();
    }
}
/**
 * \mainpage IAC Energy Monitoring project
 *
 * Introduction
 * ------------
 * This project works on an embedded platfrom to monitor energy usage.
 * This usage is collected through sensors attached to the board.
 * Then it is uploaded to a database where the data can be processed later.
 *
 * If you have any questions, please email me at klapatchaaron at gmail dot com
 *
 * Here is how some of the code is organized:
 * - main.cpp -> Well, it's where everything starts.
 * - Networking.cpp / Networking.h -> functions related to networking
 * - BoardConfig.cpp / BoardConfig.h -> functions for getting, and holding the
 *   configuration for the board
 * - Structs.h -> structs that contain configuration items
 * - OfflineLogging.cpp / OfflineLogging.h -> functions that relate to logging
 *   and deleting data to and from a file
 * - debugging.h -> Macros that are meant to assist in debugging
 *
 * 
 * Configuration File Documentation
 * --------------------------------
 * There are 4 different fields that need to be filled out for the board configuration.
 * - `BoardInfo`
 * - `ConnInfo`
 * - `Sensor`
 * - `Port`
 *
 * This is an example of filling out the `BoardInfo` field:
 * ```
 * BoardInfo:WiFi SSID,WiFi Password,Board Name
 * ```
 * To fill out a field, put the field name at the very beginning of the line, put a colon after it, and then put text after it, with subfields separated by commas.
 * 
 * This is an example of a board configuration:
 * ```
 * BoardInfo:HomeWiFi,password123,Test Board
 *
 * ConnInfo:192.168.0.3,80,test-server.com,/sensor-readings.php
 *
 * Sensor:Voltage,Volts,10,-20,20
 * Sensor:Voltage,Volts,1000,-50,50
 *
 * Port:Voltage Port,0
 * Port:Different Voltage Port,1
 * ```
 * 
 * In that configuration, there are 2 sensor types, 2 active ports.
 * All of the following descriptions are related to the previous example.
 * ### BoardInfo
 * The board will try to connect to a wifi SSID `HomeWiFi` with the password `password123` and the board's name is `Test Board`.
 *
 * ### ConnInfo
 * The board will try to connect to `192.168.0.3` at port `80`, with the hostname `test-server.com`, and try push a GET request to the file `sensor-readings.php` at the site root.
 *
 * ### Sensor
 * The first sensor has an id of 0, is measuring voltage, has volts as a unit, has a multiplier of 10, and has a valid range is from -20 to 20.
 *
 * The second sensor has an id of 1, is measuring voltage, has volts as a unit, has a multiplier of 1000, and has a valid range is from -50 to 50.
 *
 * The `Voltage` and `Volts` text in this case is not important. The sensor descriptions are based on that text, but no sensor readings are derived from those labels
 *
 * Sensor ID number are automatically assigned. The topmost sensor in the file has the id of 0, and the next one down has an ID of 1, and the next one has an ID of 2, and so on.
 *
 * ### Port
 * The first port has the name `Voltage Port` and inherits the multipliers and valid range of its sensor id. Its sensor ID is 0 in this case.
 *
 * The second port has the name `Different Voltage Port` and inherits the multipliers and valid range of its sensor id. Its sensor ID is 1 in this case.
 */
