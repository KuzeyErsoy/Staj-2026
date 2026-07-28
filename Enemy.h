#pragma once
#include "raylib.h"
#include "Player.h"

enum EnemyState {
    ENEMY_PATROL,
    ENEMY_CHASE
};

class Enemy {
public:
    Vector3 position;
    Vector3 startPoint;
    Vector3 endPoint;
    float speed;
    EnemyState state;
    float rotationAngle;
    bool isMovingToEnd;

    Enemy(Vector3 p1, Vector3 p2, float speed = 3.5f);
    ~Enemy();

    void Update(const Player& player, float dt);
    void Draw() const;
    BoundingBox GetBounds() const;
};
