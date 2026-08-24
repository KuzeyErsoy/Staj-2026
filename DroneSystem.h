#pragma once
#include "fleet/FleetTypes.h"
#include "fleet/HangarBase.h"
#include "fleet/ClusterManager.h"
#include "fleet/MissileSystem.h"
#include "fleet/Drone.h"
#include "fleet/FleetCamera.h"
#include "fleet/FleetHUD.h"
#include <vector>
#include <memory>
#include <string>

class DroneSystemManager {
public:
    DroneSystemManager();
    ~DroneSystemManager() = default;

    void Init();
    void Update(float dt);
    void Draw3D() const;
    void Draw2DHUD() const;

    void LogTerminal(const std::string& msg);
    void UpdateFleetCamera(Camera3D& camera, float dt);
    int GetActiveCameraMode() const { return fleetCamera.GetActiveCameraMode(); }
    std::string GetActiveCameraModeName() const { return fleetCamera.GetActiveCameraModeName(); }

    // UI Panel Toggles & Inspector
    void ToggleTelemetryPanel() { fleetHUD.ToggleTelemetryPanel(); }
    void ToggleResourcePanel() { fleetHUD.ToggleResourcePanel(); }
    void ToggleTerminalPanel() { fleetHUD.ToggleTerminalPanel(); }
    void ToggleCameraBanner() { fleetHUD.ToggleCameraBanner(); }
    void ToggleTacticalMap() { fleetHUD.ToggleTacticalMap(); }
    void ToggleFullscreenMap();
    bool IsMapFullscreen() const { return fleetHUD.IsMapFullscreen(); }
    void ToggleDroneInspector() { fleetHUD.ToggleDroneInspector(); }
    void ToggleAllPanels() { fleetHUD.ToggleAllPanels(); }
    void SelectInspectorDrone(int id) { fleetHUD.SelectInspectorDrone(id); }

    // Picture-in-Picture (PiP) Nose Camera
    void RenderPiPCam();
    void TogglePiPWindow();
    void CyclePiPTarget(int dir);
    void SetPiPTarget(int id) { fleetHUD.SetPiPTarget(id); }
    int GetPiPTarget() const { return fleetHUD.GetPiPTarget(); }

    // FLIR / Night Vision Modes
    void ToggleThermalFLIR();
    void ToggleNightVision();
    void SetVisionMode(DroneFleet::VisionMode mode) { fleetHUD.SetVisionMode(mode); }
    DroneFleet::VisionMode GetVisionMode() const { return fleetHUD.GetVisionMode(); }

    // Debriefing Panel & Waves (Day 6)
    void ToggleDebriefingPanel();
    void StartNextWave();

    // Fleet Accessors
    const DroneFleet::HangarBase& GetHangarBase() const { return hangarBase; }
    const std::vector<std::unique_ptr<DroneFleet::Drone>>& GetDrones() const { return drones; }
    const DroneFleet::ClusterManager& GetClusterManager() const { return clusterManager; }
    const DroneFleet::TelemetryContext& GetTelemetry() const { return telemetry; }

private:
    DroneFleet::HangarBase hangarBase;
    DroneFleet::ClusterManager clusterManager;
    DroneFleet::MissileSystem missileSystem;
    DroneFleet::FleetCamera fleetCamera;
    DroneFleet::FleetHUD fleetHUD;

    std::vector<std::unique_ptr<DroneFleet::Drone>> drones;
    DroneFleet::TelemetryContext telemetry;

    float timeScale = 1.0f;
    bool isPaused = false;

    void TogglePause();
    void IncreaseSpeed();
    void DecreaseSpeed();
    void ResetSpeed();
    void UpdateTelemetry();
};
