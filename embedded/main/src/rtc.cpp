#include "coffee/rtc.hpp"

namespace coffee {
    void init_rtc(void) {
        static bool is_init = false;
        if (is_init) {
            return;
        }

        setenv("TZ", "UTC0", 1);
        tzset();

        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        is_init = true;
    }

    bool wait_time_sync(int max_wait_sec) {
        for (int i = 0; i < max_wait_sec; i++) {
            if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
                return true;
            }

            delay(1000);
        }

        return false;
    }

    bool utc_now(std::string& buf) {
        std::time_t now = std::time(nullptr);
        if (now < 1700000000) {
            // 시간 미동기화
            return false;
        }

        struct tm tm_utc;
        gmtime_r(&now, &tm_utc);
        
        char out[21];
        size_t n = strftime(out, 21, "%Y-%m-%dT%H:%M:%SZ", &tm_utc);

        if (n == 20) {
            buf = std::string(out);

            return true;
        } else {
            buf = "";

            return false;
        }
    }
}
