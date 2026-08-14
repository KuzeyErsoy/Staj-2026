#include "MissileSystem.h"
#include "ClusterManager.h"
#include "AudioManager.h"
#include <algorithm>

namespace DroneFleet {

MissileSystem::MissileSystem() : nextMissileId(1) {
    Init();
}

void MissileSystem::Init() {
    missiles.clear();
    particles.clear();
    nextMissileId = 1;
}

void MissileSystem::Clear() {
    missiles.clear();
    particles.clear();
}

void MissileSystem::LaunchMissile(int launcherId, Vector3 startPos, Vector3 targetPos, int clusterId, int targetEnemyId) {
    MicroMissile m;
    m.id = nextMissileId++;
    m.launcherDroneId = launcherId;
    m.position = startPos;
    m.targetPos = targetPos;
    m.targetClusterId = clusterId;
    m.targetEnemyId = targetEnemyId;
    m.lifetime = 0.0f;
    m.maxLifetime = 3.5f;
    m.isAlive = true;
    m.speed = 34.0f;

    // Initial forward and slight outward/downward velocity
    Vector3 toTarget = Vector3Subtract(targetPos, startPos);
    Vector3 normDir = Vector3Normalize(toTarget);
    Vector3 randomScatter = Vector3{
        (float)GetRandomValue(-2, 2) * 0.1f,
        (float)GetRandomValue(-3, 0) * 0.1f,
        (float)GetRandomValue(-2, 2) * 0.1f
    };
    m.velocity = Vector3Scale(Vector3Add(normDir, randomScatter), m.speed * 0.6f);

    missiles.push_back(m);
}

void MissileSystem::EmitTrail(Vector3 pos, Color col) {
    MissileParticle p;
    p.position = pos;
    p.velocity = Vector3{
        (float)GetRandomValue(-10, 10) * 0.05f,
        (float)GetRandomValue(0, 10) * 0.05f,
        (float)GetRandomValue(-10, 10) * 0.05f
    };
    p.color = col;
    p.life = 0.0f;
    p.maxLife = 0.35f;
    p.size = 0.25f;
    p.isAlive = true;
    particles.push_back(p);
}

void MissileSystem::EmitExplosion(Vector3 pos, Color col, int count) {
    for (int i = 0; i < count; i++) {
        MissileParticle p;
        p.position = pos;
        p.velocity = Vector3{
            (float)GetRandomValue(-50, 50) * 0.1f,
            (float)GetRandomValue(5, 50) * 0.1f,
            (float)GetRandomValue(-50, 50) * 0.1f
        };
        p.color = (i % 2 == 0) ? col : YELLOW;
        p.life = 0.0f;
        p.maxLife = (float)GetRandomValue(20, 50) * 0.01f;
        p.size = 0.4f;
        p.isAlive = true;
        particles.push_back(p);
    }
}

void MissileSystem::Update(ClusterManager& clusterMgr, TelemetryContext& telemetry, float dt) {
    // 1. Update active missiles
    for (auto& m : missiles) {
        if (!m.isAlive) continue;

        m.lifetime += dt;
        if (m.lifetime >= m.maxLifetime) {
            m.isAlive = false;
            EmitExplosion(m.position, ORANGE, 8);
            continue;
        }

        // Find live target position if enemy is still tracked
        EnemyCluster* cluster = clusterMgr.GetClusterById(m.targetClusterId);
        if (cluster && cluster->status != ClusterStatus::CLEARED) {
            for (const auto& e : cluster->enemies) {
                if (e.id == m.targetEnemyId && e.isAlive) {
                    m.targetPos = e.position;
                    break;
                }
            }
        }

        // Homing steering physics toward target
        Vector3 desiredDir = Vector3Normalize(Vector3Subtract(m.targetPos, m.position));
        Vector3 desiredVel = Vector3Scale(desiredDir, m.speed);

        // Turn rate interpolation (proportional navigation)
        m.velocity = Vector3Lerp(m.velocity, desiredVel, dt * 7.5f);
        m.position = Vector3Add(m.position, Vector3Scale(m.velocity, dt));

        // Emit rocket smoke trail
        EmitTrail(m.position, Fade(LIGHTGRAY, 0.7f));
        if (GetRandomValue(0, 100) < 30) {
            EmitTrail(m.position, Fade(ORANGE, 0.8f));
        }

        // Check impact proximity
        float distToTarget = Vector3Distance(m.position, m.targetPos);
        if (distToTarget <= 2.2f) {
            m.isAlive = false;
            EmitExplosion(m.position, RED, 14);

            if (IsAudioDeviceReady()) {
                AudioManager::Instance().PlayHurt();
            }

            // Deal AoE splash damage to cluster
            if (cluster && cluster->status != ClusterStatus::CLEARED) {
                for (auto& e : cluster->enemies) {
                    if (e.isAlive) {
                        float ed = Vector3Distance(m.position, e.position);
                        if (ed <= 5.0f) {
                            float dmg = 45.0f * (1.0f - (ed / 5.0f));
                            e.hp -= dmg;
                            e.panicTimer = 2.5f;

                            if (e.hp <= 0.0f) {
                                e.hp = 0.0f;
                                e.isAlive = false;
                                cluster->remainingMagnitude--;
                                telemetry.eliminatedTargetsCount++;

                                int aliveArmed = 0;
                                int aliveUnarmed = 0;
                                for (const auto& unit : cluster->enemies) {
                                    if (unit.isAlive) {
                                        if (unit.isArmed) aliveArmed++;
                                        else aliveUnarmed++;
                                    }
                                }
                                cluster->armedCount = aliveArmed;
                                cluster->unarmedCount = aliveUnarmed;
                                cluster->calculatedThreatMagnitude = (int)ceilf((float)aliveArmed * 0.5f + (float)aliveUnarmed * 0.25f);
                            }
                        }
                    }
                }
            }
        }
    }

    // Cleanup dead missiles
    missiles.erase(std::remove_if(missiles.begin(), missiles.end(), [](const MicroMissile& m) {
        return !m.isAlive;
    }), missiles.end());

    // 2. Update smoke and explosion particles
    for (auto& p : particles) {
        if (!p.isAlive) continue;

        p.life += dt;
        if (p.life >= p.maxLife) {
            p.isAlive = false;
            continue;
        }

        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.velocity = Vector3Scale(p.velocity, 0.94f);
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const MissileParticle& p) {
        return !p.isAlive;
    }), particles.end());
}

void MissileSystem::Draw3D() const {
    // 1. Draw particles
    for (const auto& p : particles) {
        if (!p.isAlive) continue;
        float alpha = 1.0f - (p.life / p.maxLife);
        DrawSphere(p.position, p.size * alpha, Fade(p.color, alpha * 0.8f));
    }

    // 2. Draw missiles
    for (const auto& m : missiles) {
        if (!m.isAlive) continue;

        // Missile body
        Vector3 forward = Vector3Normalize(m.velocity);
        Vector3 tailPos = Vector3Subtract(m.position, Vector3Scale(forward, 0.8f));

        DrawLine3D(tailPos, m.position, ORANGE);
        DrawSphere(m.position, 0.25f, RED);
        DrawSphere(tailPos, 0.15f, YELLOW);
    }
}

} // namespace DroneFleet
