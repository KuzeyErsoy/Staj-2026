#pragma once
#include "raylib.h"
#include "Player.h"
#include "Collectible.h"
#include "Enemy.h"
#include <vector>

class Drone;

class Radar {
public:
    Radar(int centerX = 690, int centerY = 110, float radius = 80.0f, float worldRadius = 40.0f);
    ~Radar();

    void Draw(const Player& player, const std::vector<Collectible>& collectibles, const std::vector<Enemy>& enemies, float cameraYaw, const Drone* drone = nullptr);

private:
    int centerX;
    int centerY;
    float radius;
    float worldRadius;
};
