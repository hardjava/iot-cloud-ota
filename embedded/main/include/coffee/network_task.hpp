#ifndef COFFEE_NETWORK_TASK_HPP
#define COFFEE_NETWORK_TASK_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <mbedtls/sha256.h>

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
#include "coffee/json_task.hpp"
#include "coffee/mqtt_pub.hpp"
#include "coffee/rtc.hpp"

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
        Download(): is_fw(false),
            command_id(""),
            signed_url(""),
            storage_path(""),
            hash_256(""),
            file_size(0),
            acc_size(0),
            total_size(0),
            id(0) { }

        Download(const Download& d): is_fw(d.is_fw),
            command_id(d.command_id),
            signed_url(d.signed_url),
            storage_path(d.storage_path),
            hash_256(d.hash_256),
            file_size(d.file_size),
            acc_size(d.acc_size),
            total_size(d.total_size),
            id(d.id) { }

        Download& operator =(const Download& d) {
            is_fw = d.is_fw;
            command_id = d.command_id;
            signed_url = d.signed_url;
            storage_path = d.storage_path;
            hash_256 = d.hash_256;
            file_size = d.file_size;
            acc_size = d.acc_size;
            total_size = d.total_size;
            id = d.id;

            return *this;
        }

        bool is_fw;
        std::string command_id;
        std::string signed_url;
        std::string storage_path;
        std::string hash_256;
        std::size_t file_size;
        std::size_t acc_size;
        std::size_t total_size;
        std::size_t id;
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
     * @brief 파일 다운로드 정보를 받아 다운로드 대기열에 추가합니다
     * 
     * @param dl_info 다운로드에 필요한 관련 정보
     */
    void download_file(const Download& dl_info);

    extern void enqueue_ota(std::size_t id);

    extern QueueHandle_t wifiTextArea_q;

    extern QueueHandle_t debugTextArea_q;
}
#endif
