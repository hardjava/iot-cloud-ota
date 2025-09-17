#include "coffee/debug_task.hpp"

namespace coffee {
    /**
     * @brief debug1 작업을 비동기 처리하기 위한 FreeRTOS 태스크
     */
    static void debug1_task(void* task_param);
    
    /**
     * @brief debug2 작업을 비동기 처리하기 위한 FreeRTOS 태스크
     */
    static void debug2_task(void* task_param);

    // debug_task 태그
    static const std::string TAG = "coffee/debug_task";

    void debug1(void* debug_param) {
        if (xTaskCreatePinnedToCore(debug1_task, "debug1", 4096, debug_param, tskIDLE_PRIORITY + 6, nullptr, 0) != pdPASS) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create debug1 task\n");

            delete static_cast<std::string*>(debug_param);
        }
    }

    void debug2(void* debug_param) {
        if (xTaskCreatePinnedToCore(debug2_task, "debug2", 8192, debug_param, tskIDLE_PRIORITY + 6, nullptr, 0) != pdPASS) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create debug2 task\n");

            delete static_cast<std::string*>(debug_param);
        }
    }

    static void debug1_task(void* task_param) {
        std::string* _param = static_cast<std::string*>(task_param);
        std::string param(*_param);
        delete _param;

        if (mqtt_connected) {
            pub_test(param);
            queue_printf(debugTextArea_q, "", false, "Test message published: %s\n", param.c_str());
        } else {
            queue_printf(debugTextArea_q, "", false, "MQTT Client doesn't initialized!\n");
        }

        vTaskDelete(nullptr);
    }

    static void debug2_task(void* task_param) {
        std::string* _param = static_cast<std::string*>(task_param);
        std::string param(*_param);
        delete _param;

        queue_printf(debugTextArea_q, "", false, "%s\n", param.c_str());

        vTaskDelete(nullptr);
    }
}
