#include "Enemy.h"
#include "raymath.h"
#include <cmath>

Enemy::Enemy(Vector3 p1, Vector3 p2, float spd) {
    startPoint = p1;
    endPoint = p2;
    position = p1;
    speed = spd;
    state = ENEMY_PATROL;
    rotationAngle = 0.0f;
    isMovingToEnd = true;
    hp = 50;
    maxHp = 50;
}

Enemy::~Enemy() {}

bool Enemy::TakeDamage(int dmg) {
    hp -= dmg;
    if (hp <= 0) {
        hp = 0;
        return true; // Enemy defeated
    }
    return false;
}

void Enemy::Update(const Player& player, float dt) {
    if (hp <= 0) return;

    // Check distance to player for CHASE mode
    float dx = player.position.x - position.x;
    float dz = player.position.z - position.z;
    float distToPlayer = sqrtf(dx * dx + dz * dz);

    float chaseRadius = 12.0f;
    if (distToPlayer <= chaseRadius) {
        state = ENEMY_CHASE;
    } else {
        state = ENEMY_PATROL;
    }

    Vector3 target = isMovingToEnd ? endPoint : startPoint;
    if (state == ENEMY_CHASE) {
        target = player.position;
    }

    float tx = target.x - position.x;
    float tz = target.z - position.z;
    float distToTarget = sqrtf(tx * tx + tz * tz);

    if (distToTarget > 0.3f) {
        float dirX = tx / distToTarget;
        float dirZ = tz / distToTarget;

        float currentSpeed = (state == ENEMY_CHASE) ? (speed * 1.3f) : speed;
        position.x += dirX * currentSpeed * dt;
        position.z += dirZ * currentSpeed * dt;

        rotationAngle = atan2f(dirX, dirZ) * (180.0f / PI);
    } else if (state == ENEMY_PATROL) {
        isMovingToEnd = !isMovingToEnd;
    }
}

void Enemy::Draw() const {
    if (hp <= 0) return;

    Color bodyColor = (state == ENEMY_CHASE) ? RED : MAROON;
    DrawCubeV(position, (Vector3){ 1.4f, 1.8f, 1.4f }, bodyColor);
    DrawCubeWiresV(position, (Vector3){ 1.4f, 1.8f, 1.4f }, BLACK);

    // Glowing eye indicator
    Vector3 eyePos = {
        position.x + sinf(rotationAngle * (PI / 180.0f)) * 0.7f,
        position.y + 0.4f,
        position.z + cosf(rotationAngle * (PI / 180.0f)) * 0.7f
    };
    DrawSphere(eyePos, 0.35f, (state == ENEMY_CHASE) ? YELLOW : ORANGE);

    // 3D Floating Health Bar above enemy
    if (hp < maxHp) {
        Vector3 hpBarPos = { position.x, position.y + 1.4f, position.z };
        float barWidth = 1.6f;
        float hpRatio = (float)hp / (float)maxHp;
        DrawCubeV(hpBarPos, (Vector3){ barWidth, 0.15f, 0.15f }, DARKGRAY);
        DrawCubeV((Vector3){ hpBarPos.x - (barWidth * (1.0f - hpRatio) * 0.5f), hpBarPos.y, hpBarPos.z }, (Vector3){ barWidth * hpRatio, 0.18f, 0.18f }, GREEN);
    }
}

BoundingBox Enemy::GetBounds() const {
    if (hp <= 0) {
        return (BoundingBox){ (Vector3){0, -100, 0}, (Vector3){0, -100, 0} };
    }
    return (BoundingBox){
        (Vector3){ position.x - 0.7f, position.y - 0.9f, position.z - 0.7f },
        (Vector3){ position.x + 0.7f, position.y + 0.9f, position.z + 0.7f }
    };
}
