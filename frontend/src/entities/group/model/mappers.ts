import { Group, GroupDto } from "./types";

/**
 * Maps a GroupDto object to a Group object.
 * @param {GroupDto} group - The data transfer object from the API
 * @return {Group} The transformed group domain model
 */
export const mapGroupDto = (group: GroupDto): Group => {
  return {
    id: group.id,
    name: group.name,
    deviceCount: group.device_count,
    createdAt: new Date(group.created_at),
  };
};
