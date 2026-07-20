#pragma once
#include "raylib.h"

class Player {
public:
    Vector3 position;
    Vector3 prevPosition;
    Vector3 velocity;
    float rotationAngle;
    bool isGrounded; 
    
    Player(Vector3 startPos);
    ~Player(); 

    void Update(float cameraYaw);
    void Draw();
    BoundingBox GetBounds();

private:
    Model model;
    Texture2D texture;
    Vector3 scale;
};