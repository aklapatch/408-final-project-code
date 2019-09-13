#include "Networking.h"
#include "debugging.h"
//===-- Networking.cpp - Networking function definitions -----------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/// \file
/// Has the definitions for the funtions that interface with other networks.


/// for tesla.cs.iupui.edu
const char * web_server_ip = "134.68.51.14";
/// for https connections
uint16_t web_server_port = 443;

const char * bulk_get_request = "GET /MEuser/bulk-sensor-readings.php?Board_ID=";

const char* value_get_str = "&Value[]=";
const char * port_get_str = "&Port_ID[]=";
const char * ca_cert = "";


int connectESPWiFi(ESP8266Interface * wifi, BoardSpecs &Specs) {
    return wifi->connect(Specs.NetworkSSID.c_str(), Specs.NetworkPassword.c_str());
}
      


/// return true if you are connected, and false if you are not connected
bool checkESPWiFiConnection(ESP8266Interface * wifi){
    return wifi->get_ip_address() != NULL;
}

// FIXME: add the certificate step
int sendMessageTLS(ESP8266Interface * wifi, string & message){

    // connect to the web server
    TLSSocket * sock = new TLSSocket();

    int err = sock->set_root_ca_cert(ca_cert);
    if (err !=NSAPI_ERROR_OK)
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

    while ((err = sock->recv(buffer, 255)) > 0) {
        buffer[255] = 0;
        printf("%s",buffer);
    }
    printf("\r\n");


CLOSEFREE:
    sock->close();
FREE:
    delete sock;
    return err;
}

// sends a vector of port values instead
// gets those port values from the backup file
int sendBackupDataTLS(ESP8266Interface * wifi, BoardSpecs &Specs, const char* FileName) {
    
    vector<PortInfo> Ports = getSensorDataFromFile(Specs, FileName); 
 
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);


    for(size_t i = 0; i < End; ++i){
        if (Ports[i].Multiplier != 0){
            message_size +=  Ports[i].Name.size() + toString(Ports[i].Value).size() + get_extras;
           
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");

    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

    Message.append(Specs.DatabaseTableName);

    // append to get request for every active port
    for(size_t i = 0; i < End; ++i){
        if (Ports[i].Multiplier != 0){
            Message.append(port_get_str);
            Message.append(Ports[i].Name);
            Message.append(value_get_str);
            Message.append(toString(Ports[i].Value));
        }
    }
    Message.append("\r\n");
    
    printf("Data frame size = %d\r\n", Message.size());  
    printf("Data frame is: \r\n %s\r\n",Message.c_str()); // display data frame

    return sendMessageTLS(wifi, Message);
}


/// wi The socket must be connected to the 8266 chip (opened)
/// the cert must be set too
/// returns the int result from the mbed API
int sendBulkDataTLS(ESP8266Interface * wifi,BoardSpecs &Specs) {
 
    // make the message to send
    // get the size to allocate memory
    size_t message_size = strlen(bulk_get_request) + Specs.DatabaseTableName.size();

    size_t End = Specs.Ports.size();
    size_t get_extras = strlen(port_get_str) + strlen(value_get_str);


    for(size_t i = 0; i < End; ++i){
        if (Specs.Ports[i].Multiplier != 0){
            message_size +=  Specs.Ports[i].Name.size() + toString(Specs.Ports[i].Value).size() + get_extras;
           
        }
    }
    // add on for the \r\n
    message_size += strlen("\r\n");


    string Message = bulk_get_request;

    // reserve so that further allocations are not needed
    Message.reserve(message_size);

    Message.append(Specs.DatabaseTableName);

    // append to get request for every active port
    for(size_t i = 0; i < End; ++i){
        if (Specs.Ports[i].Multiplier != 0){
            Message.append(port_get_str);
            Message.append(Specs.Ports[i].Name);
            Message.append(value_get_str);
            Message.append(toString(Specs.Ports[i].Value));
        }
    }
    Message.append("\r\n");
    
    printf("Data frame size = %d\r\n", Message.size());  
    printf("Data frame is: \r\n %s\r\n",Message.c_str()); // display data frame

    return sendMessageTLS(wifi, Message);
}
// The functions below are from the old API ====================================

// returns a string with all the available networks
string getNetworks(Serial & Antenna){
    sendCMD(Antenna, "AT+CWLAP=\r\n");
    printf (getReply(Antenna,1).c_str());
    sendCMD(Antenna, "AT+CWLAP\r\n");
    return getReply(Antenna,4);
}

//==============================================================================
// returns true if the network is there to connect to, false otherwise
bool canConnectToNetwork(Serial & Antenna, string NetworkSSID){
    
    string Networks = getNetworks(Antenna);
    printf("\r\nNetworks are: %s \r\n", Networks.c_str());
    
    // don't connect if the network is not there
    if (Networks.find(NetworkSSID) == string::npos){
        return false;
    }   
    return true;
}

bool checkServerConnection(Serial & Antenna){
    sendCMD(Antenna, CONNECTSTRING);
    printf("\r\n Checking Database Connection.\r\n");

    string Temp = getReply(Antenna, 3);
    
    printf("\r\n Reply Was:  %s \r\n",Temp.c_str()); 
    
    sendCMD(Antenna, "AT+CIPCLOSE=5\r\n");
    
    printf("\r\n Closing Database Connection: %s \r\n", getReply(Antenna, 3).c_str());
    
    // connection is broken if you get no response
    if ( !hasOK(Temp) ){
        printf("\r\nWebserver Connection Failed\r\n");
        return false;
    }
    
    printf("\r\n Webserver Connection Succeeded \r\n");
    return true;   
}
// ============================================================================
void sendCMD(Serial &Bus, char *Message)
{
    Bus.printf("%s", Message);
}

// ============================================================================
void sendCMD(Serial &Bus, string Message)
{
    Bus.printf("%s", Message.c_str());
}

// ============================================================================
void sendCMD(Serial &Bus, const char *Message)
{
    Bus.printf("%s", Message);
}

// ============================================================================
string getReply(Serial &Bus, float timeout)
{

    // init output string
    string output(BUFFLEN, '\0');

    Timer tmp;
    tmp.start();

    int count = 0;

    while (tmp.read() <= timeout) {

        // reads from the bus and stores it into the buffer
        if (Bus.readable()) {
            output[count] = Bus.getc();
            ++count;
        }
    }

    tmp.stop(); // stop timer
    
    // trim the string
    output = output.substr(0,output.find_first_of('\0'));

    return output;
}

// ============================================================================
string getReplyWithBuffer(Serial &Bus, float StartDelay)
{
    // the Serial class has a way to get all the data into a buffer at once.
    // this method tries to use that

    wait(StartDelay); // wait for so many seconds before starting to read

    while (!Bus.readable()) {
    } // wait until the connection can be read.

    string output(BUFFLEN, '\0'); // buffer to store result

    // read data into string
    int SizeRead = Bus.scanf("%s", (char *)output.data());

    if (SizeRead == 0) {
        printf("Read 0 bytes from serial interface.\n");
    }

    return output;
}

// ============================================================================
bool sendDataTCP(Serial &Debug, Serial &Antenna, PortInfo Port,
                 string &TableName)
{

    return sendFloatToDataBase(Debug, Antenna, TableName, Port.Name, Port.Value);
}

bool hasOK(string Message){
    return Message.find("OK") != string::npos;
}

// ============================================================================
bool sendBulkDataTCP(Serial &Debug, Serial &Antenna, BoardSpecs &Specs)
{
    Debug.printf("\n---------- Start TCP_IP Connection Database---------\r\n");

    /// TODO try port 80
    sendCMD(Antenna, CONNECTSTRING);

    string Temp = getReply(Antenna, 3);
    
    // check command reply
    printf("\r\n %s \r\n",Temp.c_str());
    
    // connection is broken if you get no response
    if ( !hasOK(Temp)){
        printf("\r\nWebserver Connection Failed\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here
        printf("\r\n %s \r\n",getReply(Antenna, 3).c_str());   
        return false;
    }
    
    printf("Webserver Connection was successful :)\r\n");
    Debug.printf("\n---------- Set TCP Data frame ----------\r\n");

    string Message =
        "GET /MEuser/bulk-sensor-readings.php?Board_ID=" + Specs.DatabaseTableName;
    
    // append to get request for every active port
    size_t End = Specs.Ports.size();
    for(size_t i = 0; i < End; ++i){
        if (Specs.Ports[i].Multiplier != 0){
            Message.append("&Port_ID[]=" + Specs.Ports[i].Name + 
                "&Value[]=" + toString(Specs.Ports[i].Value));
        }
    }
    Message += "\r\n";
    
    Debug.printf("\r\nframe size = %d\r\n", (int)Message.size());

    // send the size of the data frame
    sendCMD(Antenna, "AT+CIPSEND=4," + toString((int)Message.size()) + "\r\n");
    
    Temp = getReply(Antenna,4);

    Debug.printf("\r\nReply to Frame size:\r\n %s\r\n",Temp.c_str());

    Debug.printf("\r\n---------- Send Data frame ----------\r\n");
    
    Debug.printf("Data frame is: \r\n %s\r\n",Message.c_str()); // display data frame

    sendCMD(Antenna, Message); // send data frame
    
        // display the reply and print it
    Temp = getReply(Antenna, 3);

    Debug.printf("\r\n Reply =\r\n %s\r\n",Temp.c_str());
    
    if ( !hasOK(Temp)){
        printf("\r\nFailed to send frame\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here
        return false;
    }
    
    // if you get a 'not found', you did not send data
    if ( Temp.find("404") != string::npos){
        printf("\r\nfound 404\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n");
        return false;
    }

    Debug.printf("\n---------- Close TCP/IP Connection ----------\r\n");

    sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here

    Temp = getReply(Antenna, 1); // possible optimization

    Debug.printf(Temp.c_str());
    
    return true;
}
//============================================================================
bool sendBulkDataTCP(Serial &Debug, Serial &Antenna, vector<PortInfo> Ports, BoardSpecs & Specs)
{
    Debug.printf("\n---------- Start TCP_IP Connection Database---------\r\n");

    /// TODO try port 80
    sendCMD(Antenna, CONNECTSTRING);

    string Temp = getReply(Antenna, 3);
    
    // check command reply
    printf("\r\n %s \r\n",Temp.c_str());
    
    // connection is broken if you get no response
    if ( !hasOK(Temp)){
        printf("\r\nWebserver Connection Failed\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here
        printf("\r\n %s \r\n",getReply(Antenna, 3).c_str());   
        return false;
    }
    
    printf("Webserver Connection was successful :)\r\n");
    Debug.printf("\n---------- Set TCP Data frame ----------\r\n");
    PRINTLINE;
    /// FIXME: the board pauses right around here
    string Message =
        "GET /MEuser/bulk-sensor-readings.php?Board_ID=" + Specs.DatabaseTableName;
    PRINTLINE;
    // append to get request for every active port
    size_t End = Ports.size();
    for(size_t i = 0; i < End; ++i){
        PRINTLINE;
        Message += "&Port_ID[]=" + Ports[i].Name + "&Value[]=" + toString(Ports[i].Value);
    }
    Message += "\r\n";
    
    Debug.printf("\r\nframe size = %d\r\n", (int)Message.size());

    // send the size of the data frame
    sendCMD(Antenna, "AT+CIPSEND=4," + toString((int)Message.size()) + "\r\n");

    Temp = getReply(Antenna,4);

    Debug.printf("\r\nReply to Frame size:\r\n %s\r\n",Temp.c_str());
    
    // close connection and exit on error
    if (checkNetworkError(Temp)){
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here
        getReply(Antenna,1);
        return false;
    }

    Debug.printf("\r\n---------- Send Data frame ----------\r\n");
    
    Debug.printf("Data frame is: \r\n %s\r\n",Message.c_str()); // display data frame

    sendCMD(Antenna, Message); // send data frame
    
        // display the reply and print it
    Temp = getReply(Antenna, 3);

    Debug.printf("\r\n Reply =\r\n %s\r\n",Temp.c_str());
    
    if ( !hasOK(Temp)){
        printf("\r\nFailed to send frame\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here
        return false;
    }
    
    // if you get a 'not found' error, you did not send data
    if ( Temp.find("404") != string::npos){
        printf("\r\nfound 404\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n");
        return false;
    }

    Debug.printf("\n---------- Close TCP/IP Connection ----------\r\n");

    sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here

    Temp = getReply(Antenna, 1); // possible optimization

    Debug.printf(Temp.c_str());
    
    return true;
}

// ============================================================================
short setESPMode(Serial & Antenna)
{
    Antenna.printf("AT\r\n");        
        
    string Reply = getReply(Antenna, 1); // optimization opportunity

    printf(Reply.c_str());

    if (!hasOK(Reply)){ // chip is not good if there is no 'OK'
        printf("\r\nERROR: chip did not respond to the AT command\r\n");
        return -1;
    }

    printf("\r\n---------- Starting ESP Config ----------\r\n\n");

    printf("---------- Reset & get Firmware ----------\r\n");

    sendCMD(Antenna, "AT+RST\r\n");

    Reply = getReply(Antenna,1);

    printf(Reply.c_str());

    // return error if the chip does not return 
    // TODO: decide the character that should be detected
    if ( Reply.find(":") == string::npos){
        printf("\r\nERROR: wifi reset failed\r\n");
        return -2;

    }

    printf("\n---------- Get Version ----------\r\n");

    sendCMD(Antenna, "AT+GMR\r\n");

    Reply = getReply(Antenna, 1); // optimization opportunity
    printf(Reply.c_str());

    if (!hasOK(Reply)){ // there should be an 'OK' in the response
        printf("\r\nERROR: wifi chip version retriaval failed!\r\n");
        return -3; // return an error if there is no 'OK'
    }

    // set CWMODE to 1=Station,2=AP,3=BOTH, default mode 1 (Station)
    printf("\n---------- Setting Mode ----------\r\n");

    sendCMD(Antenna, "AT+CWMODE=1\r\n");

    Reply = getReply(Antenna, 1); // optimization opportunity
    printf(Reply.c_str());

    if (!hasOK(Reply)){ // return error if there is no 'OK'
        PRINTLINE;
        printf("\r\nERROR: Failed to set WIFI chip to AP mode\r\n");
        
        return -4;
    }

    printf("\r\n---------- Setting Connection Mode ----------\r\n");

    // set CIPMUX to 0=Single,1=Multi
    sendCMD(Antenna, "AT+CIPMUX=1\r\n");

    Reply = getReply(Antenna, 1); // optimization opportunity

    printf(Reply.c_str());

    if (!hasOK(Reply)){
        PRINTLINE;
        printf("\r\nERROR: Failed to set TCP connection mode (multi/single)\r\n");
        return -5;
    }

    return 0; // successful configuration set
}

// ============================================================================
bool connectToWiFi(Serial &Debug, Serial &Antenna, BoardSpecs &Specs)
{
    
    Debug.printf("\n---------- Trying to connecting to AP ----------\r\n");
    Debug.printf("ssid = %s   pwd = %s\r\n", Specs.NetworkSSID.c_str(),
                 Specs.NetworkPassword.c_str());

    string Reply = "AT+CWJAP=\"" + Specs.NetworkSSID + "\",\"" + Specs.NetworkPassword +
                   "\"\r\n";
    /* Here (\") denotes inverted commas in strings */

    Debug.printf("\r\n Conecting to AP with command %s\r\n",Reply.c_str());

    sendCMD(Antenna, Reply.c_str());

    Reply = getReply(Antenna, 5); // optimization opportunity

    Debug.printf(Reply.c_str());
    printf("\r\n Size of reply is %d\r\n", Reply.size());
    
    if (Reply.find("FAIL") != string::npos)
        return false;
        
     // the string has null chars if nothing was returned   

    return true;
}

// ============================================================================
bool checkWiFiConnection(Serial &Antenna, BoardSpecs & Specs)
{
    printf("\n-Checking IP to determine WiFi connection status -\r\n");

    sendCMD(Antenna, "AT+CIFSR\r\n");

    string Reply = getReply(Antenna, 3); // optimization opportunity

    // c++ way to get the WIFIIP
    size_t IPBegin = Reply.find_first_of("\"");

    // Begin needs to be increased anyway to avoid \" later
    size_t IPEnd = Reply.find_first_of("\"", ++IPBegin);

    Specs.WiFiIP = Reply.substr(IPBegin, IPEnd - IPBegin);

    printf("\nThe Board's ip address = %s\r\n", Specs.WiFiIP.c_str());

    printf(
        "\n\n\n  If you get a valid (non zero) IP, ESP8266 has been set up.\r\n");

    
    // if there are no .'s , there are no ips and we are not connected
    if (Specs.WiFiIP.find(".") == string::npos)
        return false;
    
    // if the IP is != 0.0.0.0, then it is connected    
    if (Specs.WiFiIP.find("0.0.0.0") != string::npos){
        return false;
    }
    
    return true;
}
// ============================================================================
bool sendFloatToDataBase(Serial &Debug, Serial &Antenna, string RemoteTableName,
                         string PortName, float Reading)
{
    Debug.printf("\n---------- Start TCP_IP Connection with WAMP ----------\r\n");
    /// TODO try port 80
    sendCMD(Antenna, CONNECTSTRING);

    string Temp = getReply(Antenna, 3);
    
    // check command reply
    printf ("\r\n %s \r\n",Temp.c_str());
    
    // connection is broken if you get no response
    if (!hasOK(Temp)){
        printf("\r\nWebserver Connection Failed\r\n");
        return false;
    }
    
    printf("Webserver Connection was successful :)\r\n");
    Debug.printf("\n---------- Set TCP Data frame ----------\r\n");

    string Message =
        "GET /MEuser/Sensor_readings.php?Board_ID=" + RemoteTableName +
        "&Port_ID=" + PortName + "&Value=" + toString(Reading) + "\r\n";
    // print size of data frame
    Debug.printf("\r\nframe size = %d\r\n", Message.size());

    // send the size of the data frame
    sendCMD(Antenna, "AT+CIPSEND=4," + toString((int)Message.size()) + "\r\n");

    Temp = getReply(Antenna,4);
    Debug.printf("\r\nReply to Frame size:\r\n %s\r\n",Temp.c_str());
    if (!hasOK(Temp))
        return false;

    Debug.printf("\r\n---------- Send Data frame ----------\r\n");
    sendCMD(Antenna, Message); // send data frame

    Debug.printf("Data frame is: \r\n %s\r\n",Message.c_str()); // display data frame

    // display the reply and print it
    Temp = getReply(Antenna, 3);
    
    Debug.printf("\r\n Reply =\r\n %s\r\n",Temp.c_str());

    if (!hasOK(Temp))
        return false;
    
    // if you get a 'not found', you did not send data
    if ( Temp.find("404") != string::npos){
        printf("\r\nfound 404\r\n");
        sendCMD(Antenna, "AT+CIPCLOSE=5\r\n");
        return false;
    }

    Debug.printf("\n---------- Close TCP/IP Connection ----------\r\n");

    sendCMD(Antenna, "AT+CIPCLOSE=5\r\n"); // close here

    Temp = getReply(Antenna, 1); // possible optimization

    Debug.printf(Temp.c_str());
    
    return true;
}

// ============================================================================
bool sendBackupDataTCP(Serial &Debug, Serial &Antenna, BoardSpecs &Specs,
                       const char * BackupFileName)
{

    // get the backup data from the file
    printf("\r\nGetting Sensor Data entries from the backup file\r\n");
    vector<PortInfo> BackupData = getSensorDataFromFile(Specs, BackupFileName);
    

    // sends all the port data from the Specs struct
    printf("\r\n trying to Send backed up data\r\n");
    return sendBulkDataTCP(Debug, Antenna, BackupData, Specs);
}

// ============================================================================
bool checkNetworkError(string Message){
        
    if (Message.find("ERROR") != string::npos){
        return true;
    }
    
    if (Message.find("FAIL") != string::npos) {
        return true;
    }
    
    return false;
}
