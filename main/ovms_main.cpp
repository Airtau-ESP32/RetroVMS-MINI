#include "ovms_log.h"
static const char *TAG = "main";
#include "Arduino.h"
#include <string>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "ovms_peripherals.h"
#include "ovms_events.h"
#include "ovms_config.h"
#include <esp_task_wdt.h>


#include <esp32m/app.hpp>
#include <esp32m/net/ota.hpp>
#include <esp32m/net/wifi.hpp>
#include <esp32m/net/sntp.hpp>
#include <esp32m/net/interfaces.hpp>
#include <esp32m/net/mqtt.hpp>
#include "esp32m/ui/mqtt.hpp"
#include <esp32m/dev/esp32.hpp>
#include <esp32m/dev/buzzer.hpp>
#include <esp32m/dev/rmtled.hpp>
#include <esp32m/debug/tasks.hpp>
#include <esp32m/bus/scanner/i2c.hpp>
#include <esp32m/debug/pins.hpp>
// #include <esp32m/debug/button.hpp>
#include <esp32m/log/mqtt.hpp>
#include <esp32m/log/console.hpp>
#include <esp32m/log/udp.hpp>
#include <esp32m/ui.hpp>
#include <esp32m/ui/httpd.hpp>
#include <esp32m/ui/console.hpp>

#include <dist/ui.hpp>

#include "ovms_time.h"
#include "ovms_housekeeping.h"
#include "ovms_events.h"

#include "shell.hpp"
#include "sdcard.hpp"
#include "ovms_v3.hpp"
#include "cellular.hpp"

// All ESP32 manager code is in under esp32m namespace
using namespace esp32m;

const char *appName = "RetroVMS";
const char *appVersion = "0.8.0";

httpd_handle_t* httpServer = nullptr;

extern "C" void app_main()
  {
// Declare an instance of App::Init and pass the name and version
App::Init app(appName, appVersion);

app.inferHostname();

  // Send log messages to default UART
  log::addAppender(&log::Console::instance());

  // MQTT appender, sends the messages to MQTT broker:
  // log::addBufferedAppender(&log::Mqtt::instance());

  // Send log messages to rsyslog daemon over UDP.
  // It will look for the syslog.lan name on the local network.
  // If you want to specify a different name or IP address - just pass it to the
  // Udp() constructor
  // log::addBufferedAppender(new log::Udp());

  // Queue messages instead of sending them right away. Consumes slightly more
  // RAM, but doesn't block current thread, the messages are being sent
  // concurrently.
  log::useQueue();
  // Redirect ESP-IDF messages to our logger so we can send them over to rsyslog
  // and/or other medium.
  log::hookEsp32Logger();
  // Add support for Over The Air firmware updates.
  net::useWifi();
  // Replace SSID/password with your WiFi credentials if you would like to hard-code AP credentials
  // If SSID/password is not set, ESP32 will start in Access Point mode and you will be able configure
  // WiFi connection via captive portal
  // net::wifi::addAccessPoint("SSID", "password");
  net::useCellular();

  net::useSntp();
  net::useInterfaces();

  // Retrieve various pieces of info about ESP32 module to be displayed in the
  // UI.
  dev::useEsp32();

  auto apiTransport = new ui::Httpd();
  // Start embedded HTTP server for UI. 
  initUi(new Ui(apiTransport));

  // when using localhost to serve UI, only start ws server on device
  // and run  'yarn start' in ../build/webui to serve web app
  // new Ui(apiTransport);
 

  // new Ui(&ui::Console::instance());
  dev::useSdcard();
  dev::useOvmsV3();
  dev::useBuzzer(gpio_num_t(CONFIG_BUZZER));
  dev::useRmtled(gpio_num_t(CONFIG_RMT_LED));
  net::useMqtt();
  
  // enable UI for MQTT
  new Ui(&ui::Mqtt::instance());
  
  //assign server handle to global for shell
  httpServer = apiTransport->server();

  ui::useShell();
  bus::scanner::useI2C();
  debug::useTasks();
  // debug::usePins();


//***********************************************************************************************
//                                    START RETROVMS
//***********************************************************************************************

  OvmsConfig::instance(TAG).mount();
  
  OvmsCommandApp::instance(TAG).ConfigureLogging();

  logi("Registering default configs...");
  OvmsConfig::instance(TAG).RegisterParam("vehicle", "Vehicle", true, true);

  // Peripherals* MyPeripherals = NULL;

  ESP_LOGI(TAG, "Running on CPU%d",xPortGetCoreID());
  OvmsEvents::instance(TAG);
  logi("Starting HOUSEKEEPING...");
  Housekeeping::instance(TAG);
  
  }
