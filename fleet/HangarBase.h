#pragma once
#include "FleetTypes.h"
#include <vector>

namespace DroneFleet {

class HangarBase {
public:
    HangarBase();
    ~HangarBase() = default;

    void Init();
    void UpdateBaseRefill(Drone& drone, FleetContext& ctx, float dt);
    void Draw3D(const std::vector<std::unique_ptr<Drone>>& drones, VisionMode visionMode = VisionMode::NORMAL) const;

    Vector3 GetPosition() const { return position; }
    float GetRadius() const { return radius; }
    Vector3 GetDockPosition(int droneId) const;

private:
    Vector3 position;
    float radius;
    float fuelSupply;
    float ammoSupply;
    std::vector<Vector3> helipadPositions;
};

} // namespace DroneFleet
