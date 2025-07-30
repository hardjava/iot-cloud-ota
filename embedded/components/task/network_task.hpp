#ifndef COFFEE_NETWORK_TASK_HPP
#define COFFEE_NETWORK_TASK_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include <Arduino.h>
#include <WiFi.h>

#include <wi-fi.hpp>

#define COFFEE_MAX_STR_LEN 128

namespace coffee {
    /**
     * @brief Wi-Fi에 연결합니다
     * 
     *        connects to Wi-Fi
     *
     * @param ssid 연결할 AP의 SSID
     * 
     *             SSID of the AP to connect
     * 
     * @param pw 연결할 AP의 비밀번호
     * 
     *           password of the AP to connect
     */
    void connect_wifi(const char* ssid, const char* pw);

    /**
     * @brief 연결된 서버에 OK 메시지를 송신합니다
     * 
     *        sends an OK message to the connected server
     */
    void send_ok(void);

    extern WiFiClient client;

    extern SemaphoreHandle_t client_mtx;

    extern QueueHandle_t chatTextArea_q;
}
#endif
