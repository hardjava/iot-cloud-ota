import React from "react";

interface MetricCardProps {
  title: string;
  value: number | string;
}

export const MetricCard: React.FC<MetricCardProps> = ({ title, value }) => {
  return (
    <div className="p-4 bg-white rounded-lg shadow-md w-full">
      <p className="text-sm font-medium text-gray-500 text-center mb-4">
        {title}
      </p>
      <p className="text-3xl font-bold text-gray-900 text-center">{value}</p>
    </div>
  );
};
