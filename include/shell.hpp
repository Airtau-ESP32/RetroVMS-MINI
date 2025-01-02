#pragma once

#include "esp_http_server.h"
#include <esp_task_wdt.h>
#include "esp32m/app.hpp"
#include "esp32m/logging.hpp"
#include "ovms_command.h"
#include "global.h"


extern httpd_handle_t* httpServer;

#define TOKEN_MAX_LENGTH 32
#define COMPLETION_MAX_TOKENS 20

namespace esp32m {
  namespace ui {
    class Shell : public OvmsWriter , public AppObject {
     public:
      Shell(const Shell &) = delete;
      static Shell &instance();      
      const char *name() const override {
        return "shell";
      }
      int verbosity;
      virtual void SetSecure(bool secure=true);
      int puts(const char* s);
      ssize_t write(const void *buf, size_t len);
      int printf(const char* fmt, ...) __attribute__ ((format (printf, 2, 3))); 
      char** SetCompletion(int index, const char* token);

     protected:
      void handleEvent(Event &ev) override;
      char *m_completions[COMPLETION_MAX_TOKENS+2];
      char m_space[COMPLETION_MAX_TOKENS+2][TOKEN_MAX_LENGTH];

     private:
      Shell(httpd_handle_t* server);
      void run();
      TaskHandle_t _task = nullptr;
      bool _inited = false;
      bool init();
      httpd_handle_t* _server;
    };

    void useShell();

  }  // namespace ui
}  // namespace esp32m
