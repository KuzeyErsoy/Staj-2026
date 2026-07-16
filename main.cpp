#include "raylib.h"
#include "raymath.h"

#define MAX_BOXES 20

typedef struct {
    Vector3 position;
    bool active;
} Collectible;

int main() {
    InitWindow(800, 800, "Tactical Radar");
    SetTargetFPS(60);

    Vector3 defaultCameraPosition = { 20.0f, 20.0f, 20.0f };
    Vector3 defaultTargetPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 playerPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 modelScale = { 1.0f, 1.00f, 1.0f };

    Camera3D camera = { 0 };
    camera.position = defaultCameraPosition;
    camera.target = defaultTargetPosition;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model characterModel = LoadModel("../resources/character/character-a.obj");
    if (characterModel.meshCount == 0) {
        TraceLog(LOG_ERROR, "Model yuklenemedi!");
    }

    Texture2D characterTexture = LoadTexture("../resources/character/texture-a.png");
    if (characterTexture.id == 0) {
        TraceLog(LOG_ERROR, "Texture yuklenemedi!");
    }
    characterModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = characterTexture;

    int score = 0;
    bool canEat = false;
    int targetBoxIndex = -1;
    bool focusPlayer = false;

    Collectible boxes[MAX_BOXES];
    for (int i = 0; i < MAX_BOXES; i++) {
        boxes[i].position = (Vector3){
            (float)GetRandomValue(-45, 45),
            0.75f, 
            (float)GetRandomValue(-45, 45)
        };
        boxes[i].active = true;
    }

    while (!WindowShouldClose()) {
        Vector3 prevCameraPosition = camera.position;
        Vector3 prevPlayerPosition = playerPosition;

        //CAM MOVEMENTS
        if (IsKeyDown(KEY_R)) {
            camera.position = defaultCameraPosition;
            camera.target = defaultTargetPosition;
            playerPosition = (Vector3){ 0.0f, 0.0f, 0.0f };
            focusPlayer = false;
        }

        if (IsKeyPressed(KEY_T)) {
            focusPlayer = !focusPlayer;
        }

        if (focusPlayer) {
            camera.target = playerPosition;
        } else {
            camera.target = defaultTargetPosition;
        }

        //Change Camera Height
        if (IsKeyDown(KEY_UP) && IsKeyDown(KEY_LEFT_ALT)) {
            camera.position.y += 0.5f;
        }
        if (IsKeyDown(KEY_DOWN) && IsKeyDown(KEY_LEFT_ALT)) {
            camera.position.y -= 0.5f;
        }

        //Normal Focus Camera Movement
        if (IsKeyDown(KEY_RIGHT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.x += 1.0f;
            else if (IsKeyDown(KEY_SPACE)) camera.position.x -= 0.4f;
            else camera.position.x += 0.5f;
        }
        if (IsKeyDown(KEY_LEFT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.x -= 1.0f;
            else if (IsKeyDown(KEY_SPACE)) camera.position.x += 0.4f;
            else camera.position.x -= 0.5f;
        }
        if (IsKeyDown(KEY_UP) && !IsKeyDown(KEY_LEFT_ALT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.z -= 1.0f;
            else if (IsKeyDown(KEY_SPACE)) camera.position.z += 0.4f;
            else camera.position.z -= 0.5f;
        }
        if (IsKeyDown(KEY_DOWN) && !IsKeyDown(KEY_LEFT_ALT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.z += 1.0f;
            else if (IsKeyDown(KEY_SPACE)) camera.position.z -= 0.4f;
            else camera.position.z += 0.5f;
        }
        
        //Player Movements
        if (IsKeyDown(KEY_W)) {
            playerPosition.z -= 0.5f;
        }
        if (IsKeyDown(KEY_S)) {
            playerPosition.z += 0.5f;
        }
        if (IsKeyDown(KEY_A)) {
            playerPosition.x -= 0.5f;
        }
        if (IsKeyDown(KEY_D)) {
            playerPosition.x += 0.5f;
        }

        //Player Movement Boundaries
        if (playerPosition.x < -50.0f) playerPosition.x = -50.0f;
        if (playerPosition.x > 50.0f) playerPosition.x = 50.0f;
        if (playerPosition.z < -50.0f) playerPosition.z = -50.0f;
        if (playerPosition.z > 50.0f) playerPosition.z = 50.0f;

        //Eating Mechanics
        canEat = false;
        targetBoxIndex = -1;
        for (int i = 0; i < MAX_BOXES; i++) {
            if (boxes[i].active) {
                float dist = Vector3Distance(playerPosition, boxes[i].position);
                if (dist < 3.5f) {
                    canEat = true;
                    targetBoxIndex = i;
                    break;
                }
            }
        }

        if (canEat && IsKeyPressed(KEY_E)) {
            boxes[targetBoxIndex].active = false;
            score += 10;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
        
        DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 100.0f, 100.0f }, LIGHTGRAY);
        DrawGrid(100, 1.0f);
        
        DrawModelEx(characterModel, playerPosition, (Vector3){ 0.0f, 1.0f, 0.0f }, 0.0f, modelScale, WHITE);

        for (int i = 0; i < MAX_BOXES; i++) {
            if (boxes[i].active) {
                DrawCube(boxes[i].position, 1.5f, 1.5f, 1.5f, RED);
                DrawCubeWires(boxes[i].position, 1.5f, 1.5f, 1.5f, MAROON);
            }
        }

        EndMode3D();

        //UI
        DrawText(TextFormat("SCORE: %d", score), 20, 20, 30, DARKGREEN);

        if (canEat) {
            DrawText("PRESS 'E' TO INTERACT", 300, 600, 30, RED);
        }

        //HELP INSTRUCTIONS
        if (IsKeyDown(KEY_H)) {
            int helpX = GetScreenWidth() - 360;
            DrawRectangle(helpX - 10, 10, 360, 200, Fade(SKYBLUE, 0.8f));
            DrawText("Use arrow keys to move the camera", helpX, 20, 15, DARKGRAY);
            DrawText("Hold ALT + UP/DOWN to change camera height", helpX, 45, 15, DARKGRAY);
            DrawText("Hold SHIFT to move faster", helpX, 70, 15, DARKGRAY);
            DrawText("Hold SPACE to move slower", helpX, 95, 15, DARKGRAY);
            DrawText("Use WASD to move the character", helpX, 120, 15, DARKGRAY);
            DrawText("Press T to toggle camera focus", helpX, 145, 15, DARKGRAY);
            DrawText("Press R to reset camera and player", helpX, 170, 15, DARKGRAY);
        } else {
            const char* helpText = "Press H for help";
            DrawText(helpText, GetScreenWidth() - MeasureText(helpText, 20) - 20, 20, 20, DARKGRAY);
        }

        EndDrawing();
    }

    UnloadTexture(characterTexture);
    UnloadModel(characterModel);
    
    CloseWindow();

    return 0;
}