#ifndef COFFEE_EVENT_CONTROL_HPP
#define COFFEE_EVENT_CONTROL_HPP

#include <cstddef>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <Arduino.h>

#include "coffee/config.hpp"

namespace coffee {
    /**
     * @brief 이벤트 제어에 필요한 뮤텍스들을 초기화합니다
     * 
     *        initializes the mutexes required for event control
     */
    void init_mtx(void);

    /**
     * @brief 뮤텍스를 잠급니다
     * 
     *        locks the given mutex
     * 
     * @param mtx 잠글 뮤텍스
     * 
     *            the mutex to be locked
     * 
     * @param wait_time 최대 잠금 대기 시간
     * 
     *                  maximum waiting time for the lock
     * 
     * @return 잠금 성공 여부
     * 
     *         whether the lock operation succeeded
     */
    bool lock_mtx(SemaphoreHandle_t& mtx, std::size_t wait_time = COFFEE_MTX_TIMEOUT_MS);

    /**
     * @brief 뮤텍스를 잠금 해제합니다
     * 
     *        unlocks the given mutex
     * 
     * @param mtx 잠금 해제할 뮤텍스
     * 
     *            the mutex to be unlocked
     */
    void unlock_mtx(SemaphoreHandle_t& mtx);

    extern SemaphoreHandle_t debug_mtx;

    extern SemaphoreHandle_t json_mtx;

    extern SemaphoreHandle_t mqtt_mtx;

    extern SemaphoreHandle_t network_mtx;

    extern SemaphoreHandle_t ota_mtx;
}
#endif
