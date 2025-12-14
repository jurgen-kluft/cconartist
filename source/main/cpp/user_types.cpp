#include "cconartist/user_types.h"

static const char* sUserTypeUIStrings[ID_COUNT] = {
  [ID_UNKNOWN]     = "Unknown",
  [ID_TEMPERATURE] = "Temperature",
  [ID_HUMIDITY]    = "Humidity",
  [ID_PRESSURE]    = "Pressure",
  [ID_LIGHT]       = "Light",
  [ID_UV]          = "UV",
  [ID_CO]          = "Carbon Monoxide",
  [ID_CO2]         = "Carbon Dioxide",
  [ID_HCHO]        = "Formaldehyde",
  [ID_VOC]         = "VOC",
  [ID_NOX]         = "NO",
  [ID_PM005]       = "PM 0.5",
  [ID_PM010]       = "PM 1.0",
  [ID_PM025]       = "PM 2.5",
  [ID_PM040]       = "PM 4.0",
  [ID_PM100]       = "PM 10.0",
  [ID_NOISE]       = "Noise",
  [ID_VIBRATION]   = "Vibration",
  [ID_STATE]       = "State",
  [ID_BATTERY]     = "Battery",
  [ID_SWITCH1]     = "Switch1",
  [ID_SWITCH2]     = "Switch2",
  [ID_SWITCH3]     = "Switch3",
  [ID_SWITCH4]     = "Switch4",
  [ID_SWITCH5]     = "Switch5",
  [ID_SWITCH6]     = "Switch6",
  [ID_SWITCH7]     = "Switch7",
  [ID_SWITCH8]     = "Switch8",
  [ID_PRESENCE1]   = "Presence1",
  [ID_PRESENCE2]   = "Presence2",
  [ID_PRESENCE3]   = "Presence3",
  [ID_DISTANCE1]   = "Distance1",
  [ID_DISTANCE2]   = "Distance2",
  [ID_DISTANCE3]   = "Distance3",
  [ID_PX]          = "X",
  [ID_PY]          = "Y",
  [ID_PZ]          = "Z",
  [ID_RSSI]        = "RSSI",
  [ID_PERF1]       = "Performance Metric 1",
  [ID_PERF2]       = "Performance Metric 2",
  [ID_PERF3]       = "Performance Metric 3",
  [ID_VOLTAGE]     = "Voltage",
  [ID_CURRENT]     = "Current",
  [ID_POWER]       = "Power",
  [ID_ENERGY]      = "Energy",
  [ID_SENSOR]      = "Sensor Packet",
  [ID_JSON]        = "JSON",
  [ID_IMAGE]       = "Image",
};

static const char* sUserTypeKeyStrings[ID_COUNT] = {
  [ID_UNKNOWN]     = "unknown",
  [ID_TEMPERATURE] = "temperature",
  [ID_HUMIDITY]    = "humidity",
  [ID_PRESSURE]    = "pressure",
  [ID_LIGHT]       = "light",
  [ID_UV]          = "uv",
  [ID_CO]          = "co",
  [ID_CO2]         = "co2",
  [ID_HCHO]        = "hcho",
  [ID_VOC]         = "voc",
  [ID_NOX]         = "nox",
  [ID_PM005]       = "pm0.5",
  [ID_PM010]       = "pm1.0",
  [ID_PM025]       = "pm2.5",
  [ID_PM040]       = "pm4.0",
  [ID_PM100]       = "pm10.0",
  [ID_NOISE]       = "noise",
  [ID_VIBRATION]   = "vibration",
  [ID_STATE]       = "state",
  [ID_BATTERY]     = "battery",
  [ID_SWITCH1]     = "switch1",
  [ID_SWITCH2]     = "switch2",
  [ID_SWITCH3]     = "switch3",
  [ID_SWITCH4]     = "switch4",
  [ID_SWITCH5]     = "switch5",
  [ID_SWITCH6]     = "switch6",
  [ID_SWITCH7]     = "switch7",
  [ID_SWITCH8]     = "switch8",
  [ID_PRESENCE1]   = "presence1",
  [ID_PRESENCE2]   = "presence2",
  [ID_PRESENCE3]   = "presence3",
  [ID_DISTANCE1]   = "distance1",
  [ID_DISTANCE2]   = "distance2",
  [ID_DISTANCE3]   = "distance3",
  [ID_PX]          = "x",
  [ID_PY]          = "y",
  [ID_PZ]          = "z",
  [ID_RSSI]        = "rssi",
  [ID_PERF1]       = "perf1",
  [ID_PERF2]       = "perf2",
  [ID_PERF3]       = "perf3",
  [ID_VOLTAGE]     = "voltage",
  [ID_CURRENT]     = "current",
  [ID_POWER]       = "power",
  [ID_ENERGY]      = "energy",
  [ID_SENSOR]      = "sensor",
  [ID_JSON]        = "json",
  [ID_IMAGE]       = "image",
};

const char* ui_string(EUserType type)
{
    if (type >= 0 && type < ID_COUNT && sUserTypeUIStrings[type] != nullptr)
        return sUserTypeUIStrings[type];
    return "Unknown";
}

const char* key_string(EUserType type)
{
    if (type >= 0 && type < ID_COUNT && sUserTypeKeyStrings[type] != nullptr)
        return sUserTypeKeyStrings[type];
    return "unknown";
}
static EValueType sUserTypeValueType[ID_COUNT] = {
  [ID_UNKNOWN] = ValueType_U8,    [ID_TEMPERATURE] = ValueType_S16, [ID_HUMIDITY] = ValueType_U8,       [ID_PRESSURE] = ValueType_U32,  [ID_LIGHT] = ValueType_U32,        [ID_UV] = ValueType_U8,
  [ID_CO] = ValueType_U16,        [ID_CO2] = ValueType_U16,         [ID_HCHO] = ValueType_U16,          [ID_VOC] = ValueType_U16,       [ID_NOX] = ValueType_U16,          [ID_PM005] = ValueType_U16,
  [ID_PM010] = ValueType_U16,     [ID_PM025] = ValueType_U16,       [ID_PM040] = ValueType_U16,         [ID_PM100] = ValueType_U16,     [ID_NOISE] = ValueType_U8,         [ID_VIBRATION] = ValueType_U16,
  [ID_STATE] = ValueType_U8,      [ID_BATTERY] = ValueType_U8,      [ID_SWITCH] = ValueType_U8,         [ID_PRESENCE1] = ValueType_U8,  [ID_PRESENCE2] = ValueType_U8,     [ID_PRESENCE3] = ValueType_U8,
  [ID_DISTANCE1] = ValueType_U16, [ID_DISTANCE2] = ValueType_U16,   [ID_DISTANCE3] = ValueType_U16,     [ID_PX] = ValueType_S32,        [ID_PY] = ValueType_S32,           [ID_PZ] = ValueType_S32,
  [ID_RSSI] = ValueType_S8,       [ID_PERF1] = ValueType_U32,       [ID_PERF2] = ValueType_U32,         [ID_PERF3] = ValueType_U32,     [ID_VOLTAGE] = ValueType_U16,      [ID_CURRENT] = ValueType_U16,
  [ID_POWER] = ValueType_U32,     [ID_ENERGY] = ValueType_U32,      [ID_SENSOR] = ValueType_BinaryData, [ID_JSON] = ValueType_TextData, [ID_IMAGE] = ValueType_BinaryData,
};

EValueType get_value_type(EUserType type)
{
    if (type >= 0 && type < ID_COUNT)
    {
        return sUserTypeValueType[type];
    }
    return ValueType_U8;
}

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

// Get the unit for a given sensor type
EValueUnit get_value_unit(EUserType type)
{
    if (type >= 0 && type < ID_COUNT)
        return sUserTypeToUnit[type];
    return UUnknown;
}
