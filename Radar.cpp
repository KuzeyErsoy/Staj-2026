#include "Radar.h"
#include "Drone.h"
#include "raymath.h"
#include <cmath>

Radar::Radar(int centerX, int centerY, float radius, float worldRadius)
    : centerX(centerX), centerY(centerY), radius(radius), worldRadius(worldRadius) {}

Radar::~Radar() {}

void Radar::Draw(const Player& player, const std::vector<Collectible>& collectibles, const std::vector<Enemy>& enemies, float cameraYaw, const Drone* drone) {
    int screenWidth = GetScreenWidth();
    int currentCenterX = screenWidth - (int)radius - 20;
    int currentCenterY = (int)radius + 20;

    // Outer Background Container
    DrawCircle(currentCenterX, currentCenterY, radius + 4, Fade(BLACK, 0.4f));
    DrawCircle(currentCenterX, currentCenterY, radius, Fade(DARKBLUE, 0.75f));

    // Concentric Radar Rings & Crosshairs
    DrawCircleLines(currentCenterX, currentCenterY, radius, DARKGREEN);
    DrawCircleLines(currentCenterX, currentCenterY, radius * 0.66f, Fade(GREEN, 0.35f));
    DrawCircleLines(currentCenterX, currentCenterY, radius * 0.33f, Fade(GREEN, 0.25f));

    DrawLine(currentCenterX - (int)radius, currentCenterY, currentCenterX + (int)radius, currentCenterY, Fade(DARKGREEN, 0.5f));
    DrawLine(currentCenterX, currentCenterY - (int)radius, currentCenterX, currentCenterY + (int)radius, Fade(DARKGREEN, 0.5f));

    // Cardinal Direction Labels
    DrawText("N", currentCenterX - 3, currentCenterY - (int)radius + 4, 10, GREEN);
    DrawText("S", currentCenterX - 3, currentCenterY + (int)radius - 12, 10, GREEN);
    DrawText("E", currentCenterX + (int)radius - 10, currentCenterY - 4, 10, GREEN);
    DrawText("W", currentCenterX - (int)radius + 4, currentCenterY - 4, 10, GREEN);

    // Render Active Collectibles
    for (const auto& box : collectibles) {
        if (!box.active) continue;

        float relX = (box.position.x / worldRadius) * radius;
        float relZ = (box.position.z / worldRadius) * radius;

        float dist = sqrtf(relX * relX + relZ * relZ);
        if (dist > radius - 6.0f) {
            relX = (relX / dist) * (radius - 6.0f);
            relZ = (relZ / dist) * (radius - 6.0f);
        }

        int blipX = currentCenterX + (int)relX;
        int blipY = currentCenterY + (int)relZ;

        DrawCircle(blipX, blipY, 3.5f, GOLD);
        DrawCircleLines(blipX, blipY, 4.5f, Fade(YELLOW, 0.8f));
    }

    // Render Enemies as blinking red dots
    float timeVal = GetTime();
    for (const auto& e : enemies) {
        if (e.hp <= 0) continue;

        float relX = (e.position.x / worldRadius) * radius;
        float relZ = (e.position.z / worldRadius) * radius;

        float dist = sqrtf(relX * relX + relZ * relZ);
        if (dist > radius - 6.0f) {
            relX = (relX / dist) * (radius - 6.0f);
            relZ = (relZ / dist) * (radius - 6.0f);
        }

        int blipX = currentCenterX + (int)relX;
        int blipY = currentCenterY + (int)relZ;

        Color blipCol = RED;
        if (e.state == ENEMY_CHASE) {
            blipCol = ((int)(timeVal * 8.0f) % 2 == 0) ? RED : ORANGE;
        }

        DrawCircle(blipX, blipY, 4.0f, blipCol);
        DrawCircleLines(blipX, blipY, 5.0f, MAROON);
    }

    // Render Drone on Radar if active
    if (drone) {
        float dRelX = (drone->position.x / worldRadius) * radius;
        float dRelZ = (drone->position.z / worldRadius) * radius;

        float dDist = sqrtf(dRelX * dRelX + dRelZ * dRelZ);
        if (dDist > radius - 6.0f) {
            dRelX = (dRelX / dDist) * (radius - 6.0f);
            dRelZ = (dRelZ / dDist) * (radius - 6.0f);
        }

        int dBlipX = currentCenterX + (int)dRelX;
        int dBlipY = currentCenterY + (int)dRelZ;

        Color droneCol = (drone->mode == DRONE_MODE_FREE_FLIGHT) ? LIME : MAGENTA;
        DrawCircle(dBlipX, dBlipY, 4.5f, droneCol);
        DrawCircleLines(dBlipX, dBlipY, 6.0f, PURPLE);
    }

    // Render Player Dot
    float pRelX = (player.position.x / worldRadius) * radius;
    float pRelZ = (player.position.z / worldRadius) * radius;

    float pDist = sqrtf(pRelX * pRelX + pRelZ * pRelZ);
    if (pDist > radius - 6.0f) {
        pRelX = (pRelX / pDist) * (radius - 6.0f);
        pRelZ = (pRelZ / pDist) * (radius - 6.0f);
    }

    int pBlipX = currentCenterX + (int)pRelX;
    int pBlipY = currentCenterY + (int)pRelZ;

    // Player Direction Indicator Vector
    float dirRad = cameraYaw * DEG2RAD;
    float dirX = sinf(dirRad) * 12.0f;
    float dirY = -cosf(dirRad) * 12.0f;

    DrawLine(pBlipX, pBlipY, pBlipX + (int)dirX, pBlipY + (int)dirY, LIME);
    DrawCircle(pBlipX, pBlipY, 4.5f, SKYBLUE);
    DrawCircleLines(pBlipX, pBlipY, 5.5f, BLUE);
}
