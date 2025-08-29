#ifndef COFFEE_DEBUG_TASK_HPP
#define COFFEE_DEBUG_TASK_HPP

#include <cstddef>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>
#include <HTTPClient.h>

#include <cJSON.h>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/network_task.hpp"
#include "coffee/ota_task.hpp"

namespace coffee {
    /**
     * @brief 펌웨어 버전마다 달라지는 디버그용 함수 1
     * 
     *        debugging function 1 that varies depending on the firmware version
     * 
     * @param debug_param 디버그에 전달할 매개 변수
     * 
     *                    parameter passed for debugging
     */
    void debug1(void* debug_param);

    /**
     * @brief 펌웨어 버전마다 달라지는 디버그용 함수 2
     * 
     *        debugging function 2 that varies depending on the firmware version
     * 
     * @param debug_param 디버그에 전달할 매개 변수
     * 
     *                    parameter passed for debugging
     */
    void debug2(void* debug_param);
}
#endif
