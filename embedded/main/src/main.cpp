#include <esp_ota_ops.h>

#include <Arduino.h>

#include <coffee_drv/init.hpp>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/json_task.hpp"
#include "coffee/mqtt_task.hpp"
#include "coffee/network_task.hpp"
#include "coffee/rtc.hpp"
#include "coffee/ui_task.hpp"

#define COFFEE_OTA_ROLLBACK_DEBUG 0

// static void coffee_ota_rollback_gate(void);

extern "C" void app_main(void) {
    initArduino();
    Serial.begin(115200);

    Serial.printf("[coffee/main][info] iot-cloud-ota capstone design: ver. %s\n", COFFEE_FIRMWARE_VER);

    // coffee_ota_rollback_gate();

    // Serial.println("[info] ota rollback triggered");

    // delay(3000);
    
    // esp_ota_mark_app_invalid_rollback_and_reboot();

    if (!coffee_drv::init_drivers() || !coffee::init_ipc_queue()) {
        return;
    }

    coffee::init_network_config();

    coffee::init_mtx();

    if (!coffee::init_ui_task()) {
        return;
    }

    delay(100);

    coffee::init_dbg_overlay();

    delay(1000);

    if (coffee::wifi_config) {
        Serial.println("[coffee/main][info] attempting to restore previous network connection...");
        std::string last_ssid = "", last_password = "";

        coffee::get_last_wifi(last_ssid, last_password);
        if (last_ssid != "") {
            coffee::connect_wifi(last_ssid, last_password);
        }
    }

    delay(10000);

    if (coffee::mqtt_config) {
        Serial.println("[coffee/main][info] attempting to restore previous MQTT connection...");
        std::string last_addr = "";
        
        coffee::get_last_server(last_addr);
        if (last_addr != "") {
            coffee::init_mqtt(last_addr);
        }
    }
}

// static void coffee_ota_rollback_gate(void) {
//     const esp_partition_t* running_p = esp_ota_get_running_partition();
//     if (!running_p) {
//         return;
//     }

//     esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
//     if (esp_ota_get_state_partition(running_p, &state) != ESP_OK) {
//         return;
//     }

//     if (state == ESP_OTA_IMG_PENDING_VERIFY) {
//         if (COFFEE_OTA_ROLLBACK_DEBUG) {
//             Serial.println("[info] ota rollback triggered");

//             delay(3000);
            
//             esp_ota_mark_app_invalid_rollback_and_reboot();
//         } else {
//             esp_ota_mark_app_valid_cancel_rollback();
//         }
//     }
// }
