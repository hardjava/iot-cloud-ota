import { useQuery } from "@tanstack/react-query";
import { GroupApiService } from "../entities/group/api/api";
import { Group } from "../entities/group/model/types";

// Mock data for group locations
const groupLocations: Record<string, { latitude: number; longitude: number }> =
  {
    "Group A": { latitude: 37.7749, longitude: -122.4194 },
    "Group B": { latitude: 34.0522, longitude: -118.2437 },
    "Group C": { latitude: 40.7128, longitude: -74.006 },
    "Group D": { latitude: 51.5074, longitude: -0.1278 },
    "Group E": { latitude: 35.6895, longitude: 139.6917 },
  };

export const useGroupWithLocations = () => {
  const { data: groups, isLoading } = useQuery<Group[]>({
    queryKey: ["groups"],
    queryFn: GroupApiService.getGroups,
  });

  const groupsWithLocations = groups?.map((group) => ({
    ...group,
    ...(groupLocations[group.groupName] || {
      latitude: Math.random() * 180 - 90,
      longitude: Math.random() * 360 - 180,
    }),
  }));

  return { groupsWithLocations, isLoading };
};

export const GroupList = () => {
  const { groupsWithLocations, isLoading } = useGroupWithLocations();

  if (isLoading) return <div>Loading groups...</div>;

  return (
    <div className="p-6 bg-white rounded-lg shadow-md overflow-y-auto">
      <h3 className="mb-4 text-lg font-semibold text-gray-800">그룹 목록</h3>
      <ul className="space-y-3">
        {groupsWithLocations?.map((group, index) => (
          <li
            key={group.groupId}
            className="flex items-center text-sm text-gray-700"
          >
            <span
              className="w-3 h-3 mr-3 rounded-full"
              style={{ backgroundColor: `hsl(${index * 40}, 70%, 50%)` }}
            ></span>
            {group.groupName}
          </li>
        ))}
      </ul>
    </div>
  );
};
