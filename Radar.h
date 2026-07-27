#pragma once
#include "raylib.h"
#include "Player.h"
#include "Collectible.h"
#include <vector>

class Radar {
public:
    Radar(int centerX = 690, int centerY = 110, float radius = 80.0f, float worldRadius = 40.0f);
    ~Radar();

    void Draw(const Player& player, const std::vector<Collectible>& collectibles, float cameraYaw);

private:
    int centerX;
    int centerY;
    float radius;
    float worldRadius;
};
