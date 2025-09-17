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
#include <freertos/queue.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <coffee_drv/display.hpp>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"

namespace coffee {
    /**
     * @brief 펌웨어 OTA를 수행합니다
     * 
     * @param id 업데이트할 펌웨어의 ID
     */
    void ota(std::size_t id);

    extern QueueHandle_t ota_q;
}
#endif
