#ifndef COFFEE_CERT_HPP
#define COFFEE_CERT_HPP

#include <string>

#include "coffee/config.hpp"
#include "coffee/json_task.hpp"

namespace coffee {
    /**
     * @brief SD 카드로부터 인증서(PEM) 정보를 읽어옵니다
     * 
     * @param pem_path 읽어올 인증서 파일의 SD 카드 내 위치
     * 
     * @param[out] out_str 파일에서 읽은 문자열
     * 
     * @return 읽기 성공 여부
     */
    bool read_cert_pem(const std::string& pem_path, std::string& out_str);

    /**
     * @brief 인증서의 유효기간이 30일 이상 남았는지 확인합니다
     * 
     * @param cli_crt 확인할 클라이언트 인증서(PEM)의 문자열
     * 
     * @return 유효기간이 30일 이상 남았는지 여부
     */
    bool check_cert_expired(std::string& cli_crt);

    /**
     * @brief 새로운 CSR을 생성합니다
     * 
     * @details CSR은 타원곡선 암호의 일종인 EC P-256을 통해 생성합니다.
     * 
     *          생성된 CSR에는 서버에서 인증서에 서명할 공개키가 포함되어 있으며, 개인키는 CSR 생성 과정에 함께 생성되어 기기의 SD 카드에 저장합니다.
     * 
     * @param[out] out_csr 생성된 CSR
     * 
     * @return CSR 생성 성공 여부
     */
    bool generate_csr(std::string& out_csr);

    extern std::string ca_crt, cli_crt, cli_key;
}
#endif
