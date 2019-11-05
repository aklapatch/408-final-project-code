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
    }

    // dump the data from all the sensors
    int End = Specs.Ports.size();
    for (int i = 0; i < End; ++i) {

        // only dump the data if the port multipler != 0
        if (Specs.Ports[i].Multiplier != 0.0f) {

            fprintf(File, 
                    "%s,%f,%s",
                    Specs.Ports[i].Name.c_str(),
                    Specs.Ports[i].Value, 
                    Specs.Ports[i].Description.c_str()
                    );
            // I moved adding the \n down here because fprintf may have been botching it
            // I am not sure though act as you feel is best
            fputc('\n',File);
        }
    }

    fclose(File);
}
//=============================================================================
// 1. get the number of ports the board has (n ports)
// 2. go down n ports
// 3. transfer the rest of the file to another file (the first n entries were
// read earlier);
// 4. delete the original file
bool deleteDataEntry(BoardSpecs &Specs, const char *FileName) {
    mbed_printf("Deleting data entry!\r\n");
    // get the data for the ports
    // only get the data for ports where the multiplier != 0
    int Size = 0;
    int End = Specs.Ports.size();
    for (int i = 0; i < End; ++i) {
        if (Specs.Ports[i].Multiplier != 0.0f) {
            ++Size;
        }
    }

    FILE *DataFile = fopen(FileName, "rb");

    // if the file is not there, just return false (entry was not deleted)
    if (DataFile == NULL) {
        mbed_printf("Data file not found!\n");
        return false;
    }

    // go down N port readings in the file
    char *fgetstatus;
    char Line[LINESIZE + 1];
    while (Size > 0) {
        fgetstatus = fgets(Line, LINESIZE, DataFile);
        // in this case, you have already sent all the data entries and the
        // backup file should be deleted
        if (fgetstatus == NULL) {
            fclose(DataFile);
            remove(FileName);
            return false;
        }
        --Size;
    }
    int flocation = ftell(DataFile);
    if (fgets(Line, LINESIZE, DataFile) == NULL){
        fclose(DataFile);
        remove(FileName);
        mbed_printf("Removed DataFile\r\n");
        return false;
    }

    memset(Line, 0, LINESIZE+1);

    // reset file position
    fseek(DataFile, flocation, SEEK_SET);

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

    // transfer file contents

    while (fgets(Line, LINESIZE, DataFile) != NULL) {
        fputs(Line, TempFile);
        memset(Line, 0, LINESIZE + 1);
    }

    // close the data file
    fclose(DataFile);
    fclose(TempFile);

    remove(FileName); // rename the DataFile
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
    char Line[LINESIZE+1];
    Line[LINESIZE] =0;

    for (int i = 0; i < Size; ++i) {

        // grab all the data
        fgets(Line, LINESIZE, DataFile);

        output[i].Name = strtok(Line, ",");


        char * number = strtok(NULL, ",");

        output[i].Value = atof(number);

        output[i].Description = strtok(NULL, "\n");

        output[i].Multiplier = 1.0f;
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
