#include "ClusterManager.h"
#include "Drone.h"
#include "AudioManager.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace DroneFleet {

ClusterManager::ClusterManager() : nextClusterId(1) {
    Init();
}

void ClusterManager::Init() {
    clusters.clear();
    nextClusterId = 1;
}

EnemyCluster* ClusterManager::GetClusterById(int id) {
    for (auto& c : clusters) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

void ClusterManager::SpawnCustomCluster(bool forceArmed, int forceMag) {
    EnemyCluster cluster;
    cluster.id = nextClusterId++;

    float rx = (float)GetRandomValue(-350, 350);
    float rz = (float)GetRandomValue(-350, 350);
    if (fabsf(rx) < 50.0f && fabsf(rz) < 50.0f) {
        rx = 180.0f;
        rz = 180.0f;
    }
    cluster.centerPosition = Vector3{ rx, 1.0f, rz };

    int totalUnits = (forceMag > 0) ? forceMag : GetRandomValue(6, 12);
    cluster.initialMagnitude = totalUnits;
    cluster.remainingMagnitude = totalUnits;
    cluster.status = ClusterStatus::UNDETECTED;

    cluster.armedCount = 0;
    cluster.unarmedCount = 0;

    for (int i = 0; i < totalUnits; i++) {
        Enemy e;
        e.id = i + 1;
        float offsetX = (float)GetRandomValue(-14, 14);
        float offsetZ = (float)GetRandomValue(-14, 14);
        e.position = Vector3{ rx + offsetX, 1.0f, rz + offsetZ };
        e.velocity = Vector3{ 0.0f, 0.0f, 0.0f };
        e.hp = 100.0f;
        e.maxHp = 100.0f;
        e.isAlive = true;
        e.panicTimer = 0.0f;
        e.fleeTarget = e.position;
        e.isFiringCounterAttack = false;
        e.isArmed = forceArmed;

        if (e.isArmed) cluster.armedCount++;
        else cluster.unarmedCount++;

        cluster.enemies.push_back(e);
    }

    cluster.calculatedThreatMagnitude = (int)ceilf((float)cluster.armedCount * 0.5f + (float)cluster.unarmedCount * 0.25f);
    clusters.push_back(cluster);
}

void ClusterManager::SpawnCluster() {
    SpawnCustomCluster(GetRandomValue(0, 100) < 50, GetRandomValue(4, 12));
}

void ClusterManager::Update(FleetContext& ctx, float dt) {
    // 1. Spawner & Cluster Check
    int pendingSpawns = 0;
    for (auto& c : clusters) {
        if (c.status != ClusterStatus::CLEARED) {
            if (c.remainingMagnitude <= 0) {
                c.status = ClusterStatus::CLEARED;
                char buf[128];
                snprintf(buf, sizeof(buf), "[TACTICAL COMMAND] Hostile Cluster #%d CLEARED! Relocating spawner...", c.id);
                ctx.LogTerminal(buf);
                pendingSpawns++;
            }
        }
    }

    for (int s = 0; s < pendingSpawns; s++) {
        SpawnCluster();
    }

    bool hasActiveCluster = false;
    for (const auto& c : clusters) {
        if (c.status != ClusterStatus::CLEARED) {
            hasActiveCluster = true;
            break;
        }
    }

    if (!hasActiveCluster && pendingSpawns == 0) {
        SpawnCluster();
    }

    // 2. Hostile Counter-Attacks (Armed Enemies Fire Back)
    for (auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        for (auto& e : c.enemies) {
            if (!e.isAlive) {
                e.isFiringCounterAttack = false;
                continue;
            }

            e.isFiringCounterAttack = false;

            if (e.isArmed) {
                Drone* closestTargetDrone = nullptr;
                float minDist = 35.0f;

                for (auto& d : ctx.drones) {
                    if (d->role == DroneRole::TASK_FORCE && !d->isDestroyed && d->state != DroneState::IDLE_AT_BASE) {
                        float ed = Vector3Distance(e.position, d->position);
                        if (ed < minDist) {
                            minDist = ed;
                            closestTargetDrone = d.get();
                        }
                    }
                }

                if (closestTargetDrone != nullptr) {
                    e.isFiringCounterAttack = true;
                    e.counterAttackTargetPos = closestTargetDrone->position;

                    // Automatic Countermeasure Reaction when targeted by armed hostile
                    if (closestTargetDrone->flareCooldown <= 0.0f && closestTargetDrone->hp <= closestTargetDrone->maxHp * 0.85f) {
                        closestTargetDrone->DeployFlares(&ctx);
                    }

                    // Flares & Chaff decoy hostile targeting, reducing damage by 65%
                    float damageMultiplier = (closestTargetDrone->flareAlertTimer > 0.0f) ? 0.35f : 1.0f;
                    closestTargetDrone->hp -= dt * 12.0f * damageMultiplier;

                    if (closestTargetDrone->hp <= 0.0f) {
                        closestTargetDrone->hp = 0.0f;
                        closestTargetDrone->isDestroyed = true;
                        closestTargetDrone->velocity = Vector3{ 0, -15.0f, 0 };
                        closestTargetDrone->state = DroneState::IDLE_AT_BASE;
                        ctx.telemetry.destroyedDronesCount++;

                        char buf[160];
                        snprintf(buf, sizeof(buf), "[CASUALTY] Task Force #%d SHOT DOWN by Armed Hostile at (%.0f, %.0f)! Support SAR requested.",
                                 closestTargetDrone->id, closestTargetDrone->position.x, closestTargetDrone->position.z);
                        ctx.LogTerminal(buf);
                        closestTargetDrone->LogAction("SHOT DOWN by Armed Hostile. Waiting for Support SAR.");

                        if (IsAudioDeviceReady()) AudioManager::Instance().PlayHurt();
                    }
                }
            }

            // Panic fleeing movement
            if (e.panicTimer > 0.0f) {
                e.panicTimer -= dt;

                Vector3 fleeDir = Vector3Subtract(e.position, e.fleeTarget);
                float dist = Vector3Length(fleeDir);

                if (dist < 1.0f || (GetRandomValue(0, 100) < 4)) {
                    float rx = (float)GetRandomValue(-22, 22);
                    float rz = (float)GetRandomValue(-22, 22);
                    e.fleeTarget = Vector3{ c.centerPosition.x + rx, 1.0f, c.centerPosition.z + rz };
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
}

void ClusterManager::Draw3D(const std::vector<std::unique_ptr<Drone>>& drones, VisionMode visionMode) const {
    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        Color ringColor = RED;
        if (visionMode == VisionMode::FLIR_THERMAL) {
            ringColor = (c.status == ClusterStatus::UNDETECTED) ? Color{ 255, 180, 50, 160 } : Color{ 255, 80, 20, 220 };
        } else if (visionMode == VisionMode::NVG_GREEN) {
            ringColor = (c.status == ClusterStatus::UNDETECTED) ? Color{ 140, 255, 80, 160 } : Color{ 50, 255, 100, 220 };
        } else {
            if (c.status == ClusterStatus::UNDETECTED) ringColor = Color{ 220, 220, 50, 180 };
            else if (c.status == ClusterStatus::REPORTED) ringColor = ORANGE;
        }

        DrawCircle3D(c.centerPosition, 18.0f, Vector3{ 1, 0, 0 }, 90.0f, ringColor);

        // 3D Overhead marker post for discovered clusters
        if (c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) {
            Vector3 bannerPos = Vector3{ c.centerPosition.x, 14.0f, c.centerPosition.z };
            Color bannerCol = (c.armedCount > 0) ? Color{ 255, 30, 30, 255 } : Color{ 240, 160, 20, 255 };
            if (visionMode == VisionMode::FLIR_THERMAL) {
                bannerCol = (c.armedCount > 0) ? Color{ 255, 100, 40, 255 } : Color{ 255, 180, 40, 255 };
            } else if (visionMode == VisionMode::NVG_GREEN) {
                bannerCol = (c.armedCount > 0) ? Color{ 255, 80, 80, 255 } : Color{ 120, 255, 120, 255 };
            }

            DrawCube(bannerPos, 24.0f, 4.0f, 0.4f, Color{ 10, 15, 25, 230 });
            DrawCubeWires(bannerPos, 24.2f, 4.2f, 0.5f, bannerCol);
            DrawLine3D(c.centerPosition, bannerPos, bannerCol);
        }

        // Draw enemies in cluster
        for (const auto& e : c.enemies) {
            if (!e.isAlive) continue;

            if (visionMode == VisionMode::FLIR_THERMAL) {
                // --- FLIR THERMAL HEAT SIGNATURE ---
                // 1. Hot radiant infrared ground halo
                float heatPulse = 0.4f + 0.25f * sinf(GetTime() * 8.0f + (float)e.id);
                Color haloColor = e.isArmed ? Color{ 255, 90, 20, 255 } : Color{ 255, 140, 30, 255 };
                DrawCircle3D(Vector3{ e.position.x, 0.2f, e.position.z }, 2.6f, Vector3{ 1, 0, 0 }, 90.0f, Fade(haloColor, heatPulse));

                // 2. Radiant infrared outer shell
                Color shellColor = e.isArmed ? Color{ 255, 95, 15, 230 } : Color{ 255, 135, 20, 230 };
                if (e.panicTimer > 0.0f) shellColor = Color{ 255, 220, 50, 255 };
                DrawCube(e.position, 2.0f, 2.0f, 2.0f, shellColor);

                // 3. Super-heated White-Hot inner core
                DrawCube(e.position, 1.2f, 1.2f, 1.2f, Color{ 255, 255, 245, 255 });
                DrawCubeWires(e.position, 2.1f, 2.1f, 2.1f, Color{ 255, 240, 180, 255 });

                // Counter-attack tracer
                if (e.isFiringCounterAttack) {
                    DrawLine3D(e.position, e.counterAttackTargetPos, Color{ 255, 255, 255, 255 });
                    DrawSphere(e.counterAttackTargetPos, 0.5f, Color{ 255, 120, 30, 255 });
                }

                if (e.panicTimer > 0.0f) {
                    DrawCircle3D(e.position, 3.0f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 255, 230, 60, 220 });
                }

                // Thermal Health bar
                float hpPct = std::max(0.0f, e.hp / e.maxHp);
                DrawCube(Vector3{ e.position.x, e.position.y + 2.2f, e.position.z }, 2.0f, 0.3f, 0.3f, Color{ 25, 30, 40, 255 });
                DrawCube(Vector3{ e.position.x - (1.0f - hpPct), e.position.y + 2.2f, e.position.z }, 2.0f * hpPct, 0.3f, 0.3f, Color{ 255, 240, 160, 255 });
            } else if (visionMode == VisionMode::NVG_GREEN) {
                // --- NVG GREEN PHOSPHOR SIGNATURE ---
                Color enemyColor = e.isArmed ? Color{ 240, 80, 80, 255 } : Color{ 40, 220, 80, 255 };
                Color wireColor = e.isArmed ? Color{ 255, 140, 140, 255 } : Color{ 140, 255, 160, 255 };

                if (e.panicTimer > 0.0f) enemyColor = Color{ 220, 255, 80, 255 };

                DrawCube(e.position, 2.0f, 2.0f, 2.0f, enemyColor);
                DrawCubeWires(e.position, 2.1f, 2.1f, 2.1f, wireColor);

                if (e.isFiringCounterAttack) {
                    DrawLine3D(e.position, e.counterAttackTargetPos, Color{ 255, 100, 100, 255 });
                    DrawSphere(e.counterAttackTargetPos, 0.4f, Color{ 255, 180, 180, 255 });
                }

                if (e.panicTimer > 0.0f) {
                    DrawCircle3D(e.position, 2.8f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 180, 255, 100, 255 });
                }

                float hpPct = std::max(0.0f, e.hp / e.maxHp);
                DrawCube(Vector3{ e.position.x, e.position.y + 2.2f, e.position.z }, 2.0f, 0.3f, 0.3f, Color{ 10, 35, 15, 255 });
                DrawCube(Vector3{ e.position.x - (1.0f - hpPct), e.position.y + 2.2f, e.position.z }, 2.0f * hpPct, 0.3f, 0.3f, Color{ 80, 255, 120, 255 });
            } else {
                // --- NORMAL RENDERING ---
                Color enemyColor = e.isArmed ? Color{ 255, 30, 30, 255 } : Color{ 240, 150, 20, 255 };
                Color wireColor = e.isArmed ? MAROON : GOLD;

                if (e.panicTimer > 0.0f) enemyColor = Color{ 255, 200, 0, 255 };

                DrawCube(e.position, 2.0f, 2.0f, 2.0f, enemyColor);
                DrawCubeWires(e.position, 2.1f, 2.1f, 2.1f, wireColor);

                if (e.isFiringCounterAttack) {
                    DrawLine3D(e.position, e.counterAttackTargetPos, MAROON);
                    DrawSphere(e.counterAttackTargetPos, 0.4f, RED);
                }

                if (e.panicTimer > 0.0f) {
                    DrawCircle3D(e.position, 2.8f, Vector3{ 1, 0, 0 }, 90.0f, YELLOW);
                }

                // Health bar
                float hpPct = std::max(0.0f, e.hp / e.maxHp);
                DrawCube(Vector3{ e.position.x, e.position.y + 2.2f, e.position.z }, 2.0f, 0.3f, 0.3f, DARKGRAY);
                DrawCube(Vector3{ e.position.x - (1.0f - hpPct), e.position.y + 2.2f, e.position.z }, 2.0f * hpPct, 0.3f, 0.3f, e.isArmed ? RED : ORANGE);
            }
        }
    }

    // Rendezvous circles
    for (const auto& d : drones) {
        if (d->isDestroyed) continue;
        if (d->hasRendezvous) {
            Color rColor1 = (visionMode == VisionMode::FLIR_THERMAL) ? Color{ 255, 180, 50, 220 } : ((visionMode == VisionMode::NVG_GREEN) ? Color{ 140, 255, 80, 220 } : GOLD);
            Color rColor2 = (visionMode == VisionMode::FLIR_THERMAL) ? Color{ 255, 240, 140, 220 } : ((visionMode == VisionMode::NVG_GREEN) ? Color{ 200, 255, 120, 220 } : YELLOW);
            DrawCircle3D(d->rendezvousPos, 12.0f, Vector3{ 1, 0, 0 }, 90.0f, rColor1);
            DrawCircle3D(d->rendezvousPos, 6.0f, Vector3{ 1, 0, 0 }, 90.0f, rColor2);
        }
    }
}

} // namespace DroneFleet
