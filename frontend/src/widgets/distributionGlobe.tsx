import React, { useEffect, useRef, useState } from "react";
import Globe, { GlobeMethods } from "react-globe.gl";
import { scaleSequentialSqrt } from "d3-scale";
import { interpolateYlOrRd } from "d3-scale-chromatic";

interface DistributionGlobeProps {
  width: number;
  height: number;
  locations: Point[];
  rotationSpeed?: number;
  globeImageUrl?: string;
  clusterRadius?: number;
}

interface Point {
  name: string;
  latitude: number;
  longitude: number;
  value: number;
}

interface PointData {
  lat: number;
  lng: number;
  size: number;
  color: string;
  label: string;
  altitude: number;
}

const DistributionGlobe: React.FC<DistributionGlobeProps> = ({
  width,
  height,
  locations,
  rotationSpeed = 1.0,
  globeImageUrl = "//unpkg.com/three-globe/example/img/earth-night.jpg",
  clusterRadius = 50,
}) => {
  const globeRef = useRef<GlobeMethods>(undefined);
  const [pointsData, setPointsData] = useState<PointData[]>([]);
  const weightColor = scaleSequentialSqrt(interpolateYlOrRd).domain([0, 1e7]);

  // calculate distance between two points - Haversine formula
  const calculateDistance = (
    lat1: number,
    lng1: number,
    lat2: number,
    lng2: number,
  ): number => {
    const R = 6371;
    const dLat = ((lat2 - lat1) * Math.PI) / 180;
    const dLng = ((lng2 - lng1) * Math.PI) / 180;
    const a =
      Math.sin(dLat / 2) * Math.sin(dLat / 2) +
      Math.cos((lat1 * Math.PI) / 180) *
        Math.cos((lat2 * Math.PI) / 180) *
        Math.sin(dLng / 2) *
        Math.sin(dLng / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    return R * c;
  };

  useEffect(() => {
    const validLocations = locations.filter(
      (rl) => rl.latitude != null && rl.longitude != null,
    );

    const clustered: PointData[] = [];
    const processed = new Set<string>();

    validLocations.forEach((location) => {
      if (processed.has(location.name)) return;

      const nearby = validLocations.filter(
        (rl) =>
          !processed.has(rl.name) &&
          calculateDistance(
            rl.latitude,
            rl.longitude,
            location.latitude,
            location.longitude,
          ) <= clusterRadius,
      );

      if (nearby.length > 0) {
        const totalValue = nearby.reduce((sum, nr) => sum + nr.value, 0);
        const cenLat =
          nearby.reduce((sum, nr) => sum + nr.latitude * nr.value, 0) /
          totalValue;
        const cenLng =
          nearby.reduce((sum, nr) => sum + nr.longitude * nr.value, 0) /
          totalValue;

        nearby.forEach((nr) => processed.add(nr.name));

        clustered.push({
          lat: cenLat,
          lng: cenLng,
          size: 0.2,
          color: weightColor(totalValue),
          label: `${cenLat.toFixed(2)}, ${cenLng.toFixed(2)}\n${totalValue}`,
          altitude: 0.03 + Math.min(totalValue * 0.005, 0.6),
        });
      }
    });
    setPointsData(clustered);
  }, [locations, clusterRadius]);

  useEffect(() => {
    const controls = globeRef.current?.controls();
    if (controls) {
      controls.autoRotate = true;
      controls.autoRotateSpeed = rotationSpeed;
    }
    globeRef.current?.pointOfView({
      lat: 35.1731,
      lng: 129.0714,
      altitude: 1.7,
    }); // Busan!
  }, [rotationSpeed]);

  return (
    <div>
      <Globe
        ref={globeRef}
        width={width}
        height={height}
        globeImageUrl={globeImageUrl}
        bumpImageUrl="//unpkg.com/three-globe/example/img/earth-topology.png"
        backgroundColor="rgba(0,0,0,0)"
        pointsData={pointsData}
        pointAltitude="altitude"
        pointColor="color"
        pointLabel="label"
        pointRadius="size"
        pointsMerge={false}
        labelSize={1.5}
        labelDotRadius={0.4}
        labelColor={() => "rgba(255, 255, 255, 0.75)"}
        labelResolution={2}
        enablePointerInteraction={true}
      />
    </div>
  );
};

export default DistributionGlobe;
