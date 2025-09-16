package com.coffee_is_essential.iot_cloud_ota.dto;

public record GenerateRegistrationCodeResponseDto(
        String code,
        String expiresAt
) {
}
