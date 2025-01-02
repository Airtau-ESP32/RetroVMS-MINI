
set(requires
    crypto 
    dbc 
    esp_event 
    fatfs 
    id_filter 
    esp_netif 
    pcp  
    spi
    esp32can 
    mcp2515
    can 
    arduino-esp32
    app_update 
    vehicle 
    esp_https_server
    nvs_flash
    esp32m
    canopen
    ovms_tpms
    ovms_location
    vehicle_demo
    vehicle_dbc
    )


set(srcs 
    log_buffers.cpp
    metrics_standard.cpp
    ovms_command.cpp
    ovms_config.cpp
    ovms_events.cpp
    ovms_housekeeping.cpp
    ovms_peripherals.cpp
    ovms_main.cpp
    ovms_metrics.cpp
    ovms_module.cpp
    ovms_mutex.cpp
    ovms_notify.cpp
    ovms_semaphore.cpp
    ovms_time.cpp
    ovms_timer.cpp
    ovms_utils.cpp
    ovms_version.cpp
    ovms_vfs.cpp
    string_writer.cpp
    task_base.cpp
    shell.cpp
    sdcard.cpp
    ovms_v3.cpp
    cellular.cpp
    )

    
idf_component_register(SRCS ${srcs}  
                      INCLUDE_DIRS "." "../include"  "../build/web-ui"
                      REQUIRES ${requires}
                      PRIV_REQUIRES web-ui
                      EMBED_TXTFILES "certs/servercert.pem"
                      "certs/prvtkey.pem"
                      "ui/index.html"
                      "ui/xterm.js"
                      "ui/xterm.css"
                      "ui/xterm-addon-web-links.js"
                      "ui/favicon.ico"
                      "ui/logo.svg"
                      "ui/close.svg")

# list(APPEND kconfigs "${CMAKE_SOURCE_DIR}/Kconfig")

if (CONFIG_SPIRAM_CACHE_WORKAROUND)
  target_link_libraries(${COMPONENT_LIB} INTERFACE "-mfix-esp32-psram-cache-issue")
endif ()

# Hack to ensure that the loader script is loaded after (at least) memory.ld which
# defines `rtc_slow_seg` memory section.
# idf_build_get_property(prefix __PREFIX)
# target_linker_script("__${prefix}_esp_system" INTERFACE "${COMPONENT_DIR}/ovms_boot.ld")

execute_process(COMMAND git describe --always --tags --dirty
                OUTPUT_VARIABLE OVMS_VERSION
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions("-DOVMS_VERSION=\"${OVMS_VERSION}\"")

