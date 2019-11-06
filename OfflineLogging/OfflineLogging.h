#ifndef OFFLINELOGGING_H
#define OFFLINELOGGING_H
/// \file
/// \brief Has prototypes for functions that log data that cannot be sent to a
/// database

#include "BoardConfig.h"

#include <vector>

#include <cstdio>
#include <cstring>

#include "Structs.h"
/// The maximum number of characters for a line in a file when getting backup
/// data.
#define LINESIZE 128

using namespace std;

/// Deletes data entries store in \ref FileName
/// It deletes N entries where N is the number of ports currently active.
/// It also deletes the backup file when no entries are left.
bool deleteDataEntry(BoardSpecs &Specs, const char *FileName);

/// Writes the sensor data in Specs to a file.
/// It appends data if the file exists, and makes the file if it does not exist
void dumpSensorDataToFile(BoardSpecs &Specs, const char *FileName);

/// Returns a single sensor reading from the file. That includes one sample from
/// every active sensor. The size of the vector is the same size as the number
/// of active ports in Specs
vector<PortInfo> getSensorDataFromFile(BoardSpecs &Specs, const char *FileName);

/// Returns true if FileName exists in the current filesystem.
/// A full file path may be necessary for this function to work.
bool checkForBackupFile(const char *FileName);

#endif // OFFLINELOGGING
