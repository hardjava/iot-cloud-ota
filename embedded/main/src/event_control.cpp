#include <coffee/event_control.hpp>

namespace coffee {
    /**
     * @brief 디버깅 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control debugging related events
     */
    SemaphoreHandle_t debug_mtx = nullptr;

    /**
     * @brief Wi-Fi 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control Wi-Fi related events
     */
    SemaphoreHandle_t network_mtx = nullptr;

    /**
     * @brief OTA 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control OTA related events
     */
    SemaphoreHandle_t ota_mtx = nullptr;

    void init_mtx(void) {
        if (!debug_mtx) {
            debug_mtx = xSemaphoreCreateMutex();
        }

        if (!network_mtx) {
            network_mtx = xSemaphoreCreateMutex();
        }

        if (!ota_mtx) {
            ota_mtx = xSemaphoreCreateMutex();
        }

        Serial.println("[coffee/event_control][info] mutex initialization success!");
    }

    bool lock_mtx(SemaphoreHandle_t& mtx) {
        return (xSemaphoreTake(mtx, pdMS_TO_TICKS(COFFEE_MTX_TIMEOUT_MS)) == pdTRUE);
    }
    
    void unlock_mtx(SemaphoreHandle_t& mtx) {
        xSemaphoreGive(mtx);
    }
}
