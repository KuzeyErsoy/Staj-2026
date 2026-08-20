#include "AudioManager.h"
#include "CameraController.h"
#include "Collectible.h"
#include "Drone.h"
#include "DroneSystem.h"
#include "Enemy.h"
#include "Map.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "Radar.h"
#include "SaveSystem.h"
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
#define MAX_BOXES 20

enum GameState {
  STATE_MENU,
  STATE_PLAYING,
  STATE_LEVEL_UP,
  STATE_GAME_OVER,
  STATE_VICTORY,
  STATE_FLEET_SIM
};

Vector3 GetRandomCollectiblePos() {
  float x = (float)GetRandomValue(-25, 28);
  float z = (float)GetRandomValue(-25, 28);
  float heights[] = {0.75f, 2.75f, 4.75f, 6.75f, 8.75f, 10.75f};
  float y = heights[GetRandomValue(0, 5)];
  return Vector3{x, y, z};
}

void InitLevelContent(int levelIdx, Map &gameMap, std::vector<Enemy> &enemies,
                      Player &player, std::vector<Collectible> &boxes) {
  // Reset player position & status
  player.position = Vector3{10.0f, 10.0f, 0.0f};
  player.velocity = Vector3{0.0f, 0.0f, 0.0f};
  player.hp = player.maxHp;

  // Load Level Map
  std::string levelPath;
  if (levelIdx == 99) {
    levelPath = "resources/map/level_test.txt";
    if (!FileExists(levelPath.c_str()))
      levelPath = "../resources/map/level_test.txt";
  } else {
    levelPath = TextFormat("resources/map/level%d.txt", levelIdx);
    if (!FileExists(levelPath.c_str()))
      levelPath = TextFormat("../resources/map/level%d.txt", levelIdx);
  }
  gameMap.LoadLevelFromFile(levelPath);

  // Setup Enemies
  enemies.clear();
  if (levelIdx == 99) { // Test Arena
    enemies.push_back(Enemy(Vector3{-12.0f, 1.0f, -12.0f},
                            Vector3{12.0f, 1.0f, -12.0f}, 4.0f));
    enemies.push_back(
        Enemy(Vector3{15.0f, 1.0f, 5.0f}, Vector3{15.0f, 1.0f, -15.0f}, 4.5f));
    enemies.push_back(
        Enemy(Vector3{-10.0f, 1.0f, 10.0f}, Vector3{10.0f, 1.0f, 10.0f}, 4.0f));
  } else if (levelIdx == 1) {
    enemies.push_back(Enemy(Vector3{-10.0f, 1.0f, -10.0f},
                            Vector3{10.0f, 1.0f, -10.0f}, 3.5f));
    enemies.push_back(
        Enemy(Vector3{15.0f, 1.0f, 5.0f}, Vector3{15.0f, 1.0f, -15.0f}, 4.0f));
  } else if (levelIdx == 2) {
    enemies.push_back(Enemy(Vector3{-15.0f, 1.0f, -15.0f},
                            Vector3{15.0f, 1.0f, -15.0f}, 4.0f));
    enemies.push_back(
        Enemy(Vector3{-10.0f, 1.0f, 10.0f}, Vector3{10.0f, 1.0f, 10.0f}, 4.5f));
    enemies.push_back(
        Enemy(Vector3{0.0f, 3.0f, -5.0f}, Vector3{0.0f, 3.0f, 15.0f}, 3.5f));
  } else { // Level 3
    enemies.push_back(Enemy(Vector3{-18.0f, 1.0f, -18.0f},
                            Vector3{18.0f, 1.0f, -18.0f}, 5.0f));
    enemies.push_back(
        Enemy(Vector3{-18.0f, 1.0f, 18.0f}, Vector3{18.0f, 1.0f, 18.0f}, 5.0f));
    enemies.push_back(
        Enemy(Vector3{-10.0f, 5.0f, 0.0f}, Vector3{10.0f, 5.0f, 0.0f}, 4.5f));
  }

  // Reset Collectible Boxes
  boxes.clear();
  for (int i = 0; i < MAX_BOXES; i++) {
    boxes.push_back(Collectible(GetRandomCollectiblePos()));
  }
}

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1440, 860,
             "Tactical Radar - Military Drone Fleet Simulation & Combat Arena");
  SetTargetFPS(60);

  // Initialize Procedural Sound Synth Engine
  AudioManager::Instance().Init();

  Player player(Vector3{10.0f, 10.0f, 0.0f});
  Map gameMap;
  CameraController cameraController;
  Radar radar(680, 100, 75.0f, 40.0f);
  ParticleSystem particleSystem;
  Drone drone(Vector3{8.0f, 12.0f, -2.0f});
  DroneSystemManager fleetManager;
  fleetManager.Init();

  std::vector<Enemy> enemies;
  std::vector<Collectible> boxes;
  int currentLevel = 1;
  int totalBoxesCollected = 0;
  float enemySpawnTimer = 0.0f;

  InitLevelContent(currentLevel, gameMap, enemies, player, boxes);

  GameState currentState = STATE_MENU;
  DisableCursor();

  float saveMessageTimer = 0.0f;
  std::string saveMessage = "";

  // Exit Portal Goal
  Vector3 portalPos = Vector3{0.0f, 1.0f, 25.0f};

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_F11)) {
      ToggleFullscreen();
    }

    if (saveMessageTimer > 0.0f) {
      saveMessageTimer -= dt;
    }

    // --- UPDATE LOGIC BY STATE ---
    switch (currentState) {
    case STATE_MENU: {
      if (IsKeyPressed(KEY_ENTER)) {
        AudioManager::Instance().PlayClick();
        currentState = STATE_PLAYING;
      }
      if (IsKeyPressed(KEY_T)) {
        currentLevel = 99;
        InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
        saveMessage = "ENTERED DRONE TEST ARENA!";
        saveMessageTimer = 3.0f;
        AudioManager::Instance().PlayClick();
        currentState = STATE_PLAYING;
      }
      if (IsKeyPressed(KEY_F7) || IsKeyPressed(KEY_O)) {
        fleetManager.Init();
        AudioManager::Instance().PlayClick();
        currentState = STATE_FLEET_SIM;
      }
      if (IsKeyPressed(KEY_L)) {
        if (SaveSystem::LoadGame("savegame.dat", currentLevel, player)) {
          InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
          saveMessage = "SAVED GAME LOADED!";
          saveMessageTimer = 2.5f;
          AudioManager::Instance().PlayClick();
          currentState = STATE_PLAYING;
        }
      }
      break;
    }
    case STATE_PLAYING: {
      // Drone Control & Mode Toggle Key
      if (IsKeyPressed(KEY_E)) {
        drone.ToggleMode();
      }

      // Drone Test Arena Switcher Key (T)
      if (IsKeyPressed(KEY_T)) {
        currentLevel = 99;
        InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
        saveMessage = "ENTERED DRONE TEST ARENA!";
        saveMessageTimer = 3.0f;
        AudioManager::Instance().PlayClick();
      }

      // Continuous Enemy Spawner in Test Arena
      if (currentLevel == 99) {
        enemySpawnTimer += dt;
        if (enemySpawnTimer >= 3.0f) {
          enemySpawnTimer = 0.0f;

          // Count active enemies
          int activeEnemies = 0;
          for (const auto &e : enemies) {
            if (e.hp > 0)
              activeEnemies++;
          }

          if (activeEnemies < 10) {
            float sx = (float)GetRandomValue(-22, 22);
            float sz = (float)GetRandomValue(-22, 22);
            enemies.push_back(
                Enemy(Vector3{sx, 1.0f, sz}, Vector3{-sx, 1.0f, -sz}, 4.0f));
            particleSystem.EmitImpact(Vector3{sx, 1.0f, sz}, PURPLE);
          }
        }
      }

      cameraController.Update(player, &drone);

      // Only move player if not in drone free flight mode
      if (drone.mode != DRONE_MODE_FREE_FLIGHT) {
        player.Update(cameraController.GetYaw());
        gameMap.HandleCollisions(player);
      }

      drone.Update(player, enemies, dt, cameraController.GetCamera());

      // Quick Save & Quick Load Keys
      if (IsKeyPressed(KEY_F5)) {
        if (SaveSystem::SaveGame("savegame.dat", currentLevel, player)) {
          saveMessage = "GAME SAVED (F5)";
          saveMessageTimer = 2.0f;
          AudioManager::Instance().PlayClick();
        }
      }
      if (IsKeyPressed(KEY_F9)) {
        if (SaveSystem::LoadGame("savegame.dat", currentLevel, player)) {
          InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
          saveMessage = "GAME LOADED (F9)";
          saveMessageTimer = 2.0f;
          AudioManager::Instance().PlayClick();
        }
      }

      // Dash Trail Particle Trigger
      if (player.isDashing) {
        float rad = player.rotationAngle * (PI / 180.0f);
        Vector3 moveDir = Vector3{sinf(rad), 0.0f, cosf(rad)};
        particleSystem.EmitDashTrail(player.position, moveDir);
        cameraController.TriggerShake(0.12f, 0.05f);
      }

      // Collectibles update & pickup collision handling
      BoundingBox playerBounds = player.GetBounds();
      int activeCount = 0;
      for (auto &box : boxes) {
        if (box.active) {
          box.Update();
          if (CheckCollisionBoxes(playerBounds, box.GetBounds())) {
            box.active = false;
            player.AddExp(15);
            totalBoxesCollected++;
            particleSystem.EmitPickupBurst(box.position);
            AudioManager::Instance().PlayCollect();
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

      // Enemies Update & Collision Damage Check
      for (auto &enemy : enemies) {
        if (enemy.hp <= 0)
          continue;
        enemy.Update(player, dt);
        if (CheckCollisionBoxes(playerBounds, enemy.GetBounds())) {
          if (player.TakeDamage(20)) {
            cameraController.TriggerShake(0.35f, 0.25f);
            particleSystem.EmitImpact(player.position, RED);
          }
        }
      }

      // Update Particle System
      particleSystem.Update(dt);

      // Exit Portal Goal Collision & Level Transition
      float pDistToPortal = Vector3Distance(player.position, portalPos);
      if (pDistToPortal < 3.0f && totalBoxesCollected >= 5 &&
          currentLevel != 99) {
        AudioManager::Instance().PlayLevelUp();
        particleSystem.EmitPickupBurst(portalPos);
        if (currentLevel < 3) {
          currentLevel++;
          InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
          saveMessage = TextFormat("ADVANCED TO LEVEL %d!", currentLevel);
          saveMessageTimer = 3.0f;
        } else {
          currentState = STATE_VICTORY;
        }
      }

      // Check Level Up State Trigger
      if (player.HasLevelPoints()) {
        currentState = STATE_LEVEL_UP;
      }

      // Check Death Conditions (HP <= 0 or Fall into Void)
      if (player.hp <= 0 || player.position.y < -15.0f) {
        currentState = STATE_GAME_OVER;
      }
      break;
    }
    case STATE_LEVEL_UP: {
      // Allow camera orbit while paused in level up screen
      cameraController.Update(player, &drone);

      if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        if (player.UpgradeSpeed())
          AudioManager::Instance().PlayClick();
        if (!player.HasLevelPoints())
          currentState = STATE_PLAYING;
      } else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        if (player.UpgradeDash())
          AudioManager::Instance().PlayClick();
        if (!player.HasLevelPoints())
          currentState = STATE_PLAYING;
      } else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
        if (player.UpgradeDoubleJump())
          AudioManager::Instance().PlayClick();
        if (!player.HasLevelPoints())
          currentState = STATE_PLAYING;
      } else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) {
        if (player.UpgradeMaxHP())
          AudioManager::Instance().PlayClick();
        if (!player.HasLevelPoints())
          currentState = STATE_PLAYING;
      }
      break;
    }
    case STATE_GAME_OVER: {
      if (IsKeyPressed(KEY_R)) {
        AudioManager::Instance().PlayClick();
        player.hp = player.maxHp;
        player.position = Vector3{10.0f, 10.0f, 0.0f};
        player.velocity = Vector3{0.0f, 0.0f, 0.0f};
        currentState = STATE_PLAYING;
      }
      break;
    }
    case STATE_VICTORY: {
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
        AudioManager::Instance().PlayClick();
        currentLevel = 1;
        totalBoxesCollected = 0;
        InitLevelContent(currentLevel, gameMap, enemies, player, boxes);
        currentState = STATE_MENU;
      }
      break;
    }
    case STATE_FLEET_SIM: {
      fleetManager.UpdateFleetCamera(cameraController.camera, dt);
      fleetManager.Update(dt);

      if (IsKeyPressed(KEY_T))
        fleetManager.ToggleThermalFLIR();
      if (IsKeyPressed(KEY_N))
        fleetManager.ToggleNightVision();

      if (IsKeyPressed(KEY_M))
        fleetManager.ToggleFullscreenMap();
      if (IsKeyPressed(KEY_I))
        fleetManager.ToggleDroneInspector();
      if (IsKeyPressed(KEY_F1))
        fleetManager.ToggleTelemetryPanel();
      if (IsKeyPressed(KEY_F2))
        fleetManager.ToggleResourcePanel();
      if (IsKeyPressed(KEY_F3))
        fleetManager.ToggleTerminalPanel();
      if (IsKeyPressed(KEY_F4))
        fleetManager.ToggleCameraBanner();
      if (IsKeyPressed(KEY_F6) || IsKeyPressed(KEY_V))
        fleetManager.TogglePiPWindow();
      if (IsKeyPressed(KEY_LEFT_BRACKET))
        fleetManager.CyclePiPTarget(-1);
      if (IsKeyPressed(KEY_RIGHT_BRACKET))
        fleetManager.CyclePiPTarget(1);
      if (IsKeyPressed(KEY_F8))
        fleetManager.ToggleAllPanels();

      if (IsKeyPressed(KEY_ESCAPE)) {
        AudioManager::Instance().PlayClick();
        currentState = STATE_MENU;
      }
      break;
    }
    }

    // 0. Off-screen PiP Video Pass (Rendered before Main Screen Pass)
    if (currentState == STATE_FLEET_SIM) {
      fleetManager.RenderPiPCam();
    }

    // --- DRAW LOGIC BY STATE ---
    BeginDrawing();
    if (currentState == STATE_FLEET_SIM) {
      if (fleetManager.GetVisionMode() == DroneFleet::VisionMode::FLIR_THERMAL) {
        ClearBackground(Color{ 4, 8, 14, 255 });

      } else if (fleetManager.GetVisionMode() == DroneFleet::VisionMode::NVG_GREEN) {
        ClearBackground(Color{ 2, 14, 6, 255 });
      } else {
        ClearBackground(Color{ 10, 16, 26, 255 });
      }
    } else {
      ClearBackground(RAYWHITE);
    }

    // 3D Scene Pass (Rendered in all playing/menu/pause states)
    BeginMode3D(cameraController.GetCamera());

    if (currentState == STATE_FLEET_SIM) {
      fleetManager.Draw3D();
    } else {
      DrawPlane(Vector3{0.0f, 0.0f, 0.0f}, Vector2{100.0f, 100.0f},
                GREEN); // Ground plane
      gameMap.Draw();
      player.Draw();
      drone.Draw();

      for (auto &box : boxes) {
        box.Draw();
      }

      for (const auto &enemy : enemies) {
        enemy.Draw();
      }

      // Render Glowing Exit Portal Goal (if not in test arena)
      if (currentLevel != 99) {
        Color portalColor = (totalBoxesCollected >= 5) ? SKYBLUE : GRAY;
        DrawCylinder(portalPos, 1.8f, 1.8f, 3.5f, 16, Fade(portalColor, 0.7f));
        DrawCylinderWires(portalPos, 1.8f, 1.8f, 3.5f, 16,
                          (totalBoxesCollected >= 5) ? GOLD : DARKGRAY);
      }

      // Render 3D Particle System
      particleSystem.Draw();
    }

    EndMode3D();

    // 2D UI Pass
    switch (currentState) {
    case STATE_MENU: {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                    Fade(BLACK, 0.70f));

      int titleX = GetScreenWidth() / 2 - MeasureText("TACTICAL RADAR", 48) / 2;
      DrawText("TACTICAL RADAR", titleX, 160, 48, GOLD);

      int subX = GetScreenWidth() / 2 -
                 MeasureText("3D Cyber Parkour & Drone Combat", 22) / 2;
      DrawText("3D Cyber Parkour & Drone Combat", subX, 220, 22, LIGHTGRAY);

      int startX = GetScreenWidth() / 2 -
                   MeasureText("[ PRESS ENTER TO START ]", 26) / 2;
      DrawText("[ PRESS ENTER TO START ]", startX, 330, 26, LIME);

      int testX = GetScreenWidth() / 2 -
                  MeasureText("[ PRESS 'T' FOR DRONE TEST ARENA ]", 22) / 2;
      DrawText("[ PRESS 'T' FOR DRONE TEST ARENA ]", testX, 375, 22, MAGENTA);

      int fleetX =
          GetScreenWidth() / 2 -
          MeasureText("[ PRESS 'F7' OR 'O' FOR 9-DRONE FLEET SIMULATION ]", 22) / 2;
      DrawText("[ PRESS 'F7' OR 'O' FOR 9-DRONE FLEET SIMULATION ]", fleetX, 410,
               22, SKYBLUE);

      if (SaveSystem::SaveFileExists("savegame.dat")) {
        int loadX = GetScreenWidth() / 2 -
                    MeasureText("[ PRESS 'L' TO LOAD SAVED GAME ]", 22) / 2;
        DrawText("[ PRESS 'L' TO LOAD SAVED GAME ]", loadX, 450, 22, SKYBLUE);
      }

      int hintX = GetScreenWidth() / 2 -
                  MeasureText("WASD: Move | Shift: Dash | E: Drone Mode | "
                              "Space: Jump | Mouse: Orbit",
                              15) /
                      2;
      DrawText("WASD: Move | Shift: Dash | E: Drone Mode | Space: Jump | "
               "Mouse: Orbit",
               hintX, 520, 15, RAYWHITE);
      break;
    }
    case STATE_PLAYING:
    case STATE_LEVEL_UP: {
      // 2D UI - Level & Health Overlay
      if (currentLevel == 99) {
        DrawText("DRONE TEST ARENA (SPAWNER ACTIVE)", 20, 20, 24, MAGENTA);
      } else {
        DrawText(TextFormat("LEVEL: %d / 3", currentLevel), 20, 20, 26,
                 DARKGREEN);
      }

      // HP Bar
      int hpBarW = 200;
      int hpBarH = 20;
      DrawRectangle(20, 55, hpBarW, hpBarH, DARKGRAY);
      float hpRatio = (float)player.hp / (float)player.maxHp;
      DrawRectangle(20, 55, (int)(hpBarW * hpRatio), hpBarH, RED);
      DrawRectangleLines(20, 55, hpBarW, hpBarH, MAROON);
      DrawText(TextFormat("HP: %d / %d", player.hp, player.maxHp), 25, 57, 15,
               WHITE);

      // Drone Status Banner
      if (drone.mode == DRONE_MODE_FOLLOW_ATTACK) {
        DrawRectangle(20, 85, 330, 26, Fade(DARKBLUE, 0.85f));
        DrawRectangleLines(20, 85, 330, 26, MAGENTA);
        DrawText("DRONE: AUTO-ATTACK [E: Recon Flight]", 28, 90, 15, GOLD);
      } else {
        DrawRectangle(20, 85, 330, 26, Fade(DARKGREEN, 0.85f));
        DrawRectangleLines(20, 85, 330, 26, LIME);
        DrawText("DRONE: FREE FLIGHT [E: Follow Mode]", 28, 90, 15, LIME);
      }

      // Exit Portal Requirement Indicator (if not test arena)
      if (currentLevel != 99) {
        if (totalBoxesCollected < 5) {
          DrawText(TextFormat("PORTAL LOCKED: Collect %d/5 Boxes",
                              totalBoxesCollected),
                   20, 120, 18, RED);
        } else {
          DrawText("PORTAL READY: REACH EXIT PORTAL!", 20, 120, 18, LIME);
        }
      }

      if (player.levelPoints > 0) {
        DrawText(TextFormat("LEVEL POINTS AVAILABLE: %d (UPGRADES!)",
                            player.levelPoints),
                 20, 145, 18, GOLD);
      }

      // 2D UI - EXP Bar at Bottom
      int barWidth = 600;
      int barHeight = 24;
      int barX = (GetScreenWidth() - barWidth) / 2;
      int barY = GetScreenHeight() - 40;

      float expRatio = (float)player.exp / (float)player.maxExp;
      if (expRatio > 1.0f)
        expRatio = 1.0f;
      int fillWidth = (int)(barWidth * expRatio);

      DrawRectangle(barX - 2, barY - 2, barWidth + 4, barHeight + 4, DARKGRAY);
      DrawRectangle(barX, barY, barWidth, barHeight, LIGHTGRAY);
      DrawRectangle(barX, barY, fillWidth, barHeight, GOLD);
      DrawRectangleLines(barX, barY, barWidth, barHeight, ORANGE);
      DrawText(TextFormat("EXP: %d / %d  (Level %d)", player.exp, player.maxExp,
                          player.level),
               barX + barWidth / 2 - 80, barY + 4, 16, DARKBLUE);

      // 2D UI - Tactical Radar HUD (Top Right)
      radar.Draw(player, boxes, enemies, cameraController.GetYaw(), &drone);

      // Save Toast Notification Banner
      if (saveMessageTimer > 0.0f) {
        int toastW = MeasureText(saveMessage.c_str(), 20) + 40;
        int toastX = GetScreenWidth() / 2 - toastW / 2;
        DrawRectangle(toastX, 20, toastW, 35, Fade(GOLD, 0.9f));
        DrawRectangleLines(toastX, 20, toastW, 35, DARKBLUE);
        DrawText(saveMessage.c_str(), toastX + 20, 27, 20, DARKBLUE);
      }

      // Controls Help Menu Overlay
      if (IsKeyDown(KEY_H)) {
        int helpX = GetScreenWidth() - 370;
        DrawRectangle(helpX - 10, 10, 370, 280, Fade(SKYBLUE, 0.90f));
        DrawText("WASD - Character / Drone Flight Movement", helpX, 20, 15,
                 DARKGRAY);
        DrawText("Press E - Switch Drone Mode (Attack / Flight)", helpX, 45, 15,
                 DARKBLUE);
        DrawText("Press T - Enter Drone Test Arena", helpX, 70, 15, PURPLE);
        DrawText("Hold SHIFT - Rapid Dash / Fast Flight", helpX, 95, 15,
                 DARKGRAY);
        DrawText("SPACE / C - Jump / Ascend & Descend Flight", helpX, 120, 15,
                 DARKGRAY);
        DrawText("F5 - Quick Save Game", helpX, 145, 15, DARKGRAY);
        DrawText("F9 - Quick Load Game", helpX, 170, 15, DARKGRAY);
        DrawText("Press V - Toggle strategic view", helpX, 195, 15, DARKGRAY);
        DrawText("Press R - Reset position", helpX, 220, 15, DARKGRAY);
      } else {
        const char *helpText =
            "Press E: Drone Mode | Press T: Test Arena | Hold H: Help";
        DrawText(helpText, 20, GetScreenHeight() - 75, 16, DARKGRAY);
      }

      // Render LEVEL UP Modal Window if in STATE_LEVEL_UP
      if (currentState == STATE_LEVEL_UP) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Fade(BLACK, 0.65f));

        int modalW = 560;
        int modalH = 380;
        int modalX = (GetScreenWidth() - modalW) / 2;
        int modalY = (GetScreenHeight() - modalH) / 2;

        DrawRectangle(modalX, modalY, modalW, modalH, Fade(DARKGRAY, 0.95f));
        DrawRectangleLines(modalX, modalY, modalW, modalH, GOLD);

        int headerX = modalX + modalW / 2 - MeasureText("LEVEL UP!", 32) / 2;
        DrawText("LEVEL UP!", headerX, modalY + 20, 32, GOLD);

        int subX = modalX + modalW / 2 -
                   MeasureText("CHOOSE AN ABILITY UPGRADE", 20) / 2;
        DrawText("CHOOSE AN ABILITY UPGRADE", subX, modalY + 58, 20, RAYWHITE);

        // Skill Option 1
        DrawRectangle(modalX + 30, modalY + 95, modalW - 60, 55,
                      Fade(DARKBLUE, 0.8f));
        DrawRectangleLines(modalX + 30, modalY + 95, modalW - 60, 55, SKYBLUE);
        DrawText("[ 1 ] SPEED BOOST", modalX + 45, modalY + 103, 18, YELLOW);
        DrawText("+15% Character Movement Speed", modalX + 45, modalY + 125, 14,
                 LIGHTGRAY);

        // Skill Option 2
        DrawRectangle(modalX + 30, modalY + 160, modalW - 60, 55,
                      Fade(DARKBLUE, 0.8f));
        DrawRectangleLines(modalX + 30, modalY + 160, modalW - 60, 55, SKYBLUE);
        DrawText("[ 2 ] RAPID DASH", modalX + 45, modalY + 168, 18, YELLOW);
        DrawText("-20% Dash Cooldown Timer", modalX + 45, modalY + 190, 14,
                 LIGHTGRAY);

        // Skill Option 3
        Color opt3Col =
            player.hasDoubleJump ? Fade(GRAY, 0.5f) : Fade(DARKBLUE, 0.8f);
        DrawRectangle(modalX + 30, modalY + 225, modalW - 60, 55, opt3Col);
        DrawRectangleLines(modalX + 30, modalY + 225, modalW - 60, 55,
                           player.hasDoubleJump ? GRAY : SKYBLUE);
        DrawText(player.hasDoubleJump ? "[ 3 ] DOUBLE JUMP (UNLOCKED)"
                                      : "[ 3 ] DOUBLE JUMP",
                 modalX + 45, modalY + 233, 18,
                 player.hasDoubleJump ? LIGHTGRAY : YELLOW);
        DrawText("Perform a 2nd jump mid-air", modalX + 45, modalY + 255, 14,
                 LIGHTGRAY);

        // Skill Option 4
        DrawRectangle(modalX + 30, modalY + 290, modalW - 60, 55,
                      Fade(DARKBLUE, 0.8f));
        DrawRectangleLines(modalX + 30, modalY + 290, modalW - 60, 55, SKYBLUE);
        DrawText("[ 4 ] MAX HP + REFILL", modalX + 45, modalY + 298, 18,
                 YELLOW);
        DrawText("+25 Max HP & Instant Health Refill", modalX + 45,
                 modalY + 320, 14, LIGHTGRAY);

        int footerX =
            modalX + modalW / 2 -
            MeasureText("Press 1, 2, 3, or 4 on keyboard to select", 15) / 2;
        DrawText("Press 1, 2, 3, or 4 on keyboard to select", footerX,
                 modalY + 353, 15, LIME);
      }
      break;
    }
    case STATE_GAME_OVER: {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                    Fade(MAROON, 0.80f));

      int overX = GetScreenWidth() / 2 - MeasureText("GAME OVER", 48) / 2;
      DrawText("GAME OVER", overX, 280, 48, WHITE);

      int causeX =
          GetScreenWidth() / 2 -
          MeasureText("Your HP reached 0 or you fell into the void!", 22) / 2;
      DrawText("Your HP reached 0 or you fell into the void!", causeX, 345, 22,
               RAYWHITE);

      int restartX = GetScreenWidth() / 2 -
                     MeasureText("[ PRESS R TO RESTART LEVEL ]", 26) / 2;
      DrawText("[ PRESS R TO RESTART LEVEL ]", restartX, 440, 26, GOLD);
      break;
    }
    case STATE_VICTORY: {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                    Fade(DARKBLUE, 0.85f));

      int vicX = GetScreenWidth() / 2 - MeasureText("VICTORY!", 52) / 2;
      DrawText("VICTORY!", vicX, 240, 52, GOLD);

      int subVicX =
          GetScreenWidth() / 2 -
          MeasureText("You cleared all tactical gauntlet levels!", 24) / 2;
      DrawText("You cleared all tactical gauntlet levels!", subVicX, 310, 24,
               RAYWHITE);

      int statsX =
          GetScreenWidth() / 2 -
          MeasureText(TextFormat("Total Boxes Collected: %d | Final Level: %d",
                                 totalBoxesCollected, player.level),
                      20) /
              2;
      DrawText(TextFormat("Total Boxes Collected: %d | Final Level: %d",
                          totalBoxesCollected, player.level),
               statsX, 370, 20, SKYBLUE);

      int menuX = GetScreenWidth() / 2 -
                  MeasureText("[ PRESS ENTER FOR MAIN MENU ]", 24) / 2;
      DrawText("[ PRESS ENTER FOR MAIN MENU ]", menuX, 460, 24, LIME);
      break;
    }
    case STATE_FLEET_SIM: {
      fleetManager.Draw2DHUD();
      break;
    }
    }

    EndDrawing();
  }

  AudioManager::Instance().Close();
  CloseWindow();
  return 0;
}