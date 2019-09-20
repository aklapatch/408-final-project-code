#include "Networking.h"
#include "NetworkConstants.h"
#include "debugging.h"
/// \file
/// Implementation for all network functions

int connectESPWiFi(ESP8266Interface *wifi, BoardSpecs &Specs) {
    return wifi->connect(Specs.NetworkSSID.c_str(),
                         Specs.NetworkPassword.c_str());
}

//==============================================================================

// return true if you are connected, and false if you are not connected
bool checkESPWiFiConnection(ESP8266Interface *wifi) {
    return wifi->get_ip_address() != NULL;
}
// =============================================================================
int sendMessageTLS(ESP8266Interface *wifi, string &message, string &response) {

    // connect to the web server
    TLSSocket *sock = new TLSSocket();

    int err = sock->set_root_ca_cert(ca_cert);
    if (err != NSAPI_ERROR_OK)
        goto FREE;

    err = sock->open(wifi);
    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    err = sock->connect(web_server_ip, web_server_port);

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
FREE:
    delete sock;
    return err;
}

// =============================================================================
// sends a vector of port values instead
// gets those port values from the backup file
int sendBackupDataTLS(ESP8266Interface *wifi, BoardSpecs &Specs,
                      const char *FileName, string &response) {

    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);

    // make the message to send
    // get the size to allocate memory
    size_t message_size =
        strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0) {
            message_size += Ports[i].Name.size() +
                            to_string(Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");

    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

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

    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTLS(wifi, Message, response);
}

// =============================================================================
// wi The socket must be connected to the 8266 chip (opened)
// the cert must be set too
// returns the int result from the mbed API
int sendBulkDataTLS(ESP8266Interface *wifi, BoardSpecs &Specs, string & response) {

    // make the message to send
    // get the size to allocate memory
    size_t message_size =
        strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Specs.Ports[i].Multiplier != 0) {
            message_size += Specs.Ports[i].Name.size() +
                            to_string(Specs.Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");

    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

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

    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTLS(wifi, Message, response);
}
// ============================================================================
int sendMessageTCP(ESP8266Interface *wifi, string &message, string &response) {

    // connect to the web server
    TCPSocket *sock = new TCPSocket();

    int err = -1;

    if (!sock)
        goto FREE;

    err = sock->open(wifi);
    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    err = sock->connect(web_server_ip, web_server_port);

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
FREE:
    delete sock;
    return err;
}

int sendBackupDataTCP(ESP8266Interface *wifi, BoardSpecs &Specs,
                      const char *FileName, string &response) {

    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);

    // make the message to send
    // get the size to allocate memory
    size_t message_size =
        strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0) {
            message_size += Ports[i].Name.size() +
                            to_string(Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");

    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

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

    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTCP(wifi, Message, response);
}

// =============================================================================
int sendBulkDataTCP(ESP8266Interface *wifi, BoardSpecs &Specs, string &response) {

    // make the message to send
    // get the size to allocate memory
    size_t message_size =
        strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Specs.Ports[i].Multiplier != 0) {
            message_size += Specs.Ports[i].Name.size() +
                            to_string(Specs.Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");

    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

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

    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTCP(wifi, Message, response);
}
