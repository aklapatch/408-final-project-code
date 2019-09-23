#ifndef NETWORKING_H
#define NETWORKING_H
/// \file
/// Networking function declarations
#include "BoardConfig.h"
#include "ESP8266Interface.h"
#include "OfflineLogging.h"
#include "Structs.h"
#include "TCPSocket.h"
#include "TLSSocket.h"
#include "SocketAddress.h"
#include "mbed.h"

// C++ headers
#include <cstring>
#include <string>
#include <vector>

/// Arbitrary char array length
#define BUFFLEN 1024

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
/// THE ESP8266Interface MUST BE ALLOCATE/INITALIZED ALREADY. It will not be
/// freed inside this function either.
/// \param wifi The ESP8266 connection to use
/// \param Specs The data structure where the SSID and Password are pulled from
/// \returns The error code from the Mbed api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int connectESPWiFi(ESP8266Interface *wifi, BoardSpecs &Specs);

/// return true if you are connected, and false if you are not
/// \param wifi The ESP8266 instance to use
bool checkESPWiFiConnection(ESP8266Interface *wifi);

/// Opens a socket using wifi and sends message to a remote host with
/// TLS \param wifi The ESP instance to use \param message The data to send
/// \returns And error code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendMessageTLS(TLSSocket * sock, string &message, string &response);

/// Tries to send data from the current port readings in Specs to the remote
/// database param wifi The ESP8266 instance to use. This uses TLS encryption
/// and is intended for https connections
/// and database table name are pulled from here \returns And error code from
/// the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi the ESP8266 instance to use
/// \param Specs The port readings that are sent are pulled from here
int sendBulkDataTLS(TLSSocket * sock, BoardSpecs &Specs,
                    string &response);

/// Tries to send backup data to the database. This uses TLS and is intended for
/// https connections.
// \returns An error code from
/// the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi the ESP8266 instance to use
/// \param Specs the number of port samples is pulled from the number of ports
/// that Specs has
/// \param FileName The file from which to pull port readings
int sendBackupDataTLS(TLSSocket * sock, BoardSpecs &Specs,
                      const char *FileName, string &response);

/// Opens a socket using wifi and sends message to a remote host
/// (without TLS)
/// send
/// \returns And error code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi The ESP8266 instance to use.
/// \param message The plaintext message to send
int sendMessageTCP(TCPSocket * sock, string &message, string &response);

/// Tries to send data from the current port readings in Specs to the remote
/// database without TLS. This will work for http connections
/// \param wifi The ESP8266 instance to use
/// \param Specs The set of port readings and database table name are pulled
/// from here \returns And error code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendBulkDataTCP(TCPSocket * sock, BoardSpecs &Specs, string & response);

/// tries to send backup data to the database without TLS. This will work for
/// http connections
/// \param wifi The ESP8266 instance to use
/// \param Specs provides the dabase table name and board name
/// \param FileName the file to pull data readings from
/// \returns An error code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendBackupDataTCP(TCPSocket * sock, BoardSpecs &Specs,
                      const char *FileName, string & response);


/// The socked passed into this function MUST ALREADY BE SETUP. It must be allocated, the certificate must be set, and it must be initialized with a device/netowrk interface
int sendMessageToServer(ESP8266Interface *wifi, Socket * sock, BoardSpecs &Specs, string &message,
                   string &response); 
#endif
