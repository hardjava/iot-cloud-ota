#ifndef COFFEE_IPC_HPP
#define COFFEE_IPC_HPP

#include <cstdarg>
#include <cstdio>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <Arduino.h>

#include "coffee/config.hpp"

namespace coffee {
    /**
     * @brief IPC를 위한 큐를 초기화합니다
     * 
     *        initializes the queue for IPC
     * 
     * @return 초기화 성공 여부
     * 
     *         whether initialization succeeded
     */
    bool init_ipc_queue(void);

    /**
     * @brief 큐를 이용하여 메시지를 송신합니다
     * 
     *        sends a message using the queue
     * 
     * @param serial_print 시리얼 모니터 동시 출력 여부
     * 
     *                     whether to also print to the serial monitor
     * 
     * @param fmt 출력 포맷
     * 
     *            output format string
     * 
     * @param ... 출력 포맷 값
     * 
     *            values corresponding to the format string
     */
    void queue_printf(QueueHandle_t& queue, std::string tag, bool serial_print, const char* fmt, ...);

    /**
     * @brief 큐를 이용하여 메시지를 수신합니다
     * 
     *        receives a message using the queue
     * 
     * @param queue 메시지를 수신할 큐
     * 
     *              the queue from which the message will be received
     * 
     * @param[out] msg_out 수신 메시지 버퍼
     * 
     *                  the destination buffer for the received message
     * 
     * @return 메시지 수신 여부
     * 
     *         whether the message was successfully received
     */
    bool queue_poll(QueueHandle_t& queue, char* msg_out);

    extern QueueHandle_t wifiTextArea_q;

    extern QueueHandle_t debugTextArea_q;

    extern QueueHandle_t dbg_overlay_q;
}
#endif
