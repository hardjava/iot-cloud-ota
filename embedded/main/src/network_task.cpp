#include "coffee/network_task.hpp"

namespace coffee {
    /**
     * @brief FreeRTOS 태스크를 통해 Wi-Fi 연결을 시도합니다
     * 
     *        attempts to connect to Wi-Fi using a FreeRTOS task
     */
    static void init_wifi_task(void* task_param);

    /**
     * @brief FreeRTOS 태스크를 통해 파일을 다운로드합니다
     * 
     *        downloads a file using a FreeRTOS task
     */
    static void download_task(void* task_param);

    static const std::string TAG = "coffee/network_task";

    void connect_wifi(std::string ssid, std::string pw) {
        ConInfo* task_args = new ConInfo();
        if (!task_args) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to allocate task_args");

            return;
        }

        task_args->ssid = ssid;
        task_args->pw = pw;

        if (xTaskCreatePinnedToCore(init_wifi_task, "init_wifi", 8192, task_args, tskIDLE_PRIORITY + 4, nullptr, 0) != pdPASS) {
            delete task_args;
        }
    }

    void download_file(std::string signed_url, std::string storage_path) {
        Download* task_args = new Download();
        if (!task_args) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to allocate task_args");

            return;
        }

        task_args->signed_url = signed_url;
        task_args->storage_path = storage_path;

        if (xTaskCreatePinnedToCore(download_task, "download", 8192, task_args, tskIDLE_PRIORITY + 4, nullptr, 0) != pdPASS) {
            delete task_args;
        }
    }

    static void init_wifi_task(void* task_param) {
        static size_t easter_egg_cnt = 0;
        
        ConInfo* tp = static_cast<ConInfo*>(task_param);
        ConInfo ci = *tp;
        delete tp;

        easter_egg_cnt++;

        if (!lock_mtx(network_mtx)) {
            if (easter_egg_cnt >= 5) {
                queue_printf(wifiTextArea_q, "", false, "STOP DOING THAT, SONG!!!\n");
            } else {
                queue_printf(wifiTextArea_q, "", false, "Wi-Fi connection is already in progress\n");
            }

            vTaskDelete(NULL);
        }

        easter_egg_cnt = 0;

        queue_printf(wifiTextArea_q, "", false, "Attempting to connect...\n");

        if (coffee_drv::init_wifi_sta(ci.ssid, ci.pw)) {
            queue_printf(wifiTextArea_q, "", false, "Successfully connected! local IP=%s\n", WiFi.localIP().toString().c_str());

            if (wifi_config) {
                cJSON* last_ssid = cJSON_GetObjectItem(wifi_config, "last_ssid");
                cJSON* last_password = cJSON_GetObjectItem(wifi_config, "last_password");
                if (last_ssid && last_password) {
                    cJSON_SetValuestring(last_ssid, ci.ssid.c_str());
                    cJSON_SetValuestring(last_password, ci.pw.c_str());
                }

                write_config();
            }
        } else {
            queue_printf(wifiTextArea_q, "", false, "Connection failed(Time Out)\n");
        }

        unlock_mtx(network_mtx);

        vTaskDelete(NULL);
    }

    static void download_task(void* task_param) {
        static std::size_t easter_egg_cnt = 0;

        Download* tp = static_cast<Download*>(task_param);
        Download dl = *tp;
        delete tp;

        if (WiFi.status() != WL_CONNECTED) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] unable to connect to the network\n");

            vTaskDelete(nullptr);
        }

        easter_egg_cnt++;

        if (!lock_mtx(network_mtx)) {
            if (easter_egg_cnt >= 5) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] STOP DOING THAT, SONG!!!\n");
            } else {
                queue_printf(dbg_overlay_q, TAG, true, "[error] another debugging session is already running\n");
            }

            vTaskDelete(nullptr);
        }

        easter_egg_cnt = 0;

        WiFiClientSecure client;
        client.setTimeout(COFFEE_NETWORK_TIMEOUT_MS);
        client.setInsecure();

        HTTPClient http;

        if (!http.begin(client, dl.signed_url.c_str())) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] HTTPS begin failed\n");

            client.stop();

            unlock_mtx(network_mtx);

            vTaskDelete(nullptr);
        }

        int res_code = http.GET();
        if (res_code != HTTP_CODE_OK) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] HTTPS GET failed, code=%d\n", res_code);

            http.end();

            client.stop();

            unlock_mtx(network_mtx);

            vTaskDelete(nullptr);
        }

        if (SD.exists(dl.storage_path.c_str())) {
            SD.remove(dl.storage_path.c_str());
        }

        File new_file = SD.open(dl.storage_path.c_str(), FILE_WRITE);
        if (!new_file) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] file creation failed\n");

            http.end();

            client.stop();

            unlock_mtx(network_mtx);

            vTaskDelete(nullptr);
        }

        WiFiClient* stream = http.getStreamPtr();
        
        uint8_t* buf = reinterpret_cast<uint8_t*>(heap_caps_malloc(COFFEE_FILE_CHUNK_SIZE, MALLOC_CAP_SPIRAM));
        if (!buf) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] file buffer allocation failed\n");

            new_file.close();

            http.end();

            client.stop();

            unlock_mtx(network_mtx);

            vTaskDelete(nullptr);
        }

        std::size_t total_downloaded = 0;
        std::size_t download_size = 0;
        while ((download_size = stream->readBytes(buf, COFFEE_FILE_CHUNK_SIZE)) > 0) {
            if (new_file.write(buf, download_size) != download_size) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] file download failed\n");

                heap_caps_free(buf);

                new_file.close();
                
                http.end();

                client.stop();

                unlock_mtx(network_mtx);

                vTaskDelete(nullptr);
            }

            total_downloaded += download_size;

            queue_printf(dbg_overlay_q, TAG, true, "[info] file downloaded: %zuB\n", total_downloaded);
        }

        queue_printf(dbg_overlay_q, TAG, true, "[info] total downloaded size: %zuB\n", total_downloaded);

        queue_printf(dbg_overlay_q, TAG, true, "[info] file download complete!\n");

        heap_caps_free(buf);

        new_file.close();

        http.end();

        client.stop();

        unlock_mtx(network_mtx);

        vTaskDelete(nullptr);
    }
}
