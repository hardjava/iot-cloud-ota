package com.coffee_is_essential.iot_cloud_ota.dto;

public record DeviceRegisterResponseDto(
        String code,
        String expiresAt
) {
}
