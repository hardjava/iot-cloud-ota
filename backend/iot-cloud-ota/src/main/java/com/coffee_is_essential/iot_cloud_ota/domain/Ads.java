package com.coffee_is_essential.iot_cloud_ota.domain;

import com.coffee_is_essential.iot_cloud_ota.entity.AdsDeployment;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;

import java.time.OffsetDateTime;

@Getter
@AllArgsConstructor
@NoArgsConstructor
public class Ads {
    private Long id;
    private String title;
    private OffsetDateTime createdAt;
    private OffsetDateTime modifiedAt;

    public static Ads from(AdsDeployment deployment) {
        return new Ads(
                deployment.getAdsMetadata().getId(),
                deployment.getAdsMetadata().getTitle(),
                deployment.getAdsMetadata().getCreatedAt(),
                deployment.getAdsMetadata().getModifiedAt()
        );
    }
}
