import { keepPreviousData, useQuery } from "@tanstack/react-query";
import { useState } from "react";
import { fetchAdvertisementDeploymentList } from "./api";

/**
 * 광고 배포 리스트를 가져오는 커스텀 훅
 * @returns 광고 배포 리스트, 로딩 상태, 에러 메시지, 페이지 변경 함수
 */
export const useAdvertisementDeploymentSearch = (limit: number = 10) => {
  const [page, setPage] = useState(1);

  const { data, isLoading, error, isPlaceholderData } = useQuery({
    queryKey: ["advertisement-deployments", page, limit],
    queryFn: () => fetchAdvertisementDeploymentList(page, limit),
    placeholderData: keepPreviousData,
  });

  const handlePageChange = (newPage: number) => {
    setPage(newPage);
  };

  return {
    advertisementDeployments: data?.items ?? [],
    pagination: data?.paginationMeta,
    isLoading: isLoading || isPlaceholderData,
    error: error ? "광고 배포 목록을 불러오는 중 오류가 발생했습니다." : null,
    handlePageChange,
  };
};
