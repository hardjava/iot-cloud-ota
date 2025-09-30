import { useQuery } from "@tanstack/react-query";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import { firmwareDeploymentApiService } from "../entities/firmware_deployment/api/api";
import { subDays, format, eachDayOfInterval, parseISO } from "date-fns";
import { fetchAdvertisementDeploymentList } from "../entities/advertisement_deployment/api/api";

// Helper function to generate last 30 days
const getLast30Days = () => {
  const today = new Date();
  const thirtyDaysAgo = subDays(today, 29);
  return eachDayOfInterval({ start: thirtyDaysAgo, end: today });
};

export const DeploymentChart = () => {
  const { data: firmwareDeployments } = useQuery({
    queryKey: ["firmware-deployments-all"],
    queryFn: () => firmwareDeploymentApiService.getFirmwareDeployments(1, 1000),
  });

  const { data: adDeployments } = useQuery({
    queryKey: ["advertisement-deployments-all"],
    queryFn: () => fetchAdvertisementDeploymentList(1, 1000),
  });

  const processDeploymentData = () => {
    const last30Days = getLast30Days();
    const dailyStats: Record<string, { completed: number; failed: number }> =
      {};

    last30Days.forEach((day) => {
      dailyStats[format(day, "MM-dd")] = { completed: 0, failed: 0 };
    });

    const allDeployments = [
      ...(firmwareDeployments?.items ?? []),
      ...(adDeployments?.items ?? []),
    ];

    allDeployments.forEach((deployment) => {
      const dateValue = deployment.deployedAt; // or deployedAt
      const date =
        typeof dateValue === "string" ? parseISO(dateValue) : dateValue;

      if (!date) return;

      const dayKey = format(date, "MM-dd");

      if (dailyStats[dayKey]) {
        if (deployment.status === "COMPLETED") {
          dailyStats[dayKey].completed += 1;
        } else if (deployment.status === "FAILED") {
          dailyStats[dayKey].failed += 1;
        }
      }
    });

    return Object.keys(dailyStats).map((date) => ({
      date,
      ...dailyStats[date],
    }));
  };

  const chartData = processDeploymentData();

  return (
    <div className="p-6 bg-white rounded-lg shadow-md w-full">
      <h3 className="mb-4 text-lg font-semibold text-gray-800">
        최근 30일 배포 현황
      </h3>
      <ResponsiveContainer width="100%" height={200}>
        <LineChart data={chartData}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="date" />
          <YAxis />
          <Tooltip />
          <Legend />
          <Line
            type="monotone"
            dataKey="completed"
            stroke="#34d399" // Emerald 400
            name="성공"
          />
          <Line
            type="monotone"
            dataKey="failed"
            stroke="#f87171" // Red 400
            name="실패"
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
};
