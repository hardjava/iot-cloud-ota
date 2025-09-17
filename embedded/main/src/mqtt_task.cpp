#include "coffee/mqtt_task.hpp"

namespace coffee {
    /**
     * @brief MQTT 클라이언트 초기화를 위한 FreeRTOS 태스크
     */
    static void init_mqtt_task(void* task_param);

    /**
     * @brief MQTT 이벤트 콜백 함수
     * 
     *        MQTT event callback function
     */
    static void mqtt_event_cb(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);

    /**
     * @brief MQTT 토픽 이벤트를 처리 대기열에 추가합니다
     */
    static void enqueue_mqtt_event(std::string topic, std::string payload);

    /**
     * @brief MQTT 토픽 이벤트 처리 큐에서 값을 받아 처리하는 FreeRTOS 태스크
     */
    static void mqtt_event_handle_task(void* task_param);

    /**
     * @brief 펌웨어 OTA를 수행합니다
     */
    static void firmware_ota_task(void* task_param);

    /**
     * @brief 광고 OTA를 수행합니다
     */
    static void ad_ota_task(void* task_param);

    // 로그에 출력되는 태그
    // tag used for log messages
    static const std::string TAG = "coffee/mqtt_task";

    // MQTT 연결 성공 전 임시로 저장해두는 MQTT 서버 URI
    // temporary storage for the MQTT server URI before connection is established
    static std::string temp_mqtt_uri = "";

    // MQTT 토픽 ID
    // mapping of MQTT topic IDs to their corresponding topic strings
    static std::map<int, std::string> mqtt_topic_ids;

    // MQTT 이벤트 대기열
    static QueueHandle_t mqtt_event_q = nullptr;

    /**
     * @brief 현재 연결된 MQTT 서버의 URI
     * 
     *        the URI of the currently connected MQTT server
     */
    std::string mqtt_uri = "Loading...";

    /**
     * @brief MQTT 토픽을 통해 기기를 구분하기 위한 접두사
     * 
     *        prefix used to distinguish devices in MQTT topics
     */
    std::string mqtt_prefix = "";

    /**
     * @brief MQTT 클라이언트
     * 
     *        MQTT client
     */
    esp_mqtt_client_handle_t mqtt_client = nullptr;

    /**
     * @brief MQTT 클라이언트 연결 여부
     */
    bool mqtt_connected = false;

    void init_mqtt(std::string addr) {
        std::string* task_param = new std::string(addr);

        if (xTaskCreatePinnedToCore(init_mqtt_task, "init_mqtt", 8192, task_param, tskIDLE_PRIORITY + 5, nullptr, 0) != pdPASS) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create mqtt client initialization task\n");

            delete task_param;

            return;
        }

        delay(5000);
    }

    void set_mqtt_prefix(void) {
        if (!mqtt_config) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_config has not been initialized\n");

            return;
        }

        if (lock_mtx(json_mtx, portMAX_DELAY)) {
            cJSON* version = cJSON_GetObjectItem(mqtt_config, "version");
            if (!version || !cJSON_IsString(version)) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] invalid mqtt_config\n");

                unlock_mtx(json_mtx);

                return;
            }

            if(!cJSON_IsNumber(device_id)) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] invalid device ID\n");

                unlock_mtx(json_mtx);

                return;
            }

            mqtt_prefix = std::string(version->valuestring) + "/" + std::to_string(device_id->valueint) + "/";

            unlock_mtx(json_mtx);
        }
    }
    
    void clear_dir(const std::string& path) {
        File dir = SD.open(path.c_str());
        if (!dir) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to open target directory\n");

            return;
        }

        if (lock_mtx(ad_mtx, portMAX_DELAY)) {
            File entry;
            while ((entry = dir.openNextFile())) {
                if (!entry.isDirectory()) {
                    if (!SD.remove(entry.path())) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] failed to remove target entry\n");
                    }
                } else {
                    clear_dir(entry.path());
                    SD.rmdir(entry.path());
                }
                entry.close();
            }

            dir.close();

            unlock_mtx(ad_mtx);
        }
    }

    void move_all(const std::string& from_path, const std::string& to_path) {
        File dir = SD.open(from_path.c_str());
        if (!dir) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to open source directory\n");

            return;
        }

        if (lock_mtx(ad_mtx, portMAX_DELAY)) {
            File entry;
            while ((entry = dir.openNextFile())) {
                std::string source_path = from_path + "/" + entry.name();
                std::string target_path = to_path + "/" + entry.name();
                SD.rename(source_path.c_str(), target_path.c_str());

                entry.close();
            }

            dir.close();

            unlock_mtx(ad_mtx);
        }
    }

    static void init_mqtt_task(void* task_param) {
        std::string* _addr = static_cast<std::string*>(task_param);
        std::string addr(*_addr);
        delete _addr;

        if(!mqtt_event_q) {
            mqtt_event_q = xQueueCreate(COFFEE_QUEUE_SIZE * 2, sizeof(MQTT_Event*));

            if (!mqtt_event_q) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to initialize mqtt_event_q\n");

                vTaskDelete(nullptr);
                return;
            }
        }

        if (!read_cert_pem("/cert/ca.crt", ca_crt)
            || !read_cert_pem("/cert/client.crt", cli_crt)
            || !read_cert_pem("/cert/client.key", cli_key)) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to initialize certification informations\n");

            vTaskDelete(nullptr);
            return;
        }

        if (mqtt_prefix == "") {
            set_mqtt_prefix();
        }

        lock_mtx(mqtt_mtx, portMAX_DELAY);
        
        if (mqtt_client) {
            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);

            mqtt_client = nullptr;
        }

        esp_mqtt_client_config_t mqtt_cfg = { };

        temp_mqtt_uri = addr;

        std::string full_uri = std::string("mqtts://") + temp_mqtt_uri;
        
        mqtt_cfg.uri = full_uri.c_str();
        mqtt_cfg.username = nullptr;
        mqtt_cfg.password = nullptr;

        mqtt_cfg.reconnect_timeout_ms = COFFEE_NETWORK_TIMEOUT_MS;
        mqtt_cfg.task_stack = 8192;
        mqtt_cfg.task_prio = tskIDLE_PRIORITY + 4;

        // 인증서 관련 설정
        mqtt_cfg.cert_pem = ca_crt.c_str();
        mqtt_cfg.client_cert_pem = cli_crt.c_str();
        mqtt_cfg.client_key_pem = cli_key.c_str();

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

        esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_cb, NULL);

        esp_mqtt_client_start(mqtt_client);

        if (xTaskCreatePinnedToCore(mqtt_event_handle_task, "mqtt_event_handle", 4096, nullptr, tskIDLE_PRIORITY + 3, nullptr, 0) != pdPASS) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create event handle task...\n");

            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);

            mqtt_client = nullptr;
        }

        unlock_mtx(mqtt_mtx);

        init_mqtt_pub();

        if (check_cert_expired(cli_crt)) {
            std::string cli_csr = "";
            if (!generate_csr(cli_csr)) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to generate csr. update certifications aborted!\n");
            }

            pub_cert_request(cli_csr);
        }

        vTaskDelete(nullptr);
        return;
    }

    static void mqtt_event_cb(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
        esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

        switch (event->event_id) {
            case MQTT_EVENT_CONNECTED:
                mqtt_connected = true;
                queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT server connected\n");

                mqtt_uri = temp_mqtt_uri;

                if (lock_mtx(json_mtx, portMAX_DELAY)) {
                    if (!mqtt_config) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_config has not been initialized\n");

                        // 알려진 문제(#2) 참고
                        // esp_mqtt_client_stop(mqtt_client);
                        // esp_mqtt_client_destroy(mqtt_client);

                        // mqtt_client = nullptr;

                        unlock_mtx(json_mtx);

                        break;
                    }

                    {
                        cJSON* last_server = cJSON_GetObjectItem(mqtt_config, "last_server");
                        cJSON* sub_topics = cJSON_GetObjectItem(mqtt_config, "sub_topics");
                        if (!last_server || !cJSON_IsString(last_server)
                            || !sub_topics || !cJSON_IsArray(sub_topics)) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid `mqtt_config`\n");

                            // 알려진 문제(#2) 참고
                            // esp_mqtt_client_stop(mqtt_client);
                            // esp_mqtt_client_destroy(mqtt_client);

                            // mqtt_client = nullptr;

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

                            // 알려진 문제(#2) 참고
                            // esp_mqtt_client_stop(mqtt_client);
                            // esp_mqtt_client_destroy(mqtt_client);

                            // mqtt_client = nullptr;

                            unlock_mtx(json_mtx);

                            break;
                        }

                        cJSON_SetValuestring(last_server, mqtt_uri.c_str());
                    }

                    unlock_mtx(json_mtx);

                    write_config();
                }

                break;
            case MQTT_EVENT_DATA:
                {
                    // 알려진 문제(#1) 참고
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
                        enqueue_mqtt_event(acc_topic, acc_payload);
                        acc_topic.clear();
                        acc_payload.clear();
                    }
                }
                
                break;
            case MQTT_EVENT_DISCONNECTED:
                mqtt_connected = false;
                
                queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT server disconnected\n");

                mqtt_topic_ids.clear();

                break;
            case MQTT_EVENT_SUBSCRIBED:
                if(mqtt_topic_ids.find(event->msg_id) != mqtt_topic_ids.end())
                    queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT topic subscribed: %s\n", mqtt_topic_ids[event->msg_id].c_str());
                else
                    queue_printf(dbg_overlay_q, TAG, true, "[warning] unknown MQTT topic subscribed: %d\n", event->msg_id);

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
    }

    static void enqueue_mqtt_event(std::string topic, std::string payload) {
        MQTT_Event* me = new MQTT_Event(topic, payload);

        if (!mqtt_event_q) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] mqtt_event_q isn't initialized");

            delete me;

            return;
        }

        if (xQueueSend(mqtt_event_q, &me, portMAX_DELAY) != pdPASS) {
            delete me;
        }
    }

    static void mqtt_event_handle_task(void* task_param) {
        static MQTT_Event* buf;

        while (true) {
            if (xQueueReceive(mqtt_event_q, &buf, portMAX_DELAY) == pdPASS) {
                if (buf->_topic == mqtt_prefix + "update/request/firmware") {
                    cJSON* req_root = cJSON_Parse(buf->_payload.c_str());
                    if (!req_root) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
                        pub_error_log(TAG, "[error] invalid MQTT payload\n");
                    } else {
                        pub_request_ack(cJSON_GetObjectItem(req_root, "command_id")->valuestring);

                        if (xTaskCreatePinnedToCore(firmware_ota_task, "firmware_ota", 8192, req_root, tskIDLE_PRIORITY + 6, nullptr, 0) != pdPASS) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create firmware OTA task\n");
                            pub_error_log(TAG, "[error] failed to create firmware OTA task");

                            cJSON_Delete(req_root);
                        }
                    }
                } else if (buf->_topic == mqtt_prefix + "update/request/advertisement") {
                    cJSON* req_root = cJSON_Parse(buf->_payload.c_str());
                    if (!req_root) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
                        pub_error_log(TAG, "[error] invalid MQTT payload\n");
                    } else {
                        pub_request_ack(cJSON_GetObjectItem(req_root, "command_id")->valuestring);

                        if (xTaskCreatePinnedToCore(ad_ota_task, "ad_ota", 8192, req_root, tskIDLE_PRIORITY + 6, nullptr, 0) != pdPASS) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create advertisement OTA task\n");
                            pub_error_log(TAG, "[error] failed to create advertisement OTA task");

                            cJSON_Delete(req_root);
                        }
                    }
                } else if (buf->_topic == mqtt_prefix + "cert/request/ack") {
                    cJSON* req_root = cJSON_Parse(buf->_payload.c_str());
                    if (!req_root) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
                        pub_error_log(TAG, "[error] invalid MQTT payload\n");
                    } else {
                        cJSON* status = cJSON_GetObjectItem(req_root, "status");
                        if (!status) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
                            pub_error_log(TAG, "[error] invalid MQTT payload\n");
                        } else {
                            std::string req_res(status->valuestring);
                            if (req_res != "OK") {
                                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to authenticate information. please check your private key files\n");;
                            } else {
                                cJSON* root_ca = cJSON_GetObjectItem(req_root, "root_ca");
                                cJSON* client_ca = cJSON_GetObjectItem(req_root, "client_ca");

                                if (!root_ca || !client_ca) {
                                    queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
                                    pub_error_log(TAG, "[error] invalid MQTT payload\n");
                                } else {
                                    lock_mtx(cert_mtx, portMAX_DELAY);

                                    File new_ca_crt = SD.open("/cert/temp_ca.crt", FILE_WRITE);
                                    File new_cli_crt = SD.open("/cert/temp_client.crt", FILE_WRITE);

                                    if (!new_ca_crt) {
                                        queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create temporary certificate files\n");
                                        pub_error_log(TAG, "[error] failed to create temporary certificate files\n");
                                    } else if (!new_cli_crt) {
                                        new_ca_crt.close();

                                        queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create temporary certificate files\n");
                                        pub_error_log(TAG, "[error] failed to create temporary certificate files\n");
                                    } else {
                                        new_ca_crt.print(root_ca->valuestring);
                                        new_cli_crt.print(client_ca->valuestring);

                                        new_ca_crt.flush();
                                        new_ca_crt.close();

                                        new_cli_crt.flush();
                                        new_cli_crt.close();

                                        if (SD.exists("/cert/ca.crt")) {
                                            SD.remove("/cert/ca.crt");
                                        }

                                        if (SD.exists("/cert/client.crt")) {
                                            SD.remove("/cert/client.crt");
                                        }

                                        if (!SD.rename("/cert/temp_ca.crt", "/cert/ca.crt") || !SD.rename("/cert/temp_client.crt", "/cert/client.crt")) {
                                            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to rename temporary certification files. temporary files path: /cert/temp_*.crt\n");
                                            pub_error_log(TAG, "[error] failed to rename temporary certification files. temporary files path: /cert/temp_*.crt\n");
                                        }
                                    }

                                    unlock_mtx(cert_mtx);
                                }
                            }

                        }

                        cJSON_Delete(req_root);
                    }
                } else {
                    queue_printf(dbg_overlay_q, TAG, true, "[info] MQTT message received(%s): %s\n", buf->_topic.c_str(), buf->_payload.c_str());
                }

                delete buf;

                buf = nullptr;
            }
        }

        vTaskDelete(nullptr);
        return;
    }

    static void firmware_ota_task(void* task_param) {
        cJSON* req_root = static_cast<cJSON*>(task_param);

        if (!req_root) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            vTaskDelete(nullptr);
            return;
        }

        Download dl_info;
        
        dl_info.is_fw = true;

        cJSON* command_id = cJSON_GetObjectItem(req_root, "command_id");
        if (!command_id) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
            pub_error_log(TAG, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        dl_info.command_id = command_id->valuestring;

        cJSON* content = cJSON_GetObjectItem(req_root, "content");
        if (!content) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
            pub_error_log(TAG, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        cJSON* signed_url = cJSON_GetObjectItem(content, "signed_url");
        if (!signed_url) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        cJSON* url = cJSON_GetObjectItem(signed_url, "url");
        if (url && cJSON_IsString(url)) {
            dl_info.signed_url = url->valuestring;
        }

        cJSON* file_info = cJSON_GetObjectItem(content, "file_info");
        if (!file_info) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        cJSON* id = cJSON_GetObjectItem(file_info, "id");
        if (id && cJSON_IsNumber(id)) {
            dl_info.id = id->valueint;

            if (!SD.exists("/ota/")) {
                SD.mkdir("/ota/");
            }

            dl_info.storage_path = std::string("/ota/") + std::to_string(dl_info.id) + ".bin";
        }

        cJSON* file_hash = cJSON_GetObjectItem(file_info, "file_hash");
        if (file_hash && cJSON_IsString(file_hash)) {
            dl_info.hash_256 = file_hash->valuestring;
        }

        cJSON* size = cJSON_GetObjectItem(file_info, "size");
        if (size && cJSON_IsNumber(size)) {
            dl_info.file_size = size->valueint;
        }

        dl_info.acc_size = 0;
        dl_info.total_size = dl_info.file_size;

        cJSON_Delete(req_root);

        download_file(dl_info);

        delay(10);

        std::size_t buf = 0;
        if (xQueueReceive(ota_q, &buf, pdMS_TO_TICKS(COFFEE_UPDATE_TIMEOUT_MS)) != pdPASS) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] firmware download timeout\n");
        } else {
            ota(buf);
        }

        vTaskDelete(nullptr);
        return;
    }

    static void ad_ota_task(void* task_param) {
        cJSON* req_root = static_cast<cJSON*>(task_param);

        if (!req_root) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");

            vTaskDelete(nullptr);
            return;
        }

        cJSON* command_id = cJSON_GetObjectItem(req_root, "command_id");
        if (!command_id) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload\n");
            pub_error_log(TAG, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        cJSON* contents = cJSON_GetObjectItem(req_root, "contents");
        if (!contents || !cJSON_IsArray(contents)) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload: contents\n");
            pub_error_log(TAG, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        cJSON* total_size = cJSON_GetObjectItem(req_root, "total_size");
        if (!total_size || !cJSON_IsNumber(total_size)) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload: total_size\n");
            pub_error_log(TAG, "[error] invalid MQTT payload\n");

            cJSON_Delete(req_root);

            vTaskDelete(nullptr);
            return;
        }

        std::size_t contents_size = cJSON_GetArraySize(contents);
        std::size_t acc_downloaded = 0;
        for (std::size_t i = 0; i < contents_size; i++) {
            cJSON* content = cJSON_GetArrayItem(contents, i);

            Download dl_info;

            dl_info.is_fw = false;
            dl_info.command_id = command_id->valuestring;

            cJSON* signed_url = cJSON_GetObjectItem(content, "signed_url");
            if (!signed_url) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload: signed_url\n");

                cJSON_Delete(req_root);

                vTaskDelete(nullptr);
                return;
            }

            cJSON* url = cJSON_GetObjectItem(signed_url, "url");
            if (url && cJSON_IsString(url)) {
                dl_info.signed_url = url->valuestring;
            }

            cJSON* file_info = cJSON_GetObjectItem(content, "file_info");
            if (!file_info) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] invalid MQTT payload: file_info\n");

                cJSON_Delete(req_root);

                vTaskDelete(nullptr);
                return;
            }

            cJSON* id = cJSON_GetObjectItem(file_info, "id");
            if (id && cJSON_IsNumber(id)) {
                dl_info.id = id->valueint;

                if (!SD.exists("/res/temp")) {
                    SD.mkdir("/res/temp");
                }

                dl_info.storage_path = std::string("/res/temp/") + std::to_string(dl_info.id) + ".bin";
            }

            cJSON* file_hash = cJSON_GetObjectItem(file_info, "file_hash");
            if (file_hash && cJSON_IsString(file_hash)) {
                dl_info.hash_256 = file_hash->valuestring;
            }

            cJSON* size = cJSON_GetObjectItem(file_info, "size");
            if (size && cJSON_IsNumber(size)) {
                dl_info.file_size = size->valueint;
            }

            dl_info.acc_size = acc_downloaded;
            dl_info.total_size = total_size->valueint;

            download_file(dl_info);

            acc_downloaded += dl_info.file_size;
        }

        cJSON_Delete(req_root);

        delay(10);

        std::size_t downloaded_ad = 0;
        for(; downloaded_ad < contents_size; ) {
            bool buf = false;
            if (xQueueReceive(ad_q, &buf, pdMS_TO_TICKS(COFFEE_UPDATE_TIMEOUT_MS)) != pdPASS) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] ad update timeout\n");

                break;
            } else {
                downloaded_ad += buf;
            }
        }

        if (downloaded_ad == contents_size) {
            clear_dir("/res/contents");
            move_all("/res/temp", "/res/contents");
        }

        vTaskDelete(nullptr);
        return;
    }
}
