#include "coffee/ota_task.hpp"

namespace coffee {
    /**
     * @brief FreeRTOS 태스크를 통해 입력 버전명에 해당하는 펌웨어를 OTA 파티션에 업로드하고 적용합니다
     * 
     *        uploads and applies the firmware corresponding to the given version name onto the OTA partition using a FreeRTOS task
     */
    static void ota_task(void* task_param);

    static const std::string TAG = "coffee/ota_task";

    Firmware find_firmware_file(std::string version) {
        Firmware fw;

        std::string target_file_path = std::string("/ota/v") + version + ".bin";

        if (!SD.exists(target_file_path.c_str())) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] the firmware file for the corresponding version cannot be found\n");

            return fw;
        }

        fw.file = SD.open(target_file_path.c_str(), FILE_READ);
        if (!fw.file) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] the target firmware file cannot be opened\n");

            return fw;
        }

        fw.version = version;

        return fw;
    }

    void ota(Firmware target_firmware) {
        if (target_firmware.version == "") {
            return;
        }
        
        Firmware* task_param = new Firmware(target_firmware);

        if (xTaskCreatePinnedToCore(ota_task, "ota", 8192, task_param, tskIDLE_PRIORITY + 3, nullptr, 0) != pdPASS) {
            delete task_param;
        }
    }

    static void ota_task(void* task_param) {
        static std::size_t easter_egg_cnt = 0;

        Firmware* tp = static_cast<Firmware*>(task_param);
        Firmware fw = *tp;
        delete tp;

        easter_egg_cnt++;

        if (!lock_mtx(ota_mtx)) {
            if (easter_egg_cnt >= 5) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] STOP DOING THAT, SONG!!!\n");
            } else {
                queue_printf(dbg_overlay_q, TAG, true, "[error] another debugging session is already running\n");
            }

            vTaskDelete(nullptr);
        }

        easter_egg_cnt = 0;

        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA started\n");
        queue_printf(dbg_overlay_q, TAG, true, "[info] firmware version: %s\n", fw.version.c_str());

        // 파티션 정보 확인 및 OTA 타겟 파티션 결정
        const esp_partition_t* running_partition = esp_ota_get_running_partition();
        if (!running_partition) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to get running partition information\n");

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
        if (!target) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] the target partition cannot be detected\n");

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        std::string running_pt_str = running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0 ? "ota_0": "ota_1";
        std::string target_pt_str = target->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0 ? "ota_0": "ota_1";

        queue_printf(dbg_overlay_q, TAG, true, "[info] running partition is %s\n", running_pt_str.c_str());
        queue_printf(dbg_overlay_q, TAG, true, "[info] the new firmware will be stored in %s\n", target_pt_str.c_str());

        queue_printf(dbg_overlay_q, TAG, true, "[info] target partition: @0x%08X size=%uB\n", target->address, target->size);

        std::size_t firmware_size = fw.file.size();
        queue_printf(dbg_overlay_q, TAG, true, "[info] firmware file size: %zuB\n", firmware_size);

        if (firmware_size == 0 || firmware_size > target->size) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid size: firmware_size=%zuB, target->size=%uB\n", firmware_size, target->size);

            fw.file.close();

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        // 펌웨어 이미지 헤더 검사
        esp_image_header_t hdr{};

        fw.file.seek(0);
        
        std::size_t n = fw.file.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr));
        if (n != sizeof(hdr) || hdr.magic != ESP_IMAGE_HEADER_MAGIC) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] invalid firmware image\n");

            fw.file.close();

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        fw.file.seek(0);

        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA will begin in 5...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA will begin in 4...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA will begin in 3...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA will begin in 2...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] OTA will begin in 1...\n");
        delay(1000);

        coffee_drv::turn_off_lcd();

        // OTA 시작
        esp_ota_handle_t ota_handle = 0;
        esp_err_t err = esp_ota_begin(target, firmware_size, &ota_handle);
        if (err != ESP_OK) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] OTA begin failed: %s\n", esp_err_to_name(err));

            coffee_drv::turn_on_lcd();

            fw.file.close();

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        uint8_t* file_buf = reinterpret_cast<uint8_t*>(heap_caps_malloc(COFFEE_FILE_CHUNK_SIZE, MALLOC_CAP_SPIRAM));
        if (!file_buf) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] memory allocation failed\n");

            esp_ota_end(ota_handle);
            
            coffee_drv::turn_on_lcd();

            fw.file.close();

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        std::size_t total_written = 0;
        std::size_t read_size = 0;
        while (total_written != firmware_size && (read_size = fw.file.read(file_buf, COFFEE_FILE_CHUNK_SIZE)) != 0) {
            err = esp_ota_write(ota_handle, file_buf, read_size);
            if (err != ESP_OK) {
                queue_printf(dbg_overlay_q, TAG, true, "[error] OTA write failed: %s\n", esp_err_to_name(err));

                heap_caps_free(file_buf);

                esp_ota_end(ota_handle);
            
                coffee_drv::turn_on_lcd();

                fw.file.close();

                unlock_mtx(ota_mtx);

                vTaskDelete(nullptr);
            }

            total_written += read_size;
            queue_printf(dbg_overlay_q, TAG, true, "[info] written firmware: %zuB / %zuB\n", total_written, firmware_size);
        }

        heap_caps_free(file_buf);

        fw.file.close();

        err = esp_ota_end(ota_handle);
        if (err != ESP_OK) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] OTA end failed: %s\n", esp_err_to_name(err));

            coffee_drv::turn_on_lcd();

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        coffee_drv::turn_on_lcd();

        esp_app_desc_t new_firmware_desc{};
        err = esp_ota_get_partition_description(target, &new_firmware_desc);
        if (err == ESP_OK) {
            queue_printf(dbg_overlay_q, TAG, true, "[info] new image: project=%s, version=%s, time=%s %s\n",
                new_firmware_desc.project_name, new_firmware_desc.version,
                new_firmware_desc.date, new_firmware_desc.time);
        }

        // 부팅 파티션 설정
        err = esp_ota_set_boot_partition(target);
        if (err != ESP_OK) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] boot partition set failed: %s\n", esp_err_to_name(err));

            unlock_mtx(ota_mtx);

            vTaskDelete(nullptr);
        }

        queue_printf(dbg_overlay_q, TAG, true, "[info] boot partition set to %s\n", target_pt_str.c_str());

        // 재부팅
        queue_printf(dbg_overlay_q, TAG, true, "[info] rebooting in 5...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] rebooting in 4...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] rebooting in 3...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] rebooting in 2...\n");
        delay(1000);
        queue_printf(dbg_overlay_q, TAG, true, "[info] rebooting in 1...\n");
        delay(1000);

        esp_restart();

        // 여기에 도달하기 전에 리부팅
        unlock_mtx(ota_mtx);

        vTaskDelete(nullptr);
    }
}
