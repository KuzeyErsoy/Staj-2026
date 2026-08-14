#include "HangarBase.h"
#include "Drone.h"
#include "rlgl.h"
#include <cstdio>

namespace DroneFleet {

HangarBase::HangarBase()
    : position(Vector3{ 0.0f, 0.0f, 0.0f }),
      radius(25.0f),
      fuelSupply(9999.0f),
      ammoSupply(9999.0f) {
    Init();
}

void HangarBase::Init() {
    helipadPositions.clear();

    // 1. Recon Docks (Bay #1 & #2)
    helipadPositions.push_back(Vector3{ -14.0f, 0.6f, -12.0f });
    helipadPositions.push_back(Vector3{ -14.0f, 0.6f, -6.0f });

    // 2. Task Force Docks (Bay #3 .. #7)
    for (int i = 0; i < 5; i++) {
        helipadPositions.push_back(Vector3{ 0.0f, 0.6f, -12.0f + (float)i * 6.0f });
    }

    // 3. Support Docks (Bay #8 & #9)
    helipadPositions.push_back(Vector3{ 14.0f, 0.6f, -6.0f });
    helipadPositions.push_back(Vector3{ 14.0f, 0.6f, 6.0f });
}

Vector3 HangarBase::GetDockPosition(int droneId) const {
    int idx = droneId - 1;
    if (idx >= 0 && idx < (int)helipadPositions.size()) {
        return helipadPositions[idx];
    }
    return position;
}

void HangarBase::UpdateBaseRefill(Drone& drone, FleetContext& ctx, float dt) {
    if (drone.isDestroyed) return;

    float distToBase = Vector3Distance(drone.position, drone.baseDockPos);

    if (distToBase < 8.0f) {
        if (drone.state == DroneState::RETURNING_TO_BASE || drone.state == DroneState::IDLE_AT_BASE) {
            float refillRate = dt * 50.0f;
            drone.fuel = std::min(drone.maxFuel, drone.fuel + refillRate);
            drone.hp = std::min(drone.maxHp, drone.hp + refillRate);
            if (drone.maxAmmo > 0.0f) {
                drone.ammo = std::min(drone.maxAmmo, drone.ammo + refillRate);
            }

            if (drone.role == DroneRole::SUPPORT) {
                SupportDrone* sup = dynamic_cast<SupportDrone*>(&drone);
                if (sup) {
                    sup->cargoFuel = std::min(600.0f, sup->cargoFuel + dt * 100.0f);
                    sup->cargoAmmo = std::min(240.0f, sup->cargoAmmo + dt * 80.0f);
                }
            }

            if (drone.fuel >= drone.maxFuel - 1.0f && drone.hp >= drone.maxHp - 1.0f && (drone.maxAmmo <= 0.0f || drone.ammo >= drone.maxAmmo - 1.0f)) {
                if (drone.state == DroneState::RETURNING_TO_BASE) {
                    drone.ResetToDock();
                    drone.LogAction("Safely docked in Hangar. Refueled, rearmed & repaired 100%.");
                    char buf[128];
                    snprintf(buf, sizeof(buf), "[HANGAR BASE] Drone #%d 100%% refueled & repaired on Helipad. Ready.", drone.id);
                    ctx.LogTerminal(buf);
                }
            }
        }
    }
}

void HangarBase::Draw3D(const std::vector<std::unique_ptr<Drone>>& drones) const {
    DrawGrid(100, 10.0f);

    DrawCylinder(position, radius, radius, 0.2f, 32, Color{ 0, 180, 255, 40 });
    DrawCircle3D(position, radius, Vector3{ 1, 0, 0 }, 90.0f, Color{ 0, 255, 255, 200 });

    Vector3 hPos = Vector3{ 0.0f, 5.0f, 0.0f };
    DrawCubeWires(hPos, 36.0f, 10.0f, 44.0f, SKYBLUE);
    DrawCube(Vector3{ 0.0f, 10.1f, 0.0f }, 36.5f, 0.4f, 44.5f, Color{ 20, 50, 80, 220 });

    float pX[4] = { -18.0f, 18.0f, -18.0f, 18.0f };
    float pZ[4] = { -22.0f, -22.0f, 22.0f, 22.0f };
    for (int i = 0; i < 4; i++) {
        DrawCylinder(Vector3{ pX[i], 2.5f, pZ[i] }, 0.8f, 0.8f, 5.0f, 8, DARKGRAY);
    }

    DrawCylinder(Vector3{ -20.0f, 3.0f, -10.0f }, 2.5f, 2.5f, 6.0f, 16, ORANGE);
    DrawCylinder(Vector3{ -20.0f, 3.0f, 10.0f }, 2.5f, 2.5f, 6.0f, 16, YELLOW);

    for (const auto& d : drones) {
        Color padColor = (d->role == DroneRole::RECON) ? SKYBLUE : ((d->role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
        DrawCircle3D(d->baseDockPos, 2.2f, Vector3{ 1, 0, 0 }, 90.0f, padColor);
        DrawCircle3D(d->baseDockPos, 1.1f, Vector3{ 1, 0, 0 }, 90.0f, RAYWHITE);
    }
}

} // namespace DroneFleet
