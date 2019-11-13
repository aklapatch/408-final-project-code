#include "Networking.h"

#include "debugging.h"
/// \file
/// \brief Implementation for all network functions

const char *value_get_str = "&Value[]=";

/// The string that preceeds the port ID field
const char *port_get_str = "&Port_ID[]=";

const char *id_get_str = "Board_ID=";

const char *get_req_start = "GET ";

/// required for the `Host` HTTP header
const char *req_header = "Host: ";

const char *get_req_end = "\r\n";

const int response_size = 256;

int startESP(ATCmdParser *_parser) {
    _parser->send("AT+CIPCLOSE=5");
    _parser->recv("OK");
    _parser->send("AT+CWMODE=3");
    _parser->recv("OK");
    _parser->send("AT+CIPMUX=1");
    if (_parser->recv("OK"))
        return NETWORKSUCCESS;
    else
        return -1;
}

int connectESPWiFi(ATCmdParser *_parser, BoardSpecs &Specs) {

    _parser->send("AT+CWJAP=\"%s\",\"%s\"", Specs.NetworkSSID.c_str(),
                  Specs.NetworkPassword.c_str());

    if (_parser->recv("OK")) {
        if (checkESPWiFiConnection(_parser)) {
            return NETWORKSUCCESS;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}

// =============================================================================
string makeGetReqStr(BoardSpecs &Specs) {
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(get_req_start) + Specs.RemoteDir.size() +
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
    message_size +=
        strlen(id_get_str) + strlen(get_req_start) + strlen(get_req_end);
    message_size += strlen(req_header) + Specs.HostName.size() + strlen("\r\n");

    string Message = get_req_start;

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
    Message.append(req_header);
    Message.append(Specs.HostName);
    Message.append(get_req_end);

    return Message;
}
// ===========================================================================

string makeGetReqStr(vector<PortInfo> Ports, BoardSpecs &Specs) {
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(get_req_start) + Specs.RemoteDir.size() +
                          Specs.DatabaseTableName.size() + 1;
    // the 1 is for the '?'

    size_t End = Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);

    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0.0f) {
            message_size += Ports[i].Name.size() +
                            to_string(Ports[i].Value).size() + get_extras;
        }
    }
    // add on for the \r\n
    message_size +=
        strlen(id_get_str) + strlen(get_req_start) + strlen(get_req_end);

    message_size += strlen(req_header) + Specs.HostName.size() + strlen("\r\n");

    string Message = get_req_start;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

    Message.append(Specs.RemoteDir);

    Message.append("?");

    Message.append(id_get_str);

    Message.append(Specs.DatabaseTableName);

    // append to get request for every active port
    for (size_t i = 0; i < End; ++i) {
        if (Ports[i].Multiplier != 0.0f) {
            Message.append(port_get_str);
            Message.append(Ports[i].Name);
            Message.append(value_get_str);
            Message.append(to_string(Ports[i].Value));
        }
    }
    Message.append("\r\n");
    Message.append(req_header);
    Message.append(Specs.HostName);
    Message.append(get_req_end);

    return Message;
}
//==============================================================================

// return true if you are connected, and false if you are not connected
bool checkESPWiFiConnection(ATCmdParser *_parser) {
    _parser->debug_on(0);
    // 000.000.000.000 max of 15 characters
    char ip_addr[16];
    _parser->send("AT+CIFSR");

    _parser->recv("+CIFSR:STAIP,\"%15[^\"]\"", ip_addr);

    ip_addr[15] = 0;

    if (!_parser->recv("OK"))
        return false;

    // if that expression is true, then 0.0.0.0 is not in the ip address, and we
    // ar connected

    _parser->debug_on(1);
    return strstr(ip_addr, "0.0.0.0") == NULL;
}
// ============================================================================
int sendMessageTCP(ATCmdParser *_parser, BoardSpecs &Specs, string &message,
                   float &response) {

    _parser->send("AT+CIPSTART=0,\"TCP\",\"%s\",%d", Specs.RemoteIP.c_str(),
                  Specs.RemotePort);
    if (!_parser->recv("OK")) {
        _parser->send("AT+CIPCLOSE=5");
        return -1;
    }

    _parser->send("AT+CIPSEND=0,%d", message.size());

    if (!_parser->recv(">"))
        return -3;

    if (!_parser->send("%s", message.c_str()))
        return -4;

    if (!_parser->recv("+IPD"))
        return -5;

    char Buf[response_size + 1];
    _parser->read(Buf, response_size);
    Buf[response_size] = 0;
    printf("Response: %s\r\n", Buf);
    if (strstr(Buf, "404"))
        return -6;

    // get polling rate
    const char *tok = "samplerate=\"";
    char *ratestart = strstr(Buf, tok);
    // go right up to the float value
    ratestart += strlen(tok);

    if (isdigit(ratestart[0])) {
        response = atof(ratestart);
    }

    _parser->send("AT+CIPCLOSE=5");
    _parser->recv("OK");

    return NETWORKSUCCESS;
}

int sendBackupDataTCP(ATCmdParser *_parser, BoardSpecs &Specs,
                      const char *FileName, float &response) {
    printf("Sending backup data over the network \r\n");
    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);
    string Message = makeGetReqStr(Ports, Specs);
    return sendMessageTCP(_parser, Specs, Message, response);
}

// =============================================================================
int sendBulkDataTCP(ATCmdParser *_parser, BoardSpecs &Specs, float &response) {

    string message = makeGetReqStr(Specs);

    return sendMessageTCP(_parser, Specs, message, response);
}
