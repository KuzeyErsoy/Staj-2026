#include "CameraController.h"
#include "Collectible.h"
#include "Map.h"
#include "Player.h"
#include "Radar.h"
#include "raylib.h"
#include <vector>

#define MAX_BOXES 20

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_LEVEL_UP,
    STATE_GAME_OVER
};

Vector3 GetRandomCollectiblePos() {
    float x = (float)GetRandomValue(-25, 28);
    float z = (float)GetRandomValue(-25, 28);
    float heights[] = { 0.75f, 2.75f, 4.75f, 6.75f, 8.75f, 10.75f };
    float y = heights[GetRandomValue(0, 5)];
    return (Vector3){ x, y, z };
}

int main() {
    InitWindow(800, 800, "Tactical Radar - 3D Platformer");
    SetTargetFPS(60);

    Player player((Vector3){10.0f, 10.0f, 0.0f});
    Map gameMap;
    CameraController cameraController;
    Radar radar(680, 100, 75.0f, 40.0f);

    std::vector<Collectible> boxes;
    for (int i = 0; i < MAX_BOXES; i++) {
        boxes.push_back(Collectible(GetRandomCollectiblePos()));
    }

    GameState currentState = STATE_MENU;
    DisableCursor();

    while (!WindowShouldClose()) {
        // --- UPDATE LOGIC BY STATE ---
        switch (currentState) {
            case STATE_MENU: {
                if (IsKeyPressed(KEY_ENTER)) {
                    currentState = STATE_PLAYING;
                }
                break;
            }
            case STATE_PLAYING: {
                cameraController.Update(player);
                player.Update(cameraController.GetYaw());
                gameMap.HandleCollisions(player);

                // Collectibles update & pickup collision handling
                BoundingBox playerBounds = player.GetBounds();
                int activeCount = 0;
                for (auto &box : boxes) {
                    if (box.active) {
                        box.Update();
                        if (CheckCollisionBoxes(playerBounds, box.GetBounds())) {
                            box.active = false;
                            player.AddExp(10);
                        } else {
                            activeCount++;
                        }
                    }
                }

                // Dynamic Respawn Wave
                if (activeCount <= 5) {
                    for (auto &box : boxes) {
                        if (!box.active) {
                            box.Respawn(GetRandomCollectiblePos());
                        }
                    }
                }

                // Check Level Up State Trigger
                if (player.HasLevelPoints()) {
                    currentState = STATE_LEVEL_UP;
                }

                // Check Death / Void Fall Condition
                if (player.position.y < -15.0f) {
                    currentState = STATE_GAME_OVER;
                }
                break;
            }
            case STATE_LEVEL_UP: {
                // Allow camera orbit while paused in level up screen
                cameraController.Update(player);

                if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
                    player.UpgradeSpeed();
                    if (!player.HasLevelPoints()) {
                        currentState = STATE_PLAYING;
                    }
                } else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
                    player.UpgradeDash();
                    if (!player.HasLevelPoints()) {
                        currentState = STATE_PLAYING;
                    }
                }
                break;
            }
            case STATE_GAME_OVER: {
                if (IsKeyPressed(KEY_R)) {
                    player.position = (Vector3){ 10.0f, 10.0f, 0.0f };
                    player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
                    currentState = STATE_PLAYING;
                }
                break;
            }
        }

        // --- DRAW LOGIC BY STATE ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 3D Scene Pass (Rendered in all playing/menu/pause states)
        BeginMode3D(cameraController.GetCamera());
        DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){100.0f, 100.0f}, GREEN); // Ground plane
        gameMap.Draw();
        player.Draw();

        for (auto &box : boxes) {
            box.Draw();
        }
        EndMode3D();

        // 2D UI Pass
        switch (currentState) {
            case STATE_MENU: {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));
                
                int titleX = GetScreenWidth() / 2 - MeasureText("TACTICAL RADAR", 44) / 2;
                DrawText("TACTICAL RADAR", titleX, 220, 44, GOLD);

                int subX = GetScreenWidth() / 2 - MeasureText("3D Collectathon & Parkour Platformer", 22) / 2;
                DrawText("3D Collectathon & Parkour Platformer", subX, 275, 22, LIGHTGRAY);

                int startX = GetScreenWidth() / 2 - MeasureText("[ PRESS ENTER TO START ]", 26) / 2;
                DrawText("[ PRESS ENTER TO START ]", startX, 420, 26, LIME);

                int hintX = GetScreenWidth() / 2 - MeasureText("WASD: Move | Shift: Dash | Space: Jump | Mouse: Orbit Camera", 16) / 2;
                DrawText("WASD: Move | Shift: Dash | Space: Jump | Mouse: Orbit Camera", hintX, 520, 16, RAYWHITE);
                break;
            }
            case STATE_PLAYING:
            case STATE_LEVEL_UP: {
                // 2D UI - Level & Stats Overlay
                DrawText(TextFormat("LEVEL: %d", player.level), 20, 20, 26, DARKGREEN);
                if (player.levelPoints > 0) {
                    DrawText(TextFormat("LEVEL POINTS: %d", player.levelPoints), 20, 52, 24, GOLD);
                }

                // 2D UI - EXP Bar at Bottom
                int barWidth = 600;
                int barHeight = 25;
                int barX = (GetScreenWidth() - barWidth) / 2;
                int barY = GetScreenHeight() - 40;

                float expRatio = (float)player.exp / (float)player.maxExp;
                if (expRatio > 1.0f) expRatio = 1.0f;
                int fillWidth = (int)(barWidth * expRatio);

                DrawRectangle(barX - 2, barY - 2, barWidth + 4, barHeight + 4, DARKGRAY);
                DrawRectangle(barX, barY, barWidth, barHeight, LIGHTGRAY);
                DrawRectangle(barX, barY, fillWidth, barHeight, GOLD);
                DrawRectangleLines(barX, barY, barWidth, barHeight, ORANGE);
                DrawText(TextFormat("EXP: %d / %d", player.exp, player.maxExp),
                         barX + barWidth / 2 - 60, barY + 4, 18, DARKBLUE);

                // 2D UI - Tactical Radar HUD (Top Right)
                radar.Draw(player, boxes, cameraController.GetYaw());

                // Controls Help Menu Overlay
                if (IsKeyDown(KEY_H)) {
                    int helpX = GetScreenWidth() - 360;
                    DrawRectangle(helpX - 10, 10, 360, 220, Fade(SKYBLUE, 0.85f));
                    DrawText("Use arrow keys for camera (Strategic View)", helpX, 20, 15, DARKGRAY);
                    DrawText("Hold SHIFT to Dash", helpX, 45, 15, DARKGRAY);
                    DrawText("Use WASD to move the character", helpX, 120, 15, DARKGRAY);
                    DrawText("Press SPACE to jump", helpX, 145, 15, DARKGRAY);
                    DrawText("Press T to toggle strategic view", helpX, 170, 15, DARKGRAY);
                    DrawText("Press R to reset camera and player", helpX, 195, 15, DARKGRAY);
                } else {
                    const char *helpText = "Hold H for controls help";
                    DrawText(helpText, 20, GetScreenHeight() - 75, 18, DARKGRAY);
                }

                // Render LEVEL UP Modal Window if in STATE_LEVEL_UP
                if (currentState == STATE_LEVEL_UP) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.60f));

                    int modalW = 520;
                    int modalH = 300;
                    int modalX = (GetScreenWidth() - modalW) / 2;
                    int modalY = (GetScreenHeight() - modalH) / 2;

                    DrawRectangle(modalX, modalY, modalW, modalH, Fade(DARKGRAY, 0.95f));
                    DrawRectangleLines(modalX, modalY, modalW, modalH, GOLD);

                    int headerX = modalX + modalW / 2 - MeasureText("LEVEL UP!", 32) / 2;
                    DrawText("LEVEL UP!", headerX, modalY + 25, 32, GOLD);

                    int subX = modalX + modalW / 2 - MeasureText("CHOOSE AN UPGRADE", 20) / 2;
                    DrawText("CHOOSE AN UPGRADE", subX, modalY + 65, 20, RAYWHITE);

                    // Skill Option 1
                    DrawRectangle(modalX + 40, modalY + 110, modalW - 80, 60, Fade(DARKBLUE, 0.8f));
                    DrawRectangleLines(modalX + 40, modalY + 110, modalW - 80, 60, SKYBLUE);
                    DrawText("[ 1 ] SPEED BOOST", modalX + 55, modalY + 120, 20, YELLOW);
                    DrawText("+15% Character Movement Speed", modalX + 55, modalY + 144, 15, LIGHTGRAY);

                    // Skill Option 2
                    DrawRectangle(modalX + 40, modalY + 185, modalW - 80, 60, Fade(DARKBLUE, 0.8f));
                    DrawRectangleLines(modalX + 40, modalY + 185, modalW - 80, 60, SKYBLUE);
                    DrawText("[ 2 ] RAPID DASH", modalX + 55, modalY + 195, 20, YELLOW);
                    DrawText("-20% Dash Cooldown Timer", modalX + 55, modalY + 219, 15, LIGHTGRAY);

                    int footerX = modalX + modalW / 2 - MeasureText("Press '1' or '2' on keyboard to select", 16) / 2;
                    DrawText("Press '1' or '2' on keyboard to select", footerX, modalY + 265, 16, LIME);
                }
                break;
            }
            case STATE_GAME_OVER: {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(MAROON, 0.75f));

                int overX = GetScreenWidth() / 2 - MeasureText("GAME OVER", 48) / 2;
                DrawText("GAME OVER", overX, 300, 48, WHITE);

                int voidX = GetScreenWidth() / 2 - MeasureText("You fell into the infinite void!", 22) / 2;
                DrawText("You fell into the infinite void!", voidX, 365, 22, RAYWHITE);

                int restartX = GetScreenWidth() / 2 - MeasureText("[ PRESS R TO RESTART ]", 26) / 2;
                DrawText("[ PRESS R TO RESTART ]", restartX, 450, 26, GOLD);
                break;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}