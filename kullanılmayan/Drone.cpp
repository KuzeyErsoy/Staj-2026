#include "Drone.h"
#include "raymath.h"
#include "rlgl.h"
#include "AudioManager.h"
#include <math.h>

Drone::Drone(Vector3 startPos) {
    position = startPos;
    velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    yaw = 0.0f;
    pitch = 0.0f;
    mode = DRONE_MODE_FOLLOW_ATTACK;

    attackTimer = 0.0f;
    attackCooldown = 1.0f;
    attackRange = 18.0f;
    attackDamage = 25;
    isFiringLaser = false;
    laserTimer = 0.0f;
    laserTargetPos = (Vector3){ 0.0f, 0.0f, 0.0f };

    rotorAngle = 0.0f;
    hoverSinTimer = 0.0f;
    hasModel = false;
}

Drone::~Drone() {
}

void Drone::ToggleMode() {
    if (mode == DRONE_MODE_FOLLOW_ATTACK) {
        mode = DRONE_MODE_FREE_FLIGHT;
    } else {
        mode = DRONE_MODE_FOLLOW_ATTACK;
    }
    AudioManager::Instance().PlayClick();
}

void Drone::Update(Player& player, std::vector<Enemy>& enemies, float dt, const Camera& camera) {
    hoverSinTimer += dt;
    rotorAngle += 720.0f * dt;
    if (rotorAngle >= 360.0f) rotorAngle -= 360.0f;

    if (attackTimer > 0.0f) {
        attackTimer -= dt;
    }

    if (laserTimer > 0.0f) {
        laserTimer -= dt;
        if (laserTimer <= 0.0f) {
            isFiringLaser = false;
        }
    }

    if (mode == DRONE_MODE_FOLLOW_ATTACK) {
        UpdateFollowAttackMode(player, enemies, dt);
    } else {
        UpdateFreeFlightMode(dt, camera);
    }
}

void Drone::UpdateFollowAttackMode(Player& player, std::vector<Enemy>& enemies, float dt) {
    // Calculate target offset position relative to player
    float offsetDist = 3.0f;
    float targetX = player.position.x - sinf(player.rotationAngle * DEG2RAD) * offsetDist - 1.5f;
    float targetY = player.position.y + 3.0f + sinf(hoverSinTimer * 3.0f) * 0.4f;
    float targetZ = player.position.z - cosf(player.rotationAngle * DEG2RAD) * offsetDist - 1.5f;

    Vector3 targetPos = (Vector3){ targetX, targetY, targetZ };

    // Smooth LERP movement towards target position
    float lerpSpeed = 5.0f * dt;
    position.x += (targetPos.x - position.x) * lerpSpeed;
    position.y += (targetPos.y - position.y) * lerpSpeed;
    position.z += (targetPos.z - position.z) * lerpSpeed;

    // Search for closest enemy to attack
    float minDistance = attackRange;
    Enemy* closestEnemy = nullptr;

    for (auto& enemy : enemies) {
        if (enemy.hp <= 0) continue;
        float d = Vector3Distance(position, enemy.position);
        if (d < minDistance) {
            minDistance = d;
            closestEnemy = &enemy;
        }
    }

    if (closestEnemy != nullptr) {
        // Point drone towards target enemy
        float dx = closestEnemy->position.x - position.x;
        float dz = closestEnemy->position.z - position.z;
        yaw = atan2f(dx, dz) * RAD2DEG;

        // Auto Attack logic
        if (attackTimer <= 0.0f) {
            isFiringLaser = true;
            laserTimer = 0.20f;
            laserTargetPos = (Vector3){ closestEnemy->position.x, closestEnemy->position.y + 0.9f, closestEnemy->position.z };
            closestEnemy->TakeDamage(attackDamage);
            attackTimer = attackCooldown;
            AudioManager::Instance().PlayHurt();
        }
    } else {
        // Face player direction when idle
        yaw = player.rotationAngle;
    }
}

void Drone::UpdateFreeFlightMode(float dt, const Camera& camera) {
    // Mouse Look rotation in free flight
    Vector2 mouseDelta = GetMouseDelta();
    float mouseSens = 0.15f;
    yaw -= mouseDelta.x * mouseSens;
    pitch += mouseDelta.y * mouseSens;

    if (pitch > 85.0f) pitch = 85.0f;
    if (pitch < -85.0f) pitch = -85.0f;

    // Movement controls in Free Flight Mode
    Vector3 input = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(KEY_W)) input.z += 1.0f;
    if (IsKeyDown(KEY_S)) input.z -= 1.0f;
    if (IsKeyDown(KEY_A)) input.x -= 1.0f;
    if (IsKeyDown(KEY_D)) input.x += 1.0f;

    float speed = 12.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) speed = 24.0f; // Fast flight boost

    if (input.x != 0.0f || input.z != 0.0f) {
        float radYaw = yaw * DEG2RAD;
        float forwardX = sinf(radYaw);
        float forwardZ = cosf(radYaw);
        float rightX = cosf(radYaw);
        float rightZ = -sinf(radYaw);

        Vector3 moveDir = {
            forwardX * input.z + rightX * input.x,
            0.0f,
            forwardZ * input.z + rightZ * input.x
        };

        position.x += moveDir.x * speed * dt;
        position.z += moveDir.z * speed * dt;
    }

    // Ascend / Descend
    if (IsKeyDown(KEY_SPACE)) position.y += speed * dt;
    if (IsKeyDown(KEY_C))     position.y -= speed * dt;

    // Keep within reasonable world bounds
    if (position.y < 1.0f) position.y = 1.0f;
    if (position.y > 60.0f) position.y = 60.0f;
}

void Drone::Draw() const {
    if (hasModel) {
        DrawModelEx(model, position, (Vector3){ 0.0f, 1.0f, 0.0f }, yaw, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
    } else {
        DrawProceduralDrone();
    }

    // Render Laser Beam if firing
    if (isFiringLaser) {
        Vector3 startPos = (Vector3){ position.x, position.y - 0.2f, position.z };
        DrawCylinderEx(startPos, laserTargetPos, 0.08f, 0.08f, 8, RED);
        DrawCylinderEx(startPos, laserTargetPos, 0.16f, 0.16f, 8, Fade(YELLOW, 0.6f));
        DrawSphere(laserTargetPos, 0.4f, RED);
    }
}

void Drone::DrawProceduralDrone() const {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);
    rlRotatef(pitch, 1.0f, 0.0f, 0.0f);

    // Central Chassis
    DrawSphere((Vector3){ 0.0f, 0.0f, 0.0f }, 0.6f, DARKBLUE);
    DrawSphereWires((Vector3){ 0.0f, 0.0f, 0.0f }, 0.62f, 8, 8, SKYBLUE);

    // Front Camera / Scanner Eye
    DrawSphere((Vector3){ 0.0f, 0.0f, 0.55f }, 0.25f, (mode == DRONE_MODE_FREE_FLIGHT) ? LIME : RED);

    // 4 Diagonal Rotor Arms
    float armLen = 1.2f;
    Vector3 armOffsets[4] = {
        { armLen, 0.0f, armLen },
        { -armLen, 0.0f, armLen },
        { armLen, 0.0f, -armLen },
        { -armLen, 0.0f, -armLen }
    };

    for (int i = 0; i < 4; i++) {
        DrawCylinderEx((Vector3){ 0.0f, 0.0f, 0.0f }, armOffsets[i], 0.08f, 0.08f, 6, GRAY);
        
        // Motor Caps
        DrawCylinder(armOffsets[i], 0.18f, 0.18f, 0.25f, 8, DARKGRAY);

        // Spinning Propellers
        rlPushMatrix();
        rlTranslatef(armOffsets[i].x, armOffsets[i].y + 0.15f, armOffsets[i].z);
        rlRotatef(rotorAngle * ((i % 2 == 0) ? 1.0f : -1.0f), 0.0f, 1.0f, 0.0f);
        DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.9f, 0.02f, 0.12f, (mode == DRONE_MODE_FREE_FLIGHT) ? GREEN : GOLD);
        rlPopMatrix();
    }

    // Scanning Cone / Downward Light Beam
    if (mode == DRONE_MODE_FOLLOW_ATTACK) {
        DrawCylinder((Vector3){ 0.0f, -2.0f, 0.0f }, 0.1f, 1.2f, 2.0f, 12, Fade(SKYBLUE, 0.15f));
    }

    rlPopMatrix();
}
