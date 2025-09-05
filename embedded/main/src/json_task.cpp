#include "coffee/json_task.hpp"

namespace coffee {
    /**
     * @brief 모든 설정을 포함하는 JSON 루트 객체
     * 
     *        JSON root object containing all configuration settings
     */
    cJSON* config_root = nullptr;

    /**
     * @brief Wi-Fi 설정을 포함하는 JSON 객체
     * 
     *        JSON object containing Wi-Fi configuration
     */
    cJSON* wifi_config = nullptr;

    /**
     * @brief MQTT 서버 설정을 포함하는 JSON 객체
     * 
     *        JSON object containing MQTT server configuration
     */
    cJSON* mqtt_config = nullptr;

    void init_network_config(void) {
        File config_file = SD.open(COFFEE_CONFIG_JSON_FILE_PATH, FILE_READ);

        if (!config_file) {
            Serial.println("[coffee/config][error] reading file failure");

            return;
        }

        char* json_buf = (char*) heap_caps_malloc(COFFEE_CONFIG_JSON_MAX_BYTES, MALLOC_CAP_SPIRAM);

        if (!json_buf) {
            Serial.println("[coffee/config][error] allocating buffer failure");

            return;
        }

        size_t bytes = config_file.readBytes(json_buf, COFFEE_CONFIG_JSON_MAX_BYTES - 1);

        json_buf[bytes] = '\0';

        config_file.close();

        config_root = cJSON_Parse(json_buf);

        heap_caps_free(json_buf);

        if (!config_root) {
            Serial.println("[coffee/config][error] parsing json failure");

            return;
        }

        wifi_config = cJSON_GetObjectItem(config_root, "wifi");
        if (!wifi_config) {
            Serial.println("[coffee/config][error] unable to find ‘wifi’ object");

            cJSON_Delete(config_root);

            return;
        }

        mqtt_config = cJSON_GetObjectItem(config_root, "mqtt");
        if (!mqtt_config) {
            Serial.println("[coffee/config][error] unable to find ‘mqtt’ object");

            cJSON_Delete(config_root);

            return;
        }
    }

    void get_last_wifi(std::string& ssid_out, std::string& pw_out) {
        if (!config_root) {
            Serial.println("[coffee/config][error] config_root has not been initialized");

            ssid_out = "";
            pw_out = "";

            return;
        }

        if (lock_mtx(json_mtx, portMAX_DELAY)) {
            if (!cJSON_IsTrue(cJSON_GetObjectItem(wifi_config, "auto_connect"))) {
                ssid_out = "";
                pw_out = "";

                unlock_mtx(json_mtx);

                return;
            }

            cJSON* last_ssid = cJSON_GetObjectItem(wifi_config, "last_ssid");
            if (!last_ssid) {
                Serial.println("[coffee/config][error] unable to find ‘last_ssid’ object");

                ssid_out = "";
                pw_out = "";

                unlock_mtx(json_mtx);

                return;
            } else if (!cJSON_IsString(last_ssid)) {
                Serial.println("[coffee/config][error] invalid ‘last_ssid’");

                ssid_out = "";
                pw_out = "";

                unlock_mtx(json_mtx);

                return;
            } else {
                ssid_out = last_ssid->valuestring;
            }

            cJSON* last_password = cJSON_GetObjectItem(wifi_config, "last_password");
            if (!last_password) {
                Serial.println("[coffee/config][error] unable to find ‘last_password’ object");

                ssid_out = "";
                pw_out = "";

                unlock_mtx(json_mtx);

                return;
            } else if (!cJSON_IsString(last_password)) {
                Serial.println("[coffee/config][error] invalid ‘last_password’");

                ssid_out = "";
                pw_out = "";

                unlock_mtx(json_mtx);

                return;
            } else {
                pw_out = last_password->valuestring;
            }

            unlock_mtx(json_mtx);
        }

        return;
    }

    void get_last_server(std::string& addr_out) {
        if (!config_root) {
            Serial.println("[coffee/config][error] config_root has not been initialized");

            addr_out = "";

            return;
        }

        if (lock_mtx(json_mtx, portMAX_DELAY)) {
            if (!cJSON_IsTrue(cJSON_GetObjectItem(mqtt_config, "auto_connect"))) {
                addr_out = "";

                unlock_mtx(json_mtx);

                return;
            }

            cJSON* last_server = cJSON_GetObjectItem(mqtt_config, "last_server");
            if (!last_server) {
                Serial.println("[coffee/config][error] unable to find ‘last_server’ object");

                addr_out = "";

                unlock_mtx(json_mtx);

                return;
            } else if (!cJSON_IsString(last_server)) {
                Serial.println("[coffee/config][error] invalid ‘last_server’");

                addr_out = "";

                unlock_mtx(json_mtx);

                return;
            } else {
                addr_out = last_server->valuestring;
            }

            unlock_mtx(json_mtx);
        }

        return;
    }

    void write_config(void) {
        char* out_config = nullptr;
        if (lock_mtx(json_mtx, portMAX_DELAY)) {
            out_config = cJSON_Print(config_root);

            unlock_mtx(json_mtx);
        }
        
        if(!out_config) {
            Serial.println("[coffee/config][error] failed to allocate out_config");

            free(out_config);

            return;
        }

        File file = SD.open(COFFEE_TEMP_CONFIG_FILE_PATH, FILE_WRITE);

        if (!file) {
            Serial.println("[coffee/config][error] overwriting file failure");

            free(out_config);

            return;
        }

        file.print(out_config);

        file.close();

        if (!SD.remove(COFFEE_CONFIG_JSON_FILE_PATH)) {
            Serial.println("[coffee/config][error] falied to remove the old file");
            Serial.printf("    temporary file path: %s\n", COFFEE_TEMP_CONFIG_FILE_PATH);

            free(out_config);

            return;
        } else {
            if (!SD.rename(COFFEE_TEMP_CONFIG_FILE_PATH, COFFEE_CONFIG_JSON_FILE_PATH)) {
                Serial.println("[coffee/config][error] rename failed");
                Serial.printf("    temporary file path: %s\n", COFFEE_TEMP_CONFIG_FILE_PATH);

                free(out_config);

                return;
            }
        }

        Serial.println("[coffee/config][info] writing new json file success!");

        free(out_config);

        return;
    }
}
