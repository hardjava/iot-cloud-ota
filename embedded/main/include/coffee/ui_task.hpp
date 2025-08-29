#ifndef COFFEE_UI_TASK_HPP
#define COFFEE_UI_TASK_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>

#include <lvgl.h>

#include "coffee/config.hpp"
#include "coffee/ipc.hpp"
#include "ui.h"

namespace coffee {
    /**
     * @brief UI를 초기화하고 업데이트 태스크를 시작합니다
     * 
     *        initializes the UI and starts the update task
     */
    void init_ui_task(void);

    /**
     * @brief 화면 하단 디버그 오버레이를 초기화합니다
     * 
     *        initializes bottom debug overlay on the LVGL top layer
     */
    void init_dbg_overlay(void);
}
#endif
