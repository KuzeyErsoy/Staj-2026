#include "Collectible.h"

Collectible::Collectible() {
    basePosition = (Vector3){ 0.0f, 0.75f, 0.0f };
    position = basePosition;
    active = true;
    rotationAngle = 0.0f;
    size = 1.5f;
}

Collectible::Collectible(Vector3 pos) {
    basePosition = pos;
    position = pos;
    active = true;
    rotationAngle = (float)GetRandomValue(0, 360);
    size = 1.5f;
}

void Collectible::Update() {
    if (!active) return;

    // Rotation animation
    rotationAngle += 90.0f * GetFrameTime();
    if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;

    // Floating/Bobbing animation using sin wave
    float time = (float)GetTime();
    position.y = basePosition.y + sinf(time * 3.0f + basePosition.x) * 0.25f;
}

void Collectible::Draw() {
    if (!active) return;

    // Draw rotating cube
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
    
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, size, size, size, GOLD);
    DrawCubeWires((Vector3){ 0.0f, 0.0f, 0.0f }, size, size, size, ORANGE);
    
    rlPopMatrix();
}

BoundingBox Collectible::GetBounds() const {
    float halfSize = size / 2.0f;
    return (BoundingBox){
        (Vector3){ position.x - halfSize, position.y - halfSize, position.z - halfSize },
        (Vector3){ position.x + halfSize, position.y + halfSize, position.z + halfSize }
    };
}
