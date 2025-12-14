#ifndef __CCONARTIST_DECODER_USER_TYPE_H__
#define __CCONARTIST_DECODER_USER_TYPE_H__

#include "cconartist/value_type.h"
#include "cconartist/value_unit.h"

enum EUserType
{
    ID_UNKNOWN     = 0,   // Unknown
    ID_TEMPERATURE = 1,   // Temperature
    ID_HUMIDITY    = 2,   // Humidity
    ID_PRESSURE    = 3,   // Pressure
    ID_LIGHT       = 4,   // Light
    ID_UV          = 5,   // UV
    ID_CO          = 6,   // Carbon Monoxide
    ID_CO2         = 7,   // Carbon Dioxide
    ID_HCHO        = 8,   // Formaldehyde
    ID_VOC         = 9,   // Volatile Organic Compounds
    ID_NOX         = 10,  // Nitrogen Oxides
    ID_PM005       = 11,  // Particulate Matter 0.5
    ID_PM010       = 12,  // Particulate Matter 1.0
    ID_PM025       = 13,  // Particulate Matter 2.5
    ID_PM040       = 14,  // Particulate Matter 4.0
    ID_PM100       = 15,  // Particulate Matter 10.0
    ID_NOISE       = 16,  // Noise
    ID_VIBRATION   = 17,  // Vibration
    ID_STATE       = 18,  // State
    ID_BATTERY     = 19,  // Battery
    ID_SWITCH      = 21,  // On/Off, Open/Close
    ID_SWITCH1     = 21,  // On/Off, Open/Close (same as ID_SWITCH)
    ID_SWITCH2     = 22,  // On/Off, Open/Close
    ID_SWITCH3     = 23,  // On/Off, Open/Close
    ID_SWITCH4     = 24,  // On/Off, Open/Close
    ID_SWITCH5     = 25,  // On/Off, Open/Close
    ID_SWITCH6     = 26,  // On/Off, Open/Close
    ID_SWITCH7     = 27,  // On/Off, Open/Close
    ID_SWITCH8     = 28,  // On/Off, Open/Close
    ID_PRESENCE1   = 31,  // Presence1
    ID_PRESENCE2   = 32,  // Presence2
    ID_PRESENCE3   = 33,  // Presence3
    ID_DISTANCE1   = 34,  // Distance1
    ID_DISTANCE2   = 35,  // Distance2
    ID_DISTANCE3   = 36,  // Distance3
    ID_PX          = 37,  // X
    ID_PY          = 38,  // Y
    ID_PZ          = 39,  // Z
    ID_RSSI        = 40,  // RSSI
    ID_PERF1       = 41,  // Performance Metric 1
    ID_PERF2       = 42,  // Performance Metric 2
    ID_PERF3       = 43,  // Performance Metric 3
    ID_VOLTAGE     = 45,  // Voltage
    ID_CURRENT     = 46,  // Current
    ID_POWER       = 47,  // Power
    ID_ENERGY      = 48,  // Energy
    ID_SENSOR      = 49,  // Sensor
    ID_JSON        = 50,  // JSON Data
    ID_IMAGE       = 51,  // Image Data
    ID_COUNT,             // The maximum number of user id's (highest index + 1)
};

const char* ui_string(EUserType type); // UI readable string
const char* key_string(EUserType type); // Key string for serialization / deserialization
EUserType   from_string(const char* key_str); // Get EUserType from key string
EValueType  get_value_type(EUserType type);
EValueUnit  get_value_unit(EUserType type);

#endif  // __CCONARTIST_DECODER_USER_TYPE_H__
