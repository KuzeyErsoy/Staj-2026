#include "Player.h"
#include "AudioManager.h"
#include <cmath>
#include <string>

Player::Player(Vector3 startPos) {
  position = startPos;
  prevPosition = startPos;
  velocity = (Vector3){0.0f, 0.0f, 0.0f};
  rotationAngle = 0.0f;
  scale = (Vector3){1.0f, 1.0f, 1.0f};
  isGrounded = false;
  isDashing = false;
  dashTimer = 0.0f;
  dashCooldownTimer = 0.0f;
  dashDirection = (Vector3){0.0f, 0.0f, 0.0f};
  
  hp = 100;
  maxHp = 100;
  invincibilityTimer = 0.0f;

  jumpCount = 0;
  maxJumps = 1;
  hasDoubleJump = false;
  hasWallJump = false;
  isWallSliding = false;

  exp = 0;
  maxExp = 100;
  level = 1;
  levelPoints = 0;
  moveSpeedMultiplier = 1.0f;
  dashCooldownMultiplier = 1.0f;

  std::string modelPath = "resources/character/character-a.obj";
  std::string texturePath = "resources/character/texture-a.png";

  if (!FileExists(modelPath.c_str())) {
    modelPath = "../resources/character/character-a.obj";
    texturePath = "../resources/character/texture-a.png";
  }

  if (FileExists(modelPath.c_str())) {
    model = LoadModel(modelPath.c_str());
    if (FileExists(texturePath.c_str())) {
      texture = LoadTexture(texturePath.c_str());
      if (model.meshCount > 0 && texture.id > 0) {
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
      }
    }
  }
}

Player::~Player() {
  UnloadTexture(texture);
  UnloadModel(model);
}

void Player::Update(float cameraYaw) {
  prevPosition = position;
  float dt = GetFrameTime();

  if (invincibilityTimer > 0.0f) {
    invincibilityTimer -= dt;
  }

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
    dashCooldownTimer = 2.0f * dashCooldownMultiplier;

    float rad = rotationAngle * (PI / 180.0f);
    dashDirection.x = sinf(rad);
    dashDirection.z = cosf(rad);

    AudioManager::Instance().PlayDash();
  }

  // Movement Inputs
  Vector3 inputDir = {0.0f, 0.0f, 0.0f};

  if (IsKeyDown(KEY_W))
    inputDir.z -= 1.0f;
  if (IsKeyDown(KEY_S))
    inputDir.z += 1.0f;
  if (IsKeyDown(KEY_A))
    inputDir.x -= 1.0f;
  if (IsKeyDown(KEY_D))
    inputDir.x += 1.0f;

  Vector3 moveDir = {0.0f, 0.0f, 0.0f};

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

      float speed = 0.5f * moveSpeedMultiplier;
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

  // Gravity and Jumping (Support Double Jump)
  if (isGrounded) {
    jumpCount = 0;
  }

  if (IsKeyPressed(KEY_SPACE)) {
    if (isGrounded || (hasDoubleJump && jumpCount < maxJumps)) {
      velocity.y = 0.85f;
      isGrounded = false;
      jumpCount++;
      AudioManager::Instance().PlayJump();
    }
  }

  if (!isGrounded) {
    velocity.y -= 0.04f;
  }

  position.y += velocity.y;

  // Basic Floor Collision
  if (position.y <= 0.0f) {
    position.y = 0.0f;
    velocity.y = 0.0f;
    isGrounded = true;
    jumpCount = 0;
  }

  // Map Boundaries
  if (position.x < -50.0f) position.x = -50.0f;
  if (position.x > 50.0f)  position.x = 50.0f;
  if (position.z < -50.0f) position.z = -50.0f;
  if (position.z > 50.0f)  position.z = 50.0f;
}

void Player::Draw() {
  // Invincibility flash effect
  Color tint = WHITE;
  if (invincibilityTimer > 0.0f) {
    if ((int)(invincibilityTimer * 20.0f) % 2 == 0) {
      tint = RED;
    }
  }

  DrawModelEx(model, position, (Vector3){0.0f, 1.0f, 0.0f}, rotationAngle,
              scale, tint);
}

BoundingBox Player::GetBounds() {
  return (BoundingBox){
      (Vector3){position.x - 1.0f, position.y, position.z - 1.0f},
      (Vector3){position.x + 1.0f, position.y + 2.0f, position.z + 1.0f}};
}

void Player::AddExp(int amount) {
  exp += amount;
  while (exp >= maxExp) {
    exp -= maxExp;
    level++;
    levelPoints++;
    maxExp = level * 100;
    AudioManager::Instance().PlayLevelUp();
  }
}

bool Player::TakeDamage(int dmg) {
  if (invincibilityTimer > 0.0f) return false;

  hp -= dmg;
  if (hp < 0) hp = 0;
  invincibilityTimer = 1.0f;
  AudioManager::Instance().PlayHurt();
  return true;
}

void Player::Heal(int amount) {
  hp += amount;
  if (hp > maxHp) hp = maxHp;
}

bool Player::UpgradeSpeed() {
  if (levelPoints > 0) {
    levelPoints--;
    moveSpeedMultiplier += 0.15f;
    return true;
  }
  return false;
}

bool Player::UpgradeDash() {
  if (levelPoints > 0) {
    levelPoints--;
    dashCooldownMultiplier *= 0.8f;
    return true;
  }
  return false;
}

bool Player::UpgradeDoubleJump() {
  if (levelPoints > 0 && !hasDoubleJump) {
    levelPoints--;
    hasDoubleJump = true;
    maxJumps = 2;
    return true;
  }
  return false;
}

bool Player::UpgradeMaxHP() {
  if (levelPoints > 0) {
    levelPoints--;
    maxHp += 25;
    hp += 25;
    return true;
  }
  return false;
}