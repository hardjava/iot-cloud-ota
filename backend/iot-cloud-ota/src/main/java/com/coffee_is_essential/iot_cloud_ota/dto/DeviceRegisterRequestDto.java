package com.coffee_is_essential.iot_cloud_ota.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public record DeviceRegisterRequestDto(
        @NotNull(message = "device id는 필수 값입니다.")
        @JsonProperty("device_id")
        Long deviceId,

        @NotBlank(message = "device name은 필수 값입니다.")
        @JsonProperty("device_name")
        String deviceName,

        @NotBlank(message = "AuthKey는 필수 값입니다.")
        @JsonProperty("auth_key")
        String AuthKey,

        @NotBlank(message = "timestamp는 필수 값입니다.")
        String timestamp
) {
}
