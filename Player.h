#pragma once
#include "raylib.h"

class Player {
public:
    Vector3 position;
    Vector3 prevPosition;
    Vector3 velocity;
    float rotationAngle;
    
    Player(Vector3 startPos);
    ~Player(); // Destructor: Bellek yonetimi icin (RAII)

    void Update();
    void Draw();
    BoundingBox GetBounds();

private:
    Model model;
    Texture2D texture;
    Vector3 scale;
    bool isGrounded;
};