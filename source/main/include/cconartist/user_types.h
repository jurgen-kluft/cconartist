#ifndef __CCONARTIST_DECODER_USER_TYPE_H__
#define __CCONARTIST_DECODER_USER_TYPE_H__

#include "cconartist/value_type.h"
#include "cconartist/value_unit.h"

namespace nusertype
{
    typedef unsigned char value_t;
    enum enum_t
    {
        ID_UNKNOWN           = 0,   // Unknown
        ID_TEMPERATURE       = 1,   // Temperature
        ID_HUMIDITY          = 2,   // Humidity
        ID_PRESSURE          = 3,   // Pressure
        ID_LIGHT             = 4,   // Light
        ID_UV                = 5,   // UV
        ID_CO                = 6,   // Carbon Monoxide
        ID_CO2               = 7,   // Carbon Dioxide
        ID_HCHO              = 8,   // Formaldehyde
        ID_VOC               = 9,   // Volatile Organic Compounds
        ID_NOX               = 10,  // Nitrogen Oxides
        ID_PM005             = 11,  // Particulate Matter 0.5
        ID_PM010             = 12,  // Particulate Matter 1.0
        ID_PM025             = 13,  // Particulate Matter 2.5
        ID_PM040             = 14,  // Particulate Matter 4.0
        ID_PM100             = 15,  // Particulate Matter 10.0
        ID_NOISE             = 16,  // Noise
        ID_VIBRATION         = 17,  // Vibration
        ID_STATE             = 18,  // State
        ID_BATTERY           = 19,  // Battery
        ID_SWITCH1           = 21,  // On/Off, Open/Close (same as ID_SWITCH)
        ID_SWITCH2           = 22,  // On/Off, Open/Close
        ID_SWITCH3           = 23,  // On/Off, Open/Close
        ID_SWITCH4           = 24,  // On/Off, Open/Close
        ID_SWITCH5           = 25,  // On/Off, Open/Close
        ID_SWITCH6           = 26,  // On/Off, Open/Close
        ID_SWITCH7           = 27,  // On/Off, Open/Close
        ID_SWITCH8           = 28,  // On/Off, Open/Close
        ID_PRESENCE1         = 31,  // Presence1
        ID_PRESENCE2         = 32,  // Presence2
        ID_PRESENCE3         = 33,  // Presence3
        ID_DISTANCE1         = 34,  // Distance1
        ID_DISTANCE2         = 35,  // Distance2
        ID_DISTANCE3         = 36,  // Distance3
        ID_PX                = 37,  // X
        ID_PY                = 38,  // Y
        ID_PZ                = 39,  // Z
        ID_RSSI              = 40,  // RSSI
        ID_PERF1             = 41,  // Performance Metric 1
        ID_PERF2             = 42,  // Performance Metric 2
        ID_PERF3             = 43,  // Performance Metric 3
        ID_VOLTAGE           = 45,  // Voltage
        ID_CURRENT           = 46,  // Current
        ID_POWER             = 47,  // Power
        ID_ENERGY            = 48,  // Energy
        ID_SENSOR            = 49,  // Sensor
        ID_JSON              = 50,  // JSON Data
        ID_IMAGE             = 51,  // Image Data
        ID_GAS_METER         = 52,  // Gas Meter
        ID_WATER_METER       = 53,  // Water Meter
        ID_ELECTRICITY_METER = 54,  // Electric Meter
        ID_COUNT,                   // The maximum number of user id's (highest index + 1)
    };

    struct string_t
    {
        const char* str;
        enum_t      type;
    };
}  // namespace nusertype

namespace nstreamtype
{
    typedef unsigned char value_t;
    enum enum_t
    {
        TypeUnknown  = 0,
        TypeU8       = 0x1,
        TypeU16      = 0x2,
        TypeU32      = 0x3,
        TypeS8       = 0x11,
        TypeS16      = 0x12,
        TypeS32      = 0x13,
        TypeF32      = 0x15,
        TypeF64      = 0x25,
        TypeFixed    = 0x20,
        TypeVariable = 0x23,
    };
}  // namespace nstreamtype

// Note: array string array is sorted
const nusertype::enum_t* get_user_type_type_array();
const char**             get_user_type_key_string_array();
const char**             get_user_type_ui_string_array();
const short*             get_user_type_to_index_array();

const char*         to_ui_string(nusertype::enum_t type);                       // UI readable string
const char*         to_key_string(nusertype::enum_t type);                      // Key string for serialization / deserialization
nusertype::enum_t   from_string(const char* key_str);                           // Get nusertype::enum_t from key string
nusertype::enum_t   from_string(const char* key_str, const char* key_str_end);  // Get nusertype::enum_t from key string
nvaluetype::enum_t  get_value_type(nusertype::enum_t type);
nunit::enum_t       get_value_unit(nusertype::enum_t type);
nstreamtype::enum_t get_stream_type(nusertype::enum_t type);

#endif  // __CCONARTIST_DECODER_USER_TYPE_H__
