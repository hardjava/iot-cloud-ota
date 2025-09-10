package com.coffee_is_essential.iot_cloud_ota.dto;

import com.coffee_is_essential.iot_cloud_ota.domain.DeployTargetDeviceInfo;
import com.coffee_is_essential.iot_cloud_ota.domain.DeploymentContent;
import com.fasterxml.jackson.annotation.JsonProperty;

import java.time.OffsetDateTime;
import java.util.List;

/**
 * AdsDeploymentDto는 ADS(Automated Deployment System) 배포 요청에 대한 데이터를 담는 DTO입니다.
 * 이 DTO는 배포 명령 ID, 배포할 콘텐츠 목록, 대상 디바이스 정보, 총 콘텐츠 크기, 타임스탬프를 포함합니다.
 *
 * @param commandId 배포 명령의 고유 ID
 * @param contents  배포할 콘텐츠의 리스트
 * @param devices   배포 대상 디바이스의 정보 리스트
 * @param totalSize 배포할 콘텐츠의 총 크기 (바이트 단위)
 * @param timestamp 배포 요청이 생성된 시각
 */
public record AdsDeploymentDto(
        @JsonProperty("command_id")
        String commandId,
        List<DeploymentContent> contents,
        List<DeployTargetDeviceInfo> devices,
        long totalSize,
        OffsetDateTime timestamp
) {
}
