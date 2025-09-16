package com.coffee_is_essential.iot_cloud_ota.repository;

import com.coffee_is_essential.iot_cloud_ota.entity.AdsDeployment;
import com.coffee_is_essential.iot_cloud_ota.entity.FirmwareDeployment;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface AdsDeploymentJpaRepository extends JpaRepository<AdsDeployment, Long> {
    List<AdsDeployment> findByFirmwareDeployment(FirmwareDeployment firmwareDeployment);
}
