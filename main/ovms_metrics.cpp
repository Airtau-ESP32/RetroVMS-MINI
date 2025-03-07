/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "ovms_log.h"
static const char *TAG = "metrics";
static const char *UCM = "UnitConfigMap";
static const char *MET = "Metric";

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <functional>
#include <map>
#include "global.h"
#include "ovms_metrics.h"
#include "ovms_command.h"
#include "ovms_events.h"
#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
#include "ovms_script.h"
#endif
#include "ovms_config.h"
#include "rom/rtc.h"
#include "string.h"

using namespace std;

#define PERSISTENT_METRICS_MAGIC        (('O' << 24) | ('V' << 16) | ('M' << 8) | '3')
#define PERSISTENT_VERSION              3                     // increment when struct is changed

RTC_NOINIT_ATTR persistent_metrics      pmetrics;             // persistent storage container
#define NUM_PERSISTENT_VALUES           sizeof_array(pmetrics.values)
static const char*                      pmetrics_reason;      // reason pmetrics was zeroed
std::map<std::size_t, const char*>      pmetrics_keymap;     // hash key → metric name map (registry)


struct OvmsUnitInfo {
  const char *UnitCode; //< The UnitCode identifying the unit
  const char *Label;    //< The suffix to print against the value
  metric_unit_t MetricUnit; //< The Metric equivalent if there is one.
  metric_unit_t ImperialUnit; //< The Imperial equivalent if there is one.
  metric_group_t Group; //< The conversion group it belongs to.
};

#define UNIT_GAP {NULL,NULL, UnitNotFound, UnitNotFound, GrpNone}

// Mapping for information on metric info
static const OvmsUnitInfo unit_info[int(MetricUnitLast)+1] =
{
// Unit Code   Label       Metric Unt  Imperial Unt Unit Group
  {"native",   "",         Native,     Native,      GrpNone   }, // 0
  {"metric",   "",         Native,     Native,      GrpNone   }, // 1
  {"imperial", "",         Native,     Native,      GrpNone   }, // 2
  {"user",     "",         Native,     Native,      GrpNone   }, // 3
  UNIT_GAP, // 4
  UNIT_GAP, // 5
  UNIT_GAP, // 6
  UNIT_GAP, // 7
  UNIT_GAP, // 8
  UNIT_GAP, // 9
  {"km",       "km",       Native,     Miles,       GrpDistance }, // 10
  {"miles",    "M",        Kilometers, Native,      GrpDistance }, // 11
  {"meters",   "m",        Native,     Feet,        GrpDistanceShort }, // 12
  {"feet",     "ft",       Meters,     Native,      GrpDistanceShort }, // 13
  UNIT_GAP, // 14
  UNIT_GAP, // 15
  UNIT_GAP, // 16
  UNIT_GAP, // 17
  UNIT_GAP, // 18
  UNIT_GAP, // 19
  {"celcius",  "°C",       Native,     Fahrenheit,  GrpTemp     }, // 20
  {"fahrenheit","°F",      Celcius,    Native,      GrpTemp     }, // 21
  UNIT_GAP, // 22
  UNIT_GAP, // 23
  UNIT_GAP, // 24
  UNIT_GAP, // 25
  UNIT_GAP, // 26
  UNIT_GAP, // 27
  UNIT_GAP, // 28
  UNIT_GAP, // 29
  {"kpa",      "kPa",      Native,     PSI,         GrpPressure}, // 30
  {"pa",       "Pa",       Native,     PSI,         GrpPressure}, // 31
  {"psi",      "psi",      kPa,        Native,      GrpPressure}, // 32
  {"bar",      "bar",      Native,     PSI,         GrpPressure}, // 33
  UNIT_GAP, // 34
  UNIT_GAP, // 35
  UNIT_GAP, // 36
  UNIT_GAP, // 37
  UNIT_GAP, // 38
  UNIT_GAP, // 39
  {"volts",    "V",        Native,     Native,      GrpOther }, // 40
  {"amps",     "A",        Native,     Native,      GrpOther }, // 41
  {"amphours", "Ah",       Native,     Native,      GrpCharge}, // 42
  {"kw",       "kW",       Native,     Native,      GrpPower }, // 43
  {"kwh",      "kWh",      Native,     Native,      GrpEnergy}, // 44
  {"watts",    "W",        Native,     Native,      GrpPower }, // 45
  {"watthours","Wh",       Native,     Native,      GrpEnergy}, // 46
  {"kilocoulombs", "kC",   Native,     Native,      GrpCharge}, // 47
  {"megajoules",   "MJ",   Native,     Native,      GrpEnergy}, // 48
  UNIT_GAP, // 49
  {"seconds",  "Sec",      Native,     Native,      GrpTime}, // 50
  {"minutes",  "Min",      Native,     Native,      GrpTime}, // 51
  {"hours",    "Hour",     Native,     Native,      GrpTime}, // 52
  {"utc",      "UTC",      Native,     Native,      GrpTime}, // 53
  {"localtz",  "local",    Native,     Native,      GrpTime}, // 54,
  UNIT_GAP,// 55
  UNIT_GAP,// 56
  UNIT_GAP,// 57
  UNIT_GAP,// 58
  UNIT_GAP,// 59
  {"degrees",  "°",        Native,     Native,      GrpDirection}, // 60
  {"kmph",     "km/h",     Native,     Mph,         GrpSpeed}, // 61
  {"miph",     "Mph",      Kph,        Native,      GrpSpeed}, // 62
  {"mps",      "m/s",      Native,     FeetPS,      GrpSpeed}, // 63
  {"ftps",     "ft/s",     MetersPS,   Native,      GrpSpeed}, // 64
  UNIT_GAP,// 65
  UNIT_GAP,// 66
  UNIT_GAP,// 67
  UNIT_GAP,// 68
  UNIT_GAP,// 69
  UNIT_GAP,// 70
  // Acceleration:
  {"kmphps",   "km/h/s",   Native,     MphPS,       GrpAccel}, // 71
  {"miphps",   "Mph/s",    KphPS,      Native,      GrpAccel}, // 72
  {"mpss",     "m/s²",     Native,     FeetPSS,     GrpAccelShort}, // 73
  {"ftpss",    "ft/s²",    MetersPSS,  Native,      GrpAccelShort}, // 74
  UNIT_GAP,// 75
  UNIT_GAP,// 76
  UNIT_GAP,// 77
  UNIT_GAP,// 78
  UNIT_GAP,// 79
  {"dbm",      "dBm",      Native,     sq,          GrpSignal}, // 80
  {"sq",       "sq",       dbm,        Native,      GrpSignal}, // 81
  UNIT_GAP,// 82
  UNIT_GAP,// 83
  UNIT_GAP,// 84
  UNIT_GAP,// 85
  UNIT_GAP,// 86
  UNIT_GAP,// 87
  UNIT_GAP,// 88
  UNIT_GAP,// 89
  {"percent",  "%",        Native,     Native,      GrpRatio}, // 90
  {"permille", "\u2030",   Native,     Native,      GrpRatio}, // 91
  UNIT_GAP,// 92
  UNIT_GAP,// 93
  UNIT_GAP,// 94
  UNIT_GAP,// 95
  UNIT_GAP,// 96
  UNIT_GAP,// 97
  UNIT_GAP,// 98
  UNIT_GAP,// 99
  // Energy consumption:
  {"whpkm",    "Wh/km",    Native,     WattHoursPM, GrpConsumption}, // 100
  {"whpmi",    "Wh/mi",    WattHoursPK,Native,      GrpConsumption}, // 101
  {"kwhp100km","kWh/100km",Native,     MPkWh,       GrpConsumption}, // 102
  {"kmpkwh",   "km/kWh",   Native,     MPkWh,       GrpConsumption}, // 103
  {"mipkwh",   "mi/kWh",   KPkWh,      Native,      GrpConsumption}, // 104
  UNIT_GAP,// 105
  UNIT_GAP,// 106
  UNIT_GAP,// 107
  UNIT_GAP,// 108
  UNIT_GAP,// 109
  // Torque:
  {"nm",       "Nm",       Native,     Native,      GrpTorque} // 110
};
#undef UNIT_GAP

struct OvmsUnitGroupInfo {
  const char *GroupCode; //< The GroupCode identifying the group
  const char *Label;    //< The label to be used for config
};
#define GROUP_GAP {NULL, NULL}
// Mapping for information on metric groups
static const OvmsUnitGroupInfo group_info[int(MetricGroupLast)+1] =
{
// Group Code      Config Label
  { "none",        NULL                      }, // 0
  { "other",       NULL                      }, // 1
  { "distance",    "Distance"                }, // 2
  { "speed",       "Speed"                   }, // 3
  { "accel",       "Acceleration"            }, // 4
  { "power",       "Power"                   }, // 5
  { "energy",      "Energy"                  }, // 6
  { "consumption", "Energy Consumption"      }, // 7
  { "temp",        "Temperature"             }, // 8
  { "pressure",    "Pressure"                }, // 9
  { "time",        NULL                      }, // 10
  { "signal",      "Signal Strength"         }, // 11
  { "torque",      NULL                      }, // 12
  { "direction",   NULL                      }, // 13
  { "ratio",       "Ratio"                   }, // 14
  { "charge",      "Charge"                  }, // 15
  // Short dimensions from here:
  GROUP_GAP, // 16
  GROUP_GAP, // 17
  { "distanceshort","Height"                 }, // 2+16=18
  GROUP_GAP,
  { "accelshort",   "Acceleration (short)"   }, // 4+16=20
};

static inline int mi_to_km(int mi)
  {
  return mi * 4023 / 2500; // mi * 1.6092
  }
static inline float mi_to_km(float mi)
  {
  return mi * 1.609347;
  }
static inline double mi_to_km(double mi)
  {
  return mi * 1.609347;
  }

static inline int km_to_mi(int km)
  {
  return km * 2500 / 4023; // km / 1.6092
  }
static inline float km_to_mi(float km)
  {
  return km * 0.6213700; // km / 1.609347;
  }
static inline double km_to_mi(double km)
  {
  return km * 0.6213700; // 1 / 1.609347;
  }
const int feet_per_mile = 5280;

// Alias for reading clarity.
template<typename T>
T pmi_to_pkm(T pmi)
  {
  return km_to_mi(pmi);
  }
// Alias for reading clarity.
template<typename T>
T pkm_to_pmi(T pkm)
  {
  return mi_to_km(pkm);
  }

/*
 * Returns the group of the metric.
 * simplify - Means those separated for (eventual) user config
 *            are folded to one metric.
 */
metric_group_t GetMetricGroup(metric_unit_t unit)
  {
  uint8_t unit_i = static_cast<uint8_t>(unit);
  if (unit_i <= uint8_t(MetricUnitLast))
    return unit_info[unit_i].Group;
  return GrpNone;
  }

static inline metric_group_t MetricGroupSimplify(metric_group_t group)
  {
    return static_cast<metric_group_t>(static_cast<uint8_t>(group) & GrpFoldMask);
  }

static inline metric_group_t GetMetricGroupSimplify(metric_unit_t unit)
  {
    // Removes High-bit to fold the 'Short' metrics back onto their equivalents.
    return MetricGroupSimplify(GetMetricGroup(unit));
  }
/**
 * Return a list of Metric Groups for configuration.
 */
bool OvmsMetricGroupConfigList(metric_group_list_t& groups)
  {
  bool found = false;
  groups.reserve(12);
  for (uint8_t idx = 0; idx <= GrpFoldMask; ++idx)
    {
    if (group_info[idx].Label != NULL)
      {
      found = true;
      metric_group_t grp = static_cast<metric_group_t>(idx);
      groups.insert(groups.end(), grp);
      // Get unfolded version
      uint8_t unfold = idx + GrpUnfold;
      grp = static_cast<metric_group_t>(unfold);
      if (OvmsMetricGroupLabel(grp) != NULL)
        groups.insert(groups.end(), grp);
      }
    }
  return found;
  }

const char* OvmsMetricGroupLabel(metric_group_t group)
  {
  if (group > MetricGroupLast)
    return NULL;
  return group_info[static_cast<uint8_t>(group)].Label;
  }

const char* OvmsMetricGroupName(metric_group_t group)
  {
  if (group > MetricGroupLast)
    return NULL;
  return group_info[static_cast<uint8_t>(group)].GroupCode;
  }

bool OvmsMetricGroupUnits(metric_group_t group, metric_unit_set_t& units)
  {
  bool found = false;
  for (metric_unit_t unit = MetricUnitFirst; unit <= MetricUnitLast; unit = metric_unit_t(1+(uint8_t)unit))
    {
    if ( group == GetMetricGroup(unit))
      {
      units.insert(unit);
      found=true;
      }
    }
  return found;
  }

std::string OvmsMetricGetUserConfig(metric_group_t group)
  {
  std::string cfg = "units.";
  cfg += OvmsMetricGroupName(group);
  std::string value = OvmsConfig::instance(TAG).GetParamValue("vehicle", cfg);
  if (group == GrpDistance)
    {
    // Backwards compat
    if (value == "M")
      value = "miles";
    else if (value == "K")
      value = "km";
    }
  return value;
  }
void OvmsMetricSetUserConfig(metric_group_t group, std::string value)
  {
  std::string cfg = "units.";
  cfg += OvmsMetricGroupName(group);
  if (group == GrpDistance)
    {
    // Backwards compat
    if (value == "miles")
      value = "M";
    else if (value == "km")
      value = "K";
    }
  if (OvmsConfig::instance(TAG).GetParamValue("vehicle", cfg) != value)
    OvmsConfig::instance(TAG).SetParamValue("vehicle", cfg, value);
  }
void OvmsMetricSetUserConfig(metric_group_t group, metric_unit_t unit)
  {
  switch (group)
    {
    case GrpNone:
    case GrpOther:
       return;
    default: ;
    }
  switch (unit)
    {
    case ToMetric:
    case ToImperial:
    case ToUser:
    case UnitNotFound: break;
    case Native:
      OvmsMetricSetUserConfig(group, "");
      break;
    default:
      OvmsMetricSetUserConfig(group, OvmsMetricUnitName(unit));
    }
  }

/**
 * Returns the User-specified unit for the given unit group.
 * \param group The target group.
 * \param defaultUnit The unit to use if no user unit is specified (defaults to 'Native');
 */
metric_unit_t OvmsMetricGetUserUnit(metric_group_t group, metric_unit_t defaultUnit )
  {
  std::string unit_name = OvmsMetricGetUserConfig(group);
  if (unit_name.empty())
    return defaultUnit;
  metric_unit_t unit = OvmsMetricUnitFromName(unit_name.c_str());
  if (unit == UnitNotFound)
    return defaultUnit;
  return unit;
  }

/*
 * Modifies the 'to' target to match the real target (or Native for no change).
 * Handles ToMetric/ToImperial conversion types.
 *
 * full_check takes into account whether a conversion CAN be done (used for
 * printing correct labels)
 */
bool CheckTargetUnit(metric_unit_t from, metric_unit_t &to, bool full_check)
  {
  if (from == Other)
    {
    to = from;
    return true;
    }
  switch (to)
    {
    case Native: return true;
    case ToMetric:
      {
      uint8_t unit_i = static_cast<uint8_t>(from);
      if (unit_i <= uint8_t(MetricUnitLast))
        to = unit_info[unit_i].MetricUnit;
      return true;
      }
    case ToImperial:
      {
      uint8_t unit_i = static_cast<uint8_t>(from);
      if (unit_i <= uint8_t(MetricUnitLast))
        to = unit_info[unit_i].ImperialUnit;
      return true;
      }
    case ToUser:
      {
      metric_group_t from_grp = GetMetricGroup(from);
      to = UnitConfigMap::instance(TAG).GetUserUnit(from_grp);
      return true;
      }
    default:
      if (to == from)
        {
        to = Native;
        return true;
        }
      else
        {
        if (full_check)
          {
          metric_group_t from_grp = GetMetricGroupSimplify(from);
          if (from_grp == GrpNone || from_grp == GrpOther)
            {
            to = Native;
            return false;
            }
          else if (from_grp != GetMetricGroupSimplify(to))
            {
            to = Native;
            return false;
            }
          }
        return true;
        }
      break;
    }
  }

/**
 * Converts/Checks that the specified unit conversion is allowed.
 * \return The actual unit to use or UnitNotFound if invalid.
 * \param fromUnit The unit value of the value. Must be a real Unit.
 * \param toUnit  The unit being converted to. Can be a psuedo-unit (Native, ToUser, ToMetric, ToImperial).
 */
metric_unit_t OvmsMetricCheckUnit(metric_unit_t fromUnit, metric_unit_t toUnit)
  {
  if (not CheckTargetUnit(fromUnit, toUnit, true))
    return UnitNotFound;
  if (toUnit == Native)
    return fromUnit;
  return toUnit;
  }

void metrics_list(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  bool found = false;
  bool show_staleness = false;
  bool show_set = false;
  bool only_persist = false;
  bool display_strings = false;
  metric_unit_t def_unit = ToUser;
  const char* show_only = NULL;
  int i;
  for (i=0;i<argc;i++)
    {
    const char *cp = argv[i];
    if (*cp != '-')
      {
      if (show_only != NULL)
        {
        cmd->PutUsage(writer);
        return;
        }
      show_only = cp;
      continue;
      }
    for (++cp; *cp != '\0'; ++cp)
      {
      switch (*cp)
        {
        case 'c':
          show_set = true;
          break;
        case 'i':
          def_unit = ToImperial;
          break;
        case 'm':
          def_unit = ToMetric;
          break;
        case 'n':
          def_unit = Native;
          break;
        case 'p':
          only_persist = true;
          break;
        case 's':
          show_staleness = true;
          break;
        case 't':
          display_strings = true;
          break;
        default:
          cmd->PutUsage(writer);
          return;
        }
      }
    }
  for (OvmsMetric* m=OvmsMetrics::instance(TAG).m_first; m != NULL; m=m->m_next)
    {
    if (only_persist && !m->m_persist)
      continue;
    const char *k = m->m_name;
    if (show_only != NULL && strstr(k, show_only) == NULL)
      continue;
    found = true;
    if (show_set)
      {
      if (m->IsDefined())
        writer->printf("metrics set %s %s\n", k, m->AsString().c_str());
      continue;
      }

    metric_unit_t use_unit = def_unit;
    metric_unit_t my_unit = m->GetUnits();
    if (my_unit == TimeUTC)
      use_unit = TimeLocal;
    else
      {
      CheckTargetUnit(my_unit, use_unit, true);
      if (use_unit == Native)
        use_unit = my_unit;
      }

    std::string v = m->AsUnitString("", use_unit);
    if (show_staleness)
      {
      int age = m->Age();
      if (age>99)
        age=99;
      if (v.empty())
        writer->printf("[---] ");
      else
        writer->printf("[%02d%c] ", age, (m->IsStale() ? 'S' : '-' ));
      }
    if (v.empty())
      {
      writer->printf("%s\n",k);
      continue;
      }
    // Apply (linux) "cat -t" semantics to strings
    const char *s;
    if (!display_strings || !m->IsString())
      s = v.c_str();
    else
      s = display_encode(v).c_str();
    writer->printf("%-40.40s %s\n", k, s);
    }
  if (show_only && !found)
    writer->puts("Unrecognised metric name");
  }

void metrics_persist(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  if (argc > 0)
    {
    if (strcmp(argv[0], "-r") != 0)
      {
      cmd->PutUsage(writer);
      return;
      }
    pmetrics.magic = 0;
    }
  if (pmetrics.magic != PERSISTENT_METRICS_MAGIC)
    writer->puts("Persistent metrics will be reset on the next boot");
  writer->printf("version %d, ", pmetrics.version);
  writer->printf("serial %d, ", pmetrics.serial);
  if (pmetrics_reason != NULL)
    writer->printf("%s caused reset, ", pmetrics_reason);
  writer->printf("%d bytes, and ", pmetrics.size);
  writer->printf("%d of %d slots used\n", pmetrics.used, NUM_PERSISTENT_VALUES);
  }

static int metrics_set_validate(OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv, bool complete)
  {
  switch (argc)
    {
    case 1: return OvmsMetrics::instance(TAG).Validate(writer, argc, argv[0], complete);
    case 3:
      {
      OvmsMetric *metric = OvmsMetrics::instance(TAG).FindUniquePrefix(argv[0]);
      if (!metric)
        return -1;
      metric_unit_t unit = metric->GetUnits();
      metric_group_t group = GetMetricGroup(unit);
      return OvmsMetricUnit_Validate(writer, argc, argv[2], complete, group);
      }
    }
  return -1;
  }

void metrics_set(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  const char *unit = NULL;
  if (argc > 2)
    unit = OvmsMetricUnit_FindUniquePrefix(argv[2]);
  if (OvmsMetrics::instance(TAG).Set(argv[0],argv[1], unit))
    writer->puts("Metric set");
  else
    writer->puts("Metric could not be set");
  }

static int metrics_get_validate(OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv, bool complete)
  {
  switch (argc)
    {
    case 1: return OvmsMetrics::instance(TAG).Validate(writer, argc, argv[0], complete);
    case 2:
      {
      OvmsMetric *metric = OvmsMetrics::instance(TAG).FindUniquePrefix(argv[0]);
      if (!metric)
        return -1;
      metric_unit_t unit = metric->GetUnits();
      metric_group_t group = GetMetricGroup(unit);
      return OvmsMetricUnit_Validate(writer, argc, argv[1], complete, group);
      }
    }
  return -1;
  }
void metrics_get(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  const char *unit = NULL;
  if (argc > 1)
    unit = OvmsMetricUnit_FindUniquePrefix(argv[1]);
  std::string str = OvmsMetrics::instance(TAG).GetUnitStr(argv[0], unit);
  writer->puts(str.c_str());
  }

bool pmetrics_check()
  {
  bool ret = true;
  if (pmetrics.magic != PERSISTENT_METRICS_MAGIC)
    {
    ESP_LOGE(TAG, "pmetrics_check: bad magic");
    ret = false;
    }
  if (pmetrics.version != PERSISTENT_VERSION)
    {
    ESP_LOGE(TAG, "pmetrics_check: bad version");
    ret = false;
    }
  if (pmetrics.size != sizeof(pmetrics))
    {
    ESP_LOGE(TAG, "pmetrics_check: bad size");
    ret = false;
    }
  if ((pmetrics.used < 0 || pmetrics.used > NUM_PERSISTENT_VALUES))
    {
    ESP_LOGE(TAG, "pmetrics_check: out of range used");
    ret = false;
    }
  for (OvmsMetric* m = OvmsMetrics::instance(TAG).m_first; m != NULL; m = m->m_next)
    {
    if (m->m_persist && !m->CheckPersist())
      {
      ret = false;
      break;
      }
    }
  return ret;
  }

persistent_values *pmetrics_find(const char *name)
  {
  int i;
  persistent_values *vp;
  std::size_t namehash = std::hash<std::string>{}(name);
  for (i = 0, vp = pmetrics.values; i < pmetrics.used; ++i, ++vp)
    {
    if (vp->namehash == namehash)
      return vp;
    }
  return NULL;
  }

void pmetrics_init(bool refresh = false)
  {
  memset(&pmetrics, 0, sizeof(pmetrics));
  pmetrics.magic = PERSISTENT_METRICS_MAGIC;
  pmetrics.version = PERSISTENT_VERSION;
  pmetrics.size = sizeof(persistent_metrics);
  if (refresh)
    {
    for (OvmsMetric* m = OvmsMetrics::instance(TAG).m_first; m != NULL; m = m->m_next)
      m->RefreshPersist();
    }
  }

persistent_values *pmetrics_register(const char *name)
  {
  ESP_LOGI(TAG, "pmetrics_register: '%s'", name);
  int i;
  persistent_values *vp;
  std::size_t namehash = std::hash<std::string>{}(name);

  // check for hash collision:
  auto it = pmetrics_keymap.find(namehash);
  if (it != pmetrics_keymap.end() && strcmp(it->second, name) != 0)
    {
    ESP_LOGE(TAG, "pmetrics_register: cannot persist '%s' due to hash collision with '%s'",
      name, it->second);
    return NULL;
    }

  // find slot:
  for (i = 0, vp = pmetrics.values; i < pmetrics.used; ++i, ++vp)
    {
    if (vp->namehash == namehash)
      break;
    }

  // not found? assign to next free slot:
  if (i >= pmetrics.used)
    {
    if (i >= NUM_PERSISTENT_VALUES)
      {
      ESP_LOGE(TAG, "no free slots, used: %i, slot: %i, pmetric '%s'", pmetrics.used, i, name);
      return NULL;
      }
    vp->namehash = namehash;
    memset(&vp->value, 0, sizeof(vp->value));
    ++pmetrics.used;
    }

  ESP_LOGD(TAG, "pmetrics_register: '%s' => slot=%d, used %d/%d",
    name, i, pmetrics.used, NUM_PERSISTENT_VALUES);
  pmetrics_keymap[namehash] = name;
  return vp;
  }

void OvmsMetrics::EventSystemShutDown(std::string event, void* data)
  {
  /* Check for corruption and repair of possible before shutting down */
  if (!pmetrics_check())
    {
    ESP_LOGI(TAG, "Persistent metrics shutdown check failed");
    pmetrics_init(true);
    }
  }

void metrics_trace(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  if (strcmp(cmd->GetName(),"on")==0)
    OvmsMetrics::instance(TAG).m_trace = true;
  else
    OvmsMetrics::instance(TAG).m_trace = false;

  writer->printf("Metric tracing is now %s\n",cmd->GetName());
  }

void metrics_units(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  const char* show_only = NULL;
  int i;
  for (i=0;i<argc;i++)
    {
    const char *cp = argv[i];
    if (*cp != '-')
      {
      if (show_only != NULL)
        {
        cmd->PutUsage(writer);
        return;
        }
      show_only = cp;
      continue;
      }
    }

  bool found = false;
  for (metric_unit_t unit = MetricUnitFirst; unit <= MetricUnitLast; unit = metric_unit_t(1+(uint8_t)unit))
    {
    const char *metric_name = OvmsMetricUnitName(unit);
    if (metric_name == NULL)
      continue;
    if (show_only != NULL && strstr(metric_name, show_only) == NULL)
      continue;
    const char *metric_label;
    switch (unit)
      {
      case Native:     metric_label = "(internal/original unit)"; break;
      case ToMetric:   metric_label = "(equivalent metric unit)"; break;
      case ToImperial: metric_label = "(equivalent Imperial unit)"; break;
      case ToUser:     metric_label = "(user-configured unit)"; break;
      default:         metric_label = OvmsMetricUnitLabel(unit);
      }
    writer->printf("%12s : %s\n", metric_name, metric_label);
    found = true;
    }
  if (show_only && !found)
    writer->puts("Unrecognised unit name");
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
static duk_ret_t DukOvmsMetricHasValue(duk_context *ctx)
  {
  DukContext dc(ctx);
  const char *mn = duk_to_string(ctx,0);
  OvmsMetric *m = OvmsMetrics::instance(TAG).Find(mn);
  if (!m)
    return 0;
  dc.Push(m->IsDefined());
  return 1;
  }

static duk_ret_t DukOvmsMetricValue(duk_context *ctx)
  {
  DukContext dc(ctx);
  const char *mn = duk_to_string(ctx,0);
  OvmsMetric *m = OvmsMetrics::instance(TAG).Find(mn);
  if (!m)
    return 0;
  bool decode = true;
  const char *un =  NULL;
  bool has_unit = false;
  if (duk_check_type_mask(ctx, 1, DUK_TYPE_MASK_BOOLEAN))
    decode = duk_opt_boolean(ctx, 1, true);
  else
    {
    un = duk_opt_string(ctx, 1, NULL);
    decode = duk_opt_boolean(ctx, 2, true);
    has_unit = un != NULL;
    }
  metric_unit_t unit = OvmsMetricUnitFromName(un);

  if (m && unit != UnitNotFound)
    {
    if (decode)
      m->DukPush(dc, unit);
    else if (has_unit)
      dc.Push(m->AsUnitString("", unit));
    else
      dc.Push(m->AsString(""));
    return 1;  /* one return value */
    }
  else
    return 0;
  }

static duk_ret_t DukOvmsMetricJSON(duk_context *ctx)
  {
  const char *mn = duk_to_string(ctx,0);
  OvmsMetric *m = OvmsMetrics::instance(TAG).Find(mn);
  if (m)
    {
    duk_push_string(ctx, m->AsJSON().c_str());
    return 1;  /* one return value */
    }
  else
    return 0;
  }

static duk_ret_t DukOvmsMetricFloat(duk_context *ctx)
  {
  const char *mn = duk_to_string(ctx,0);
  OvmsMetric *m = OvmsMetrics::instance(TAG).Find(mn);
  const char *un = duk_opt_string(ctx,1,NULL);
  metric_unit_t unit = OvmsMetricUnitFromName(un);
  if (m && unit != UnitNotFound)
    {
    duk_push_number(ctx, float2double(m->AsFloat(0, unit)));
    return 1;  /* one return value */
    }
  else
    return 0;
  }

static duk_ret_t DukOvmsMetricGetValues(duk_context *ctx)
  {
  OvmsMetric *m;
  DukContext dc(ctx);

  bool has_unit = false;
  bool decode = true;
  const char *un =  NULL;
  if (duk_check_type_mask(ctx, 1, DUK_TYPE_MASK_BOOLEAN))
    decode = duk_opt_boolean(ctx, 1, true);
  else
    {
    un = duk_opt_string(ctx, 1, NULL);
    has_unit = un != NULL;
    decode = duk_opt_boolean(ctx, 2, true);
    }
  metric_unit_t unit = OvmsMetricUnitFromName(un);

  duk_idx_t obj_idx = dc.PushObject();

  // helper: set object property from metric
  auto set_metric = [&dc, obj_idx, decode, unit, has_unit](OvmsMetric *m)
    {
    if (decode)
      m->DukPush(dc, unit);
    else if (has_unit)
      dc.Push(m->AsUnitString("", unit));
    else
      dc.Push(m->AsString());
    dc.PutProp(obj_idx, m->m_name);
    };

  if (duk_is_array(ctx, 0))
    {
    // get metric names from array:
    for (int i=0; duk_get_prop_index(ctx, 0, i); i++)
      {
      m = OvmsMetrics::instance(TAG).Find(duk_to_string(ctx, -1));
      if (m) set_metric(m);
      duk_pop(ctx);
      }
    duk_pop(ctx);
    }
  else if (duk_is_object(ctx, 0))
    {
    // get metric names from object properties:
    duk_enum(ctx, 0, 0);
    while (duk_next(ctx, -1, true))
      {
      m = OvmsMetrics::instance(TAG).Find(duk_to_string(ctx, -2));
      if (m) set_metric(m);
      duk_pop_2(ctx);
      }
    duk_pop(ctx);
    }
  else
    {
    // simple metric name substring filter:
    const char *filter = duk_opt_string(ctx, 0, "");
    for (m = OvmsMetrics::instance(TAG).m_first; m; m = m->m_next)
      {
      if (*filter && !strstr(m->m_name, filter))
        continue;
      set_metric(m);
      }
    }

  return 1;
  }

#endif //#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE

MetricCallbackEntry::MetricCallbackEntry(std::string caller, MetricCallback callback)
  {
  m_caller = caller;
  m_callback = callback;
  }

MetricCallbackEntry::~MetricCallbackEntry()
  {
  }

// Construct On First Use instantiation 
OvmsMetrics &OvmsMetrics::instance(const char* caller) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
        ESP_LOGI(TAG, "COFU by %s", caller);
    }
    static OvmsMetrics _instance;
    return _instance;
}

OvmsMetrics::OvmsMetrics()
  {
  m_nextmodifier = 1;
  m_first = NULL;
  m_trace = false;

  // Register our commands
  OvmsCommand* cmd_metric = OvmsCommandApp::instance(TAG).RegisterCommand("metrics","METRICS framework");
  cmd_metric->RegisterCommand("list","Show all metrics", metrics_list,
      "[-cimnpst] [<metric>]\n"
      "Display a metric, show all by default\n"
      "-c = display persistent metrics set commands\n"
      "-i = display imperial units where possible\n"
      "-m = display metric units where possible\n"
      "-n = show metrics in native units\n"
      "-p = display only persistent metrics\n"
      "-s = show metric staleness\n"
      "-t = display non-printing characters and tabs in string metrics" , 0, 2);
  cmd_metric->RegisterCommand("persist","Show persistent metrics info", metrics_persist, "[-r]\n"
      "-r = reset persistent metrics", 0, 1);
  cmd_metric->RegisterCommand("set","Set the value of a metric",metrics_set, "<metric> <value> [<unit>]", 2, 3, true, metrics_set_validate);

  cmd_metric->RegisterCommand("get","Get the value of a metric",metrics_get, "<metric> [<unit>]", 1, 2, true, metrics_get_validate);
  cmd_metric->RegisterCommand("units","List available units",metrics_units, "[<name>]",0,1);

  OvmsCommand* cmd_metrictrace = cmd_metric->RegisterCommand("trace","METRIC trace framework");
  cmd_metrictrace->RegisterCommand("on","Turn metric tracing ON",metrics_trace);
  cmd_metrictrace->RegisterCommand("off","Turn metric tracing OFF",metrics_trace);

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
  ESP_LOGI(TAG, "Expanding DUKTAPE javascript engine");
  DuktapeObjectRegistration* dto = new DuktapeObjectRegistration("OvmsMetrics");
  dto->RegisterDuktapeFunction(DukOvmsMetricHasValue, 1, "HasValue");
  dto->RegisterDuktapeFunction(DukOvmsMetricValue, 3, "Value");
  dto->RegisterDuktapeFunction(DukOvmsMetricJSON, 1, "AsJSON");
  dto->RegisterDuktapeFunction(DukOvmsMetricFloat, 2, "AsFloat");
  dto->RegisterDuktapeFunction(DukOvmsMetricGetValues, 3, "GetValues");
  MyDuktape.RegisterDuktapeObject(dto);
#endif //#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE

/* Initialize persistent metrics on cold boot or corruption */
// if (rtc_get_reset_reason(0) == POWERON_RESET || !pmetrics_check())
  pmetrics_init();
ESP_LOGI(TAG, "Persistent metrics serial %u using %d bytes, %d/%d slots used",
    ++pmetrics.serial, sizeof(pmetrics), pmetrics.used, NUM_PERSISTENT_VALUES);

  // Register our event
#ifdef bind
  #undef bind  // Kludgy, but works
#endif
  using std::placeholders::_1;
  using std::placeholders::_2;
  OvmsEvents::instance(TAG).RegisterEvent(TAG, "system.shutdown",
      std::bind(&OvmsMetrics::EventSystemShutDown, this, _1, _2));

  }

OvmsMetrics::~OvmsMetrics()
  {
  for (OvmsMetric* m = m_first; m!=NULL;)
    {
    OvmsMetric* c = m;
    m = m->m_next;
    delete c;
    }
  }

void OvmsMetrics::RegisterMetric(OvmsMetric* metric)
  {
  // Quick simple check for if we are the first metric.
  if (m_first == NULL)
    {
    m_first = metric;
    return;
    }

  // Now, check if we are before the first
  if (strcmp(m_first->m_name,metric->m_name)>=0)
    {
    metric->m_next = m_first;
    m_first = metric;
    return;
    }

  // Search for the correct place to insert
  for (OvmsMetric*m = m_first; m!=NULL; m=m->m_next)
    {
    if (m->m_next == NULL)
      {
      m->m_next = metric;
      return;
      }
    if (strcmp(m->m_next->m_name,metric->m_name)>=0)
      {
      metric->m_next = m->m_next;
      m->m_next = metric;
      return;
      }
    }
  }

void OvmsMetrics::DeregisterMetric(OvmsMetric* metric)
  {
  if (m_first == metric)
    {
    m_first = metric->m_next;
    delete metric;
    return;
    }

  for (OvmsMetric* m=m_first;m!=NULL;m=m->m_next)
    {
    if (m->m_next == metric)
      {
      m->m_next = metric->m_next;
      delete metric;
      return;
      }
    }
  }

std::string OvmsMetrics::GetUnitStr(const char* metric, const char *unit)
  {
  OvmsMetric* m = Find(metric);
  if (m == NULL) return "(not found)";
  metric_unit_t metric_unit = Native;
  if (unit != NULL)
    {
    metric_unit_t found_unit = OvmsMetricUnitFromName(unit);
    if (found_unit == UnitNotFound)
      return "(invalid unit)";
    metric_unit = found_unit;
    }
  return m->AsUnitString("(not set)", metric_unit);
  }

bool OvmsMetrics::Set(const char* metric, const char* value, const char *unit)
  {
  OvmsMetric* m = Find(metric);
  if (m == NULL) return false;
  metric_unit_t metric_unit = Native;
  if (unit != NULL)
    {
    metric_unit_t found_unit = OvmsMetricUnitFromName(unit);
    if (found_unit == UnitNotFound)
      return false;
    metric_unit = found_unit;
    }

  m->SetValue(std::string(value), metric_unit);
  return true;
  }

bool OvmsMetrics::SetInt(const char* metric, int value)
  {
  OvmsMetricInt* m = (OvmsMetricInt*)Find(metric);
  if (m == NULL) return false;

  m->SetValue(value);
  return true;
  }

bool OvmsMetrics::SetBool(const char* metric, bool value)
  {
  OvmsMetricBool* m = (OvmsMetricBool*)Find(metric);
  if (m == NULL) return false;

  m->SetValue(value);
  return true;
  }

bool OvmsMetrics::SetFloat(const char* metric, float value)
  {
  OvmsMetricFloat* m = (OvmsMetricFloat*)Find(metric);
  if (m == NULL) return false;

  m->SetValue(value);
  return true;
  }

OvmsMetric* OvmsMetrics::Find(const char* metric)
  {
  for (OvmsMetric* m=m_first; m != NULL; m=m->m_next)
    {
    if (strcmp(m->m_name,metric)==0) return m;
    }
  return NULL;
  }

OvmsMetric* OvmsMetrics::FindUniquePrefix(const char* token) const
  {
  size_t len = strlen(token);
  OvmsMetric* found = NULL;
  for (OvmsMetric* m=m_first; m != NULL; m=m->m_next)
    {
    if (strncmp(m->m_name, token, len) == 0)
      {
      if (len == strlen(m->m_name))
        return m;
      if (found)
        return NULL;
      else
        found = m;
      }
    }
  return found;
  }
bool OvmsMetrics::GetCompletion(OvmsWriter* writer, const char* token) const
  {
    unsigned int index = 0;
    bool match = false;
    writer->SetCompletion(index, NULL);
    if (token)
      {
      size_t len = strlen(token);
      for (OvmsMetric* m=m_first; m != NULL; m=m->m_next)
        {
        if (strncmp(m->m_name, token, len) == 0)
        {
          writer->SetCompletion(index++, m->m_name);
          match = true;
        }
      }
    }
    return match;
  }
int OvmsMetrics::Validate(OvmsWriter* writer, int argc, const char* token, bool complete) const
  {
  if (complete)
    {
    if (!GetCompletion(writer, token))
      return -1;
    }
  else
    {
    if (FindUniquePrefix(token) == NULL)
      {
      if (strcmp(token, "?") != 0)
        writer->printf("Metric: %s is not defined\n", token);
      return -1;
      }
    }
  return argc;
  }

OvmsMetricInt* OvmsMetrics::InitInt(const char* metric, uint16_t autostale, int value, metric_unit_t units, bool persist)
  {
  OvmsMetricInt *m = (OvmsMetricInt*)Find(metric);
  if (m==NULL) m = new OvmsMetricInt(metric, autostale, units, persist);

  if (!m->IsDefined())
    m->SetValue(value);
  return m;
  }

OvmsMetricBool* OvmsMetrics::InitBool(const char* metric, uint16_t autostale, bool value, metric_unit_t units, bool persist)
  {
  OvmsMetricBool *m = (OvmsMetricBool*)Find(metric);

  if (m==NULL) m = new OvmsMetricBool(metric, autostale, units, persist);

  if (!m->IsDefined())
    m->SetValue(value);
  return m;
  }

OvmsMetricFloat* OvmsMetrics::InitFloat(const char* metric, uint16_t autostale, float value, metric_unit_t units, bool persist)
  {
  OvmsMetricFloat *m = (OvmsMetricFloat*)Find(metric);

  if (m==NULL) m = new OvmsMetricFloat(metric, autostale, units, persist);

  if (!m->IsDefined())
    m->SetValue(value);
  return m;
  }

OvmsMetricString* OvmsMetrics::InitString(const char* metric, uint16_t autostale, const char* value, metric_unit_t units)
  {
  OvmsMetricString *m = (OvmsMetricString*)Find(metric);
  if (m==NULL) m = new OvmsMetricString(metric, autostale, units);

  if (value && !m->IsDefined())
    m->SetValue(value);
  return m;
  }

void OvmsMetrics::RegisterListener(std::string caller, std::string name, MetricCallback callback)
  {
  auto k = m_listeners.find(name);
  if (k == m_listeners.end())
    {
    m_listeners[name] = new MetricCallbackList();
    k = m_listeners.find(name);
    }
  if (k == m_listeners.end())
    {
    ESP_LOGE(TAG, "Problem registering metric %s for caller %s",name.c_str(),caller.c_str());
    return;
    }

  MetricCallbackList *ml = k->second;
  ml->push_back(new MetricCallbackEntry(caller,callback));
  }

void OvmsMetrics::DeregisterListener(std::string caller)
  {
  MetricCallbackMap::iterator itm=m_listeners.begin();
  while (itm!=m_listeners.end())
    {
    MetricCallbackList* ml = itm->second;
    MetricCallbackList::iterator itc=ml->begin();
    while (itc!=ml->end())
      {
      MetricCallbackEntry* ec = *itc;
      if (ec->m_caller == caller)
        {
        itc = ml->erase(itc);
        delete ec;
        }
      else
        {
        ++itc;
        }
      }
    if (ml->empty())
      {
      itm = m_listeners.erase(itm);
      delete ml;
      }
    else
      {
      ++itm;
      }
    }
  }

void OvmsMetrics::NotifyModified(OvmsMetric* metric)
  {
  if (m_trace &&
      strcmp(metric->m_name, "m.monotonic") != 0 &&
      strcmp(metric->m_name, "m.time.utc") != 0 &&
      strcmp(metric->m_name, "v.e.parktime") != 0 &&
      strcmp(metric->m_name, "v.e.drivetime") != 0 &&
      strcmp(metric->m_name, "v.c.time") != 0)
    {
    ESP_LOGI(TAG, "Modified metric %s: %s",
      metric->m_name, metric->AsUnitString().c_str());
    }

  auto k = m_listeners.find("*");
  for (int x=0;x<2;x++)
    {
    if (k != m_listeners.end())
      {
      MetricCallbackList* ml = k->second;
      if (ml)
        {
        for (MetricCallbackList::iterator itc=ml->begin(); itc!=ml->end(); ++itc)
          {
          MetricCallbackEntry* ec = *itc;
          ec->m_callback(metric);
          }
        }
      }
    k = m_listeners.find(metric->m_name);
    }
  }

size_t OvmsMetrics::RegisterModifier()
  {
  return m_nextmodifier++;
  }

void OvmsMetrics::InitialiseSlot(size_t modifier)
  {
  // Set for send.
  SetAllUnitSend(modifier);
  unsigned long bit = 1ul << modifier;
  for (OvmsMetric* m = m_first; m != NULL; m = m->m_next)
    {
     if (m->IsDefined())
       m->m_modified |= bit;
    }
  }

void OvmsMetrics::SetAllUnitSend(size_t modifier)
  {
  for (OvmsMetric* m = m_first; m != NULL; m = m->m_next)
    m->SetUnitSend(modifier);
  }
void OvmsMetrics::SetAllGroupUnitSend(metric_group_t group)
  {
  for (OvmsMetric* m = m_first; m != NULL; m = m->m_next)
    {
    if (group == GetMetricGroup(m->GetUnits()))
      m->SetUnitSendAll();
    }
  }
unsigned long OvmsMetrics::GetUnitSendAll()
  {
  unsigned long ret = 0;
  for (OvmsMetric* m = m_first; m != NULL; m = m->m_next)
    ret |= m->m_sendunit;
  return ret;
  }
OvmsMetric::OvmsMetric(const char* name, uint16_t autostale, metric_unit_t units, bool persist)
  {
  m_defined = NeverDefined;
  m_modified = 0;
  m_name = name;
  m_lastmodified = 0;
  m_autostale = autostale;
  m_stale = false;
  m_units = units;
  m_next = NULL;
  m_persist = false;          // only set by metrics supporting persistence
  OvmsMetrics::instance(MET).RegisterMetric(this);
  }

OvmsMetric::~OvmsMetric()
  {
  OvmsMetrics::instance(MET).DeregisterMetric(this);

  // Warning: pointers to a deleted OvmsMetric can still be held locally in
  //  other modules. If you delete metrics, take care to inform all readers
  //  (i.e. by broadcasting a module shutdown event).
  }

std::string OvmsMetric::AsString(const char* defvalue, metric_unit_t units, int precision)
  {
  return std::string(defvalue);
  }

std::string OvmsMetric::AsUnitString(const char* defvalue, metric_unit_t units, int precision)
  {
  if (!IsDefined())
    return std::string(defvalue);

  // Need the converted unit for putting the label.
  auto currentUnits = GetUnits();
  CheckTargetUnit(currentUnits, units, true);
  return AsString(defvalue, units, precision) + OvmsMetricUnitLabel(units==Native ? currentUnits : units);
  }

std::string OvmsMetric::AsJSON(const char* defvalue, metric_unit_t units, int precision)
  {
  std::string buf = "\"";
  buf.append(json_encode(AsString(defvalue, units, precision)));
  buf.append("\"");
  return buf;
  }

float OvmsMetric::AsFloat(const float defvalue, metric_unit_t units)
  {
  return defvalue;
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
void OvmsMetric::DukPush(DukContext &dc, metric_unit_t units)
  {
  dc.Push(AsString("", units));
  }
#endif

bool OvmsMetric::SetValue(std::string value, metric_unit_t units)
  {
  return false;
  }

bool OvmsMetric::SetValue(dbcNumber& value)
  {
  return false;
  }

void OvmsMetric::operator=(std::string value)
  {
  }

uint32_t OvmsMetric::LastModified()
  {
  return m_lastmodified;
  }

uint32_t OvmsMetric::Age()
  {
  return monotonictime - m_lastmodified;
  }

void OvmsMetric::SetModified(bool changed)
  {
  if (m_defined == NeverDefined)
    m_defined = FirstDefined;
  else
    m_defined = Defined;
  m_stale = false;
  m_lastmodified = monotonictime;
  if (changed)
    {
    m_modified = ULONG_MAX;
    OvmsMetrics::instance(MET).NotifyModified(this);
    }
  }

bool OvmsMetric::IsUnitSend(size_t modifier)
  {
    return m_sendunit & (1ul << modifier);
  }
bool OvmsMetric::IsUnitSendAndClear(size_t modifier)
  {
  unsigned long bit = 1ul << modifier;
  unsigned long send = m_sendunit.fetch_and(~bit);
  return (send & bit) != 0;
  }
void OvmsMetric::ClearUnitSend(size_t modifier)
  {
    m_sendunit &= ~(1ul << modifier);
  }
void OvmsMetric::SetUnitSend(size_t modifier)
  {
  unsigned long bit = 1ul << modifier;
  m_sendunit |= bit;
  }
void OvmsMetric::SetUnitSendAll()
  {
  m_sendunit = ULONG_MAX;
  }

bool OvmsMetric::IsDefined()
  {
  return (m_defined != NeverDefined);
  }

bool OvmsMetric::IsFirstDefined()
  {
  return (m_defined == FirstDefined);
  }

bool OvmsMetric::IsPersistent()
  {
  return m_persist;
  }

bool OvmsMetric::CheckPersist()
  {
  return true;
  }

void OvmsMetric::RefreshPersist()
  {
  }

/**
 * IsStale: check if metric value has not been set within the staleness period / since marked stale
 *  Note: a persistent metric won't be stale immediately after a reboot, because
 *    it's unknown when the value was set before the reboot. If you need to assert
 *    freshness, use IsFresh().
 */
bool OvmsMetric::IsStale()
  {
  if (m_autostale > 0)
    {
    if (m_lastmodified + m_autostale < monotonictime)
      m_stale = true;
    else
      m_stale = false;
    }
  return m_stale;
  }

/**
 * IsFresh: check if metric value has been set explicitly within the staleness period / since marked stale
 *  Note: a persistent metric won't be fresh immediately after a reboot, because
 *    it's unknown when the value was set before the reboot. It needs to receive a
 *    live value to become fresh.
 */
bool OvmsMetric::IsFresh()
  {
  if (m_defined == NeverDefined)
    return false;
  else if (m_persist && m_defined == FirstDefined)
    return false;
  else
    return !IsStale();
  }

void OvmsMetric::SetStale(bool stale)
  {
  m_stale = stale;
  }

void OvmsMetric::SetAutoStale(uint16_t seconds)
  {
  m_autostale = seconds;
  }

metric_unit_t OvmsMetric::GetUnits()
  {
  return m_units;
  }

bool OvmsMetric::IsModified(size_t modifier)
  {
  return m_modified & 1ul << modifier;
  }

bool OvmsMetric::IsModifiedAndClear(size_t modifier)
  {
  unsigned long bit = 1ul << modifier;
  unsigned long mod = m_modified.fetch_and(~bit);
  return mod & bit;
  }

void OvmsMetric::ClearModified(size_t modifier)
  {
  m_modified &= ~(1ul << modifier);
  }

void OvmsMetric::Clear()
  {
  SetValue("");
  m_defined = NeverDefined;
  m_stale = true;
  }

OvmsMetricInt::OvmsMetricInt(const char* name, uint16_t autostale, metric_unit_t units, bool persist)
  : OvmsMetric(name, autostale, units, persist)
  {
  m_value = 0;
  m_valuep = NULL;
  m_persist = persist;

  if (m_persist)
    {
    persistent_values *vp = pmetrics_register(name);
    if (!vp)
      {
      m_persist = false;
      }
    else
      {
      m_valuep = reinterpret_cast<int*>(&vp->value);
      if (m_value != *m_valuep)
        {
        m_value = *m_valuep;
        SetModified(true);
        ESP_LOGI(TAG, "persist %s = %s", name, AsUnitString().c_str());
        }
      }
    }
  }

OvmsMetricInt::~OvmsMetricInt()
  {
  }

bool OvmsMetricInt::CheckPersist()
  {
  if (!m_persist || !m_valuep || !IsDefined())
    return true;
  if (*m_valuep != m_value)
    {
    ESP_LOGE(TAG, "CheckPersist: bad value for %s", m_name);
    return false;
    }
  persistent_values *vp = pmetrics_find(m_name);
  if (vp == NULL)
    {
    ESP_LOGE(TAG, "CheckPersist: can't find %s", m_name);
    return false;
    }
  if (m_valuep != reinterpret_cast<int*>(&vp->value))
    {
    ESP_LOGE(TAG, "CheckPersist: bad address for %s", m_name);
    return false;
    }
  return true;
  }

void OvmsMetricInt::RefreshPersist()
  {
  if (m_persist && m_valuep && IsDefined())
    *m_valuep = m_value;
  }

std::string OvmsMetricInt::AsString(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    {
    char buffer[33];
    int value = m_value;
    if ((units != Native)&&(units != m_units))
      value = UnitConvert(m_units,units,m_value);
    if (units == TimeUTC || units == TimeLocal)
      {
      int seconds = value % 60;
      value /= 60;
      int minutes = value % 60;
      value /= 60;
      int hours = value;
      snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
      }
    else
      itoa (value,buffer,10);
    return buffer;
    }
  else
    {
    return std::string(defvalue);
    }
  }

std::string OvmsMetricInt::AsJSON(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    return AsString(defvalue, units, precision);
  else
    return std::string((defvalue && *defvalue) ? defvalue : "0");
  }

float OvmsMetricInt::AsFloat(const float defvalue, metric_unit_t units)
  {
  return (float)AsInt((int)defvalue, units);
  }

int OvmsMetricInt::AsInt(const int defvalue, metric_unit_t units)
  {
  if (IsDefined())
    {
    if ((units != Native)&&(units != m_units))
      return UnitConvert(m_units,units,m_value);
    else
      return m_value;
    }
  else
    return defvalue;
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
void OvmsMetricInt::DukPush(DukContext &dc, metric_unit_t units)
  {
  dc.Push(AsInt(0, units));
  }
#endif

bool OvmsMetricInt::SetValue(int value, metric_unit_t units)
  {
  int nvalue = value;
  if ((units != Other)&&(units != m_units))
    nvalue=UnitConvert(units,m_units,value);

  if (m_value != nvalue)
    {
    m_value = nvalue;
    if (m_valuep)
      *m_valuep = m_value;
    SetModified(true);
    return true;
    }
  else
    {
    SetModified(false);
    return false;
    }
  }

bool OvmsMetricInt::SetValue(std::string value, metric_unit_t units)
  {
  int nvalue = atoi(value.c_str());
  return SetValue(nvalue, units);
  }

bool OvmsMetricInt::SetValue(dbcNumber& value)
  {
  return SetValue(value.GetSignedInteger());
  }

void OvmsMetricInt::Clear()
  {
  SetValue(0);
  OvmsMetric::Clear();
  }

OvmsMetricBool::OvmsMetricBool(const char* name, uint16_t autostale, metric_unit_t units, bool persist)
  : OvmsMetric(name, autostale, units, persist)
  {
  m_value = false;
  m_valuep = NULL;
  m_persist = persist;

  if (m_persist)
    {
    persistent_values *vp = pmetrics_register(name);
    if (!vp)
      {
      m_persist = false;
      }
    else
      {
      m_valuep = reinterpret_cast<bool*>(&vp->value);
      if (m_value != *m_valuep)
        {
        m_value = *m_valuep;
        SetModified(true);
        ESP_LOGI(TAG, "persist %s = %s", name, AsUnitString().c_str());
        }
      }
    }
  }

OvmsMetricBool::~OvmsMetricBool()
  {
  }

bool OvmsMetricBool::CheckPersist()
  {
  if (!m_persist || !m_valuep || !IsDefined())
    return true;
  if (*m_valuep != m_value)
    {
    ESP_LOGE(TAG, "CheckPersist: bad value for %s", m_name);
    return false;
    }
  persistent_values *vp = pmetrics_find(m_name);
  if (vp == NULL)
    {
    ESP_LOGE(TAG, "CheckPersist: can't find %s", m_name);
    return false;
    }
  if (m_valuep != reinterpret_cast<bool*>(&vp->value))
    {
    ESP_LOGE(TAG, "CheckPersist: bad address for %s", m_name);
    return false;
    }
  return true;
  }

void OvmsMetricBool::RefreshPersist()
  {
  if (m_persist && m_valuep && IsDefined())
    *m_valuep = m_value;
  }

std::string OvmsMetricBool::AsString(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    {
    if (m_value)
      return std::string("yes");
    else
      return std::string("no");
    }
  else
    {
    return std::string(defvalue);
    }
  }

std::string OvmsMetricBool::AsJSON(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    {
    if (m_value)
      return std::string("true");
    else
      return std::string("false");
    }
  else
    {
    if (strtobool(defvalue) == true)
      return std::string("true");
    else
      return std::string("false");
    }
  }

float OvmsMetricBool::AsFloat(const float defvalue, metric_unit_t units)
  {
  return (float)AsBool((bool)defvalue);
  }

int OvmsMetricBool::AsBool(const bool defvalue)
  {
  if (IsDefined())
    return m_value;
  else
    return defvalue;
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
void OvmsMetricBool::DukPush(DukContext &dc, metric_unit_t units)
  {
  dc.Push(m_value);
  }
#endif

bool OvmsMetricBool::SetValue(bool value)
  {
  if (m_value != value)
    {
    m_value = value;
    if (m_valuep)
      *m_valuep = m_value;
    SetModified(true);
    return true;
    }
  else
    {
    SetModified(false);
    return false;
    }
  }

bool OvmsMetricBool::SetValue(std::string value, metric_unit_t units)
  {
  bool nvalue = strtobool(value);
  if (m_value != nvalue)
    {
    m_value = nvalue;
    if (m_valuep)
      *m_valuep = m_value;
    SetModified(true);
    return true;
    }
  else
    {
    SetModified(false);
    return false;
    }
  }

bool OvmsMetricBool::SetValue(dbcNumber& value)
  {
  return SetValue((bool)value.GetUnsignedInteger());
  }

void OvmsMetricBool::Clear()
  {
  SetValue(false);
  OvmsMetric::Clear();
  }

OvmsMetricFloat::OvmsMetricFloat(const char* name, uint16_t autostale, metric_unit_t units, bool persist)
  : OvmsMetric(name, autostale, units, persist)
  {
  m_value = 0.0;
  m_valuep = NULL;
  m_persist = persist;

  if (m_persist)
    {
    persistent_values *vp = pmetrics_register(name);
    if (!vp)
      {
      m_persist = false;
      }
    else
      {
      m_valuep = reinterpret_cast<float*>(&vp->value);
      if (m_value != *m_valuep)
        {
        m_value = *m_valuep;
        SetModified(true);
        ESP_LOGI(TAG, "persist %s = %s", name, AsUnitString().c_str());
        }
      }
    }
  }

OvmsMetricFloat::~OvmsMetricFloat()
  {
  }

bool OvmsMetricFloat::CheckPersist()
  {
  if (!m_persist || !m_valuep || !IsDefined())
    return true;
  if (*m_valuep != m_value)
    {
    ESP_LOGE(TAG, "CheckPersist: bad value for %s", m_name);
    return false;
    }
  persistent_values *vp = pmetrics_find(m_name);
  if (vp == NULL)
    {
    ESP_LOGE(TAG, "CheckPersist: can't find %s", m_name);
    return false;
    }
  if (m_valuep != reinterpret_cast<float*>(&vp->value))
    {
    ESP_LOGE(TAG, "CheckPersist: bad address for %s", m_name);
    return false;
    }
  return true;
  }

void OvmsMetricFloat::RefreshPersist()
  {
  if (m_persist && m_valuep && IsDefined())
    *m_valuep = m_value;
  }

std::string OvmsMetricFloat::AsString(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    {
    std::ostringstream ss;
    if (precision >= 0)
      {
      ss.precision(precision); // Set desired precision
      ss << fixed;
      }
    if ((units != Other)&&(units != m_units))
      ss << UnitConvert(m_units,units,m_value);
    else
      ss << m_value;
    std::string s(ss.str());
    return s;
    }
  else
    {
    return std::string(defvalue);
    }
  }

std::string OvmsMetricFloat::AsJSON(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    return AsString(defvalue, units, precision);
  else
    return std::string((defvalue && *defvalue) ? defvalue : "0");
  }

float OvmsMetricFloat::AsFloat(const float defvalue, metric_unit_t units)
  {
  if (IsDefined())
    {
    if ((units != Other)&&(units != m_units))
      return UnitConvert(m_units,units,m_value);
    else
      return m_value;
    }
  else
    return defvalue;
  }

int OvmsMetricFloat::AsInt(const int defvalue, metric_unit_t units)
  {
  return (int) AsFloat((float) defvalue, units);
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
void OvmsMetricFloat::DukPush(DukContext &dc, metric_unit_t units)
  {
  dc.Push(AsFloat(0, units));
  }
#endif

bool OvmsMetricFloat::SetValue(float value, metric_unit_t units)
  {
  float nvalue = value;
  if ((units != Other)&&(units != m_units)) nvalue=UnitConvert(units,m_units,value);
  if (m_value != nvalue)
    {
    m_value = nvalue;
    if (m_valuep)
      *m_valuep = m_value;
    SetModified(true);
    return true;
    }
  else
    {
    SetModified(false);
    return false;
    }
  }

bool OvmsMetricFloat::SetValue(std::string value, metric_unit_t units)
  {
  float nvalue = atof(value.c_str());
  return SetValue(nvalue, units);
  }

bool OvmsMetricFloat::SetValue(dbcNumber& value)
  {
  return SetValue((float)value.GetDouble());
  }

void OvmsMetricFloat::Clear()
  {
  SetValue(0);
  OvmsMetric::Clear();
  }

OvmsMetricString::OvmsMetricString(const char* name, uint16_t autostale, metric_unit_t units, bool persist)
  : OvmsMetric(name, autostale, units, persist)
  {
  }

OvmsMetricString::~OvmsMetricString()
  {
  }

std::string OvmsMetricString::AsString(const char* defvalue, metric_unit_t units, int precision)
  {
  if (IsDefined())
    {
    OvmsMutexLock lock(&m_mutex);
    return m_value;
    }
  else
    {
    return std::string(defvalue);
    }
  }

#ifdef CONFIG_OVMS_SC_JAVASCRIPT_DUKTAPE
void OvmsMetricString::DukPush(DukContext &dc, metric_unit_t units)
  {
  OvmsMutexLock lock(&m_mutex);
  dc.Push(m_value);
  }
#endif

bool OvmsMetricString::SetValue(std::string value, metric_unit_t units)
  {
  if (m_mutex.Lock())
    {
    bool modified = false;
    if (m_value.compare(value)!=0)
      {
      m_value = value;
      modified = true;
      }
    m_mutex.Unlock();
    SetModified(modified);
    return modified;
    }
  return false;
  }

void OvmsMetricString::Clear()
  {
  SetValue("");
  OvmsMetric::Clear();
  }

/// Get the label for the metric.
const char* OvmsMetricUnitLabel(metric_unit_t units)
  {
  uint8_t unit_i = static_cast<uint8_t>(units);
  if (unit_i > uint8_t(MetricUnitLast))
    return "";
  const char *res = unit_info[unit_i].Label;
  if (res == NULL)
    return "";
  return res;
  }

/// Get the Name for the unit.
const char* OvmsMetricUnitName(metric_unit_t units)
  {
  uint8_t unit_i = static_cast<uint8_t>(units);
  if (unit_i > uint8_t(MetricUnitLast))
    return NULL;
  return unit_info[unit_i].UnitCode;
  }

metric_unit_t OvmsMetricUnitFromName(const char* unit, bool allowUniquePrefix)
  {
  if (unit == NULL || unit[0] == '\0')
    return Native;

  metric_unit_t res = UnitNotFound;
  int unit_len = strlen(unit);
  for (metric_unit_t metric = MetricUnitFirst; metric <= MetricUnitLast; metric = metric_unit_t(1+(uint8_t)metric))
    {
    const char * name = unit_info[(uint8_t)metric].UnitCode;
    if (name == NULL)
      continue;

    if (strncasecmp(name, unit, unit_len) == 0)
      {
      if (unit_len == strlen(name))
        return metric;
      else if (allowUniquePrefix)
        {
        if (res != UnitNotFound)
          return UnitNotFound;
        res = metric;
        }
      }
    }
  return res;
  }

const char *OvmsMetricUnit_FindUniquePrefix(const char* token)
  {

  if (token == NULL || token[0] == '\0')
    return NULL;

  size_t len = strlen(token);
  const char* found = NULL;

  for (metric_unit_t metric = MetricUnitFirst; metric <= MetricUnitLast; metric = metric_unit_t(1+(uint8_t)metric))
    {
    const char * name = unit_info[(uint8_t)metric].UnitCode;
    if (name == NULL)
      continue;

    if (strncasecmp(name, token, len) == 0)
      {
      if (len == strlen(name))
        return name;
      if (found)
        return NULL;
      else
        found = name;
      }
    }
  return found;
  }

bool OvmsMetricUnit_GetCompletion(OvmsWriter* writer, const char* token, metric_group_t group = GrpNone)
  {
  unsigned int index = 0;
  bool match = false;
  writer->SetCompletion(index, NULL);
  if (token)
    {
    size_t len = strlen(token);
    auto group_simple = MetricGroupSimplify(group);

    for (metric_unit_t metric = MetricUnitFirst; metric <= MetricUnitLast; metric = metric_unit_t(1+(uint8_t)metric))
      {
      const char * name = unit_info[(uint8_t)metric].UnitCode;
      if (name == NULL)
        continue;
      if (group != GrpNone )
        {
        auto group_current = MetricGroupSimplify(unit_info[(uint8_t)metric].Group);
        if ( group_current != GrpNone && (group_simple != group_current ))
          continue;
        }
      if (strncasecmp(name, token, len) == 0)
        {
        writer->SetCompletion(index++, name);
        match = true;
        }
      }
    }
  return match;
  }

int OvmsMetricUnit_Validate(OvmsWriter* writer, int argc, const char* token, bool complete, metric_group_t group)
  {
  if (complete)
    {
    if (!OvmsMetricUnit_GetCompletion(writer, token, group))
      return -1;
    }
  else
    {
    if (OvmsMetricUnit_FindUniquePrefix(token) == NULL)
      {
      if (strcmp(token, "?") != 0)
        writer->printf("Error: %s is not defined\n", token);
      return -1;
      }
    }
  return argc;
  }

int UnitConvert(metric_unit_t from, metric_unit_t to, int value)
  {
  CheckTargetUnit(from, to, false);
  if (to == Native)
    return value;

  switch (from)
    {
    case Kilometers:
      switch (to)
        {
        case Miles:   return km_to_mi(value);
        case Meters:  return value*1000;
        case Feet:    return km_to_mi(value) * feet_per_mile;
        default: break;
        }
      break;
    case Miles:
      switch (to)
        {
        case Kilometers: return mi_to_km(value);
        case Meters:     return mi_to_km(value*1000);
        case Feet:       return value * feet_per_mile;
        default: break;
        }
      break;
    case Meters:
      switch (to)
        {
        case Miles:      return km_to_mi(value)/1000;
        case Kilometers: return value/1000;
        case Feet:       return km_to_mi( value * feet_per_mile)/ 1000;
        default: break;
        }
      break;
    case Feet:
      switch (to)
        {
        case Kilometers: return mi_to_km(value)/feet_per_mile;
        case Meters:     return (mi_to_km(value*1000)/feet_per_mile);
        case Miles:      return value / feet_per_mile;
        default: break;
        }
      break;
    case KphPS:
      switch (to)
        {
        case MphPS:     return km_to_mi(value);
        case MetersPSS: return value * 10 /36;
        case FeetPSS:   return km_to_mi(value*feet_per_mile)/3600;
        default: break;
        }
      break;
    case MphPS:
      switch (to)
        {
        case KphPS:     return mi_to_km(value);
        case MetersPSS: return mi_to_km(value*10)/36;
        case FeetPSS:   return value*feet_per_mile/3600;
        default: break;
        }
      break;
    case MetersPSS:
      switch (to)
        {
        case KphPS:     return (value*36) / 10;
        case MphPS:     return km_to_mi(value * 36) / 10;
        case FeetPSS:   return km_to_mi(value*feet_per_mile);
        default: break;
        }
      break;
    case FeetPSS:
      switch (to)
        {
        case KphPS:     return (mi_to_km(value * 36 )/(feet_per_mile*10));
        case MphPS:     return value *3600/feet_per_mile;
        case MetersPSS: return mi_to_km(value/feet_per_mile)*1000;
        default: break;
        }
      break;
    case kW:
      if (to == Watts) return (value*1000);
      break;
    case Watts:
      if (to == kW) return (value/1000);
      break;
    case kWh:
      switch (to)
        {
        case WattHours: return (value*1000);
        case MegaJoules:  return (value*3.6);
        default: break;
        }
    case WattHours:
      switch (to)
        {
        case kWh:       return (value/1000);
        case MegaJoules:  return (value*9/2500);
        default: break;
        }
    case MegaJoules:
      switch (to)
        {
        case kWh: return (value * 5 / 18);
        case WattHours:  return (value * 2500/9);
        default: break;
        }
    case AmpHours:
      switch (to)
        {
        case Kilocoulombs:  return (value * 18) / 5; // * 3600 / 1000
        default: break;
        }
    case Kilocoulombs:
      switch (to)
        {
        case AmpHours:  return (value * 5) / 18; // * 1000 / 3600
        default: break;
        }
    case WattHoursPK:
      switch (to)
        {
        case WattHoursPM: return pkm_to_pmi(value);
        case kWhP100K: return value / 10;
        case KPkWh:    return value ? static_cast<int>(1000.0 / value) : 0;
        case MPkWh:    return value ? static_cast<int>(km_to_mi(1000.0 / value)) : 0;
        default: break;
        }
      break;
    case WattHoursPM:
      switch (to)
        {
        case WattHoursPK: return pmi_to_pkm(value);
        case kWhP100K: return pmi_to_pkm(value) / 10;
        case KPkWh:    return value ? static_cast<int>(mi_to_km(1000.0 / value)) : 0;
        case MPkWh:    return value ? static_cast<int>(1000.0 / value) : 0;
        default: break;
        }
      break;
    case kWhP100K:
      switch (to)
        {
        case WattHoursPM: return pkm_to_pmi(value * 10);
        case WattHoursPK: return value * 10;
        case KPkWh:       return value ? static_cast<int>(100.0 / value) : 0;
        case MPkWh:       return value ? static_cast<int>(km_to_mi(100.0 / value)) : 0;
        default: break;
        }
      break;
    case KPkWh:
      switch (to)
        {
        case WattHoursPM: return value ? static_cast<int>(1000.0 / km_to_mi(float(value))) : 0;
        case WattHoursPK: return value ? static_cast<int>(1/(1000.0 * value)) : 0;
        case kWhP100K:    return value ? static_cast<int>(100.0/value) : 0;
        case MPkWh:       return km_to_mi(value);
        default: break;
        }
      break;
    case MPkWh:
      switch (to)
        {
        case WattHoursPM: return value ? 1000/value : 0;
        case WattHoursPK: return value ? static_cast<int>(1000 / mi_to_km(float(value))) : 0;
        case kWhP100K:    return value ? static_cast<int>(100.0/mi_to_km(float(value))) : 0;
        case KPkWh:       return mi_to_km(value);
        default: break;
        }
      break;
    case Celcius:
      if (to == Fahrenheit) return ((value*9)/5) + 32;
      break;
    case Fahrenheit:
      if (to == Celcius) return ((value-32)*5)/9;
      break;
    case kPa:
      switch (to)
        {
        case Pa:  return value*1000;
        case Bar: return value/100;
        case PSI: return int((float)value * 0.14503773773020923);
        default: break;
        }
      break;
    case Pa:
      switch (to)
        {
        case kPa: return value/1000;
        case Bar: return value/100000;
        case PSI: return int((float)value * 0.00014503773773020923);
        default: break;
        }
      break;
    case PSI:
      switch (to)
        {
        case kPa: return int((float)value * 6.894757293168361);
        case Pa:  return int((float)value * 6894.757293168361);
        case Bar: return int((float)value * 0.06894757293168361);
        default: break;
        }
      break;
    case Bar:
      switch (to)
        {
        case Pa:  return value*100000;
        case kPa: return value*100;
        case PSI: return int((float)value * 14.503773773020923);
        default: break;
        }
      break;
    case Seconds:
      if (to == Minutes) return value/60;
      else if (to == Hours) return value/3600;
      break;
    case Minutes:
      if (to == Seconds || to == TimeUTC || to == TimeLocal) return value*60;
      else if (to == Hours) return value/60;
      break;
    case Hours:
      if (to == Seconds || to == TimeUTC || to == TimeLocal) return value*3600;
      else if (to == Minutes) return value*60;
      break;
    case TimeUTC:
      if (to == TimeLocal)
        {
        time_t now;
        time(&now);
        now -= now % (24*60*60);        // Back to midnight UTC
        now += value;                   // The target time today
        struct tm* tmu = localtime(&now);
        return (tmu->tm_hour * 60 + tmu->tm_min) * 60 + tmu->tm_sec;
        }
      else if (to == Minutes) return value/60;
      else if (to == Hours) return value/3600;
      break;
    case Kph:
      switch (to)
        {
        case Mph: return km_to_mi(value);
        case MetersPS: return value * 5 / 18; // 1000/3600 
        case FeetPS: return km_to_mi(value* feet_per_mile)/3600;
        default: break;
        }
    case Mph:
      switch (to)
        {
        case Kph:    return  mi_to_km(value);
        case FeetPS: return value * feet_per_mile / 3600;
        case MetersPS: return mi_to_km(value * 5 ) / 18;
        default: break;
        }
    case MetersPS:
      switch (to)
        {
        case Mph: return km_to_mi(value * 18) / 5; //  3600/ 1000
        case Kph: return value * 18 / 5; // 3600/1000
        case FeetPS: return km_to_mi(value* feet_per_mile) / 1000;
        default: break;
        }
    case FeetPS:
      switch (to)
        {
        case Kph: return  mi_to_km(value) / feet_per_mile;
        case Mph:  return value * 3600 / feet_per_mile;
        case MetersPS: return mi_to_km(value * 1000 ) / feet_per_mile;
        default: break;
        }
    case dbm:
      if (to == sq) return (value <= -51) ? ((value + 113)/2) : 0;
      break;
    case sq:
      if (to == dbm) return (value <= 31) ? (-113 + (value*2)) : 0;
      break;
    case Percentage:
      if (to == Permille) return value*10;
      break;
    case Permille:
      if (to == Percentage) return value/10;
      break;
    default:
      return value;
    }
  return value;
  }

float UnitConvert(metric_unit_t from, metric_unit_t to, float value)
  {
  CheckTargetUnit(from, to, false);
  if (to == Native)
    return value;

  switch (from)
    {
    case Kilometers:
      switch (to)
        {
        case Miles:   return km_to_mi(value);
        case Meters:  return value*1000;
        case Feet:    return km_to_mi(value) * feet_per_mile;
        default: break;
        }
      break;
    case Miles:
      switch (to)
        {
        case Kilometers: return mi_to_km(value);
        case Meters:     return (mi_to_km(value)*1000);
        case Feet:       return value * feet_per_mile;
        default: break;
        }
      break;
    case Meters:
      switch (to)
        {
        case Miles:       return km_to_mi(value/1000);
        case Kilometers:  return value/1000;
        case Feet:        return km_to_mi(value/1000) * feet_per_mile;
        default: break;
        }
      break;
    case Feet:
      switch (to)
        {
        case Kilometers: return mi_to_km(value/feet_per_mile);
        case Meters:     return (mi_to_km(value/feet_per_mile)*1000);
        case Miles:      return value / feet_per_mile;
        default: break;
        }
      break;
    case KphPS:
      switch (to)
        {
        case MphPS:     return km_to_mi(value);
        case MetersPSS: return value/3.6;
        case FeetPSS:   return km_to_mi(value)*feet_per_mile/3600;
        default: break;
        }
      break;
    case MphPS:
      switch (to)
        {
        case KphPS:     return mi_to_km(value);
        case MetersPSS: return mi_to_km(value)/3.6;
        case FeetPSS:   return value*feet_per_mile/3600;
        default: break;
        }
      break;
    case MetersPSS:
      switch (to)
        {
        case KphPS:     return (value*3.6);
        case MphPS:     return km_to_mi(value)*3.6;
        case FeetPSS:   return km_to_mi(value)*feet_per_mile;
        default: break;
        }
      break;
    case FeetPSS:
      switch (to)
        {
        case KphPS:     return (mi_to_km(value/feet_per_mile)*3.6);
        case MphPS:     return value *3600/feet_per_mile;
        case MetersPSS: return mi_to_km(value/feet_per_mile)*1000;
        default: break;
        }
      break;
    case kW:
      if (to == Watts) return (value*1000);
      break;
    case Watts:
      if (to == kW) return (value/1000);
      break;

    case kWh:
      switch (to)
        {
        case WattHours: return (value*1000);
        case MegaJoules:  return (value*3.6);
        default: break;
        }
    case WattHours:
      switch (to)
        {
        case kWh:       return (value*0.001);
        case MegaJoules:  return (value*0.0036);
        default: break;
        }
    case MegaJoules:
      switch (to)
        {
        case kWh: return (value * 2.777778);
        case WattHours:  return (value * 277.7778);
        default: break;
        }
    case AmpHours:
      switch (to)
        {
        case Kilocoulombs:  return value * 3.6; // * 3600 / 1000
        default: break;
        }
    case Kilocoulombs:
      switch (to)
        {
        case AmpHours:  return value * 0.277778; // * 1000 / 3600
        default: break;
        }
    case WattHoursPK:
      switch (to)
        {
        case WattHoursPM: return pkm_to_pmi(value);
        case kWhP100K:    return value / 10;
        case KPkWh:       return value ? 1000.0 / value : 0;
        case MPkWh:       return value ? (km_to_mi(1000.0 / value)) : 0;
        default: break;
        }
      break;
    case WattHoursPM:
      switch (to)
        {
        case WattHoursPK: return pmi_to_pkm(value);
        case kWhP100K:    return pmi_to_pkm(value) / 10;
        case KPkWh:       return value ? (mi_to_km(1000.0 / value)) : 0;
        case MPkWh:       return value ? (1000.0 / value) : 0;
        default: break;
        }
      break;
    case kWhP100K:
      switch (to)
        {
        case WattHoursPM: return pkm_to_pmi(value * 10);
        case WattHoursPK: return value * 10;
        case KPkWh:       return value ? (100.0 / value) : 0;
        case MPkWh:       return value ? km_to_mi(100.0 / value) : 0;
        default: break;
        }
      break;
    case KPkWh:
      switch (to)
        {
        case WattHoursPM: return value ? (1000.0 / km_to_mi(value)) : 0;
        case WattHoursPK: return value ? (0.001 / value) : 0;
        case kWhP100K:    return value ? (100.0 / value) : 0;
        case MPkWh:       return km_to_mi(value);
        default: break;
        }
      break;
    case MPkWh:
      switch (to)
        {
        case WattHoursPM: return value ? 1000/value : 0;
        case WattHoursPK: return value ? (1000 / mi_to_km(value)) : 0;
        case kWhP100K:    return value ? (100.0/mi_to_km(value)) : 0;
        case KPkWh:       return mi_to_km(value);
        default: break;
        }
      break;
    case Celcius:
      if (to == Fahrenheit) return ((value*9)/5) + 32;
      break;
    case Fahrenheit:
      if (to == Celcius) return ((value-32)*5)/9;
      break;
    case kPa:
      switch (to)
        {
        case Pa:  return value*1000;
        case Bar: return value/100;
        case PSI: return value * 0.14503773773020923;
        default: break;
        }
      break;
    case Pa:
      switch (to)
        {
        case kPa: return value/1000;
        case Bar: return value/100000;
        case PSI: return value * 0.00014503773773020923;
        default: break;
        }
      break;
    case PSI:
      switch (to)
        {
        case kPa: return value * 6.894757293168361;
        case Pa:  return value * 6894.757293168361;
        case Bar: return value * 0.06894757293168361;
        default: break;
        }
      break;
    case Bar:
      switch (to)
        {
        case Pa:  return value*100000;
        case kPa: return value*100;
        case PSI: return value * 14.503773773020923;
        default: break;
        }
      break;
    case Seconds:
      if (to == Minutes) return value/60;
      else if (to == Hours) return value/3600;
      break;
    case Minutes:
      if (to == Seconds) return value*60;
      else if (to == Hours) return value/60;
      break;
    case Hours:
      if (to == Seconds) return value*3600;
      else if (to == Minutes) return value*60;
      break;
    case Kph:
      switch (to)
        {
        case Mph: return km_to_mi(value);
        case MetersPS: return value * 0.277778; // 1000/3600 
        case FeetPS: return km_to_mi(value* feet_per_mile)/3600;
        default: break;
        }
    case Mph:
      switch (to)
        {
        case Kph:    return  mi_to_km(value);
        case FeetPS: return value * feet_per_mile / 3600;
        case MetersPS: return mi_to_km(value) * 0.277778; // 1000/36000
        default: break;
        }
    case MetersPS:
      switch (to)
        {
        case Mph: return km_to_mi(value ) * 3.6; //  3600/ 1000
        case Kph: return value * 3.6; // 3600/1000
        case FeetPS: return km_to_mi(value* feet_per_mile) / 1000;
        default: break;
        }
    case FeetPS:
      switch (to)
        {
        case Kph: return  mi_to_km(value) / feet_per_mile;
        case Mph:  return value * 3600 / feet_per_mile;
        case MetersPS: return mi_to_km(value * 1000 ) / feet_per_mile;
        default: break;
        }
    case dbm:
      if (to == sq) return int((value <= -51) ? ((value + 113)/2) : 0);
      break;
    case sq:
      if (to == dbm) return int((value <= 31) ? (-113 + (value*2)) : 0);
      break;
    case Percentage:
      if (to == Permille) return value*10.0;
      break;
    case Permille:
      if (to == Percentage) return value*0.10;
      break;
    default:
      return value;
    }
  return value;
  }

// Construct On First Use instantiation 
UnitConfigMap &UnitConfigMap::instance(const char* caller) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
        ESP_LOGI(TAG, "COFU by %s", caller);
    }
    static UnitConfigMap _instance;
    return _instance;
}


UnitConfigMap::UnitConfigMap()
  {
  for (auto it = m_modified.begin(); it != m_modified.end(); ++it)
    *it = 0;
  for (auto it = m_map.begin(); it != m_map.end(); ++it)
    *it = UnitNotFound;
  OvmsMetricGroupConfigList(config_groups);

#ifdef bind
  #undef bind  // Kludgy, but works
#endif
  using std::placeholders::_1;
  using std::placeholders::_2;
  OvmsEvents::instance(UCM).RegisterEvent(TAG, "config.changed",
      std::bind(&UnitConfigMap::ConfigEventListener, this, _1, _2));
  OvmsEvents::instance(UCM).RegisterEvent(TAG, "config.mounted",
      std::bind(&UnitConfigMap::ConfigMountedListener, this, _1, _2));
  }

void UnitConfigMap::Load()
  {
  OvmsMutexLock store_lock(&m_store_lock);

  // Fill the groups with a user configurable list
  for (auto grpit = config_groups.begin(); grpit != config_groups.end(); ++grpit)
    {
    uint8_t igrp = static_cast<uint8_t>(*grpit);
    if (igrp < m_map.size())
      {
      auto newValue = OvmsMetricGetUserUnit(*grpit);
      if (m_map[igrp] != newValue)
        {
        m_map[igrp] = newValue;
        switch (*grpit)
          {
          case GrpNone:
          case GrpOther: break;
          default:
            OvmsMetrics::instance(UCM).SetAllGroupUnitSend(*grpit);
            m_modified[igrp] = ULONG_MAX;
          }
        }
      }
    }
  }

void UnitConfigMap::InitialiseSlot(size_t modifier)
  {
  unsigned long bit = 1ul << modifier;
  for (auto it = m_modified.begin(); it != m_modified.end(); ++it)
    *it |= bit;
  }

metric_unit_t UnitConfigMap::GetUserUnit( metric_group_t group, metric_unit_t defaultUnit )
  {
  uint8_t groupint = static_cast<uint8_t>(group);
  if (groupint >= m_map.size())
    return defaultUnit;
  OvmsMutexLock store_lock(&m_store_lock);
  auto res = m_map[groupint];
  switch (res) {
    case UnitNotFound: return defaultUnit;
    case Native: return (defaultUnit == UnitNotFound) ? Native : defaultUnit;
    default: return res;
  }
  }

metric_unit_t UnitConfigMap::GetUserUnit( metric_unit_t unit)
  {
  metric_group_t grp = GetMetricGroup(unit);
  if (grp == GrpNone || grp == GrpOther)
    return UnitNotFound;
  return GetUserUnit(grp);
  }

void UnitConfigMap::ConfigEventListener(std::string event, void* data)
  {
  if (data == NULL)
    return;
  OvmsConfigParam *config = (OvmsConfigParam *)data;
  if (config->GetName() == "vehicle")
    Load();
  }

void UnitConfigMap::ConfigMountedListener(std::string event, void* data)
  {
  Load();
  }

void UnitConfigMap::ConfigList(metric_group_list_t& groups)
  {
  groups.insert(groups.begin(), config_groups.begin(), config_groups.end());
  }

bool UnitConfigMap::HasModified(size_t modifier)
  {
  for (auto it = config_groups.begin(); it != config_groups.end(); ++it)
    if (IsModified(*it, modifier))
      return true;
  return false;
  }

bool UnitConfigMap::IsModified( metric_group_t group, size_t modifier)
  {
  uint8_t groupint = static_cast<uint8_t>(group);
  if (groupint >= m_modified.size())
    return false;
   return m_modified[groupint] & (1ul << modifier);
  }

bool UnitConfigMap::IsModifiedAndClear(metric_group_t group, size_t modifier)
  {
  uint8_t groupint = static_cast<uint8_t>(group);
  if (groupint >= m_modified.size())
    return false;
  unsigned long bit = 1ul << modifier;
  unsigned long mod = m_modified[groupint].fetch_and(~bit);
  return mod & bit;
  }
