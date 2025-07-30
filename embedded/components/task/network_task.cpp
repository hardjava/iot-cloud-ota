#include "network_task.hpp"

namespace coffee {
    /**
     * @brief 화면에 표시할 텍스트를 출력 큐에 추가합니다
     * 
     *        adds the text to be displayed on the screen to the output queue
     */
    static inline void queue_text(const char* send_text);

    /**
     * @brief 서버로부터 메시지를 받아 출력합니다
     * 
     *        receives messages from the server and prints them
     */
    static void receive_text(void* task_param);
    
    /**
     * @brief TCP 소켓 통신 클라이언트
     * 
     *        TCP socket communication client
     */
    WiFiClient client;

    /**
     * @brief client 동시 접근 제어를 위한 뮤텍스
     * 
     *        mutex for synchronizing concurrent access to client
     */
    SemaphoreHandle_t client_mtx;

    /**
     * @brief chatTextArea의 동시 접근 제어를 위한 큐
     * 
     *        queue for synchronizing concurrent access to chatTextArea
     */
    QueueHandle_t chatTextArea_q;

    const static char* HOST = "121.175.190.51";
    const static uint16_t PORT = 24072;
    
    void connect_wifi(const char* ssid, const char* pw) {
        if (WiFi.status() == WL_CONNECTED)
            queue_text("Already connected to Wi-Fi...\n");
        else {
            queue_text("Connecting Wi-Fi...\n");
            if (coffee::init_wifi_sta(ssid, pw))
                queue_text("Successfully connected to Wi-Fi!\n");
            else {
                queue_text("Failed to connect Wi-Fi... Trying again...\n");
                if (coffee::init_wifi_sta(ssid, pw))
                    queue_text("Successfully connected to Wi-Fi!\n");
                else {
                    queue_text("Failed to connect Wi-Fi...\n");

                    return;
                }
            }
        }

        if (xSemaphoreTake(client_mtx, portMAX_DELAY) == pdTRUE) {
            if (client.connected())
                queue_text("Already connected to server...\n");
            else {
                queue_text("Connecting server...\n");
                if (client.connect(HOST, PORT, COFFEE_WIFI_TIMEOUT_MS)) {
                    queue_text("Successfully connected to server!\n");

                    xTaskCreatePinnedToCore(receive_text, "receive_text", 4096, NULL, 3, NULL, 0);
                } else {
                    queue_text("Failed to connect server... Trying again...\n");
                    if (client.connect(HOST, PORT, COFFEE_WIFI_TIMEOUT_MS)) {
                        queue_text("Successfully connected to server!\n");

                        xTaskCreatePinnedToCore(receive_text, "receive_text", 4096, NULL, 3, NULL, 0);
                    } else
                        queue_text("Failed to connect server...\n");
                }
            }

            xSemaphoreGive(client_mtx);
        }
    }

    void send_ok(void) {
        if (xSemaphoreTake(client_mtx, portMAX_DELAY) == pdTRUE) {
            if (!(WiFi.status() == WL_CONNECTED && client.connected())) {
                queue_text("Failed to connect server...\n");

                xSemaphoreGive(client_mtx);

                return;
            }

            client.println("OK");

            queue_text("Client: OK\n");

            xSemaphoreGive(client_mtx);
        }
    }

    static inline void queue_text(const char* send_text) {
        char buf[COFFEE_MAX_STR_LEN];
        strncpy(buf, send_text, COFFEE_MAX_STR_LEN);
        buf[COFFEE_MAX_STR_LEN - 1] = '\0';

        xQueueSend(chatTextArea_q, buf, portMAX_DELAY);
    }

    static void receive_text(void* task_param) {
        while (true) {
            if (xSemaphoreTake(client_mtx, portMAX_DELAY) == pdTRUE) {
                if (!(WiFi.status() == WL_CONNECTED && client.connected())) {
                    queue_text("Failed to connect server...\n");

                    xSemaphoreGive(client_mtx);

                    break;
                }

                while (client.available()) {
                    String recv_txt = client.readStringUntil('\n');

                    char buf[COFFEE_MAX_STR_LEN];
                    snprintf(buf, sizeof(buf), "Server: %s\n", recv_txt.c_str());

                    queue_text(buf);
                }

                xSemaphoreGive(client_mtx);
            }

            delay(10);
        }

        vTaskDelete(NULL);
    }
}
