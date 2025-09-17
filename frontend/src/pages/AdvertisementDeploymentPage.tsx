import ReactPaginate from "react-paginate";
import { useAdvertisementDeploymentSearch } from "../entities/advertisement_deployment/api/useAdvertisementDeploymentSearch";
import { AdvertisementDeploymentList } from "../entities/advertisement_deployment/ui/AdvertisementDeploymentList";
import { MainTile } from "../widgets/layout/ui/MainTile";
import { TitleTile } from "../widgets/layout/ui/TitleTile";
import { ChevronLeft, ChevronRight } from "lucide-react";

export const AdvertisementDeploymentPage = () => {
  const {
    advertisementDeployments,
    pagination,
    isLoading,
    error,
    handlePageChange,
  } = useAdvertisementDeploymentSearch();

  const handlePageClick = (event: { selected: number }) => {
    const newPage = event.selected + 1;
    handlePageChange(newPage);
  };

  const pageCount = pagination?.totalPage ?? 0;
  const currentPage = pagination?.page ?? 1;

  return (
    <div className="flex flex-col">
      <div className="mb-8">
        <TitleTile
          title="광고 관리"
          description="광고 업로드 및 원격 업데이트"
        />
      </div>
      <div>
        <MainTile title="광고 배포 현황">
          <AdvertisementDeploymentList
            deployments={advertisementDeployments}
            isLoading={isLoading}
            error={error}
          />

          <div className="flex items-center justify-center mt-6 text-neutral-500">
            {pageCount > 0 && (
              <ReactPaginate
                previousLabel={<ChevronLeft size={18} />}
                nextLabel={<ChevronRight size={18} />}
                pageCount={pageCount}
                onPageChange={handlePageClick}
                forcePage={currentPage - 1}
                containerClassName="flex items-center space-x-3"
                pageClassName="hover:text-black"
                pageLinkClassName="text-sm px-2 py-1"
                previousClassName="flex items-center hover:text-black"
                nextClassName="flex items-center hover:text-black"
                activeClassName="text-black font-semibold"
                disabledClassName="opacity-50 cursor-not-allowed"
                breakLabel="..."
                breakClassName="px-2"
              />
            )}
          </div>
        </MainTile>
      </div>
    </div>
  );
};