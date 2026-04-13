#include "cconartist/user_types.h"
#include "cconartist/types.h"

#include <string.h>  // for NULL

// clang-format off
static const char* sUserTypeUiStrings[] = {
   "Battery",
   "Carbon Dioxide",
   "Carbon Monoxide",
   "Current",
   "Distance1",
   "Distance2",
   "Distance3",
   "ElectricityMeter",
   "Energy",
   "Formaldehyde",
   "GasMeter",
   "Humidity",
   "Image",
   "JSON",
   "Light",
   "Nitrogen Oxides",
   "Noise",
   "Performance Metric 1",
   "Performance Metric 2",
   "Performance Metric 3",
   "PM 0.5",
   "PM 1.0",
   "PM 10.0",
   "PM 2.5",
   "PM 4.0",
   "Power",
   "Presence1",
   "Presence2",
   "Presence3",
   "Pressure",
   "PX",
   "PY",
   "PZ",
   "RSSI",
   "Sensor Packet",
   "State",
   "Switch1",
   "Switch2",
   "Switch3",
   "Switch4",
   "Switch5",
   "Switch6",
   "Switch7",
   "Switch8",
   "Temperature",
   "Unknown",
   "UV",
   "Vibration",
   "VOC",
   "Voltage",
   "WaterMeter",
};
// clang-format on

// clang-format off
static const char* sUserTypeKeyStrings[] = {
   "battery",
   "co",
   "co2",
   "current",
   "distance1",
   "distance2",
   "distance3",
   "electricity_meter",
   "energy",
   "gas_meter",
   "hcho",
   "humidity",
   "image",
   "json",
   "light",
   "noise",
   "nox",
   "perf1",
   "perf2",
   "perf3",
   "pm0.5",
   "pm1.0",
   "pm10.0",
   "pm2.5",
   "pm4.0",
   "power",
   "presence1",
   "presence2",
   "presence3",
   "pressure",
   "px",
   "py",
   "pz",
   "rssi",
   "sensor",
   "state",
   "switch1",
   "switch2",
   "switch3",
   "switch4",
   "switch5",
   "switch6",
   "switch7",
   "switch8",
   "temperature",
   "unknown",
   "uv",
   "vibration",
   "voc",
   "voltage",
   "water_meter",
};
// clang-format on

// Sorted array of ndata::enum_t according to sUserTypeStrings
// clang-format off
static const ndata::enum_t sUserTypeTypes[] = {
    ndata::ID_BATTERY            ,
    ndata::ID_CO                 ,
    ndata::ID_CO2                ,
    ndata::ID_CURRENT            ,
    ndata::ID_DISTANCE1          ,
    ndata::ID_DISTANCE2          ,
    ndata::ID_DISTANCE3          ,
    ndata::ID_kWAh               ,
    ndata::ID_ENERGY             ,
    ndata::ID_GAS_M3             ,
    ndata::ID_HCHO               ,
    ndata::ID_HUMIDITY           ,
    ndata::ID_IMAGE              ,
    ndata::ID_JSON               ,
    ndata::ID_LIGHT              ,
    ndata::ID_NOISE              ,
    ndata::ID_NOX                ,
    ndata::ID_PERF1              ,
    ndata::ID_PERF2              ,
    ndata::ID_PERF3              ,
    ndata::ID_PM005              ,
    ndata::ID_PM010              ,
    ndata::ID_PM100              ,
    ndata::ID_PM025              ,
    ndata::ID_PM040              ,
    ndata::ID_POWER              ,
    ndata::ID_PRESENCE1          ,
    ndata::ID_PRESENCE2          ,
    ndata::ID_PRESENCE3          ,
    ndata::ID_PRESSURE           ,
    ndata::ID_PX                 ,
    ndata::ID_PY                 ,
    ndata::ID_PZ                 ,
    ndata::ID_RSSI               ,
    ndata::ID_SENSOR             ,
    ndata::ID_STATE              ,
    ndata::ID_SWITCH1            ,
    ndata::ID_SWITCH2            ,
    ndata::ID_SWITCH3            ,
    ndata::ID_SWITCH4            ,
    ndata::ID_SWITCH5            ,
    ndata::ID_SWITCH6            ,
    ndata::ID_SWITCH7            ,
    ndata::ID_SWITCH8            ,
    ndata::ID_TEMPERATURE        ,
    ndata::ID_UNKNOWN            ,
    ndata::ID_UV                 ,
    ndata::ID_VIBRATION          ,
    ndata::ID_VOC                ,
    ndata::ID_VOLTAGE            ,
    ndata::ID_WATER_M3           ,
};
// clang-format on

// The index here maps into the sorted arrays above
// clang-format off
static const short sUserTypeIndices[] = {
    [ndata::ID_BATTERY]                 = 19,
    [ndata::ID_CO]                      = 6,
    [ndata::ID_CO2]                     = 7,
    [ndata::ID_CURRENT]                 = 43,
    [ndata::ID_DISTANCE1]               = 32,
    [ndata::ID_DISTANCE2]               = 33,
    [ndata::ID_DISTANCE3]               = 34,
    [ndata::ID_kWAh]                    = 51,
    [ndata::ID_ENERGY]                  = 45,
    [ndata::ID_GAS_M3]                  = 49,
    [ndata::ID_HCHO]                    = 8,
    [ndata::ID_HUMIDITY]                = 2,
    [ndata::ID_IMAGE]                   = 48,
    [ndata::ID_JSON]                    = 47,
    [ndata::ID_LIGHT]                   = 4,
    [ndata::ID_NOISE]                   = 16,
    [ndata::ID_NOX]                     = 10,
    [ndata::ID_PERF1]                   = 39,
    [ndata::ID_PERF2]                   = 40,
    [ndata::ID_PERF3]                   = 41,
    [ndata::ID_PM005]                   = 11,
    [ndata::ID_PM010]                   = 12,
    [ndata::ID_PM100]                   = 15,
    [ndata::ID_PM025]                   = 13,
    [ndata::ID_PM040]                   = 14,
    [ndata::ID_POWER]                   = 44,
    [ndata::ID_PRESENCE1]               = 29,
    [ndata::ID_PRESENCE2]               = 30,
    [ndata::ID_PRESENCE3]               = 31,
    [ndata::ID_PRESSURE]                = 3,
    [ndata::ID_PX]                      = 35,
    [ndata::ID_PY]                      = 36,
    [ndata::ID_PZ]                      = 37,
    [ndata::ID_RSSI]                    = 38,
    [ndata::ID_SENSOR]                  = 46,
    [ndata::ID_STATE]                   = 18,
    [ndata::ID_SWITCH1]                 = 21,
    [ndata::ID_SWITCH2]                 = 22,
    [ndata::ID_SWITCH3]                 = 23,
    [ndata::ID_SWITCH4]                 = 24,
    [ndata::ID_SWITCH5]                 = 25,
    [ndata::ID_SWITCH6]                 = 26,
    [ndata::ID_SWITCH7]                 = 27,
    [ndata::ID_SWITCH8]                 = 28,
    [ndata::ID_TEMPERATURE]             = 1,
    [ndata::ID_UNKNOWN]                 = 0,
    [ndata::ID_UV]                      = 5,
    [ndata::ID_VIBRATION]               = 17,
    [ndata::ID_VOC]                     = 9,
    [ndata::ID_VOLTAGE]                 = 42,
    [ndata::ID_WATER_M3]                = 50,
};
// clang-format on

const ndata::enum_t* get_user_type_type_array() { return sUserTypeTypes; }
const char**         get_user_type_key_string_array() { return sUserTypeKeyStrings; }
const char**         get_user_type_ui_string_array() { return sUserTypeUiStrings; }
const short*         get_user_type_to_index_array() { return sUserTypeIndices; }

const char* to_ui_string(ndata::enum_t type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < ndata::ID_COUNT && sUserTypeUiStrings[index] != nullptr)
        return sUserTypeUiStrings[index];
    return "Unknown";
}

const char* to_key_string(ndata::enum_t type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < ndata::ID_COUNT && sUserTypeKeyStrings[index] != nullptr)
        return sUserTypeKeyStrings[index];
    return "unknown";
}

ndata::enum_t from_string(const char* key_str)
{
    // find string end of key_str
    const char* key_str_end = key_str;
    while (*key_str_end != '\0')
    {
        key_str_end++;
    }
    return from_string(key_str, key_str_end);
}

ndata::enum_t from_string(const char* key_str, const char* key_str_end)
{
    const int len = key_str_end - key_str;

    // We can binary search because the array is sorted
    int left  = 0;
    int right = ndata::ID_COUNT - 1;
    while (left <= right)
    {
        const int mid = left + (right - left) / 2;
        int       cmp = strncmp(key_str, sUserTypeKeyStrings[mid], len);
        if (cmp == 0 && (int)strlen(sUserTypeKeyStrings[mid]) == len)
        {
            return (ndata::enum_t)sUserTypeTypes[mid];
        }
        else if (cmp < 0)
        {
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return ndata::ID_UNKNOWN;
}

// clang-format off
static nvalue::enum_t sUserTypeValueType[ndata::ID_COUNT] = {
    [ndata::ID_UNKNOWN]          = nvalue::TypeU8,
    [ndata::ID_TEMPERATURE]      = nvalue::TypeS16,
    [ndata::ID_HUMIDITY]         = nvalue::TypeU8,
    [ndata::ID_PRESSURE]         = nvalue::TypeU32,
    [ndata::ID_LIGHT]            = nvalue::TypeU32,
    [ndata::ID_UV]               = nvalue::TypeU8,
    [ndata::ID_CO]               = nvalue::TypeU16,
    [ndata::ID_CO2]              = nvalue::TypeU16,
    [ndata::ID_HCHO]             = nvalue::TypeU16,
    [ndata::ID_VOC]              = nvalue::TypeU16,
    [ndata::ID_NOX]              = nvalue::TypeU16,
    [ndata::ID_PM005]            = nvalue::TypeU16,
    [ndata::ID_PM010]            = nvalue::TypeU16,
    [ndata::ID_PM025]            = nvalue::TypeU16,
    [ndata::ID_PM040]            = nvalue::TypeU16,
    [ndata::ID_PM100]            = nvalue::TypeU16,
    [ndata::ID_NOISE]            = nvalue::TypeU8,
    [ndata::ID_VIBRATION]        = nvalue::TypeU16,
    [ndata::ID_STATE]            = nvalue::TypeU8,
    [ndata::ID_BATTERY]          = nvalue::TypeU8,
    [ndata::ID_SWITCH1]          = nvalue::TypeU8,
    [ndata::ID_SWITCH2]          = nvalue::TypeU8,
    [ndata::ID_SWITCH3]          = nvalue::TypeU8,
    [ndata::ID_SWITCH4]          = nvalue::TypeU8,
    [ndata::ID_SWITCH5]          = nvalue::TypeU8,
    [ndata::ID_SWITCH6]          = nvalue::TypeU8,
    [ndata::ID_SWITCH7]          = nvalue::TypeU8,
    [ndata::ID_SWITCH8]          = nvalue::TypeU8,
    [ndata::ID_PRESENCE1]        = nvalue::TypeU8,
    [ndata::ID_PRESENCE2]        = nvalue::TypeU8,
    [ndata::ID_PRESENCE3]        = nvalue::TypeU8,
    [ndata::ID_DISTANCE1]        = nvalue::TypeU16,
    [ndata::ID_DISTANCE2]        = nvalue::TypeU16,
    [ndata::ID_DISTANCE3]        = nvalue::TypeU16,
    [ndata::ID_PX]               = nvalue::TypeS32,
    [ndata::ID_PY]               = nvalue::TypeS32,
    [ndata::ID_PZ]               = nvalue::TypeS32,
    [ndata::ID_RSSI]             = nvalue::TypeS8,
    [ndata::ID_PERF1]            = nvalue::TypeU32,
    [ndata::ID_PERF2]            = nvalue::TypeU32,
    [ndata::ID_PERF3]            = nvalue::TypeU32,
    [ndata::ID_VOLTAGE]          = nvalue::TypeU16,
    [ndata::ID_CURRENT]          = nvalue::TypeU16,
    [ndata::ID_POWER]            = nvalue::TypeU32,
    [ndata::ID_ENERGY]           = nvalue::TypeU32,
    [ndata::ID_SENSOR]           = nvalue::TypeData,
    [ndata::ID_JSON]             = nvalue::TypeString,
    [ndata::ID_IMAGE]            = nvalue::TypeData,
    [ndata::ID_GAS_M3]           = nvalue::TypeF64,
    [ndata::ID_WATER_M3]         = nvalue::TypeF64,
    [ndata::ID_kWAh]             = nvalue::TypeF64,
};
// clang-format on

nvalue::enum_t get_value_type(ndata::enum_t type)
{
    if (type >= 0 && type < ndata::ID_COUNT)
    {
        return sUserTypeValueType[type];
    }
    return nvalue::TypeU8;
}

// clang-format off
static nunit::enum_t sUserTypeToUnit[ndata::ID_COUNT] = {
  [ndata::ID_UNKNOWN]           = nunit::UUnknown,
  [ndata::ID_TEMPERATURE]       = nunit::UTemperature,
  [ndata::ID_HUMIDITY]          = nunit::UPercent,
  [ndata::ID_PRESSURE]          = nunit::UPascal,
  [ndata::ID_LIGHT]             = nunit::ULux,
  [ndata::ID_UV]                = nunit::UUvIndex,
  [ndata::ID_CO]                = nunit::UPpm,
  [ndata::ID_CO2]               = nunit::UPpm,
  [ndata::ID_HCHO]              = nunit::Uugm3,
  [ndata::ID_VOC]               = nunit::Uugm3,
  [ndata::ID_NOX]               = nunit::UPpb,
  [ndata::ID_PM005]             = nunit::Uugm3,
  [ndata::ID_PM010]             = nunit::UPpm,
  [ndata::ID_PM025]             = nunit::UPpm,
  [ndata::ID_PM040]             = nunit::UPpm,
  [ndata::ID_PM100]             = nunit::UPpm,
  [ndata::ID_NOISE]             = nunit::UDecibels,
  [ndata::ID_VIBRATION]         = nunit::UHertz,
  [ndata::ID_STATE]             = nunit::UTrueFalse,
  [ndata::ID_SWITCH1]           = nunit::UOnOff,
  [ndata::ID_SWITCH2]           = nunit::UOnOff,
  [ndata::ID_SWITCH3]           = nunit::UOnOff,
  [ndata::ID_SWITCH4]           = nunit::UOnOff,
  [ndata::ID_SWITCH5]           = nunit::UOnOff,
  [ndata::ID_SWITCH6]           = nunit::UOnOff,
  [ndata::ID_SWITCH7]           = nunit::UOnOff,
  [ndata::ID_SWITCH8]           = nunit::UOnOff,
  [ndata::ID_PRESENCE1]         = nunit::UPresentAbsent,
  [ndata::ID_PRESENCE2]         = nunit::UPresentAbsent,
  [ndata::ID_PRESENCE3]         = nunit::UPresentAbsent,
  [ndata::ID_DISTANCE1]         = nunit::UMeters,
  [ndata::ID_DISTANCE2]         = nunit::UMeters,
  [ndata::ID_DISTANCE3]         = nunit::UMeters,
  [ndata::ID_PX]                = nunit::UMeters,
  [ndata::ID_PY]                = nunit::UMeters,
  [ndata::ID_PZ]                = nunit::UMeters,
  [ndata::ID_RSSI]              = nunit::UdBm,
  [ndata::ID_BATTERY]           = nunit::UPercent,
  [ndata::ID_PERF1]             = nunit::UBinaryData,
  [ndata::ID_PERF2]             = nunit::UBinaryData,
  [ndata::ID_PERF3]             = nunit::UBinaryData,
  [ndata::ID_VOLTAGE]           = nunit::UMilliVolt,
  [ndata::ID_CURRENT]           = nunit::UMilliAmpere,
  [ndata::ID_POWER]             = nunit::UWatts,
  [ndata::ID_ENERGY]            = nunit::UKiloWattHour,
  [ndata::ID_SENSOR]            = nunit::UBinaryData,
  [ndata::ID_JSON]              = nunit::UBinaryData,
  [ndata::ID_IMAGE]             = nunit::UBinaryData,
  [ndata::ID_GAS_M3]            = nunit::UCubicMeters,
  [ndata::ID_WATER_M3]          = nunit::UCubicMeters,
  [ndata::ID_kWAh]              = nunit::UKiloWattHour,
};
// clang-format on

// Get the unit for a given sensor type
nunit::enum_t get_value_unit(ndata::enum_t type)
{
    if (type >= 0 && type < ndata::ID_COUNT)
        return sUserTypeToUnit[type];
    return nunit::UUnknown;
}

// clang-format off
static nvalue::enum_t sDataTypeToValueType[ndata::ID_COUNT] = {
  [ndata::ID_UNKNOWN]           = nvalue::TypeUnknown,
  [ndata::ID_TEMPERATURE]       = nvalue::TypeS8,
  [ndata::ID_HUMIDITY]          = nvalue::TypeU8,
  [ndata::ID_PRESSURE]          = nvalue::TypeU16,
  [ndata::ID_LIGHT]             = nvalue::TypeU16,
  [ndata::ID_UV]                = nvalue::TypeU8,
  [ndata::ID_CO]                = nvalue::TypeU16,
  [ndata::ID_CO2]               = nvalue::TypeU16,
  [ndata::ID_HCHO]              = nvalue::TypeU16,
  [ndata::ID_VOC]               = nvalue::TypeU16,
  [ndata::ID_NOX]               = nvalue::TypeU16,
  [ndata::ID_PM005]             = nvalue::TypeU8,
  [ndata::ID_PM010]             = nvalue::TypeU8,
  [ndata::ID_PM025]             = nvalue::TypeU8,
  [ndata::ID_PM040]             = nvalue::TypeU8,
  [ndata::ID_PM100]             = nvalue::TypeU8,
  [ndata::ID_NOISE]             = nvalue::TypeU8,
  [ndata::ID_VIBRATION]         = nvalue::TypeF32,
  [ndata::ID_STATE]             = nvalue::TypeU8,
  [ndata::ID_SWITCH1]           = nvalue::TypeU8,
  [ndata::ID_SWITCH2]           = nvalue::TypeU8,
  [ndata::ID_SWITCH3]           = nvalue::TypeU8,
  [ndata::ID_SWITCH4]           = nvalue::TypeU8,
  [ndata::ID_SWITCH5]           = nvalue::TypeU8,
  [ndata::ID_SWITCH6]           = nvalue::TypeU8,
  [ndata::ID_SWITCH7]           = nvalue::TypeU8,
  [ndata::ID_SWITCH8]           = nvalue::TypeU8,
  [ndata::ID_PRESENCE1]         = nvalue::TypeU8,
  [ndata::ID_PRESENCE2]         = nvalue::TypeU8,
  [ndata::ID_PRESENCE3]         = nvalue::TypeU8,
  [ndata::ID_DISTANCE1]         = nvalue::TypeU8,
  [ndata::ID_DISTANCE2]         = nvalue::TypeU8,
  [ndata::ID_DISTANCE3]         = nvalue::TypeU8,
  [ndata::ID_PX]                = nvalue::TypeU16,
  [ndata::ID_PY]                = nvalue::TypeU16,
  [ndata::ID_PZ]                = nvalue::TypeU16,
  [ndata::ID_RSSI]              = nvalue::TypeS8,
  [ndata::ID_BATTERY]           = nvalue::TypeU8,
  [ndata::ID_PERF1]             = nvalue::TypeU16,
  [ndata::ID_PERF2]             = nvalue::TypeU16,
  [ndata::ID_PERF3]             = nvalue::TypeU16,
  [ndata::ID_VOLTAGE]           = nvalue::TypeU8,
  [ndata::ID_CURRENT]           = nvalue::TypeU8,
  [ndata::ID_POWER]             = nvalue::TypeF32,
  [ndata::ID_ENERGY]            = nvalue::TypeF64,
  [ndata::ID_SENSOR]            = nvalue::TypeData,
  [ndata::ID_JSON]              = nvalue::TypeData,
  [ndata::ID_IMAGE]             = nvalue::TypeData,
  [ndata::ID_GAS_M3]            = nvalue::TypeF64,
  [ndata::ID_WATER_M3]          = nvalue::TypeF64,
  [ndata::ID_kWAh]              = nvalue::TypeF64,
};
// clang-format on

nvalue::enum_t get_stream_type(ndata::enum_t type)
{
    if (type >= 0 && type < ndata::ID_COUNT)
        return sDataTypeToValueType[type];
    return nvalue::TypeUnknown;
}

// This table is here to help maintain the sorted order of the key strings and ui strings
// when adding new user types.

//     KEY STRING               ID                  INDEX                   UI STRING                        UNIT
// "battery"              ,  [ndata::ID_BATTERY]           = 19        ,  "Battery",                   UPercent,
// "co"                   ,  [ndata::ID_CO]                = 6         ,  "CO",                        UPpm,
// "co2"                  ,  [ndata::ID_CO2]               = 7         ,  "CO2",                       UPpm,
// "current"              ,  [ndata::ID_CURRENT]           = 43        ,  "Current",                   UMilliAmpere,
// "distance1"            ,  [ndata::ID_DISTANCE1]         = 32        ,  "Distance1",                 UMeters,
// "distance2"            ,  [ndata::ID_DISTANCE2]         = 33        ,  "Distance2",                 UMeters,
// "distance3"            ,  [ndata::ID_DISTANCE3]         = 34        ,  "Distance3",                 UMeters,
// "electricity_meter"    ,  [ndata::ID_kWAh]              = 51        ,  "Electricity Meter",         UKiloWattHour,
// "energy"               ,  [ndata::ID_ENERGY]            = 45        ,  "Energy",                    UKiloWattHour,
// "gas_meter"            ,  [ndata::ID_GAS_M3]            = 49        ,  "Gas Meter",                 UCubicMeters,
// "hcho"                 ,  [ndata::ID_HCHO]              = 8         ,  "Formaldehyde",              UPpm,
// "humidity"             ,  [ndata::ID_HUMIDITY]          = 2         ,  "Humidity",                  UPercent,
// "image"                ,  [ndata::ID_IMAGE]             = 48        ,  "Image",                     UBinaryData,
// "json"                 ,  [ndata::ID_JSON]              = 47        ,  "JSON",                      UBinaryData,
// "light"                ,  [ndata::ID_LIGHT]             = 4         ,  "Light",                     ULux,
// "noise"                ,  [ndata::ID_NOISE]             = 16        ,  "Noise",                     UDecibels,
// "nox"                  ,  [ndata::ID_NOX]               = 10        ,  "Nitrogen Oxides",           UPpm,
// "perf1"                ,  [ndata::ID_PERF1]             = 39        ,  "Perf1",                     UBinaryData,
// "perf2"                ,  [ndata::ID_PERF2]             = 40        ,  "Perf2",                     UBinaryData,
// "perf3"                ,  [ndata::ID_PERF3]             = 41        ,  "Perf3",                     UBinaryData,
// "pm0.5"                ,  [ndata::ID_PM005]             = 11        ,  "PM 0.5",                    UPpm,
// "pm1.0"                ,  [ndata::ID_PM010]             = 12        ,  "PM 1.0",                    UPpm,
// "pm10.0"               ,  [ndata::ID_PM100]             = 15        ,  "PM 10.0",                   UPpm,
// "pm2.5"                ,  [ndata::ID_PM025]             = 13        ,  "PM 2.5",                    UPpm,
// "pm4.0"                ,  [ndata::ID_PM040]             = 14        ,  "PM 4.0",                    UPpm,
// "power"                ,  [ndata::ID_POWER]             = 44        ,  "Power",                     UWatts,
// "presence1"            ,  [ndata::ID_PRESENCE1]         = 29        ,  "Presence1",                 UPresentAbsent,
// "presence2"            ,  [ndata::ID_PRESENCE2]         = 30        ,  "Presence2",                 UPresentAbsent,
// "presence3"            ,  [ndata::ID_PRESENCE3]         = 31        ,  "Presence3",                 UPresentAbsent,
// "pressure"             ,  [ndata::ID_PRESSURE]          = 3         ,  "Pressure",                  UPascal,
// "px"                   ,  [ndata::ID_PX]                = 35        ,  "Positional X",              UMeters,
// "py"                   ,  [ndata::ID_PY]                = 36        ,  "Positional Y",              UMeters,
// "pz"                   ,  [ndata::ID_PZ]                = 37        ,  "Positional Z",              UMeters,
// "rssi"                 ,  [ndata::ID_RSSI]              = 38        ,  "RSSI",                      UdBm,
// "sensor"               ,  [ndata::ID_SENSOR]            = 46        ,  "Sensor",                    UBinaryData,
// "state"                ,  [ndata::ID_STATE]             = 18        ,  "State",                     UTrueFalse,
// "switch"               ,  [ndata::ID_SWITCH]            = 20        ,  "Switch",                    UOnOff,
// "switch1"              ,  [ndata::ID_SWITCH1]           = 21        ,  "Switch1",                   UOnOff,
// "switch2"              ,  [ndata::ID_SWITCH2]           = 22        ,  "Switch2",                   UOnOff,
// "switch3"              ,  [ndata::ID_SWITCH3]           = 23        ,  "Switch3",                   UOnOff,
// "switch4"              ,  [ndata::ID_SWITCH4]           = 24        ,  "Switch4",                   UOnOff,
// "switch5"              ,  [ndata::ID_SWITCH5]           = 25        ,  "Switch5",                   UOnOff,
// "switch6"              ,  [ndata::ID_SWITCH6]           = 26        ,  "Switch6",                   UOnOff,
// "switch7"              ,  [ndata::ID_SWITCH7]           = 27        ,  "Switch7",                   UOnOff,
// "switch8"              ,  [ndata::ID_SWITCH8]           = 28        ,  "Switch8",                   UOnOff,
// "temperature"          ,  [ndata::ID_TEMPERATURE]       = 1         ,  "Temperature",               UTemperature,
// "unknown"              ,  [ndata::ID_UNKNOWN]           = 0         ,  "Unknown",                   UUnknown,
// "uv"                   ,  [ndata::ID_UV]                = 5         ,  "UV",                        UUvIndex,
// "vibration"            ,  [ndata::ID_VIBRATION]         = 17        ,  "Vibration",                 UHertz,
// "voc"                  ,  [ndata::ID_VOC]               = 9         ,  "VOC",                       UPpm,
// "voltage"              ,  [ndata::ID_VOLTAGE]           = 42        ,  "Voltage",                   UMilliVolt,
// "water_meter"          ,  [ndata::ID_WATER_M3]          = 50        ,  "Water Meter",               UCubicMeters,
