#ifndef COFFEE_MQTT_TASK_HPP
#define COFFEE_MQTT_TASK_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include <mqtt_client.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <cJSON.h>

#include "coffee/cert.hpp"
#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/json_task.hpp"
#include "coffee/mqtt_pub.hpp"
#include "coffee/network_task.hpp"
#include "coffee/ota_task.hpp"

namespace coffee {
    class MQTT_Event {
    public:
        MQTT_Event(const std::string& topic, const std::string& payload): _topic(topic), _payload(payload) { }
        MQTT_Event(const MQTT_Event& me): _topic(me._topic), _payload(me._payload) { }

        MQTT_Event& operator =(const MQTT_Event& me) {
            _topic = me._topic;
            _payload = me._payload;

            return *this;
        }

        std::string _topic;
        std::string _payload;
    };

    /**
     * @brief MQTT 클라이언트를 초기화합니다
     * 
     *        initializes the MQTT client
     * 
     * @param addr 접속할 MQTT 서버 주소
     */
    void init_mqtt(std::string addr);

    /**
     * @brief MQTT 토픽을 통한 기기 구분을 위해 접두사를 설정합니다
     * 
     *        sets the prefix used to distinguish devices in MQTT topics
     */
    void set_mqtt_prefix(void);

    /**
     * @brief 디렉토리 내부 파일들을 모두 지웁니다
     * 
     * @param path 내부를 비울 디렉토리 주소
     */
    void clear_dir(const std::string& path);

    /**
     * @brief 디렉토리 내 모든 파일들을 옮깁니다
     * 
     * @param from_path 옮길 파일들을 포함한 디렉토리 주소
     * 
     * @param to_path 도착 디렉토리 주소
     */
    void move_all(const std::string& from_path, const std::string& to_path);

    extern std::string mqtt_uri;

    extern std::string mqtt_prefix;

    extern esp_mqtt_client_handle_t mqtt_client;

    extern bool mqtt_connected;
}
#endif
