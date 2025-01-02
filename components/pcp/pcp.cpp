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
static const char *TAG = "pcp";

#include <stdio.h>
#include <string.h>
#include "pcp.h"
#include "ovms_events.h"

OvmsCommand* powercmd = NULL;

void power_cmd(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  const char* devname = cmd->GetParent()->GetName();
  const char* pmname = cmd->GetName();

  pcp* device = pcpapp::instance().FindDeviceByName(devname);
  PowerMode pm = pcpapp::instance().FindPowerModeByName(pmname);
  if ((device == NULL)||(pm == PowerMode::Undefined))
    {
    writer->puts("Internal error: finding device or power mode name failed");
    return;
    }

  device->SetPowerMode(pm);
  writer->printf("Power mode of %s is now %s\n",devname,pmname);
  }

void power_status(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  const char* devname = cmd->GetParent()->GetName();

  pcp* device = pcpapp::instance().FindDeviceByName(devname);
  const char* pmname = pcpapp::instance().FindPowerModeByType(device->GetPowerMode());

  writer->printf("Power for %s is %s\n",devname,pmname);
  }

// Construct On First Use instantiation
pcpapp &pcpapp::instance(const char* caller) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
        ESP_LOGI(TAG, "COFU by %s", caller);
    }
    static pcpapp _instance;
    return _instance;
}

pcpapp::pcpapp()
  {
  ESP_LOGI(TAG, "Initialising POWER");

  m_mappm["on"] = On;
  m_mappm["sleep"] = Sleep;
  m_mappm["deepsleep"] = DeepSleep;
  m_mappm["off"] = Off;
  m_mappm["devel"] = Devel;
  powercmd = OvmsCommandApp::instance(TAG).RegisterCommand("power","Power control",NULL,"$C $G$");
  }


void pcpapp::Register(const char* name, pcp* device)
  {
  m_map[name] = device;
  OvmsCommand* devcmd = powercmd->RegisterCommand(name,"Power control");
  for (auto it=m_mappm.begin(); it!=m_mappm.end(); ++it)
    {
    devcmd->RegisterCommand(it->first,"Power control",power_cmd);
    }
  devcmd->RegisterCommand("status","Power control status",power_status);
  }

void pcpapp::Deregister(const char* name)
  {
  m_map.erase(name);
  }

pcp* pcpapp::FindDeviceByName(const char* name)
  {
  auto iter = m_map.find(name);
  if (iter != m_map.end())
    {
    return iter->second;
    }
  return NULL;
  }

PowerMode pcpapp::FindPowerModeByName(const char* name)
  {
  auto iter = m_mappm.find(name);
  if (iter != m_mappm.end())
    {
    return iter->second;
    }
  return PowerMode::Undefined;
  }

const char* pcpapp::FindPowerModeByType(PowerMode mode)
  {
  const char* pmname= NULL;
  for (auto it=m_mappm.begin(); it!=m_mappm.end(); ++it)
    {
    if (it->second == mode)
      pmname = it->first;
    }
  if (pmname == NULL) pmname = "undefined";
  return pmname;
  }

pcp::pcp(const char* name)
  {
  m_name = name;
  m_powermode = On;
  pcpapp::instance().Register(name,this);
  }

pcp::~pcp()
  {
  pcpapp::instance().Deregister(m_name);
  }

void pcp::SetPowerMode(PowerMode powermode)
  {
  if (m_powermode != powermode)
    {
    m_powermode = powermode;
    const char* pmname = pcpapp::instance().FindPowerModeByType(m_powermode);
    char event[32] = "power.";
    strcat(event,m_name);
    strcat(event,".");
    strcat(event,pmname);
    OvmsEvents::instance(TAG).SignalEvent(event,NULL);
    }
  }

PowerMode pcp::GetPowerMode()
  {
  return m_powermode;
  }

const char* pcp::GetName()
  {
  return m_name;
  }
