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
    isDashing = false;
    dashTimer = 0.0f;
    dashCooldownTimer = 0.0f;
    dashDirection = (Vector3){ 0.0f, 0.0f, 0.0f };

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

void Player::Update(float cameraYaw) {
    prevPosition = position;
    float dt = GetFrameTime();

    if (dashCooldownTimer > 0.0f) {
        dashCooldownTimer -= dt;
    }

    if (isDashing) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) {
            isDashing = false;
        }
    }

    if (IsKeyPressed(KEY_LEFT_SHIFT) && !isDashing && dashCooldownTimer <= 0.0f) {
        isDashing = true;
        dashTimer = 0.2f;
        dashCooldownTimer = 2.0f;
        
        float rad = rotationAngle * (PI / 180.0f);
        dashDirection.x = sinf(rad);
        dashDirection.z = cosf(rad);
    }

    // Movement Inputs
    Vector3 inputDir = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) inputDir.z -= 1.0f;
    if (IsKeyDown(KEY_S)) inputDir.z += 1.0f;
    if (IsKeyDown(KEY_A)) inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) inputDir.x += 1.0f;

    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (isDashing) {
        float dashSpeed = 2.0f;
        moveDir.x = dashDirection.x * dashSpeed;
        moveDir.z = dashDirection.z * dashSpeed;

        position.x += moveDir.x;
        position.z += moveDir.z;
    } else {
        if (inputDir.x != 0.0f || inputDir.z != 0.0f) {
            float length = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
            inputDir.x /= length;
            inputDir.z /= length;

            float speed = 0.5f;
            float sinYaw = sinf(cameraYaw);
            float cosYaw = cosf(cameraYaw);

            moveDir.x = (cosYaw * inputDir.x) + (-sinYaw * -inputDir.z);
            moveDir.z = (-sinYaw * inputDir.x) + (-cosYaw * -inputDir.z);

            moveDir.x *= speed;
            moveDir.z *= speed;

            position.x += moveDir.x;
            position.z += moveDir.z;

            // Rotation Logic
            rotationAngle = atan2f(moveDir.x, moveDir.z) * (180.0f / PI);
        }
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