#pragma once
#include "raylib.h"
#include <vector>
#include <string>

namespace DroneFleet {

// --- ENUM TANIMLARI ---
enum class DroneRole { RECON, TASK_FORCE, SUPPORT };

enum class DroneState {
    IDLE_AT_BASE,
    PATROL,
    SCANNING,
    MOVING_TO_TARGET,
    ENGAGING,
    REQUESTING_SUPPORT,
    RESUPPLYING_IN_AIR,
    RETURNING_TO_BASE
};

enum class ClusterStatus { UNDETECTED, REPORTED, ENGAGED, CLEARED, REQUIRES_RECON };

// --- VERİ YAPILARI ---
struct Enemy {
    int id;
    Vector3 position;
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    float hp = 100.0f;
    float maxHp = 100.0f;
    bool isAlive = true;
    float panicTimer = 0.0f;
    Vector3 fleeTarget = { 0.0f, 0.0f, 0.0f };
};

struct EnemyCluster {
    int id;
    Vector3 centerPosition;
    int initialMagnitude;   // Gruptaki toplam düşman sayısı
    int remainingMagnitude; // Anlık canlı düşman sayısı
    ClusterStatus status = ClusterStatus::UNDETECTED;
    std::vector<Enemy> enemies;
};

struct Drone {
    int id;
    DroneRole role;
    DroneState state = DroneState::IDLE_AT_BASE;
    Vector3 position;
    Vector3 targetPosition;
    
    // Kaynaklar (0.0 - 100.0)
    float fuel = 100.0f;
    float ammo = 100.0f;
    float maxFuel = 100.0f;
    float maxAmmo = 100.0f;
    
    // Support Kargo Kapasitesi (Sadece SUPPORT rolündekiler için)
    float cargoFuel = 200.0f;
    float cargoAmmo = 200.0f;

    int assignedClusterId = -1;

    // Görsel Sınıf Nitelikleri & Fizik Nitelikleri
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float rotorAngle = 0.0f;
    bool isFiringLaser = false;
    Vector3 laserTargetPos = { 0, 0, 0 };
    bool isResupplyBeamActive = false;
    Vector3 resupplyTargetPos = { 0, 0, 0 };
};

struct HomeBase {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float radius = 20.0f;
    float fuelSupply = 9999.0f; // Sınırsız üs stoğu
    float ammoSupply = 9999.0f;
};

// HUD ve Terminal Veri Paneli
struct TelemetryContext {
    int totalFleetCount = 5;
    int activeInMission = 0;
    int dockedAtBase = 5;
    
    int knownTargetsMagnitude = 0;
    int eliminatedTargetsCount = 0;
    int activeClustersCount = 0;
    
    std::vector<std::string> terminalLogs;
};

} // namespace DroneFleet

class DroneSystemManager {
public:
    DroneSystemManager();
    ~DroneSystemManager();

    void Init();
    void Update(float dt);
    void Draw3D() const;
    void Draw2DHUD() const;

    void LogTerminal(const std::string& msg);
    void UpdateFleetCamera(Camera3D& camera, float dt);
    int GetActiveCameraMode() const { return activeCameraMode; }
    std::string GetActiveCameraModeName() const;

    // Fleet accessors
    const DroneFleet::HomeBase& GetHomeBase() const { return base; }
    const std::vector<DroneFleet::Drone>& GetDrones() const { return drones; }
    const std::vector<DroneFleet::EnemyCluster>& GetClusters() const { return clusters; }
    const DroneFleet::TelemetryContext& GetTelemetry() const { return telemetry; }

private:
    DroneFleet::HomeBase base;
    std::vector<DroneFleet::Drone> drones;
    std::vector<DroneFleet::EnemyCluster> clusters;
    DroneFleet::TelemetryContext telemetry;

    int nextClusterId;
    float scanTimer;
    float patrolAngle;

    // Fleet Camera Controls
    int activeCameraMode; // 0 = Free Flight Cam, 1..5 = Tracking Drones 1..5
    Vector3 freeCamPos;
    float freeCamYaw;
    float freeCamPitch;
    float trackCamYaw;
    float trackCamPitch;

    // Font Handling
    Font customFont;
    bool fontLoaded;

    void DrawCustomText(const char* text, float posX, float posY, float fontSize, Color color) const;

    void SpawnCluster();
    void ApplyFlankingAndSeparation(DroneFleet::Drone& drone, float dt);
    void UpdateRecon(DroneFleet::Drone& drone, float dt);
    void UpdateTaskForce(DroneFleet::Drone& drone, float dt);
    void UpdateSupport(DroneFleet::Drone& drone, float dt);
    void UpdateBaseRefill(DroneFleet::Drone& drone, float dt);
    void UpdateTelemetry();

    void DrawDrone3D(const DroneFleet::Drone& drone) const;
    void DrawBase3D() const;
    void DrawClusters3D() const;
};
