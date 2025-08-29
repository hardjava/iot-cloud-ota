#ifndef COFFEE_MAIN_CONFIG_HPP
#define COFFEE_MAIN_CONFIG_HPP

#define COFFEE_STR_HELPER(x) #x
#define COFFEE_STR(x) COFFEE_STR_HELPER(x)

/**
 * @def COFFEE_FIRMWARE_VER_MAJOR
 * 
 * @def COFFEE_FIRMWARE_VER_MINOR
 * 
 * @def COFFEE_FIRMWARE_VER_PATCH
 * 
 * @def COFFEE_FIRMWARE_VER_NUM
 * 
 * @def COFFEE_ALPHA_TEST
 * 
 * @def COFFEE_BETA_TEST
 * 
 * @def COFFEE_FIRMWARE_VER
 * 
 * @brief 펌웨어 버전 정보
 * 
 *        firmware version information
 */
#define COFFEE_FIRMWARE_VER_MAJOR 0
#define COFFEE_FIRMWARE_VER_MINOR 0
#define COFFEE_FIRMWARE_VER_PATCH 3

#define COFFEE_FIRMWARE_VER_NUM COFFEE_STR(COFFEE_FIRMWARE_VER_MAJOR) "." COFFEE_STR(COFFEE_FIRMWARE_VER_MINOR) "." COFFEE_STR(COFFEE_FIRMWARE_VER_PATCH)

#define COFFEE_ALPHA_TEST 1
#define COFFEE_BETA_TEST  0
#if (COFFEE_ALPHA_TEST && COFFEE_BETA_TEST)
    #error "only one of COFFEE_ALPHA_TEST or COFFEE_BETA_TEST can be 1"
#endif

#if COFFEE_ALPHA_TEST
  #define COFFEE_FIRMWARE_VER COFFEE_FIRMWARE_VER_NUM "-alpha"
#elif COFFEE_BETA_TEST
  #define COFFEE_FIRMWARE_VER COFFEE_FIRMWARE_VER_NUM "-beta"
#else
  #define COFFEE_FIRMWARE_VER COFFEE_FIRMWARE_VER_NUM
#endif


/*
   이벤트 제어 설정

   event control configurations
 */

/**
 * @def COFFEE_MTX_TIMEOUT_MS
 * 
 * @brief 뮤텍스 잠금 대기 시간
 * 
 *        mutex lock waiting time in milliseconds
 */
#define COFFEE_MTX_TIMEOUT_MS 100


/*
   IPC 설정

   IPC configurations
 */

/**
 * @def COFFEE_QUEUE_SIZE
 * 
 * @brief 메시지 전달을 위한 큐 크기
 */
#define COFFEE_QUEUE_SIZE 8

/**
 * @def COFFEE_MAX_STR_BUF
 * 
 * @brief 문자열 버퍼 최대 크기
 * 
 *        maximum size of the string buffer
 */
#define COFFEE_MAX_STR_BUF 256


/*
   네트워크 태스크 설정

   network task configurations
 */

/**
 * @def COFFEE_FILE_CHUNK_SIZE
 * 
 * @brief 파일 청크 크기
 * 
 *        file chunk size
 */
#define COFFEE_FILE_CHUNK_SIZE 0x19000

/**
 * @def COFFEE_NETWORK_TIMEOUT_MS
 * 
 * @brief 네트워크 최대 대기 시간
 * 
 *        maximum network timeout duration
 */
#define COFFEE_NETWORK_TIMEOUT_MS 10000


/*
   UI 태스크 설정

   UI task configurations
 */

/**
 * @def COFFEE_AD_TERM
 * 
 * @brief 광고 이미지 변경 주기(ms)
 * 
 *        interval for switching advertisement images in milliseconds
 */
#define COFFEE_AD_TERM 5000


#include <string>

#include <esp_heap_caps.h>

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

#include <cJSON.h>

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
