import { createBrowserRouter, Navigate } from "react-router";
import { Layout } from "../widgets/layout/ui/Layout";
import { DashboardPage } from "../pages/DashboardPage";
import { FirmwareListPage } from "../pages/FirmwareListPage";
import { AdListPage } from "../pages/AdListPage";
import { MonitoringPage } from "../pages/MonitoringPage";
import { FirmwareDetailPage } from "../pages/FirmwareDetailPage";
import { DevicePage } from "../pages/DevicePage";
import { FirmwareDeploymentPage } from "../pages/FirmwareDeploymentPage";
import { FirmwareDeploymentDetailPage } from "../pages/FirmwareDeploymentDetailPage";
import { AdDetailPage } from "../pages/AdDetailPage";
import { DeviceDetailPage } from "../pages/DeviceDetailPage";
import { AdvertisementDeploymentPage } from "../pages/AdvertisementDeploymentPage";
import { AdvertisementDeploymentDetailPage } from "../pages/AdvertisementDeploymentDetailPage";

export const Router = createBrowserRouter([
  {
    path: "/",
    element: <Layout />,
    children: [
      // 초기 엔트리 경로를 dashboard로 지정
      { index: true, element: <Navigate to="/dashboard" replace /> },
      { path: "dashboard", element: <DashboardPage /> },
      { path: "firmware", element: <Navigate to="/firmware/list" replace /> },
      { path: "firmware/list", element: <FirmwareListPage /> },
      { path: "firmware/:id", element: <FirmwareDetailPage /> },
      { path: "firmware/deployment", element: <FirmwareDeploymentPage /> },
      {
        path: "firmware/deployment/:id",
        element: <FirmwareDeploymentDetailPage />,
      },
      { path: "device", element: <DevicePage /> },
      { path: "device/:deviceId", element: <DeviceDetailPage /> },
      { path: "ads", element: <Navigate to="/ads/list" replace /> },
      { path: "ads/list", element: <AdListPage /> },
      { path: "ads/:id", element: <AdDetailPage /> },
      {
        path: "ads/deployment",
        element: <AdvertisementDeploymentPage />,
      },
      {
        path: "ads/deployment/:id",
        element: <AdvertisementDeploymentDetailPage />,
      },
      { path: "monitoring", element: <MonitoringPage /> },
    ],
  },
]);
