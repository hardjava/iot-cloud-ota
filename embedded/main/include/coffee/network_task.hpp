#ifndef COFFEE_NETWORK_TASK_HPP
#define COFFEE_NETWORK_TASK_HPP

#include <cstddef>
#include <string>

#include <esp_heap_caps.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <Arduino.h>
#include <FS.h>
#include <HTTPClient.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <cJSON.h>

#include <coffee_drv/wifi.hpp>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"

namespace coffee {
    /**
     * @brief Wi-Fi 연결 정보 클래스
     * 
     *        class representing Wi-Fi connection information
     */
    class ConInfo {
    public:
        std::string ssid;
        std::string pw;
    };

    /**
     * @brief 다운로드 정보 클래스
     * 
     *        class representing download information
     */
    class Download {
    public:
        std::string signed_url;
        std::string storage_path;
    };

    /**
     * @brief Wi-Fi에 연결합니다
     * 
     *        connects to Wi-Fi
     *
     * @param ssid 연결할 Wi-Fi의 SSID
     * 
     *             the SSID of the Wi-Fi to connect
     * 
     * @param pw 연결할 Wi-Fi의 비밀번호
     * 
     *           the password of the Wi-Fi to connect
     */
    void connect_wifi(std::string ssid, std::string pw);

    /**
     * @brief Signed URL로부터 파일을 받아 SD 카드에 저장합니다
     * 
     *        downloads a file from the given signed URL and saves it to the SD card
     * 
     * @param signed_url 파일을 다운로드할 Signed URL
     * 
     *                   the signed URL from which the file will be downloaded
     * 
     * @param storage_path 파일을 저장할 SD 카드 내 위치
     * 
     *                     the path on the SD card where the file will be stored
     */
    void download_file(std::string signed_url, std::string storage_path);

    extern QueueHandle_t wifiTextArea_q;

    extern QueueHandle_t debugTextArea_q;
}
#endif
