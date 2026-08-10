#pragma once
#include "raylib.h"
#include "rlgl.h"
#include <math.h>

class Collectible {
public:
    Vector3 basePosition;
    Vector3 position;
    bool active;
    float rotationAngle;
    float size;

    Collectible();
    Collectible(Vector3 pos);

    void Update();
    void Draw();
    void Respawn(Vector3 newPos);
    BoundingBox GetBounds() const;
};
