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

// Sorted array of nusertype::enum_t according to sUserTypeStrings
// clang-format off
static const nusertype::enum_t sUserTypeTypes[] = {
    nusertype::ID_BATTERY            ,
    nusertype::ID_CO                 ,
    nusertype::ID_CO2                ,
    nusertype::ID_CURRENT            ,
    nusertype::ID_DISTANCE1          ,
    nusertype::ID_DISTANCE2          ,
    nusertype::ID_DISTANCE3          ,
    nusertype::ID_ELECTRICITY_METER  ,
    nusertype::ID_ENERGY             ,
    nusertype::ID_GAS_METER          ,
    nusertype::ID_HCHO               ,
    nusertype::ID_HUMIDITY           ,
    nusertype::ID_IMAGE              ,
    nusertype::ID_JSON               ,
    nusertype::ID_LIGHT              ,
    nusertype::ID_NOISE              ,
    nusertype::ID_NOX                ,
    nusertype::ID_PERF1              ,
    nusertype::ID_PERF2              ,
    nusertype::ID_PERF3              ,
    nusertype::ID_PM005              ,
    nusertype::ID_PM010              ,
    nusertype::ID_PM100              ,
    nusertype::ID_PM025              ,
    nusertype::ID_PM040              ,
    nusertype::ID_POWER              ,
    nusertype::ID_PRESENCE1          ,
    nusertype::ID_PRESENCE2          ,
    nusertype::ID_PRESENCE3          ,
    nusertype::ID_PRESSURE           ,
    nusertype::ID_PX                 ,
    nusertype::ID_PY                 ,
    nusertype::ID_PZ                 ,
    nusertype::ID_RSSI               ,
    nusertype::ID_SENSOR             ,
    nusertype::ID_STATE              ,
    nusertype::ID_SWITCH1            ,
    nusertype::ID_SWITCH2            ,
    nusertype::ID_SWITCH3            ,
    nusertype::ID_SWITCH4            ,
    nusertype::ID_SWITCH5            ,
    nusertype::ID_SWITCH6            ,
    nusertype::ID_SWITCH7            ,
    nusertype::ID_SWITCH8            ,
    nusertype::ID_TEMPERATURE        ,
    nusertype::ID_UNKNOWN            ,
    nusertype::ID_UV                 ,
    nusertype::ID_VIBRATION          ,
    nusertype::ID_VOC                ,
    nusertype::ID_VOLTAGE            ,
    nusertype::ID_WATER_METER        ,
};
// clang-format on

// The index here maps into the sorted arrays above
// clang-format off
static const short sUserTypeIndices[] = {
    [nusertype::ID_BATTERY]                 = 19,
    [nusertype::ID_CO]                      = 6,
    [nusertype::ID_CO2]                     = 7,
    [nusertype::ID_CURRENT]                 = 43,
    [nusertype::ID_DISTANCE1]               = 32,
    [nusertype::ID_DISTANCE2]               = 33,
    [nusertype::ID_DISTANCE3]               = 34,
    [nusertype::ID_ELECTRICITY_METER]       = 51,
    [nusertype::ID_ENERGY]                  = 45,
    [nusertype::ID_GAS_METER]               = 49,
    [nusertype::ID_HCHO]                    = 8,
    [nusertype::ID_HUMIDITY]                = 2,
    [nusertype::ID_IMAGE]                   = 48,
    [nusertype::ID_JSON]                    = 47,
    [nusertype::ID_LIGHT]                   = 4,
    [nusertype::ID_NOISE]                   = 16,
    [nusertype::ID_NOX]                     = 10,
    [nusertype::ID_PERF1]                   = 39,
    [nusertype::ID_PERF2]                   = 40,
    [nusertype::ID_PERF3]                   = 41,
    [nusertype::ID_PM005]                   = 11,
    [nusertype::ID_PM010]                   = 12,
    [nusertype::ID_PM100]                   = 15,
    [nusertype::ID_PM025]                   = 13,
    [nusertype::ID_PM040]                   = 14,
    [nusertype::ID_POWER]                   = 44,
    [nusertype::ID_PRESENCE1]               = 29,
    [nusertype::ID_PRESENCE2]               = 30,
    [nusertype::ID_PRESENCE3]               = 31,
    [nusertype::ID_PRESSURE]                = 3,
    [nusertype::ID_PX]                      = 35,
    [nusertype::ID_PY]                      = 36,
    [nusertype::ID_PZ]                      = 37,
    [nusertype::ID_RSSI]                    = 38,
    [nusertype::ID_SENSOR]                  = 46,
    [nusertype::ID_STATE]                   = 18,
    [nusertype::ID_SWITCH1]                 = 21,
    [nusertype::ID_SWITCH2]                 = 22,
    [nusertype::ID_SWITCH3]                 = 23,
    [nusertype::ID_SWITCH4]                 = 24,
    [nusertype::ID_SWITCH5]                 = 25,
    [nusertype::ID_SWITCH6]                 = 26,
    [nusertype::ID_SWITCH7]                 = 27,
    [nusertype::ID_SWITCH8]                 = 28,
    [nusertype::ID_TEMPERATURE]             = 1,
    [nusertype::ID_UNKNOWN]                 = 0,
    [nusertype::ID_UV]                      = 5,
    [nusertype::ID_VIBRATION]               = 17,
    [nusertype::ID_VOC]                     = 9,
    [nusertype::ID_VOLTAGE]                 = 42,
    [nusertype::ID_WATER_METER]             = 50,
};
// clang-format on

const nusertype::enum_t* get_user_type_type_array() { return sUserTypeTypes; }
const char**             get_user_type_key_string_array() { return sUserTypeKeyStrings; }
const char**             get_user_type_ui_string_array() { return sUserTypeUiStrings; }
const short*             get_user_type_to_index_array() { return sUserTypeIndices; }

const char* to_ui_string(nusertype::enum_t type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < nusertype::ID_COUNT && sUserTypeUiStrings[index] != nullptr)
        return sUserTypeUiStrings[index];
    return "Unknown";
}

const char* to_key_string(nusertype::enum_t type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < nusertype::ID_COUNT && sUserTypeKeyStrings[index] != nullptr)
        return sUserTypeKeyStrings[index];
    return "unknown";
}

nusertype::enum_t from_string(const char* key_str)
{
    // find string end of key_str
    const char* key_str_end = key_str;
    while (*key_str_end != '\0')
    {
        key_str_end++;
    }
    return from_string(key_str, key_str_end);
}

nusertype::enum_t from_string(const char* key_str, const char* key_str_end)
{
    const int len = key_str_end - key_str;

    // We can binary search because the array is sorted
    int left  = 0;
    int right = nusertype::ID_COUNT - 1;
    while (left <= right)
    {
        const int mid = left + (right - left) / 2;
        int       cmp = strncmp(key_str, sUserTypeKeyStrings[mid], len);
        if (cmp == 0 && (int)strlen(sUserTypeKeyStrings[mid]) == len)
        {
            return (nusertype::enum_t)sUserTypeTypes[mid];
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
    return nusertype::ID_UNKNOWN;
}

// clang-format off
static nvaluetype::enum_t sUserTypeValueType[nusertype::ID_COUNT] = {
    [nusertype::ID_UNKNOWN] = nvaluetype::TypeU8,
    [nusertype::ID_TEMPERATURE] = nvaluetype::TypeS16,
    [nusertype::ID_HUMIDITY] = nvaluetype::TypeU8,
    [nusertype::ID_PRESSURE] = nvaluetype::TypeU32,
    [nusertype::ID_LIGHT] = nvaluetype::TypeU32,
    [nusertype::ID_UV] = nvaluetype::TypeU8,
    [nusertype::ID_CO] = nvaluetype::TypeU16,
    [nusertype::ID_CO2] = nvaluetype::TypeU16,
    [nusertype::ID_HCHO] = nvaluetype::TypeU16,
    [nusertype::ID_VOC] = nvaluetype::TypeU16,
    [nusertype::ID_NOX] = nvaluetype::TypeU16,
    [nusertype::ID_PM005] = nvaluetype::TypeU16,
    [nusertype::ID_PM010] = nvaluetype::TypeU16,
    [nusertype::ID_PM025] = nvaluetype::TypeU16,
    [nusertype::ID_PM040] = nvaluetype::TypeU16,
    [nusertype::ID_PM100] = nvaluetype::TypeU16,
    [nusertype::ID_NOISE] = nvaluetype::TypeU8,
    [nusertype::ID_VIBRATION] = nvaluetype::TypeU16,
    [nusertype::ID_STATE] = nvaluetype::TypeU8,
    [nusertype::ID_BATTERY] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH1] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH2] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH3] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH4] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH5] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH6] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH7] = nvaluetype::TypeU8,
    [nusertype::ID_SWITCH8] = nvaluetype::TypeU8,
    [nusertype::ID_PRESENCE1] = nvaluetype::TypeU8,
    [nusertype::ID_PRESENCE2] = nvaluetype::TypeU8,
    [nusertype::ID_PRESENCE3] = nvaluetype::TypeU8,
    [nusertype::ID_DISTANCE1] = nvaluetype::TypeU16,
    [nusertype::ID_DISTANCE2] = nvaluetype::TypeU16,
    [nusertype::ID_DISTANCE3] = nvaluetype::TypeU16,
    [nusertype::ID_PX] = nvaluetype::TypeS32,
    [nusertype::ID_PY] = nvaluetype::TypeS32,
    [nusertype::ID_PZ] = nvaluetype::TypeS32,
    [nusertype::ID_RSSI] = nvaluetype::TypeS8,
    [nusertype::ID_PERF1] = nvaluetype::TypeU32,
    [nusertype::ID_PERF2] = nvaluetype::TypeU32,
    [nusertype::ID_PERF3] = nvaluetype::TypeU32,
    [nusertype::ID_VOLTAGE] = nvaluetype::TypeU16,
    [nusertype::ID_CURRENT] = nvaluetype::TypeU16,
    [nusertype::ID_POWER] = nvaluetype::TypeU32,
    [nusertype::ID_ENERGY] = nvaluetype::TypeU32,
    [nusertype::ID_SENSOR] = nvaluetype::TypeData,
    [nusertype::ID_JSON] = nvaluetype::TypeString,
    [nusertype::ID_IMAGE] = nvaluetype::TypeData,
    [nusertype::ID_GAS_METER] = nvaluetype::TypeF64,
    [nusertype::ID_WATER_METER] = nvaluetype::TypeF64,
    [nusertype::ID_ELECTRICITY_METER] = nvaluetype::TypeF64,
};
// clang-format on

nvaluetype::enum_t get_value_type(nusertype::enum_t type)
{
    if (type >= 0 && type < nusertype::ID_COUNT)
    {
        return sUserTypeValueType[type];
    }
    return nvaluetype::TypeU8;
}

// clang-format off
static nunit::enum_t sUserTypeToUnit[nusertype::ID_COUNT] = {
  [nusertype::ID_UNKNOWN]           = nunit::UUnknown,
  [nusertype::ID_TEMPERATURE]       = nunit::UTemperature,
  [nusertype::ID_HUMIDITY]          = nunit::UPercent,
  [nusertype::ID_PRESSURE]          = nunit::UPascal,
  [nusertype::ID_LIGHT]             = nunit::ULux,
  [nusertype::ID_UV]                = nunit::UUvIndex,
  [nusertype::ID_CO]                = nunit::UPpm,
  [nusertype::ID_CO2]               = nunit::UPpm,
  [nusertype::ID_HCHO]              = nunit::Uugm3,
  [nusertype::ID_VOC]               = nunit::Uugm3,
  [nusertype::ID_NOX]               = nunit::UPpb,
  [nusertype::ID_PM005]             = nunit::Uugm3,
  [nusertype::ID_PM010]             = nunit::UPpm,
  [nusertype::ID_PM025]             = nunit::UPpm,
  [nusertype::ID_PM040]             = nunit::UPpm,
  [nusertype::ID_PM100]             = nunit::UPpm,
  [nusertype::ID_NOISE]             = nunit::UDecibels,
  [nusertype::ID_VIBRATION]         = nunit::UHertz,
  [nusertype::ID_STATE]             = nunit::UTrueFalse,
  [nusertype::ID_SWITCH1]           = nunit::UOnOff,
  [nusertype::ID_SWITCH2]           = nunit::UOnOff,
  [nusertype::ID_SWITCH3]           = nunit::UOnOff,
  [nusertype::ID_SWITCH4]           = nunit::UOnOff,
  [nusertype::ID_SWITCH5]           = nunit::UOnOff,
  [nusertype::ID_SWITCH6]           = nunit::UOnOff,
  [nusertype::ID_SWITCH7]           = nunit::UOnOff,
  [nusertype::ID_SWITCH8]           = nunit::UOnOff,
  [nusertype::ID_PRESENCE1]         = nunit::UPresentAbsent,
  [nusertype::ID_PRESENCE2]         = nunit::UPresentAbsent,
  [nusertype::ID_PRESENCE3]         = nunit::UPresentAbsent,
  [nusertype::ID_DISTANCE1]         = nunit::UMeters,
  [nusertype::ID_DISTANCE2]         = nunit::UMeters,
  [nusertype::ID_DISTANCE3]         = nunit::UMeters,
  [nusertype::ID_PX]                = nunit::UMeters,
  [nusertype::ID_PY]                = nunit::UMeters,
  [nusertype::ID_PZ]                = nunit::UMeters,
  [nusertype::ID_RSSI]              = nunit::UdBm,
  [nusertype::ID_BATTERY]           = nunit::UPercent,
  [nusertype::ID_PERF1]             = nunit::UBinaryData,
  [nusertype::ID_PERF2]             = nunit::UBinaryData,
  [nusertype::ID_PERF3]             = nunit::UBinaryData,
  [nusertype::ID_VOLTAGE]           = nunit::UMilliVolt,
  [nusertype::ID_CURRENT]           = nunit::UMilliAmpere,
  [nusertype::ID_POWER]             = nunit::UWatts,
  [nusertype::ID_ENERGY]            = nunit::UKiloWattHour,
  [nusertype::ID_SENSOR]            = nunit::UBinaryData,
  [nusertype::ID_JSON]              = nunit::UBinaryData,
  [nusertype::ID_IMAGE]             = nunit::UBinaryData,
  [nusertype::ID_GAS_METER]         = nunit::UCubicMeters,
  [nusertype::ID_WATER_METER]       = nunit::UCubicMeters,
  [nusertype::ID_ELECTRICITY_METER] = nunit::UKiloWattHour,
};
// clang-format on

// Get the unit for a given sensor type
nunit::enum_t get_value_unit(nusertype::enum_t type)
{
    if (type >= 0 && type < nusertype::ID_COUNT)
        return sUserTypeToUnit[type];
    return nunit::UUnknown;
}

// clang-format off
static nstreamtype::enum_t sUserTypeToStreamType[nusertype::ID_COUNT] = {
  [nusertype::ID_UNKNOWN]           = nstreamtype::TypeUnknown,
  [nusertype::ID_TEMPERATURE]       = nstreamtype::TypeS8,
  [nusertype::ID_HUMIDITY]          = nstreamtype::TypeU8,
  [nusertype::ID_PRESSURE]          = nstreamtype::TypeU16,
  [nusertype::ID_LIGHT]             = nstreamtype::TypeU16,
  [nusertype::ID_UV]                = nstreamtype::TypeU8,
  [nusertype::ID_CO]                = nstreamtype::TypeU16,
  [nusertype::ID_CO2]               = nstreamtype::TypeU16,
  [nusertype::ID_HCHO]              = nstreamtype::TypeU16,
  [nusertype::ID_VOC]               = nstreamtype::TypeU16,
  [nusertype::ID_NOX]               = nstreamtype::TypeU16,
  [nusertype::ID_PM005]             = nstreamtype::TypeU8,
  [nusertype::ID_PM010]             = nstreamtype::TypeU8,
  [nusertype::ID_PM025]             = nstreamtype::TypeU8,
  [nusertype::ID_PM040]             = nstreamtype::TypeU8,
  [nusertype::ID_PM100]             = nstreamtype::TypeU8,
  [nusertype::ID_NOISE]             = nstreamtype::TypeU8,
  [nusertype::ID_VIBRATION]         = nstreamtype::TypeF32,
  [nusertype::ID_STATE]             = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH1]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH2]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH3]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH4]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH5]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH6]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH7]           = nstreamtype::TypeU8,
  [nusertype::ID_SWITCH8]           = nstreamtype::TypeU8,
  [nusertype::ID_PRESENCE1]         = nstreamtype::TypeU8,
  [nusertype::ID_PRESENCE2]         = nstreamtype::TypeU8,
  [nusertype::ID_PRESENCE3]         = nstreamtype::TypeU8,
  [nusertype::ID_DISTANCE1]         = nstreamtype::TypeU8,
  [nusertype::ID_DISTANCE2]         = nstreamtype::TypeU8,
  [nusertype::ID_DISTANCE3]         = nstreamtype::TypeU8,
  [nusertype::ID_PX]                = nstreamtype::TypeU16,
  [nusertype::ID_PY]                = nstreamtype::TypeU16,
  [nusertype::ID_PZ]                = nstreamtype::TypeU16,
  [nusertype::ID_RSSI]              = nstreamtype::TypeS8,
  [nusertype::ID_BATTERY]           = nstreamtype::TypeU8,
  [nusertype::ID_PERF1]             = nstreamtype::TypeU16,
  [nusertype::ID_PERF2]             = nstreamtype::TypeU16,
  [nusertype::ID_PERF3]             = nstreamtype::TypeU16,
  [nusertype::ID_VOLTAGE]           = nstreamtype::TypeU8,
  [nusertype::ID_CURRENT]           = nstreamtype::TypeU8,
  [nusertype::ID_POWER]             = nstreamtype::TypeF32,
  [nusertype::ID_ENERGY]            = nstreamtype::TypeF64,
  [nusertype::ID_SENSOR]            = nstreamtype::TypeVariable,
  [nusertype::ID_JSON]              = nstreamtype::TypeVariable,
  [nusertype::ID_IMAGE]             = nstreamtype::TypeVariable,
  [nusertype::ID_GAS_METER]         = nstreamtype::TypeF64,
  [nusertype::ID_WATER_METER]       = nstreamtype::TypeF64,
  [nusertype::ID_ELECTRICITY_METER] = nstreamtype::TypeF64,
};
// clang-format on


nstreamtype::enum_t get_stream_type(nusertype::enum_t type)
{
    if (type >= 0 && type < nusertype::ID_COUNT)
        return sUserTypeToStreamType[type];
    return nstreamtype::TypeUnknown;
}

// This table is here to help maintain the sorted order of the key strings and ui strings
// when adding new user types.

//     KEY STRING               ID                  INDEX                   UI STRING                        UNIT
// "battery"              ,  [nusertype::ID_BATTERY]           = 19        ,  "Battery",                   UPercent,
// "co"                   ,  [nusertype::ID_CO]                = 6         ,  "CO",                        UPpm,
// "co2"                  ,  [nusertype::ID_CO2]               = 7         ,  "CO2",                       UPpm,
// "current"              ,  [nusertype::ID_CURRENT]           = 43        ,  "Current",                   UMilliAmpere,
// "distance1"            ,  [nusertype::ID_DISTANCE1]         = 32        ,  "Distance1",                 UMeters,
// "distance2"            ,  [nusertype::ID_DISTANCE2]         = 33        ,  "Distance2",                 UMeters,
// "distance3"            ,  [nusertype::ID_DISTANCE3]         = 34        ,  "Distance3",                 UMeters,
// "electricity_meter"    ,  [nusertype::ID_ELECTRICITY_METER] = 51        ,  "Electricity Meter",         UKiloWattHour,
// "energy"               ,  [nusertype::ID_ENERGY]            = 45        ,  "Energy",                    UKiloWattHour,
// "gas_meter"            ,  [nusertype::ID_GAS_METER]         = 49        ,  "Gas Meter",                 UCubicMeters,
// "hcho"                 ,  [nusertype::ID_HCHO]              = 8         ,  "Formaldehyde",              UPpm,
// "humidity"             ,  [nusertype::ID_HUMIDITY]          = 2         ,  "Humidity",                  UPercent,
// "image"                ,  [nusertype::ID_IMAGE]             = 48        ,  "Image",                     UBinaryData,
// "json"                 ,  [nusertype::ID_JSON]              = 47        ,  "JSON",                      UBinaryData,
// "light"                ,  [nusertype::ID_LIGHT]             = 4         ,  "Light",                     ULux,
// "noise"                ,  [nusertype::ID_NOISE]             = 16        ,  "Noise",                     UDecibels,
// "nox"                  ,  [nusertype::ID_NOX]               = 10        ,  "Nitrogen Oxides",           UPpm,
// "perf1"                ,  [nusertype::ID_PERF1]             = 39        ,  "Perf1",                     UBinaryData,
// "perf2"                ,  [nusertype::ID_PERF2]             = 40        ,  "Perf2",                     UBinaryData,
// "perf3"                ,  [nusertype::ID_PERF3]             = 41        ,  "Perf3",                     UBinaryData,
// "pm0.5"                ,  [nusertype::ID_PM005]             = 11        ,  "PM 0.5",                    UPpm,
// "pm1.0"                ,  [nusertype::ID_PM010]             = 12        ,  "PM 1.0",                    UPpm,
// "pm10.0"               ,  [nusertype::ID_PM100]             = 15        ,  "PM 10.0",                   UPpm,
// "pm2.5"                ,  [nusertype::ID_PM025]             = 13        ,  "PM 2.5",                    UPpm,
// "pm4.0"                ,  [nusertype::ID_PM040]             = 14        ,  "PM 4.0",                    UPpm,
// "power"                ,  [nusertype::ID_POWER]             = 44        ,  "Power",                     UWatts,
// "presence1"            ,  [nusertype::ID_PRESENCE1]         = 29        ,  "Presence1",                 UPresentAbsent,
// "presence2"            ,  [nusertype::ID_PRESENCE2]         = 30        ,  "Presence2",                 UPresentAbsent,
// "presence3"            ,  [nusertype::ID_PRESENCE3]         = 31        ,  "Presence3",                 UPresentAbsent,
// "pressure"             ,  [nusertype::ID_PRESSURE]          = 3         ,  "Pressure",                  UPascal,
// "px"                   ,  [nusertype::ID_PX]                = 35        ,  "Positional X",              UMeters,
// "py"                   ,  [nusertype::ID_PY]                = 36        ,  "Positional Y",              UMeters,
// "pz"                   ,  [nusertype::ID_PZ]                = 37        ,  "Positional Z",              UMeters,
// "rssi"                 ,  [nusertype::ID_RSSI]              = 38        ,  "RSSI",                      UdBm,
// "sensor"               ,  [nusertype::ID_SENSOR]            = 46        ,  "Sensor",                    UBinaryData,
// "state"                ,  [nusertype::ID_STATE]             = 18        ,  "State",                     UTrueFalse,
// "switch"               ,  [nusertype::ID_SWITCH]            = 20        ,  "Switch",                    UOnOff,
// "switch1"              ,  [nusertype::ID_SWITCH1]           = 21        ,  "Switch1",                   UOnOff,
// "switch2"              ,  [nusertype::ID_SWITCH2]           = 22        ,  "Switch2",                   UOnOff,
// "switch3"              ,  [nusertype::ID_SWITCH3]           = 23        ,  "Switch3",                   UOnOff,
// "switch4"              ,  [nusertype::ID_SWITCH4]           = 24        ,  "Switch4",                   UOnOff,
// "switch5"              ,  [nusertype::ID_SWITCH5]           = 25        ,  "Switch5",                   UOnOff,
// "switch6"              ,  [nusertype::ID_SWITCH6]           = 26        ,  "Switch6",                   UOnOff,
// "switch7"              ,  [nusertype::ID_SWITCH7]           = 27        ,  "Switch7",                   UOnOff,
// "switch8"              ,  [nusertype::ID_SWITCH8]           = 28        ,  "Switch8",                   UOnOff,
// "temperature"          ,  [nusertype::ID_TEMPERATURE]       = 1         ,  "Temperature",               UTemperature,
// "unknown"              ,  [nusertype::ID_UNKNOWN]           = 0         ,  "Unknown",                   UUnknown,
// "uv"                   ,  [nusertype::ID_UV]                = 5         ,  "UV",                        UUvIndex,
// "vibration"            ,  [nusertype::ID_VIBRATION]         = 17        ,  "Vibration",                 UHertz,
// "voc"                  ,  [nusertype::ID_VOC]               = 9         ,  "VOC",                       UPpm,
// "voltage"              ,  [nusertype::ID_VOLTAGE]           = 42        ,  "Voltage",                   UMilliVolt,
// "water_meter"          ,  [nusertype::ID_WATER_METER]       = 50        ,  "Water Meter",               UCubicMeters,
