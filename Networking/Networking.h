#ifndef NETWORKING_H
#define NETWORKING_H
//===-- Networking.h - Networking function prototypes --------------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/// \file
/// Has function protoypes that generally interface with the networking chip
/// to make network connections.

#include "Structs.h"
#include "mbed.h"
#include "BoardConfig.h"
#include "OfflineLogging.h"

// C++ headers
#include <string>
#include <vector>

/// Arbitrary char array length
#define BUFFLEN 1024

/// The string with the IP, port, and AT command used to init the connection
#define CONNECTSTRING ("AT+CIPSTART=4,\"TCP\",\"149.165.231.70\",8804\r\n")

using namespace std;

/// Returns true if NetworkSSID is found in the list of networks that are detected
/// by the Antenna
/// \param Antenna The connection to scan for networks
/// \param NetworkSSID The network SSID to scan for
bool canConnectToNetwork(Serial & Antenna, string NetworkSSID);

/// sets the connection mode of the ESP chip.
/// Also prints the chip specs and version
void setESPMode(Serial & Debug, Serial & Antenna);

/// Returns true if it is connected to a newtork, false if not.
/// It checks the IP address and sees if the IP is 0.0.0.0 or nonsensical.
bool checkWiFiConnection(Serial &Antenna, BoardSpecs & Specs);

/// Attempts to connect to the network specified in the Specs struct.
/// Returns true if the connection succeeds. This function first checks if the 
/// NetworkSSID specified in the Specs parameter is there, and it will only try 
/// to connect if that SSID is there. This function DOES NOT update the internal
/// IP address.
/// \param Specs Has the WiFi SSID and the password of the network to connect to
/// \param Antenna The Serial device used to make the connection
/// \param Debug Used to relay debug information
bool connectToWiFi(Serial &Debug, Serial &Antenna, BoardSpecs &Specs);

/// Sends the message to the specified seraial connection.
/// \param Message The message to send
/// \param Bus The connection used to send the message
void sendCMD(Serial &Bus, char *Message);

/// Sends the Command to a specified Serial connection.
/// \param Bus The string is send along this connection
/// \param Message This message is sent along the specified connection.
void sendCMD(Serial &Bus, string Message);

/// Sends the Command to a specified Serial connection.
/// \param Bus The string is send along this connection
/// \param Message This message is sent along the specified connection.
void sendCMD(Serial &Bus, const char *Message);

/// Gets the reply from the Specified serial chip during the timeout value
/// \param Bus The serial device to listen to
/// \param timeout controls how long to wait for reply (seconds?)
string getReply(Serial &Bus, float timeout);

/// This function has not been tested and should not be used unless you are blessed
/// with inordinate amounts of spare time.
/// Retrieves data buffered in the Serial connection specified with Bus.
/// This should be more efficient than \ref getReply, but has not been tested.
/// \param Bus The serial device to get data from
/// \param StartDelay The time to wait (seconds) before reading data
string getReplyWithBuffer(Serial &Bus, float StartDelay);

/// Uploads the port's reading static location (ip address) on the network.
/// This function also prints debugging information to the Debug connection
/// and checks if certain relays are on or off.
/// \param Debug The Serial connection where debug information is sent
/// \param Antenna The wifi connection that is used to communicate
/// \param PortInfo Has the Port's information to be sent
/// \param TableName The name of the table where the port's information should
bool sendDataTCP(Serial &Debug, Serial &Antenna, PortInfo Port,
                 string &TableName);

/// Uploads the port's readings to the remote database using Antenna
/// This function also prints debugging information to the Debug connection
/// and checks if certain relays are on or off.
/// \param Debug The Serial connection where debug information is sent
/// \param Antenna The wifi connection that is used to communicate
/// \param Specs The data from all sensors and the database table name is
/// pulled from this variables
bool sendBulkDataTCP(Serial &Debug, Serial &Antenna, BoardSpecs &Specs);

/// Uploads the port's readings to the remote database using Antenna
/// This function also prints debugging information to the Debug connection
/// and checks if certain relays are on or off.
/// \param Debug The Serial connection where debug information is sent
/// \param Antenna The wifi connection that is used to communicate
/// \param Specs Pulls the database table name from here
/// \param Ports Gets Port readings from here.
bool sendBulkDataTCP(Serial &Debug, Serial &Antenna, vector<PortInfo> Ports, BoardSpecs & Specs);

/// Grabs the data from the backup file and tries to upload it to the remote database.
/// It calls other functions internally to handle reading and writing the file.
/// This function uses the internal board configuration to associate data with each port.
/// \param Debug The Serial connection where debug information is sent
/// \param Antenna The wifi connection that is used to communicate
/// \param Specs The database table name is pulled from here
/// \param BackupFile The name of the file to pull logged data from
bool sendBackupDataTCP(Serial &Debug, Serial &Antenna, BoardSpecs &Specs,
                       const char * BackupFileName);

/// This function transmits the data to the database.
/// Other functions wrap around this one to send data.
/// \param Debug The Serial connection where debug information is sent
/// \param Antenna The wifi connection that is used to communicate
/// \param RemoteTableName The name for the remote database table.
/// \param Reading These values are sent to the remote database
/// \param PortName The name of that specific port
bool sendFloatToDataBase(Serial &Debug, Serial &Antenna, string RemoteTableName,
                         string PortName, float Reading);
          
/// Scans the passed in string and looks for keywords that indicate an error.
/// Returns true if an error keyword is found, and false if one is not found.
bool checkNetworkError(string Message);

/// Returns true if you can connect to the Server specified by \ref CONNECTSTRING
/// and false if the connection fails.
bool checkServerConnection(Serial & Antenna);

/// return true if OK was found in Message. Successful AT commands often have OK
/// in the reply string, so this function can be used to check for such errors
bool hasOK(string Message);
#endif // NETWORKING