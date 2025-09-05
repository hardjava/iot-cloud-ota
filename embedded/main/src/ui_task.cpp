#include "coffee/ui_task.hpp"

namespace coffee {
    /**
     * @brief SD 카드에서 모든 광고 이미지 주소를 불러옵니다
     * 
     *        loads all advertisement image addresses from the SD card
     */
    static bool get_ads(void);

    /**
     * @brief UI 업데이트 태스크
     * 
     *        UI updating task
     */
    static void ui_task(void* task_param);

    //디버그 오버레이 배경
    // background object of the debug overlay
    static lv_obj_t* dbg_overlay_bar = nullptr;

    // 디버그 오버레이 배경 스타일
    // style for the background of the debug overlay
    static lv_style_t dbg_overlay_bar_style;

    // 디버그 오버레이 레이블
    // label object of the debug overlay
    static lv_obj_t* dbg_overlay_label = nullptr;

    // 디버그 오버레이 레이블 스타일
    // style for the label of the debug overlay
    static lv_style_t dbg_overlay_label_style;
    
    /**
     * @brief 광고 이미지 주소 리스트
     * 
     *        list of advertisement image addresses
     */
    std::vector<std::string> ads;

    bool init_ui_task(void) {
        Serial.println("[coffee/ui_task][info] initializing UI...");

        if (!get_ads()) {
            Serial.println("[coffee/ui_task][error] failed to read the list of ad images");

            return false;
        }

        ui_init();

        xTaskCreatePinnedToCore(coffee::ui_task, "ui", 8192, nullptr, tskIDLE_PRIORITY + 2, nullptr, 1);

        return true;
    }

    void init_dbg_overlay(void) {
        if (dbg_overlay_bar) {
            return;
        }

        lv_style_init(&dbg_overlay_bar_style);
        lv_style_set_bg_opa(&dbg_overlay_bar_style, LV_OPA_60);
        lv_style_set_bg_color(&dbg_overlay_bar_style, lv_color_black());
        lv_style_set_border_width(&dbg_overlay_bar_style, 0);
        lv_style_set_pad_all(&dbg_overlay_bar_style, 6);

        lv_style_init(&dbg_overlay_label_style);
        lv_style_set_text_color(&dbg_overlay_label_style, lv_color_white());

        dbg_overlay_bar = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(dbg_overlay_bar);
        lv_obj_add_style(dbg_overlay_bar, &dbg_overlay_bar_style, 0);
        lv_obj_set_size(dbg_overlay_bar, lv_pct(100), 28);
        lv_obj_align(dbg_overlay_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(dbg_overlay_bar, LV_OBJ_FLAG_FLOATING);

        dbg_overlay_label = lv_label_create(dbg_overlay_bar);
        lv_obj_add_style(dbg_overlay_label, &dbg_overlay_label_style, 0);
        lv_label_set_long_mode(dbg_overlay_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(dbg_overlay_label, lv_pct(100));
        lv_obj_align(dbg_overlay_label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_label_set_text(dbg_overlay_label, "[coffee/ui_task][info] debug overlay initialization success!");
        
        Serial.println("[coffee/ui_task][info] debug overlay initialization success!");
    }

    void toggle_dbg_overlay(void) {
        if (!dbg_overlay_bar) {
            return;
        }

        if (lv_obj_has_flag(dbg_overlay_bar, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(dbg_overlay_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(dbg_overlay_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }

    static bool get_ads(void) {
        static const std::string AD_DIR = "/res/contents";
        File dir = SD.open(AD_DIR.c_str());
        if (!dir) {
            Serial.printf("[coffee/ui_task][error] failed to open ad directory\n");

            return false;
        }

        File ad;
        while ((ad = dir.openNextFile())) {
            if (!ad.isDirectory()) {
                std::string ad_path = std::string("S:") + AD_DIR + "/" + ad.name();
                ads.push_back(ad_path);
            }
            ad.close();
        }

        dir.close();

        return true;
    }

    static void ui_task(void* task_param) {
        char buf[COFFEE_MAX_STR_BUF] = { 0 };

        while (true) {
            if (dbg_overlay_label && queue_poll(dbg_overlay_q, buf)) {
                lv_label_set_text(dbg_overlay_label, buf);
            }

            lv_timer_handler();

            delay(10);
        }

        vTaskDelete(nullptr);
    }
}
