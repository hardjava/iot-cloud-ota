#ifndef COFFEE_MQTT_TASK_HPP
#define COFFEE_MQTT_TASK_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include <mqtt_client.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <cJSON.h>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/json_task.hpp"
#include "coffee/network_task.hpp"
#include "coffee/ota_task.hpp"

namespace coffee {
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
     * 
     * @param region 지역 구분 문자열
     * 
     *               region identifier string
     * 
     * @param store 매장 구분 문자열
     * 
     *              store identifier string
     */
    void set_mqtt_prefix(std::string region, std::string store);

    extern std::string mqtt_uri;

    extern std::string mqtt_region;

    extern std::string mqtt_store;

    extern esp_mqtt_client_handle_t mqtt_client;
}
#endif
