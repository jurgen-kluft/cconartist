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
   "Energy",
   "Formaldehyde",
   "Humidity",
   "Image",
   "JSON",
   "Light",
   "NO",
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
   "X",
   "Y",
   "Z",
};
// clang-format on

// clang-format off
static const char* sUserTypeKeyStrings[] = {
   "battery",
   "co2",
   "co",
   "current",
   "distance1",
   "distance2",
   "distance3",
   "energy",
   "hcho",
   "humidity",
   "image",
   "json",
   "light",
   "nox",
   "noise",
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
   "px",
   "py",
   "pz",
};
// clang-format on

// Sorted array of EUserType according to sUserTypeStrings
// clang-format off
static const unsigned char sUserTypeTypes[] = {
    ID_BATTERY,
    ID_CO2,
    ID_CO,
    ID_CURRENT,
    ID_DISTANCE1,
    ID_DISTANCE2,
    ID_DISTANCE3,
    ID_ENERGY,
    ID_HCHO,
    ID_HUMIDITY,
    ID_IMAGE,
    ID_JSON,
    ID_LIGHT,
    ID_NOX,
    ID_NOISE,
    ID_PERF1,
    ID_PERF2,
    ID_PERF3,
    ID_PM005,
    ID_PM010,
    ID_PM100,
    ID_PM025,
    ID_PM040,
    ID_POWER,
    ID_PRESENCE1,
    ID_PRESENCE2,
    ID_PRESENCE3,
    ID_PRESSURE,
    ID_RSSI,
    ID_SENSOR,
    ID_STATE,
    ID_SWITCH1,
    ID_SWITCH2,
    ID_SWITCH3,
    ID_SWITCH4,
    ID_SWITCH5,
    ID_SWITCH6,
    ID_SWITCH7,
    ID_SWITCH8,
    ID_TEMPERATURE,
    ID_UNKNOWN,
    ID_UV,
    ID_VIBRATION,
    ID_VOC,
    ID_VOLTAGE,
    ID_PX,
    ID_PY,
    ID_PZ,
};
// clang-format on

// The index here maps into the sorted arrays above
// clang-format off
static const short sUserTypeIndices[] = {
  [ID_UNKNOWN]     = 40,
  [ID_TEMPERATURE] = 39,
  [ID_HUMIDITY]    = 9,
  [ID_PRESSURE]    = 26,
  [ID_LIGHT]       = 12,
  [ID_UV]          = 41,
  [ID_CO]          = 6,
  [ID_CO2]         = 7,
  [ID_HCHO]        = 8,
  [ID_VOC]         = 9,
  [ID_NOX]         = 10,
  [ID_PM005]       = 17,
  [ID_PM010]       = 18,
  [ID_PM025]       = 20,
  [ID_PM040]       = 21,
  [ID_PM100]       = 19,
  [ID_NOISE]       = 15,
  [ID_VIBRATION]   = 16,
  [ID_STATE]       = 18,
  [ID_BATTERY]     = 0,
  [ID_SWITCH1]     = 31,
  [ID_SWITCH2]     = 32,
  [ID_SWITCH3]     = 33,
  [ID_SWITCH4]     = 34,
  [ID_SWITCH5]     = 35,
  [ID_SWITCH6]     = 36,
  [ID_SWITCH7]     = 37,
  [ID_SWITCH8]     = 38,
  [ID_PRESENCE1]   = 24,
  [ID_PRESENCE2]   = 25,
  [ID_PRESENCE3]   = 26,
  [ID_DISTANCE1]   = 4,
  [ID_DISTANCE2]   = 5,
  [ID_DISTANCE3]   = 6,
  [ID_PX]          = 45,
  [ID_PY]          = 46,
  [ID_PZ]          = 47,
  [ID_RSSI]        = 27,
  [ID_PERF1]       = 14,
  [ID_PERF2]       = 15,
  [ID_PERF3]       = 16,
  [ID_VOLTAGE]     = 42,
  [ID_CURRENT]     = 3,
  [ID_POWER]       = 22,
  [ID_ENERGY]      = 7,
  [ID_SENSOR]      = 28,
  [ID_JSON]        = 11,
  [ID_IMAGE]       = 10,
};
// clang-format on

const unsigned char* get_user_type_type_array() { return sUserTypeTypes; }
const char**         get_user_type_key_string_array() { return sUserTypeKeyStrings; }
const char**         get_user_type_ui_string_array() { return sUserTypeUiStrings; }
const short*         get_user_type_to_index_array() { return sUserTypeIndices; }

const char* to_ui_string(EUserType type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < ID_COUNT && sUserTypeUiStrings[index] != nullptr)
        return sUserTypeUiStrings[index];
    return "Unknown";
}

const char* to_key_string(EUserType type)
{
    const short index = sUserTypeIndices[type];
    if (index >= 0 && index < ID_COUNT && sUserTypeKeyStrings[index] != nullptr)
        return sUserTypeKeyStrings[index];
    return "unknown";
}

EUserType from_string(const char* key_str)
{
    // find string end of key_str
    const char* key_str_end = key_str;
    while (*key_str_end != '\0')
    {
        key_str_end++;
    }
    return from_string(key_str, key_str_end);
}

EUserType from_string(const char* key_str, const char* key_str_end)
{
    const int len = key_str_end - key_str;

    // We can binary search because the array is sorted
    int left  = 0;
    int right = ID_COUNT - 1;
    while (left <= right)
    {
        const int mid = left + (right - left) / 2;
        int       cmp = strncmp(key_str, sUserTypeKeyStrings[mid], len);
        if (cmp == 0 && (int)strlen(sUserTypeKeyStrings[mid]) == len)
        {
            return (EUserType)sUserTypeTypes[mid];
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
    return ID_UNKNOWN;
}

// clang-format off
static EValueType sUserTypeValueType[ID_COUNT] = {
    [ID_UNKNOWN] = ValueType_U8,
    [ID_TEMPERATURE] = ValueType_S16,
    [ID_HUMIDITY] = ValueType_U8,
    [ID_PRESSURE] = ValueType_U32,
    [ID_LIGHT] = ValueType_U32,
    [ID_UV] = ValueType_U8,
    [ID_CO] = ValueType_U16,
    [ID_CO2] = ValueType_U16,
    [ID_HCHO] = ValueType_U16,
    [ID_VOC] = ValueType_U16,
    [ID_NOX] = ValueType_U16,
    [ID_PM005] = ValueType_U16,
    [ID_PM010] = ValueType_U16,
    [ID_PM025] = ValueType_U16,
    [ID_PM040] = ValueType_U16,
    [ID_PM100] = ValueType_U16,
    [ID_NOISE] = ValueType_U8,
    [ID_VIBRATION] = ValueType_U16,
    [ID_STATE] = ValueType_U8,
    [ID_BATTERY] = ValueType_U8,
    [ID_SWITCH] = ValueType_U8,
    [ID_PRESENCE1] = ValueType_U8,
    [ID_PRESENCE2] = ValueType_U8,
    [ID_PRESENCE3] = ValueType_U8,
    [ID_DISTANCE1] = ValueType_U16,
    [ID_DISTANCE2] = ValueType_U16,
    [ID_DISTANCE3] = ValueType_U16,
    [ID_PX] = ValueType_S32,
    [ID_PY] = ValueType_S32,
    [ID_PZ] = ValueType_S32,
    [ID_RSSI] = ValueType_S8,
    [ID_PERF1] = ValueType_U32,
    [ID_PERF2] = ValueType_U32,
    [ID_PERF3] = ValueType_U32,
    [ID_VOLTAGE] = ValueType_U16,
    [ID_CURRENT] = ValueType_U16,
    [ID_POWER] = ValueType_U32,
    [ID_ENERGY] = ValueType_U32,
    [ID_SENSOR] = ValueType_BinaryData,
    [ID_JSON] = ValueType_TextData,
    [ID_IMAGE] = ValueType_BinaryData,
};
// clang-format on

EValueType get_value_type(EUserType type)
{
    if (type >= 0 && type < ID_COUNT)
    {
        return sUserTypeValueType[type];
    }
    return ValueType_U8;
}

// clang-format off
static EValueUnit sUserTypeToUnit[] = {
  [ID_UNKNOWN]     = UUnknown,
  [ID_TEMPERATURE] = UTemperature,
  [ID_HUMIDITY]    = UPercent,
  [ID_PRESSURE]    = UPascal,
  [ID_LIGHT]       = ULux,
  [ID_UV]          = UUvIndex,
  [ID_CO]          = UPpm,
  [ID_CO2]         = UPpm,
  [ID_HCHO]        = UPpm,
  [ID_VOC]         = UPpm,
  [ID_NOX]         = UPpm,
  [ID_PM005]       = UPpm,
  [ID_PM010]       = UPpm,
  [ID_PM025]       = UPpm,
  [ID_PM040]       = UPpm,
  [ID_PM100]       = UPpm,
  [ID_NOISE]       = UDecibels,
  [ID_VIBRATION]   = UAcceleration,
  [ID_STATE]       = UTrueFalse,
  [ID_SWITCH]      = UOnOff,
  [ID_PRESENCE1]   = UPresentAbsent,
  [ID_PRESENCE2]   = UPresentAbsent,
  [ID_PRESENCE3]   = UPresentAbsent,
  [ID_DISTANCE1]   = UMeters,
  [ID_DISTANCE2]   = UMeters,
  [ID_DISTANCE3]   = UMeters,
  [ID_PX]          = UMeters,
  [ID_PY]          = UMeters,
  [ID_PZ]          = UMeters,
  [ID_RSSI]        = UdBm,
  [ID_BATTERY]     = UPercent,
  [ID_PERF1]       = UBinaryData,
  [ID_PERF2]       = UBinaryData,
  [ID_PERF3]       = UBinaryData,
  [ID_VOLTAGE]     = UMilliVolt,
  [ID_CURRENT]     = UMilliAmpere,
  [ID_POWER]       = UWatts,
  [ID_ENERGY]      = UKiloWattHour,
  [ID_SENSOR]      = UBinaryData,
  [ID_JSON]        = UBinaryData,
  [ID_IMAGE]       = UBinaryData,
};
// clang-format on

// Get the unit for a given sensor type
EValueUnit get_value_unit(EUserType type)
{
    if (type >= 0 && type < ID_COUNT)
        return sUserTypeToUnit[type];
    return UUnknown;
}
