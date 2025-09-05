#include <cstdint>

#include <Arduino.h>

#include <coffee_drv/init.hpp>
#include <coffee_drv/utility.hpp>
#include <coffee_drv/wifi.hpp>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/json_task.hpp"
#include "coffee/mqtt_task.hpp"
#include "coffee/ui_task.hpp"

extern "C" void app_main(void) {
    initArduino();
    Serial.begin(115200);

    Serial.printf("[coffee/main][info] iot-cloud-ota capstone design: ver. %s\n", COFFEE_FIRMWARE_VER);

    if (!coffee_drv::init_drivers() || !coffee::init_ipc_queue()) {
        return;
    }

    // coffee_drv::set_mem_monitor(1000);

    coffee::init_network_config();

    coffee::init_mtx();

    if (!coffee::init_ui_task()) {
        return;
    }

    coffee::init_dbg_overlay();

    if (coffee::wifi_config) {
        Serial.println("[coffee/main][info] attempting to restore previous network connection...");
        std::string last_ssid = "", last_password = "";

        coffee::get_last_wifi(last_ssid, last_password);
        if (last_ssid != "") {
            coffee_drv::init_wifi_sta(last_ssid, last_password);
        }
    }

    if (coffee::mqtt_config) {
        Serial.println("[coffee/main][info] attempting to restore previous MQTT connection...");
        std::string last_addr = "";
        
        coffee::get_last_server(last_addr);
        if (last_addr != "") {
            coffee::init_mqtt(last_addr);
        }
    }
}
