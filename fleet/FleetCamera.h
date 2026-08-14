#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <memory>

namespace DroneFleet {

class Drone;

class FleetCamera {
public:
    FleetCamera();
    ~FleetCamera() = default;

    void Update(Camera3D& camera, const std::vector<std::unique_ptr<Drone>>& drones, float dt);
    std::string GetActiveCameraModeName() const;
    int GetActiveCameraMode() const { return activeCameraMode; }
    void SetActiveCameraMode(int mode) { activeCameraMode = mode; }
    Camera3D GetCurrentCamera3D(const std::vector<std::unique_ptr<Drone>>& drones) const;

private:
    int activeCameraMode; // 0 = Free Cam, 1..9 = Tracking Drones 1..9
    Vector3 freeCamPos;
    float freeCamYaw;
    float freeCamPitch;
    float trackCamYaw;
    float trackCamPitch;
};

} // namespace DroneFleet
