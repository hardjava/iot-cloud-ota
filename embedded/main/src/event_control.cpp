#include <coffee/event_control.hpp>

namespace coffee {
    /**
     * @brief 디버깅 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control debugging related events
     */
    SemaphoreHandle_t debug_mtx = nullptr;

    /**
     * @brief cJSON 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control cJSON related events
     */
    SemaphoreHandle_t json_mtx = nullptr;

    /**
     * @brief MQTT 관련 이벤트를 제어하는 뮤텍스
     * 
     *        mutex used to control MQTT related events
     */
    SemaphoreHandle_t mqtt_mtx = nullptr;

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

    /**
     * @brief 광고 관련 이벤트를 제어하는 뮤텍스
     */
    SemaphoreHandle_t ad_mtx = nullptr;

    /**
     * @brief 인증서 관련 파일 접근 보호를 위한 뮤텍스
     * 
     * @details 인증서 갱신 시도 시 동일 파일에 읽기 접근이 일어날 수 있기 때문에 이를 보호하기 위하여 추가하였습니다
     */
    SemaphoreHandle_t cert_mtx = nullptr;

    void init_mtx(void) {
        if (!debug_mtx) {
            debug_mtx = xSemaphoreCreateMutex();
        }
        
        if (!json_mtx) {
            json_mtx = xSemaphoreCreateMutex();
        }

        if (!mqtt_mtx) {
            mqtt_mtx = xSemaphoreCreateMutex();
        }

        if (!network_mtx) {
            network_mtx = xSemaphoreCreateMutex();
        }

        if (!ota_mtx) {
            ota_mtx = xSemaphoreCreateMutex();
        }

        if (!ad_mtx) {
            ad_mtx = xSemaphoreCreateMutex();
        }

        if (!cert_mtx) {
            cert_mtx = xSemaphoreCreateMutex();
        }

        if (!debug_mtx || !json_mtx || !mqtt_mtx || !network_mtx || !ota_mtx || !ad_mtx || !cert_mtx) {
            Serial.println("[coffee/event_control][error] failed to initialize mutexes");
        } else {
            Serial.println("[coffee/event_control][info] mutex initialization success!");
        }
    }

    bool lock_mtx(SemaphoreHandle_t& mtx, std::size_t wait_time) {
        if (!mtx) {
            return false;
        }

        if (wait_time == portMAX_DELAY) {
            return (xSemaphoreTake(mtx, portMAX_DELAY) == pdTRUE);
        } else {
            return (xSemaphoreTake(mtx, pdMS_TO_TICKS(wait_time)) == pdTRUE);
        }
    }
    
    void unlock_mtx(SemaphoreHandle_t& mtx) {
        if (!mtx) {
            return;
        }

        xSemaphoreGive(mtx);
    }
}
