#include "coffee/ipc.hpp"

namespace coffee {
	/**
	 * @brief  debugTextArea에 표시될 텍스트를 전달하는 큐
	 * 
	 * 	       queue for delivering text to be displayed in debugTextArea
	 */
	QueueHandle_t debugTextArea_q;
	
	/**
	 * @brief  serverTextArea에 표시될 텍스트를 전달하는 큐
	 * 
	 * 	       queue for delivering text to be displayed in serverTextArea
	 */
	QueueHandle_t serverTextArea_q;
	
	/**
	 * @brief  wifiTextArea에 표시될 텍스트를 전달하는 큐
	 * 
	 * 	       queue for delivering text to be displayed in wifiTextArea
	 */
	QueueHandle_t wifiTextArea_q;

	/**
	 * @brief 디버그 오버레이에 표시될 텍스트를 전달하는 큐
	 * 
	 *        queue for delivering text to be displayed in the debug overlay
	 */
	QueueHandle_t dbg_overlay_q;

	bool init_ipc_queue(void) {
        if (!debugTextArea_q) {
            debugTextArea_q = xQueueCreate(COFFEE_QUEUE_SIZE, COFFEE_MAX_STR_BUF);
        }
		
        if (!serverTextArea_q) {
            serverTextArea_q = xQueueCreate(COFFEE_QUEUE_SIZE, COFFEE_MAX_STR_BUF);
        }
		
        if (!wifiTextArea_q) {
            wifiTextArea_q = xQueueCreate(COFFEE_QUEUE_SIZE, COFFEE_MAX_STR_BUF);
        }

        if (!dbg_overlay_q) {
            dbg_overlay_q = xQueueCreate(COFFEE_QUEUE_SIZE * 10, COFFEE_MAX_STR_BUF);
        }

		if (!debugTextArea_q || !serverTextArea_q || !wifiTextArea_q || !dbg_overlay_q) {
			Serial.println("[coffee/ipc][error] queue creation failed");

			return false;
		}

		Serial.println("[coffee/ipc][info] ipc queue initialization success!");

		return true;
	}

    void queue_printf(QueueHandle_t& queue, std::string tag, bool serial_print, const char* fmt, ...) {
        char line[COFFEE_MAX_STR_BUF] = { 0 };
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(line, sizeof(line), fmt, ap);
        line[COFFEE_MAX_STR_BUF - 1] = '\0';
        va_end(ap);

		std::string res;
		if (tag != "") {
			res = std::string("[") + tag + "]" + line;
		} else {
			res = std::string(line);
		}

		if (serial_print) {
        	Serial.print(res.c_str());
		}
		
        if (!queue) {
            return;
        } else if (queue == dbg_overlay_q && res[res.length() - 1] == '\n') {
			res[res.length() - 1] = '\0';
		}

        xQueueSend(queue, res.c_str(), portMAX_DELAY);
    }

	bool queue_poll(QueueHandle_t& queue, char* msg_out) {
        if (!queue) {
            return false;
        }

		return (xQueueReceive(queue, msg_out, 0) == pdPASS);
	}
}
