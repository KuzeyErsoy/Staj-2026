#include "Player.h"
#include "raymath.h"
#include <math.h>

Player::Player(Vector3 startPos) {
    position = startPos;
    prevPosition = startPos;
    velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    rotationAngle = 0.0f;
    scale = (Vector3){ 1.0f, 1.0f, 1.0f };
    isGrounded = false;

    model = LoadModel("../resources/character/character-a.obj");
    texture = LoadTexture("../resources/character/texture-a.png");
    
    if (model.meshCount > 0 && texture.id > 0) {
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    }
}

Player::~Player() {
    UnloadTexture(texture);
    UnloadModel(model);
}

void Player::Update() {
    prevPosition = position;

    // Movement Inputs
    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) moveDir.z -= 0.5f;
    if (IsKeyDown(KEY_S)) moveDir.z += 0.5f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 0.5f;
    if (IsKeyDown(KEY_D)) moveDir.x += 0.5f;

    position.x += moveDir.x;
    position.z += moveDir.z;

    // Rotation Logic
    if (moveDir.x != 0.0f || moveDir.z != 0.0f) {
        // atan2f gives angle in radians. Convert to degrees.
        rotationAngle = atan2f(moveDir.x, moveDir.z) * (180.0f / PI);
    }

    // Gravity and Jumping
    velocity.y -= 0.05f;
    position.y += velocity.y;

    // Basic Floor Collision
    if (position.y <= 0.0f) {
        position.y = 0.0f;
        velocity.y = 0.0f;
        isGrounded = true;
    } else {
        isGrounded = false;
    }

    if (IsKeyPressed(KEY_SPACE) && isGrounded) {
        velocity.y = 1.0f;
        isGrounded = false;
    }

    // Boundaries
    if (position.x < -50.0f) position.x = -50.0f;
    if (position.x > 50.0f) position.x = 50.0f;
    if (position.z < -50.0f) position.z = -50.0f;
    if (position.z > 50.0f) position.z = 50.0f;
}

void Player::Draw() {
    DrawModelEx(model, position, (Vector3){ 0.0f, 1.0f, 0.0f }, rotationAngle, scale, WHITE);
}

BoundingBox Player::GetBounds() {
    return (BoundingBox){
        (Vector3){ position.x - 1.0f, position.y, position.z - 1.0f },
        (Vector3){ position.x + 1.0f, position.y + 2.0f, position.z + 1.0f }
    };
}