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
#include "Networking.h"
#include "OfflineLogging.h"
#include "debugging.h"
#include <cmath>
#include "mbed.h"
#include <stdio.h>
#include <errno.h>

#include "BlockDevice.h"

// This will take the system's default block device
BlockDevice *bd = BlockDevice::get_default_instance();

#include "FATFileSystem.h"
FATFileSystem fs("sd");

using namespace std;  

// Aaron's notes
// Control flow for detecting if you can connect to the webserver or not.
// * first detect if the network is there, if yes, connect, if no, then don't connect
//      * There: connect -> set connected to WiFi to true
//      * Not There: don't connect, set not connected to false
// * Next, do all your port reading
//  * if you are not connected to WiFi, then check if you can connect, and connect
//   * if connecting succeeds, then try to send data, but don't try if you did not connect
//  
// * 

int main()
{
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
    }

    Serial pc(USBTX, USBRX);  // Serial Communication
    Serial esp(PTC17, PTC16); // tx, rx (Wifi)
    // wifi pins (PTC17(tx),PTC16(rx))

    // data is gathered from these ports/sensor pins
    AnalogIn Port[] = {PTB2,  PTB3, PTB10, PTB11, PTC11,
                       PTC10, PTC2, PTC0,  PTC9,  PTC8 };

    pc.baud(115200);
    esp.baud(115200);

    BoardSpecs Specs = readSDCard("/sd/IAC_Config_File.txt");
    PRINTSTRING(Specs.DatabaseTableName);
    
    bool OfflineMode = false; // indicates whether to actually send data or not
    bool ServerConnection = false;
    // flags that determine connection status
    bool ConnectedToWiFi = false;

    // if there is no database tableName, or it is all spaces, then exit
    if (Specs.DatabaseTableName == "" || Specs.DatabaseTableName == " "){
        printf("\r\n No Database Table Name Specified, Entering offline mode\r\n");
        OfflineMode = true;
    }
    
    // TODO, just do logging if the WiFi chip is not present.
    else if (setESPMode(esp) != NETWORK_SUCCESS){
        printf("\r\nWifi Chip is not responding, Entering offline mode\r\n");
        OfflineMode = true;
    }

    

    /// FIXME: the below logic does not work consistently and should be simplefied where possible
    
    if (!OfflineMode){ // only send data if the wifi chip can

        ConnectedToWiFi = connectToWiFi(pc, esp, Specs); // try to connect

        /// backup data needs to be sent first, because if it is not sent first, then
        /// the data will be out of order in the database, and that will be bad.
        while (checkForBackupFile(BackupFileName) && ConnectedToWiFi ) {
            printf("\r\n Sending logged data to the database\r\n");
                // send the backup data to the database
            ServerConnection = sendBackupDataTCP(pc, esp, Specs, BackupFileName);
                
            if (!ServerConnection){
                printf("\r\n Data was not fully send to the webserver \r\n");
                
                // update wifi connectivity
                ConnectedToWiFi = checkWiFiConnection(esp, Specs); 
                
                break; // exit if the Server Connection fails
            
            } else {
                printf("\r\n Data was successfully sent to the database \r\n");
            }
        }
    }

    // get the number of ports for the loop
    const size_t NumPorts = Specs.Ports.size();
    
    while (true) {

        // iterate through all the ports
        for (size_t i = 0; i < NumPorts; ++i) {

            // only reads the port if a port is connected
            if (Specs.Ports[i].Multiplier != 0.0f) {

                // read the port
                Specs.Ports[i].Value = Port[i].read() * Specs.Ports[i].Multiplier;
                
                // set error indicator if the sample is out of range
                if (Specs.Ports[i].Value > Specs.Ports[i].RangeEnd){
                    Specs.Ports[i].Value = HUGE_VAL;
                    printf(
                    "\r\nPort value exceeded valid sample value range, assigning error value\r\n");
                } 
                else if(Specs.Ports[i].Value < Specs.Ports[i].RangeStart){
                    Specs.Ports[i].Value = -HUGE_VAL;
                    printf(
                    "\r\nPort value is under the valid sample range, assigning error value\r\n");
                }
                // print data
                pc.printf("\r\n%s's value = %f\r\n",Specs.Ports[i].Name.c_str(), Specs.Ports[i].Value);
            }
        }

        // data will be transmitted while this timer is below the PollingInterval
        PollingTimer.start();

        // only try to send data if the wifi chip is working
        if (!OfflineMode){
        
            // check if the server connection is due to a wifi connection issue.
            if (!ConnectedToWiFi){
                ConnectedToWiFi=connectToWiFi(pc, esp, Specs);
            }    
            
            // if the board is connected to the network, send data to the database
            if (ConnectedToWiFi) {

                // send backed up data while waiting for the polling rate to expire
                while( (PollingTimer.read() <= PollingInterval) && checkForBackupFile(BackupFileName)) {

                    printf("\r\n Sending backup up data to the database. \r\n");
                    // send the backup data to the database
                    ServerConnection = sendBackupDataTCP(pc, esp, Specs, BackupFileName);
                        
                    if (!ServerConnection){
                        printf ("\r\n Failed to transmit backed up data to the Database \r\n");
                        break; // stop transmitting if data transmission failed.
                        
                    } else { // delete data entry if data was sent
                        deleteDataEntry(Specs, BackupFileName);
                    }
                }
                
                if (ServerConnection) {
                    printf("\r\n Sending the last port reading to the database \r\n");
                    // sends the data for all ports to the remote database
                    ServerConnection = sendBulkDataTCP(pc, esp, Specs);
                } else {
                    dumpSensorDataToFile(Specs,BackupFileName);
                    printf("\r\n Backed up Active Port data\r\n");
                }
                
            } else { // back up data if you are not connected   
                dumpSensorDataToFile(Specs,BackupFileName);
                printf("\r\n Backed up Active Port data\r\n");
            }
            
            if (!ServerConnection){
                printf("\r\n Checking WiFi connectivity, since data was not sent================\r\n");
                ConnectedToWiFi = checkWiFiConnection(esp, Specs); 
            }

        } else { // in offline mode, just dump data to file
            printf("\r\nIn offline mode. Dumping data to file.\r\n");
            dumpSensorDataToFile(Specs,BackupFileName);
        }

        // wait until the Polling rate is up before reading again.
        while (PollingTimer.read() <= PollingInterval) {}

        // Reset Timer
        PollingTimer.stop();
        PollingTimer.reset();
    }
}