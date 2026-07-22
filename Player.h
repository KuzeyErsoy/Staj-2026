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
    
    Player(Vector3 startPos);
    ~Player(); 

    void Update(float cameraYaw);
    void Draw();
    BoundingBox GetBounds();
    void AddExp(int amount);

private:
    Model model;
    Texture2D texture;
    Vector3 scale;
};