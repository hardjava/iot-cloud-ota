#include "coffee/mqtt_pub.hpp"

#include <esp_freertos_hooks.h>

namespace coffee {
    /**
     * @brief 발행 대기열에 메시지를 추가합니다
     */
    static void publish_msg(const PubMessage& pm);

    /**
     * @brief MQTT 메시지를 발행하는 FreeRTOS 태스크
     */
    static void pub_mqtt_task(void* task_param);

    /**
     * @brief 기기 상태 정보를 주기적으로 송신하는 FreeRTOS 태스크
     */
    static void pub_sys_stat_task(void* task_param);

    /**
     * @brief CPU 코어별 사용량을 확인합니다
     */
    static bool measure_cpu_usage(uint32_t period_ms, float& core0_out, float& core1_out);

    static const std::string TAG = "coffee/mqtt_pub";

    /* 
       송신 템플릿
     */

    static const std::string stat_temp = R"r({
        "firmware_version": "0.0.0",
        "advertisements": [],
        "system": {
            "cpu_usage": {
                    "core_0": 0.0,
                    "core_1": 0.0
            },
            "memory_usage": 0.0,
            "storage_usage": 0.0,
            "uptime": 0
        },
        "network": {
            "connection_type": "wifi",
            "signal_strength": 0,
            "local_ip": "...",
            "gateway_ip": "..."
        },
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string ack_temp = R"r({
        "status": "ACKNOWLEDGED",
        "command_id": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string progress_temp = R"r({
        "command_id": "...",
        "progress": 0,
        "downloaded_bytes": 0,
        "total_bytes": 0,
        "speed_kbps": 0,
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string result_temp = R"r({
        "command_id": "...",
        "status": "SUCCESS",
        "message": "Download completed successfully",
        "checksum_verified": true,
        "download_ms": 0,
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string error_temp = R"r({
        "error_tag": "...",
        "log": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string sales_temp = R"r({
        "type": "...",
        "sub_type": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string cert_temp = R"r({
        "csr": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string regist_temp = R"r({
        "device_name": "...",
        "auth_key": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    static const std::string test_temp = R"r({
        "message": "...",
        "timestamp": "2025-01-01T00:00:00Z"
    })r";

    /*
       CPU 사용량 계산 관련
     */
    static volatile uint32_t s_idle_cnt_core0 = 0;

    static volatile uint32_t s_idle_cnt_core1 = 0;
    
    static portMUX_TYPE s_idle_mux = portMUX_INITIALIZER_UNLOCKED;

    static bool idle_hook_core0(void) {
        s_idle_cnt_core0++;
        return true;
    }

    static bool idle_hook_core1(void) {
        s_idle_cnt_core1++;
        return true;
    }

    static void ensure_idle_hooks_registered() {
        static bool s_inited = false;
        if (s_inited) return;

        esp_register_freertos_idle_hook_for_cpu(idle_hook_core0, 0);
        esp_register_freertos_idle_hook_for_cpu(idle_hook_core1, 1);
        s_inited = true;
    }

    /**
     * @brief MQTT 메시지 발행을 위한 대기열
     */
    QueueHandle_t mqtt_pub_q = nullptr;

    bool init_mqtt_pub(void) {
        static TaskHandle_t pub_mqtt_hd = nullptr;
        static TaskHandle_t pub_sys_stat_hd = nullptr;
        
        if (!mqtt_pub_q) {
            mqtt_pub_q = xQueueCreate(COFFEE_QUEUE_SIZE * 10, sizeof(PubMessage*));

            if (!mqtt_pub_q) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to initialize mqtt publish module\n");

                return false;
            }
        }

        if (!pub_mqtt_hd) {
            if (xTaskCreatePinnedToCore(pub_mqtt_task, "pub_mqtt", 8192, nullptr, tskIDLE_PRIORITY + 5, &pub_mqtt_hd, 0) != pdPASS) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to initialize mqtt publish module\n");

                return false;
            }
        }

        if (!pub_sys_stat_hd) {
            if (xTaskCreatePinnedToCore(pub_sys_stat_task, "pub_sys_stat", 8192, nullptr, tskIDLE_PRIORITY + 3, &pub_sys_stat_hd, 0) != pdPASS) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to initialize mqtt publish module\n");

                return false;
            }
        }
        
        return true;
    }

    void pub_request_ack(const std::string& command_id) {
        cJSON* pub_root = cJSON_Parse(ack_temp.c_str());

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "command_id"), command_id.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        PubMessage pm;
        pm.topic = mqtt_prefix + "update/request/ack";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_cancel_ack(const std::string& command_id) {
        cJSON* pub_root = cJSON_Parse(ack_temp.c_str());

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "command_id"), command_id.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        PubMessage pm;
        pm.topic = mqtt_prefix + "update/cancel/ack";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_update_progress(const std::string& command_id, std::size_t dl_b, std::size_t tt_b, int64_t time) {
        if (tt_b == 0 || time == 0) {
            return;
        }

        cJSON* pub_root = cJSON_Parse(progress_temp.c_str());

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "command_id"), command_id.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        std::size_t progress = static_cast<std::size_t>((static_cast<double>(dl_b) / tt_b) * 100.0);
        cJSON_SetNumberValue(cJSON_GetObjectItem(pub_root, "progress"), progress);

        cJSON_SetNumberValue(cJSON_GetObjectItem(pub_root, "downloaded_bytes"), dl_b);
        cJSON_SetNumberValue(cJSON_GetObjectItem(pub_root, "total_bytes"), tt_b);
        cJSON_SetNumberValue(cJSON_GetObjectItem(pub_root, "speed_kbps"), (dl_b * 8) / (time / 1000));

        PubMessage pm;
        pm.topic = mqtt_prefix + "update/progress";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_update_result(const std::string& command_id, const std::string& stat, const std::string& msg, std::size_t dl_ms) {
        delay(1000);

        cJSON* pub_root = cJSON_Parse(result_temp.c_str());

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "command_id"), command_id.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        if (stat != "SUCCESS") {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "status"), stat.c_str());
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "message"), msg.c_str());
            cJSON_SetBoolValue(cJSON_GetObjectItem(pub_root, "checksum_verified"), false);
        }

        cJSON_SetNumberValue(cJSON_GetObjectItem(pub_root, "download_ms"), dl_ms);

        PubMessage pm;
        pm.topic = mqtt_prefix + "update/result";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_error_log(const std::string& TAG, const std::string& log) {
        cJSON* pub_root = cJSON_Parse(error_temp.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "error_tag"), TAG.c_str());
        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "log"), log.c_str());

        PubMessage pm;
        pm.topic = mqtt_prefix + "status/error_log";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_sales_data(const std::string& type, const std::string& sub_type) {
        cJSON* pub_root = cJSON_Parse(sales_temp.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "type"), type.c_str());
        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "sub_type"), sub_type.c_str());

        PubMessage pm;
        pm.topic = mqtt_prefix + "sales/data";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_cert_request(const std::string& csr) {
        cJSON* pub_root = cJSON_Parse(cert_temp.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "csr"), csr.c_str());

        PubMessage pm;
        pm.topic = mqtt_prefix + "cert/request";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_regist(const std::string& auth_key) {
        cJSON* pub_root = cJSON_Parse(regist_temp.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }
        
        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "device_name"), serial_number->valuestring);
        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "auth_key"), auth_key.c_str());

        PubMessage pm;
        pm.topic = mqtt_prefix + "regist";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    void pub_test(const std::string& msg) {
        cJSON* pub_root = cJSON_Parse(test_temp.c_str());

        std::string now;
        if (utc_now(now)) {
            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
        }

        cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "message"), msg.c_str());

        PubMessage pm;
        pm.topic = mqtt_prefix + "test";
        pm.payload = pub_root;

        publish_msg(pm);
    }

    static void publish_msg(const PubMessage& pm) {
        PubMessage* task_param = new PubMessage(pm);

        if (!mqtt_pub_q) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_pub_q isn't initialized\n");
            delete task_param;
        }

        // 만약 네트워크 지연이 좀 심하면 pdMS_TO_TICKS(100)을 그냥 portMAX_DELAY로 변경하기
        if (xQueueSend(mqtt_pub_q, &task_param, pdMS_TO_TICKS(100)) != pdPASS) {
            delete task_param;
        }
    }

    static void pub_mqtt_task(void* task_param) {
        PubMessage* buf = nullptr;

        while (true) {
            if (!WiFi.isConnected() || !mqtt_client) {
                vTaskDelay(pdMS_TO_TICKS(1000));

                continue;
            }

            if (xQueueReceive(mqtt_pub_q, &buf, portMAX_DELAY) == pdPASS) {
                if (!buf) {
                    continue;
                }

                char* _payload = cJSON_PrintUnformatted(buf->payload);
                std::string payload(_payload);
                cJSON_free(_payload);

                int msg_id = esp_mqtt_client_publish(
                    mqtt_client,
                    buf->topic.c_str(),
                    payload.c_str(),
                    payload.size(),
                    1,
                    0
                );

                if (msg_id < 0) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] publish failed: %s\n", buf->topic.c_str());
                }

                cJSON_Delete(buf->payload);

                delete buf;

                buf = nullptr;
            }
        }

        vTaskDelete(nullptr);
    }

    static void pub_sys_stat_task(void* task_param) {
        vTaskDelay(pdMS_TO_TICKS(60000));

        while (true) {
            if (!WiFi.isConnected() || !mqtt_client) {
                vTaskDelay(pdMS_TO_TICKS(1000));

                continue;
            }

            cJSON* pub_root = cJSON_Parse(stat_temp.c_str());
            
            std::string now;
            if (utc_now(now)) {
                cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "timestamp"), now.c_str());
            }

            cJSON_SetValuestring(cJSON_GetObjectItem(pub_root, "firmware_version"), COFFEE_FIRMWARE_VER);

            if(!get_ad_ids()) {
                cJSON_Delete(pub_root);

                vTaskDelete(nullptr);

                return;
            }

            if (lock_mtx(ad_mtx, portMAX_DELAY)) {
                for (int id: ad_ids) {
                    cJSON* obj = cJSON_CreateObject();
                    cJSON_AddNumberToObject(obj, "id", id);

                    cJSON_AddItemToArray(cJSON_GetObjectItem(pub_root, "advertisements"), obj);
                }

                unlock_mtx(ad_mtx);
            }

            cJSON* system = cJSON_GetObjectItem(pub_root, "system");
            cJSON* cpu_usage = cJSON_GetObjectItem(system, "cpu_usage");
            
            float core0_usage = 0.0f, core1_usage = 0.0f;
            measure_cpu_usage(1000, core0_usage, core1_usage);

            cJSON_SetNumberValue(cJSON_GetObjectItem(cpu_usage, "core_0"), core0_usage);
            cJSON_SetNumberValue(cJSON_GetObjectItem(cpu_usage, "core_1"), core1_usage);

            float sram_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024.0f;
            float sram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024.0f;

            // float psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024.0f;
            // float psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f;

            // float mem_total = sram_total + psram_total;
            // float mem_free = sram_free + psram_free;

            // float mem_usage = (mem_total - mem_free) / mem_total * 100.0;
            float mem_usage = (sram_total - sram_free) / sram_total * 100.0;
            cJSON_SetNumberValue(cJSON_GetObjectItem(system, "memory_usage"), mem_usage);

            uint64_t total_sd = SD.cardSize();
            uint64_t used_sd = SD.usedBytes();

            float sd_usage = static_cast<float>(static_cast<double>(used_sd) / total_sd) * 100.0;
            cJSON_SetNumberValue(cJSON_GetObjectItem(system, "storage_usage"), sd_usage);

            uint64_t uptime = esp_timer_get_time() / 1000000ULL;
            cJSON_SetNumberValue(cJSON_GetObjectItem(system, "uptime"), uptime);

            cJSON* network = cJSON_GetObjectItem(pub_root, "network");

            cJSON_SetNumberValue(cJSON_GetObjectItem(network, "signal_strength"), WiFi.RSSI());
            cJSON_SetValuestring(cJSON_GetObjectItem(network, "local_ip"), WiFi.localIP().toString().c_str());
            cJSON_SetValuestring(cJSON_GetObjectItem(network, "gateway_ip"), WiFi.gatewayIP().toString().c_str());

            PubMessage pm;
            pm.topic = mqtt_prefix + "status/system";
            pm.payload = pub_root;

            publish_msg(pm);

            vTaskDelay(pdMS_TO_TICKS(59000));
        }

        vTaskDelete(nullptr);
    }

    static bool measure_cpu_usage(uint32_t period_ms, float& core0_out, float& core1_out) {
        if (period_ms == 0) return false;

        ensure_idle_hooks_registered();

        uint32_t c0_0, c1_0;
        portENTER_CRITICAL(&s_idle_mux);
        c0_0 = s_idle_cnt_core0;
        c1_0 = s_idle_cnt_core1;
        portEXIT_CRITICAL(&s_idle_mux);

        vTaskDelay(pdMS_TO_TICKS(period_ms));

        uint32_t c0_1, c1_1;
        portENTER_CRITICAL(&s_idle_mux);
        c0_1 = s_idle_cnt_core0;
        c1_1 = s_idle_cnt_core1;
        portEXIT_CRITICAL(&s_idle_mux);

        uint32_t d0 = c0_1 - c0_0;
        uint32_t d1 = c1_1 - c1_0;

        uint64_t sum = (uint64_t)d0 + (uint64_t)d1;
        if (sum == 0) {
            core0_out = 0.0f;
            core1_out = 0.0f;
            return true;
        }

        float idleRatio0 = static_cast<float>(d0) / static_cast<float>(sum);
        float idleRatio1 = static_cast<float>(d1) / static_cast<float>(sum);

        float busy0 = (1.0f - idleRatio0) * 100.0f;
        float busy1 = (1.0f - idleRatio1) * 100.0f;

        if (busy0 < 0.0f) busy0 = 0.0f; else if (busy0 > 100.0f) busy0 = 100.0f;
        if (busy1 < 0.0f) busy1 = 0.0f; else if (busy1 > 100.0f) busy1 = 100.0f;

        core0_out = busy0;
        core1_out = busy1;

        return true;
    }
}
