package com.coffee_is_essential.iot_cloud_ota.dto;

import java.time.OffsetDateTime;

public record DeviceRegisterResponseDto(
        String status,
        OffsetDateTime timestamp
) {
}
