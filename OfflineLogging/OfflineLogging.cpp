//===-- OfflineLogging.cpp Data logging definitions ----------------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/** @file
    Implementations for Data logging functions

*/
#include "OfflineLogging.h"
#include "debugging.h"
// ============================================================================
void dumpSensorDataToFile(BoardSpecs &Specs, const char *FileName) {
    FILE *File = fopen(FileName, "r");

    // if the file is not there, open in write, not append mode
    if (File == NULL) {
        mbed_printf("making new data file \r\n");
        File = fopen(FileName, "wb");

        if (File == NULL) {
            mbed_printf(
                "Failed to open %s for logging. Skipping data logging\r\n",
                FileName);
            return;
        }

    } else {
        mbed_printf("Appending data to data file \r\n");
        fclose(File);
        File = fopen(FileName, "ab");
        fseek(File, 0, SEEK_END); // go to end of the file to append data
    }

    // dump the data from all the sensors
    int End = Specs.Ports.size();
    for (int i = 0; i < End; ++i) {

        // only dump the data if the port multipler != 0
        if (Specs.Ports[i].Multiplier != 0.0f) {

            fwrite(Specs.Ports[i].Name.c_str(),
                   sizeof(char) * Specs.Ports[i].Name.size(), 1, File);

            fputc(',', File);

            fwrite(&(Specs.Ports[i].Value), sizeof(float), 1,
                   File); // write binary data

            fputc(',', File);

            fwrite(Specs.Ports[i].Description.c_str(),
                   sizeof(char) * Specs.Ports[i].Description.size(), 1, File);
            fputc('\n', File);
        }
    }

    fclose(File);
}
//=============================================================================
// 1. get the number of ports the board has
// 2. count the number of entries (\n)
// 3. go back to the beginning of the file
// 4. write (numlines - portNum) lines of text to a temporary file
// 5. delete the old file
// 6. rename the temporary file
bool deleteDataEntry(BoardSpecs &Specs, const char *FileName) {

    // get the data for the ports
    // only get the data for ports where the multiplier != 0
    int Size = 0;
    int End = Specs.Ports.size();
    PRINTLINE;
    for (int i = 0; i < End; ++i) {
        PRINTLINE;
        if (Specs.Ports[i].Multiplier != 0) {
            ++Size;
        }
    }

    FILE *DataFile = fopen(FileName, "rb");

    PRINTLINE;
    // if the file is not there, just return false (entry was not deleted)
    if (DataFile == NULL) {
        mbed_printf("Data file not found!\n");
        return false;
    }

    // see how many lines (\n) are in the file
    size_t line_num = 0;
    int charac = 'a';
    while ((charac = fgetc(DataFile)) != EOF) {
        if (charac == '\n')
            ++line_num;
    }

    // get the number of lines that you want to save
    line_num -= Size;

    // go to the beginning of the file
    rewind(DataFile);

    // at this point, you need get the remaining data into a different file
    // We do not need to eat up all the memory, so we will do it in steps

    // use constant backup filename
    const char TempFileName[] = {"/sd/#~TemporaryFile.dat~#"};

    // open the file with that temp name
    FILE *TempFile = fopen(TempFileName, "wb");

    // stop if the file was not able to be opened
    if (TempFile == NULL) {
        mbed_printf("Failed to open temporary file!\n");
        return true; // data still needs to be transmitted
    }

    // dump the data one line at a time
    // we do not want to use all of the board's memory transferring file data
    char Temp[LINESIZE];
    // transfer file contents
    while (line_num-- > 0) {
        fgets(Temp, LINESIZE, DataFile);
        fputs(Temp, TempFile);
        memset(Temp, '\0', LINESIZE * sizeof(char));
    }

    // close the data file
    fclose(DataFile);
    fclose(TempFile);

    remove(FileName);
    rename(TempFileName, FileName);
    return true; // still data to read (probably)
}

// ============================================================================
vector<PortInfo> getSensorDataFromFile(BoardSpecs &Specs,
                                       const char *FileName) {
    FILE *DataFile = fopen(FileName, "rb");

    // if the file is not there, just return an empty vector
    if (DataFile == NULL) {
        mbed_printf("Data file not found!\n");
        return std::vector<PortInfo>(0);
    }

    // get the data for the ports
    // only get the data for ports where the multiplier != 0
    int Size = 0;
    int End = Specs.Ports.size();
    for (int i = 0; i < End; ++i) {

        if (Specs.Ports[i].Multiplier != 0.0f) {
            ++Size;
        }
    }
    // init vector to read data into
    vector<PortInfo> output(Size);

    // get the data from the file to insert into the output vec
    char Temp[LINESIZE];
    for (int i = 0; i < Size; ++i) {
        fgets(Temp, LINESIZE * sizeof(char), DataFile);

        // sort through commas to initialize the port
        output[i].Name = strtok(Temp, ",");

        output[i].Multiplier = 1.0;
        memcpy(&(output[i].Value), strtok(NULL, ","), sizeof(float));

        output[i].Description = strtok(NULL, "\n");
        mbed_printf(output[i].Description.c_str());
    }
    fclose(DataFile);
    return output;
}

// ============================================================================
bool checkForBackupFile(const char *FileName) {
    // try to open the file to see if it is there.
    FILE *BackupFile = fopen(FileName, "rb");

    if (BackupFile == NULL)
        return false;

    fclose(BackupFile);
    return true;
}
