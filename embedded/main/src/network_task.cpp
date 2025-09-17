#include "coffee/network_task.hpp"

namespace coffee {
    /**
     * @brief FreeRTOS 태스크를 통해 Wi-Fi 연결을 시도합니다
     * 
     *        attempts to connect to Wi-Fi using a FreeRTOS task
     */
    static void init_wifi_task(void* task_param);

    /**
     * @brief 대기열에서 다운로드 정보를 받아와 파일을 다운로드합니다
     */
    static void download_task(void* task_param);

    /**
     * @brief 길이 64의 해시 문자열을 32바이트 바이너리로 변환하는 헬퍼
     * 
     *        helper function to convert a 64-character hash string into a 32-byte binary array
     */
    static bool hex_to_bin32(const std::string& hash, uint8_t out[32]);

    // 로그에 출력되는 태그
    // tag used for log messages
    static const std::string TAG = "coffee/network_task";

    static QueueHandle_t download_q = nullptr;

    static TaskHandle_t download_task_hd = nullptr;

    void connect_wifi(std::string ssid, std::string pw) {
        ConInfo* task_args = new ConInfo();
        if (!task_args) {
            queue_printf(dbg_overlay_q, TAG, true, "[warning] failed to allocate task_args\n");

            return;
        }

        task_args->ssid = ssid;
        task_args->pw = pw;

        if (!download_q) {
            download_q = xQueueCreate(COFFEE_QUEUE_SIZE * 2, sizeof(Download*));
            if (!download_q) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create `download_q`\n");

                return;
            }
        }

        if (xTaskCreatePinnedToCore(init_wifi_task, "init_wifi", 4096, task_args, tskIDLE_PRIORITY + 6, nullptr, 0) != pdPASS) {
            Serial.println("[coffee/network_task.cpp][error] failed to create init_wifi task");

            delete task_args;
        }
    }

    void download_file(const Download& dl_info) {
        Download* task_args = new Download(dl_info);
        if (!task_args) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to allocate task_args\n");

            return;
        }

        if(xQueueSend(download_q, &task_args, portMAX_DELAY) != pdPASS) {
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

            vTaskDelete(nullptr);
            return;
        }

        easter_egg_cnt = 0;

        queue_printf(wifiTextArea_q, "", false, "Attempting to connect...\n");

        if (coffee_drv::init_wifi_sta(ci.ssid, ci.pw)) {
            queue_printf(wifiTextArea_q, "", false, "Successfully connected! local IP=%s\n", WiFi.localIP().toString().c_str());

            if (wifi_config) {
                if (lock_mtx(json_mtx, portMAX_DELAY)) {
                    cJSON* last_ssid = cJSON_GetObjectItem(wifi_config, "last_ssid");
                    cJSON* last_password = cJSON_GetObjectItem(wifi_config, "last_password");
                    if (last_ssid && last_password) {
                        cJSON_SetValuestring(last_ssid, ci.ssid.c_str());
                        cJSON_SetValuestring(last_password, ci.pw.c_str());
                    }

                    unlock_mtx(json_mtx);
                }

                write_config();
            }
        } else {
            queue_printf(wifiTextArea_q, "", false, "Connection failed(Time Out)\n");
        }

        unlock_mtx(network_mtx);

        delay(1000);

        init_rtc();
        if (!wait_time_sync(10)) {
                queue_printf(dbg_overlay_q, TAG, true, "[warning] time sync timeover\n");
            if (!wait_time_sync(30)) {
                queue_printf(dbg_overlay_q, TAG, true, "[warning] time sync timeover\n");
            } else {
                queue_printf(dbg_overlay_q, TAG, true, "[info] time sync success!\n");
            }
        } else {
            queue_printf(dbg_overlay_q, TAG, true, "[info] time sync success!\n");
        }

        if (!download_task_hd) {
            if (xTaskCreatePinnedToCore(download_task, "download", 8192, nullptr, tskIDLE_PRIORITY + 5, &download_task_hd, 0) != pdPASS) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create download task\n");
            }
        }

        vTaskDelete(nullptr);
        return;
    }

    static void download_task(void* task_param) {
        static Download* buf = nullptr;

        while (true) {
            if (xQueueReceive(download_q, &buf, portMAX_DELAY) == pdPASS) {
                if (!buf) {
                    continue;
                }

                Download dl = *buf;
                delete buf;
                buf = nullptr;

                if (WiFi.status() != WL_CONNECTED) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] unable to connect to the network\n");

                    continue;
                }

                lock_mtx(network_mtx, portMAX_DELAY);

                WiFiClientSecure client;
                client.setTimeout(COFFEE_NETWORK_TIMEOUT_MS);
                client.setInsecure();

                HTTPClient http;

                if (!http.begin(client, dl.signed_url.c_str())) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] HTTPS begin failed\n");

                    client.stop();

                    unlock_mtx(network_mtx);

                    continue;
                }

                int res_code = http.GET();
                if (res_code != HTTP_CODE_OK) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] HTTPS GET failed, code=%d\n", res_code);

                    http.end();

                    client.stop();

                    unlock_mtx(network_mtx);

                    continue;
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

                    continue;
                }

                WiFiClient* stream = http.getStreamPtr();
                
                uint8_t* buf = reinterpret_cast<uint8_t*>(heap_caps_malloc(COFFEE_FILE_CHUNK_SIZE, MALLOC_CAP_SPIRAM));
                if (!buf) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] file buffer allocation failed\n");

                    new_file.close();

                    http.end();

                    client.stop();

                    unlock_mtx(network_mtx);

                    continue;
                }

                queue_printf(dbg_overlay_q, TAG, true, "[info] file download started...\n");

                uint8_t bin_hash[32] = { 0 };

                bool hash_arrived = false;
                
                if (!dl.hash_256.empty()) {
                    if (!hex_to_bin32(dl.hash_256, bin_hash)) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] invalid expected SHA-256 hex string\n");
                        
                        heap_caps_free(buf);

                        new_file.close();

                        http.end();

                        client.stop();
                        
                        unlock_mtx(network_mtx);
                        
                        continue;
                    } else {
                        hash_arrived = true;
                    }
                } else {
                    queue_printf(dbg_overlay_q, TAG, true, "[warning] received hash information is empty. hash will be computed but not compared\n");
                }

                mbedtls_sha256_context sha_ctx;
                mbedtls_sha256_init(&sha_ctx);
                mbedtls_sha256_starts_ret(&sha_ctx, 0);

                std::size_t total_written = 0;
                int64_t wait_start = esp_timer_get_time();
                bool ok = true;

                int64_t t0 = esp_timer_get_time();
                while (total_written < dl.file_size) {
                    size_t want = dl.file_size - total_written;
                    if (want > COFFEE_FILE_CHUNK_SIZE) {
                        want = COFFEE_FILE_CHUNK_SIZE;
                    }

                    int n = stream->readBytes(buf, want);

                    if (n > 0) {
                        size_t w = new_file.write(buf, n);
                        if (w != static_cast<size_t>(n)) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] file write failed\n");

                            ok = false;
                            pub_update_result(dl.command_id, "ERROR", "file write failed", esp_timer_get_time() - t0);
                            
                            break;
                        }

                        if (mbedtls_sha256_update_ret(&sha_ctx, buf, n) != 0) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] sha256 update failed\n");

                            ok = false;
                            pub_update_result(dl.command_id, "ERROR", "sha256 update failed", esp_timer_get_time() - t0);
                            
                            break;
                        }

                        total_written += n;

                        wait_start = esp_timer_get_time();
                    } else {
                        int64_t wait_time = (esp_timer_get_time() - wait_start) / 1000;
                        if (wait_time >= COFFEE_NETWORK_TIMEOUT_MS) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] network timeout\n");

                            ok = false;
                            pub_update_result(dl.command_id, "TIMEOUT", "network timeout", esp_timer_get_time() - t0);
                            
                            break;
                        }
                    }

                    if (ok) {
                        static std::size_t cnt = 0;
                        
                        if (cnt == 0 || total_written == dl.file_size) {
                            queue_printf(dbg_overlay_q, TAG, true, "[info] file downloaded %zuB / %zuB\n", total_written, dl.file_size);
                            pub_update_progress(dl.command_id, dl.acc_size + total_written, dl.total_size, esp_timer_get_time() - t0);
                        }

                        cnt = (cnt + 1) % 4;
                    }

                    delay(10);
                }

                std::size_t t1 = esp_timer_get_time();

                if (ok && total_written != dl.file_size) {
                    queue_printf(dbg_overlay_q, TAG, true, "[error] size mismatch: got %zuB / expected %zuB\n", total_written, dl.file_size);

                    ok = false;
                    pub_update_result(dl.command_id, "ERROR", "downloaded file size mismatch", t1 - t0);
                }

                if (ok) {
                    uint8_t calc_hash[32];

                    if (mbedtls_sha256_finish_ret(&sha_ctx, calc_hash) != 0) {
                        queue_printf(dbg_overlay_q, TAG, true, "[error] SHA-256 finish failed\n");
                        pub_update_result(dl.command_id, "ERROR", "SHA-256 finish failed", t1 - t0);

                        ok = false;
                    } else {
                        static const char* hexd = "0123456789abcdef";

                        char hex_buf[65] = {0};

                        for (int i = 0; i < 32; ++i) {
                            hex_buf[i * 2] = hexd[(calc_hash[i] >> 4) & 0xF];
                            hex_buf[i * 2 + 1] = hexd[calc_hash[i] & 0xF];
                        }

                        if (hash_arrived) {
                            if (memcmp(calc_hash, bin_hash, 32) != 0) {
                                queue_printf(dbg_overlay_q, TAG, true, "[error] hash mismatch - calculated hash: %s\n", hex_buf);
                                pub_update_result(dl.command_id, "ERROR", "hash mismatch", t1 - t0);

                                ok = false;
                            }
                        } else {
                            // 참고용
                            queue_printf(dbg_overlay_q, TAG, true, "[info] calculated hash: %s\n", hex_buf);
                        }
                    }
                }

                mbedtls_sha256_free(&sha_ctx);
                
                new_file.flush();
                new_file.close();

                if (!ok) {
                    // 실패 시 부분 파일은 제거(재다운로드 필요)
                    SD.remove(dl.storage_path.c_str());
                } else {
                    queue_printf(dbg_overlay_q, TAG, true, "[info] file download success! size: %zuB, hash check: OK\n", total_written);
                    pub_update_result(dl.command_id, "SUCCESS", "file download success!", t1 - t0);

                    if(dl.is_fw) {
                        std::size_t _id = dl.id;
                        if (xQueueSend(ota_q, &_id, portMAX_DELAY) != pdPASS) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to enqueue firmware download done message\n");
                        }
                    } else {
                        bool b = true;
                        if (xQueueSend(ad_q, &b, portMAX_DELAY) != pdPASS) {
                            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to enqueue ad downloaded done message\n");
                        }
                    }
                }

                heap_caps_free(buf);

                http.end();

                client.stop();

                unlock_mtx(network_mtx);

                continue;
            }
        }

        vTaskDelete(nullptr);
        return;
    }

    static bool hex_to_bin32(const std::string& hash, uint8_t out[32]) {
        if (hash == "" || hash.size() != 64) {
            return false;
        }

        auto hexval = [](char c) -> int {
            if ('0' <= c && c <= '9') {
                return c - '0';
            }

            if ('a' <= c && c <= 'f') {
                return 10 + (c - 'a');
            }
            
            if ('A' <= c && c <= 'F') {
                return 10 + (c - 'A');
            }

            return -1;
        };

        for (int i = 0; i < 32; ++i) {
            int hi = hexval(hash[i * 2]), lo = hexval(hash[i * 2 + 1]);

            if (hi < 0 || lo < 0) {
                return false;
            }

            out[i] = static_cast<uint8_t>((hi << 4) | lo);
        }

        return true;
    }
}
