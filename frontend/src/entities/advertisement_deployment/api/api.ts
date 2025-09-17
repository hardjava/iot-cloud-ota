import { apiClient } from "../../../shared/api/client";
import {
  AdvertisementDeploymentList,
  AdvertisementDeploymentDetail,
} from "../model/types";

/**
 * 광고 배포 리스트를 가져옵니다.
 * @param page 페이지 번호
 * @param limit 페이지당 항목 수
 * @returns 광고 배포 리스트
 */
export const fetchAdvertisementDeploymentList = async (
  page: number,
  limit: number,
): Promise<AdvertisementDeploymentList> => {
  const response = await apiClient.get("/api/ads/deployment/list", {
    params: { page, limit },
  });
  return response.data;
};

/**
 * 특정 광고 배포의 상세 정보를 가져옵니다.
 * @param id 광고 배포 ID
 * @returns 광고 배포 상세 정보
 */
export const fetchAdvertisementDeploymentDetail = async (
  id: number,
): Promise<AdvertisementDeploymentDetail> => {
  const response = await apiClient.get(`/api/ads/deployment/${id}`);
  return response.data;
};
