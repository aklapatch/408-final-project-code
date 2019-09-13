//===-- BoardConfig.cpp - Definitions for board config functions ----------===//
//
// Part of the IAC energy monitoring project
//
//===----------------------------------------------------------------------===//
/// \file
/// Has function definitions for configuring the board with its data structures

#include "BoardConfig.h"
#include "debugging.h"
// ============================================================================
BoardSpecs readSDCard(const char *FileName) {

    // try to open sd card
    mbed_printf("\r\nReading from SD card...\r\n\n\n");
    FILE *fp = fopen(FileName, "rb"); // the 'b' in 'rb' may not be necessary

    BoardSpecs Output;

    if (fp != NULL) {
        mbed_printf(" \r\n ---- Config File ---- \r\n");

        // get file size for buffer
        fseek(fp, 0, SEEK_END);
        size_t FileSize = ftell(fp);
        char *Buffer = new char[FileSize + 1];

        // reset file pointer and  read file
        rewind(fp);
        fread(Buffer, sizeof(char), FileSize, fp);
        // set \0 for Cstring compatability
        Buffer[FileSize] = '\0';

        // print it
        mbed_printf("%s", Buffer);

        delete[] Buffer; // clean up

        mbed_printf(" \r\n ---- End of Config File ---- \r\n");

        // read config from SD card
        rewind(fp);
        Output = readConfigText(fp);
        mbed_printf("\r\n %d Ports were configured\r\n", Output.Ports.size());
        fclose(fp);

    } else {
        mbed_printf("\nReading Failed!\r\n");
    }

    return Output;
}

// increments Text until is not pointed at a ' ' character
char *goPastSpaces(char *Text) {
    while (Text[0] == ' ') {
        ++Text;
    }
    return Text;
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
            strtok(Buffer, "="); // get past the = sign

            const char token[] = ","; // token to split values

            char *value = strtok(NULL, token);
            tmp.ID = atoi(value); // get id value
            PRINTINT(tmp.ID);

            // get waht the sensor is measuring
            value = strtok(NULL, token);
            tmp.Type = value;
            PRINTSTRING(tmp.Type);

            // get the unit of the sensor
            value = strtok(NULL, token);
            tmp.Unit = value;
            PRINTSTRING(tmp.Unit);

            // get unit multiplier
            value = strtok(NULL, token);
            tmp.Multiplier = atof(value);
            PRINTFLOAT(tmp.Multiplier);

            // get range start
            value = strtok(NULL, token);
            tmp.RangeStart = atof(value);

            // get range end till end of line
            value = strtok(NULL, "\n");
            tmp.RangeEnd = atof(value);

            Specs.Sensors.push_back(tmp); // store those values
        }
    }

    rewind(fp); // get ready to read again.

    // get a whole line instead of reading one char at a time
    while (fgets(Buffer, BUFFLEN, fp) != NULL) {

        // checks the character at the beginning of each line
        if (Buffer[0] == 'B' && strstr(Buffer, "Board")) {
            const char s[2] = ":";

            // get board id and assign it
            Specs.ID = strtok(Buffer, s);

            // get WIFI SSID and assign it
            Specs.NetworkSSID = strtok(NULL, s);

            // get WIFI Password and assign it
            Specs.NetworkPassword = strtok(NULL, s);

            // getting and assigning Database tablename
            Specs.DatabaseTableName = strtok(NULL, s); // opt opportunity
        } else if (Buffer[0] == 'P') { // if a port description is detected

            // use a deque or queue to temporarily hold a Port entry
            // then assign them to the vector in Specs
            PortInfo tmp;

            ++prtCnt;

            if (prtCnt < 10) {

                // get sensor ID
                tmp.SensorID = int(Buffer[9] - '0');

            } else {
                tmp.SensorID = int(Buffer[10] - '0');
            }
            // grab the port name too
            tmp.Name = strtok(Buffer, ":");

            // remove whitespace from the name
            size_t SpaceDex = tmp.Name.find_first_of(' ');
            while (SpaceDex != string::npos) {
                tmp.Name.erase(SpaceDex);
                SpaceDex = tmp.Name.find_first_of(' ');
            }

            // get port multiplier
            tmp.Multiplier = setUnitMultiplier(Specs.Sensors, tmp.SensorID);

            // get sensorname
            tmp.Description = getSensorName(Specs.Sensors, tmp.SensorID);

            mbed_printf("Port Info: %s  %d  %0.2f    %s\r\n", tmp.Name.c_str(),
                   tmp.SensorID, tmp.Multiplier, tmp.Description.c_str());

            // store the port in the boardSpecs struct only if it means anything
            if (tmp.Multiplier != 0.0f) {
                Specs.Ports.push_back(tmp);
            }
        }
    }

    return Specs;
}

// ============================================================================
float setUnitMultiplier(vector<SensorInfo> &Sensors, size_t Sens_ID) {
    // if there is no sensor for that id, then return 0
    if (Sens_ID >= Sensors.size()) {
        return 0.0;
    }

    return Sensors[Sens_ID].Multiplier;
}

// ============================================================================
string getSensorName(vector<SensorInfo> &Sensors, size_t Sens_ID) {

    // if there is no sensor for that id, then return error message
    if (Sens_ID >= Sensors.size()) {
        return "No Sensor";
    }

    const char * in = " in ";

    size_t str_size = strlen(in) + Sensors[Sens_ID].Type.size() + Sensors[Sens_ID].Unit.size();

    string ret_str; ret_str.reserve(str_size);

    ret_str.append(Sensors[Sens_ID].Type);
    ret_str.append(in);
    ret_str.append(Sensors[Sens_ID].Unit);

    return ret_str; 
}
