#ifndef COFFEE_RTC_HPP
#define COFFEE_RTC_HPP

#include <ctime>
#include <string>

#include <sys/time.h>
#include <esp_sntp.h>

#include <Arduino.h>

namespace coffee {
    /**
     * @brief SNTP 서버를 통해 RTC를 초기화합니다
     */
    void init_rtc(void);

    /**
     * @brief 시간이 동기화될 때까지 대기합니다
     * 
     * @param max_wait_sec 시간이 동기화될 때까지 기다리는 최대 시간(초)
     */
    bool wait_time_sync(int max_wait_sec);

    /**
     * @brief 현재 시간을 UTC 기준의 ISO8601 형식으로 출력합니다
     * 
     * @param[out] buf 현재 시간 문자열을 출력할 버퍼
     */
    bool utc_now(std::string& buf);
}
#endif
