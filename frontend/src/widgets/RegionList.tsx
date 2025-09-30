import { useQuery } from "@tanstack/react-query";
import { RegionApiService } from "../entities/region/api/regionApi";
import { Region } from "../entities/region/model/types";

// Mock data for region locations
const regionLocations: Record<string, { latitude: number; longitude: number }> =
  {
    Seoul: { latitude: 37.5665, longitude: 126.978 },
    Busan: { latitude: 35.1796, longitude: 129.0756 },
    Incheon: { latitude: 37.4563, longitude: 126.7052 },
    Daegu: { latitude: 35.8714, longitude: 128.6014 },
    Daejeon: { latitude: 36.3504, longitude: 127.3845 },
    Gwangju: { latitude: 35.1595, longitude: 126.8526 },
  };

export const useRegionWithLocations = () => {
  const { data: regions, isLoading } = useQuery<Region[]>({
    queryKey: ["regions"],
    queryFn: RegionApiService.getRegions,
  });

  const regionsWithLocations = regions?.map((region) => ({
    ...region,
    ...(regionLocations[region.regionName] || {
      latitude: Math.random() * 180 - 90,
      longitude: Math.random() * 360 - 180,
    }),
  }));

  return { regionsWithLocations, isLoading };
};

export const RegionList = () => {
  const { regionsWithLocations, isLoading } = useRegionWithLocations();

  if (isLoading) return <div>Loading regions...</div>;

  return (
    <div className="p-6 bg-white rounded-lg shadow-md overflow-y-auto">
      <h3 className="mb-4 text-lg font-semibold text-gray-800">리전 목록</h3>
      <ul className="space-y-3">
        {regionsWithLocations?.map((region, index) => (
          <li
            key={region.regionId}
            className="flex items-center text-sm text-gray-700"
          >
            <span
              className="w-3 h-3 mr-3 rounded-full"
              style={{ backgroundColor: `hsl(${index * 60}, 70%, 50%)` }}
            ></span>
            {region.regionName}
          </li>
        ))}
      </ul>
    </div>
  );
};
