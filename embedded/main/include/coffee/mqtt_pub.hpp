#ifndef COFFEE_MQTT_PUB_HPP
#define COFFEE_MQTT_PUB_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <mqtt_client.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>

#include <cJSON.h>

#include "coffee/config.hpp"
#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"
#include "coffee/json_task.hpp"
#include "coffee/rtc.hpp"
#include "coffee/ui_task.hpp"

namespace coffee {
    /**
     * @brief 발행할 MQTT 메시지 정보
     */
    class PubMessage {
    public:
        std::string topic;
        cJSON* payload;
    };

    /**
     * @brief MQTT 메시지 대기열을 이용하여 순차적으로 브로커에 발행하는 FreeRTOS 태스크를 초기화합니다
     */
    bool init_mqtt_pub(void);

    /**
     * @brief update/request에 대한 ack를 발행합니다
     * 
     * @param command_id 수신한 update/request의 command_id
     */
    void pub_request_ack(const std::string& command_id);

    /**
     * @brief update/cancel에 대한 ack를 발행합니다
     * 
     * @param command_id 수신한 update/cancel의 command_id
     */
    void pub_cancel_ack(const std::string& command_id);

    /**
     * @brief 다운로드 진행 상황을 발행합니다
     * 
     * @param command_id 수신한 update/request의 command_id
     * 
     * @param dl_b 다운로드한 크기(B)
     * 
     * @param tt_b 다운로드할 파일의 전체 크기(B)
     * 
     * @param time 다운로드 시작 후 흐른 시간
     */
    void pub_update_progress(const std::string& command_id, std::size_t dl_b, std::size_t tt_b, int64_t time);

    /**
     * @brief update/request에 대한 최종 결과를 발행합니다
     * 
     * @param command_id 수신한 update/request의 command_id
     * 
     * @param stat 최종 결과(SUCCESS|ERROR|TIMEOUT|CANCELED)
     * 
     * @param msg 결과에 추가할 메시지
     * 
     * @param dl_ms 업데이트에 소요된 시간(ms)
     */
    void pub_update_result(const std::string& command_id, const std::string& stat, const std::string& msg, std::size_t dl_ms);

    /**
     * @brief 시스템 오류 발생 정보를 발행합니다
     * 
     * @param TAG 오류 발생 지점 태그
     * 
     * @param log 오류 로그
     */
    void pub_error_log(const std::string& TAG, const std::string& log);

    /**
     * @brief 매출 / 거래 관련 데이터를 발행합니다
     * 
     * @param type 매출 / 거래 데이터 타입
     * 
     * @param sub_type 매출 / 거래 데이터 세부 타입
     */
    void pub_sales_data(const std::string& type, const std::string& sub_type);

    /**
     * @brief 인증서 갱신을 위해 관련 정보를 발행합니다
     * 
     * @param csr 암호화된 만료가 가까워진 인증서 정보
     */
    void pub_cert_request(const std::string& csr);

    /**
     * @brief 기기 등록을 위해 관련 정보를 발행합니다
     * 
     * @param auth_key 인증 키
     */
    void pub_regist(const std::string& auth_key);

    /**
     * @brief 테스트 정보를 발행합니다
     * 
     * @param msg 테스트 메시지
     */
    void pub_test(const std::string& msg);

    extern std::string mqtt_prefix;

    extern esp_mqtt_client_handle_t mqtt_client;

    extern QueueHandle_t mqtt_pub_q;
}
#endif
