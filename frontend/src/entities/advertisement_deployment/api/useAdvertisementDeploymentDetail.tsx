import { useQuery } from "@tanstack/react-query";
import { fetchAdvertisementDeploymentDetail } from "./api";

/**
 * 광고 배포 상세 정보를 가져오는 커스텀 훅
 * @param id 광고 배포 ID
 * @returns 광고 배포 상세 정보, 로딩 상태, 에러 메시지
 */
export const useAdvertisementDeploymentDetail = (id: number | null) => {
  const { data, isLoading, error } = useQuery({
    queryKey: ["advertisement-deployment-detail", id],
    queryFn: () => {
      if (!id) {
        return Promise.reject(new Error("ID is required"));
      }
      return fetchAdvertisementDeploymentDetail(id);
    },
    enabled: !!id, // Only run the query if the id is not null
  });

  return {
    deployment: data,
    isLoading,
    error: error
      ? "광고 배포 상세 정보를 불러오는 중 오류가 발생했습니다."
      : null,
  };
};

