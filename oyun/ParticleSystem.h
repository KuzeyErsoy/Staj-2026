#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float lifetime;
    float maxLifetime;
    bool isCube;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void EmitPickupBurst(Vector3 pos);
    void EmitDashTrail(Vector3 pos, Vector3 dir);
    void EmitLandingDust(Vector3 pos);
    void EmitDownwashDust(Vector3 pos, float intensity = 1.0f);
    void EmitImpact(Vector3 pos, Color color);

    void Update(float dt);
    void Draw();
    void Clear();

private:
    std::vector<Particle> particles;
};
