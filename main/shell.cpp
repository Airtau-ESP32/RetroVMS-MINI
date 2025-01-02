#include "shell.hpp"
#include "esp32m/events.hpp"
#include <esp32m/ui/httpd.hpp>
#include "esp32m/defs.hpp"


#include "Shellminator.hpp"
#include "Shellminator-IO.hpp"
#include "Shellminator-Browser-Response.hpp" // <- It contains the webpage data

#include "esp32m/logging.hpp"


static const char *TAG = "shell";

#define SHELL_PREFIX "/app/shell/"
#define MAX_TOKENS 16  // Maximum number of tokens

// Create a Shellminator object
Shellminator shell;

const char logo[] =
"\033[38;05;208;1m\r\n"
"            ____       __           _    ____  ________     \r\n"
"       ___ / __ \\___  / /__________| |  / /  |/  / ___/ ___\r\n"
"     ____ / /_/ / _ \\/ __/ ___/ __ \\ | / / /|_/ /\\__ \\/_____\r\n"
"   _____ / _, _/  __/ /_/ /  / /_/ / |/ / /  / /___/ /_______ \r\n"
"        /_/ |_|\\___/\\__/_/   \\____/|___/_/  /_//____/       \r\n"
"\r\n"
"\r\n\033[0;37m"
"Visit:\033[1;32m https://retrovms.com\r\n\r\n"
;


//------------------------------------------------------------
// generates a response for Shellminator root page.
//------------------------------------------------------------
extern const char root_start[] asm("_binary_index_html_start");
extern const char root_end[] asm("_binary_index_html_end");

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    logd("Serve shell root: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
                 httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = SHELL_PREFIX,
    .method = HTTP_GET,
    .handler = root_get_handler
};

//------------------------------------------------------------
// generates a response /favicon.ico
//------------------------------------------------------------
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static const httpd_uri_t favicon = {
    .uri = SHELL_PREFIX "favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_get_handler
};

//------------------------------------------------------------
// generates a response /logo.svg
//------------------------------------------------------------
static esp_err_t logo_get_handler(httpd_req_t *req)
{
    extern const unsigned char logo_svg_start[] asm("_binary_logo_svg_start");
    extern const unsigned char logo_svg_end[]   asm("_binary_logo_svg_end");
    const size_t logo_svg_size = (logo_svg_end - logo_svg_start);
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)logo_svg_start, logo_svg_size-1);
    return ESP_OK;
}

static const httpd_uri_t servelogo = {
    .uri = SHELL_PREFIX "logo.svg",
    .method = HTTP_GET,
    .handler = logo_get_handler
};

//------------------------------------------------------------
// generates a response /close.svg
//------------------------------------------------------------
static esp_err_t close_get_handler(httpd_req_t *req)
{
    extern const unsigned char close_svg_start[] asm("_binary_close_svg_start");
    extern const unsigned char close_svg_end[]   asm("_binary_close_svg_end");
    const size_t close_svg_size = (close_svg_end - close_svg_start);
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)close_svg_start, close_svg_size-1);
    return ESP_OK;
}

static const httpd_uri_t closesvg = {
    .uri = SHELL_PREFIX "close.svg",
    .method = HTTP_GET,
    .handler = close_get_handler
};
//------------------------------------------------------------
// generates a response for /xterm.js
//------------------------------------------------------------

extern const char xterm_js_start[] asm("_binary_xterm_js_start");
extern const char xterm_js_end[] asm("_binary_xterm_js_end");

static esp_err_t xterm_js_handler(httpd_req_t *req)
{
    const uint32_t xterm_js_len = xterm_js_end - xterm_js_start;

    logd("Serve xterm_js");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, xterm_js_start, xterm_js_len);

    return ESP_OK;
}

static const httpd_uri_t xterm_js = {
    .uri = SHELL_PREFIX "xterm.js",
    .method = HTTP_GET,
    .handler = xterm_js_handler
};


//------------------------------------------------------------
// generates a response for /xterm.css
//------------------------------------------------------------

extern const char xterm_css_start[] asm("_binary_xterm_css_start");
extern const char xterm_css_end[] asm("_binary_xterm_css_end");

static esp_err_t xterm_css_handler(httpd_req_t *req)
{
    const uint32_t xterm_css_len = xterm_css_end - xterm_css_start;

    logd("Serve xterm_css");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, xterm_css_start, xterm_css_len);

    return ESP_OK;
}

static const httpd_uri_t xterm_css = {
    .uri = SHELL_PREFIX "xterm.css",
    .method = HTTP_GET,
    .handler = xterm_css_handler
};

//------------------------------------------------------------
// generates a response for /xterm-addon-web-links.js
//------------------------------------------------------------

extern const char xterm_addon_web_links_js_start[] asm("_binary_xterm_addon_web_links_js_start");
extern const char xterm_addon_web_links_js_end[] asm("_binary_xterm_addon_web_links_js_end");

static esp_err_t xterm_addon_web_links_js_handler(httpd_req_t *req)
{
    const uint32_t xterm_addon_web_links_js_len = xterm_addon_web_links_js_end - xterm_addon_web_links_js_start;

    logd("Serve xterm_addon_web_links_js");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, xterm_addon_web_links_js_start, xterm_addon_web_links_js_len);

    return ESP_OK;
}

static const httpd_uri_t xterm_addon_web_links_js = {
    .uri = SHELL_PREFIX "xterm-addon-web-links.js",
    .method = HTTP_GET,
    .handler = xterm_addon_web_links_js_handler
};


//------------------------------------------------------------
// shell over websocket handler
//------------------------------------------------------------

  esp_err_t wsShellHandler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {  
      logd("wshell id %d opened",  httpd_req_to_sockfd(req));
      // initialize shellminator object.
      shell.begin (&(req->handle), httpd_req_to_sockfd(req), "RetroVMS" );
      return ESP_OK;
    }
    // get new ws shell data
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ESP_CHECK_RETURN(httpd_ws_recv_frame(req, &ws_pkt, 0));

    ws_pkt.payload = (uint8_t *)calloc(1, ws_pkt.len + 1);
    if (!ws_pkt.payload)
      return ESP_ERR_NO_MEM;

    esp_err_t ret = ESP_ERROR_CHECK_WITHOUT_ABORT(
        httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len));
    if (ret == ESP_OK) {
      switch (ws_pkt.type) {
        case HTTPD_WS_TYPE_TEXT:
          shell.webSocketPush(ws_pkt.payload, ws_pkt.len);
          break;
        default:
          logi("WShell packet type %d was not handled", ws_pkt.type);
          break;
      }
    }
    free(ws_pkt.payload);
    return ret;
  }

  static const httpd_uri_t wshell = {
            .uri = SHELL_PREFIX "ws",
            .method = HTTP_GET,
            .handler = wsShellHandler,
            // .user_ctx = this,
            .is_websocket = true,
            .handle_ws_control_frames = false,
            .supported_subprotocol = 0
            };


namespace esp32m {
  namespace ui {

    void Shell::run() {
      uint8_t codes[10];
      esp_task_wdt_add(NULL);
      for (;;) {
        esp_task_wdt_reset();
        shell.update();
        delay(10);
      }
    }

    void executionFunction( char* cmd ) {
      char* cmdCopy = strdup(cmd);
      // Tokenize the input command
      char* argv[MAX_TOKENS];  // Array of pointers to tokens
      int argc = 0;  // Number of tokens
      char* token = strtok(cmdCopy, " ");  // Tokenize by space
      while (token != NULL && argc < MAX_TOKENS) {
          argv[argc] = token;  // Store pointer to token directly
          argc++;  // Increment token count
          token = strtok(NULL, " ");  // Get next token
      }
      OvmsCommandApp::instance(TAG).Execute(Shell::instance().verbosity, &Shell::instance(), argc, argv);
      free(cmdCopy);
    }

    // completion handling is so convoluted in OVMS it bears some explanation...
    // completionFunction() 
    //    => OvmsCommandApp.Complete()
    //      => m_root.Complete()
    //        => OvmsCommand::Complete()
    //          => m_children.GetCompletion()
    //            => OvmsCommandMap::GetCompletion()
    //              => writer->SetCompletion()
    //                => Shell::SetCompletion()

     void completionFunction( char* cmd ) {
      logi("partial: %s", cmd);
      char* cmdCopy = strdup(cmd);
      // Tokenize the input command
      char* argv[MAX_TOKENS];  // Array of pointers to tokens
      int argc = 0;  // Number of tokens
      char* token = strtok(cmdCopy, " ");  // Tokenize by space
      while (token != NULL && argc < MAX_TOKENS) {
          argv[argc] = token;  // Store pointer to token directly
          argc++;  // Increment token count
          token = strtok(NULL, " ");  // Get next token
      }
      char** candidates = OvmsCommandApp::instance(TAG).Complete(&Shell::instance(), argc, argv);
      int count = 0;
      while (candidates[count] != nullptr) {
        count++;
      }
      if (count==0) logi("no candidates");
      else if (count==1) {    // only one candidate, so complete for user
        std::string lastToken = argv[argc-1];
        logi("last token: %s", lastToken.c_str());
        std::string candidate = candidates[0];
        size_t pos = candidate.find(lastToken);
        std::string suffix = candidate.substr(pos + lastToken.length());
        logi("suffix: %s", suffix.c_str());
        for (char c : suffix) {
          shell.process(c);
        }
        shell.process(' ');
      } else {                // multiple candidates, print on new line for user
        shell.print("\n\r");
        for (int i=0; i<count ; i++) {
          logi("candidate: %s", candidates[i]);
          shell.print(candidates[i]);
          shell.print(" ");
        }
        // reprint prompt and original command
        shell.print("\n\r");
        shell.printBanner();
        shell.print(cmd);
      }
      free(cmdCopy);
    }

    // OVMSwriter child method eventually called by completionFunction()
    char** Shell::SetCompletion(int index, const char* token)
      {
      if (index < COMPLETION_MAX_TOKENS+1)
        {
        if (index == COMPLETION_MAX_TOKENS)
          token = "...";
        if (token)
          {
          strncpy(m_space[index], token, TOKEN_MAX_LENGTH-1);
          m_space[index][TOKEN_MAX_LENGTH-1] = '\0';
          m_completions[index] = m_space[index];
          ++index;
          }
        m_completions[index] = NULL;
        }
      return m_completions;
      }


    Shell::Shell(httpd_handle_t* server) {
      _server=server;
      xTaskCreate([](void *self) { ((Shell *)self)->run(); }, "m/shell", 4096,
            this, tskIDLE_PRIORITY + 1, &_task);
      shell.addExecFunc (&executionFunction);
      shell.addCmpltFunc(&completionFunction);
      // assume secure for now
      this->SetSecure(true);
    }

    void Shell::handleEvent(Event &ev) {
      if (EventInit::is(ev, 0)) {
        logd("Shell started %p, handle %p ", _server, *_server);
        httpd_register_uri_handler(*_server, &wshell);
        httpd_register_uri_handler(*_server, &root);
        httpd_register_uri_handler(*_server, &servelogo);
        httpd_register_uri_handler(*_server, &closesvg);
        httpd_register_uri_handler(*_server, &xterm_js);
        httpd_register_uri_handler(*_server, &xterm_css);
        httpd_register_uri_handler(*_server, &xterm_addon_web_links_js); 
        httpd_register_uri_handler(*_server, &favicon); 
        // Attach the logo shown at login
        shell.attachLogo( logo );
      }
    }

    void Shell::SetSecure(bool secure) {
      OvmsWriter::SetSecure(secure);
      // m_rl.prompt_str = secure ? secure_prompt : _PROMPT_DEFAULT;
    }

    /// Print immutable c-string
    ///
    int Shell::puts(const char* s) {
      shell.print(s) ;
      shell.print("\n\r");
      return 0;
    }


    /// Print buffer
    ///
    ssize_t Shell::write(const void *buf, size_t len) {
        shell.write(reinterpret_cast<const uint8_t*>(buf), len);
        return len;
    }

    /// Print formatted string
    ///
    int Shell::printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        
        // Determine the length of the formatted string
        int len = vsnprintf(nullptr, 0, fmt, args);
        if (len < 0) {
            va_end(args);
            return -1;  // Error in formatting
        }
        
        // Allocate memory for the formatted string
        char *buffer = (char *)malloc(len + 1);  // +1 for the null terminator
        if (buffer == nullptr) {
            va_end(args);
            return -1;  // Memory allocation failed
        }

        // Format the string into the buffer
        vsnprintf(buffer, len + 1, fmt, args);
        va_end(args);

        // Write the formatted string to the output
        shell.write(reinterpret_cast<const uint8_t*>(buffer), len);
        // Free the allocated memory
        free(buffer);
        shell.print("\r");
        
        return len;
    }

    Shell &Shell::instance() {
      static Shell i(httpServer);
      return i;
    }
// 
    void useShell() {
      Shell::instance();
    }    

  }  // namespace ui
}  // namespace esp32m
