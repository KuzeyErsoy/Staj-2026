#pragma once
#include "FleetTypes.h"
#include <vector>
#include <memory>
#include <string>

namespace DroneFleet {

enum class MapFilterMode {
    ALL_DRONES,      // [1] All Drones (9)
    RECON_ONLY,      // [2] Recon Only (2)
    TASK_FORCE_ONLY, // [3] Task Force Only (5)
    SUPPORT_ONLY,    // [4] Support Only (2)
    HOSTILES_ONLY    // [5] Hostiles / Threat Sectors Only
};

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
    bool IsMapFullscreen() const { return isMapFullscreen; }
    void ToggleDroneInspector() { showDroneInspector = !showDroneInspector; }
    void ToggleAllPanels();
    void SelectInspectorDrone(int id) { if (id >= 1 && id <= 9) inspectorDroneId = id; }
    int GetInspectorDroneId() const { return inspectorDroneId; }

    // Tactical Map Filters & Interactive Inspection (Day 8)
    void SetMapFilter(MapFilterMode mode) { activeMapFilter = mode; }
    MapFilterMode GetMapFilter() const { return activeMapFilter; }
    void SelectCluster(int id) { selectedClusterId = id; }
    int GetSelectedClusterId() const { return selectedClusterId; }
    void SelectMapDrone(int id) { selectedMapDroneId = id; }
    int GetSelectedMapDroneId() const { return selectedMapDroneId; }

    // Picture-in-Picture (PiP) Nose Camera
    void TogglePiPWindow() { showPiPWindow = !showPiPWindow; }
    void CyclePiPTarget(int dir);
    void SetPiPTarget(int id) { if (id >= 0 && id <= 9) pipDroneTarget = id; }
    int GetPiPTarget() const { return pipDroneTarget; }
    bool IsPiPEnabled() const { return showPiPWindow; }
    RenderTexture2D GetPiPRenderTexture() const { return pipTexture; }
    int GetEffectivePiPDroneIndex(const std::vector<std::unique_ptr<Drone>>& drones) const;

    // FLIR / Night Vision Modes
    void ToggleThermalFLIR();
    void ToggleNightVision();
    void SetVisionMode(VisionMode mode) { visionMode = mode; }
    VisionMode GetVisionMode() const { return visionMode; }

    // Operation Debriefing Panel (Day 6)
    void ToggleDebriefingPanel() { showDebriefingPanel = !showDebriefingPanel; }
    void SetDebriefingPanel(bool show) { showDebriefingPanel = show; }
    bool IsDebriefingActive() const { return showDebriefingPanel; }

private:
    Font customFont;
    bool fontLoaded;

    RenderTexture2D pipTexture;
    bool pipTextureLoaded;
    bool showPiPWindow = true;
    int pipDroneTarget = 0; // 0 = Auto Priority, 1..9 = Specific Drone

    VisionMode visionMode = VisionMode::NORMAL;

    bool showTelemetryPanel = true;
    bool showResourcePanel = true;
    bool showTerminalPanel = true;
    bool showMissionsPanel = true;
    bool showCameraBanner = true;
    bool showTacticalMap = true;
    bool isMapFullscreen = false;
    bool showDroneInspector = false;
    bool showDebriefingPanel = false;
    int inspectorDroneId = 1;

    // Tactical Map Filter & Selection State (Day 8)
    mutable MapFilterMode activeMapFilter = MapFilterMode::ALL_DRONES;
    mutable int selectedClusterId = -1;
    mutable int selectedMapDroneId = -1;

    void DrawActiveMissionsPanel2D(const std::vector<EnemyCluster>& clusters, const std::vector<std::unique_ptr<Drone>>& drones) const;
    void DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const;
    void DrawSNRCoverageRings2D(int centerX, int centerY, float scale, bool fullscreen) const;
    void DrawTargetAnalysisCard2D(int cardX, int cardY, int cardW, int cardH, const EnemyCluster& cluster, const std::vector<std::unique_ptr<Drone>>& drones) const;
    void DrawMapDroneQuickCard2D(int cardX, int cardY, int cardW, int cardH, const Drone& drone) const;
    void DrawMapFilterBar2D(int barX, int barY, int barW, int barH, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const;
    void DrawDroneInspector2D(const std::vector<std::unique_ptr<Drone>>& drones) const;
    void DrawPiPCam2D(const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters, const FleetCamera& fleetCam) const;
    void DrawFPVSignalOverlay2D(const Drone& activeDrone) const;
    void DrawFLIROverlay2D(const Camera3D& cam, const std::vector<EnemyCluster>& clusters, const std::vector<std::unique_ptr<Drone>>& drones) const;
    void DrawDebriefingPanel2D(const TelemetryContext& telemetry, const std::vector<std::unique_ptr<Drone>>& drones) const;
};

} // namespace DroneFleet

