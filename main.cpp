#include "raylib.h"
int main() {
    InitWindow(800, 600, "Hello Raylib");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;


    while (!WindowShouldClose()) {
        //Fine Tuning Camera Movement
        if (IsKeyDown(KEY_RIGHT) && IsKeyDown(KEY_SPACE)) {
            camera.position.x -= 0.4f;
        }
        if (IsKeyDown(KEY_LEFT) && IsKeyDown(KEY_SPACE)) {
            camera.position.x += 0.4f;
        }
        if (IsKeyDown(KEY_UP) && IsKeyDown(KEY_SPACE)) {
            camera.position.z += 0.4f;
        }
        if (IsKeyDown(KEY_DOWN) && IsKeyDown(KEY_SPACE)) {
            camera.position.z -= 0.4f;
        }


        //Normal Focus Camera Movement
        if (IsKeyDown(KEY_RIGHT)) {
            camera.position.x += 0.5f;
        }
        if (IsKeyDown(KEY_LEFT)) {
            camera.position.x -= 0.5f;
        }
        if (IsKeyDown(KEY_UP)) {
            camera.position.z -= 0.5f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            camera.position.z += 0.5f;
        }
        //Fast Camera Movement
        if (IsKeyDown(KEY_RIGHT) && IsKeyDown(KEY_LEFT_SHIFT)) {
            camera.position.x += 1.0f;
        }
        if (IsKeyDown(KEY_LEFT) && IsKeyDown(KEY_LEFT_SHIFT)) {
            camera.position.x -= 1.0f;
        }
        if (IsKeyDown(KEY_UP) && IsKeyDown(KEY_LEFT_SHIFT)) {
            camera.position.z -= 1.0f;
        }
        if (IsKeyDown(KEY_DOWN) && IsKeyDown(KEY_LEFT_SHIFT)) {
            camera.position.z += 1.0f;
        }

        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
        DrawGrid(20, 3.0f);
        DrawCube((Vector3){ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, RED);
        DrawCubeWires((Vector3){ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, GREEN);

        EndMode3D();
        DrawText("Use arrow keys to move the camera", 10, 10, 20, DARKGRAY);
        EndDrawing();

    }
    CloseWindow();

    return 0;
}