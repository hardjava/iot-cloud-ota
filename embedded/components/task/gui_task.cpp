#include "gui_task.hpp"

namespace coffee {
    void init_gui(void) {
        ui_init();

        chatTextArea_q = xQueueCreate(8, COFFEE_MAX_STR_LEN);
        client_mtx = xSemaphoreCreateMutex();

        xTaskCreatePinnedToCore(coffee::gui_task, "gui", 8192, NULL, 4, NULL, 1);
    }

    void gui_task(void* task_param) {
        char recieved_text[COFFEE_MAX_STR_LEN];

        // UI에서 실수한 부분 수정
        // correct mistakes in the UI
        lv_textarea_add_text(ui_chatTextArea, "\n");

        while (true) {
            if (xQueueReceive(chatTextArea_q, &recieved_text, 0) == pdPASS)
                print_text(recieved_text);

            lv_timer_handler();

            delay(5);
        }
    }

    static void print_text(const char* text) {
        lv_textarea_add_text(ui_chatTextArea, text);

        Serial.print("gui_task: print text\n\t");
        Serial.println(text);
    }
}
