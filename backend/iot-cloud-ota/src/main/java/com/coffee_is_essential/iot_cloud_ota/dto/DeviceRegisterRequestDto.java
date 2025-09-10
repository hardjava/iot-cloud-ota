package com.coffee_is_essential.iot_cloud_ota.dto;

import jakarta.validation.constraints.NotNull;

public record DeviceRegisterRequestDto(
        @NotNull(message = "region id는 필수 값입니다.")
        Long regionId,
        @NotNull(message = "group id는 필수 값입니다.")
        Long groupId
) {
}
