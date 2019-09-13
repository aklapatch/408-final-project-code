//===-- main.cpp - main driver definition ---------------------------------===//
//
// Part of the IAC energy monitoring project
//
//===----------------------------------------------------------------------===//
/// \file
/// Has the main() function.
//===----------------------------------------------------------------------===//

/**
 * \mainpage IAC Energy Monitoring project
 *
 * Introduction
 * ------------
 * This project works on an embedded platfrom to monitor energy usage.
 * This usage is collected through sensors attached to the board.
 * Then it is uploaded to a database where the data can be processed later.
 */

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
#include <stdio.h>

#include "BlockDevice.h"

// This will take the system's default block device
BlockDevice *bd = BlockDevice::get_default_instance();

#include "FATFileSystem.h"
FATFileSystem fs("sd");

using namespace std;

// Aaron's notes
// Control flow for detecting if you can connect to the webserver or not.
// * first detect if the network is there, if yes, connect, if no, then don't
// connect
//      * There: connect -> set connected to WiFi to true
//      * Not There: don't connect, set not connected to false
// * Next, do all your port reading
//  * if you are not connected to WiFi, then check if you can connect, and
//  connect
//   * if connecting succeeds, then try to send data, but don't try if you did
//   not connect
//

int main() {
    mbed_trace_init();

    // interval for the sensor polling
    float PollingInterval = 5.0;

    // name of the file where data is stored
    const char BackupFileName[] = "/sd/BackupFile.dat";

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
    PRINTLINE;

    const char *config_file = "/sd/IAC_Config_File.txt";

    printf("\r\nReading board settings from %s\r\n", config_file);
    BoardSpecs Specs = readSDCard("/sd/IAC_Config_File.txt");
    PRINTSTRING(Specs.DatabaseTableName);

    bool OfflineMode = false; // indicates whether to actually send data or not
    bool ServerConnection = false;
    // flags that determine connection status
    bool ConnectedToWiFi = false;

    ESP8266Interface *wifi = new ESP8266Interface(PTC17, PTC16);
    if (!wifi) {
        printf("\r\n ESP Chip was not initialized, entering offline mode\r\n");
        OfflineMode = true;
    }

    // if there is no database tableName, or it is all spaces, then exit
    if (Specs.DatabaseTableName == "" || Specs.DatabaseTableName == " ") {
        printf(
            "\r\n No Database Table Name Specified, Entering offline mode\r\n");
        OfflineMode = true;
    }

    int wifi_err = 0;
    if (!OfflineMode) {
        printf("trying to connect to %s\r\n", Specs.NetworkSSID.c_str());
        wifi_err = connectESPWiFi(wifi, Specs);
        if (wifi_err != NSAPI_ERROR_OK) {
            printf("\r\n failed to connect to %s. Error code = %d \r\n",
                   Specs.NetworkSSID.c_str(), wifi_err);
            ConnectedToWiFi = false;
        } else {
            ConnectedToWiFi = true;
            printf(" connected to %s\r\n", Specs.NetworkSSID.c_str());
        }
    }

    /// FIXME: the below logic does not work consistently and should be
    /// simplefied where possible

    if (!OfflineMode &&
        ConnectedToWiFi) { // only send data if the wifi chip can

        /// backup data needs to be sent first, because if it is not sent first,
        /// then the data will be out of order in the database, and that will be
        /// bad.
        while (checkForBackupFile(BackupFileName) && ConnectedToWiFi) {
            printf("\r\n Sending logged data to the database\r\n");
            // send the backup data to the database
            wifi_err = sendBackupDataTLS(wifi, Specs, BackupFileName);

            if (wifi_err != NSAPI_ERROR_OK) {
                printf("\r\n Data was not fully send to the webserver \r\n");

                // update wifi connectivity
                ConnectedToWiFi = checkESPWiFiConnection(wifi);

                break; // exit if the Server Connection fails

            } else {
                printf("\r\n Data was successfully sent to the database \r\n");
                // delete the sent entry
                // this function will delete the file when
                // it is empty
                deleteDataEntry(Specs, BackupFileName);
            }
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
                    printf("\r\nPort value exceeded valid sample value range, "
                           "assigning "
                           "error value\r\n");
                } else if (Specs.Ports[i].Value < Specs.Ports[i].RangeStart) {
                    Specs.Ports[i].Value = -HUGE_VAL;
                    printf("\r\nPort value is under the valid sample range, "
                           "assigning "
                           "error value\r\n");
                }
                // print data
                printf("\r\n%s's value = %f\r\n", Specs.Ports[i].Name.c_str(),
                       Specs.Ports[i].Value);
            }
        }

        // data will be transmitted while this timer is below the
        // PollingInterval
        PollingTimer.start();

        // only try to send data if the wifi chip is working
        if (!OfflineMode) {

            // try to connect to wifi again if you are not connected now
            if (!ConnectedToWiFi) {

                printf("Trying to connect to %s \r\n",
                       Specs.NetworkSSID.c_str());
                wifi_err = connectESPWiFi(wifi, Specs);

                if (wifi_err != NSAPI_ERROR_OK) {
                    printf("Connection attempt failed error = %d\r\n",
                           wifi_err);
                    ConnectedToWiFi = false;
                } else {
                    printf("Connected to %s \r\n", Specs.NetworkSSID.c_str());
                    ConnectedToWiFi = true;
                }
            }

            // if the board is connected to the network, send data to the
            // database
            if (ConnectedToWiFi) {

                // send backed up data while waiting for the polling rate to
                // expire
                while ((PollingTimer.read() <= PollingInterval) &&
                       checkForBackupFile(BackupFileName)) {

                    printf("\r\n Sending backup up data to the database. \r\n");
                    // send the backup data to the database
                    wifi_err = sendBackupDataTLS(wifi, Specs, BackupFileName);

                    if (wifi_err != NSAPI_ERROR_OK) {
                        printf("\r\n Failed to transmit backed up data to the "
                               "Database \r\n");
                        ServerConnection = false;
                        printf("Error code = %d\r\n", wifi_err);
                        break; // stop transmitting if data transmission failed.

                    } else { // delete data entry if data was sent
                        deleteDataEntry(Specs, BackupFileName);
                    }
                }

                if (ServerConnection) {
                    printf("\r\n Sending the last port reading to the database "
                           "\r\n");
                    // sends the data for all ports to the remote database
                    wifi_err = sendBulkDataTLS(wifi, Specs);
                    if (wifi_err != NSAPI_ERROR_OK) {

                        ServerConnection = false;
                        printf(
                            "Could not send data to database, error = %d\r\n",
                            wifi_err);
                    }
                } else {
                    dumpSensorDataToFile(Specs, BackupFileName);
                    printf("\r\n Backed up Active Port data\r\n");
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
