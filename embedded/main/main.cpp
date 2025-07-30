#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <Arduino.h>

#include <driver.hpp>

#include <gui_task.hpp>

#define COFFEE_BAUD_RATE 115200

extern "C" void app_main(void) {
    initArduino();
    Serial.begin((unsigned long) COFFEE_BAUD_RATE);

    if (!coffee::init_drivers())
        return;

    coffee::init_gui();
}
