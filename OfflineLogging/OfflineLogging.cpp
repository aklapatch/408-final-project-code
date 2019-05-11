//===-- OfflineLogging.cpp Data logging definitions ----------------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/** @file
 Has definitions for functions that log data that cannot be sent to a
 database
*/
#include "OfflineLogging.h"
#include "debugging.h"
// ============================================================================
void dumpSensorDataToFile(BoardSpecs &Specs, const char *FileName)
{
    FILE *File = fopen(FileName, "r");

    // if the file is not there, open in write, not append mode
    if (File == NULL) {
        printf("making new data file \r\n");
        File = fopen(FileName, "wb");
    } else {
        printf("Appending data to data file \r\n");
        fclose(File);
        File = fopen(FileName, "ab");
        fseek(File, 0, SEEK_END); // go to end of the file to append data
    }

    // dump the data from all the sensors
    int End = Specs.Ports.size();
    for(int i = 0; i  < End; ++i) {

        // only dump the data if the port multipler != 0
        if (Specs.Ports[i].Multiplier != 0.0f) {
            
            fwrite(Specs.Ports[i].Name.c_str(),sizeof(char)*Specs.Ports[i].Name.size(),1, File);
            
            fputc(',',File);
            
            fwrite(&(Specs.Ports[i].Value), sizeof(float), 1, File); // write binary data
            
            fputc(',',File);
            
            fwrite(Specs.Ports[i].Description.c_str(),sizeof(char)*Specs.Ports[i].Description.size(),1,File);
            fputc('\n', File);
        }
    }

    fclose(File);
}
//=============================================================================
bool deleteDataEntry(BoardSpecs & Specs,const char *FileName){
    FILE *DataFile = fopen(FileName, "rb");

    PRINTLINE;
    // if the file is not there, just return false (entry was not deleted)
    if (DataFile == NULL) {
        printf("Data file not found!\n");
        return false;
    }

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

    // go through the same number of entries that the port has
    for(int i = 0; i < Size; ++i){
        PRINTLINE;
        int c = 0;
        while((c =fgetc(DataFile)) != '\n' && c != EOF){ 
            printf("\r\n var is %d\r\n",c); 
        }
        
        if (c == EOF){ // return false if the end of the file was hit
            fclose(DataFile);
            printf("\r\nData file emptied, deleting it now.\r\n");
            remove(FileName); // remove the backup file
            return false;
        }
            
        PRINTLINE;
    }

    // at this point, you need get the remaining data into a different file
    // We do not need to eat up all the memory, so we will do it in steps
    
    // use constant backup filename
    const char TempFileName[] = { "/sd/.~_TemporaryFile.dat" };

    // open the file with that temp name
    FILE *TempFile = fopen(TempFileName, "wb");

    // stop if the file was not able to be opened
    if (TempFile == NULL) {
        printf("Failed to open temporary file!\n");
        return true; // data still needs to be transmitted
    }

    // dump the data one line at a time
    // we do not want to use all of the board's memory transferring file data
    char Temp[LINESIZE];
    size_t BytesRead = LINESIZE; 
    // transfer file contents
    while ( BytesRead == LINESIZE){ // condition stops when EOF happens
        BytesRead = fread(Temp,sizeof(char),LINESIZE, DataFile);
        
        fwrite(Temp, sizeof(char), BytesRead, TempFile);
    }

    // close the data file
    fclose(DataFile);
    fclose(TempFile);
    
    DataFile = fopen(FileName,"wb");
    TempFile = fopen(TempFileName,"rb");
    if (DataFile == NULL || TempFile == NULL){
        printf("\r\ncould not open file for reading back into it\r\n");
    } else {
            // transfer file contents
        while ( BytesRead == LINESIZE){ // condition stops when EOF happens
            BytesRead = fread(Temp,sizeof(char),LINESIZE, TempFile);
            
            fwrite(DataFile, sizeof(char), BytesRead, TempFile);
        }
    }
    fclose(DataFile);
    fclose(TempFile);
    remove(TempFileName);
    return true; // still data to read (probably)
}

// ============================================================================
vector<PortInfo> getSensorDataFromFile(BoardSpecs &Specs, const char *FileName)
{
    FILE *DataFile = fopen(FileName, "rb");

    // if the file is not there, just return an empty vector
    if (DataFile == NULL) {
        printf("Data file not found!\n");
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
    for(int i = 0; i < Size; ++i){
        fgets(Temp, LINESIZE*sizeof(char), DataFile);
        
        // sort through commas to initialize the port
        output[i].Name = strtok(Temp,",");
        
        output[i].Multiplier = 1.0;
        memcpy(&(output[i].Value),strtok(NULL,","),sizeof(float));
        
        output[i].Description = strtok(NULL,"\n");
        printf(output[i].Description.c_str());
        
    }
    fclose(DataFile);
    return output;
}

// ============================================================================
bool checkForBackupFile(const char *FileName)
{
    // try to open the file to see if it is there.
    FILE *BackupFile = fopen(FileName, "rb");

    if (BackupFile == NULL) {

        return false;
    } else {

        fclose(BackupFile);
        return true;
    }
}