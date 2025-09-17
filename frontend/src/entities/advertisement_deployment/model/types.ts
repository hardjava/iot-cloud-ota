import { Device } from "../../device/model/types";
import { Group } from "../../group/model/types";
import { Region } from "../../region/model/types";
import { PaginationMeta } from "../../../shared/api/types";

/**
 * 배포 대상 타입
 */
export type DeploymentTarget = "DEVICE" | "GROUP" | "REGION";

/**
 * 광고 배포 정보 인터페이스
 */
export interface AdvertisementDeployment {
  id: number;
  deploymentType: DeploymentTarget;
  targetInfo: Device[] | Group[] | Region[];
  totalCount: number;
  successCount: number;
  inProgressCount: number;
  failedCount: number;
  status: "PENDING" | "IN_PROGRESS" | "COMPLETED";
  deployedAt: string;
}

/**
 * 광고 배포 상세 정보 인터페이스
 */
export interface AdvertisementDeploymentDetail extends AdvertisementDeployment {
  commandId: string;
  ads: {
    id: number;
    title: string;
    createdAt: string;
    modifiedAt: string;
  }[];
  expiresAt: string;
  devices: {
    id: number;
    status: "SUCCESS" | "FAIL" | "IN_PROGRESS";
    progress: number;
    timestamp: string;
  }[];
}

/**
 * 광고 배포 리스트 인터페이스
 */
export interface AdvertisementDeploymentList {
  items: AdvertisementDeployment[];
  paginationMeta: PaginationMeta;
}
