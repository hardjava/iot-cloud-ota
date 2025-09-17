#include <coffee/cert.hpp>

#include <ctime>

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/x509.h>
#include <mbedtls/error.h>

#include <Arduino.h>
#include <SD.h>
#include <FS.h>

#include "coffee/event_control.hpp"
#include "coffee/ipc.hpp"

namespace coffee {
    // 로깅용 태그
    static const std::string TAG = "coffee/cert";

    /**
     * @brief SD 카드로부터 읽은 인증서(PEM) 문자열
     */
    std::string ca_crt, cli_crt, cli_key;

    bool read_cert_pem(const std::string& pem_path, std::string& out_str) {
        lock_mtx(cert_mtx, portMAX_DELAY);

        File file = SD.open(pem_path.c_str(), FILE_READ);
        if (!file) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to read the certification file");

            unlock_mtx(cert_mtx);
            return false;
        }

        out_str.clear();
        out_str.reserve(file.size() + 1);
        
        while (file.available()) {
            out_str.push_back(static_cast<char>(file.read()));
        }

        file.close();

        if (out_str.empty() || out_str.back() != '\0') {
            out_str.push_back('\0');
        }

        unlock_mtx(cert_mtx);
        return true;
    }

    bool check_cert_expired(std::string& cli_crt) {
        mbedtls_x509_crt crt;
        mbedtls_x509_crt_init(&crt);

        int ret = mbedtls_x509_crt_parse(&crt, reinterpret_cast<const unsigned char*>(cli_crt.c_str()), cli_crt.size() + 1);
        if (ret < 0) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] falied to parse x509: -0x%04x\n", -ret);
            
            mbedtls_x509_crt_free(&crt);

            return false;
        }

        time_t now;
        time(&now);

        struct tm valid_to_tm;
        valid_to_tm.tm_year = crt.valid_to.year - 1900;
        valid_to_tm.tm_mon  = crt.valid_to.mon  - 1;
        valid_to_tm.tm_mday = crt.valid_to.day;
        valid_to_tm.tm_hour = crt.valid_to.hour;
        valid_to_tm.tm_min  = crt.valid_to.min;
        valid_to_tm.tm_sec  = crt.valid_to.sec;

        time_t expire_time = mktime(&valid_to_tm);

        double diff_days = difftime(expire_time, now) / 86400.0;

        mbedtls_x509_crt_free(&crt);

        queue_printf(dbg_overlay_q, TAG, true, "[info] cert expires in %.1f days\n", diff_days);

        return (diff_days <= 30.0);
    }

    bool generate_csr(std::string& out_csr) {
        static std::string csr_subj = std::string("CN=") + serial_number->valuestring;

        int ret = 0;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_pk_context pk;
        mbedtls_x509write_csr req;

        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_pk_init(&pk);
        mbedtls_x509write_csr_init(&req);

        const char* pers = "csr_gen";
        if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
            reinterpret_cast<const unsigned char*>(pers), strlen(pers))) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }

        // 1) 키 컨테이너를 EC 키로 준비
        if ((ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }

        // 2) EC P-256 키 생성
        if ((ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk),
            mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }

        // 3) CSR 설정: 주체명/키/해시 알고리즘
        mbedtls_x509write_csr_set_key(&req, &pk);
        if ((ret = mbedtls_x509write_csr_set_subject_name(&req, csr_subj.c_str())) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }
        
        mbedtls_x509write_csr_set_md_alg(&req, MBEDTLS_MD_SHA256);

        // 4) PEM 출력 버퍼
        std::string key_pem, csr_pem;
        key_pem.resize(4096);
        csr_pem.resize(4096);

        // 5) 개인키 PEM 작성 (평문 PEM)
        if ((ret = mbedtls_pk_write_key_pem(&pk, reinterpret_cast<unsigned char*>(&key_pem[0]), key_pem.size())) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }
        key_pem.resize(strlen(key_pem.c_str()));

        // 6) CSR PEM 작성
        if ((ret = mbedtls_x509write_csr_pem(&req, reinterpret_cast<unsigned char*>(&csr_pem[0]), 
            csr_pem.size(), mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
            mbedtls_x509write_csr_free(&req);
            mbedtls_pk_free(&pk);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return false;
        }
        csr_pem.resize(strlen(csr_pem.c_str()));

        // 정리
        mbedtls_x509write_csr_free(&req);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);

        out_csr = csr_pem;

        if (SD.exists("/cert/temp.key")) {
            SD.remove("/cert/temp.key");
        }

        File temp_key = SD.open("/cert/temp.key", FILE_WRITE);
        if (!temp_key) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to create a temporary key file\n");

            out_csr = "";

            return false;
        }

        temp_key.print(key_pem.c_str());

        temp_key.flush();
        temp_key.close();

        if (SD.exists("/cert/client.key")) {
            SD.remove("/cert/client.key");
        }

        if (!SD.rename("/cert/temp.key", "/cert/client.key")) {
            queue_printf(dbg_overlay_q, TAG, true, "[error] failed to rename temporary key file\n");
            queue_printf(dbg_overlay_q, TAG, true, "[error]     temp file path: /cert/temp.key\n");

            out_csr = "";

            return false;
        }

        return true;
    }
}
