#include "cconartist/sensor_types.h"

static const char* sSensorTypeStrings[ID_SENSORCOUNT] = {
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
  [ID_SWITCH]      = "Switch",
  [ID_SWITCH2]     = "Switch",
  [ID_SWITCH3]     = "Switch",
  [ID_SWITCH4]     = "Switch",
  [ID_SWITCH5]     = "Switch",
  [ID_SWITCH6]     = "Switch",
  [ID_SWITCH7]     = "Switch",
  [ID_SWITCH8]     = "Switch",
  [ID_PRESENCE1]   = "Presence",
  [ID_PRESENCE2]   = "Presence",
  [ID_PRESENCE3]   = "Presence",
  [ID_DISTANCE1]   = "Distance",
  [ID_DISTANCE2]   = "Distance",
  [ID_DISTANCE3]   = "Distance",
  [ID_PX]          = "X",
  [ID_PY]          = "Y",
  [ID_PZ]          = "Z",
  [ID_RSSI]        = "RSSI",
  [ID_PERF1]       = "Performance Metric",
  [ID_PERF2]       = "Performance Metric",
  [ID_PERF3]       = "Performance Metric",
  [ID_VOLTAGE]     = "Voltage",
  [ID_CURRENT]     = "Current",
  [ID_POWER]       = "Power",
  [ID_ENERGY]      = "Energy",
};

const char* to_string(ESensorType type)
{
    if (type >= 0 && type < ID_SENSORCOUNT && sSensorTypeStrings[type] != nullptr)
        return sSensorTypeStrings[type];
    return "Unknown";
}

static EUnit sSensorTypeToUnit[] = {
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
};

// Get the unit for a given sensor type
EUnit get_unit(ESensorType type)
{
    if (type >= 0 && type < ID_SENSORCOUNT)
        return sSensorTypeToUnit[type];
    return UUnknown;
}
