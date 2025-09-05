#ifndef COFFEE_JSON_TASK_HPP
#define COFFEE_JSON_TASK_HPP

#include <cstdint>
#include <string>

#include <esp_heap_caps.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <cJSON.h>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"

#define COFFEE_CONFIG_JSON_MAX_BYTES 1024

#define COFFEE_CONFIG_JSON_FILE_PATH "/res/network_config.json"

#define COFFEE_TEMP_CONFIG_FILE_PATH "/res/network_config.json.tmp"

namespace coffee {
    /**
     * @brief JSON 파일로부터 네트워크 정보를 읽어 처리합니다
     * 
     *        reads and processes network information from a JSON file
     */
    void init_network_config(void);

    /**
     * @brief JSON 파일로부터 마지막 Wi-Fi 연결 정보를 불러옵니다
     * 
     *        loads the last Wi-Fi connection information from a JSON file
     * 
     * @param[out] ssid_out 마지막으로 연결된 Wi-Fi SSID
     * 
     *                      the SSID of the last connected Wi-Fi
     * 
     * @param[out] pw_out 마지막으로 연결된 Wi-Fi 비밀번호
     * 
     *                    the password of the last connected Wi-Fi
     * 
     * @return 마지막 Wi-Fi 연결 정보 불러오기 성공 여부
     * 
     *         whether loading the last Wi-Fi connection information succeeded
     */
    void get_last_wifi(std::string& ssid_out, std::string& pw_out);

    /**
     * @brief JSON 파일로부터 마지막 MQTT 서버 연결 정보를 불러옵니다
     * 
     *        loads the last MQTT server connection information from a JSON file
     * 
     * @param[out] addr_out 마지막으로 연결된 MQTT 서버 주소
     * 
     *                      the MQTT server address of the last connected Wi-Fi
     * 
     * @return 마지막 MQTT 서버 연결 정보 불러오기 성공 여부
     * 
     *         whether loading the last MQTT server connection information succeeded
     */
    void get_last_server(std::string& addr_out);

    /**
     * @brief config_root의 내용을 이용하여 파일을 업데이트 합니다
     * 
     *        updates the file using the contents of config_root
     * 
     * @return 파일 업데이트 성공 여부
     * 
     *         whether the file update was successful
     */
    void write_config(void);

    extern cJSON* config_root;

    extern cJSON* wifi_config;

    extern cJSON* mqtt_config;
}
#endif
