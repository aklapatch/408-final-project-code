#include "Networking.h"

#include "debugging.h"
/// \file
/// Implementation for all network functions

const char *value_get_str = "&Value[]=";

/// The string that preceeds the port ID field
const char *port_get_str = "&Port_ID[]=";

const char *id_get_str = "Board_ID=";

const char *get_req_start = "GET ";

const char *req_header = "Host: ";

const char *get_req_end = "\r\n";

const int response_size = 512;

/// A certificate for TLS communication. This certificate is from:
/// https://github.com/ARMmbed/mbed-os-example-tls-socket/blob/master/main.cpp
const char *ca_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIENjCCAx6gAwIBAgIBATANBgkqhkiG9w0BAQUFADBvMQswCQYDVQQGEwJTRTEU\n"
    "MBIGA1UEChMLQWRkVHJ1c3QgQUIxJjAkBgNVBAsTHUFkZFRydXN0IEV4dGVybmFs\n"
    "IFRUUCBOZXR3b3JrMSIwIAYDVQQDExlBZGRUcnVzdCBFeHRlcm5hbCBDQSBSb290\n"
    "MB4XDTAwMDUzMDEwNDgzOFoXDTIwMDUzMDEwNDgzOFowbzELMAkGA1UEBhMCU0Ux\n"
    "FDASBgNVBAoTC0FkZFRydXN0IEFCMSYwJAYDVQQLEx1BZGRUcnVzdCBFeHRlcm5h\n"
    "bCBUVFAgTmV0d29yazEiMCAGA1UEAxMZQWRkVHJ1c3QgRXh0ZXJuYWwgQ0EgUm9v\n"
    "dDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALf3GjPm8gAELTngTlvt\n"
    "H7xsD821+iO2zt6bETOXpClMfZOfvUq8k+0DGuOPz+VtUFrWlymUWoCwSXrbLpX9\n"
    "uMq/NzgtHj6RQa1wVsfwTz/oMp50ysiQVOnGXw94nZpAPA6sYapeFI+eh6FqUNzX\n"
    "mk6vBbOmcZSccbNQYArHE504B4YCqOmoaSYYkKtMsE8jqzpPhNjfzp/haW+710LX\n"
    "a0Tkx63ubUFfclpxCDezeWWkWaCUN/cALw3CknLa0Dhy2xSoRcRdKn23tNbE7qzN\n"
    "E0S3ySvdQwAl+mG5aWpYIxG3pzOPVnVZ9c0p10a3CitlttNCbxWyuHv77+ldU9U0\n"
    "WicCAwEAAaOB3DCB2TAdBgNVHQ4EFgQUrb2YejS0Jvf6xCZU7wO94CTLVBowCwYD\n"
    "VR0PBAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wgZkGA1UdIwSBkTCBjoAUrb2YejS0\n"
    "Jvf6xCZU7wO94CTLVBqhc6RxMG8xCzAJBgNVBAYTAlNFMRQwEgYDVQQKEwtBZGRU\n"
    "cnVzdCBBQjEmMCQGA1UECxMdQWRkVHJ1c3QgRXh0ZXJuYWwgVFRQIE5ldHdvcmsx\n"
    "IjAgBgNVBAMTGUFkZFRydXN0IEV4dGVybmFsIENBIFJvb3SCAQEwDQYJKoZIhvcN\n"
    "AQEFBQADggEBALCb4IUlwtYj4g+WBpKdQZic2YR5gdkeWxQHIzZlj7DYd7usQWxH\n"
    "YINRsPkyPef89iYTx4AWpb9a/IfPeHmJIZriTAcKhjW88t5RxNKWt9x+Tu5w/Rw5\n"
    "6wwCURQtjr0W4MHfRnXnJK3s9EK0hZNwEGe6nQY1ShjTK3rMUUKhemPR5ruhxSvC\n"
    "Nr4TDea9Y355e6cJDUCrat2PisP29owaQgVR1EX1n6diIWgVIEM8med8vSTYqZEX\n"
    "c4g/VhsxOBi0cQ+azcgOno4uG+GMmIPLHzHxREzGBHNJdmAPx/i9F4BrLunMTA5a\n"
    "mnkPIAou1Z5jJh5VkpTYghdae9C8x49OhgQ=\n"
    "-----END CERTIFICATE-----";

int startESP(ATCmdParser *_parser) {
    _parser->send("AT+CIPCLOSE=5");
    _parser->recv("OK");
    _parser->send("AT+CWMODE=3");
    _parser->recv("OK");
    _parser->send("AT+CIPMUX=1");
    if (_parser->recv("OK"))
        return 0;
    else
        return -1;
}

int connectESPWiFi(ATCmdParser *_parser, BoardSpecs &Specs) {

    _parser->send("AT+CWJAP=\"%s\",\"%s\"", Specs.NetworkSSID.c_str(),
                  Specs.NetworkPassword.c_str());
    return _parser->recv("OK");
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
    // 000.000.000.000 max of 15 characters
    char ip_addr[16];
    _parser->send("AT+CIFSR");

    _parser->recv("+CIFSR:STAIP,\"%15[^\"]\"", ip_addr);

    ip_addr[15] = 0;

    if (!_parser->recv("OK"))
        return false;

    // if that expression is true, then 0.0.0.0 is not in the ip address, and we
    // ar connected

    return strstr(ip_addr, "0.0.0.0") == NULL;
}
// ============================================================================
int sendMessageTCP(ATCmdParser *_parser, UARTSerial *_serial, BoardSpecs &Specs,
                   string &message, float &response) {

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

    char Buf[255];
    _parser->read(Buf, 254);
    Buf[254] = 0;
    mbed_printf("Response: %s\r\n", Buf);
    if (strstr(Buf, "404"))
        return -6;
    
    // get polling rate
    const char *tok = "samplerate=\"";
    char * ratestart = strstr(Buf, tok);
    // go right up to the float value
    ratestart += strlen(tok);

    if (isdigit(ratestart[0])){
        response = atof(ratestart);
    }

    _parser->send("AT+CIPCLOSE=5");
    _parser->recv("OK");

    return NETWORKSUCCESS;
}

int sendBackupDataTCP(ATCmdParser *_parser, UARTSerial *_serial,
                      BoardSpecs &Specs, const char *FileName,
                      float &response) {
    mbed_printf("Sending backup data over the network \r\n");
    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);
    string Message = makeGetReqStr(Ports, Specs);
    return sendMessageTCP(_parser, _serial, Specs, Message, response);
}

// =============================================================================
int sendBulkDataTCP(ATCmdParser *_parser, UARTSerial *_serial,
                    BoardSpecs &Specs, float &response) {

    string message = makeGetReqStr(Specs);

    return sendMessageTCP(_parser, _serial, Specs, message, response);
}

// =============================================================================
int sendMessageTLS(ESP8266Interface *wifi, BoardSpecs &Specs, string &message,
                   string &response) {

    TLSSocket *sock = new TLSSocket();
    int err = sock->open(wifi);
    if (err != NSAPI_ERROR_OK)
        goto CLOSEFREE;

    err = sock->connect(Specs.RemoteIP.c_str(), Specs.RemotePort);

    if (err != NSAPI_ERROR_OK)
        goto CLOSE;

    err = sock->send(message.c_str(), message.size());
    if (err != NSAPI_ERROR_OK)
        goto CLOSE;

    // get the response
    char buffer[256];
    response.clear();
    response.reserve(256);

    while ((err = sock->recv(buffer, 255)) > 0) {
        buffer[255] = '\0';
        mbed_printf("%s", buffer);
    }

    // store the last 255 characters of the response in the string
    response.assign(buffer);

CLOSE:
    sock->close();

CLOSEFREE:
    delete wifi;
    return err;
}

// =============================================================================
// sends a vector of port values instead
// gets those port values from the backup file
int sendBackupDataTLS(ESP8266Interface *wifi, BoardSpecs &Specs,
                      const char *FileName, string &response) {

    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName);

    string Message = makeGetReqStr(Ports, Specs);
    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame
    return sendMessageTLS(wifi, Specs, Message, response);
}

// =============================================================================
// wi The socket must be connected to the 8266 chip (opened)
// the cert must be set too
// returns the int result from the mbed API
int sendBulkDataTLS(ESP8266Interface *wifi, BoardSpecs &Specs,
                    string &response) {

    string Message = makeGetReqStr(Specs);
    mbed_printf("Data frame size = %d\r\n", Message.size());
    mbed_printf("Data frame is: \r\n %s\r\n",
                Message.c_str()); // display data frame

    return sendMessageTLS(wifi, Specs, Message, response);
}
