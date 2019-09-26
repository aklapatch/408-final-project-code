#ifndef NETWORKING_H
#define NETWORKING_H
/// \file
/// Networking function declarations
#include "BoardConfig.h"
#include "ESP8266Interface.h"
#include "OfflineLogging.h"
#include "SocketAddress.h"
#include "Structs.h"
#include "TCPSocket.h"
#include "TLSSocket.h"
#include "mbed.h"

// C++ headers
#include <cstring>
#include <string>
#include <vector>

/// Arbitrary char array length
#define BUFFLEN 1024

using namespace std;

/// Uses the SSID and Password stored in Specs to connect to that network
/// Returns the mbed error code that you get when trying to connect
/// THE ESP8266Interface MUST BE ALLOCATE/INITALIZED ALREADY. It will not be
/// freed inside this function either.
/// \param wifi The ESP8266 connection to use
/// \param Specs The data structure where the SSID and Password are pulled from
/// \returns The error code from the Mbed api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int connectESPWiFi(ESP8266Interface *wifi, BoardSpecs &Specs);

/// return true if you are connected to a wifi network, and false if you are not
/// \param wifi The ESP8266 instance to use
bool checkESPWiFiConnection(ESP8266Interface *wifi);

/// Opens a socket using wifi and sends message to a remote host with
/// TLS. sock MUST already be allocated and paired with a network interface, and
/// have its ca_cert set too \param wifi The ESP instance to use \param message
/// The data to send \returns And error code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendMessageTLS(TLSSocket *sock, string &message, string &response);

/// Tries to send data from the current port readings in Specs to the remote
/// database param wifi The ESP8266 instance to use. This uses TLS encryption
/// and is intended for https connections. sock MUST already be allocated and
/// paired with a network interface, and have its ca_cert set too and database
/// table name are pulled from here \returns And error code from the Mbed Socket
/// api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi the ESP8266 instance to use
/// \param Specs The port readings that are sent are pulled from here
int sendBulkDataTLS(TLSSocket *sock, BoardSpecs &Specs, string &response);

/// Tries to send backup data to the database. This uses TLS and is intended for
/// https connections.sock MUST already be allocated and paired with a network
/// interface, and have its ca_cert set too
// \returns An error code from
/// the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi the ESP8266 instance to use
/// \param Specs the number of port samples is pulled from the number of ports
/// that Specs has
/// \param FileName The file from which to pull port readings
int sendBackupDataTLS(TLSSocket *sock, BoardSpecs &Specs, const char *FileName,
                      string &response);

/// Opens a socket using wifi and sends message to a remote host
/// (without TLS)
/// send. sock MUST already be allocated and paired with a network interface,
/// and have its ca_cert set too \returns And error code from the Mbed Socket
/// api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
/// \param wifi The ESP8266 instance to use.
/// \param message The plaintext message to send
int sendMessageTCP(TCPSocket *sock, string &message, string &response);

/// Tries to send data from the current port readings in Specs to the remote
/// database without TLS. This will work for http connections. sock MUST already
/// be allocated and paired with a network interface, and have its ca_cert set
/// too \param wifi The ESP8266 instance to use \param Specs The set of port
/// readings and database table name are pulled from here \returns And error
/// code from the Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendBulkDataTCP(TCPSocket *sock, BoardSpecs &Specs, string &response);

/// tries to send backup data to the database without TLS. This will work for
/// http connections. sock MUST already be allocated and paired with a network
/// interface, and have its ca_cert set too \param wifi The ESP8266 instance to
/// use \param Specs provides the dabase table name and board name \param
/// FileName the file to pull data readings from \returns An error code from the
/// Mbed Socket api: see
/// https://github.com/ARMmbed/mbed-os/blob/master/features/netsocket/nsapi_types.h#L37
int sendBackupDataTCP(TCPSocket *sock, BoardSpecs &Specs, const char *FileName,
                      string &response);

#endif
