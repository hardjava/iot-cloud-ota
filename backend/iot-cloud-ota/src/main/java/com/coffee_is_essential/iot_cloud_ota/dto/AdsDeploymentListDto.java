package com.coffee_is_essential.iot_cloud_ota.dto;

import com.coffee_is_essential.iot_cloud_ota.domain.AdsDeploymentMetadata;

import org.springframework.data.domain.Pageable;

import java.util.List;

public record AdsDeploymentListDto(
        List<AdsDeploymentMetadata> items,
        PaginationMetadataDto paginationMeta
) {
    public static AdsDeploymentListDto of(List<AdsDeploymentMetadata> items, Pageable pageable, int totalPages, long totalElements) {
        PaginationMetadataDto metadataDto = new PaginationMetadataDto(
                pageable.getPageNumber() + 1,
                pageable.getPageSize(),
                totalPages,
                totalElements
        );

        return new AdsDeploymentListDto(items, metadataDto);
    }
}
