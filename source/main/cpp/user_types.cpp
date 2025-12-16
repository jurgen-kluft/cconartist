#include "cconartist/user_types.h"

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
static EValueType sUserTypeValueType[nusertype::ID_COUNT] = {
    [nusertype::ID_UNKNOWN] = ValueType_U8,
    [nusertype::ID_TEMPERATURE] = ValueType_S16,
    [nusertype::ID_HUMIDITY] = ValueType_U8,
    [nusertype::ID_PRESSURE] = ValueType_U32,
    [nusertype::ID_LIGHT] = ValueType_U32,
    [nusertype::ID_UV] = ValueType_U8,
    [nusertype::ID_CO] = ValueType_U16,
    [nusertype::ID_CO2] = ValueType_U16,
    [nusertype::ID_HCHO] = ValueType_U16,
    [nusertype::ID_VOC] = ValueType_U16,
    [nusertype::ID_NOX] = ValueType_U16,
    [nusertype::ID_PM005] = ValueType_U16,
    [nusertype::ID_PM010] = ValueType_U16,
    [nusertype::ID_PM025] = ValueType_U16,
    [nusertype::ID_PM040] = ValueType_U16,
    [nusertype::ID_PM100] = ValueType_U16,
    [nusertype::ID_NOISE] = ValueType_U8,
    [nusertype::ID_VIBRATION] = ValueType_U16,
    [nusertype::ID_STATE] = ValueType_U8,
    [nusertype::ID_BATTERY] = ValueType_U8,
    [nusertype::ID_SWITCH1] = ValueType_U8,
    [nusertype::ID_SWITCH2] = ValueType_U8,
    [nusertype::ID_SWITCH3] = ValueType_U8,
    [nusertype::ID_SWITCH4] = ValueType_U8,
    [nusertype::ID_SWITCH5] = ValueType_U8,
    [nusertype::ID_SWITCH6] = ValueType_U8,
    [nusertype::ID_SWITCH7] = ValueType_U8,
    [nusertype::ID_SWITCH8] = ValueType_U8,
    [nusertype::ID_PRESENCE1] = ValueType_U8,
    [nusertype::ID_PRESENCE2] = ValueType_U8,
    [nusertype::ID_PRESENCE3] = ValueType_U8,
    [nusertype::ID_DISTANCE1] = ValueType_U16,
    [nusertype::ID_DISTANCE2] = ValueType_U16,
    [nusertype::ID_DISTANCE3] = ValueType_U16,
    [nusertype::ID_PX] = ValueType_S32,
    [nusertype::ID_PY] = ValueType_S32,
    [nusertype::ID_PZ] = ValueType_S32,
    [nusertype::ID_RSSI] = ValueType_S8,
    [nusertype::ID_PERF1] = ValueType_U32,
    [nusertype::ID_PERF2] = ValueType_U32,
    [nusertype::ID_PERF3] = ValueType_U32,
    [nusertype::ID_VOLTAGE] = ValueType_U16,
    [nusertype::ID_CURRENT] = ValueType_U16,
    [nusertype::ID_POWER] = ValueType_U32,
    [nusertype::ID_ENERGY] = ValueType_U32,
    [nusertype::ID_SENSOR] = ValueType_BinaryData,
    [nusertype::ID_JSON] = ValueType_TextData,
    [nusertype::ID_IMAGE] = ValueType_BinaryData,
    [nusertype::ID_GAS_METER] = ValueType_F64,
    [nusertype::ID_WATER_METER] = ValueType_F64,
    [nusertype::ID_ELECTRICITY_METER] = ValueType_F64,
};
// clang-format on

EValueType get_value_type(nusertype::enum_t type)
{
    if (type >= 0 && type < nusertype::ID_COUNT)
    {
        return sUserTypeValueType[type];
    }
    return ValueType_U8;
}

// clang-format off
static EValueUnit sUserTypeToUnit[nusertype::ID_COUNT] = {
  [nusertype::ID_UNKNOWN]           = UUnknown,
  [nusertype::ID_TEMPERATURE]       = UTemperature,
  [nusertype::ID_HUMIDITY]          = UPercent,
  [nusertype::ID_PRESSURE]          = UPascal,
  [nusertype::ID_LIGHT]             = ULux,
  [nusertype::ID_UV]                = UUvIndex,
  [nusertype::ID_CO]                = UPpm,
  [nusertype::ID_CO2]               = UPpm,
  [nusertype::ID_HCHO]              = UPpm,
  [nusertype::ID_VOC]               = UPpm,
  [nusertype::ID_NOX]               = UPpm,
  [nusertype::ID_PM005]             = UPpm,
  [nusertype::ID_PM010]             = UPpm,
  [nusertype::ID_PM025]             = UPpm,
  [nusertype::ID_PM040]             = UPpm,
  [nusertype::ID_PM100]             = UPpm,
  [nusertype::ID_NOISE]             = UDecibels,
  [nusertype::ID_VIBRATION]         = UHertz,
  [nusertype::ID_STATE]             = UTrueFalse,
  [nusertype::ID_SWITCH1]           = UOnOff,
  [nusertype::ID_SWITCH2]           = UOnOff,
  [nusertype::ID_SWITCH3]           = UOnOff,
  [nusertype::ID_SWITCH4]           = UOnOff,
  [nusertype::ID_SWITCH5]           = UOnOff,
  [nusertype::ID_SWITCH6]           = UOnOff,
  [nusertype::ID_SWITCH7]           = UOnOff,
  [nusertype::ID_SWITCH8]           = UOnOff,
  [nusertype::ID_PRESENCE1]         = UPresentAbsent,
  [nusertype::ID_PRESENCE2]         = UPresentAbsent,
  [nusertype::ID_PRESENCE3]         = UPresentAbsent,
  [nusertype::ID_DISTANCE1]         = UMeters,
  [nusertype::ID_DISTANCE2]         = UMeters,
  [nusertype::ID_DISTANCE3]         = UMeters,
  [nusertype::ID_PX]                = UMeters,
  [nusertype::ID_PY]                = UMeters,
  [nusertype::ID_PZ]                = UMeters,
  [nusertype::ID_RSSI]              = UdBm,
  [nusertype::ID_BATTERY]           = UPercent,
  [nusertype::ID_PERF1]             = UBinaryData,
  [nusertype::ID_PERF2]             = UBinaryData,
  [nusertype::ID_PERF3]             = UBinaryData,
  [nusertype::ID_VOLTAGE]           = UMilliVolt,
  [nusertype::ID_CURRENT]           = UMilliAmpere,
  [nusertype::ID_POWER]             = UWatts,
  [nusertype::ID_ENERGY]            = UKiloWattHour,
  [nusertype::ID_SENSOR]            = UBinaryData,
  [nusertype::ID_JSON]              = UBinaryData,
  [nusertype::ID_IMAGE]             = UBinaryData,
  [nusertype::ID_GAS_METER]         = UCubicMeters,
  [nusertype::ID_WATER_METER]       = UCubicMeters,
  [nusertype::ID_ELECTRICITY_METER] = UKiloWattHour,
};
// clang-format on

// Get the unit for a given sensor type
EValueUnit get_value_unit(nusertype::enum_t type)
{
    if (type >= 0 && type < nusertype::ID_COUNT)
        return sUserTypeToUnit[type];
    return UUnknown;
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
