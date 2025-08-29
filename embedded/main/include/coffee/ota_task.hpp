#ifndef COFFEE_OTA_TASK_HPP
#define COFFEE_OTA_TASK_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include <esp_app_format.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <coffee_drv/display.hpp>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"

namespace coffee {
    /**
     * @brief 펌웨어 정보 클래스
     * 
     *        class representing firmware information
     */
    class Firmware {
    public:
        File file;
        std::string version;
    };

    /**
     * @brief 버전명을 통해 SD 카드에서 목표 펌웨어 파일 찾습니다
     * 
     *        finds the target firmware file on the SD card using the given version name
     * 
     * @param version 목표 펌웨어 버전
     * 
     *                the target firmware version
     * 
     * @return SD 카드에서 찾은 목표 펌웨어 파일
     * 
     *         the target firmware file found on the SD card
     */
    Firmware find_firmware_file(std::string version);

    /**
     * @brief 목표 펌웨어 파일을 이용하여 OTA를 수행합니다
     * 
     *        performs OTA update using the target firmware file
     * 
     * @param target_firmware OTA 목표 펌웨어
     * 
     *                        the target firmware file for OTA
     */
    void ota(Firmware target_firmware);
}
#endif
