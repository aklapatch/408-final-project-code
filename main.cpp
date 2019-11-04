

#include "BoardConfig.h"
#include "ESP8266.h"
#include "ESP8266Interface.h"
#include "Networking.h"
#include "OfflineLogging.h"
#include "debugging.h"
#include "mbed.h"
#include "mbed_trace.h"
#include <cmath>
#include <errno.h>

#include "BlockDevice.h"

#include "ATCmdParser.h"
#include "UARTSerial.h"

// This will take the system's default block device
BlockDevice *bd = BlockDevice::get_default_instance();

#include "FATFileSystem.h"
FATFileSystem fs("sd");

using namespace std;

/// returns the characters between startchars and endchars. It returns an empty
/// string if it cannot find the start and enc filter sequences
string textBetween(string input, const char *startchars, const char *endchars) {

    // check if the string is big engough to actually filter
    // this will throw and exception if we do not do this
    size_t filter_size = strlen(startchars) + strlen(endchars);
    if (filter_size >= input.size())
        return "";

    size_t start_idx = input.find(startchars);
    size_t end_idx = input.find(endchars);

    // return an empty string if you cannot find the filter text
    if (start_idx != string::npos || end_idx != string::npos)
        return "";

    if (start_idx > end_idx)
        return "";

    start_idx += strlen(startchars);

    return input.substr(start_idx, end_idx - start_idx);
}

const char *filter_start = "samplerate=\"";
const char *filter_end = "\"></span>";

/// extracts the sample rate from response text, converts it to a float, and
/// returns it. This returns -1 if it fails.
float extractSampleRate(string &message_response) {
    string rate_text = textBetween(message_response, filter_start, filter_end);
    if (rate_text == "")
        return -1;
    else {
        if (isdigit(rate_text[0]))

            return stof(rate_text);

        else
            return -1;
    }
}

int main() {

    // interval for the sensor polling
    float PollingInterval = 5.0;

    // name of the file where data is stored
    const char BackupFileName[] = "/sd/PortReadings.dat";

    Timer PollingTimer; // Timer that controlls when polling happens

    // Try to mount the filesystem
    mbed_printf("Mounting the filesystem... ");
    fflush(stdout);
    int err = fs.mount(bd);
    mbed_printf("%s\n", (err ? "Fail :(" : "OK"));
    mbed_printf("\r\n");
    if (err) {
        // Reformat if we can't mount the filesystem
        // this should only happen on the first boot
        mbed_printf("No filesystem found, formatting... ");
        fflush(stdout);
        err = fs.reformat(bd);
        mbed_printf("%s\n", (err ? "Fail :(" : "OK"));
        if (err) {
            error("error: %s (%d)\n", strerror(-err), err);
        }
        mbed_printf(
            "There is not config file since the drive was just formatted\r\n");
        mbed_printf("Exiting\r\n");
        return -1;
    }

    // data is gathered from these ports/sensor pins
    AnalogIn Port[] = {PTB2,  PTB3, PTB10, PTB11, PTC11,
                       PTC10, PTC2, PTC0,  PTC9,  PTC8};
    PRINTLINE;
    const char *config_file = "/sd/IAC_Config_File.txt";

    bool OfflineMode = false; // indicates whether to actually send data or not
    bool ServerConnection = false;

    string response(""); // a response from tcp/tls connections
    PRINTLINE;
    UARTSerial *_serial = new UARTSerial(PTC17, PTC16, 115200);
    ATCmdParser *_parser = new ATCmdParser(_serial);

    _parser->debug_on(1);
    _parser->set_delimiter("\r\n");
    _parser->set_timeout(5000);

    mbed_printf("\r\nReading board settings from %s\r\n", config_file);
    BoardSpecs Specs = readSDCard("/sd/IAC_Config_File.txt");
    wait(4);

    // if (!checkESPWiFiConnection(_parser))
    if (startESP(_parser) != NETWORKSUCCESS) {

        mbed_printf(
            "\r\n ESP Chip was not initialized, entering offline mode\r\n");
        OfflineMode = true;
    }

    if (!checkESPWiFiConnection(_parser))
        connectESPWiFi(_parser, Specs);

    mbed_printf("%s\r\n", response.c_str());

    // if there is no database tableName, or it is all spaces, then exit
    if (Specs.DatabaseTableName == "" || Specs.DatabaseTableName == " ") {
        mbed_printf(
            "\r\n No Database Table Name Specified, Entering offline mode\r\n");
        OfflineMode = true;
    }

    // there needs to be a remote ip and remote directory specified too
    if (Specs.RemoteDir == " " || Specs.RemoteDir == "") {
        OfflineMode = true;

        mbed_printf(
            "\r\n No Remote directory specified, Entering offline mode\r\n");
    }

    if (Specs.RemoteIP == " " || Specs.RemoteIP == "") {
        OfflineMode = true;

        mbed_printf(
            "\r\n No Remote IP address specified, Entering offline mode\r\n");
    }

    if (Specs.RemotePort == 0) {
        OfflineMode = true;

        mbed_printf("\r\n No Remote port specified, Entering offline mode\r\n");
    }

    if (Specs.HostName == "" || Specs.HostName == " ") {
        OfflineMode = true;

        mbed_printf("\r\n No Remote Hostname found, Entering offline mode\r\n");
    }

    int wifi_err = NETWORKSUCCESS;
    if (!OfflineMode) {
        if (!checkESPWiFiConnection(_parser)) {
            mbed_printf("trying to connect to %s\r\n",
                        Specs.NetworkSSID.c_str());
            wifi_err = connectESPWiFi(_parser, Specs);
        }

        if (wifi_err != NETWORKSUCCESS) {
            mbed_printf("\r\n failed to connect to %s. Error code = %d \r\n",
                        Specs.NetworkSSID.c_str(), wifi_err);
        } else {
            mbed_printf(" connected to %s\r\n", Specs.NetworkSSID.c_str());
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
                if (Specs.Ports[i].Value > Specs.Ports[i].RangeEnd) {
                    Specs.Ports[i].Value = HUGE_VAL;
                    mbed_printf(
                        "\r\nPort value exceeded valid sample value range, "
                        "assigning "
                        "error value\r\n");
                } else if (Specs.Ports[i].Value < Specs.Ports[i].RangeStart) {
                    Specs.Ports[i].Value = -HUGE_VAL;
                    mbed_printf(
                        "\r\nPort value is under the valid sample range, "
                        "assigning "
                        "error value\r\n");
                }
                // print data
                mbed_printf("\r\n%s's value = %f\r\n",
                            Specs.Ports[i].Name.c_str(), Specs.Ports[i].Value);
            }
        }

        // data will be transmitted while this timer is below the
        // PollingInterval
        PollingTimer.start();

        // only try to send data if the wifi chip is working
        if (!OfflineMode) {

            // try to connect to wifi again if you are not connected now
            if (!checkESPWiFiConnection(_parser)) {

                mbed_printf("Trying to connect to %s \r\n",
                            Specs.NetworkSSID.c_str());
                wifi_err = connectESPWiFi(_parser, Specs);

                if (wifi_err != NETWORKSUCCESS) {
                    mbed_printf("Connection attempt failed error = %d\r\n",
                                wifi_err);
                } else {
                    mbed_printf("Connected to %s \r\n",
                                Specs.NetworkSSID.c_str());
                }
            }

            // if the board is connected to the network, send data to the
            // database
            if (checkESPWiFiConnection(_parser)) {

                // send backed up data while waiting for the polling rate to
                // expire
                while ((PollingTimer.read() <= PollingInterval) &&
                       checkForBackupFile(BackupFileName)) {

                    mbed_printf(
                        "\r\n Sending backed up data to the database. \r\n");
                    PRINTLINE;
                    wifi_err = sendBackupDataTCP(_parser, _serial, Specs,
                                                 BackupFileName, response);
                    float tmp = extractSampleRate(response);

                    if (tmp != -1 && tmp > 0) {
                        PollingInterval = tmp;
                        mbed_printf("Sample interval is now %f\r\n",
                                    PollingInterval);
                    }

                    if (wifi_err != NETWORKSUCCESS) {
                        mbed_printf(
                            "\r\n Failed to transmit backed up data to the "
                            "Database \r\n");
                        ServerConnection = false;
                        mbed_printf("Error code = %d\r\n", wifi_err);
                        break; // stop transmitting if data transmission failed.

                    } else { // delete data entry if data was sent
                        deleteDataEntry(Specs, BackupFileName);
                    }
                }

                if (ServerConnection) {
                    mbed_printf(
                        "\r\n Sending the last port reading to the database "
                        "\r\n");

                    wifi_err =
                        sendBulkDataTCP(_parser, _serial, Specs, response);

                    if (wifi_err != NETWORKSUCCESS) {

                        ServerConnection = false;
                        mbed_printf(
                            "Could not send data to database, error = %d\r\n",
                            wifi_err);
                    }
                } else {
                    dumpSensorDataToFile(Specs, BackupFileName);
                }

            } else { // back up data if you are not connected
                dumpSensorDataToFile(Specs, BackupFileName);
                mbed_printf("\r\n Backed up Active Port data\r\n");
            }

        } else { // in offline mode, just dump data to file
            mbed_printf("\r\nIn offline mode. Dumping data to file.\r\n");
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
 * Here is how some of the code is organized:
 * - Networking.cpp / Networking.h -> functions related to networking
 * - BoardConfig.cpp / BoardConfig.h -> functions for getting, and holding the
 *   configuration for the board
 * - Structs.h -> structs that contain configuration items
 * - OfflineLogging.cpp / OfflineLogging.h -> functions that relate to logging
 *   and deleting data to and from a file
 * - debugging.h -> Macros that are meant to assist in debugging
 */
