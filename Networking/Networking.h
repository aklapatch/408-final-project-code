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
#include "ESP8266Interface.h"
#include "TCPSocket.h"
#include "TLSSocket.h"

// C++ headers
#include <string>
#include <vector>
#include <cstring>

/// Arbitrary char array length
#define BUFFLEN 1024

/// The string with the IP, port, and AT command used to init the connection
#define CONNECTSTRING ("AT+CIPSTART=4,\"TCP\",\"149.165.231.70\",8804\r\n")


/// The macro that indicates the error value where no network errors occured
#define NETWORK_SUCCESS 0

using namespace std;

// for the API rework
// Here are the steps for sending data
// 1. init wifi interface
// 2. connect to wifi
// 3. set up a socket for that wifi interface
//      * for tls, set root cert
// 4. connect the socket to tehe remote host and port
// 5. send data to the remote host
// 6. receive response
// 7. close socket
// 8. disconnect from wifi
//
// 1. should be done in main
//
// 2. should be done in a function (to get wifi settings)
//
// 3. can be done in main
//
// 4-7 should be done in one function
//
// 8 does not need to be done 
//
// get_ip_address == NULL when you are not connected, so we can check that
//
/// Uses the SSID and Password stored in Specs to connect to that network
/// Returns the mbed error code that you get when trying to connect
/// THE ESP8266Interface MUST BE ALLOCATE/INITALIZED ALREADY
/// \param wifi the ESP8266 connection to use
/// \param Specs The data structure where the SSID and Password pulled from
/// \returns The error code from the Mbed api https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int connectESPWiFi(ESP8266Interface * wifi, BoardSpecs & Specs);


/// return true if you are connected, and false if you are not connected
bool checkESPWiFiConnection(ESP8266Interface * wifi);

/// tries to send data tr from Specs
int sendBulkDataTLS(ESP8266Interface * wifi, BoardSpecs &Specs) ;

/// tries to send backup data to the database
int sendBackupDataTLS(ESP8266Interface * wifi,  BoardSpecs & Specs, const char * FileName);

bool checkESPWiFiConnection(ESP8266Interface * wifi);

// The functions below are from the old API ====================================



/// TODO: figure out if this needs to be ported to the ESP8266Interface or dropped 
/// Returns true if NetworkSSID is found in the list of networks that are detected
/// by the Antenna
/// \param Antenna The connection to scan for networks
/// \param NetworkSSID The network SSID to scan for
bool canConnectToNetwork(Serial & Antenna, string NetworkSSID);

/// sets the connection mode of the ESP chip.
/// Also prints the chip specs and version
/// Returns an error code depending on where the application fails
/// Returns a 0 if successful
short setESPMode(Serial & Antenna);

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
