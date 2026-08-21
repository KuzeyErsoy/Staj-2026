#pragma once
#include "FleetTypes.h"
#include <vector>
#include <string>

namespace DroneFleet {

// --- BASE ABSTRACT DRONE CLASS ---
class Drone {
public:
    int id = 0;
    DroneRole role = DroneRole::RECON;
    DroneState state = DroneState::IDLE_AT_BASE;
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 targetPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };

    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float rotorAngle = 0.0f;

    float fuel = 100.0f;
    float maxFuel = 100.0f;
    float ammo = 100.0f;
    float maxAmmo = 100.0f;
    float hp = 100.0f;
    float maxHp = 100.0f;
    bool isDestroyed = false;
    bool isBeingRecovered = false;

    Vector3 baseDockPos = { 0.0f, 0.0f, 0.0f };
    std::vector<std::string> actionLogs;

    // Tactical state
    int assignedClusterId = -1;
    Vector3 rendezvousPos = { 0.0f, 0.0f, 0.0f };
    bool hasRendezvous = false;
    int assignedSupportId = -1;

    // Radio & Telemetry Link Metrics (SNR & Distance)
    float distanceFromBase = 0.0f;
    float snrDb = 45.0f;
    float signalQuality = 1.0f; // 0.0 to 1.0
    float telemetryLatencyMs = 12.0f;
    float bitrateMbps = 12.0f;

    // Visual FX state
    bool isFiringLaser = false;
    Vector3 laserTargetPos = { 0.0f, 0.0f, 0.0f };
    bool isResupplyBeamActive = false;
    Vector3 resupplyTargetPos = { 0.0f, 0.0f, 0.0f };

    // Defensive Flare / Chaff Countermeasures
    std::vector<CountermeasureParticle> flareParticles;
    float flareCooldown = 0.0f;
    float flareAlertTimer = 0.0f;

    Drone(int id, DroneRole role);
    virtual ~Drone() = default;

    void LogAction(const std::string& msg);
    void DeployFlares(FleetContext* ctx = nullptr);
    void UpdateFlares(float dt);
    virtual void Update(FleetContext& ctx, float dt) = 0;
    virtual void Draw3D(VisionMode visionMode = VisionMode::NORMAL) const;
    virtual void ResetToDock();

protected:
    void UpdateMovementPhysics(float targetSpeed, float dt);
    void ApplyFlankingAndSeparation(FleetContext& ctx, float dt);
    void DrawNavigationLEDs(VisionMode visionMode) const;
    void DrawGroundSpotlight(VisionMode visionMode) const;
    void DrawFlares(VisionMode visionMode) const;
};

// --- RECON DRONE (SCOUT / HIGH ALTITUDE SWEEP) ---
class ReconDrone : public Drone {
public:
    int reconSectorIndex = 0;
    float scanTimer = 0.0f;

    ReconDrone(int id, Vector3 dockPos, int startingSector = 0);
    void Update(FleetContext& ctx, float dt) override;
    void Draw3D(VisionMode visionMode = VisionMode::NORMAL) const override;
};

// --- TASK FORCE DRONE (COMBAT STRIKE / PINCER FORMATION / MISSILES) ---
class TaskForceDrone : public Drone {
public:
    float missileCooldown = 0.0f;

    TaskForceDrone(int id, Vector3 dockPos);
    void Update(FleetContext& ctx, float dt) override;
    void Draw3D(VisionMode visionMode = VisionMode::NORMAL) const override;
};

// --- SUPPORT DRONE (CARGO TANKER / COMBAT MEDIC / FORWARD LOITERING) ---
class SupportDrone : public Drone {
public:
    float cargoFuel = 600.0f;
    float cargoAmmo = 240.0f;

    int servicingReconId = -1;
    int servicingTFId = -1;
    int recoveringDroneId = -1;
    Vector3 loiterCenterPos = { 0.0f, 34.0f, 0.0f };

    SupportDrone(int id, Vector3 dockPos);
    void Update(FleetContext& ctx, float dt) override;
    void Draw3D(VisionMode visionMode = VisionMode::NORMAL) const override;
    void ResetToDock() override;
};

} // namespace DroneFleet
