/// \file
/// \brief Definitions for board configuration functions
#include "BoardConfig.h"
#include "debugging.h"
#include <cctype>

void printSpecs(BoardSpecs &Specs) {
    printf("\r\nBoard information\r\n");
    printf("Network SSID = %s \r\n", Specs.NetworkSSID.c_str());

    printf("Network Password = %s\t", Specs.NetworkPassword.c_str());
    printf("Board name = %s\r\n",
           Specs.DatabaseTableName.c_str());


    printf("Remote IP = %s\t", Specs.RemoteIP.c_str());
    printf("Remote Get Request directory = %s\r\n",

           Specs.RemoteDir.c_str());

    printf("remote http port = %d\t", Specs.RemotePort);

    printf("Remote Hostname = %s\r\n", Specs.HostName.c_str());
}
// ============================================================================
BoardSpecs readSDCard(const char *FileName) {

    // try to open sd card
    printf("\r\nReading from SD card...\r\n\n\n");
    FILE *fp = fopen(FileName, "rb");

    BoardSpecs Output;

    if (fp != NULL) {

        // get file size for buffer
        fseek(fp, 0, SEEK_END);
        size_t FileSize = ftell(fp);
        char *Buffer = new char[FileSize + 1];

        // reset file pointer and  read file
        rewind(fp);
        fread(Buffer, sizeof(char), FileSize, fp);
        // set \0 for Cstring compatability
        Buffer[FileSize] = '\0';

        delete[] Buffer; // clean up

        // read config from SD card
        rewind(fp);
        Output = readConfigText(fp);
        printf("\r\n %d Ports were configured\r\n", Output.Ports.size());
        fclose(fp);

    } else {
        printf("\nReading Failed!\r\n");
    }

    return Output;
}


// ============================================================================
BoardSpecs readConfigText(FILE *fp) {
    BoardSpecs Specs;
    Specs.Ports.reserve(10);   // reserve space for ports
    Specs.Sensors.reserve(10); // and sensor types

    int prtCnt = 0; // # of ports

    // temporary buffer
    char Buffer[BUFFLEN];

    // read through and get sensor ids before getting the port information
    while (fgets(Buffer, BUFFLEN, fp) != NULL) {

        // if the line has 'SensorID' in it, then get the sensor info from it
        if (strstr(Buffer, "SensorID") && Buffer[0] == 'S') {
            SensorInfo tmp;

            // get the id number
            strtok(Buffer, ":"); // get past the :

            const char token[] = ","; // token to split values

            char *value = strtok(NULL, token);
            tmp.Type = value;

            // get the unit of the sensor
            value = strtok(NULL, token);
            tmp.Unit = value;

            // get unit multiplier
            value = strtok(NULL, token);
            tmp.Multiplier = atof(value);

            // get range start
            value = strtok(NULL, token);
            tmp.RangeStart = atof(value);

            // get range end till end of line
            value = strtok(NULL, "\n");
            tmp.RangeEnd = atof(value);

            Specs.Sensors.push_back(tmp); // store those values
            printf("Sensor type: %s, Unit: %s, range start: %f, range-end: %f\r\n", tmp.Type.c_str(),
                   tmp.Unit.c_str(), tmp.RangeStart, tmp.RangeEnd);
        }
    }

    rewind(fp); // get ready to read again.

    while (fgets(Buffer, BUFFLEN, fp) != NULL) {

        const char *s = ":";

        // save the remote connection info
        if (Buffer[0] == 'C' && strstr(Buffer, "ConnInfo")) {

            // we don't need the first token
            char *tmp = strtok(Buffer, s);

            Specs.RemoteIP = strtok(NULL, ",");

            // make sure there is a digit to convert, and set an error value
            tmp = strtok(NULL, ",");
            if (isdigit(tmp[0])) {
                Specs.RemotePort = atoi(tmp);
            } else {
                Specs.RemotePort = 0;
            }

            Specs.HostName = strtok(NULL, ",");

            Specs.RemoteDir = strtok(NULL, "\n");
        }

        // checks the character at the beginning of each line
        if (Buffer[0] == 'B' && strstr(Buffer, "Board")) {

            // get past the :
            strtok(Buffer, s);

            // get WIFI SSID and assign it
            Specs.NetworkSSID = strtok(NULL, ",");

            // get WIFI Password and assign it
            Specs.NetworkPassword = strtok(NULL, ",");

            // getting and assigning Database tablename
            Specs.DatabaseTableName = strtok(NULL, "\n"); 
        } else if (Buffer[0] == 'P' && strstr(Buffer, "Port")) { // if a port description is detected

            // hold a Port entry
            // then assign them to the vector in Specs
            PortInfo tmp;

            ++prtCnt;
            // skip the : 
            strtok(Buffer, ":"); 
            
            // grab the port name 
            tmp.Name = strtok(NULL, ",");

            tmp.SensorID = atoi(strtok(NULL, "\n"));

            if (tmp.SensorID < Specs.Sensors.size() && tmp.SensorID >= 0){

            // get port multiplier
                tmp.Multiplier = Specs.Sensors[ tmp.SensorID].Multiplier;

                // get sensorname
                tmp.Description = Specs.Sensors[ tmp.SensorID].Type;
                tmp.Description.append(" in ");
                tmp.Description.append(Specs.Sensors[ tmp.SensorID].Unit);

                tmp.RangeEnd = Specs.Sensors[tmp.SensorID].RangeEnd;
                tmp.RangeStart= Specs.Sensors[tmp.SensorID].RangeStart;

            printf("Port Info: name= %s id=  %d Multiplier= %0.2f description=%s\r\n", tmp.Name.c_str(),

                   tmp.SensorID, tmp.Multiplier, tmp.Description.c_str());

            if (tmp.Multiplier != 0.0f){
                Specs.Ports.push_back(tmp);
            }
            else {

                printf ("Port %s has a multiplier of 0, skipping\r\n", tmp.Name.c_str());
            }
            }
            else {
                // skip this sensor, sensor id is bad
                printf("Port %s has an out of bounds Sensor ID, skipping\r\n", tmp.Name.c_str());
            }
            

        }
    }
    printSpecs(Specs);
    return Specs;
}

