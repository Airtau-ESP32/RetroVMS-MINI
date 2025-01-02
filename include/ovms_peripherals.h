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

#ifndef __PERIPHERALS_H__
#define __PERIPHERALS_H__

#include "pcp.h"
#include "spi.h"

#ifdef CONFIG_OVMS_COMP_ADC
#include "esp32adc.h"
#endif // #ifdef CONFIG_OVMS_COMP_ADC

#ifdef CONFIG_OVMS_COMP_MCP2515
#include "mcp2515.h"
#endif // #ifdef CONFIG_OVMS_COMP_MCP2515

#ifdef CONFIG_OVMS_COMP_EXTERNAL_SWCAN
#include "swcan.h"
#endif // #ifdef CONFIG_OVMS_COMP_EXTERNAL_SWCAN

#ifdef CONFIG_OVMS_COMP_ESP32CAN
#include "esp32can.h"
#endif // #ifdef CONFIG_OVMS_COMP_ESP32CAN

#ifdef CONFIG_OVMS_COMP_MAX7317
#include "max7317.h"
#endif // #ifdef CONFIG_OVMS_COMP_MAX7317

#ifdef CONFIG_OVMS_COMP_BLUETOOTH
#include "esp32bluetooth.h"
#endif // #ifdef CONFIG_OVMS_COMP_BLUETOOTH

#ifdef CONFIG_OVMS_COMP_WIFI
#include "esp32wifi.h"
#endif // #ifdef CONFIG_OVMS_COMP_WIFI

#ifdef CONFIG_OVMS_COMP_SDCARD
#include "sdcard.h"
#endif // #ifdef CONFIG_OVMS_COMP_SDCARD

#ifdef CONFIG_OVMS_COMP_CELLULAR
#include "ovms_cellular.h"
#endif // #ifdef CONFIG_OVMS_COMP_CELLULAR

#ifdef CONFIG_OVMS_COMP_OBD2ECU
#include "obd2ecu.h"
#endif // #ifdef CONFIG_OVMS_COMP_OBD2ECU

#ifdef CONFIG_OVMS_COMP_EXT12V
#include "ext12v.h"
#endif // #ifdef CONFIG_OVMS_COMP_EXT12V

#define MODULE_GPIO_SW2           0       // SW2: firmware download / factory reset


class Peripherals 
  {
  public:
    Peripherals();
    ~Peripherals();

    spi* m_spibus;

#ifdef CONFIG_OVMS_COMP_MAX7317
    max7317* m_max7317;
#endif // #ifdef CONFIG_OVMS_COMP_MAX7317

#ifdef CONFIG_OVMS_COMP_ESP32CAN
    esp32can* m_esp32can;
#endif // #ifdef CONFIG_OVMS_COMP_ESP32CAN

#ifdef CONFIG_OVMS_COMP_WIFI
    esp32wifi* m_esp32wifi;
#endif // #ifdef CONFIG_OVMS_COMP_WIFI

#ifdef CONFIG_OVMS_COMP_BLUETOOTH
    esp32bluetooth* m_esp32bluetooth;
#endif // #ifdef CONFIG_OVMS_COMP_BLUETOOTH

#ifdef CONFIG_OVMS_COMP_ADC
    esp32adc* m_esp32adc;
#endif // #ifdef CONFIG_OVMS_COMP_ADC

#ifdef CONFIG_OVMS_COMP_MCP2515
    mcp2515* m_mcp2515_1;
    mcp2515* m_mcp2515_2;
#endif // #ifdef CONFIG_OVMS_COMP_MCP2515

#ifdef CONFIG_OVMS_COMP_EXTERNAL_SWCAN
    swcan* m_mcp2515_swcan;
#endif // #ifdef CONFIG_OVMS_COMP_EXTERNAL_SWCAN

#ifdef CONFIG_OVMS_COMP_SDCARD
    sdcard* m_sdcard;
#endif // #ifdef CONFIG_OVMS_COMP_SDCARD

#ifdef CONFIG_OVMS_COMP_CELLULAR
    modem* m_cellular_modem;
#endif // #ifdef CONFIG_OVMS_COMP_CELLULAR

#ifdef CONFIG_OVMS_COMP_OBD2ECU
    obd2ecu* m_obd2ecu;
#endif // #ifdef CONFIG_OVMS_COMP_OBD2ECU

#ifdef CONFIG_OVMS_COMP_EXT12V
    ext12v* m_ext12v;
#endif // #ifdef CONFIG_OVMS_COMP_EXT12V
  };

#endif //#ifndef __PERIPHERALS_H__
