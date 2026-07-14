#include "raylib.h"
int main() {
    InitWindow(800, 800, "Tactical Radar");
    SetTargetFPS(60);

    Vector3 defaultCameraPosition = { 0.0f, 10.0f, 10.0f };
    Vector3 defaultTargetPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 cubePosition = { 0.0f, 0.5f, 0.0f };

    Camera3D camera = { 0 };
    camera.position = defaultCameraPosition;
    camera.target = defaultTargetPosition;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;


    while (!WindowShouldClose()) {
    
    //CAM MOVEMENTS


        if (IsKeyDown(KEY_R)) {
            camera.position = defaultCameraPosition;
            camera.target = defaultTargetPosition;
            cubePosition = { 0.0f, 0.5f, 0.0f };
        }

        //Normal Focus Camera Movement
        if (IsKeyDown(KEY_RIGHT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                camera.position.x += 1.0f;
            }
            else if (IsKeyDown(KEY_SPACE)) {
                camera.position.x -= 0.4f;
            }
            else {
                camera.position.x += 0.5f;
            }
        }
        if (IsKeyDown(KEY_LEFT)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                camera.position.x -= 1.0f;
            }
            else if (IsKeyDown(KEY_SPACE)) {
                camera.position.x += 0.4f;
            }
            else {
                camera.position.x -= 0.5f;
            }
            camera.position.x -= 0.5f;
        }
        if (IsKeyDown(KEY_UP)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                camera.position.z -= 1.0f;
            }
            else if (IsKeyDown(KEY_SPACE)) {
                camera.position.z += 0.4f;
            }
            else {
                camera.position.z -= 0.5f;
            }
            camera.position.z -= 0.5f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                camera.position.z += 1.0f;
            }
            else if (IsKeyDown(KEY_SPACE)) {
                camera.position.z -= 0.4f;
            }
            else {
                camera.position.z += 0.5f;
            }
            camera.position.z += 0.5f;
        }

        if (IsKeyDown(KEY_W)) {
            cubePosition.z -= 0.5f;
        }
        if (IsKeyDown(KEY_S)) {
            cubePosition.z += 0.5f;
        }
        if (IsKeyDown(KEY_A)) {
            cubePosition.x -= 0.5f;
        }
        if (IsKeyDown(KEY_D)) {
            cubePosition.x += 0.5f;
        }

        if (cubePosition.x < -10.0f) cubePosition.x = -10.0f;
        if (cubePosition.x > 10.0f) cubePosition.x = 10.0f;
        if (cubePosition.z < -10.0f) cubePosition.z = -10.0f;
        if (cubePosition.z > 10.0f) cubePosition.z = 10.0f;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
        DrawGrid(20, 3.0f);
        DrawCube(cubePosition, 1.0f, 1.0f, 1.0f, RED);
        DrawCubeWires(cubePosition, 1.0f, 1.0f, 1.0f, GREEN);
        DrawLine3D(Vector3 {-10.5f, 0.0f, -10.5f}, {-10.5f, 0.0f, 10.5f}, RED);
        DrawLine3D(Vector3 {-10.5f, 0.0f, -10.5f}, {10.5f, 0.0f, -10.5f}, RED);
        DrawLine3D(Vector3 {10.5f, 0.0f, -10.5f}, {10.5f, 0.0f, 10.5f}, RED);
        DrawLine3D(Vector3 {-10.5f, 0.0f, 10.5f}, {10.5f, 0.0f, 10.5f}, RED);

        EndMode3D();
        //HELP INSTRUCTIONS
        
        if (IsKeyDown(KEY_H)) {
            DrawText("Use arrow keys to move the camera", 10, 10, 20, DARKGRAY);
            DrawText("Hold SHIFT to move faster", 10, 40, 20, DARKGRAY);
            DrawText("Hold SPACE to move slower", 10, 70, 20, DARKGRAY);
            DrawText("Use WASD to move the cube", 10, 100, 20, DARKGRAY);
            DrawText("Press R to reset camera and cube position", 10, 130, 20, DARKGRAY);
        } else {
            DrawText("Press H for help", 10, 10, 20, DARKGRAY);
        }
        EndDrawing();

    }
    CloseWindow();

    return 0;
}