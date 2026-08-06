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

    // Hostile Anti-Drone Counter-Attack
    bool isFiringCounterAttack = false;
    Vector3 counterAttackTargetPos = { 0.0f, 0.0f, 0.0f };
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

    // Health & Damage System
    float hp = 100.0f;
    float maxHp = 100.0f;
    bool isDestroyed = false;
    
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
    // Docking & Logs
    Vector3 baseDockPos = { 0.0f, 0.0f, 0.0f };
    std::vector<std::string> actionLogs;
    
    // Recon Grid Sweep & Rendezvous State
    int reconSectorIndex = 0;
    Vector3 currentSweepWaypoint = { 0.0f, 20.0f, 0.0f };
    Vector3 rendezvousPos = { 0.0f, 0.0f, 0.0f };
    bool hasRendezvous = false;
    int assignedSupportId = -1;
    int servicingReconId = -1;

    void LogAction(const std::string& msg) {
        int totalSecs = (int)GetTime();
        int mins = (totalSecs / 60) % 60;
        int secs = totalSecs % 60;
        char timeStr[32];
        snprintf(timeStr, sizeof(timeStr), "[%02d:%02d] ", mins, secs);
        actionLogs.push_back(std::string(timeStr) + msg);
        if (actionLogs.size() > 10) actionLogs.erase(actionLogs.begin());
    }
};

struct HomeBase {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float radius = 25.0f;
    float fuelSupply = 9999.0f;
    float ammoSupply = 9999.0f;
};

// HUD ve Terminal Veri Paneli
struct TelemetryContext {
    int totalFleetCount = 14;
    int activeInMission = 0;
    int dockedAtBase = 14;
    
    int knownTargetsMagnitude = 0;
    int eliminatedTargetsCount = 0;
    int activeClustersCount = 0;
    int destroyedDronesCount = 0;
    
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

    // UI Panel Toggles & Inspector
    void ToggleTelemetryPanel() { showTelemetryPanel = !showTelemetryPanel; }
    void ToggleResourcePanel() { showResourcePanel = !showResourcePanel; }
    void ToggleTerminalPanel() { showTerminalPanel = !showTerminalPanel; }
    void ToggleCameraBanner() { showCameraBanner = !showCameraBanner; }
    void ToggleTacticalMap() { showTacticalMap = !showTacticalMap; }
    void ToggleFullscreenMap() { isMapFullscreen = !isMapFullscreen; }
    void ToggleDroneInspector() { showDroneInspector = !showDroneInspector; }
    void ToggleAllPanels() {
        bool anyOn = showTelemetryPanel || showResourcePanel || showTerminalPanel || showCameraBanner || showTacticalMap;
        showTelemetryPanel = !anyOn;
        showResourcePanel = !anyOn;
        showTerminalPanel = !anyOn;
        showCameraBanner = !anyOn;
        showTacticalMap = !anyOn;
    }
    void SelectInspectorDrone(int id) {
        if (id >= 1 && id <= 14) inspectorDroneId = id;
    }

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
    int activeCameraMode; // 0 = Free Flight Cam, 1..14 = Tracking Drones 1..14
    Vector3 freeCamPos;
    float freeCamYaw;
    float freeCamPitch;
    float trackCamYaw;
    float trackCamPitch;

    // Font Handling
    Font customFont;
    bool fontLoaded;

    // UI Toggles & Inspector State
    bool showTelemetryPanel = true;
    bool showResourcePanel = true;
    bool showTerminalPanel = true;
    bool showCameraBanner = true;
    bool showTacticalMap = true;
    bool isMapFullscreen = false;
    bool showDroneInspector = false;
    int inspectorDroneId = 1;

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

    // UI Drawing Helpers
    void DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen) const;
    void DrawDroneInspector2D() const;
};
