#include "coffee/debug_task.hpp"

namespace coffee {
    /**
     * @brief FreeRTOS 태스크를 이용하여 디버깅을 수행합니다
     * 
     *        performs debugging using a FreeRTOS task
     */
    // static void debug1_task(void* task_param);

    void debug1(void* debug_param) {
        // xTaskCreatePinnedToCore(debug1_task, "debug1", 4096, debug_param, tskIDLE_PRIORITY + 4, nullptr, 0);
    }

    void debug2(void* debug_param) {
        static size_t easter_egg_cnt = 0;

        std::string* tp = static_cast<std::string*>(debug_param);
        std::string firmware_version = *tp;
        delete tp;

        easter_egg_cnt++;

        if (!lock_mtx(debug_mtx) || !lock_mtx(ota_mtx)) {
            if (easter_egg_cnt >= 5) {
                queue_printf(debugTextArea_q, "", false, "STOP DOING THAT, SONG!!!\n");
            } else {
                queue_printf(debugTextArea_q, "", false, "Another debugging session is already running.\n");
            }

            return;
        }

        auto ff = find_firmware_file(firmware_version);

        unlock_mtx(debug_mtx);
        unlock_mtx(ota_mtx);

        if (ff.version != "") {
            ota(ff);
        }
    }

    // static void debug1_task(void* task_param) {
    //     static size_t easter_egg_cnt = 0;

    //     std::string* tp = static_cast<std::string*>(task_param);
    //     std::string firmware_id = *tp;
    //     delete tp;

    //     easter_egg_cnt++;

    //     if (!lock_mtx(debug_mtx) || !lock_mtx(network_mtx)) {
    //         if (easter_egg_cnt >= 5) {
    //             queue_printf(debugTextArea_q, "", false, "STOP DOING THAT, SONG!!!\n");
    //         } else {
    //             queue_printf(debugTextArea_q, "", false, "Another debugging session is already running.\n");
    //         }

    //         vTaskDelete(nullptr);
    //     }

    //     easter_egg_cnt = 0;

    //     queue_printf(debugTextArea_q, "", false, "Firmware download started\n");
    //     queue_printf(debugTextArea_q, "", false, "Firmware ID: %s\n", firmware_id.c_str());

    //     // Signed URL 및 파일 정보 발급
    //     HTTPClient http;
    //     http.setConnectTimeout(COFFEE_NETWORK_TIMEOUT_MS);
    //     http.setTimeout(COFFEE_NETWORK_TIMEOUT_MS);

    //     std::string ENDPOINT = std::string("http://backend-alb-497054189.ap-northeast-2.elb.amazonaws.com/api/firmwares/metadata/") + firmware_id + "/deployment";

    //     if (!http.begin(ENDPOINT.c_str())) {
    //         queue_printf(debugTextArea_q, "", false, "HTTP begin failed\n");

    //         unlock_mtx(network_mtx);

    //         unlock_mtx(debug_mtx);

    //         vTaskDelete(nullptr);
    //     }

    //     http.addHeader("Content-Type", "application/json");

    //     std::string REQ_BODY(R"r({
    //             "deploymentType": "DEVICE",
    //             "deviceIds": [1, 2, 3],
    //             "groupIds": [],
    //             "regionIds": []
    //         })r");

    //     int res_code = http.POST(String(REQ_BODY.c_str()));
    //     if (res_code != HTTP_CODE_OK) {
    //         queue_printf(debugTextArea_q, "", false, "HTTP POST failed, code=%d\n", res_code);

    //         http.end();

    //         unlock_mtx(network_mtx);

    //         unlock_mtx(debug_mtx);
            
    //         vTaskDelete(nullptr);
    //     }

    //     std::string res_body(http.getString().c_str());

    //     http.end();

    //     cJSON* res_root = cJSON_Parse(res_body.c_str());
    //     if (!res_root) {
    //         queue_printf(debugTextArea_q, "", false, "Response parse failed\n");

    //         unlock_mtx(network_mtx);

    //         unlock_mtx(debug_mtx);

    //         vTaskDelete(nullptr);
    //     }

    //     std::string signed_url;
    //     cJSON* res_signed_url = cJSON_GetObjectItem(res_root, "signedUrl");
    //     cJSON* res_file_info = cJSON_GetObjectItem(res_root, "fileInfo");
    //     if (!res_signed_url || !cJSON_IsString(res_signed_url)
    //         || !res_file_info || !cJSON_IsObject(res_file_info)) {
    //         queue_printf(debugTextArea_q, "", false, "Invalid HTTP response\n");

    //         cJSON_Delete(res_root);

    //         unlock_mtx(network_mtx);

    //         unlock_mtx(debug_mtx);

    //         vTaskDelete(nullptr);
    //     } else {
    //         signed_url = std::string(res_signed_url->valuestring);
    //     }

    //     std::string version = "";
    //     std::size_t file_size = 0;
    //     cJSON* res_version = cJSON_GetObjectItem(res_file_info, "version");
    //     cJSON* res_file_size = cJSON_GetObjectItem(res_file_info, "fileSize");
    //     if (!res_version || !cJSON_IsString(res_version)
    //         || !res_file_size || !cJSON_IsNumber(res_file_size)) {
    //         queue_printf(debugTextArea_q, "", false, "Invalid HTTP response\n");

    //         cJSON_Delete(res_root);

    //         unlock_mtx(network_mtx);

    //         unlock_mtx(debug_mtx);

    //         vTaskDelete(nullptr);
    //     } else {
    //         version = std::string(res_version->valuestring);
    //         file_size = static_cast<std::size_t>(res_file_size->valueint);
    //     }

    //     queue_printf(debugTextArea_q, "", false, "Firmware info:\n");
    //     queue_printf(debugTextArea_q, "", false, "    Version: %s\n", version.c_str());

    //     queue_printf(debugTextArea_q, "", false, "    Size: %zuB\n", file_size);

    //     cJSON_Delete(res_root);

    //     unlock_mtx(network_mtx);

    //     download_file(signed_url, std::string("/ota/") + version + ".bin");

    //     unlock_mtx(debug_mtx);

    //     vTaskDelete(nullptr);
    // }
}
