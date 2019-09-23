#include "Networking.h"

#include "debugging.h"
/// \file
/// Implementation for all network functions

const char *value_get_str = "&Value[]=";

/// The string that preceeds the port ID field
const char *port_get_str = "&Port_ID[]=";

const char *id_get_str = "Board_ID=";

const char *getstr = "GET ";

int connectESPWiFi(ESP8266Interface *wifi, BoardSpecs &Specs) {
    int wifi_err = wifi->connect(Specs.NetworkSSID.c_str(),
                         Specs.NetworkPassword.c_str());
        
    if (wifi_err == NSAPI_ERROR_IS_CONNECTED)
        return NSAPI_ERROR_OK;

   return wifi_err; 
}

// =============================================================================
string makeGetReqStr(BoardSpecs &Specs) {
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(getstr) + Specs.RemoteDir.size() +
                          Specs.DatabaseTableName.size() + 1;
    // the 1 is for the '?'

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Specs.Ports[i].Multiplier != 0) {
            message_size += Specs.Ports[i].Name.size() +
                            to_string(Specs.Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen(id_get_str) + strlen(getstr) + strlen("\r\n");

    string Message = getstr;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

    Message.append(Specs.RemoteDir);

    Message.append("?");

    Message.append(id_get_str);

    Message.append(Specs.DatabaseTableName);

    // append to get request for every active port
    for (size_t i = 0; i < End; ++i) {
        if (Specs.Ports[i].Multiplier != 0) {
            Message.append(port_get_str);
            Message.append(Specs.Ports[i].Name);
            Message.append(value_get_str);
            Message.append(to_string(Specs.Ports[i].Value));
        }
    }
    Message.append("\r\n");

    return Message;
}
// ===============================================================================

string makeGetReqStr(vector<PortInfo> Ports, BoardSpecs &Specs) {
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(getstr) + Specs.RemoteDir.size() +
                          Specs.DatabaseTableName.size() + 1;
    // the 1 is for the '?'

    size_t End = Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0) {
            message_size += Ports[i].Name.size() +
                            to_string(Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen(id_get_str) + strlen(getstr) + strlen("\r\n");

    string Message = getstr;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

    Message.append(Specs.RemoteDir);

    Message.append("?");

    Message.append(id_get_str);

    Message.append(Specs.DatabaseTableName);

    // append to get request for every active port
    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0) {
            Message.append(port_get_str);
            Message.append(Ports[i].Name);
            Message.append(value_get_str);
            Message.append(to_string(Ports[i].Value));
        }
    }
    Message.append("\r\n");

    return Message;
}
//==============================================================================

// return true if you are connected, and false if you are not connected
bool checkESPWiFiConnection(ESP8266Interface *wifi) {
    return wifi->get_ip_address() != NULL;
}
// =============================================================================
int sendMessageTLS(TLSSocket * sock, BoardSpecs &Specs, string &message,
                   string &response) {

    int err = sock->connect(Specs.RemoteIP.c_str(), Specs.RemotePort);

    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    err = sock->send(message.c_str(), message.size());
    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    // get the response
    char buffer[256];
    response.clear();

    while ((err = sock->recv(buffer, 256)) > 0) {
        response.append(buffer);
    }

CLOSEFREE:
    sock->close();
    return err;
}

/// The socked passed into this function MUST ALREADY BE SETUP. It must be allocated, the certificate must be set, and it must be initialized with a device/netowrk interface
int sendMessageToServer(ESP8266Interface *wifi, Socket * sock, BoardSpecs &Specs, string &message,
                   string &response) {

    int err = sock->connect(SocketAddress(Specs.RemoteIP.c_str(), Specs.RemotePort));

    if (err != NSAPI_ERROR_OK)
        goto CLOSE;

    err = sock->send(message.c_str(), message.size());
    if (err != NSAPI_ERROR_OK)
        goto CLOSE;

    // get the response
    char buffer[256];
    response.clear();

    while ((err = sock->recv(buffer, 256)) > 0) {
        response.append(buffer);
    }

CLOSE:
    sock->close();

    return err;
}

// =============================================================================
// sends a vector of port values instead
// gets those port values from the backup file
int sendBackupDataTLS(TLSSocket * sock, BoardSpecs &Specs,
                      const char *FileName, string &response) {

    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);

    string Message = makeGetReqStr(Specs);
    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame
    return sendMessageTLS(sock, Specs, Message, response);
}

// =============================================================================
// wi The socket must be connected to the 8266 chip (opened)
// the cert must be set too
// returns the int result from the mbed API
int sendBulkDataTLS(TLSSocket * sock, BoardSpecs &Specs,
                    string &response) {

    string Message = makeGetReqStr(Specs);
    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTLS(sock, Specs, Message, response);
}
// ============================================================================
int sendMessageTCP(TCPSocket * sock, BoardSpecs &Specs, string &message,
                   string &response) {

    // connect to the web server
    int err = sock->connect(Specs.RemoteIP.c_str(), Specs.RemotePort);

    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    err = sock->send(message.c_str(), message.size());
    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    // get the response
    char buffer[256];
    response.clear();

    while ((err = sock->recv(buffer, 256)) > 0) {
        response.append(buffer);
    }

CLOSEFREE:
    return err;
}

int sendBackupDataTCP(TCPSocket * sock, BoardSpecs &Specs,
                      const char *FileName, string &response) {

    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);

    string Message = makeGetReqStr(Ports, Specs);

    return sendMessageTCP(sock, Specs, Message, response);
}

// =============================================================================
int sendBulkDataTCP(TCPSocket * sock, BoardSpecs &Specs,
                    string &response) {

    string message = makeGetReqStr(Specs);

    return sendMessageTCP(sock, Specs, message, response);
}
