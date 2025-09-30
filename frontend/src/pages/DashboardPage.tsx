import { MainTile } from "../widgets/layout/ui/MainTile";
import { TitleTile } from "../widgets/layout/ui/TitleTile";
import DistributionGlobe from "../widgets/distributionGlobe";
import { useQuery } from "@tanstack/react-query";
import { RegionApiService } from "../entities/region/api/regionApi";
import { GroupApiService } from "../entities/group/api/api";
import { useFirmwareDeploymentSearch } from "../entities/firmware_deployment/api/useFirmwareDeploymentSearch";
import { useAdvertisementDeploymentSearch } from "../entities/advertisement_deployment/api/useAdvertisementDeploymentSearch";
import { GroupList, useGroupWithLocations } from "../widgets/GroupList";
import { MetricCard } from "../widgets/MetricCard";
import { DeploymentChart } from "../widgets/DeploymentChart";
import { deviceApiService } from "../entities/device/api/api";

export const DashboardPage = () => {
  const { data: devices } = useQuery({
    queryKey: ["devices"],
    queryFn: deviceApiService.getDevices,
  });
  const { data: regions } = useQuery({
    queryKey: ["regions"],
    queryFn: RegionApiService.getRegions,
  });
  const { data: groups } = useQuery({
    queryKey: ["groups"],
    queryFn: GroupApiService.getGroups,
  });

  const { firmwareDeployments } = useFirmwareDeploymentSearch(1000);
  const { advertisementDeployments } = useAdvertisementDeploymentSearch(1000);
  const { groupsWithLocations } = useGroupWithLocations();

  const totalDevices = devices?.length ?? 0;
  const totalRegions = regions?.length ?? 0;
  const totalGroups = groups?.length ?? 0;

  const completedFirmwareDeployments = firmwareDeployments.filter(
    (d) => d.status === "COMPLETED",
  ).length;
  const completedAdDeployments = advertisementDeployments.filter(
    (d) => d.status === "COMPLETED",
  ).length;
  const totalCompletedDeployments =
    completedFirmwareDeployments + completedAdDeployments;

  const locations = groupsWithLocations?.map((group, _) => ({
    device: group.groupName,
    latitude: group.latitude,
    longitude: group.longitude,
    // color: `hsl(${index * 40}, 70%, 50%)`,
  }));

  return (
    <div className="flex flex-col gap-8">
      <TitleTile title="대시보드" description="디바이스 정보 및 최근 활동" />

      <div className="flex flex-col p-8 bg-white rounded-md">
        <div className="flex flex-col gap-4">
          <div className="flex gap-4 h-28 w-full">
            <MetricCard title="총 리전" value={totalRegions} />
            <MetricCard title="총 그룹" value={totalGroups} />
            <MetricCard title="총 기기" value={totalDevices} />
            <MetricCard title="완료된 배포" value={totalCompletedDeployments} />
          </div>

          <div className="flex gap-2 justify-between">
            {locations && (
              <div className="p-4 bg-white rounded-lg shadow-md">
                <DistributionGlobe
                  width={window.innerWidth * 0.6}
                  height={window.innerHeight * 0.6}
                  locations={locations}
                />
              </div>
            )}
            <GroupList />
          </div>

          <div className="flex flex-col gap-4">
            <DeploymentChart />
            <DeploymentChart />
          </div>
        </div>
      </div>
    </div>
  );
};

