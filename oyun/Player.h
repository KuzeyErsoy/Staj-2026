#pragma once
#include "raylib.h"


class Player {
public:
    Vector3 position;
    Vector3 prevPosition;
    Vector3 velocity;
    float rotationAngle;
    bool isGrounded;
    bool isDashing;
    float dashTimer;
    float dashCooldownTimer;
    Vector3 dashDirection;
    
    // Health & Invincibility
    int hp;
    int maxHp;
    float invincibilityTimer;

    // Movement & Skill Progression
    int jumpCount;
    int maxJumps;
    bool hasDoubleJump;
    bool hasWallJump;
    bool isWallSliding;

    int exp;
    int maxExp;
    int level;
    int levelPoints;

    // Skill & stat modifiers
    float moveSpeedMultiplier;
    float dashCooldownMultiplier;

    Player(Vector3 startPos);
    ~Player(); 

    void Update(float cameraYaw);
    void Draw();
    BoundingBox GetBounds();
    void AddExp(int amount);
    bool TakeDamage(int dmg);
    void Heal(int amount);

    bool HasLevelPoints() const { return levelPoints > 0; }
    bool UpgradeSpeed();
    bool UpgradeDash();
    bool UpgradeDoubleJump();
    bool UpgradeMaxHP();

private:
    Model model;
    Texture2D texture;
    Vector3 scale;
};