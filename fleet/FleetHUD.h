#pragma once
#include "FleetTypes.h"
#include <vector>
#include <memory>
#include <string>

namespace DroneFleet {

class FleetCamera;

class FleetHUD {
public:
    FleetHUD();
    ~FleetHUD();

    void Init();
    void Draw2DHUD(const TelemetryContext& telemetry,
                   const std::vector<std::unique_ptr<Drone>>& drones,
                   const std::vector<EnemyCluster>& clusters,
                   const FleetCamera& fleetCam,
                   float timeScale,
                   bool isPaused) const;

    void DrawCustomText(const char* text, float posX, float posY, float fontSize, Color color) const;

    // UI Toggles
    void ToggleTelemetryPanel() { showTelemetryPanel = !showTelemetryPanel; }
    void ToggleResourcePanel() { showResourcePanel = !showResourcePanel; }
    void ToggleTerminalPanel() { showTerminalPanel = !showTerminalPanel; }
    void ToggleCameraBanner() { showCameraBanner = !showCameraBanner; }
    void ToggleTacticalMap() { showTacticalMap = !showTacticalMap; }
    void ToggleFullscreenMap() { isMapFullscreen = !isMapFullscreen; }
    void ToggleDroneInspector() { showDroneInspector = !showDroneInspector; }
    void ToggleAllPanels();
    void SelectInspectorDrone(int id) { if (id >= 1 && id <= 9) inspectorDroneId = id; }
    int GetInspectorDroneId() const { return inspectorDroneId; }

private:
    Font customFont;
    bool fontLoaded;

    bool showTelemetryPanel = true;
    bool showResourcePanel = true;
    bool showTerminalPanel = true;
    bool showMissionsPanel = true;
    bool showCameraBanner = true;
    bool showTacticalMap = true;
    bool isMapFullscreen = false;
    bool showDroneInspector = false;
    int inspectorDroneId = 1;

    void DrawActiveMissionsPanel2D(const std::vector<EnemyCluster>& clusters, const std::vector<std::unique_ptr<Drone>>& drones) const;
    void DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const;
    void DrawDroneInspector2D(const std::vector<std::unique_ptr<Drone>>& drones) const;
};

} // namespace DroneFleet
