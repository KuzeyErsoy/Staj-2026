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

    // Try loading vector TTF fonts (Arial / Helvetica) for crisp typography
    const char* fontPaths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "resources/fonts/arial.ttf",
        "../resources/fonts/arial.ttf"
    };

    for (const char* path : fontPaths) {
        if (FileExists(path)) {
            customFont = LoadFontEx(path, 48, NULL, 0);
            if (customFont.texture.id > 0) {
                SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
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
        case 1: return "TRACKING DRONE #1 [RECON]";
        case 2: return "TRACKING DRONE #2 [TASK FORCE 1]";
        case 3: return "TRACKING DRONE #3 [TASK FORCE 2]";
        case 4: return "TRACKING DRONE #4 [TASK FORCE 3]";
        case 5: return "TRACKING DRONE #5 [SUPPORT]";
        default: return "FREE CAM";
    }
}

void DroneSystemManager::UpdateFleetCamera(Camera3D& camera, float dt) {
    // Camera mode toggles
    if (IsKeyPressed(KEY_TAB)) {
        activeCameraMode = (activeCameraMode + 1) % 6;
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
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) { activeCameraMode = 1; LogTerminal("[COMMAND CAM] Tracking Drone #1 [RECON]."); if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) { activeCameraMode = 2; LogTerminal("[COMMAND CAM] Tracking Drone #2 [TASK FORCE 1]."); if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) { activeCameraMode = 3; LogTerminal("[COMMAND CAM] Tracking Drone #3 [TASK FORCE 2]."); if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) { activeCameraMode = 4; LogTerminal("[COMMAND CAM] Tracking Drone #4 [TASK FORCE 3]."); if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FIVE) || IsKeyPressed(KEY_KP_5)) { activeCameraMode = 5; LogTerminal("[COMMAND CAM] Tracking Drone #5 [SUPPORT]."); if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }

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
        if (IsKeyDown(KEY_D)) freeCamPos = Vector3Add(freeCamPos, Vector3Scale(right, flySpeed));
        if (IsKeyDown(KEY_A)) freeCamPos = Vector3Subtract(freeCamPos, Vector3Scale(right, flySpeed));
        if (IsKeyDown(KEY_SPACE)) freeCamPos.y += flySpeed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) freeCamPos.y -= flySpeed;

        camera.position = freeCamPos;
        camera.target = Vector3Add(freeCamPos, forward);
        camera.fovy = 60.0f;
    } else {
        // INTERACTIVE MOUSE ORBIT TRACKING DRONE #1..#5
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
    if (telemetry.terminalLogs.size() > 18) {
        telemetry.terminalLogs.erase(telemetry.terminalLogs.begin());
    }
}

void DroneSystemManager::Init() {
    base.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    base.radius = 20.0f;
    base.fuelSupply = 9999.0f;
    base.ammoSupply = 9999.0f;

    drones.clear();

    // 1x RECON Drone (Surveillance / Target Acquisition)
    Drone recon;
    recon.id = 1;
    recon.role = DroneRole::RECON;
    recon.state = DroneState::IDLE_AT_BASE;
    recon.position = (Vector3){ 0.0f, 3.0f, 0.0f };
    recon.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    recon.targetPosition = recon.position;
    recon.maxFuel = 100.0f; recon.fuel = 100.0f;
    recon.maxAmmo = 100.0f; recon.ammo = 100.0f;
    drones.push_back(recon);

    // 3x TASK FORCE Drones (Tactical Combat Interceptors)
    for (int i = 0; i < 3; i++) {
        Drone tf;
        tf.id = 2 + i;
        tf.role = DroneRole::TASK_FORCE;
        tf.state = DroneState::IDLE_AT_BASE;
        tf.position = (Vector3){ (float)(i - 1) * 6.0f, 3.0f, 6.0f };
        tf.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        tf.targetPosition = tf.position;
        tf.maxFuel = 100.0f; tf.fuel = 100.0f;
        tf.maxAmmo = 100.0f; tf.ammo = 100.0f;
        drones.push_back(tf);
    }

    // 1x SUPPORT Drone (Aerial Logistics & Heavy Resupply Cargo)
    Drone supp;
    supp.id = 5;
    supp.role = DroneRole::SUPPORT;
    supp.state = DroneState::IDLE_AT_BASE;
    supp.position = (Vector3){ 0.0f, 3.0f, -6.0f };
    supp.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    supp.targetPosition = supp.position;
    supp.maxFuel = 120.0f; supp.fuel = 120.0f;
    supp.maxAmmo = 100.0f; supp.ammo = 100.0f;
    supp.cargoFuel = 250.0f; supp.cargoAmmo = 250.0f;
    drones.push_back(supp);

    clusters.clear();
    nextClusterId = 1;
    telemetry.terminalLogs.clear();
    telemetry.eliminatedTargetsCount = 0;

    LogTerminal("[TACTICAL COMMAND] Military Autonomous 5-Drone Fleet Operational.");
    LogTerminal("[BASE LOGISTICS] Home Base active at grid origin (0,0,0) - Radius 20m.");

    SpawnCluster();
}

void DroneSystemManager::SpawnCluster() {
    EnemyCluster cluster;
    cluster.id = nextClusterId++;
    
    float rx = (float)GetRandomValue(-380, 380);
    float rz = (float)GetRandomValue(-380, 380);
    if (fabsf(rx) < 40.0f && fabsf(rz) < 40.0f) {
        rx = 180.0f;
        rz = 180.0f;
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
    snprintf(buf, sizeof(buf), "[SAT-RECON] Hostile Target Cluster #%d detected at (%.0f, %.0f) - Est. Magnitude: %d", cluster.id, rx, rz, mag);
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

void DroneSystemManager::UpdateRecon(Drone& drone, float dt) {
    drone.fuel -= dt * 0.8f;
    if (drone.fuel <= 18.0f) {
        drone.state = DroneState::RETURNING_TO_BASE;
        drone.targetPosition = (Vector3){ 0.0f, 12.0f, 0.0f };
        LogTerminal("[RECON] Low fuel warning (<18%). Returning to Home Base for refueling.");
        return;
    }

    EnemyCluster* targetCluster = nullptr;
    for (auto& c : clusters) {
        if (c.status == ClusterStatus::UNDETECTED || c.status == ClusterStatus::REQUIRES_RECON) {
            targetCluster = &c;
            break;
        }
    }

    if (targetCluster != nullptr) {
        drone.targetPosition = (Vector3){ targetCluster->centerPosition.x, 16.0f, targetCluster->centerPosition.z };
        float dist = Vector3Distance(drone.position, targetCluster->centerPosition);

        if (dist < 120.0f) {
            drone.state = DroneState::SCANNING;
            scanTimer += dt;
            if (scanTimer >= 1.8f) {
                targetCluster->status = ClusterStatus::REPORTED;
                scanTimer = 0.0f;
                char buf[128];
                snprintf(buf, sizeof(buf), "[RECON] Target Cluster #%d VERIFIED at (%.0f, %.0f)! Transmitting coords to Task Force...",
                         targetCluster->id, targetCluster->centerPosition.x, targetCluster->centerPosition.z);
                LogTerminal(buf);
                if (IsAudioDeviceReady()) AudioManager::Instance().PlayCollect();
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
        }
    } else {
        drone.state = DroneState::PATROL;
        patrolAngle += dt * 0.12f;
        drone.targetPosition = (Vector3){ cosf(patrolAngle) * 280.0f, 20.0f, sinf(patrolAngle) * 280.0f };
    }
}

void DroneSystemManager::UpdateTaskForce(Drone& drone, float dt) {
    if (drone.fuel <= 20.0f || drone.ammo <= 15.0f) {
        if (drone.state != DroneState::REQUESTING_SUPPORT && drone.state != DroneState::RESUPPLYING_IN_AIR) {
            drone.state = DroneState::REQUESTING_SUPPORT;
            char buf[128];
            snprintf(buf, sizeof(buf), "[TASK FORCE #%d] Resource Critical! Fuel: %.0f%%, Ammo: %.0f%%. Broadcasting Support Request!",
                     drone.id, drone.fuel, drone.ammo);
            LogTerminal(buf);
        }
    }

    if (drone.fuel <= 8.0f || (drone.ammo <= 0.0f && drone.state != DroneState::RESUPPLYING_IN_AIR)) {
        drone.state = DroneState::RETURNING_TO_BASE;
        drone.targetPosition = (Vector3){ (float)(drone.id - 3) * 7.0f, 5.0f, 7.0f };
        drone.assignedClusterId = -1;
        return;
    }

    EnemyCluster* targetCluster = nullptr;
    for (auto& c : clusters) {
        if (c.id == drone.assignedClusterId && c.status != ClusterStatus::CLEARED) {
            targetCluster = &c;
            break;
        }
    }

    if (!targetCluster) {
        for (auto& c : clusters) {
            if ((c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) && c.remainingMagnitude > 0) {
                targetCluster = &c;
                drone.assignedClusterId = c.id;
                c.status = ClusterStatus::ENGAGED;
                char buf[128];
                snprintf(buf, sizeof(buf), "[TASK FORCE #%d] Assigned to Target Cluster #%d. Intercepting hostiles!", drone.id, c.id);
                LogTerminal(buf);
                break;
            }
        }
    }

    if (targetCluster && targetCluster->remainingMagnitude > 0) {
        // TACTICAL FLANKING FORMATION: Distribute Task Force drones in wide arc around cluster
        float tfIdx = (float)(drone.id - 2); // 0, 1, 2
        float baseAngle = atan2f(targetCluster->centerPosition.x - base.position.x, targetCluster->centerPosition.z - base.position.z);
        float offsetAngle = baseAngle + (tfIdx - 1.0f) * 0.75f;
        float standoffRadius = 34.0f + tfIdx * 6.0f;
        float standoffHeight = 12.0f + tfIdx * 4.0f;

        drone.targetPosition = (Vector3){
            targetCluster->centerPosition.x + sinf(offsetAngle) * standoffRadius,
            standoffHeight,
            targetCluster->centerPosition.z + cosf(offsetAngle) * standoffRadius
        };

        float dist = Vector3Distance(drone.position, targetCluster->centerPosition);

        if (dist < 110.0f && drone.ammo > 0.0f) {
            drone.state = DroneState::ENGAGING;
            drone.fuel -= dt * 1.1f;
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

                    if (IsAudioDeviceReady()) AudioManager::Instance().PlayHurt();
                }
            } else {
                drone.isFiringLaser = false;
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
            drone.isFiringLaser = false;
            drone.fuel -= dt * 0.9f;
        }
    } else {
        drone.isFiringLaser = false;
        if (drone.state != DroneState::REQUESTING_SUPPORT && drone.state != DroneState::RESUPPLYING_IN_AIR) {
            drone.state = DroneState::IDLE_AT_BASE;
            drone.targetPosition = (Vector3){ (float)(drone.id - 3) * 7.0f, 4.0f, 7.0f };
        }
    }
}

void DroneSystemManager::UpdateSupport(Drone& drone, float dt) {
    drone.fuel -= dt * 0.7f;

    Drone* needyDrone = nullptr;
    float lowestRes = 999.0f;

    for (auto& d : drones) {
        if (d.role == DroneRole::TASK_FORCE && (d.state == DroneState::REQUESTING_SUPPORT || d.fuel < 35.0f || d.ammo < 30.0f)) {
            float res = d.fuel + d.ammo;
            if (res < lowestRes) {
                lowestRes = res;
                needyDrone = &d;
            }
        }
    }

    if (needyDrone && (drone.cargoFuel > 0.0f || drone.cargoAmmo > 0.0f)) {
        float tfHeading = needyDrone->yaw * DEG2RAD;
        drone.targetPosition = (Vector3){
            needyDrone->position.x - sinf(tfHeading) * 5.0f,
            needyDrone->position.y + 6.0f,
            needyDrone->position.z - cosf(tfHeading) * 5.0f
        };

        float dist = Vector3Distance(drone.position, needyDrone->position);

        if (dist < 35.0f) {
            drone.state = DroneState::RESUPPLYING_IN_AIR;
            drone.isResupplyBeamActive = true;
            drone.resupplyTargetPos = needyDrone->position;

            float transferFuel = dt * 20.0f;
            float transferAmmo = dt * 30.0f;

            if (drone.cargoFuel >= transferFuel && needyDrone->fuel < needyDrone->maxFuel) {
                drone.cargoFuel -= transferFuel;
                needyDrone->fuel = std::min(needyDrone->maxFuel, needyDrone->fuel + transferFuel);
            }
            if (drone.cargoAmmo >= transferAmmo && needyDrone->ammo < needyDrone->maxAmmo) {
                drone.cargoAmmo -= transferAmmo;
                needyDrone->ammo = std::min(needyDrone->maxAmmo, needyDrone->ammo + transferAmmo);
            }

            needyDrone->state = DroneState::RESUPPLYING_IN_AIR;

            if (needyDrone->fuel >= 92.0f && needyDrone->ammo >= 92.0f) {
                needyDrone->state = DroneState::MOVING_TO_TARGET;
                char buf[128];
                snprintf(buf, sizeof(buf), "[LOGISTICS SUPPORT] Task Force #%d Aerial Resupply Completed!", needyDrone->id);
                LogTerminal(buf);
            }
        } else {
            drone.state = DroneState::MOVING_TO_TARGET;
            drone.isResupplyBeamActive = false;
        }
    } else {
        drone.isResupplyBeamActive = false;
        if (drone.cargoFuel <= 20.0f || drone.cargoAmmo <= 20.0f || drone.fuel <= 20.0f) {
            drone.state = DroneState::RETURNING_TO_BASE;
            drone.targetPosition = (Vector3){ 0.0f, 5.0f, -6.0f };
        } else {
            drone.state = DroneState::PATROL;
            drone.targetPosition = (Vector3){ 0.0f, 16.0f, 0.0f };
        }
    }
}

void DroneSystemManager::UpdateBaseRefill(Drone& drone, float dt) {
    float distToBase = Vector3Distance(drone.position, base.position);

    if (distToBase < base.radius + 12.0f) {
        if (drone.state == DroneState::RETURNING_TO_BASE || drone.state == DroneState::IDLE_AT_BASE) {
            float refillRate = dt * 35.0f;
            drone.fuel = std::min(drone.maxFuel, drone.fuel + refillRate);
            drone.ammo = std::min(drone.maxAmmo, drone.ammo + refillRate);

            if (drone.role == DroneRole::SUPPORT) {
                drone.cargoFuel = std::min(250.0f, drone.cargoFuel + dt * 60.0f);
                drone.cargoAmmo = std::min(250.0f, drone.cargoAmmo + dt * 60.0f);
            }

            if (drone.fuel >= drone.maxFuel && drone.ammo >= drone.maxAmmo) {
                if (drone.state == DroneState::RETURNING_TO_BASE) {
                    drone.state = DroneState::IDLE_AT_BASE;
                    char buf[128];
                    snprintf(buf, sizeof(buf), "[BASE LOGISTICS] Drone #%d fully refueled, rearmed & combat-ready.", drone.id);
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

        switch (d.role) {
            case DroneRole::RECON:      UpdateRecon(d, dt); break;
            case DroneRole::TASK_FORCE: UpdateTaskForce(d, dt); break;
            case DroneRole::SUPPORT:    UpdateSupport(d, dt); break;
        }

        UpdateBaseRefill(d, dt);
    }

    UpdateTelemetry();
}

void DroneSystemManager::DrawBase3D() const {
    DrawGrid(100, 10.0f);

    DrawCylinder(base.position, base.radius, base.radius, 0.4f, 32, (Color){ 0, 220, 255, 60 });
    DrawCircle3D(base.position, base.radius, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 0, 255, 255, 220 });
    DrawCircle3D(base.position, base.radius * 0.5f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 0, 200, 255, 140 });

    DrawCubeWires((Vector3){ 0.0f, 0.5f, 0.0f }, 4.0f, 1.0f, 4.0f, SKYBLUE);
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

void DroneSystemManager::Draw2DHUD() const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 1. Top-Left Telemetry Panel (Compact 340px width, 175px height - NO OVERLAP)
    int leftW = 340;
    int leftH = 175;
    int leftX = 20;
    int leftY = 15;

    DrawRectangle(leftX, leftY, leftW, leftH, (Color){ 4, 10, 20, 240 });
    DrawRectangleLines(leftX, leftY, leftW, leftH, SKYBLUE);
    DrawRectangleLines(leftX + 2, leftY + 2, leftW - 4, leftH - 4, Fade(SKYBLUE, 0.35f));

    DrawCustomText("LOGISTICS & TELEMETRY", leftX + 15, leftY + 12, 16.0f, SKYBLUE);
    DrawLine(leftX + 15, leftY + 34, leftX + leftW - 15, leftY + 34, SKYBLUE);

    DrawCustomText(TextFormat("Total Fleet: %d Drones (Air: %d | Base: %d)", telemetry.totalFleetCount, telemetry.activeInMission, telemetry.dockedAtBase), leftX + 15, leftY + 42, 13.0f, RAYWHITE);
    DrawCustomText(TextFormat("Active Clusters: %d", telemetry.activeClustersCount), leftX + 15, leftY + 62, 13.0f, ORANGE);
    DrawCustomText(TextFormat("Hostile Magnitude: %d Units", telemetry.knownTargetsMagnitude), leftX + 15, leftY + 82, 13.0f, YELLOW);
    DrawCustomText(TextFormat("Targets Neutralized: %d Units", telemetry.eliminatedTargetsCount), leftX + 15, leftY + 102, 13.0f, GREEN);

    DrawCustomText("Fleet: 1x RECON | 3x TASK FORCE | 1x SUPPORT", leftX + 15, leftY + 130, 11.0f, LIGHTGRAY);
    DrawCustomText("Grid: 1000x1000m | Home Base R=20m", leftX + 15, leftY + 148, 11.0f, GRAY);

    // 2. Top-Right Individual Drone Status Panel (Compact 340px width, 175px height - NO OVERLAP)
    int rightW = 340;
    int rightH = 175;
    int rightX = screenWidth - rightW - 20;
    int rightY = 15;

    DrawRectangle(rightX, rightY, rightW, rightH, (Color){ 4, 10, 20, 240 });
    DrawRectangleLines(rightX, rightY, rightW, rightH, ORANGE);
    DrawRectangleLines(rightX + 2, rightY + 2, rightW - 4, rightH - 4, Fade(ORANGE, 0.35f));

    DrawCustomText("FLEET SENSORS & RESOURCES", rightX + 15, rightY + 12, 16.0f, ORANGE);
    DrawLine(rightX + 15, rightY + 34, rightX + rightW - 15, rightY + 34, ORANGE);

    int startY = rightY + 42;
    for (const auto& d : drones) {
        const char* roleStr = (d.role == DroneRole::RECON) ? "[RECON]" : ((d.role == DroneRole::TASK_FORCE) ? "[TASK]" : "[SUPP]");
        Color roleCol = (d.role == DroneRole::RECON) ? SKYBLUE : ((d.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);

        DrawCustomText(TextFormat("#%d %s", d.id, roleStr), rightX + 15, startY, 12.0f, roleCol);

        DrawCustomText("FUEL", rightX + 100, startY, 11.0f, LIGHTGRAY);
        DrawRectangle(rightX + 135, startY + 2, 75, 10, DARKGRAY);
        DrawRectangle(rightX + 135, startY + 2, (int)(75 * (d.fuel / d.maxFuel)), 10, YELLOW);
        DrawRectangleLines(rightX + 135, startY + 2, 75, 10, MAROON);

        DrawCustomText("AMMO", rightX + 220, startY, 11.0f, LIGHTGRAY);
        DrawRectangle(rightX + 255, startY + 2, 70, 10, DARKGRAY);
        DrawRectangle(rightX + 255, startY + 2, (int)(70 * (d.ammo / d.maxAmmo)), 10, RED);
        DrawRectangleLines(rightX + 255, startY + 2, 70, 10, MAROON);

        startY += 24;
    }

    // 3. Top-Center Camera Mode & Flight Control Banner (500px width, 55px height - NO OVERLAP)
    int bannerW = 500;
    int bannerH = 55;
    int bannerX = (screenWidth - bannerW) / 2;
    int bannerY = 15;

    DrawRectangle(bannerX, bannerY, bannerW, bannerH, (Color){ 4, 10, 20, 245 });
    DrawRectangleLines(bannerX, bannerY, bannerW, bannerH, GOLD);
    DrawRectangleLines(bannerX + 2, bannerY + 2, bannerW - 4, bannerH - 4, Fade(GOLD, 0.35f));

    std::string camTitle = "CAMERA VIEW: " + GetActiveCameraModeName();
    Color modeColor = (activeCameraMode == 0) ? SKYBLUE : ((activeCameraMode == 1) ? SKYBLUE : ((activeCameraMode == 5) ? LIME : ORANGE));
    
    float titleW = fontLoaded ? MeasureTextEx(customFont, camTitle.c_str(), 15.0f, 1.0f).x : (float)MeasureText(camTitle.c_str(), 15);
    DrawCustomText(camTitle.c_str(), bannerX + (bannerW - titleW) / 2.0f, bannerY + 8, 15.0f, modeColor);

    const char* hintMsg = "[TAB] Cycle View | [1..5] Track Drone | [0/C] Free Fly | Mouse: 360 Orbit";
    float hintW = fontLoaded ? MeasureTextEx(customFont, hintMsg, 11.0f, 1.0f).x : (float)MeasureText(hintMsg, 11);
    DrawCustomText(hintMsg, bannerX + (bannerW - hintW) / 2.0f, bannerY + 30, 11.0f, YELLOW);

    // 4. Bottom Command Terminal Log Drawer
    int termH = 160;
    int termY = screenHeight - termH - 15;
    int termW = screenWidth - 40;

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
