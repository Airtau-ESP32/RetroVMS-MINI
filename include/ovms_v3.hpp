#pragma once

#include <hal/gpio_types.h>

#include "esp32m/device.hpp"
#include "esp32m/events/request.hpp"
#include "esp32m/io/pins.hpp"
#include <esp32m/io/gpio.hpp>
#include <string>
#include "ovms_metrics.h"
#include "ovms_config.h"
#include "id_include_exclude_filter.h"

extern uint32_t monotonictime;

typedef std::map<std::string, uint32_t> OvmsV3ClientMap;

namespace esp32m {
  namespace dev {

    class OvmsV3 : public AppObject {
     public:
      enum class State { Disconnected = 0, Connected = 1 };
      OvmsV3(const OvmsV3 &) = delete;
      
      static OvmsV3 &instance();
      const char *name() const override {
        return "ovmsv3";
      }

     public:
      void EventListener(std::string event, void* data);
      void CountClients();
      void Ticker1(std::string event, void* data);
      void Ticker60(std::string event, void* data);
      void ConfigChanged(OvmsConfigParam* param);
      void MetricModified(OvmsMetric* metric);
      void TransmitAllMetrics();
      void TransmitModifiedMetrics();


    public:
      static const char *toString(State s);
      std::string _topic_prefix;
      int _lasttx;
      int _peers;
      int _lasttx_stream;
      int _lasttx_sendall;
      int _streaming;
      int _updatetime_idle;
      int _updatetime_connected;
      int _updatetime_awake;
      int _updatetime_on;
      int _updatetime_charging;
      int _updatetime_sendall;
      bool _sendall;
      bool _notify_info_pending;
      bool _notify_error_pending;
      bool _notify_alert_pending;
      bool _notify_data_pending;
      int _notify_data_waitcomp;
      OvmsV3ClientMap _clients;

    protected:
      DynamicJsonDocument *getState(const JsonVariantConst args) override;
      bool handleRequest(Request &req) override;
      void handleEvent(Event &ev) override;


     private:
      OvmsV3(); 
      void TransmitMetric(OvmsMetric* metric);
      void setState(State state);
      IdIncludeExcludeFilter _metrics_filter;
      State _state = State::Disconnected;
    };

    OvmsV3 *useOvmsV3();

  }  // namespace dev
}  // namespace esp32m