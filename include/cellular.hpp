#pragma once

#include "esp32m/device.hpp"
#include "esp32m/events.hpp"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "cxx_include/esp_modem_api.hpp"

#include "esp_modem_config.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_modem_dce_wrap esp_modem_dce_t;
}

namespace esp32m {
  namespace net {

    class Cellular;

    class PppEvent : public Event {
     public:
      ip_event_t event() const { return _event; }
      Cellular *cellular() { return _cellular; }

      bool is(ip_event_t event) const;

      static bool is(Event &ev, PppEvent **r);
      static bool is(Event &ev, ip_event_t event, PppEvent **r = nullptr);

      static void publish(ip_event_t event);

     protected:
      constexpr static const char *Type = "cellular";

     private:
      ip_event_t _event;
      Cellular *_cellular = nullptr;
      PppEvent(ip_event_t event): Event(Type), _event(event) {}
      void claim(Cellular *e) { _cellular = e; }
      friend class Cellular;
    };

    class Cellular : public Device {
     public:
      Cellular(const Cellular &) = delete;
      Cellular(const char *name);
      ~Cellular();
      const char *name() const override {
        return _name;
      }
      esp_netif_t *netif() const {
        return _ppp_if;
      }

     protected:
      void handleEvent(Event &ev) override;

     private:
      const char *_name;
      uint8_t _macAddr[6] = {0};
      bool _ready = false;
      esp_netif_t *_ppp_if = nullptr;
      esp_event_handler_instance_t _gotIpHandle = nullptr,
                                   _lostIpHandle = nullptr;
      esp_err_t start();
      esp_err_t stop();
    };

    Cellular *useCellular(const char *name = nullptr);

  }  // namespace net
}  // namespace esp32m