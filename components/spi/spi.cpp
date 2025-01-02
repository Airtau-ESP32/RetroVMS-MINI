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

#include <string.h>
#include <cstdarg>
#include "spi.h"

static const char *TAG = "spi";


spi::spi(const char* name, spi_host_device_t host, int misopin, int mosipin, int clkpin)
  : pcp(name)
  {
  ESP_LOGI(TAG, "Initializing %s bus", name);
  m_mtx = xSemaphoreCreateMutex();
  spi_bus_config_t m_buscfg = {};
  m_buscfg.miso_io_num=misopin;
  m_buscfg.mosi_io_num=mosipin;
  m_buscfg.sclk_io_num=clkpin;
  m_buscfg.quadwp_io_num=-1;
  m_buscfg.quadhd_io_num=-1;
  m_buscfg.max_transfer_sz = (4 * 8);
  m_host = host;   
  esp_err_t ret = spi_bus_initialize(host, &m_buscfg, 0);    
  m_initialized = true;
  }

spi::~spi()
  {
  }

bool spi::LockBus(TickType_t delay)
  {
  return (xSemaphoreTake(m_mtx, delay) == pdTRUE);
  }

void spi::UnlockBus()
  {
  xSemaphoreGive(m_mtx);
  }
uint8_t* spi::spi_cmd(spi_device_handle_t spi, uint8_t* buf, int rxlen, int txlen, ...)
  {
  
  va_list args;
  esp_err_t ret;

  memset(buf,0,rxlen+txlen);

  va_start(args, txlen);
  for (int k=0; k<txlen; k++)
    {
    buf[k] = va_arg(args,int);
    }
  va_end(args);

  spi_transaction_t t;
  memset(&t, 0, sizeof(t));       //Zero out the transaction
  t.length=(txlen+rxlen)*8;       // Transmit length (in bits)
  // We are using spi_master_no_dma in full-duplex mode (even if below we return only the part received after transmitted bytes == half-duplex),
  // so here we configure the library to copy all the data SPI registers during transaction (full-duplex)
  t.rxlength=(txlen+rxlen)*8;   // Receive length (in bits) 
  t.tx_buffer=buf;                // Buffer to send
  t.rx_buffer=buf;                // Buffer to receive
  t.user=(void*)0;                // D/C needs to be set to 0
  if (LockBus(portMAX_DELAY))
    {
   
    ret=spi_device_polling_transmit(spi, &t);  // Transmit!

    assert(ret==ESP_OK);            // Should have had no issues.
    UnlockBus();
    }
  return buf + txlen; // return only the data received after tx (half-duplex)
  }


