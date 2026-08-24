#include "Drone.h"
#include "ClusterManager.h"
#include "MissileSystem.h"
#include "HangarBase.h"
#include "AudioManager.h"
#include "rlgl.h"
#include <cstdio>
#include <ctime>

namespace DroneFleet {

// ==========================================
// BASE DRONE IMPLEMENTATION
// ==========================================

Drone::Drone(int id, DroneRole role)
    : id(id), role(role), state(DroneState::IDLE_AT_BASE) {}

void Drone::LogAction(const std::string& msg) {
    int totalSecs = (int)GetTime();
    int mins = (totalSecs / 60) % 60;
    int secs = totalSecs % 60;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "[%02d:%02d] ", mins, secs);
    actionLogs.push_back(std::string(timeStr) + msg);
    if (actionLogs.size() > 10) {
        actionLogs.erase(actionLogs.begin());
    }
}

void Drone::ResetToDock() {
    state = DroneState::IDLE_AT_BASE;
    position = baseDockPos;
    targetPosition = baseDockPos;
    velocity = Vector3{ 0.0f, 0.0f, 0.0f };
    yaw = 0.0f;
    pitch = 0.0f;
    roll = 0.0f;
    isDestroyed = false;
    isBeingRecovered = false;
    assignedClusterId = -1;
    hasRendezvous = false;
    assignedSupportId = -1;
    isFiringLaser = false;
    isResupplyBeamActive = false;
    distanceFromBase = 0.0f;
    snrDb = 45.0f;
    signalQuality = 1.0f;
    telemetryLatencyMs = 12.0f;
    bitrateMbps = 12.0f;
    flareParticles.clear();
    flareCooldown = 0.0f;
    flareAlertTimer = 0.0f;
    downwashParticles.clear();
}

void Drone::UpdateMovementPhysics(float targetSpeed, float dt) {
    rotorAngle += dt * 720.0f;
    if (rotorAngle > 360.0f) rotorAngle -= 360.0f;

    // --- RADIO TELEMETRY & SNR COMPUTATION ---
    if (isDestroyed) {
        snrDb = 0.0f;
        signalQuality = 0.0f;
        bitrateMbps = 0.0f;
        telemetryLatencyMs = 999.0f;
    } else {
        distanceFromBase = Vector3Distance(position, Vector3{ 0.0f, 0.0f, 0.0f });
        // Logarithmic Path Loss: 45.0 dB at 20m, decreasing with 20 * log10(d / 20)
        float refDist = 20.0f;
        float distClamped = std::max(1.0f, distanceFromBase);
        snrDb = 45.0f - 20.0f * log10f(distClamped / refDist);
        snrDb = std::clamp(snrDb, 5.0f, 45.0f);

        // Quality: 1.0 (at 45 dB) down to 0.0 (at 10 dB or less)
        signalQuality = std::clamp((snrDb - 10.0f) / 35.0f, 0.0f, 1.0f);

        // Latency: Base 12ms + 1ms per 25 meters
        telemetryLatencyMs = 12.0f + (distanceFromBase / 25.0f);

        // Adaptive bitrate: 12.0 Mbps down to 1.5 Mbps
        bitrateMbps = 1.5f + (signalQuality * 10.5f);
    }

    if (state == DroneState::IDLE_AT_BASE) {
        position = Vector3Lerp(position, baseDockPos, dt * 5.0f);
        velocity = Vector3{ 0.0f, 0.0f, 0.0f };
        pitch = 0.0f;
        roll = 0.0f;
        return;
    }

    Vector3 moveDir = Vector3Subtract(targetPosition, position);
    float dist = Vector3Length(moveDir);

    if (dist > 0.8f) {
        Vector3 normDir = Vector3Scale(moveDir, 1.0f / dist);
        Vector3 desiredVel = Vector3Scale(normDir, targetSpeed);

        velocity = Vector3Lerp(velocity, desiredVel, dt * 2.5f);
        position = Vector3Add(position, Vector3Scale(velocity, dt));

        yaw = atan2f(velocity.x, velocity.z) * RAD2DEG;
        float speedVal = Vector3Length(velocity);
        pitch = std::clamp(speedVal * 1.1f, -20.0f, 20.0f);
    } else {
        velocity = Vector3Lerp(velocity, Vector3{ 0.0f, 0.0f, 0.0f }, dt * 3.5f);
        position = Vector3Add(position, Vector3Scale(velocity, dt));
        pitch = std::clamp(pitch * 0.9f, -2.0f, 2.0f);
    }
}

void Drone::ApplyFlankingAndSeparation(FleetContext& ctx, float dt) {
    for (auto& other : ctx.drones) {
        if (other->id == id || other->isDestroyed) continue;
        float dist = Vector3Distance(position, other->position);
        float minDist = (role == DroneRole::SUPPORT && other->role == DroneRole::TASK_FORCE && state == DroneState::RESUPPLYING_IN_AIR) ? 4.5f : 9.0f;

        if (dist < minDist && dist > 0.05f) {
            Vector3 pushDir = Vector3Normalize(Vector3Subtract(position, other->position));
            float pushForce = (minDist - dist) * 10.0f;
            velocity = Vector3Add(velocity, Vector3Scale(pushDir, pushForce * dt));
        }
    }
}

void Drone::Draw3D(VisionMode visionMode) const {
    if (isDestroyed) {
        if (visionMode == VisionMode::FLIR_THERMAL) {
            DrawCube(position, 1.8f, 0.4f, 1.8f, Color{ 25, 28, 35, 255 });
            DrawCubeWires(position, 1.85f, 0.45f, 1.85f, Color{ 70, 80, 95, 255 });
            DrawCircle3D(position, 1.8f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 180, 40, 20, 255 }, 0.5f));
        } else if (visionMode == VisionMode::NVG_GREEN) {
            DrawCube(position, 1.8f, 0.4f, 1.8f, Color{ 10, 35, 15, 255 });
            DrawCubeWires(position, 1.85f, 0.45f, 1.85f, Color{ 30, 90, 40, 255 });
            DrawCircle3D(position, 1.8f, Vector3{ 1, 0, 0 }, 90.0f, Fade(GREEN, 0.4f));
        } else {
            DrawCube(position, 1.8f, 0.4f, 1.8f, DARKGRAY);
            DrawCubeWires(position, 1.85f, 0.45f, 1.85f, BLACK);
            DrawCircle3D(position, 1.8f, Vector3{ 1, 0, 0 }, 90.0f, Fade(RED, 0.6f));
        }
        DrawFlares(visionMode);
        return;
    }

    Color bodyColor = WHITE;
    Color wireColor = DARKGRAY;

    if (visionMode == VisionMode::FLIR_THERMAL) {
        // Cold dark carbon chassis with glowing cool thermal edges
        bodyColor = Color{ 48, 56, 68, 255 };
        wireColor = Color{ 180, 220, 245, 255 };
    } else if (visionMode == VisionMode::NVG_GREEN) {
        bodyColor = Color{ 15, 90, 30, 255 };
        wireColor = Color{ 80, 255, 110, 255 };
    } else {
        switch (role) {
            case DroneRole::RECON:      bodyColor = SKYBLUE; break;
            case DroneRole::TASK_FORCE: bodyColor = ORANGE; break;
            case DroneRole::SUPPORT:    bodyColor = LIME; break;
        }
    }

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);
    rlRotatef(pitch, 1.0f, 0.0f, 0.0f);

    DrawCube(Vector3{ 0, 0, 0 }, 1.8f, 0.6f, 1.8f, bodyColor);
    DrawCubeWires(Vector3{ 0, 0, 0 }, 1.85f, 0.65f, 1.85f, wireColor);

    float rOffset = 1.3f;
    Vector3 rPositions[4] = {
        Vector3{ rOffset, 0.3f, rOffset },
        Vector3{ -rOffset, 0.3f, rOffset },
        Vector3{ rOffset, 0.3f, -rOffset },
        Vector3{ -rOffset, 0.3f, -rOffset }
    };

    if (visionMode == VisionMode::FLIR_THERMAL) {
        // FLIR: Motor hubs are active white-hot heat emitters!
        for (int i = 0; i < 4; i++) {
            DrawCylinder(rPositions[i], 0.6f, 0.6f, 0.12f, 8, Color{ 255, 250, 225, 255 });
            DrawSphere(rPositions[i], 0.28f, Color{ 255, 255, 240, 255 });
        }
    } else if (visionMode == VisionMode::NVG_GREEN) {
        for (int i = 0; i < 4; i++) {
            DrawCylinder(rPositions[i], 0.6f, 0.6f, 0.1f, 8, Color{ 40, 160, 60, 255 });
            DrawSphere(rPositions[i], 0.22f, Color{ 120, 255, 140, 255 });
        }
    } else {
        for (int i = 0; i < 4; i++) {
            DrawCylinder(rPositions[i], 0.6f, 0.6f, 0.1f, 8, DARKGRAY);
        }
    }

    rlPopMatrix();

    // 1. Strobe Navigation LEDs & Anti-Collision Beacons
    DrawNavigationLEDs(visionMode);

    // 2. Dynamic Ground Spotlight & Volumetric Downlight
    DrawGroundSpotlight(visionMode);

    // Engine exhaust heat aura in FLIR mode
    if (visionMode == VisionMode::FLIR_THERMAL) {
        float pulse = 0.5f + 0.3f * sinf(GetTime() * 10.0f + (float)id);
        DrawCircle3D(Vector3{ position.x, position.y + 0.35f, position.z }, 2.2f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 255, 130, 20, 255 }, pulse * 0.45f));
    }

    if (isFiringLaser) {
        if (visionMode == VisionMode::FLIR_THERMAL) {
            DrawLine3D(position, laserTargetPos, Color{ 255, 255, 255, 255 });
            DrawSphere(laserTargetPos, 0.6f, Color{ 255, 240, 180, 255 });
            DrawCircle3D(laserTargetPos, 1.4f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 255, 120, 0, 255 }, 0.6f));
        } else if (visionMode == VisionMode::NVG_GREEN) {
            DrawLine3D(position, laserTargetPos, Color{ 120, 255, 140, 255 });
            DrawSphere(laserTargetPos, 0.5f, Color{ 80, 255, 100, 255 });
        } else {
            DrawLine3D(position, laserTargetPos, RED);
            DrawSphere(laserTargetPos, 0.5f, RED);
        }
    }

    if (isResupplyBeamActive) {
        if (visionMode == VisionMode::FLIR_THERMAL) {
            DrawLine3D(position, resupplyTargetPos, Color{ 230, 255, 255, 255 });
            DrawSphere(resupplyTargetPos, 0.8f, Color{ 180, 240, 255, 255 });
        } else if (visionMode == VisionMode::NVG_GREEN) {
            DrawLine3D(position, resupplyTargetPos, Color{ 80, 255, 120, 255 });
            DrawSphere(resupplyTargetPos, 0.7f, Color{ 120, 255, 160, 255 });
        } else {
            DrawLine3D(position, resupplyTargetPos, GREEN);
            DrawSphere(resupplyTargetPos, 0.7f, LIME);
        }
    }

    // 3. Defensive Magnesium Flares & Chaff Particles
    DrawFlares(visionMode);

    // 4. Dynamic Atmosphere, Wind & Rotor Downwash Dust FX (Day 7)
    DrawDownwashFX(visionMode);
}

void Drone::DrawNavigationLEDs(VisionMode visionMode) const {
    if (isDestroyed) return;

    float timeSec = (float)GetTime();
    float timeOffset = (float)id * 0.28f;

    // Military aviation double-flash strobe pattern
    bool isEngagedCombat = (role == DroneRole::TASK_FORCE && (state == DroneState::ENGAGING || isFiringLaser));
    bool isResupplying = (role == DroneRole::SUPPORT && state == DroneState::RESUPPLYING_IN_AIR);

    bool strobeBurst = false;
    if (isEngagedCombat) {
        // Rapid combat flashing strobe (6 Hz)
        strobeBurst = (fmodf(timeSec * 6.0f + (float)id * 0.5f, 1.0f) < 0.45f);
    } else if (isResupplying) {
        // Fast dual rescue strobe
        strobeBurst = (fmodf(timeSec * 4.0f, 1.0f) < 0.4f);
    } else {
        // Standard double-flash rhythm
        float cycle = fmodf(timeSec + timeOffset, 1.15f);
        strobeBurst = (cycle < 0.08f) || (cycle > 0.16f && cycle < 0.24f);
    }

    // Arm positions in world coordinates (rotated by yaw)
    float r = 1.3f;
    float radYaw = yaw * (PI / 180.0f);
    float cosY = cosf(radYaw);
    float sinY = sinf(radYaw);

    Vector3 armFL = Vector3{ position.x + (r * cosY - r * sinY), position.y + 0.35f, position.z + (r * sinY + r * cosY) };
    Vector3 armFR = Vector3{ position.x + (-r * cosY - r * sinY), position.y + 0.35f, position.z + (-r * sinY + r * cosY) };
    Vector3 armAL = Vector3{ position.x + (r * cosY + r * sinY), position.y + 0.35f, position.z + (r * sinY - r * cosY) };
    Vector3 armAR = Vector3{ position.x + (-r * cosY + r * sinY), position.y + 0.35f, position.z + (-r * sinY - r * cosY) };
    Vector3 bellyPos = Vector3{ position.x, position.y - 0.32f, position.z };

    // Select color palette by role & vision mode
    Color strobeCol = WHITE;
    Color portCol = Color{ 255, 40, 40, 255 };    // Left: Red
    Color starCol = Color{ 40, 255, 60, 255 };    // Right: Green
    Color aftCol = RAYWHITE;

    if (visionMode == VisionMode::FLIR_THERMAL) {
        strobeCol = Color{ 255, 240, 180, 255 };
        portCol = Color{ 255, 120, 20, 255 };
        starCol = Color{ 255, 160, 30, 255 };
        aftCol = Color{ 255, 220, 150, 255 };
    } else if (visionMode == VisionMode::NVG_GREEN) {
        strobeCol = Color{ 140, 255, 160, 255 };
        portCol = Color{ 80, 255, 100, 255 };
        starCol = Color{ 120, 255, 140, 255 };
        aftCol = Color{ 180, 255, 180, 255 };
    } else {
        switch (role) {
            case DroneRole::RECON:
                strobeCol = Color{ 0, 240, 255, 255 }; // Electric Cyan Scout Strobe
                aftCol = SKYBLUE;
                break;
            case DroneRole::TASK_FORCE:
                strobeCol = isEngagedCombat ? Color{ 255, 20, 20, 255 } : Color{ 255, 140, 0, 255 }; // Crimson Combat or Amber
                aftCol = isEngagedCombat ? RED : ORANGE;
                break;
            case DroneRole::SUPPORT:
                strobeCol = Color{ 50, 255, 90, 255 }; // Emerald Logistics Strobe
                aftCol = LIME;
                break;
        }
    }

    // 1. Static navigation LEDs on arm tips
    DrawSphere(armFL, 0.12f, portCol);
    DrawSphere(armFR, 0.12f, starCol);
    DrawSphere(armAL, 0.12f, aftCol);
    DrawSphere(armAR, 0.12f, aftCol);

    // 2. High-intensity flashing strobe beacon
    if (strobeBurst) {
        DrawSphere(bellyPos, 0.22f, WHITE);
        DrawSphere(bellyPos, 0.70f, Fade(strobeCol, 0.55f));
        DrawSphere(armFL, 0.35f, Fade(portCol, 0.45f));
        DrawSphere(armFR, 0.35f, Fade(starCol, 0.45f));

        // Subtle anti-collision strobe glare line
        DrawLine3D(Vector3{ bellyPos.x - 0.7f, bellyPos.y, bellyPos.z },
                   Vector3{ bellyPos.x + 0.7f, bellyPos.y, bellyPos.z }, Fade(strobeCol, 0.75f));
    }
}

void Drone::DrawGroundSpotlight(VisionMode visionMode) const {
    if (isDestroyed || position.y < 1.2f) return;

    Vector3 groundPos = Vector3{ position.x, 0.05f, position.z };
    float spotRadius = std::clamp(position.y * 0.38f + 3.5f, 4.0f, 22.0f);

    Color spotColor = Color{ 0, 180, 255, 255 };
    if (visionMode == VisionMode::FLIR_THERMAL) {
        spotColor = Color{ 255, 150, 40, 255 };
    } else if (visionMode == VisionMode::NVG_GREEN) {
        spotColor = Color{ 60, 255, 100, 255 };
    } else {
        switch (role) {
            case DroneRole::RECON:      spotColor = Color{ 0, 200, 255, 255 }; break;
            case DroneRole::TASK_FORCE: spotColor = Color{ 255, 130, 20, 255 }; break;
            case DroneRole::SUPPORT:    spotColor = Color{ 50, 240, 90, 255 }; break;
        }
    }

    // 1. Outer Soft Ground Spotlight Disc
    DrawCircle3D(groundPos, spotRadius, Vector3{ 1, 0, 0 }, 90.0f, Fade(spotColor, 0.18f));
    DrawCircle3D(groundPos, spotRadius * 0.5f, Vector3{ 1, 0, 0 }, 90.0f, Fade(spotColor, 0.28f));
    DrawCircle3D(groundPos, spotRadius, Vector3{ 1, 0, 0 }, 90.0f, Fade(spotColor, 0.35f));

    // 2. Subtle Conical Volumetric Light Beam
    Vector3 topLightPos = Vector3{ position.x, position.y - 0.25f, position.z };
    DrawCylinderEx(topLightPos, groundPos, 0.35f, spotRadius, 16, Fade(spotColor, 0.045f));

    // 3. Role-Specific Ground Spotlight FX
    if (role == DroneRole::RECON && state != DroneState::IDLE_AT_BASE) {
        // Rotating tactical recon radar sweep line on ground
        float sweepAngle = GetTime() * 2.8f + (float)id * 1.5f;
        Vector3 sweepEnd = Vector3{ groundPos.x + sinf(sweepAngle) * spotRadius, 0.06f, groundPos.z + cosf(sweepAngle) * spotRadius };
        DrawLine3D(groundPos, sweepEnd, Fade(spotColor, 0.60f));
        DrawCircle3D(groundPos, spotRadius * 0.25f, Vector3{ 1, 0, 0 }, 90.0f, Fade(WHITE, 0.35f));
    } else if (role == DroneRole::TASK_FORCE) {
        // Combat target spotlight designation when engaging
        if (isFiringLaser && laserTargetPos.y >= 0.0f) {
            Vector3 tgtGround = Vector3{ laserTargetPos.x, 0.08f, laserTargetPos.z };
            DrawCircle3D(tgtGround, 4.5f, Vector3{ 1, 0, 0 }, 90.0f, Fade(RED, 0.55f));
            DrawCircle3D(tgtGround, 2.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(GOLD, 0.70f));
            DrawLine3D(position, tgtGround, Fade(Color{ 255, 60, 40, 255 }, 0.30f));
        }
    } else if (role == DroneRole::SUPPORT) {
        // Expanding safe-zone halo when loitering or resupplying
        if (state == DroneState::RESUPPLYING_IN_AIR || state == DroneState::LOITERING_IN_AREA) {
            float pulseRadius = fmodf(GetTime() * 12.0f, spotRadius * 1.3f);
            DrawCircle3D(groundPos, pulseRadius, Vector3{ 1, 0, 0 }, 90.0f, Fade(spotColor, 0.45f * (1.0f - pulseRadius / (spotRadius * 1.3f))));
            DrawCircle3D(groundPos, spotRadius * 1.2f, Vector3{ 1, 0, 0 }, 90.0f, Fade(LIME, 0.30f));
        }
    }
}

void Drone::DeployFlares(FleetContext* ctx) {
    if (isDestroyed || flareCooldown > 0.0f) return;

    flareCooldown = 4.5f;   // 4.5 sec tactical reload/recharge cooldown
    flareAlertTimer = 2.5f; // 2.5 sec active evasion & HUD alert banner

    LogAction("Deployed Defensive Magnesium Flare Salvo (Countermeasure)");
    if (ctx) {
        ctx->telemetry.flaresFiredCount += 16;
        char buf[128];
        snprintf(buf, sizeof(buf), "[DEFENSE] Drone #%d DEPLOYED FLARES (Hostile Lock Decoyed)!", id);
        ctx->LogTerminal(buf);
    }

    if (IsAudioDeviceReady()) {
        AudioManager::Instance().PlayFlare();
    }

    float radYaw = yaw * (PI / 180.0f);
    Vector3 fwd = Vector3{ sinf(radYaw), 0.0f, cosf(radYaw) };
    Vector3 right = Vector3{ cosf(radYaw), 0.0f, -sinf(radYaw) };
    Vector3 up = Vector3{ 0.0f, 1.0f, 0.0f };

    // Lateral flare ejection pods: Left & Right wings (8 particles per side = 16 total)
    for (int side = -1; side <= 1; side += 2) {
        Vector3 podPos = Vector3Add(position, Vector3Scale(right, (float)side * 1.1f));
        podPos.y += 0.15f;

        for (int i = 0; i < 8; i++) {
            CountermeasureParticle p;
            p.position = podPos;

            float speedSpread = (float)GetRandomValue(14, 22);
            float lateralVariance = (float)GetRandomValue(-15, 15) * 0.01f;
            float rearVariance = (float)GetRandomValue(20, 60) * -0.01f; // eject angled rearwards
            float upVariance = (float)GetRandomValue(15, 55) * 0.01f;   // eject slightly upwards

            Vector3 ejectDir = Vector3Normalize(Vector3Add(
                Vector3Scale(right, (float)side + lateralVariance),
                Vector3Add(Vector3Scale(fwd, rearVariance), Vector3Scale(up, upVariance))
            ));

            // Combined ejection velocity with drone inertia
            p.velocity = Vector3Add(Vector3Scale(ejectDir, speedSpread), Vector3Scale(velocity, 0.40f));
            p.life = 0.0f;
            p.maxLife = (float)GetRandomValue(12, 20) * 0.1f; // 1.2s - 2.0s burn time
            p.size = (float)GetRandomValue(30, 48) * 0.01f;
            p.color = (GetRandomValue(0, 10) > 3) ? GOLD : WHITE;
            p.isSmoke = false;
            p.isAlive = true;

            flareParticles.push_back(p);
        }
    }
}

void Drone::UpdateFlares(float dt) {
    if (flareCooldown > 0.0f) flareCooldown -= dt;
    if (flareAlertTimer > 0.0f) flareAlertTimer -= dt;

    if (flareParticles.empty()) return;

    std::vector<CountermeasureParticle> newSmoke;

    for (auto& p : flareParticles) {
        if (!p.isAlive) continue;

        p.life += dt;
        if (p.life >= p.maxLife) {
            p.isAlive = false;
            continue;
        }

        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));

        if (!p.isSmoke) {
            p.velocity = Vector3Scale(p.velocity, 0.92f); // Aerodynamic drag
            p.velocity.y -= 7.5f * dt;                     // Gravity

            // Spawn lingering smoke puff trail behind burning flare
            if (GetRandomValue(0, 100) < 28) {
                CountermeasureParticle s;
                s.position = p.position;
                s.velocity = Vector3Scale(p.velocity, 0.12f);
                s.life = 0.0f;
                s.maxLife = 0.85f;
                s.size = p.size * 1.5f;
                s.color = Fade(LIGHTGRAY, 0.35f);
                s.isSmoke = true;
                s.isAlive = true;
                newSmoke.push_back(s);
            }
        } else {
            p.velocity = Vector3Scale(p.velocity, 0.88f);
            p.velocity.y += 0.6f * dt; // Smoke buoyancy
            p.size += dt * 0.35f;      // Smoke expands
        }
    }

    for (const auto& s : newSmoke) {
        flareParticles.push_back(s);
    }

    flareParticles.erase(std::remove_if(flareParticles.begin(), flareParticles.end(), [](const CountermeasureParticle& p) {
        return !p.isAlive;
    }), flareParticles.end());
}

void Drone::DrawFlares(VisionMode visionMode) const {
    if (flareParticles.empty()) return;

    for (const auto& p : flareParticles) {
        if (!p.isAlive) continue;

        float alpha = std::clamp(1.0f - (p.life / p.maxLife), 0.0f, 1.0f);

        if (p.isSmoke) {
            if (visionMode == VisionMode::FLIR_THERMAL) {
                DrawSphere(p.position, p.size, Fade(Color{ 80, 50, 35, 255 }, alpha * 0.30f));
            } else if (visionMode == VisionMode::NVG_GREEN) {
                DrawSphere(p.position, p.size, Fade(Color{ 30, 80, 40, 255 }, alpha * 0.25f));
            } else {
                DrawSphere(p.position, p.size, Fade(DARKGRAY, alpha * 0.25f));
            }
        } else {
            if (visionMode == VisionMode::FLIR_THERMAL) {
                // Intense White-Hot Thermal Emitter Signature
                DrawSphere(p.position, p.size * 0.7f, Color{ 255, 255, 255, 255 });
                DrawSphere(p.position, p.size * 1.5f, Fade(Color{ 255, 240, 180, 255 }, alpha * 0.85f));
                DrawCircle3D(p.position, p.size * 3.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 255, 130, 20, 255 }, alpha * 0.60f));
            } else if (visionMode == VisionMode::NVG_GREEN) {
                // NVG High-Intensity Phosphor Flare Bloom
                DrawSphere(p.position, p.size * 0.65f, Color{ 240, 255, 240, 255 });
                DrawSphere(p.position, p.size * 1.4f, Fade(Color{ 140, 255, 160, 255 }, alpha * 0.80f));
                DrawCircle3D(p.position, p.size * 2.6f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 60, 255, 100, 255 }, alpha * 0.45f));
            } else {
                // Normal Daylight: Sparking Golden / Magnesium Combustion
                DrawSphere(p.position, p.size * 0.65f, WHITE);
                DrawSphere(p.position, p.size * 1.25f, Fade(GOLD, alpha * 0.90f));
                DrawSphere(p.position, p.size * 2.1f, Fade(ORANGE, alpha * 0.45f));
                DrawCircle3D(p.position, p.size * 2.6f, Vector3{ 1, 0, 0 }, 90.0f, Fade(YELLOW, alpha * 0.40f));
            }
        }
    }
}

void Drone::UpdateDownwash(float dt) {
    if (isDestroyed) {
        if (!downwashParticles.empty()) {
            for (auto& p : downwashParticles) {
                p.life += dt;
                if (p.life >= p.maxLife) p.isAlive = false;
                p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
            }
            downwashParticles.erase(std::remove_if(downwashParticles.begin(), downwashParticles.end(), [](const DownwashParticle& p) {
                return !p.isAlive;
            }), downwashParticles.end());
        }
        return;
    }

    // Dynamic Atmospheric Wind Vector
    Vector3 wind = { sinf((float)GetTime() * 0.5f) * 3.0f, 0.0f, cosf((float)GetTime() * 0.3f) * 2.0f };

    // Downwash generation when drone is low altitude (y < 8.0m)
    if (position.y < 8.0f) {
        float downwashIntensity = std::clamp((8.0f - position.y) / 8.0f, 0.0f, 1.0f);
        
        // Spawn radial ground dust particles
        int spawnCount = (downwashIntensity > 0.4f) ? 2 : 1;
        if (state == DroneState::IDLE_AT_BASE) spawnCount = 0; // Resting on dock

        for (int i = 0; i < spawnCount; i++) {
            if (downwashParticles.size() >= 80) break;

            float ang = (float)GetRandomValue(0, 360) * DEG2RAD;
            float r = (float)GetRandomValue(8, 28) * 0.1f * (1.0f + (1.0f - downwashIntensity));
            float radSpeed = (float)GetRandomValue(20, 45) * 0.1f * downwashIntensity;

            DownwashParticle p;
            p.position = Vector3{
                position.x + cosf(ang) * r,
                0.04f + (float)GetRandomValue(0, 20) * 0.01f,
                position.z + sinf(ang) * r
            };

            Vector3 radialDir = Vector3{ cosf(ang), (float)GetRandomValue(5, 30) * 0.02f * downwashIntensity, sinf(ang) };
            p.velocity = Vector3Add(
                Vector3Scale(radialDir, radSpeed * 2.8f),
                Vector3Add(Vector3Scale(wind, 0.85f), Vector3Scale(velocity, 0.25f))
            );

            p.life = 0.0f;
            p.maxLife = 0.5f + (float)GetRandomValue(20, 60) * 0.01f;
            p.size = 0.14f + (float)GetRandomValue(0, 16) * 0.01f;
            p.color = Color{ 210, 185, 140, 255 };
            p.isAlive = true;

            downwashParticles.push_back(p);
        }
    }

    // Update active downwash dust particles
    for (auto& p : downwashParticles) {
        if (!p.isAlive) continue;

        p.life += dt;
        if (p.life >= p.maxLife) {
            p.isAlive = false;
            continue;
        }

        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.velocity = Vector3Scale(p.velocity, 0.90f); // Ground aerodynamic drag
        p.velocity = Vector3Add(p.velocity, Vector3Scale(wind, dt * 1.8f)); // Atmospheric wind drift
        p.size += dt * 0.28f; // Dust cloud expands as it dissipates
    }

    downwashParticles.erase(std::remove_if(downwashParticles.begin(), downwashParticles.end(), [](const DownwashParticle& p) {
        return !p.isAlive;
    }), downwashParticles.end());
}

void Drone::DrawDownwashFX(VisionMode visionMode) const {
    if (isDestroyed || (position.y >= 8.0f && downwashParticles.empty())) return;

    float downwashAlpha = std::clamp((8.0f - position.y) / 8.0f, 0.0f, 1.0f);
    Vector3 wind = { sinf((float)GetTime() * 0.5f) * 3.0f, 0.0f, cosf((float)GetTime() * 0.3f) * 2.0f };

    if (position.y < 8.0f && state != DroneState::IDLE_AT_BASE) {
        // Ground epicenter with atmospheric wind shear offset
        float windShear = (1.0f - downwashAlpha * 0.5f);
        Vector3 groundCenter = Vector3{
            position.x + wind.x * 0.25f * windShear,
            0.03f,
            position.z + wind.z * 0.25f * windShear
        };

        float maxRadius = 3.0f + (1.0f - downwashAlpha) * 4.5f;

        // 1. Concentric Radial Ground Dust Shockwaves
        for (int i = 0; i < 3; i++) {
            float phase = fmodf((float)GetTime() * 3.2f + (float)i * 0.33f + (float)id * 0.4f, 1.0f);
            float ringRadius = 0.8f + phase * maxRadius;
            float ringAlpha = downwashAlpha * (1.0f - phase) * 0.60f;

            Color ringCol = Color{ 210, 185, 140, 255 };
            if (visionMode == VisionMode::FLIR_THERMAL) {
                ringCol = Color{ 240, 130, 30, 255 }; // Thermal ground friction heat wash
            } else if (visionMode == VisionMode::NVG_GREEN) {
                ringCol = Color{ 80, 220, 110, 255 }; // Phosphor illuminated dust wave
            }

            DrawCircle3D(groundCenter, ringRadius, Vector3{ 1, 0, 0 }, 90.0f, Fade(ringCol, ringAlpha * 0.45f));
            DrawCircle3D(groundCenter, ringRadius * 0.88f, Vector3{ 1, 0, 0 }, 90.0f, Fade(ringCol, ringAlpha * 0.18f));
        }

        // 2. High-Pressure Downward Rotor Core Ground Footprint
        Color coreCol = Color{ 190, 160, 120, 255 };
        if (visionMode == VisionMode::FLIR_THERMAL) coreCol = Color{ 255, 150, 40, 255 };
        else if (visionMode == VisionMode::NVG_GREEN) coreCol = Color{ 100, 240, 130, 255 };

        DrawCircle3D(groundCenter, 2.2f * downwashAlpha + 0.5f, Vector3{ 1, 0, 0 }, 90.0f, Fade(coreCol, downwashAlpha * 0.25f));
    }

    // 3. Dynamic Micro-Particle Swirl Cloud
    for (const auto& p : downwashParticles) {
        if (!p.isAlive) continue;

        float alpha = std::clamp(1.0f - (p.life / p.maxLife), 0.0f, 1.0f) * downwashAlpha;
        Color pCol = p.color;

        if (visionMode == VisionMode::FLIR_THERMAL) {
            pCol = Color{ 245, 140, 35, 255 };
        } else if (visionMode == VisionMode::NVG_GREEN) {
            pCol = Color{ 70, 210, 100, 255 };
        }

        DrawSphere(p.position, p.size, Fade(pCol, alpha * 0.55f));
    }
}

// ==========================================
// RECON DRONE IMPLEMENTATION
// ==========================================

ReconDrone::ReconDrone(int id, Vector3 dockPos, int startingSector)
    : Drone(id, DroneRole::RECON), reconSectorIndex(startingSector) {
    baseDockPos = dockPos;
    position = baseDockPos;
    targetPosition = position;
    maxFuel = 200.0f;
    fuel = 200.0f;
    maxAmmo = 0.0f;
    ammo = 0.0f;
    maxHp = 160.0f;
    hp = 160.0f;
    LogAction("Docked at Hangar Bay #" + std::to_string(id) + " (4x Speed, High-Altitude 50m Sweep, 160 HP)");
}

void ReconDrone::Update(FleetContext& ctx, float dt) {
    UpdateFlares(dt);
    UpdateDownwash(dt);

    if (isDestroyed) {
        if (position.y > 0.4f) {
            position.y += velocity.y * dt;
            if (position.y < 0.4f) position.y = 0.4f;
        }
        return;
    }

    if (state == DroneState::RETURNING_TO_BASE) {
        targetPosition = baseDockPos;
        UpdateMovementPhysics(32.0f, dt);
        ApplyFlankingAndSeparation(ctx, dt);
        return;
    }

    if (state == DroneState::IDLE_AT_BASE) {
        if (fuel >= 195.0f && hp >= 120.0f) {
            state = DroneState::PATROL;
            hasRendezvous = false;
            assignedSupportId = -1;
            LogAction("Launched on High-Altitude (50m) Grid Sweep (4x Speed).");
            ctx.LogTerminal("[RECON #" + std::to_string(id) + "] Launched on High-Altitude (50m) Grid Sector Sweep.");
        }
        UpdateMovementPhysics(32.0f, dt);
        return;
    }

    fuel -= dt * 0.8f;
    ctx.telemetry.totalFuelBurned += dt * 0.8f;

    // --- LOW FUEL: INITIATE AERIAL RENDEZVOUS WITH SUPPORT ---
    if (fuel <= 60.0f && state != DroneState::RETURNING_TO_BASE && state != DroneState::RESUPPLYING_IN_AIR) {
        if (!hasRendezvous && assignedSupportId == -1) {
            Drone* availSupport = nullptr;
            for (auto& s : ctx.drones) {
                if (s->role == DroneRole::SUPPORT && !s->isDestroyed) {
                    SupportDrone* sup = dynamic_cast<SupportDrone*>(s.get());
                    if (sup && sup->cargoFuel > 50.0f && (sup->state == DroneState::IDLE_AT_BASE || sup->state == DroneState::PATROL || sup->state == DroneState::LOITERING_IN_AREA)) {
                        availSupport = sup;
                        break;
                    }
                }
            }

            if (availSupport != nullptr) {
                assignedSupportId = availSupport->id;
                SupportDrone* sup = dynamic_cast<SupportDrone*>(availSupport);
                if (sup) sup->servicingReconId = id;

                Vector3 rendezvousPoint = Vector3{
                    (position.x + ctx.hangarBase.GetPosition().x) * 0.5f,
                    22.0f,
                    (position.z + ctx.hangarBase.GetPosition().z) * 0.5f
                };

                rendezvousPos = rendezvousPoint;
                hasRendezvous = true;
                state = DroneState::REQUESTING_SUPPORT;
                targetPosition = rendezvousPoint;

                availSupport->rendezvousPos = rendezvousPoint;
                availSupport->hasRendezvous = true;
                availSupport->state = DroneState::MOVING_TO_TARGET;
                availSupport->targetPosition = rendezvousPoint;

                char buf[180];
                snprintf(buf, sizeof(buf), "[RECON #%d] Low Fuel (%.0f%%)! Initiating Mid-Air Rendezvous with Support #%d at (%.0f, %.0f)",
                         id, (fuel / maxFuel) * 100.0f, availSupport->id, rendezvousPoint.x, rendezvousPoint.z);
                ctx.LogTerminal(buf);
                LogAction("Low fuel! Flying to Rendezvous Point for aerial refuel by Support #" + std::to_string(availSupport->id));
                availSupport->LogAction("Dispatched to Mid-Air Rendezvous Point to refuel Recon #" + std::to_string(id));
            } else {
                state = DroneState::RETURNING_TO_BASE;
                targetPosition = baseDockPos;
                LogAction("Low fuel (<30%), No Support available. RTB to Hangar.");
                ctx.LogTerminal("[RECON #" + std::to_string(id) + "] Low fuel warning. No Support available -> RTB to Hangar.");
            }
            UpdateMovementPhysics(32.0f, dt);
            ApplyFlankingAndSeparation(ctx, dt);
            return;
        }
    }

    if (state == DroneState::REQUESTING_SUPPORT || state == DroneState::RESUPPLYING_IN_AIR) {
        targetPosition = rendezvousPos;
        UpdateMovementPhysics(32.0f, dt);
        ApplyFlankingAndSeparation(ctx, dt);
        return;
    }

    // --- RECON RADAR SCANNING OF CLUSTERS ---
    EnemyCluster* detectedCluster = nullptr;
    for (auto& c : ctx.clusterManager.GetClusters()) {
        if (c.status == ClusterStatus::UNDETECTED || c.status == ClusterStatus::REQUIRES_RECON) {
            float dist = Vector3Distance(position, c.centerPosition);
            if (dist <= 140.0f) {
                detectedCluster = &c;
                break;
            }
        }
    }

    if (detectedCluster != nullptr) {
        targetPosition = Vector3{ detectedCluster->centerPosition.x, 35.0f, detectedCluster->centerPosition.z };
        float dist = Vector3Distance(position, detectedCluster->centerPosition);

        if (dist <= 70.0f) {
            state = DroneState::SCANNING;
            scanTimer += dt;
            if (scanTimer >= 0.8f) {
                detectedCluster->status = ClusterStatus::REPORTED;
                scanTimer = 0.0f;
                
                int aliveArmed = 0;
                int aliveUnarmed = 0;
                for (const auto& e : detectedCluster->enemies) {
                    if (e.isAlive) {
                        if (e.isArmed) aliveArmed++;
                        else aliveUnarmed++;
                    }
                }
                detectedCluster->armedCount = aliveArmed;
                detectedCluster->unarmedCount = aliveUnarmed;
                detectedCluster->calculatedThreatMagnitude = (int)ceilf((float)aliveArmed * 0.5f + (float)aliveUnarmed * 0.25f);

                char buf[180];
                snprintf(buf, sizeof(buf), "[RECON #%d] Cluster #%d VERIFIED at 50%% Radar Radius! Threat Mag: %d (%d Armed, %d Unarmed)",
                         id, detectedCluster->id, detectedCluster->calculatedThreatMagnitude, aliveArmed, aliveUnarmed);
                ctx.LogTerminal(buf);
                LogAction("Verified Hostile Cluster #" + std::to_string(detectedCluster->id) + " (Threat Mag: " + std::to_string(detectedCluster->calculatedThreatMagnitude) + ")");
                if (IsAudioDeviceReady()) AudioManager::Instance().PlayCollect();
            }
        } else {
            state = DroneState::MOVING_TO_TARGET;
        }
    } else {
        state = DroneState::PATROL;

        // High Altitude (50m) Grid Sweep Waypoints
        Vector3 gridWaypoints[16] = {
            { -270.0f, 50.0f, -270.0f }, { -90.0f, 50.0f, -270.0f }, { 90.0f, 50.0f, -270.0f }, { 270.0f, 50.0f, -270.0f },
            { 270.0f, 50.0f, -90.0f },  { 90.0f, 50.0f, -90.0f },   { -90.0f, 50.0f, -90.0f }, { -270.0f, 50.0f, -90.0f },
            { -270.0f, 50.0f, 90.0f },  { -90.0f, 50.0f, 90.0f },   { 90.0f, 50.0f, 90.0f },   { 270.0f, 50.0f, 90.0f },
            { 270.0f, 50.0f, 270.0f },  { 90.0f, 50.0f, 270.0f },   { -90.0f, 50.0f, 270.0f }, { -270.0f, 50.0f, 270.0f }
        };

        Vector3 wp = gridWaypoints[reconSectorIndex % 16];
        targetPosition = wp;

        if (Vector3Distance(position, wp) < 25.0f) {
            reconSectorIndex = (reconSectorIndex + 1) % 16;
            LogAction("High-Altitude Sweeping Sector #" + std::to_string(reconSectorIndex + 1));
        }
    }

    UpdateMovementPhysics(32.0f, dt);
    ApplyFlankingAndSeparation(ctx, dt);
}

void ReconDrone::Draw3D(VisionMode visionMode) const {
    Drone::Draw3D(visionMode);
    if (!isDestroyed && state != DroneState::IDLE_AT_BASE) {
        if (visionMode == VisionMode::FLIR_THERMAL) {
            DrawCircle3D(position, 140.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 100, 200, 255, 255 }, 0.25f));
            DrawCircle3D(position, 140.5f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 140, 220, 255, 200 });
            DrawCircle3D(position, 70.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 255, 180, 50, 255 }, 0.30f));
        } else if (visionMode == VisionMode::NVG_GREEN) {
            DrawCircle3D(position, 140.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 50, 255, 100, 255 }, 0.25f));
            DrawCircle3D(position, 140.5f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 80, 255, 120, 200 });
            DrawCircle3D(position, 70.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 180, 255, 80, 255 }, 0.30f));
        } else {
            DrawCircle3D(position, 140.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(SKYBLUE, 0.35f));
            DrawCircle3D(position, 140.5f, Vector3{ 1, 0, 0 }, 90.0f, SKYBLUE);
            DrawCircle3D(position, 70.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(GOLD, 0.40f));
        }
    }
}

// ==========================================
// TASK FORCE DRONE IMPLEMENTATION
// ==========================================

TaskForceDrone::TaskForceDrone(int id, Vector3 dockPos)
    : Drone(id, DroneRole::TASK_FORCE) {
    baseDockPos = dockPos;
    position = baseDockPos;
    targetPosition = position;
    maxFuel = 100.0f;
    fuel = 100.0f;
    maxAmmo = 60.0f;
    ammo = 60.0f;
    maxHp = 200.0f;
    hp = 200.0f;
    LogAction("Docked at Hangar Bay #" + std::to_string(id) + " (2x Speed, Pincer Strike, Micro-Missiles, 200 HP)");
}

void TaskForceDrone::Update(FleetContext& ctx, float dt) {
    UpdateFlares(dt);
    UpdateDownwash(dt);

    if (isDestroyed) {
        if (position.y > 0.4f) {
            position.y += velocity.y * dt;
            if (position.y < 0.4f) position.y = 0.4f;
        }
        return;
    }

    if (missileCooldown > 0.0f) {
        missileCooldown -= dt;
    }

    if (state != DroneState::IDLE_AT_BASE) {
        fuel -= dt * 1.0f;
        ctx.telemetry.totalFuelBurned += dt * 1.0f;
    }

    // Critical Emergency Trigger
    if (hp <= (maxHp * 0.5f) && flareCooldown <= 0.0f) {
        DeployFlares(&ctx);
    }

    if (fuel <= 12.0f || (ammo <= 0.0f && state != DroneState::RESUPPLYING_IN_AIR) || hp <= 25.0f) {
        if (state != DroneState::RETURNING_TO_BASE) {
            state = DroneState::RETURNING_TO_BASE;
            targetPosition = baseDockPos;
            assignedClusterId = -1;
            isFiringLaser = false;
            LogAction("Resource Low/Damage! RTB to Hangar.");
        }
    }

    // Support Request Trigger
    if (fuel <= 45.0f || ammo <= 25.0f || hp <= 120.0f) {
        if (state != DroneState::REQUESTING_SUPPORT && state != DroneState::RESUPPLYING_IN_AIR && state != DroneState::RETURNING_TO_BASE) {
            state = DroneState::REQUESTING_SUPPORT;
            isFiringLaser = false;

            Vector3 midPoint = Vector3{
                (position.x + ctx.hangarBase.GetPosition().x) * 0.5f,
                20.0f,
                (position.z + ctx.hangarBase.GetPosition().z) * 0.5f
            };
            rendezvousPos = midPoint;
            hasRendezvous = true;
            targetPosition = midPoint;

            LogAction("Requesting Midpoint Support Tanker (F:" + std::to_string((int)fuel) + "%, A:" + std::to_string((int)ammo) + "%, HP:" + std::to_string((int)hp) + ")");
            ctx.LogTerminal("[TF #" + std::to_string(id) + "] Low resources! Flying to Midpoint Rendezvous to meet Support.");
        }
    }

    if (state == DroneState::REQUESTING_SUPPORT) {
        targetPosition = rendezvousPos;

        // Check if any support tanker has cargo and can service
        bool supportAvailable = false;
        for (const auto& s : ctx.drones) {
            if (s->role == DroneRole::SUPPORT && !s->isDestroyed) {
                const SupportDrone* sup = dynamic_cast<const SupportDrone*>(s.get());
                if (sup && (sup->cargoFuel > 20.0f || sup->cargoAmmo > 20.0f)) {
                    supportAvailable = true;
                    break;
                }
            }
        }

        // If no support is available or fuel drops critically low while waiting, RTB directly
        if (!supportAvailable || fuel <= 20.0f) {
            state = DroneState::RETURNING_TO_BASE;
            targetPosition = baseDockPos;
            hasRendezvous = false;
            assignedSupportId = -1;
            LogAction("No Support Tanker available / Low fuel. RTB to Hangar.");
            ctx.LogTerminal("[TF #" + std::to_string(id) + "] No Support Tanker available -> RTB to Hangar.");
        }
    } else if (state == DroneState::RESUPPLYING_IN_AIR) {
        targetPosition = rendezvousPos;
    }

    EnemyCluster* targetCluster = nullptr;
    for (auto& c : ctx.clusterManager.GetClusters()) {
        if (c.id == assignedClusterId && c.status != ClusterStatus::CLEARED) {
            targetCluster = &c;
            break;
        }
    }

    // Auto-dispatch / Mid-air retask
    if (!targetCluster) {
        for (auto& c : ctx.clusterManager.GetClusters()) {
            if ((c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) && c.calculatedThreatMagnitude > 0) {
                int currentlyAssigned = 0;
                for (const auto& other : ctx.drones) {
                    if (other->role == DroneRole::TASK_FORCE && other->assignedClusterId == c.id && !other->isDestroyed) {
                        currentlyAssigned++;
                    }
                }

                int requiredDispatch = std::clamp(c.calculatedThreatMagnitude, 1, 5);
                if (currentlyAssigned < requiredDispatch) {
                    bool isEligibleForDispatch = (state == DroneState::IDLE_AT_BASE);
                    bool isEligibleForMidAirRetask = (state != DroneState::ENGAGING && fuel >= 35.0f && ammo >= 15.0f && hp >= 60.0f);

                    if (isEligibleForDispatch || isEligibleForMidAirRetask) {
                        targetCluster = &c;
                        assignedClusterId = c.id;
                        state = DroneState::MOVING_TO_TARGET;
                        c.status = ClusterStatus::ENGAGED;

                        // Mandatory Support Escort for Armed Hostiles
                        if (c.armedCount > 0) {
                            for (auto& s : ctx.drones) {
                                if (s->role == DroneRole::SUPPORT && s->state == DroneState::IDLE_AT_BASE && !s->isDestroyed) {
                                    s->assignedClusterId = c.id;
                                    s->state = DroneState::MOVING_TO_TARGET;
                                    s->LogAction("Deploying as COMBAT ESCORT for Strike Group at Cluster #" + std::to_string(c.id));
                                    ctx.LogTerminal("[ARMED ESCORT] Support #" + std::to_string(s->id) + " dispatched alongside Strike Group against Armed Hostiles at Cluster #" + std::to_string(c.id));
                                    break;
                                }
                            }
                        }

                        char buf[180];
                        if (isEligibleForMidAirRetask && !isEligibleForDispatch) {
                            snprintf(buf, sizeof(buf), "[MID-AIR RETASK] TF #%d (F:%.0f%%, A:%.0f%%) U-turned mid-air -> Reassigned to Cluster #%d (Threat Mag: %d)",
                                     id, fuel, ammo, c.id, c.calculatedThreatMagnitude);
                            LogAction("Retasked mid-air to Cluster #" + std::to_string(c.id) + " (Threat Mag: " + std::to_string(c.calculatedThreatMagnitude) + ")");
                        } else {
                            snprintf(buf, sizeof(buf), "[STRIKE DISPATCH] TF #%d launched from Hangar -> Cluster #%d (Threat Mag: %d | Interceptors: %d/%d)",
                                     id, c.id, c.calculatedThreatMagnitude, currentlyAssigned + 1, requiredDispatch);
                            LogAction("Dispatched to Cluster #" + std::to_string(c.id) + " (Threat Mag: " + std::to_string(c.calculatedThreatMagnitude) + ")");
                        }
                        ctx.LogTerminal(buf);
                        break;
                    }
                }
            }
        }
    }

    if (state == DroneState::RETURNING_TO_BASE && !targetCluster) {
        targetPosition = baseDockPos;
        isFiringLaser = false;
        UpdateMovementPhysics(16.0f, dt);
        ApplyFlankingAndSeparation(ctx, dt);
        return;
    }

    // --- PINCER / KUŞATMA FORMATION & COMBAT ENGAGEMENT ---
    if (targetCluster && targetCluster->calculatedThreatMagnitude > 0 && state != DroneState::REQUESTING_SUPPORT && state != DroneState::RESUPPLYING_IN_AIR) {
        // Calculate squad pincer index
        std::vector<int> squadTfIds;
        for (const auto& other : ctx.drones) {
            if (other->role == DroneRole::TASK_FORCE && other->assignedClusterId == targetCluster->id && !other->isDestroyed) {
                squadTfIds.push_back(other->id);
            }
        }
        std::sort(squadTfIds.begin(), squadTfIds.end());

        int mySquadIdx = 0;
        for (size_t i = 0; i < squadTfIds.size(); i++) {
            if (squadTfIds[i] == id) {
                mySquadIdx = (int)i;
                break;
            }
        }

        int totalInSquad = std::max(1, (int)squadTfIds.size());
        float angleStep = (2.0f * PI) / (float)totalInSquad;
        float baseAngle = atan2f(targetCluster->centerPosition.x - ctx.hangarBase.GetPosition().x, targetCluster->centerPosition.z - ctx.hangarBase.GetPosition().z);
        float pincerAngle = baseAngle + (float)mySquadIdx * angleStep;

        float standoffRadius = 32.0f;
        float tacticalAltitude = 15.0f + (float)(mySquadIdx % 3) * 3.0f; // Tactical altitude layer (15m - 21m)

        targetPosition = Vector3{
            targetCluster->centerPosition.x + sinf(pincerAngle) * standoffRadius,
            tacticalAltitude,
            targetCluster->centerPosition.z + cosf(pincerAngle) * standoffRadius
        };

        float dist = Vector3Distance(position, targetCluster->centerPosition);

        if (dist < 110.0f && ammo > 0.0f) {
            state = DroneState::ENGAGING;
            ammo -= dt * 3.0f;

            Enemy* closestEnemy = nullptr;
            float minDist = 9999.0f;
            for (auto& e : targetCluster->enemies) {
                if (e.isAlive) {
                    float ed = Vector3Distance(position, e.position);
                    if (e.isArmed) ed -= 50.0f; // Priority on armed enemies

                    if (ed < minDist) {
                        minDist = ed;
                        closestEnemy = &e;
                    }
                }
            }

            if (closestEnemy) {
                isFiringLaser = true;
                laserTargetPos = closestEnemy->position;
                closestEnemy->hp -= dt * 35.0f;

                // --- BALLISTIC MICRO-MISSILE SALVO ---
                if (missileCooldown <= 0.0f && ammo >= 5.0f) {
                    missileCooldown = 1.2f + (float)GetRandomValue(0, 5) * 0.1f;
                    ammo -= 2.0f;
                    Vector3 launchPos = Vector3{ position.x, position.y - 0.4f, position.z };
                    ctx.missileSystem.LaunchMissile(id, launchPos, closestEnemy->position, targetCluster->id, closestEnemy->id);
                    ctx.telemetry.missilesFiredCount++;
                }

                closestEnemy->panicTimer = 3.0f;
                closestEnemy->fleeTarget = position;

                for (auto& sq : targetCluster->enemies) {
                    if (sq.isAlive && Vector3Distance(sq.position, closestEnemy->position) < 18.0f) {
                        sq.panicTimer = 2.5f;
                        sq.fleeTarget = position;
                    }
                }

                if (closestEnemy->hp <= 0.0f) {
                    closestEnemy->hp = 0.0f;
                    closestEnemy->isAlive = false;
                    targetCluster->remainingMagnitude--;
                    ctx.telemetry.eliminatedTargetsCount++;

                    int aliveArmed = 0;
                    int aliveUnarmed = 0;
                    for (const auto& e : targetCluster->enemies) {
                        if (e.isAlive) {
                            if (e.isArmed) aliveArmed++;
                            else aliveUnarmed++;
                        }
                    }
                    targetCluster->armedCount = aliveArmed;
                    targetCluster->unarmedCount = aliveUnarmed;
                    targetCluster->calculatedThreatMagnitude = (int)ceilf((float)aliveArmed * 0.5f + (float)aliveUnarmed * 0.25f);

                    char buf[160];
                    snprintf(buf, sizeof(buf), "[TASK FORCE #%d] %s Target Neutralized! Cluster #%d Threat Mag: %d (%d Armed, %d Unarmed)",
                             id, closestEnemy->isArmed ? "ARMED" : "UNARMED", targetCluster->id, targetCluster->calculatedThreatMagnitude, aliveArmed, aliveUnarmed);
                    ctx.LogTerminal(buf);
                    LogAction("Neutralized " + std::string(closestEnemy->isArmed ? "Armed" : "Unarmed") + " target in Cluster #" + std::to_string(targetCluster->id));

                    if (IsAudioDeviceReady()) AudioManager::Instance().PlayHurt();
                }
            } else {
                isFiringLaser = false;
            }
        } else {
            state = DroneState::MOVING_TO_TARGET;
            isFiringLaser = false;
        }
    } else {
        isFiringLaser = false;
        if (state != DroneState::REQUESTING_SUPPORT && state != DroneState::RESUPPLYING_IN_AIR) {
            if (state != DroneState::IDLE_AT_BASE) {
                state = DroneState::RETURNING_TO_BASE;
                targetPosition = baseDockPos;
            }
        }
    }

    UpdateMovementPhysics(16.0f, dt);
    ApplyFlankingAndSeparation(ctx, dt);
}

void TaskForceDrone::Draw3D(VisionMode visionMode) const {
    Drone::Draw3D(visionMode);
}

// ==========================================
// SUPPORT DRONE IMPLEMENTATION
// ==========================================

SupportDrone::SupportDrone(int id, Vector3 dockPos)
    : Drone(id, DroneRole::SUPPORT) {
    baseDockPos = dockPos;
    position = baseDockPos;
    targetPosition = position;
    maxFuel = 400.0f;
    fuel = 400.0f;
    maxAmmo = 0.0f;
    ammo = 0.0f;
    maxHp = 300.0f;
    hp = 300.0f;
    cargoFuel = 600.0f;
    cargoAmmo = 240.0f;
    loiterCenterPos = Vector3{ 0.0f, 34.0f, 0.0f };
    LogAction("Docked at Hangar Bay #" + std::to_string(id) + " (2x Speed Buff, 4x Fuel, Forward Loitering, 300 HP)");
}

void SupportDrone::ResetToDock() {
    Drone::ResetToDock();
    cargoFuel = 600.0f;
    cargoAmmo = 240.0f;
    servicingReconId = -1;
    servicingTFId = -1;
    recoveringDroneId = -1;
}

void SupportDrone::Update(FleetContext& ctx, float dt) {
    UpdateFlares(dt);
    UpdateDownwash(dt);

    if (isDestroyed) {
        if (position.y > 0.4f) {
            position.y += velocity.y * dt;
            if (position.y < 0.4f) position.y = 0.4f;
        }
        return;
    }

    if (state == DroneState::RETURNING_TO_BASE) {
        targetPosition = baseDockPos;
        isResupplyBeamActive = false;
        UpdateMovementPhysics(16.0f, dt);
        ApplyFlankingAndSeparation(ctx, dt);
        return;
    }

    if (state != DroneState::IDLE_AT_BASE) {
        fuel -= dt * 0.7f;
        ctx.telemetry.totalFuelBurned += dt * 0.7f;
    }

    // =========================================================================
    // PRIORITY #1: COMBAT MEDIC & RESUPPLY FOR LIVING ACTIVE TASK FORCE DRONES
    // (Live combatants must stay fueled & armed to fight and clear hostiles!)
    // =========================================================================
    Drone* needyTF = nullptr;
    if (servicingTFId != -1) {
        for (auto& d : ctx.drones) {
            if (d->id == servicingTFId && !d->isDestroyed) {
                needyTF = d.get();
                break;
            }
        }
        if (needyTF == nullptr) servicingTFId = -1;
    }

    if (needyTF == nullptr && (cargoFuel > 10.0f || cargoAmmo > 10.0f)) {
        float lowestRes = 999.0f;
        for (auto& d : ctx.drones) {
            if (d->role == DroneRole::TASK_FORCE && !d->isDestroyed && d->state != DroneState::IDLE_AT_BASE) {
                if (d->assignedSupportId == -1 || d->assignedSupportId == id) {
                    if (d->state == DroneState::REQUESTING_SUPPORT || d->fuel < 50.0f || d->ammo < 30.0f || d->hp < 140.0f) {
                        float priorityVal = d->fuel + d->ammo + d->hp;
                        if (priorityVal < lowestRes) {
                            lowestRes = priorityVal;
                            needyTF = d.get();
                        }
                    }
                }
            }
        }
        if (needyTF != nullptr) {
            servicingTFId = needyTF->id;
            needyTF->assignedSupportId = id;
        }
    }

    if (needyTF && (cargoFuel > 5.0f || cargoAmmo > 5.0f)) {
        recoveringDroneId = -1;
        servicingReconId = -1;

        if (state == DroneState::IDLE_AT_BASE || state == DroneState::LOITERING_IN_AREA || state == DroneState::PATROL) {
            state = DroneState::MOVING_TO_TARGET;
            LogAction("Priority 1: Dispatched to resupply live TF #" + std::to_string(needyTF->id));
            ctx.LogTerminal("[COMBAT RESUPPLY] Support #" + std::to_string(id) + " flying to resupply live Task Force #" + std::to_string(needyTF->id));
        }

        Vector3 targetPos = needyTF->hasRendezvous ? needyTF->rendezvousPos : needyTF->position;
        targetPosition = targetPos;

        float dist = Vector3Distance(position, needyTF->position);

        if (dist < 35.0f) {
            state = DroneState::RESUPPLYING_IN_AIR;
            isResupplyBeamActive = true;
            resupplyTargetPos = needyTF->position;
            needyTF->state = DroneState::RESUPPLYING_IN_AIR;

            float transferFuel = dt * 50.0f;
            float transferAmmo = dt * 60.0f;
            float transferRepair = dt * 55.0f;

            if (cargoFuel >= transferFuel && needyTF->fuel < needyTF->maxFuel) {
                cargoFuel -= transferFuel;
                needyTF->fuel = std::min(needyTF->maxFuel, needyTF->fuel + transferFuel);
            }
            if (cargoAmmo >= transferAmmo && needyTF->ammo < needyTF->maxAmmo) {
                cargoAmmo -= transferAmmo;
                needyTF->ammo = std::min(needyTF->maxAmmo, needyTF->ammo + transferAmmo);
            }
            if (needyTF->hp < needyTF->maxHp) {
                needyTF->hp = std::min(needyTF->maxHp, needyTF->hp + transferRepair);
            }

            if (needyTF->fuel >= needyTF->maxFuel - 1.0f && needyTF->ammo >= needyTF->maxAmmo - 1.0f && needyTF->hp >= needyTF->maxHp - 1.0f) {
                needyTF->fuel = needyTF->maxFuel;
                needyTF->ammo = needyTF->maxAmmo;
                needyTF->hp = needyTF->maxHp;
                needyTF->state = DroneState::MOVING_TO_TARGET;
                needyTF->hasRendezvous = false;
                needyTF->assignedSupportId = -1;

                servicingTFId = -1;
                isResupplyBeamActive = false;

                char buf[160];
                snprintf(buf, sizeof(buf), "[100%% RESUPPLY COMPLETE] Support #%d finished 100%% refuel & rearm for live TF #%d!", id, needyTF->id);
                ctx.LogTerminal(buf);
                LogAction("Completed 100% full combat resupply for Task Force #" + std::to_string(needyTF->id));
                needyTF->LogAction("100% Refueled, Rearmed & Repaired mid-air by Support #" + std::to_string(id));
            }
        } else {
            state = DroneState::MOVING_TO_TARGET;
            isResupplyBeamActive = false;
        }

        UpdateMovementPhysics(16.0f, dt);
        ApplyFlankingAndSeparation(ctx, dt);
        return;
    }

    // =========================================================================
    // PRIORITY #2: RECON AERIAL RENDEZVOUS REFUEL
    // =========================================================================
    if (servicingReconId != -1) {
        int rIdx = servicingReconId - 1;
        if (rIdx >= 0 && rIdx < (int)ctx.drones.size()) {
            auto& reconDrone = ctx.drones[rIdx];

            targetPosition = reconDrone->rendezvousPos;
            float dist = Vector3Distance(position, reconDrone->position);

            if (dist < 35.0f) {
                state = DroneState::RESUPPLYING_IN_AIR;
                isResupplyBeamActive = true;
                resupplyTargetPos = reconDrone->position;
                reconDrone->state = DroneState::RESUPPLYING_IN_AIR;

                float transferFuel = dt * 45.0f;
                if (cargoFuel >= transferFuel && reconDrone->fuel < reconDrone->maxFuel) {
                    cargoFuel -= transferFuel;
                    reconDrone->fuel = std::min(reconDrone->maxFuel, reconDrone->fuel + transferFuel);
                }

                if (reconDrone->fuel >= reconDrone->maxFuel - 2.0f) {
                    reconDrone->fuel = reconDrone->maxFuel;
                    reconDrone->state = DroneState::PATROL;
                    reconDrone->hasRendezvous = false;
                    reconDrone->assignedSupportId = -1;
                    servicingReconId = -1;
                    isResupplyBeamActive = false;

                    char buf[160];
                    snprintf(buf, sizeof(buf), "[RENDEZVOUS SUCCESS] Support #%d refueled Recon #%d 100%% at Mid-Air Rendezvous!", id, reconDrone->id);
                    ctx.LogTerminal(buf);
                    LogAction("Completed mid-air rendezvous refuel for Recon #" + std::to_string(reconDrone->id));
                    reconDrone->LogAction("Refueled 100% mid-air at Rendezvous Point. Resuming sector sweep.");
                }
            } else {
                state = DroneState::MOVING_TO_TARGET;
                isResupplyBeamActive = false;
            }

            UpdateMovementPhysics(16.0f, dt);
            ApplyFlankingAndSeparation(ctx, dt);
            return;
        }
    }

    // =========================================================================
    // PRIORITY #3: SAR DOWNED DRONE CRASH RECOVERY
    // (Only executed when living combatants are serviced and crash site is clear)
    // =========================================================================
    Drone* downedDrone = nullptr;
    for (auto& d : ctx.drones) {
        if (d->isDestroyed) {
            downedDrone = d.get();
            break;
        }
    }

    if (downedDrone != nullptr) {
        bool hostilesNearCrash = false;
        for (const auto& c : ctx.clusterManager.GetClusters()) {
            if (c.status != ClusterStatus::CLEARED && c.calculatedThreatMagnitude > 0) {
                if (Vector3Distance(c.centerPosition, downedDrone->position) <= 80.0f) {
                    hostilesNearCrash = true;
                    break;
                }
            }
        }

        // If hostiles are near the crash, do not hover right on top of enemy fire!
        // Instead, hold a safe loitering orbit behind the front line while live TF cleans the area.
        if (hostilesNearCrash) {
            recoveringDroneId = -1;
            isResupplyBeamActive = false;
            state = DroneState::LOITERING_IN_AREA;
            targetPosition = Vector3{ downedDrone->position.x * 0.5f, 34.0f, downedDrone->position.z * 0.5f };
        } else {
            recoveringDroneId = downedDrone->id;

            if (state == DroneState::IDLE_AT_BASE || state == DroneState::PATROL || state == DroneState::LOITERING_IN_AREA) {
                state = DroneState::MOVING_TO_TARGET;
                LogAction("Crash area safe. Dispatched on SAR recovery for TF #" + std::to_string(downedDrone->id));
                ctx.LogTerminal("[SAR MISSION] Crash area secured. Support #" + std::to_string(id) + " recovering downed Drone #" + std::to_string(downedDrone->id));
            }

            targetPosition = Vector3{ downedDrone->position.x, 4.0f, downedDrone->position.z };
            float dist = Vector3Distance(position, downedDrone->position);

            if (dist < 35.0f) {
                state = DroneState::RESUPPLYING_IN_AIR;
                isResupplyBeamActive = true;
                resupplyTargetPos = downedDrone->position;

                float repairAmount = dt * 60.0f;
                downedDrone->hp = std::min(downedDrone->maxHp, downedDrone->hp + repairAmount);

                if (downedDrone->hp >= downedDrone->maxHp - 2.0f) {
                    downedDrone->hp = downedDrone->maxHp;
                    downedDrone->fuel = downedDrone->maxFuel * 0.8f;
                    downedDrone->ammo = downedDrone->maxAmmo * 0.8f;
                    downedDrone->isDestroyed = false;
                    downedDrone->isBeingRecovered = false;
                    downedDrone->state = DroneState::MOVING_TO_TARGET;

                    recoveringDroneId = -1;
                    isResupplyBeamActive = false;
                    if (ctx.telemetry.destroyedDronesCount > 0) ctx.telemetry.destroyedDronesCount--;
                    ctx.telemetry.dronesRepairedCount++;

                    char buf[160];
                    snprintf(buf, sizeof(buf), "[SAR RECOVERY] Support #%d RECOVERED & REBOOTED Task Force #%d at crash site (HP 100%%)!", id, downedDrone->id);
                    ctx.LogTerminal(buf);
                    LogAction("Successfully recovered & rebooted TF #" + std::to_string(downedDrone->id) + " at crash site.");
                    downedDrone->LogAction("Recovered & rebooted mid-field by Support #" + std::to_string(id));

                    if (IsAudioDeviceReady()) AudioManager::Instance().PlayLevelUp();
                }
            } else {
                state = DroneState::MOVING_TO_TARGET;
                isResupplyBeamActive = false;
            }

            UpdateMovementPhysics(16.0f, dt);
            ApplyFlankingAndSeparation(ctx, dt);
            return;
        }
    }

    // =========================================================================
    // PRIORITY #4: FORWARD LOITERING STATION OR RTB
    // =========================================================================
    servicingTFId = -1;
    servicingReconId = -1;
    recoveringDroneId = -1;
    isResupplyBeamActive = false;

    bool activeCombatOccurring = false;
    Vector3 activeCombatCenter = ctx.hangarBase.GetPosition();

    for (const auto& c : ctx.clusterManager.GetClusters()) {
        if (c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) {
            activeCombatOccurring = true;
            activeCombatCenter = c.centerPosition;
            break;
        }
    }

    if (activeCombatOccurring && cargoFuel > 40.0f && fuel > 40.0f) {
        state = DroneState::LOITERING_IN_AREA;
        float loiterOffset = (id == 8) ? -60.0f : 60.0f;
        loiterCenterPos = Vector3{
            activeCombatCenter.x * 0.4f + loiterOffset,
            34.0f,
            activeCombatCenter.z * 0.4f
        };
        targetPosition = loiterCenterPos;
    } else {
        if (cargoFuel <= 20.0f || fuel <= 20.0f || !activeCombatOccurring) {
            if (state != DroneState::RETURNING_TO_BASE && state != DroneState::IDLE_AT_BASE) {
                state = DroneState::RETURNING_TO_BASE;
                targetPosition = baseDockPos;
                LogAction("Cargo depleted or operation clear. RTB to Hangar.");
            }
        }
    }

    UpdateMovementPhysics(16.0f, dt);
    ApplyFlankingAndSeparation(ctx, dt);
}

void SupportDrone::Draw3D(VisionMode visionMode) const {
    Drone::Draw3D(visionMode);
    if (state == DroneState::LOITERING_IN_AREA) {
        if (visionMode == VisionMode::FLIR_THERMAL) {
            DrawCircle3D(position, 12.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 100, 240, 255, 255 }, 0.4f));
        } else if (visionMode == VisionMode::NVG_GREEN) {
            DrawCircle3D(position, 12.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(Color{ 80, 255, 120, 255 }, 0.4f));
        } else {
            DrawCircle3D(position, 12.0f, Vector3{ 1, 0, 0 }, 90.0f, Fade(LIME, 0.4f));
        }
    }
}

} // namespace DroneFleet
