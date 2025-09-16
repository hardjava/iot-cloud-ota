package com.coffee_is_essential.iot_cloud_ota.domain;

import com.coffee_is_essential.iot_cloud_ota.entity.FirmwareDeployment;
import com.coffee_is_essential.iot_cloud_ota.enums.DeploymentType;
import com.coffee_is_essential.iot_cloud_ota.enums.OverallStatus;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;

import java.time.OffsetDateTime;
import java.util.List;

@Getter
@AllArgsConstructor
@NoArgsConstructor
public class AdsDeploymentMetadata {
    private Long id;
    private DeploymentType deploymentType;
    private List<Target> targetInfo;
    private Long totalCount;
    private Long successCount;
    private Long inProgressCount;
    private Long failedCount;
    private OverallStatus status;
    private OffsetDateTime deployedAt;

    public static AdsDeploymentMetadata of(FirmwareDeployment deployment, List<Target> targetInfo, ProgressCount progressCount, OverallStatus overallStatus) {

        return new AdsDeploymentMetadata(
                deployment.getId(),
                deployment.getDeploymentType(),
                targetInfo,
                progressCount.getTotalCount(),
                progressCount.getSuccessCount(),
                progressCount.getInProgressCount(),
                progressCount.getFailedCount(),
                overallStatus,
                deployment.getDeployedAt()
        );
    }
}
