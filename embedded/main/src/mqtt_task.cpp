#include "coffee/mqtt_task.hpp"

namespace coffee {
    /**
     * @brief MQTT 이벤트 콜백 함수
     * 
     *        MQTT event callback function
     */
    static void mqtt_event_cb(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);

    /**
     * @brief MQTT 토픽 이벤트 처리 함수
     * 
     *        function to handle MQTT topic events
     */
    static void mqtt_event_handler(std::string topic, std::string payload);

    /**
     * @brief MQTT를 이용한 OTA 테스트를 위한 FreeRTOS 태스크
     * 
     *        FreeRTOS task for testing OTA via MQTT
     */
    static void test_mqtt_ota_task(void* task_param);

    // 로그에 출력되는 태그
    // tag used for log messages
    static const std::string TAG = "coffee/mqtt_task";

    // MQTT 토픽을 통해 기기를 구분하기 위한 접두사
    // prefix used to distinguish devices in MQTT topics
    static std::string mqtt_prefix = "";

    // MQTT 연결 성공 전 임시로 저장해두는 MQTT 서버 URI
    // temporary storage for the MQTT server URI before connection is established
    static std::string temp_mqtt_uri = "";

    // MQTT 토픽 ID
    // mapping of MQTT topic IDs to their corresponding topic strings
    static std::map<int, std::string> mqtt_topic_ids;

    /**
     * @brief 현재 연결된 MQTT 서버의 URI
     * 
     *        the URI of the currently connected MQTT server
     */
    std::string mqtt_uri = "Loading...";

    /**
     * @brief 기기 구분을 위한 MQTT 토픽의 지역 정보
     * 
     *        region information for distinguishing devices in MQTT topics
     */
    std::string mqtt_region = "";

    /**
     * @brief 기기 구분을 위한 MQTT 토픽의 점포 정보
     * 
     *        store information for distinguishing devices in MQTT topics
     */
    std::string mqtt_store = "";

    /**
     * @brief MQTT 클라이언트
     * 
     *        MQTT client
     */
    esp_mqtt_client_handle_t mqtt_client = nullptr;

    void init_mqtt(std::string addr) {
        if (mqtt_prefix == "") {
            if (!mqtt_config) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_config has not been initialized\n");

                return;
            }

            if (lock_mtx(json_mtx, portMAX_DELAY)) {
                cJSON* version = cJSON_GetObjectItem(mqtt_config, "version");
                cJSON* region = cJSON_GetObjectItem(mqtt_config, "region");
                cJSON* store = cJSON_GetObjectItem(mqtt_config, "store");
                if (!version || !region || !store) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] invalid mqtt_config\n");

                    unlock_mtx(json_mtx);

                    return;
                }

                mqtt_region = region->valuestring;
                mqtt_store = store->valuestring;

                mqtt_prefix = std::string(version->valuestring) + "/" + mqtt_region + "/" + mqtt_store + "/";

                unlock_mtx(json_mtx);
            }
        }

        lock_mtx(mqtt_mtx, portMAX_DELAY);
        
        if (mqtt_client) {
            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);

            mqtt_client = nullptr;
        }

        esp_mqtt_client_config_t mqtt_cfg = { };

        temp_mqtt_uri = addr;

        std::string full_uri = std::string("mqtt://") + temp_mqtt_uri;
        
        mqtt_cfg.uri = full_uri.c_str();
        mqtt_cfg.username = nullptr;
        mqtt_cfg.password = nullptr;

        mqtt_cfg.reconnect_timeout_ms = COFFEE_NETWORK_TIMEOUT_MS;
        mqtt_cfg.task_stack = 4096;
        mqtt_cfg.task_prio = tskIDLE_PRIORITY + 4;

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

        esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_cb, NULL);

        esp_mqtt_client_start(mqtt_client);

        unlock_mtx(mqtt_mtx);
    }

    void set_mqtt_prefix(std::string region, std::string store) {
        if (region == "" && store == "") {
            return;
        }

        if (!mqtt_config) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_config has not been initialized\n");

            return;
        }

        if (lock_mtx(json_mtx, portMAX_DELAY)) {
            cJSON* version = cJSON_GetObjectItem(mqtt_config, "version");
            if (!version) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] invalid mqtt_config\n");

                unlock_mtx(json_mtx);

                return;
            }

            mqtt_region = region;
            mqtt_store = store;

            mqtt_prefix = std::string(version->valuestring) + "/" + region + "/" + store + "/";

            unlock_mtx(json_mtx);
        }
    }

    static void mqtt_event_cb(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
        lock_mtx(mqtt_mtx, portMAX_DELAY);

        esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

        switch (event->event_id) {
            case MQTT_EVENT_CONNECTED:
                queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT server connected\n");

                mqtt_uri = temp_mqtt_uri;

                if (lock_mtx(json_mtx, portMAX_DELAY)) {
                    if (!mqtt_config) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_config has not been initialized\n");

                        esp_mqtt_client_stop(mqtt_client);
                        esp_mqtt_client_destroy(mqtt_client);

                        mqtt_client = nullptr;

                        unlock_mtx(json_mtx);

                        break;
                    }

                    {
                        cJSON* last_server = cJSON_GetObjectItem(mqtt_config, "last_server");
                        cJSON* region = cJSON_GetObjectItem(mqtt_config, "region");
                        cJSON* store = cJSON_GetObjectItem(mqtt_config, "store");
                        cJSON* sub_topics = cJSON_GetObjectItem(mqtt_config, "sub_topics");
                        if (!last_server || !cJSON_IsString(last_server)
                            || !region || !cJSON_IsString(region)
                            || !store || !cJSON_IsString(store)
                            || !sub_topics || !cJSON_IsArray(sub_topics)) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid `mqtt_config`\n");

                            esp_mqtt_client_stop(mqtt_client);
                            esp_mqtt_client_destroy(mqtt_client);

                            mqtt_client = nullptr;

                            unlock_mtx(json_mtx);

                            break;
                        }

                        int size = cJSON_GetArraySize(sub_topics);
                        bool err_flag = false;
                        for (int i = 0; i < size; i++) {
                            cJSON* topic = cJSON_GetArrayItem(sub_topics, i);
                            if (!cJSON_IsString(topic)) {
                                err_flag = true;

                                unlock_mtx(json_mtx);

                                break;
                            }

                            std::string full_name = mqtt_prefix + topic->valuestring;

                            mqtt_topic_ids.insert({esp_mqtt_client_subscribe(mqtt_client, full_name.c_str(), 1), full_name});
                        }

                        if (err_flag) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid `sub_topics`\n");

                            esp_mqtt_client_stop(mqtt_client);
                            esp_mqtt_client_destroy(mqtt_client);

                            mqtt_client = nullptr;

                            unlock_mtx(json_mtx);

                            break;
                        }

                        cJSON_SetValuestring(last_server, mqtt_uri.c_str());
                        cJSON_SetValuestring(region, mqtt_region.c_str());
                        cJSON_SetValuestring(store, mqtt_store.c_str());
                    }

                    unlock_mtx(json_mtx);

                    write_config();
                }

                break;
            case MQTT_EVENT_DATA:
                {
                    static std::string acc_topic;
                    static std::string acc_payload;

                    if (event->current_data_offset == 0) {
                        acc_topic.clear();
                        acc_payload.clear();
                        if (event->topic && event->topic_len > 0) {
                            acc_topic.assign(event->topic, event->topic_len);
                        }
                    }

                    acc_payload.append(event->data, event->data_len);

                    if (event->current_data_offset + event->data_len == event->total_data_len) {
                        mqtt_event_handler(acc_topic, acc_payload);
                        acc_topic.clear();
                        acc_payload.clear();
                    }
                }
                
                break;
            case MQTT_EVENT_DISCONNECTED:
                queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT server disconnected\n");

                break;
            case MQTT_EVENT_SUBSCRIBED:
                queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT topic subscribed: %s\n", mqtt_topic_ids[event->msg_id].c_str());

                break;
            case MQTT_EVENT_BEFORE_CONNECT:
            case MQTT_EVENT_UNSUBSCRIBED:
            case MQTT_EVENT_PUBLISHED:
            case MQTT_EVENT_DELETED:
            case MQTT_EVENT_ERROR:
            case MQTT_EVENT_ANY:
                break;
            default:
                queue_printf(dbg_overlay_q, TAG, true, "[warning] unknown MQTT event(%d)\n", event->event_id);

                break;
        }

        unlock_mtx(mqtt_mtx);
    }

    static void mqtt_event_handler(std::string topic, std::string payload) {
        if (topic == mqtt_prefix + "debug") {
            cJSON* temp_root = cJSON_Parse(payload.c_str());

            xTaskCreatePinnedToCore(test_mqtt_ota_task, "test_mqtt_ota", 8192, temp_root, tskIDLE_PRIORITY + 6, nullptr, 0);
        } else {
            queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT message received(%s): %s\n", topic.c_str(), payload.c_str());
        }
    }

    static void test_mqtt_ota_task(void* task_param) {
        cJSON* temp_root = static_cast<cJSON*>(task_param);

        if (!temp_root) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            vTaskDelete(nullptr);
        }

        Download dl_info;
        
        cJSON* temp_signed_url = cJSON_GetObjectItem(temp_root, "signedUrl");
        cJSON* temp_file_info = cJSON_GetObjectItem(temp_root, "fileInfo");
        if (!temp_signed_url || !cJSON_IsString(temp_signed_url)
            || !temp_file_info || !cJSON_IsObject(temp_file_info)) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            cJSON_Delete(temp_root);

            vTaskDelete(nullptr);
        } else {
            dl_info.signed_url = std::string(temp_signed_url->valuestring);
        }

        std::string version;
        cJSON* temp_version = cJSON_GetObjectItem(temp_file_info, "version");
        cJSON* temp_file_size = cJSON_GetObjectItem(temp_file_info, "fileSize");
        cJSON* temp_file_hash = cJSON_GetObjectItem(temp_file_info, "fileHash");
        if (!temp_version || !cJSON_IsString(temp_version)
            || !temp_file_size || !cJSON_IsNumber(temp_file_size)
            || !temp_file_hash || !cJSON_IsString(temp_file_hash)) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            cJSON_Delete(temp_root);

            vTaskDelete(nullptr);
        } else {
            version = std::string(temp_version->valuestring);

            dl_info.storage_path = std::string("/ota/") + version + ".bin";
            dl_info.file_size = static_cast<std::size_t>(temp_file_size->valueint);
            dl_info.hash_256 = std::string(temp_file_hash->valuestring);
        }

        cJSON_Delete(temp_root);

        download_file(dl_info);

        delay(10);

        if (lock_mtx(network_mtx, portMAX_DELAY)) {
            unlock_mtx(network_mtx);

            if (SD.exists(dl_info.storage_path.c_str())) {
                ota(find_firmware_file(version));
            } else {
                queue_printf(dbg_overlay_q, TAG, true, "[error] update aborted...\n");
            }
        }

        vTaskDelete(nullptr);
    }
}
