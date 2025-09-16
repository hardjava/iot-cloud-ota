package com.coffee_is_essential.iot_cloud_ota.dto;

import com.coffee_is_essential.iot_cloud_ota.domain.Ads;
import com.coffee_is_essential.iot_cloud_ota.domain.DeviceDeploymentStatus;
import com.coffee_is_essential.iot_cloud_ota.domain.ProgressCount;
import com.coffee_is_essential.iot_cloud_ota.domain.Target;
import com.coffee_is_essential.iot_cloud_ota.entity.FirmwareDeployment;
import com.coffee_is_essential.iot_cloud_ota.entity.OverallDeploymentStatus;
import com.coffee_is_essential.iot_cloud_ota.enums.DeploymentType;
import com.coffee_is_essential.iot_cloud_ota.enums.OverallStatus;

import java.time.OffsetDateTime;
import java.util.List;

public record DetailAdsDeploymentDto(
        Long id,
        String commandId,
        List<Ads> ads,
        DeploymentType deploymentType,
        List<Target> targetInfo,
        Long totalCount,
        Long successCount,
        Long inProgressCount,
        Long failedCount,
        OverallStatus status,
        OffsetDateTime deployedAt,
        OffsetDateTime expiresAt,
        List<DeviceDeploymentStatus> devices
) {
    public static DetailAdsDeploymentDto of(FirmwareDeployment deployment, List<Target> targetInfo, List<DeviceDeploymentStatus> devices, ProgressCount progressCount, OverallDeploymentStatus status, List<Ads> ads) {
        return new DetailAdsDeploymentDto(
                deployment.getId(),
                deployment.getCommandId(),
                ads,
                deployment.getDeploymentType(),
                targetInfo,
                progressCount.getTotalCount(),
                progressCount.getSuccessCount(),
                progressCount.getInProgressCount(),
                progressCount.getFailedCount(),
                status.getOverallStatus(),
                deployment.getDeployedAt(),
                deployment.getExpiresAt(),
                devices
        );
    }
}
