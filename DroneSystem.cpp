#include "DroneSystem.h"
#include "AudioManager.h"
#include <cstdio>
#include <ctime>

namespace DroneFleet {

void FleetContext::LogTerminal(const std::string& msg) {
    int totalSecs = (int)GetTime();
    int mins = (totalSecs / 60) % 60;
    int secs = totalSecs % 60;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "[%02d:%02d] ", mins, secs);

    telemetry.terminalLogs.push_back(std::string(timeStr) + msg);
    if (telemetry.terminalLogs.size() > 60) {
        telemetry.terminalLogs.erase(telemetry.terminalLogs.begin());
    }
}

} // namespace DroneFleet

using namespace DroneFleet;

DroneSystemManager::DroneSystemManager()
    : timeScale(1.0f), isPaused(false) {}

void DroneSystemManager::LogTerminal(const std::string& msg) {
    int totalSecs = (int)GetTime();
    int mins = (totalSecs / 60) % 60;
    int secs = totalSecs % 60;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "[%02d:%02d] ", mins, secs);

    telemetry.terminalLogs.push_back(std::string(timeStr) + msg);
    if (telemetry.terminalLogs.size() > 60) {
        telemetry.terminalLogs.erase(telemetry.terminalLogs.begin());
    }
}

void DroneSystemManager::TogglePause() {
    isPaused = !isPaused;
    LogTerminal(isPaused ? "[SIM CONTROL] Simulation PAUSED." : "[SIM CONTROL] Simulation RESUMED.");
}

void DroneSystemManager::IncreaseSpeed() {
    if (timeScale < 7.9f) timeScale *= 2.0f;
    char buf[64];
    snprintf(buf, sizeof(buf), "[SIM CONTROL] Speed increased: %.2fx", timeScale);
    LogTerminal(buf);
}

void DroneSystemManager::DecreaseSpeed() {
    if (timeScale > 0.3f) timeScale *= 0.5f;
    char buf[64];
    snprintf(buf, sizeof(buf), "[SIM CONTROL] Speed decreased: %.2fx", timeScale);
    LogTerminal(buf);
}

void DroneSystemManager::ResetSpeed() {
    timeScale = 1.0f;
    isPaused = false;
    LogTerminal("[SIM CONTROL] Speed reset to 1.0x Normal.");
}

void DroneSystemManager::Init() {
    hangarBase.Init();
    clusterManager.Init();
    missileSystem.Init();
    fleetHUD.Init();

    drones.clear();

    // 1. RECON #1 & #2 (High-Altitude 50m Sweeps, 4x Speed, 160 HP)
    drones.push_back(std::make_unique<ReconDrone>(1, hangarBase.GetDockPosition(1), 0));
    drones.push_back(std::make_unique<ReconDrone>(2, hangarBase.GetDockPosition(2), 8));

    // 2. TASK FORCE #3 .. #7 (Pincer Formations, Micro-Missiles, 2x Speed, 200 HP)
    for (int i = 0; i < 5; i++) {
        int id = 3 + i;
        drones.push_back(std::make_unique<TaskForceDrone>(id, hangarBase.GetDockPosition(id)));
    }

    // 3. SUPPORT #8 & #9 (Forward Loitering Station, SAR, 2x Speed, 300 HP)
    drones.push_back(std::make_unique<SupportDrone>(8, hangarBase.GetDockPosition(8)));
    drones.push_back(std::make_unique<SupportDrone>(9, hangarBase.GetDockPosition(9)));

    telemetry.terminalLogs.clear();
    telemetry.eliminatedTargetsCount = 0;
    telemetry.destroyedDronesCount = 0;
    timeScale = 1.0f;
    isPaused = false;

    LogTerminal("[TACTICAL COMMAND] 9-Drone Military Autonomous Fleet Initialized (OOP Architecture).");
    LogTerminal("[THREAT FORMULA] Magnitude = (Armed*0.5 + Unarmed*0.25) [Halved Ratios]");
    LogTerminal("[DOCTRINES ACTIVE] High-Alt Recon (50m), Pincer Strike, Micro-Missiles & Loitering Tankers.");

    clusterManager.SpawnCluster();
}

void DroneSystemManager::UpdateFleetCamera(Camera3D& camera, float dt) {
    fleetCamera.Update(camera, drones, dt);
    if (fleetCamera.GetActiveCameraMode() >= 1 && fleetCamera.GetActiveCameraMode() <= 9) {
        fleetHUD.SelectInspectorDrone(fleetCamera.GetActiveCameraMode());
    }
}

void DroneSystemManager::UpdateTelemetry() {
    telemetry.activeInMission = 0;
    telemetry.dockedAtBase = 0;
    telemetry.knownTargetsMagnitude = 0;
    telemetry.activeClustersCount = 0;

    for (const auto& d : drones) {
        if (d->isDestroyed) continue;
        if (d->state == DroneState::IDLE_AT_BASE) telemetry.dockedAtBase++;
        else telemetry.activeInMission++;
    }

    for (const auto& c : clusterManager.GetClusters()) {
        if (c.status != ClusterStatus::CLEARED) {
            telemetry.activeClustersCount++;
            if (c.status != ClusterStatus::UNDETECTED) {
                telemetry.knownTargetsMagnitude += c.calculatedThreatMagnitude;
            }
        }
    }
}

void DroneSystemManager::Update(float dt) {
    // Hotkeys
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
        TogglePause();
    }
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_RIGHT)) {
        IncreaseSpeed();
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_LEFT)) {
        DecreaseSpeed();
    }
    if (IsKeyPressed(KEY_R) && !isPaused && timeScale != 1.0f) {
        ResetSpeed();
    }

    // Manual Cluster Spawners
    if (IsKeyPressed(KEY_K)) {
        clusterManager.SpawnCustomCluster(true, GetRandomValue(6, 12));
        LogTerminal("[MANUAL SPAWN] Forced ARMED Cluster generated!");
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }
    if (IsKeyPressed(KEY_L)) {
        clusterManager.SpawnCustomCluster(false, GetRandomValue(6, 12));
        LogTerminal("[MANUAL SPAWN] Forced UNARMED Cluster generated!");
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }
    if (IsKeyPressed(KEY_J)) {
        clusterManager.SpawnCustomCluster(GetRandomValue(0, 100) < 50, GetRandomValue(10, 14));
        LogTerminal("[MANUAL SPAWN] Heavy Threat Cluster generated!");
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }

    // Hotkeys for Vision Modes
    if (IsKeyPressed(KEY_T)) {
        ToggleThermalFLIR();
    }
    if (IsKeyPressed(KEY_N)) {
        ToggleNightVision();
    }

    // Picture-in-Picture (PiP) Hotkeys
    if (IsKeyPressed(KEY_F6) || IsKeyPressed(KEY_V)) {
        TogglePiPWindow();
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        CyclePiPTarget(-1);
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        CyclePiPTarget(1);
    }

    // Manual Defensive Flare Deployment Hotkey (X)
    if (IsKeyPressed(KEY_X)) {
        int targetCam = fleetCamera.GetActiveCameraMode();
        int targetDroneId = (targetCam >= 1 && targetCam <= 9) ? targetCam : fleetHUD.GetInspectorDroneId();

        Drone* targetDrone = nullptr;
        for (auto& d : drones) {
            if (d->id == targetDroneId && !d->isDestroyed && d->state != DroneState::IDLE_AT_BASE) {
                targetDrone = d.get();
                break;
            }
        }

        // Fallback to active PiP target or any active drone in flight
        if (!targetDrone) {
            int pipIdx = fleetHUD.GetEffectivePiPDroneIndex(drones);
            if (pipIdx >= 0 && pipIdx < (int)drones.size() && !drones[pipIdx]->isDestroyed) {
                targetDrone = drones[pipIdx].get();
            }
        }

        if (targetDrone) {
            FleetContext tempCtx{ drones, clusterManager, missileSystem, hangarBase, telemetry };
            targetDrone->DeployFlares(&tempCtx);
        }
    }

    float effectiveDt = isPaused ? 0.0f : dt * timeScale;
    if (effectiveDt <= 0.0f) return;

    // Create FleetContext
    FleetContext ctx{
        drones,
        clusterManager,
        missileSystem,
        hangarBase,
        telemetry
    };

    // 1. Update Enemy Clusters & Hostile Attacks
    clusterManager.Update(ctx, effectiveDt);

    // 2. Update Drones
    for (auto& d : drones) {
        d->Update(ctx, effectiveDt);
        hangarBase.UpdateBaseRefill(*d, ctx, effectiveDt);
    }

    // 3. Update Ballistic Missiles and FX
    missileSystem.Update(clusterManager, telemetry, effectiveDt);

    // 4. Update Telemetry counters
    UpdateTelemetry();
}

void DroneSystemManager::TogglePiPWindow() {
    fleetHUD.TogglePiPWindow();
    LogTerminal(fleetHUD.IsPiPEnabled() ? "[PICTURE-IN-PICTURE] Live FPV Nose Camera feed enabled." : "[PICTURE-IN-PICTURE] Live FPV Nose Camera feed disabled.");
    if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
}

void DroneSystemManager::CyclePiPTarget(int dir) {
    fleetHUD.CyclePiPTarget(dir);
    int target = fleetHUD.GetPiPTarget();
    if (target == 0) {
        LogTerminal("[PICTURE-IN-PICTURE] Target mode: AUTO PRIORITY LOCK (Combat / Threat).");
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "[PICTURE-IN-PICTURE] Locked to Drone #%d Nose Cam.", target);
        LogTerminal(buf);
    }
    if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
}

void DroneSystemManager::RenderPiPCam() {
    if (!fleetHUD.IsPiPEnabled() || drones.empty()) return;

    RenderTexture2D pipTex = fleetHUD.GetPiPRenderTexture();
    if (pipTex.id <= 0) return;

    int droneIdx = fleetHUD.GetEffectivePiPDroneIndex(drones);
    if (droneIdx < 0 || droneIdx >= (int)drones.size()) return;

    const auto& drone = *drones[droneIdx];

    // Calculate Nose Camera position and forward orientation
    Camera3D noseCam;
    noseCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
    noseCam.fovy = 70.0f;
    noseCam.projection = CAMERA_PERSPECTIVE;

    float yawRad = drone.yaw * DEG2RAD;
    float pitchRad = drone.pitch * DEG2RAD;

    Vector3 forward = Vector3{
        sinf(yawRad) * cosf(pitchRad),
        -sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad)
    };

    Vector3 noseOffset = Vector3{
        forward.x * 1.5f,
        0.35f + forward.y * 1.5f,
        forward.z * 1.5f
    };

    noseCam.position = Vector3Add(drone.position, noseOffset);
    noseCam.target = Vector3Add(noseCam.position, Vector3Scale(forward, 40.0f));

    // Render 3D Scene into PiP RenderTexture
    BeginTextureMode(pipTex);

    VisionMode vm = fleetHUD.GetVisionMode();
    if (vm == VisionMode::FLIR_THERMAL) {
        ClearBackground(Color{ 4, 8, 14, 255 });
    } else if (vm == VisionMode::NVG_GREEN) {
        ClearBackground(Color{ 2, 14, 6, 255 });
    } else {
        ClearBackground(Color{ 10, 16, 26, 255 });
    }

    BeginMode3D(noseCam);
    Draw3D();
    EndMode3D();

    EndTextureMode();
}

void DroneSystemManager::ToggleThermalFLIR() {
    fleetHUD.ToggleThermalFLIR();
    if (fleetHUD.GetVisionMode() == VisionMode::FLIR_THERMAL) {
        LogTerminal("[OPTICS SENSOR] FLIR Thermal LWIR Mode Activated (White-Hot).");
    } else {
        LogTerminal("[OPTICS SENSOR] Normal Daylight Camera Mode Restored.");
    }
    if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
}

void DroneSystemManager::ToggleNightVision() {
    fleetHUD.ToggleNightVision();
    if (fleetHUD.GetVisionMode() == VisionMode::NVG_GREEN) {
        LogTerminal("[OPTICS SENSOR] NVG Gen-3 Green Phosphor Mode Activated.");
    } else {
        LogTerminal("[OPTICS SENSOR] Normal Daylight Camera Mode Restored.");
    }
    if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
}

void DroneSystemManager::Draw3D() const {
    VisionMode vm = fleetHUD.GetVisionMode();
    hangarBase.Draw3D(drones, vm);
    clusterManager.Draw3D(drones, vm);
    missileSystem.Draw3D(vm);

    for (const auto& d : drones) {
        d->Draw3D(vm);
    }
}

void DroneSystemManager::Draw2DHUD() const {
    fleetHUD.Draw2DHUD(telemetry, drones, clusterManager.GetClusters(), fleetCamera, timeScale, isPaused);
}

