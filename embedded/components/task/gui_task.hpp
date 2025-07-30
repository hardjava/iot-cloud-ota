#ifndef COFFEE_GUI_TASK_HPP
#define COFFEE_GUI_TASK_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <lvgl.h>

#include <ui.h>

#include "network_task.hpp"

namespace coffee {
    /**
     * @brief 동시 접근 제어 객체들을 초기화하고 GUI 태스크를 생성합니다
     * 
     *        initializes synchronization objects and creates the GUI task
     */
    void init_gui(void);

    /**
     * @brief GUI 생성 및 업데이트 태스크
     * 
     *        task for creating and updating the GUI
     */
    void gui_task(void* task_param);

    /**
     * @brief 텍스트를 화면과 시리얼 모니터에 출력합니다
     * 
     * 		  prints text to the display and serial monitor
     */
    static void print_text(const char* text);
}
#endif
