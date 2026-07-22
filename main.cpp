#include "CameraController.h"
#include "Collectible.h"
#include "Map.h"
#include "Player.h"
#include "raylib.h"
#include <vector>

#define MAX_BOXES 20

int main() {
  InitWindow(800, 800, "Tactical Radar");
  SetTargetFPS(60);

  Player player((Vector3){10.0f, 10.0f, 0.0f});
  Map gameMap;
  CameraController cameraController;

  std::vector<Collectible> boxes;
  for (int i = 0; i < MAX_BOXES; i++) {
    Vector3 pos = {(float)GetRandomValue(-20, 20), 0.75f,
                   (float)GetRandomValue(-20, 20)};
    boxes.push_back(Collectible(pos));
  }

  DisableCursor();

  while (!WindowShouldClose()) {
    // --- UPDATE ---
    cameraController.Update(player);
    player.Update(cameraController.GetYaw());
    gameMap.HandleCollisions(player);

    // Update collectibles & handle collisions
    BoundingBox playerBounds = player.GetBounds();
    for (auto &box : boxes) {
      if (box.active) {
        box.Update();
        if (CheckCollisionBoxes(playerBounds, box.GetBounds())) {
          box.active = false;
          player.AddExp(10);
        }
      }
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // 3D Scene
    BeginMode3D(cameraController.GetCamera());
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){100.0f, 100.0f},
              GREEN); // Ground
    gameMap.Draw();
    player.Draw();

    for (auto &box : boxes) {
      box.Draw();
    }
    EndMode3D();

    // 2D UI - EXP Bar at Bottom
    int barWidth = 600;
    int barHeight = 25;
    int barX = (GetScreenWidth() - barWidth) / 2;
    int barY = GetScreenHeight() - 40;

    float expRatio = (float)player.exp / (float)player.maxExp;
    if (expRatio > 1.0f)
      expRatio = 1.0f;
    int fillWidth = (int)(barWidth * expRatio);

    // EXP Bar background & fill
    DrawRectangle(barX - 2, barY - 2, barWidth + 4, barHeight + 4, DARKGRAY);
    DrawRectangle(barX, barY, barWidth, barHeight, LIGHTGRAY);
    DrawRectangle(barX, barY, fillWidth, barHeight, GOLD);
    DrawRectangleLines(barX, barY, barWidth, barHeight, ORANGE);
    DrawText(TextFormat("EXP: %d / %d", player.exp, player.maxExp),
             barX + barWidth / 2 - 50, barY + 4, 18, DARKBLUE);

    // Controls Help Menu
    if (IsKeyDown(KEY_H)) {
      int helpX = GetScreenWidth() - 360;
      DrawRectangle(helpX - 10, 10, 360, 220, Fade(SKYBLUE, 0.8f));
      DrawText("Use arrow keys to move the camera (Strategic)", helpX, 20, 15,
               DARKGRAY);
      DrawText("Hold SHIFT to Dash", helpX, 45, 15, DARKGRAY);
      DrawText("Use WASD to move the character", helpX, 120, 15, DARKGRAY);
      DrawText("Press SPACE to jump", helpX, 145, 15, DARKGRAY);
      DrawText("Press T to toggle strategic view", helpX, 170, 15, DARKGRAY);
      DrawText("Press R to reset camera and player", helpX, 195, 15, DARKGRAY);
    } else {
      const char *helpText = "Press H for help";
      DrawText(helpText, GetScreenWidth() - MeasureText(helpText, 20) - 20, 20,
               20, DARKGRAY);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}