#ifndef NETWORKING_H
#define NETWORKING_H
/// \file
/// \brief Networking function declarations
#include "ATCmdParser.h"
#include "BoardConfig.h"
#include "OfflineLogging.h"
#include "SocketAddress.h"
#include "Structs.h"
#include "UARTSerial.h"
#include "mbed.h"

// C++ headers
#include <cstring>
#include <string>
#include <vector>

/// Arbitrary char array length
#define BUFFLEN 1024

/// a constant value that is returned from some functions upon a successful
// network operations
#define NETWORKSUCCESS (0)

using namespace std;
/// starts the ESP8266 with the correct settings:
/// CIPMUX=1 and CWMODE=3
/// returns NETWORKSUCCESS if successful, -1 otherwise.
int startESP(ATCmdParser *_parser);

/// Uses the SSID and Password stored in Specs to connect to that network
/// returns NETWORKSUCCESS if successful, and a negative integer otherwise
int connectESPWiFi(ATCmdParser *_parser, BoardSpecs &Specs);

/// return true if you are connected to a wifi network, and false if you are not
bool checkESPWiFiConnection(ATCmdParser *_parser);

/// makes a get request string to send the port readings from Ports to the
/// remote database in Specs
string makeGetReqStr(vector<PortInfo> Ports, BoardSpecs &Specs);

/// makes a get request string to send the Port samples in Specs to the remote
/// database specified in Specs
string makeGetReqStr(BoardSpecs &Specs);

/// Sends message over TCP to the destination specified in Specs
/// response is the new sampling interval for the board that you get
/// back from the server.
int sendMessageTCP(ATCmdParser *_parser, BoardSpecs &Specs, string &message,
                   float &response);

/// sends a GET request with the most recent port readings to the remote
/// location specified in Specs. response is the new sampling interval for the
/// board that you get back from the server.
int sendBulkDataTCP(ATCmdParser *_parser, BoardSpecs &Specs, float &response);

/// grabs port readings from FileName and
/// sends a GET request with those readings to the remote location specified in
/// Specs. response is the new sampling interval for the board that you get back
/// from the server.
int sendBackupDataTCP(ATCmdParser *_parser, BoardSpecs &Specs,
                      const char *FileName, float &response);
#endif
