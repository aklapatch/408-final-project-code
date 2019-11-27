#ifndef BOARDCONFIG_H
#define BOARDCONFIG_H
/// \file
/// \brief Has the function prototypes for functions that set the board's
/// configuration.

#include "Networking.h"
#include "Structs.h"
#include "mbed.h"

// C headers
//
#include <cstdlib>
#include <cstring>

using namespace std;

/// Arbitrary length for char buffers
#define BUFFLEN 1024

/// Prints out most of the values of the member variables in Specs
/// This excludes the port configuration values.
void printSpecs(BoardSpecs &Specs);

/// Tries to open the specified file to read and return the configuration.
/// \returns The board's configuration
/// \sa BoardSpecs
BoardSpecs readSDCard(const char *FileName);

/// Reads the fp and derives the board configuration from that file.
/// This function gets the Board's ID, network SSID, network password, and
/// database table name from the file pointer. In addition to that, it reads the
/// number of ports and their type. All of that data is returned in a struct.
/// \param fp The file pointer to read
/// \returns The board's configuration
/// \sa BoardSpecs
BoardSpecs readConfigText(FILE *fp);


#endif // BOARDCONFIG
