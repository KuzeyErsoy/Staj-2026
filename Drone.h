#pragma once
#include "raylib.h"
#include "Player.h"
#include "Enemy.h"
#include <vector>

enum DroneMode {
    DRONE_MODE_FOLLOW_ATTACK,
    DRONE_MODE_FREE_FLIGHT
};

class Drone {
public:
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float pitch;
    DroneMode mode;

    // Combat & Laser Attributes
    float attackTimer;
    float attackCooldown;
    float attackRange;
    int attackDamage;
    bool isFiringLaser;
    float laserTimer;
    Vector3 laserTargetPos;

    // Visual & Animation
    float rotorAngle;
    float hoverSinTimer;
    bool hasModel;

    Drone(Vector3 startPos);
    ~Drone();

    void Update(Player& player, std::vector<Enemy>& enemies, float dt, const Camera& camera);
    void Draw() const;
    void ToggleMode();

private:
    Model model;
    Texture2D texture;

    void UpdateFollowAttackMode(Player& player, std::vector<Enemy>& enemies, float dt);
    void UpdateFreeFlightMode(float dt, const Camera& camera);
    void DrawProceduralDrone() const;
};
