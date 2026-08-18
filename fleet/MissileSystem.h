#pragma once
#include "FleetTypes.h"
#include <vector>

namespace DroneFleet {

struct MicroMissile {
    int id = 0;
    int launcherDroneId = 0;
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Vector3 targetPos = { 0.0f, 0.0f, 0.0f };
    int targetClusterId = -1;
    int targetEnemyId = -1;
    float lifetime = 0.0f;
    float maxLifetime = 3.5f;
    bool isAlive = true;
    float speed = 28.0f;
};

struct MissileParticle {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Color color = WHITE;
    float life = 0.0f;
    float maxLife = 0.5f;
    float size = 0.3f;
    bool isAlive = true;
};

class MissileSystem {
public:
    MissileSystem();
    ~MissileSystem() = default;

    void Init();
    void LaunchMissile(int launcherId, Vector3 startPos, Vector3 targetPos, int clusterId, int targetEnemyId);
    void Update(ClusterManager& clusterMgr, TelemetryContext& telemetry, float dt);
    void Draw3D(VisionMode visionMode = VisionMode::NORMAL) const;
    void Clear();

    const std::vector<MicroMissile>& GetMissiles() const { return missiles; }

private:
    std::vector<MicroMissile> missiles;
    std::vector<MissileParticle> particles;
    int nextMissileId;

    void EmitTrail(Vector3 pos, Color col);
    void EmitExplosion(Vector3 pos, Color col, int count = 12);
};

} // namespace DroneFleet
