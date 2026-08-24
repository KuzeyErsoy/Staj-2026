#include "ParticleSystem.h"
#include <stdlib.h>

ParticleSystem::ParticleSystem() {
    particles.reserve(200);
}

ParticleSystem::~ParticleSystem() {
    particles.clear();
}

void ParticleSystem::EmitPickupBurst(Vector3 pos) {
    for (int i = 0; i < 20; i++) {
        Particle p;
        p.position = pos;
        float vx = ((float)GetRandomValue(-100, 100) / 100.0f) * 4.0f;
        float vy = ((float)GetRandomValue(50, 150) / 100.0f) * 4.0f;
        float vz = ((float)GetRandomValue(-100, 100) / 100.0f) * 4.0f;
        p.velocity = Vector3{ vx, vy, vz };
        
        // Shiny gold/cyan colors
        p.color = (GetRandomValue(0, 1) == 0) ? GOLD : SKYBLUE;
        p.size = (float)GetRandomValue(15, 30) / 100.0f;
        p.lifetime = 0.6f + ((float)GetRandomValue(0, 40) / 100.0f);
        p.maxLifetime = p.lifetime;
        p.isCube = true;

        particles.push_back(p);
    }
}

void ParticleSystem::EmitDashTrail(Vector3 pos, Vector3 dir) {
    for (int i = 0; i < 4; i++) {
        Particle p;
        p.position = Vector3{
            pos.x + ((float)GetRandomValue(-30, 30) / 100.0f),
            pos.y + 0.2f + ((float)GetRandomValue(-20, 20) / 100.0f),
            pos.z + ((float)GetRandomValue(-30, 30) / 100.0f)
        };
        p.velocity = Vector3{
            -dir.x * 1.5f + ((float)GetRandomValue(-50, 50) / 100.0f),
            ((float)GetRandomValue(10, 40) / 100.0f),
            -dir.z * 1.5f + ((float)GetRandomValue(-50, 50) / 100.0f)
        };
        p.color = (GetRandomValue(0, 1) == 0) ? RAYWHITE : LIGHTGRAY;
        p.size = (float)GetRandomValue(20, 40) / 100.0f;
        p.lifetime = 0.35f;
        p.maxLifetime = p.lifetime;
        p.isCube = false;

        particles.push_back(p);
    }
}

void ParticleSystem::EmitLandingDust(Vector3 pos) {
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.position = pos;
        float vx = ((float)GetRandomValue(-100, 100) / 100.0f) * 3.0f;
        float vy = ((float)GetRandomValue(20, 80) / 100.0f) * 2.0f;
        float vz = ((float)GetRandomValue(-100, 100) / 100.0f) * 3.0f;
        p.velocity = Vector3{ vx, vy, vz };
        p.color = GRAY;
        p.size = (float)GetRandomValue(15, 30) / 100.0f;
        p.lifetime = 0.4f;
        p.maxLifetime = p.lifetime;
        p.isCube = false;

        particles.push_back(p);
    }
}

void ParticleSystem::EmitDownwashDust(Vector3 pos, float intensity) {
    Vector3 wind = { sinf((float)GetTime() * 0.5f) * 3.0f, 0.0f, cosf((float)GetTime() * 0.3f) * 2.0f };
    int count = (int)(10.0f * intensity);
    for (int i = 0; i < count; i++) {
        Particle p;
        float ang = ((float)GetRandomValue(0, 360)) * (3.14159f / 180.0f);
        float dist = ((float)GetRandomValue(10, 35) / 10.0f);
        p.position = Vector3{ pos.x + cosf(ang) * dist, 0.05f, pos.z + sinf(ang) * dist };
        
        float speed = ((float)GetRandomValue(20, 50) / 10.0f) * intensity;
        p.velocity = Vector3{
            cosf(ang) * speed + wind.x * 0.8f,
            ((float)GetRandomValue(5, 25) / 10.0f) * intensity,
            sinf(ang) * speed + wind.z * 0.8f
        };
        p.color = Color{ 210, 185, 140, 255 };
        p.size = (float)GetRandomValue(18, 32) / 100.0f;
        p.lifetime = 0.5f + ((float)GetRandomValue(0, 40) / 100.0f);
        p.maxLifetime = p.lifetime;
        p.isCube = false;

        particles.push_back(p);
    }
}

void ParticleSystem::EmitImpact(Vector3 pos, Color color) {
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.position = pos;
        float vx = ((float)GetRandomValue(-100, 100) / 100.0f) * 5.0f;
        float vy = ((float)GetRandomValue(50, 150) / 100.0f) * 3.0f;
        float vz = ((float)GetRandomValue(-100, 100) / 100.0f) * 5.0f;
        p.velocity = Vector3{ vx, vy, vz };
        p.color = color;
        p.size = (float)GetRandomValue(20, 35) / 100.0f;
        p.lifetime = 0.5f;
        p.maxLifetime = p.lifetime;
        p.isCube = true;

        particles.push_back(p);
    }
}

void ParticleSystem::Update(float dt) {
    Vector3 wind = { sinf((float)GetTime() * 0.5f) * 2.0f, 0.0f, cosf((float)GetTime() * 0.3f) * 1.5f };

    for (size_t i = 0; i < particles.size(); ) {
        particles[i].lifetime -= dt;
        if (particles[i].lifetime <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            particles[i].position.x += particles[i].velocity.x * dt;
            particles[i].position.y += particles[i].velocity.y * dt;
            particles[i].position.z += particles[i].velocity.z * dt;

            // Wind drift & gravity effect
            particles[i].velocity.x += wind.x * dt * 0.8f;
            particles[i].velocity.z += wind.z * dt * 0.8f;
            particles[i].velocity.y -= 4.0f * dt;
            
            // Shrink over lifetime
            float ratio = particles[i].lifetime / particles[i].maxLifetime;
            particles[i].color = Fade(particles[i].color, ratio);
            
            i++;
        }
    }
}

void ParticleSystem::Draw() {
    for (const auto &p : particles) {
        if (p.isCube) {
            DrawCubeV(p.position, Vector3{ p.size, p.size, p.size }, p.color);
        } else {
            DrawSphere(p.position, p.size, p.color);
        }
    }
}

void ParticleSystem::Clear() {
    particles.clear();
}
