#ifndef __CCONARTIST_DECODER_USER_TYPE_H__
#define __CCONARTIST_DECODER_USER_TYPE_H__

#include "cconartist/value_type.h"
#include "cconartist/value_unit.h"

namespace ndata
{
    typedef unsigned char type_t;
    enum enum_t
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
        ID_SWITCH1     = 21,  // On/Off, Open/Close (same as ID_SWITCH)
        ID_SWITCH2     = 22,  // On/Off, Open/Close
        ID_SWITCH3     = 23,  // On/Off, Open/Close
        ID_SWITCH4     = 24,  // On/Off, Open/Close
        ID_SWITCH5     = 25,  // On/Off, Open/Close
        ID_SWITCH6     = 26,  // On/Off, Open/Close
        ID_SWITCH7     = 27,  // On/Off, Open/Close
        ID_SWITCH8     = 28,  // On/Off, Open/Close
        ID_SWITCH9     = 29,  // On/Off, Open/Close
        ID_PRESENCE1   = 51,  // Presence1
        ID_PRESENCE2   = 52,  // Presence2
        ID_PRESENCE3   = 53,  // Presence3
        ID_DISTANCE1   = 54,  // Distance1
        ID_DISTANCE2   = 55,  // Distance2
        ID_DISTANCE3   = 56,  // Distance3
        ID_PX          = 57,  // X
        ID_PY          = 58,  // Y
        ID_PZ          = 59,  // Z
        ID_RSSI        = 60,  // RSSI
        ID_PERF1       = 61,  // Performance Metric 1
        ID_PERF2       = 62,  // Performance Metric 2
        ID_PERF3       = 63,  // Performance Metric 3
        ID_VOLTAGE     = 65,  // Voltage
        ID_CURRENT     = 66,  // Current
        ID_POWER       = 67,  // Power
        ID_ENERGY      = 68,  // Energy
        ID_SENSOR      = 69,  // Sensor
        ID_JSON        = 70,  // JSON Data
        ID_IMAGE       = 71,  // Image Data
        ID_GAS_M3      = 72,  // Gas Meter
        ID_WATER_M3    = 73,  // Water Meter
        ID_kWAh        = 74,  // kWAh (electricity) Meter

        // Note: Many more user types can be added here, but we need to keep the total count under 65535

        ID_COUNT,  // The maximum number of user id's (highest index + 1)
    };

}  // namespace ndata

namespace nvalue
{
    typedef unsigned char type_t;
    enum enum_t
    {
        TypeUnknown  = 0,
        TypeU8       = 0x1,
        TypeU16      = 0x2,
        TypeU32      = 0x3,
        TypeU64      = 0x4,
        TypeS8       = 0x11,
        TypeS16      = 0x12,
        TypeS32      = 0x13,
        TypeS64      = 0x14,
        TypeF32      = 0x15,
        TypeF64      = 0x25,
        TypeData     = 0x21,
        TypeString   = 0x22,
    };
}  // namespace nvalue

// Note: array string array is sorted
const ndata::enum_t* get_data_type_array();
const char**         get_data_type_key_string_array();
const char**         get_data_type_ui_string_array();
const short*         get_data_type_to_index_array();

const char*    to_ui_string(ndata::enum_t type);                           // UI readable string
const char*    to_key_string(ndata::enum_t type);                          // Key string for serialization / deserialization
ndata::enum_t  from_string(const char* key_str);                           // Get ndata::enum_t from key string
ndata::enum_t  from_string(const char* key_str, const char* key_str_end);  // Get ndata::enum_t from key string
nvalue::enum_t get_value_type(ndata::enum_t type);
nunit::enum_t  get_value_unit(ndata::enum_t type);

#endif  // __CCONARTIST_DECODER_USER_TYPE_H__
