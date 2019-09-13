#ifndef STRUCTS_H
#define STRUCTS_H
//===-- Structs.h - Board information data structure header --------------===//
//
// Part of the IAC energy monitoring project
//
//===---------------------------------------------------------------------===//
/// \file
/// Has the structs that store the board's information.

#include "mbed.h"
#include <string>
#include <vector>
using namespace std;

/// Houses the information for each port
struct PortInfo {

    /// Name of the port (Port_1, Port_2, etc.)
    string Name;

    /// The value of the port, read from a sensor.
    float Value;

    /// Port description
    string Description;

    /// Multiplies port value to convert units
    float Multiplier;

    /// Indicates what type of sensor is attached to the port
    int SensorID;

    float RangeStart; ///< Any port reading below this is considered an error.

    float RangeEnd; ///< Any port reading above this is cosidered an error.

    /// Default Constructor.
    /// Sets all string values to "", integers to 0, and floats to 0.0
    PortInfo()
        : Name(""), Value(0.0), Description(""), Multiplier(0.0), SensorID(0),
          RangeStart(0.0), RangeEnd(0.0) {}
};

/// Stores information regarding specific sensors
struct SensorInfo {
    int ID; ///< The sensor's id integer

    string Type; ///< quatity that the sensor type measures.

    string Unit; ///< Unit the quantity is measured in

    float
        Multiplier; ///< Multiplier to convert sensor's value to specified unit

    float RangeStart; ///< Any port reading below this is considered an error.

    float RangeEnd; ///< Any port reading above this is cosidered an error.

    SensorInfo()
        : ID(0), Type("No Sensor"), Unit("No Unit"), Multiplier(0.0),
          RangeStart(0.0), RangeEnd(0) {}
};

/// Contains board properties and the ports' data and info
/// To access a port's data, use this syntax: BoardSpecs.Ports[i].Value
/// \sa PortInfo
struct BoardSpecs {

    /// Unique identifier for the Board
    string ID;

    /// Name of the WiFi network to connect to
    string NetworkSSID;

    /// Password of the above WiFi network
    string NetworkPassword;

    /// The IP address of this board
    string WiFiIP;

    /// The Database table name that this board is associated with
    string DatabaseTableName;

    /// The collection of ports and their information
    vector<PortInfo> Ports;

    /// Collection of possible sensor types.
    vector<SensorInfo> Sensors;

    /// Sets the number of ports for the board
    void setPortNum(unsigned int Num) { Ports.resize(Num); }

    /// Returns number of ports
    unsigned int getPortNum() { return Ports.size(); }

    /// Default constructor.
    /// Sets all strings to "" and sets the initializes the vector size to 0
    BoardSpecs()
        : ID(""), NetworkSSID(""), NetworkPassword(""), WiFiIP(""),
          DatabaseTableName(""), Ports() {}
};

#endif // STRUCTS
