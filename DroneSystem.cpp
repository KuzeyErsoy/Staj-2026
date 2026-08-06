#include "DroneSystem.h"
#include "raymath.h"
#include "AudioManager.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

#ifndef CYAN
#define CYAN SKYBLUE
#endif

using namespace DroneFleet;

DroneSystemManager::DroneSystemManager()
    : nextClusterId(1), scanTimer(0.0f), patrolAngle(0.0f),
      activeCameraMode(0), freeCamPos((Vector3){ 0.0f, 45.0f, 75.0f }),
      freeCamYaw(0.0f), freeCamPitch(-0.4f),
      trackCamYaw(0.0f), trackCamPitch(0.3f),
      fontLoaded(false) {

    // Try loading system Arial font or fallback
    const char* fontPaths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "resources/fonts/arial.ttf",
        "../resources/fonts/arial.ttf"
    };

    for (const char* path : fontPaths) {
        if (FileExists(path)) {
            customFont = LoadFontEx(path, 36, NULL, 0);
            if (customFont.texture.id > 0) {
                GenTextureMipmaps(&customFont.texture);
                SetTextureFilter(customFont.texture, TEXTURE_FILTER_TRILINEAR);
                fontLoaded = true;
                break;
            }
        }
    }
}

DroneSystemManager::~DroneSystemManager() {
    if (fontLoaded) {
        UnloadFont(customFont);
    }
}

void DroneSystemManager::DrawCustomText(const char* text, float posX, float posY, float fontSize, Color color) const {
    if (fontLoaded) {
        DrawTextEx(customFont, text, (Vector2){ posX, posY }, fontSize, 1.0f, color);
    } else {
        DrawText(text, (int)posX, (int)posY, (int)fontSize, color);
    }
}

std::string DroneSystemManager::GetActiveCameraModeName() const {
    switch (activeCameraMode) {
        case 0: return "FREE COMMAND FLY CAM";
        case 1: return "TRACKING DRONE #1 [RECON 1]";
        case 2: return "TRACKING DRONE #2 [RECON 2]";
        case 3: return "TRACKING DRONE #3 [TASK FORCE 1]";
        case 4: return "TRACKING DRONE #4 [TASK FORCE 2]";
        case 5: return "TRACKING DRONE #5 [TASK FORCE 3]";
        case 6: return "TRACKING DRONE #6 [TASK FORCE 4]";
        case 7: return "TRACKING DRONE #7 [TASK FORCE 5]";
        case 8: return "TRACKING DRONE #8 [TASK FORCE 6]";
        case 9: return "TRACKING DRONE #9 [TASK FORCE 7]";
        case 10: return "TRACKING DRONE #10 [TASK FORCE 8]";
        case 11: return "TRACKING DRONE #11 [TASK FORCE 9]";
        case 12: return "TRACKING DRONE #12 [TASK FORCE 10]";
        case 13: return "TRACKING DRONE #13 [SUPPORT 1]";
        case 14: return "TRACKING DRONE #14 [SUPPORT 2]";
        default: return "FREE CAM";
    }
}

void DroneSystemManager::UpdateFleetCamera(Camera3D& camera, float dt) {
    // Camera mode toggles
    if (IsKeyPressed(KEY_TAB)) {
        activeCameraMode = (activeCameraMode + 1) % 15;
        char buf[128];
        snprintf(buf, sizeof(buf), "[COMMAND CAM] Switched view to: %s", GetActiveCameraModeName().c_str());
        LogTerminal(buf);
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }
    if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0) || IsKeyPressed(KEY_C)) {
        activeCameraMode = 0;
        LogTerminal("[COMMAND CAM] Switched view to FREE COMMAND FLY CAM.");
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }

    // Direct key shortcuts 1..9 for quick camera access
    if (IsKeyPressed(KEY_ONE))   { activeCameraMode = 1;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_TWO))   { activeCameraMode = 2;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_THREE)) { activeCameraMode = 3;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FOUR))  { activeCameraMode = 4;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FIVE))  { activeCameraMode = 5;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_SIX))   { activeCameraMode = 6;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_SEVEN)) { activeCameraMode = 7;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_EIGHT)) { activeCameraMode = 8;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_NINE))  { activeCameraMode = 9;  if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }

    if (activeCameraMode >= 1 && activeCameraMode <= 14) {
        inspectorDroneId = activeCameraMode;
    }

    if (activeCameraMode == 0) {
        // FREE COMMAND FLY CAM (WASD + Mouse + Space/Ctrl/Shift)
        Vector2 mouseDelta = GetMouseDelta();
        freeCamYaw -= mouseDelta.x * 0.003f;
        freeCamPitch += mouseDelta.y * 0.003f;
        if (freeCamPitch > 1.4f) freeCamPitch = 1.4f;
        if (freeCamPitch < -1.4f) freeCamPitch = -1.4f;

        float flySpeed = 50.0f * dt;
        if (IsKeyDown(KEY_LEFT_SHIFT)) flySpeed *= 2.5f;

        Vector3 forward = (Vector3){ sinf(freeCamYaw) * cosf(freeCamPitch), -sinf(freeCamPitch), cosf(freeCamYaw) * cosf(freeCamPitch) };
        Vector3 right = (Vector3){ cosf(freeCamYaw), 0.0f, -sinf(freeCamYaw) };

        if (IsKeyDown(KEY_W)) freeCamPos = Vector3Add(freeCamPos, Vector3Scale(forward, flySpeed));
        if (IsKeyDown(KEY_S)) freeCamPos = Vector3Subtract(freeCamPos, Vector3Scale(forward, flySpeed));

        if (IsKeyDown(KEY_D)) freeCamPos = Vector3Subtract(freeCamPos, Vector3Scale(right, flySpeed));
        if (IsKeyDown(KEY_A)) freeCamPos = Vector3Add(freeCamPos, Vector3Scale(right, flySpeed));

        if (IsKeyDown(KEY_SPACE)) freeCamPos.y += flySpeed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) freeCamPos.y -= flySpeed;

        camera.position = freeCamPos;
        camera.target = Vector3Add(freeCamPos, forward);
        camera.fovy = 60.0f;
    } else {
        int droneIdx = activeCameraMode - 1;
        if (droneIdx >= 0 && droneIdx < (int)drones.size()) {
            const auto& targetDrone = drones[droneIdx];

            Vector2 mouseDelta = GetMouseDelta();
            trackCamYaw -= mouseDelta.x * 0.003f;
            trackCamPitch += mouseDelta.y * 0.003f;
            if (trackCamPitch > 1.2f) trackCamPitch = 1.2f;
            if (trackCamPitch < -0.5f) trackCamPitch = -0.5f;

            float camDist = 16.0f;
            Vector3 orbitOffset = (Vector3){
                sinf(trackCamYaw) * cosf(trackCamPitch) * camDist,
                sinf(trackCamPitch) * camDist + 3.0f,
                cosf(trackCamYaw) * cosf(trackCamPitch) * camDist
            };

            Vector3 desiredCamPos = Vector3Add(targetDrone.position, orbitOffset);
            camera.position = Vector3Lerp(camera.position, desiredCamPos, dt * 7.0f);
            camera.target = Vector3Lerp(camera.target, (Vector3){ targetDrone.position.x, targetDrone.position.y + 1.2f, targetDrone.position.z }, dt * 10.0f);
            camera.fovy = 55.0f;
        }
    }
}

void DroneSystemManager::LogTerminal(const std::string& msg) {
    int totalSecs = (int)GetTime();
    int mins = (totalSecs / 60) % 60;
    int secs = totalSecs % 60;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "[%02d:%02d] ", mins, secs);

    telemetry.terminalLogs.push_back(std::string(timeStr) + msg);
    if (telemetry.terminalLogs.size() > 20) {
        telemetry.terminalLogs.erase(telemetry.terminalLogs.begin());
    }
}

void DroneSystemManager::Init() {
    base.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    base.radius = 25.0f;
    base.fuelSupply = 9999.0f;
    base.ammoSupply = 9999.0f;

    drones.clear();

    // 1. RECON #1 & #2 (Bays #1, #2) — 2x Fuel = 200.0f, 0 Ammo
    Drone r1; r1.id = 1; r1.role = DroneRole::RECON; r1.state = DroneState::IDLE_AT_BASE;
    r1.baseDockPos = (Vector3){ -14.0f, 0.6f, -18.0f }; r1.position = r1.baseDockPos; r1.targetPosition = r1.position;
    r1.velocity = (Vector3){ 0, 0, 0 }; r1.maxFuel = 200.0f; r1.fuel = 200.0f; r1.maxAmmo = 0.0f; r1.ammo = 0.0f;
    r1.reconSectorIndex = 0; r1.LogAction("Docked at Hangar Helipad Bay #1 (2x Fuel Tank)"); drones.push_back(r1);

    Drone r2; r2.id = 2; r2.role = DroneRole::RECON; r2.state = DroneState::IDLE_AT_BASE;
    r2.baseDockPos = (Vector3){ -14.0f, 0.6f, -12.0f }; r2.position = r2.baseDockPos; r2.targetPosition = r2.position;
    r2.velocity = (Vector3){ 0, 0, 0 }; r2.maxFuel = 200.0f; r2.fuel = 200.0f; r2.maxAmmo = 0.0f; r2.ammo = 0.0f;
    r2.reconSectorIndex = 8; r2.LogAction("Docked at Hangar Helipad Bay #2 (2x Fuel Tank)"); drones.push_back(r2);

    // 2. TASK FORCE #1 .. #5 (Row A, Bays #3..#7) — 1x Fuel = 100.0f, 100 Ammo
    for (int i = 0; i < 5; i++) {
        Drone tf; tf.id = 3 + i; tf.role = DroneRole::TASK_FORCE; tf.state = DroneState::IDLE_AT_BASE;
        tf.baseDockPos = (Vector3){ -14.0f, 0.6f, -6.0f + (float)i * 6.0f }; tf.position = tf.baseDockPos; tf.targetPosition = tf.position;
        tf.velocity = (Vector3){ 0, 0, 0 }; tf.maxFuel = 100.0f; tf.fuel = 100.0f; tf.maxAmmo = 100.0f; tf.ammo = 100.0f;
        tf.LogAction("Docked at Hangar Helipad Bay #" + std::to_string(tf.id)); drones.push_back(tf);
    }

    // 3. TASK FORCE #6 .. #10 (Row B, Bays #8..#12) — 1x Fuel = 100.0f, 100 Ammo
    for (int i = 0; i < 5; i++) {
        Drone tf; tf.id = 8 + i; tf.role = DroneRole::TASK_FORCE; tf.state = DroneState::IDLE_AT_BASE;
        tf.baseDockPos = (Vector3){ 14.0f, 0.6f, -18.0f + (float)i * 6.0f }; tf.position = tf.baseDockPos; tf.targetPosition = tf.position;
        tf.velocity = (Vector3){ 0, 0, 0 }; tf.maxFuel = 100.0f; tf.fuel = 100.0f; tf.maxAmmo = 100.0f; tf.ammo = 100.0f;
        tf.LogAction("Docked at Hangar Helipad Bay #" + std::to_string(tf.id)); drones.push_back(tf);
    }

    // 4. SUPPORT #1 & #2 (Row B, Bays #13, #14) — 4x Fuel = 400.0f, Cargo = 600 Fuel / 400 Ammo
    for (int i = 0; i < 2; i++) {
        Drone s; s.id = 13 + i; s.role = DroneRole::SUPPORT; s.state = DroneState::IDLE_AT_BASE;
        s.baseDockPos = (Vector3){ 14.0f, 0.6f, 12.0f + (float)i * 6.0f }; s.position = s.baseDockPos; s.targetPosition = s.position;
        s.velocity = (Vector3){ 0, 0, 0 }; s.maxFuel = 400.0f; s.fuel = 400.0f; s.maxAmmo = 0.0f; s.ammo = 0.0f;
        s.cargoFuel = 600.0f; s.cargoAmmo = 400.0f;
        s.LogAction("Docked at Hangar Helipad Bay #" + std::to_string(s.id) + " (4x Heavy Fuel Tank)"); drones.push_back(s);
    }

    clusters.clear();
    nextClusterId = 1;
    telemetry.terminalLogs.clear();
    telemetry.eliminatedTargetsCount = 0;

    LogTerminal("[TACTICAL COMMAND] 14-Drone Military Autonomous Fleet Operational.");
    LogTerminal("[HANGAR BASE] 3D Military Hangar active at (0,0,0) with 14 Helipads.");

    SpawnCluster();
}

void DroneSystemManager::SpawnCluster() {
    EnemyCluster cluster;
    cluster.id = nextClusterId++;
    
    float rx = (float)GetRandomValue(-370, 370);
    float rz = (float)GetRandomValue(-370, 370);
    if (fabsf(rx) < 50.0f && fabsf(rz) < 50.0f) {
        rx = 200.0f;
        rz = 200.0f;
    }
    cluster.centerPosition = (Vector3){ rx, 1.0f, rz };

    int mag = GetRandomValue(3, 12);
    cluster.initialMagnitude = mag;
    cluster.remainingMagnitude = mag;
    cluster.status = ClusterStatus::UNDETECTED;

    for (int i = 0; i < mag; i++) {
        Enemy e;
        e.id = i + 1;
        float offsetX = (float)GetRandomValue(-14, 14);
        float offsetZ = (float)GetRandomValue(-14, 14);
        e.position = (Vector3){ rx + offsetX, 1.0f, rz + offsetZ };
        e.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        e.hp = 100.0f;
        e.maxHp = 100.0f;
        e.isAlive = true;
        e.panicTimer = 0.0f;
        e.fleeTarget = e.position;
        cluster.enemies.push_back(e);
    }

    clusters.push_back(cluster);

    char buf[128];
    snprintf(buf, sizeof(buf), "[INTEL] Unknown Hostile Cluster #%d spawned in grid sector. Recon units searching...", cluster.id);
    LogTerminal(buf);
}

void DroneSystemManager::ApplyFlankingAndSeparation(Drone& drone, float dt) {
    for (auto& other : drones) {
        if (other.id == drone.id) continue;
        float dist = Vector3Distance(drone.position, other.position);
        float minDist = (drone.role == DroneRole::SUPPORT && other.role == DroneRole::TASK_FORCE && drone.state == DroneState::RESUPPLYING_IN_AIR) ? 4.5f : 9.0f;

        if (dist < minDist && dist > 0.05f) {
            Vector3 pushDir = Vector3Normalize(Vector3Subtract(drone.position, other.position));
            float pushForce = (minDist - dist) * 10.0f;
            drone.velocity = Vector3Add(drone.velocity, Vector3Scale(pushDir, pushForce * dt));
        }
    }
}

// --- FIX 1: FIXED RENDEZVOUS POINT & EXCLUSIVE 1-SUPPORT ASSIGNMENT FOR RECON ---
void DroneSystemManager::UpdateRecon(Drone& drone, float dt) {
    if (drone.state == DroneState::RETURNING_TO_BASE) {
        drone.targetPosition = drone.baseDockPos;
        return;
    }

    if (drone.state == DroneState::IDLE_AT_BASE) {
        if (drone.fuel >= 195.0f) {
            drone.state = DroneState::PATROL;
            drone.hasRendezvous = false;
            drone.assignedSupportId = -1;
            drone.LogAction("Launched from Hangar on Grid Sector Sweep.");
            LogTerminal("[RECON #" + std::to_string(drone.id) + "] Launched from Hangar on Grid Sector Sweep.");
        }
        return;
    }

    drone.fuel -= dt * 0.8f;

    // Fuel < 25% (50 out of 200 maxFuel) -> Request 1 Support & calculate FIXED Rendezvous Point ONCE!
    if (drone.fuel <= 50.0f && drone.state != DroneState::RESUPPLYING_IN_AIR && drone.state != DroneState::REQUESTING_SUPPORT && drone.state != DroneState::RETURNING_TO_BASE) {
        Drone* selectedSupport = nullptr;
        for (auto& s : drones) {
            if (s.role == DroneRole::SUPPORT && s.servicingReconId == -1 && s.cargoFuel > 50.0f && s.state != DroneState::RETURNING_TO_BASE) {
                selectedSupport = &s;
                break;
            }
        }

        if (selectedSupport != nullptr) {
            // CALCULATE FIXED RENDEZVOUS POINT ONCE! NEVER RECALCULATE OR MOVE IT WHILE ACTIVE!
            drone.rendezvousPos = (Vector3){
                (drone.position.x + selectedSupport->position.x) * 0.5f,
                20.0f,
                (drone.position.z + selectedSupport->position.z) * 0.5f
            };
            drone.hasRendezvous = true;
            drone.assignedSupportId = selectedSupport->id;
            selectedSupport->servicingReconId = drone.id;
            drone.state = DroneState::REQUESTING_SUPPORT;
            drone.targetPosition = drone.rendezvousPos; // Recon flies straight to fixed rendezvous point & hovers!

            char buf[180];
            snprintf(buf, sizeof(buf), "[RECON #%d] Signal: Fuel <25%% (%.0f/200). Fixed Rendezvous set at (%.0f, %.0f). Assigned Support #%d.",
                     drone.id, drone.fuel, drone.rendezvousPos.x, drone.rendezvousPos.z, selectedSupport->id);
            LogTerminal(buf);
            drone.LogAction("Fixed Rendezvous at (" + std::to_string((int)drone.rendezvousPos.x) + ", " + std::to_string((int)drone.rendezvousPos.z) + ") assigned to Support #" + std::to_string(selectedSupport->id));
        } else {
            // No Support available -> Emergency RTB
            drone.state = DroneState::RETURNING_TO_BASE;
            drone.targetPosition = drone.baseDockPos;
            drone.LogAction("No Support available. Emergency RTB to Hangar.");
            LogTerminal("[RECON #" + std::to_string(drone.id) + "] No Support available. RTB to Hangar.");
            return;
        }
    }

    // While waiting for or being refueled at Rendezvous Point: HOVER AT FIXED RENDEZVOUS POS!
    if (drone.state == DroneState::REQUESTING_SUPPORT || drone.state == DroneState::RESUPPLYING_IN_AIR) {
        if (drone.hasRendezvous) {
            drone.targetPosition = drone.rendezvousPos;
        }

        // CRITICAL RULE: Resupply operation is NOT finished until Recon fuel is 100% FULL (200/200)!
        if (drone.fuel >= drone.maxFuel - 2.0f) {
            drone.fuel = drone.maxFuel;
            drone.hasRendezvous = false;
            if (drone.assignedSupportId != -1) {
                drones[drone.assignedSupportId - 1].servicingReconId = -1;
                drone.assignedSupportId = -1;
            }
            drone.state = DroneState::PATROL;
            drone.LogAction("100% Refueled mid-air! Resuming Grid Sector Sweep.");
            LogTerminal("[RECON #" + std::to_string(drone.id) + "] 100% Refueled mid-air! Resuming Grid Sector Sweep.");
        }
        return;
    }

    // Check if any undetected cluster is within Recon's 140m radar sensor radius!
    EnemyCluster* detectedCluster = nullptr;
    for (auto& c : clusters) {
        if (c.status == ClusterStatus::UNDETECTED || c.status == ClusterStatus::REQUIRES_RECON) {
            float dist = Vector3Distance(drone.position, c.centerPosition);
            if (dist <= 140.0f) {
                detectedCluster = &c;
                break;
            }
        }
    }

    if (detectedCluster != nullptr) {
        drone.targetPosition = (Vector3){ detectedCluster->centerPosition.x, 18.0f, detectedCluster->centerPosition.z };
        float dist = Vector3Distance(drone.position, detectedCluster->centerPosition);

        if (dist < 30.0f) {
            drone.state = DroneState::SCANNING;
            scanTimer += dt;
            if (scanTimer >= 1.8f) {
                detectedCluster->status = ClusterStatus::REPORTED;
                scanTimer = 0.0f;
                char buf[160];
                snprintf(buf, sizeof(buf), "[RECON #%d] Target Cluster #%d VERIFIED at (%.0f, %.0f) - Est. Magnitude: %d Units!",
                         drone.id, detectedCluster->id, detectedCluster->centerPosition.x, detectedCluster->centerPosition.z, detectedCluster->remainingMagnitude);
                LogTerminal(buf);
                drone.LogAction("Verified Hostile Cluster #" + std::to_string(detectedCluster->id) + " (Mag: " + std::to_string(detectedCluster->remainingMagnitude) + ")");
                if (IsAudioDeviceReady()) AudioManager::Instance().PlayCollect();
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
        }
    } else {
        // Systematic Grid Sector Sweep (16 sectors across [-360, 360])
        drone.state = DroneState::PATROL;

        Vector3 gridWaypoints[16] = {
            { -270.0f, 22.0f, -270.0f }, { -90.0f, 22.0f, -270.0f }, { 90.0f, 22.0f, -270.0f }, { 270.0f, 22.0f, -270.0f },
            { 270.0f, 22.0f, -90.0f },  { 90.0f, 22.0f, -90.0f },   { -90.0f, 22.0f, -90.0f }, { -270.0f, 22.0f, -90.0f },
            { -270.0f, 22.0f, 90.0f },  { -90.0f, 22.0f, 90.0f },   { 90.0f, 22.0f, 90.0f },   { 270.0f, 22.0f, 90.0f },
            { 270.0f, 22.0f, 270.0f },  { 90.0f, 22.0f, 270.0f },   { -90.0f, 22.0f, 270.0f }, { -270.0f, 22.0f, 270.0f }
        };

        Vector3 wp = gridWaypoints[drone.reconSectorIndex % 16];
        drone.targetPosition = wp;

        if (Vector3Distance(drone.position, wp) < 25.0f) {
            drone.reconSectorIndex = (drone.reconSectorIndex + 1) % 16;
            drone.LogAction("Sweeping Sector #" + std::to_string(drone.reconSectorIndex + 1));
        }
    }
}

// --- FIX 2: TASK FORCE MAGNITUDE DISPATCH & MID-AIR RETASKING (>50% FUEL & AMMO) ---
void DroneSystemManager::UpdateTaskForce(Drone& drone, float dt) {
    if (drone.state == DroneState::RETURNING_TO_BASE) {
        drone.targetPosition = drone.baseDockPos;
        return;
    }

    if (drone.state != DroneState::IDLE_AT_BASE) {
        drone.fuel -= dt * 1.0f;
    }

    if (drone.fuel <= 20.0f || drone.ammo <= 15.0f) {
        if (drone.state != DroneState::REQUESTING_SUPPORT && drone.state != DroneState::RESUPPLYING_IN_AIR && drone.state != DroneState::RETURNING_TO_BASE) {
            drone.state = DroneState::REQUESTING_SUPPORT;
            drone.LogAction("Low Resource! Requesting Aerial Support (F:" + std::to_string((int)drone.fuel) + "%, A:" + std::to_string((int)drone.ammo) + "%)");
        }
    }

    if (drone.fuel <= 8.0f || (drone.ammo <= 0.0f && drone.state != DroneState::RESUPPLYING_IN_AIR)) {
        if (drone.state != DroneState::RETURNING_TO_BASE) {
            drone.state = DroneState::RETURNING_TO_BASE;
            drone.targetPosition = drone.baseDockPos;
            drone.assignedClusterId = -1;
            drone.LogAction("Critical Resource! Emergency RTB to Hangar.");
            LogTerminal("[TASK FORCE #" + std::to_string(drone.id) + "] Emergency RTB to Hangar.");
        }
        return;
    }

    EnemyCluster* targetCluster = nullptr;
    for (auto& c : clusters) {
        if (c.id == drone.assignedClusterId && c.status != ClusterStatus::CLEARED) {
            targetCluster = &c;
            break;
        }
    }

    // MAGNITUDE DISPATCH & MID-AIR RETASKING PROTOCOL
    if (!targetCluster) {
        for (auto& c : clusters) {
            if ((c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) && c.remainingMagnitude > 0) {
                int currentlyAssigned = 0;
                for (const auto& otherTF : drones) {
                    if (otherTF.role == DroneRole::TASK_FORCE && otherTF.assignedClusterId == c.id) {
                        currentlyAssigned++;
                    }
                }

                int requiredDispatch = std::min(c.remainingMagnitude, 10);
                if (currentlyAssigned < requiredDispatch) {
                    // Check if this Task Force drone is eligible for dispatch OR mid-air retasking:
                    // 1. Must NOT be currently engaging in laser combat (state != ENGAGING)
                    // 2. Must have Fuel > 50% and Ammo > 50%!
                    bool isEligibleForDispatch = (drone.state == DroneState::IDLE_AT_BASE);
                    bool isEligibleForMidAirRetask = (drone.state != DroneState::ENGAGING && drone.fuel > 50.0f && drone.ammo > 50.0f);

                    if (isEligibleForDispatch || isEligibleForMidAirRetask) {
                        targetCluster = &c;
                        drone.assignedClusterId = c.id;
                        drone.state = DroneState::MOVING_TO_TARGET;
                        c.status = ClusterStatus::ENGAGED;

                        char buf[160];
                        if (isEligibleForMidAirRetask && isEligibleForDispatch == false) {
                            snprintf(buf, sizeof(buf), "[MID-AIR RETASK] Task Force #%d (F:%.0f%%, A:%.0f%%) retasked mid-air to Cluster #%d!",
                                     drone.id, drone.fuel, drone.ammo, c.id);
                            drone.LogAction("Retasked mid-air to Cluster #" + std::to_string(c.id) + " (Mag: " + std::to_string(c.remainingMagnitude) + ")");
                        } else {
                            snprintf(buf, sizeof(buf), "[DISPATCH] Task Force #%d launched from Hangar (Cluster #%d Mag: %d, Dispatched: %d/%d)",
                                     drone.id, c.id, c.remainingMagnitude, currentlyAssigned + 1, requiredDispatch);
                            drone.LogAction("Dispatched to Cluster #" + std::to_string(c.id) + " (Mag: " + std::to_string(c.remainingMagnitude) + ")");
                        }
                        LogTerminal(buf);
                        break;
                    }
                }
            }
        }
    }

    if (targetCluster && targetCluster->remainingMagnitude > 0) {
        float tfIdx = (float)(drone.id - 3); // 0 .. 9
        float baseAngle = atan2f(targetCluster->centerPosition.x - base.position.x, targetCluster->centerPosition.z - base.position.z);
        float offsetAngle = baseAngle + (tfIdx - 4.5f) * 0.35f;
        float standoffRadius = 32.0f + (float)((int)tfIdx % 3) * 6.0f;
        float standoffHeight = 12.0f + (float)((int)tfIdx % 4) * 4.0f;

        drone.targetPosition = (Vector3){
            targetCluster->centerPosition.x + sinf(offsetAngle) * standoffRadius,
            standoffHeight,
            targetCluster->centerPosition.z + cosf(offsetAngle) * standoffRadius
        };

        float dist = Vector3Distance(drone.position, targetCluster->centerPosition);

        if (dist < 110.0f && drone.ammo > 0.0f) {
            drone.state = DroneState::ENGAGING;
            drone.ammo -= dt * 4.0f;

            Enemy* closestEnemy = nullptr;
            float minDist = 9999.0f;
            for (auto& e : targetCluster->enemies) {
                if (e.isAlive) {
                    float ed = Vector3Distance(drone.position, e.position);
                    if (ed < minDist) {
                        minDist = ed;
                        closestEnemy = &e;
                    }
                }
            }

            if (closestEnemy) {
                drone.isFiringLaser = true;
                drone.laserTargetPos = closestEnemy->position;
                closestEnemy->hp -= dt * 40.0f;

                closestEnemy->panicTimer = 3.0f;
                closestEnemy->fleeTarget = drone.position;

                for (auto& sq : targetCluster->enemies) {
                    if (sq.isAlive && Vector3Distance(sq.position, closestEnemy->position) < 18.0f) {
                        sq.panicTimer = 2.5f;
                        sq.fleeTarget = drone.position;
                    }
                }

                if (closestEnemy->hp <= 0.0f) {
                    closestEnemy->hp = 0.0f;
                    closestEnemy->isAlive = false;
                    targetCluster->remainingMagnitude--;
                    telemetry.eliminatedTargetsCount++;

                    char buf[128];
                    snprintf(buf, sizeof(buf), "[TASK FORCE #%d] Target Neutralized! Cluster #%d Remaining Hostiles: %d",
                             drone.id, targetCluster->id, targetCluster->remainingMagnitude);
                    LogTerminal(buf);
                    drone.LogAction("Neutralized hostile in Cluster #" + std::to_string(targetCluster->id));

                    if (IsAudioDeviceReady()) AudioManager::Instance().PlayHurt();
                }
            } else {
                drone.isFiringLaser = false;
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
            drone.isFiringLaser = false;
        }
    } else {
        drone.isFiringLaser = false;
        if (drone.state != DroneState::REQUESTING_SUPPORT && drone.state != DroneState::RESUPPLYING_IN_AIR) {
            if (drone.state != DroneState::IDLE_AT_BASE) {
                drone.state = DroneState::RETURNING_TO_BASE;
                drone.targetPosition = drone.baseDockPos;
            }
        }
    }
}

// --- FIX 1: SUPPORT RENDEZVOUS DISPATCH & 100% RECON REFUELING ---
void DroneSystemManager::UpdateSupport(Drone& drone, float dt) {
    if (drone.state == DroneState::RETURNING_TO_BASE) {
        drone.targetPosition = drone.baseDockPos;
        return;
    }

    if (drone.state != DroneState::IDLE_AT_BASE) {
        drone.fuel -= dt * 0.7f;
    }

    // Check if this Support drone is assigned to service a Recon drone
    if (drone.servicingReconId != -1) {
        int rIdx = drone.servicingReconId - 1;
        if (rIdx >= 0 && rIdx < (int)drones.size()) {
            auto& reconDrone = drones[rIdx];

            if (reconDrone.state == DroneState::REQUESTING_SUPPORT || reconDrone.state == DroneState::RESUPPLYING_IN_AIR) {
                if (drone.state == DroneState::IDLE_AT_BASE) {
                    drone.state = DroneState::MOVING_TO_TARGET;
                    drone.LogAction("Launched from Hangar to Rendezvous with Recon #" + std::to_string(reconDrone.id));
                    LogTerminal("[SUPPORT #" + std::to_string(drone.id) + "] Launched to Rendezvous with Recon #" + std::to_string(reconDrone.id));
                }

                // Fly directly to FIXED Rendezvous Point!
                Vector3 targetDest = reconDrone.hasRendezvous ? reconDrone.rendezvousPos : reconDrone.position;
                drone.targetPosition = (Vector3){ targetDest.x, 20.0f, targetDest.z };

                float dist = Vector3Distance(drone.position, reconDrone.position);

                if (dist < 35.0f) {
                    drone.state = DroneState::RESUPPLYING_IN_AIR;
                    drone.isResupplyBeamActive = true;
                    drone.resupplyTargetPos = reconDrone.position;
                    reconDrone.state = DroneState::RESUPPLYING_IN_AIR;

                    float transferFuel = dt * 45.0f; // Fast mid-air fuel transfer
                    if (drone.cargoFuel >= transferFuel && reconDrone.fuel < reconDrone.maxFuel) {
                        drone.cargoFuel -= transferFuel;
                        reconDrone.fuel = std::min(reconDrone.maxFuel, reconDrone.fuel + transferFuel);
                    }

                    // Complete ONLY when Recon is 100% FULL (200/200)!
                    if (reconDrone.fuel >= reconDrone.maxFuel - 2.0f) {
                        reconDrone.fuel = reconDrone.maxFuel;
                        reconDrone.hasRendezvous = false;
                        reconDrone.assignedSupportId = -1;
                        reconDrone.state = DroneState::PATROL;

                        drone.servicingReconId = -1;
                        drone.isResupplyBeamActive = false;
                        drone.state = DroneState::RETURNING_TO_BASE;
                        drone.targetPosition = drone.baseDockPos;

                        char buf[140];
                        snprintf(buf, sizeof(buf), "[SUPPORT #%d] Recon #%d 100%% Refueled! Returning to Hangar.", drone.id, reconDrone.id);
                        LogTerminal(buf);
                        drone.LogAction("Completed 100% refuel for Recon #" + std::to_string(reconDrone.id) + ". RTB to Hangar.");
                    }
                } else {
                    drone.state = DroneState::MOVING_TO_TARGET;
                    drone.isResupplyBeamActive = false;
                }
                return;
            }
        }
    }

    // Task Force resupply handling
    Drone* needyTF = nullptr;
    float lowestRes = 999.0f;

    for (auto& d : drones) {
        if (d.role == DroneRole::TASK_FORCE && (d.state == DroneState::REQUESTING_SUPPORT || d.fuel < 35.0f || d.ammo < 25.0f)) {
            float priorityVal = d.fuel + d.ammo;
            if (priorityVal < lowestRes) {
                lowestRes = priorityVal;
                needyTF = &d;
            }
        }
    }

    if (needyTF && (drone.cargoFuel > 0.0f || drone.cargoAmmo > 0.0f)) {
        if (drone.state == DroneState::IDLE_AT_BASE) {
            drone.state = DroneState::MOVING_TO_TARGET;
            drone.LogAction("Launched from Hangar to resupply Task Force #" + std::to_string(needyTF->id));
            LogTerminal("[SUPPORT #" + std::to_string(drone.id) + "] Launched to resupply Task Force #" + std::to_string(needyTF->id));
        }

        float headingRad = needyTF->yaw * DEG2RAD;
        drone.targetPosition = (Vector3){
            needyTF->position.x - sinf(headingRad) * 5.0f,
            needyTF->position.y + 6.0f,
            needyTF->position.z - cosf(headingRad) * 5.0f
        };

        float dist = Vector3Distance(drone.position, needyTF->position);

        if (dist < 35.0f) {
            drone.state = DroneState::RESUPPLYING_IN_AIR;
            drone.isResupplyBeamActive = true;
            drone.resupplyTargetPos = needyTF->position;

            float transferFuel = dt * 30.0f;
            float transferAmmo = dt * 40.0f;

            if (drone.cargoFuel >= transferFuel && needyTF->fuel < needyTF->maxFuel) {
                drone.cargoFuel -= transferFuel;
                needyTF->fuel = std::min(needyTF->maxFuel, needyTF->fuel + transferFuel);
            }
            if (drone.cargoAmmo >= transferAmmo && needyTF->ammo < needyTF->maxAmmo) {
                drone.cargoAmmo -= transferAmmo;
                needyTF->ammo = std::min(needyTF->maxAmmo, needyTF->ammo + transferAmmo);
            }

            needyTF->state = DroneState::RESUPPLYING_IN_AIR;

            if (needyTF->fuel >= 95.0f && needyTF->ammo >= 95.0f) {
                needyTF->state = DroneState::MOVING_TO_TARGET;
                char buf[140];
                snprintf(buf, sizeof(buf), "[SUPPORT #%d] Mid-air Resupply for Task Force #%d Completed!", drone.id, needyTF->id);
                LogTerminal(buf);
                drone.LogAction("Completed mid-air resupply for Task Force #" + std::to_string(needyTF->id));
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
            drone.isResupplyBeamActive = false;
        }
    } else {
        drone.isResupplyBeamActive = false;
        if (drone.cargoFuel <= 20.0f || drone.fuel <= 20.0f) {
            if (drone.state != DroneState::RETURNING_TO_BASE) {
                drone.state = DroneState::RETURNING_TO_BASE;
                drone.targetPosition = drone.baseDockPos;
                drone.LogAction("Cargo/Fuel depleted. Returning to Hangar.");
            }
        } else {
            if (drone.state != DroneState::IDLE_AT_BASE && needyTF == nullptr) {
                drone.state = DroneState::RETURNING_TO_BASE;
                drone.targetPosition = drone.baseDockPos;
            }
        }
    }
}

void DroneSystemManager::UpdateBaseRefill(Drone& drone, float dt) {
    float distToBase = Vector3Distance(drone.position, drone.baseDockPos);

    if (distToBase < 8.0f) {
        if (drone.state == DroneState::RETURNING_TO_BASE || drone.state == DroneState::IDLE_AT_BASE) {
            float refillRate = dt * 50.0f;
            drone.fuel = std::min(drone.maxFuel, drone.fuel + refillRate);
            if (drone.maxAmmo > 0.0f) {
                drone.ammo = std::min(drone.maxAmmo, drone.ammo + refillRate);
            }

            if (drone.role == DroneRole::SUPPORT) {
                drone.cargoFuel = std::min(600.0f, drone.cargoFuel + dt * 100.0f);
                drone.cargoAmmo = std::min(400.0f, drone.cargoAmmo + dt * 80.0f);
            }

            if (drone.fuel >= drone.maxFuel - 1.0f && (drone.maxAmmo <= 0.0f || drone.ammo >= drone.maxAmmo - 1.0f)) {
                if (drone.state == DroneState::RETURNING_TO_BASE) {
                    drone.state = DroneState::IDLE_AT_BASE;
                    drone.position = drone.baseDockPos;
                    drone.velocity = (Vector3){ 0, 0, 0 };
                    drone.hasRendezvous = false;
                    drone.assignedSupportId = -1;
                    drone.servicingReconId = -1;
                    drone.LogAction("Safely docked in Hangar. Refueled & rearmed 100%.");
                    char buf[128];
                    snprintf(buf, sizeof(buf), "[HANGAR BASE] Drone #%d 100%% refueled & rearmed on Helipad. Ready.", drone.id);
                    LogTerminal(buf);
                }
            }
        }
    }
}

void DroneSystemManager::UpdateTelemetry() {
    telemetry.activeInMission = 0;
    telemetry.dockedAtBase = 0;
    telemetry.knownTargetsMagnitude = 0;
    telemetry.activeClustersCount = 0;

    for (const auto& d : drones) {
        if (d.state == DroneState::IDLE_AT_BASE) telemetry.dockedAtBase++;
        else telemetry.activeInMission++;
    }

    for (const auto& c : clusters) {
        if (c.status != ClusterStatus::CLEARED) {
            telemetry.activeClustersCount++;
            if (c.status != ClusterStatus::UNDETECTED) {
                telemetry.knownTargetsMagnitude += c.remainingMagnitude;
            }
        }
    }
}

void DroneSystemManager::Update(float dt) {
    // 1. Spawner & Cluster Check
    bool hasActiveCluster = false;
    for (auto& c : clusters) {
        if (c.status != ClusterStatus::CLEARED) {
            if (c.remainingMagnitude <= 0) {
                c.status = ClusterStatus::CLEARED;
                char buf[128];
                snprintf(buf, sizeof(buf), "[TACTICAL COMMAND] Hostile Cluster #%d CLEARED! Relocating spawner...", c.id);
                LogTerminal(buf);
                SpawnCluster();
            } else {
                hasActiveCluster = true;
            }
        }
    }

    if (!hasActiveCluster) {
        SpawnCluster();
    }

    // 2. Hostile Enemy Panic & Evasion Scatter Behavior
    for (auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;
        for (auto& e : c.enemies) {
            if (!e.isAlive) continue;

            if (e.panicTimer > 0.0f) {
                e.panicTimer -= dt;

                Vector3 fleeDir = Vector3Subtract(e.position, e.fleeTarget);
                float dist = Vector3Length(fleeDir);

                if (dist < 1.0f || (GetRandomValue(0, 100) < 4)) {
                    float rx = (float)GetRandomValue(-22, 22);
                    float rz = (float)GetRandomValue(-22, 22);
                    e.fleeTarget = (Vector3){ c.centerPosition.x + rx, 1.0f, c.centerPosition.z + rz };
                } else {
                    Vector3 normDir = Vector3Scale(fleeDir, 1.0f / dist);
                    float speed = 7.5f;
                    e.position = Vector3Add(e.position, Vector3Scale(normDir, speed * dt));

                    float distFromCenter = Vector3Distance(e.position, c.centerPosition);
                    if (distFromCenter > 25.0f) {
                        Vector3 backToCenter = Vector3Normalize(Vector3Subtract(c.centerPosition, e.position));
                        e.position = Vector3Add(e.position, Vector3Scale(backToCenter, speed * dt * 1.5f));
                    }
                }
            }
        }
    }

    // 3. Drone Movement & Tactical Formation Physics
    for (auto& d : drones) {
        d.rotorAngle += dt * 720.0f;
        if (d.rotorAngle > 360.0f) d.rotorAngle -= 360.0f;

        switch (d.role) {
            case DroneRole::RECON:      UpdateRecon(d, dt); break;
            case DroneRole::TASK_FORCE: UpdateTaskForce(d, dt); break;
            case DroneRole::SUPPORT:    UpdateSupport(d, dt); break;
        }

        UpdateBaseRefill(d, dt);

        if (d.state == DroneState::IDLE_AT_BASE) {
            d.position = Vector3Lerp(d.position, d.baseDockPos, dt * 5.0f);
            d.velocity = (Vector3){ 0, 0, 0 };
            d.pitch = 0.0f;
            d.roll = 0.0f;
            continue;
        }

        float maxSpeed = (d.role == DroneRole::RECON) ? 16.0f : ((d.role == DroneRole::SUPPORT) ? 11.0f : 14.0f);

        Vector3 moveDir = Vector3Subtract(d.targetPosition, d.position);
        float dist = Vector3Length(moveDir);

        if (dist > 0.8f) {
            Vector3 normDir = Vector3Scale(moveDir, 1.0f / dist);
            Vector3 desiredVel = Vector3Scale(normDir, maxSpeed);

            d.velocity = Vector3Lerp(d.velocity, desiredVel, dt * 2.2f);
            d.position = Vector3Add(d.position, Vector3Scale(d.velocity, dt));

            d.yaw = atan2f(d.velocity.x, d.velocity.z) * RAD2DEG;
            float speedVal = Vector3Length(d.velocity);
            d.pitch = std::clamp(speedVal * 1.1f, -20.0f, 20.0f);
        } else {
            d.velocity = Vector3Lerp(d.velocity, (Vector3){ 0.0f, 0.0f, 0.0f }, dt * 3.5f);
            d.position = Vector3Add(d.position, Vector3Scale(d.velocity, dt));
            d.pitch = std::clamp(d.pitch * 0.9f, -2.0f, 2.0f);
        }

        ApplyFlankingAndSeparation(d, dt);
    }

    UpdateTelemetry();
}

void DroneSystemManager::DrawBase3D() const {
    DrawGrid(100, 10.0f);

    DrawCylinder(base.position, base.radius, base.radius, 0.2f, 32, (Color){ 0, 180, 255, 40 });
    DrawCircle3D(base.position, base.radius, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 0, 255, 255, 200 });

    Vector3 hPos = (Vector3){ 0.0f, 5.0f, 0.0f };
    DrawCubeWires(hPos, 36.0f, 10.0f, 44.0f, SKYBLUE);
    DrawCube((Vector3){ 0.0f, 10.1f, 0.0f }, 36.5f, 0.4f, 44.5f, (Color){ 20, 50, 80, 220 });

    float pX[4] = { -18.0f, 18.0f, -18.0f, 18.0f };
    float pZ[4] = { -22.0f, -22.0f, 22.0f, 22.0f };
    for (int i = 0; i < 4; i++) {
        DrawCylinder((Vector3){ pX[i], 2.5f, pZ[i] }, 0.8f, 0.8f, 5.0f, 8, DARKGRAY);
    }

    DrawCylinder((Vector3){ -20.0f, 3.0f, -10.0f }, 2.5f, 2.5f, 6.0f, 16, ORANGE);
    DrawCylinder((Vector3){ -20.0f, 3.0f, 10.0f }, 2.5f, 2.5f, 6.0f, 16, YELLOW);

    for (const auto& d : drones) {
        Color padColor = (d.role == DroneRole::RECON) ? SKYBLUE : ((d.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
        DrawCircle3D(d.baseDockPos, 2.2f, (Vector3){ 1, 0, 0 }, 90.0f, padColor);
        DrawCircle3D(d.baseDockPos, 1.1f, (Vector3){ 1, 0, 0 }, 90.0f, RAYWHITE);
    }
}

void DroneSystemManager::DrawClusters3D() const {
    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        Color ringColor = RED;
        if (c.status == ClusterStatus::UNDETECTED) ringColor = (Color){ 220, 220, 50, 180 };
        else if (c.status == ClusterStatus::REPORTED) ringColor = ORANGE;

        DrawCircle3D(c.centerPosition, 18.0f, (Vector3){ 1, 0, 0 }, 90.0f, ringColor);

        for (const auto& e : c.enemies) {
            if (!e.isAlive) continue;

            Color enemyColor = RED;
            if (e.panicTimer > 0.0f) enemyColor = (Color){ 255, 120, 0, 255 };

            DrawCube(e.position, 2.0f, 2.0f, 2.0f, enemyColor);
            DrawCubeWires(e.position, 2.1f, 2.1f, 2.1f, MAROON);

            if (e.panicTimer > 0.0f) {
                DrawCircle3D(e.position, 2.8f, (Vector3){ 1, 0, 0 }, 90.0f, YELLOW);
            }

            float hpPct = std::max(0.0f, e.hp / e.maxHp);
            DrawCube((Vector3){ e.position.x, e.position.y + 2.2f, e.position.z }, 2.0f, 0.3f, 0.3f, DARKGRAY);
            DrawCube((Vector3){ e.position.x - (1.0f - hpPct), e.position.y + 2.2f, e.position.z }, 2.0f * hpPct, 0.3f, 0.3f, GREEN);
        }
    }

    for (const auto& d : drones) {
        if (d.role == DroneRole::RECON && d.state != DroneState::IDLE_AT_BASE) {
            DrawCircle3D(d.position, 140.0f, (Vector3){ 1, 0, 0 }, 90.0f, Fade(SKYBLUE, 0.15f));
        }
        if (d.hasRendezvous) {
            DrawCircle3D(d.rendezvousPos, 12.0f, (Vector3){ 1, 0, 0 }, 90.0f, Fade(GOLD, 0.6f));
        }
    }
}

void DroneSystemManager::DrawDrone3D(const Drone& drone) const {
    Color bodyColor = WHITE;
    switch (drone.role) {
        case DroneRole::RECON:      bodyColor = SKYBLUE; break;
        case DroneRole::TASK_FORCE: bodyColor = ORANGE; break;
        case DroneRole::SUPPORT:    bodyColor = LIME; break;
    }

    rlPushMatrix();
    rlTranslatef(drone.position.x, drone.position.y, drone.position.z);
    rlRotatef(drone.yaw, 0.0f, 1.0f, 0.0f);
    rlRotatef(drone.pitch, 1.0f, 0.0f, 0.0f);

    DrawCube((Vector3){ 0, 0, 0 }, 1.8f, 0.6f, 1.8f, bodyColor);
    DrawCubeWires((Vector3){ 0, 0, 0 }, 1.85f, 0.65f, 1.85f, DARKGRAY);

    float rOffset = 1.3f;
    Vector3 rPositions[4] = {
        (Vector3){ rOffset, 0.3f, rOffset },
        (Vector3){ -rOffset, 0.3f, rOffset },
        (Vector3){ rOffset, 0.3f, -rOffset },
        (Vector3){ -rOffset, 0.3f, -rOffset }
    };

    for (int i = 0; i < 4; i++) {
        DrawCylinder(rPositions[i], 0.6f, 0.6f, 0.1f, 8, DARKGRAY);
    }

    rlPopMatrix();

    if (drone.isFiringLaser) {
        DrawLine3D(drone.position, drone.laserTargetPos, RED);
        DrawSphere(drone.laserTargetPos, 0.5f, RED);
    }

    if (drone.isResupplyBeamActive) {
        DrawLine3D(drone.position, drone.resupplyTargetPos, GREEN);
        DrawSphere(drone.resupplyTargetPos, 0.7f, LIME);
    }
}

void DroneSystemManager::Draw3D() const {
    DrawBase3D();
    DrawClusters3D();
    for (const auto& d : drones) {
        DrawDrone3D(d);
    }
}

void DroneSystemManager::DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen) const {
    DrawRectangle(mapX, mapY, mapW, mapH, (Color){ 3, 8, 16, 245 });
    DrawRectangleLines(mapX, mapY, mapW, mapH, SKYBLUE);
    DrawRectangleLines(mapX + 2, mapY + 2, mapW - 4, mapH - 4, Fade(SKYBLUE, 0.35f));

    DrawCustomText(fullscreen ? "TACTICAL RADAR ARENA MAP - 1000x1000m GRID (FULLSCREEN)" : "TACTICAL MAP",
                   mapX + 15, mapY + 10, fullscreen ? 18.0f : 14.0f, SKYBLUE);

    float scale = (float)mapW / 1000.0f;
    int centerX = mapX + mapW / 2;
    int centerY = mapY + mapH / 2;

    DrawCircleLines(centerX, centerY, (int)(200 * scale), Fade(SKYBLUE, 0.25f));
    DrawCircleLines(centerX, centerY, (int)(400 * scale), Fade(SKYBLUE, 0.25f));
    DrawLine(mapX, centerY, mapX + mapW, centerY, Fade(SKYBLUE, 0.2f));
    DrawLine(centerX, mapY, centerX, mapY + mapH, Fade(SKYBLUE, 0.2f));

    DrawRectangleLines(centerX - 6, centerY - 6, 12, 12, SKYBLUE);
    DrawCustomText("BASE", centerX - 12, centerY - 18, 10.0f, SKYBLUE);

    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        int cx = centerX + (int)(c.centerPosition.x * scale);
        int cy = centerY + (int)(c.centerPosition.z * scale);

        Color cCol = RED;
        if (c.status == ClusterStatus::UNDETECTED) cCol = YELLOW;
        else if (c.status == ClusterStatus::REPORTED) cCol = ORANGE;

        DrawCircle(cx, cy, 5, cCol);
        if (fullscreen) {
            DrawText(TextFormat("#%d (%d)", c.id, c.remainingMagnitude), cx + 7, cy - 5, 11, RAYWHITE);
        }
    }

    for (const auto& d : drones) {
        int dx = centerX + (int)(d.position.x * scale);
        int dy = centerY + (int)(d.position.z * scale);

        Color dCol = (d.role == DroneRole::RECON) ? SKYBLUE : ((d.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
        DrawCircle(dx, dy, (d.state == DroneState::IDLE_AT_BASE) ? 3 : 5, dCol);

        if (fullscreen) {
            DrawText(TextFormat("#%d", d.id), dx + 6, dy - 6, 11, dCol);
        }

        if (d.role == DroneRole::RECON && d.state != DroneState::IDLE_AT_BASE) {
            DrawCircleLines(dx, dy, (int)(140 * scale), Fade(SKYBLUE, 0.3f));
        }
        if (d.hasRendezvous) {
            int rX = centerX + (int)(d.rendezvousPos.x * scale);
            int rY = centerY + (int)(d.rendezvousPos.z * scale);
            DrawCircleLines(rX, rY, 6, GOLD);
        }
    }
}

void DroneSystemManager::DrawDroneInspector2D() const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int panelW = 420;
    int panelH = 500;
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    DrawRectangle(panelX, panelY, panelW, panelH, (Color){ 4, 12, 24, 252 });
    DrawRectangleLines(panelX, panelY, panelW, panelH, GOLD);
    DrawRectangleLines(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Fade(GOLD, 0.4f));

    DrawCustomText("INDIVIDUAL DRONE TELEMETRY INSPECTOR", panelX + 20, panelY + 15, 17.0f, GOLD);
    DrawLine(panelX + 20, panelY + 40, panelX + panelW - 20, panelY + 40, GOLD);

    int idx = inspectorDroneId - 1;
    if (idx >= 0 && idx < (int)drones.size()) {
        const auto& d = drones[idx];

        const char* roleStr = (d.role == DroneRole::RECON) ? "RECON SCOUT (2x Fuel)" : ((d.role == DroneRole::TASK_FORCE) ? "TASK FORCE INTERCEPTOR (1x Fuel)" : "SUPPORT LOGISTICS (4x Fuel)");
        Color roleCol = (d.role == DroneRole::RECON) ? SKYBLUE : ((d.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);

        DrawCustomText(TextFormat("DRONE #%d - %s", d.id, roleStr), panelX + 20, panelY + 50, 15.0f, roleCol);

        const char* stateStr = "IDLE AT BASE";
        switch (d.state) {
            case DroneState::PATROL: stateStr = "PATROL / GRID SWEEP"; break;
            case DroneState::SCANNING: stateStr = "SCANNING TARGET"; break;
            case DroneState::MOVING_TO_TARGET: stateStr = "MOVING TO TARGET"; break;
            case DroneState::ENGAGING: stateStr = "ENGAGING HOSTILES"; break;
            case DroneState::REQUESTING_SUPPORT: stateStr = "HOVERING AT FIXED RENDEZVOUS"; break;
            case DroneState::RESUPPLYING_IN_AIR: stateStr = "RESUPPLYING IN AIR"; break;
            case DroneState::RETURNING_TO_BASE: stateStr = "RETURNING TO HANGAR"; break;
            default: stateStr = "IDLE AT BASE"; break;
        }
        DrawCustomText(TextFormat("Status: %s", stateStr), panelX + 20, panelY + 75, 14.0f, RAYWHITE);

        DrawCustomText(TextFormat("Position: (%.1f, %.1f, %.1f)", d.position.x, d.position.y, d.position.z), panelX + 20, panelY + 98, 13.0f, LIGHTGRAY);
        DrawCustomText(TextFormat("Speed: %.1f m/s", Vector3Length(d.velocity)), panelX + 20, panelY + 118, 13.0f, LIGHTGRAY);

        DrawCustomText("FUEL CAPACITY:", panelX + 20, panelY + 145, 13.0f, YELLOW);
        DrawRectangle(panelX + 150, panelY + 147, 230, 14, DARKGRAY);
        DrawRectangle(panelX + 150, panelY + 147, (int)(230 * (d.fuel / d.maxFuel)), 14, YELLOW);
        DrawCustomText(TextFormat("%.0f / %.0f", d.fuel, d.maxFuel), panelX + 390, panelY + 145, 11.0f, RAYWHITE);

        if (d.role != DroneRole::RECON) {
            DrawCustomText("AMMO CAPACITY:", panelX + 20, panelY + 170, 13.0f, RED);
            DrawRectangle(panelX + 150, panelY + 172, 230, 14, DARKGRAY);
            DrawRectangle(panelX + 150, panelY + 172, (int)(230 * (d.ammo / d.maxAmmo)), 14, RED);
            DrawCustomText(TextFormat("%.0f%%", d.ammo), panelX + 390, panelY + 170, 11.0f, RAYWHITE);
        } else {
            DrawCustomText("WEAPONS / AMMO:", panelX + 20, panelY + 170, 13.0f, GRAY);
            DrawCustomText("[ NO WEAPONS / UNARMED SCOUT ]", panelX + 150, panelY + 170, 12.0f, GRAY);
        }

        if (d.role == DroneRole::SUPPORT) {
            DrawCustomText("CARGO FUEL:", panelX + 20, panelY + 195, 13.0f, LIME);
            DrawRectangle(panelX + 150, panelY + 197, 230, 14, DARKGRAY);
            DrawRectangle(panelX + 150, panelY + 197, (int)(230 * (d.cargoFuel / 600.0f)), 14, LIME);
        }

        DrawCustomText("INDIVIDUAL FLIGHT ACTION LOGS:", panelX + 20, panelY + 230, 14.0f, LIME);
        DrawLine(panelX + 20, panelY + 248, panelX + panelW - 20, panelY + 248, LIME);

        int logY = panelY + 258;
        for (const auto& logMsg : d.actionLogs) {
            DrawCustomText(logMsg.c_str(), panelX + 25, logY, 12.0f, RAYWHITE);
            logY += 18;
        }
    }

    DrawCustomText("[1..9] Select Drone | [I] Close Inspector", panelX + panelW / 2 - 120, panelY + panelH - 25, 12.0f, GRAY);
}

void DroneSystemManager::Draw2DHUD() const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (isMapFullscreen) {
        DrawTacticalMap2D(20, 20, screenWidth - 40, screenHeight - 40, true);
        DrawCustomText("[M] Exit Fullscreen Map Mode", 40, screenHeight - 50, 15.0f, GOLD);
        return;
    }

    // 1. Top-Left Telemetry Panel
    if (showTelemetryPanel) {
        int leftW = 340;
        int leftH = 175;
        int leftX = 20;
        int leftY = 15;

        DrawRectangle(leftX, leftY, leftW, leftH, (Color){ 4, 10, 20, 240 });
        DrawRectangleLines(leftX, leftY, leftW, leftH, SKYBLUE);
        DrawRectangleLines(leftX + 2, leftY + 2, leftW - 4, leftH - 4, Fade(SKYBLUE, 0.35f));

        DrawCustomText("LOGISTICS & TELEMETRY", leftX + 15, leftY + 12, 16.0f, SKYBLUE);
        DrawLine(leftX + 15, leftY + 34, leftX + leftW - 15, leftY + 34, SKYBLUE);

        DrawCustomText(TextFormat("Total Fleet: %d Drones (Air: %d | Hangar: %d)", telemetry.totalFleetCount, telemetry.activeInMission, telemetry.dockedAtBase), leftX + 15, leftY + 42, 13.0f, RAYWHITE);
        DrawCustomText(TextFormat("Active Clusters: %d", telemetry.activeClustersCount), leftX + 15, leftY + 62, 13.0f, ORANGE);
        DrawCustomText(TextFormat("Hostile Magnitude: %d Units", telemetry.knownTargetsMagnitude), leftX + 15, leftY + 82, 13.0f, YELLOW);
        DrawCustomText(TextFormat("Targets Neutralized: %d Units", telemetry.eliminatedTargetsCount), leftX + 15, leftY + 102, 13.0f, GREEN);

        DrawCustomText("Fleet: 2x RECON | 10x TASK FORCE | 2x SUPPORT", leftX + 15, leftY + 130, 11.0f, LIGHTGRAY);
        DrawCustomText("Base: 3D Hangar Base (0,0,0) - 14 Helipads", leftX + 15, leftY + 148, 11.0f, GRAY);
    }

    // 2. Top-Right Fleet Resource Panel (ALL 14 DRONES)
    if (showResourcePanel) {
        int rightW = 340;
        int rightH = 370;
        int rightX = screenWidth - rightW - 20;
        int rightY = 15;

        DrawRectangle(rightX, rightY, rightW, rightH, (Color){ 4, 10, 20, 245 });
        DrawRectangleLines(rightX, rightY, rightW, rightH, ORANGE);
        DrawRectangleLines(rightX + 2, rightY + 2, rightW - 4, rightH - 4, Fade(ORANGE, 0.35f));

        DrawCustomText("FLEET SENSORS & STATUS (14 DRONES)", rightX + 15, rightY + 10, 14.0f, ORANGE);
        DrawLine(rightX + 15, rightY + 28, rightX + rightW - 15, rightY + 28, ORANGE);

        int startY = rightY + 34;
        for (int i = 0; i < (int)drones.size(); i++) {
            const auto& d = drones[i];
            const char* roleStr = (d.role == DroneRole::RECON) ? "[RECON]" : ((d.role == DroneRole::TASK_FORCE) ? "[TASK]" : "[SUPP]");
            Color roleCol = (d.role == DroneRole::RECON) ? SKYBLUE : ((d.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);

            const char* tag = "IDLE";
            Color tagCol = GRAY;
            switch (d.state) {
                case DroneState::PATROL: tag = "SWEEP"; tagCol = SKYBLUE; break;
                case DroneState::SCANNING: tag = "SCAN"; tagCol = YELLOW; break;
                case DroneState::MOVING_TO_TARGET: tag = "MOVE"; tagCol = LIGHTGRAY; break;
                case DroneState::ENGAGING: tag = "ENGAGE"; tagCol = RED; break;
                case DroneState::REQUESTING_SUPPORT: tag = "RDV"; tagCol = GOLD; break;
                case DroneState::RESUPPLYING_IN_AIR: tag = "SUPP"; tagCol = LIME; break;
                case DroneState::RETURNING_TO_BASE: tag = "RTB"; tagCol = ORANGE; break;
                default: tag = "IDLE"; tagCol = GRAY; break;
            }

            DrawCustomText(TextFormat("#%-2d %s", d.id, roleStr), rightX + 12, startY, 10.0f, roleCol);
            DrawCustomText(tag, rightX + 115, startY, 10.0f, tagCol);

            DrawCustomText("F", rightX + 165, startY, 10.0f, LIGHTGRAY);
            DrawRectangle(rightX + 178, startY + 2, 60, 8, DARKGRAY);
            DrawRectangle(rightX + 178, startY + 2, (int)(60 * (d.fuel / d.maxFuel)), 8, YELLOW);

            if (d.role != DroneRole::RECON) {
                DrawCustomText("A", rightX + 250, startY, 10.0f, LIGHTGRAY);
                DrawRectangle(rightX + 263, startY + 2, 55, 8, DARKGRAY);
                DrawRectangle(rightX + 263, startY + 2, (int)(55 * (d.ammo / d.maxAmmo)), 8, RED);
            } else {
                DrawCustomText("-", rightX + 275, startY, 10.0f, DARKGRAY);
            }

            startY += 22;
        }
    }

    // 3. Top-Center Camera Mode Banner
    if (showCameraBanner) {
        int bannerW = 500;
        int bannerH = 55;
        int bannerX = (screenWidth - bannerW) / 2;
        int bannerY = 15;

        DrawRectangle(bannerX, bannerY, bannerW, bannerH, (Color){ 4, 10, 20, 245 });
        DrawRectangleLines(bannerX, bannerY, bannerW, bannerH, GOLD);
        DrawRectangleLines(bannerX + 2, bannerY + 2, bannerW - 4, bannerH - 4, Fade(GOLD, 0.35f));

        std::string camTitle = "CAMERA VIEW: " + GetActiveCameraModeName();
        Color modeColor = (activeCameraMode == 0) ? SKYBLUE : ((activeCameraMode <= 2) ? SKYBLUE : ((activeCameraMode >= 13) ? LIME : ORANGE));
        
        float titleW = fontLoaded ? MeasureTextEx(customFont, camTitle.c_str(), 15.0f, 1.0f).x : (float)MeasureText(camTitle.c_str(), 15);
        DrawCustomText(camTitle.c_str(), bannerX + (bannerW - titleW) / 2.0f, bannerY + 8, 15.0f, modeColor);

        const char* hintMsg = "[TAB] Cycle | [1..9] Track | [M] Full Map | [I] Drone Inspector | [F1-F8] UI";
        float hintW = fontLoaded ? MeasureTextEx(customFont, hintMsg, 11.0f, 1.0f).x : (float)MeasureText(hintMsg, 11);
        DrawCustomText(hintMsg, bannerX + (bannerW - hintW) / 2.0f, bannerY + 30, 11.0f, YELLOW);
    }

    // 4. Tactical Mini-Map Box (Bottom-Right)
    if (showTacticalMap) {
        int mapW = 200;
        int mapH = 200;
        int mapX = screenWidth - mapW - 20;
        int mapY = screenHeight - mapH - 20;
        DrawTacticalMap2D(mapX, mapY, mapW, mapH, false);
    }

    // 5. Bottom Command Terminal Log Drawer
    if (showTerminalPanel) {
        int mapOffset = showTacticalMap ? 230 : 20;
        int termH = 160;
        int termY = screenHeight - termH - 20;
        int termW = screenWidth - mapOffset - 30;

        DrawRectangle(20, termY, termW, termH, (Color){ 3, 7, 14, 248 });
        DrawRectangleLines(20, termY, termW, termH, LIME);
        DrawRectangleLines(22, termY + 2, termW - 4, termH - 4, Fade(LIME, 0.35f));

        DrawCustomText("TACTICAL OPERATIONAL COMMAND TERMINAL LOGS", 35, termY + 10, 15.0f, LIME);
        DrawLine(35, termY + 30, 20 + termW - 15, termY + 30, LIME);

        int logY = termY + 36;
        int showCount = std::min(6, (int)telemetry.terminalLogs.size());
        int startIndex = (int)telemetry.terminalLogs.size() - showCount;

        for (int i = startIndex; i < (int)telemetry.terminalLogs.size(); i++) {
            DrawCustomText(telemetry.terminalLogs[i].c_str(), 35, logY, 13.0f, (Color){ 0, 255, 120, 255 });
            logY += 18;
        }
    }

    // 6. Drone Inspector Overlay Window (`I` Key)
    if (showDroneInspector) {
        DrawDroneInspector2D();
    }
}
