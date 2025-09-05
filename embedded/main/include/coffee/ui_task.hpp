#ifndef COFFEE_UI_TASK_HPP
#define COFFEE_UI_TASK_HPP

#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <lvgl.h>

#include "coffee/config.hpp"
#include "coffee/ipc.hpp"
#include "ui.h"

namespace coffee {
    /**
     * @brief UI를 초기화하고 업데이트 태스크를 시작합니다
     * 
     *        initializes the UI and starts the update task
     * 
     * @return UI 초기화 성공 여부
     * 
     *         whether the UI was successfully initialized
     */
    bool init_ui_task(void);

    /**
     * @brief 화면 하단 디버그 오버레이를 초기화합니다
     * 
     *        initializes bottom debug overlay on the LVGL top layer
     */
    void init_dbg_overlay(void);

    /**
     * @brief 디버그 오버레이를 토글합니다
     * 
     *        toggles the debug overlay
     */
    void toggle_dbg_overlay(void);

    extern std::vector<std::string> ads;
}
#endif
