#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

namespace DroneFleet {

// --- ENUMS ---
enum class DroneRole {
    RECON,
    TASK_FORCE,
    SUPPORT
};

enum class DroneState {
    IDLE_AT_BASE,
    PATROL,
    SCANNING,
    MOVING_TO_TARGET,
    ENGAGING,
    REQUESTING_SUPPORT,
    RESUPPLYING_IN_AIR,
    LOITERING_IN_AREA,
    RETURNING_TO_BASE
};

enum class ClusterStatus {
    UNDETECTED,
    REPORTED,
    ENGAGED,
    CLEARED,
    REQUIRES_RECON
};

enum class VisionMode {
    NORMAL,
    FLIR_THERMAL,
    NVG_GREEN
};

// --- DATA STRUCTURES ---
struct CountermeasureParticle {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Color color = WHITE;
    float life = 0.0f;
    float maxLife = 1.5f;
    float size = 0.28f;
    bool isSmoke = false;
    bool isAlive = true;
};

struct Enemy {
    int id = 0;
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    float hp = 100.0f;
    float maxHp = 100.0f;
    bool isAlive = true;
    float panicTimer = 0.0f;
    Vector3 fleeTarget = { 0.0f, 0.0f, 0.0f };

    // Hostile Counter-Attack
    bool isArmed = true; // Armed (0.5 threat mag, counter-attacks) vs Unarmed (0.25 threat mag, flees)
    bool isFiringCounterAttack = false;
    Vector3 counterAttackTargetPos = { 0.0f, 0.0f, 0.0f };
};

struct EnemyCluster {
    int id = 0;
    Vector3 centerPosition = { 0.0f, 0.0f, 0.0f };
    int initialMagnitude = 0;
    int remainingMagnitude = 0;
    int armedCount = 0;
    int unarmedCount = 0;
    int calculatedThreatMagnitude = 0;
    ClusterStatus status = ClusterStatus::UNDETECTED;
    std::vector<Enemy> enemies;
};

struct TelemetryContext {
    int totalFleetCount = 9;
    int activeInMission = 0;
    int dockedAtBase = 9;
    
    int knownTargetsMagnitude = 0;
    int eliminatedTargetsCount = 0;
    int activeClustersCount = 0;
    int destroyedDronesCount = 0;
    
    std::vector<std::string> terminalLogs;
};

// Forward Declarations
class Drone;
class ClusterManager;
class MissileSystem;
class HangarBase;

// Context passed to drones during Update to access fleet components without circular dependencies
struct FleetContext {
    std::vector<std::unique_ptr<Drone>>& drones;
    ClusterManager& clusterManager;
    MissileSystem& missileSystem;
    HangarBase& hangarBase;
    TelemetryContext& telemetry;

    void LogTerminal(const std::string& msg);
};

} // namespace DroneFleet
