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
    
    int exp;
    int maxExp;
    int level;
    int levelPoints;

    // Skill & stat modifiers for future skill selection
    float moveSpeedMultiplier;
    float dashCooldownMultiplier;

    Player(Vector3 startPos);
    ~Player(); 

    void Update(float cameraYaw);
    void Draw();
    BoundingBox GetBounds();
    void AddExp(int amount);
    bool HasLevelPoints() const { return levelPoints > 0; }
    bool UpgradeSpeed();
    bool UpgradeDash();

private:
    Model model;
    Texture2D texture;
    Vector3 scale;
};