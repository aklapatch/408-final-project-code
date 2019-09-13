#ifndef OFFLINELOGGING_H
#define OFFLINELOGGING_H
//===-- OfflineLogging.h Data logging prototypes  ------------------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/// \file
/// Has prototypes for functions that log data that cannot be sent to a database

#include "BoardConfig.h"

#include <vector>

#include <cstdio>
#include <cstring>

#include "Structs.h"
/// The maximum number of characters for a line in a file when getting backup
/// data.
#define LINESIZE 128

using namespace std;

/// deletes data entries from the file listed
/// Returns false if the file was, or should be deleted (no entries are there),
/// returns true if there is more data left. more data to send. This function
/// will delete the backup data file
bool deleteDataEntry(BoardSpecs &Specs, const char *FileName);

/// Writes the sensor data to a file.
/// It appends data if the file exists, and makes the file if it does not exist
void dumpSensorDataToFile(BoardSpecs &Specs, const char *FileName);

/// Returns a single sensor reading from the file. That includes one sample from
/// every sensor. The size of the vector is the same size as the number of
/// active ports in Specs
vector<PortInfo> getSensorDataFromFile(BoardSpecs &Specs, const char *FileName);

/// Returns true if FileName exists in the current filesystem.
/// A full file path may be necessary for this function to work.
bool checkForBackupFile(const char *FileName);

#endif // OFFLINELOGGING
