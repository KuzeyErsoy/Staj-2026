#include "raylib.h"
int main() {
    InitWindow(800, 800, "Tactical Radar");
    SetTargetFPS(60);

    Vector3 defaultCameraPosition = { 10.0f, 10.0f, 10.0f };
    Vector3 defaultTargetPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 cubePosition = { 0.0f, 5.0f, 0.0f };
    Vector3 cubeSize = { 10.0f, 10.0f, 10.0f };

    Camera3D camera = { 0 };
    camera.position = defaultCameraPosition;
    camera.target = defaultTargetPosition; // Set the camera to look at the default target position
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;


    while (!WindowShouldClose()) {
        Vector3 prevCameraPosition = camera.position;
        Vector3 prevCubePosition = cubePosition;

    //CAM MOVEMENTS


        if (IsKeyDown(KEY_R)) {
            camera.position = defaultCameraPosition;
            camera.target = defaultTargetPosition;
            cubePosition = { 0.0f, 5.0f, 0.0f };
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
        
        //Cube Movements
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


        //Cube Movement Boundaries
        if (cubePosition.x < -20.0f) cubePosition.x = -20.0f;
        if (cubePosition.x > 20.0f) cubePosition.x = 20.0f;
        if (cubePosition.z < -20.0f) cubePosition.z = -20.0f;
        if (cubePosition.z > 20.0f) cubePosition.z = 20.0f;
        
        float halfX = cubeSize.x / 2.0f;
        float halfZ = cubeSize.z / 2.0f;
        float halfY = cubeSize.y / 2.0f;

        //Collision Detector
        bool isCollision = (camera.position.x >= cubePosition.x - halfX && camera.position.x <= cubePosition.x + halfX) &&
                           (camera.position.z >= cubePosition.z - halfZ && camera.position.z <= cubePosition.z + halfZ) &&
                           (camera.position.y >= cubePosition.y - halfY && camera.position.y <= cubePosition.y + halfY);
        
        if (isCollision) {
            cubePosition = prevCubePosition;
            camera.position = prevCameraPosition;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
        DrawGrid(20, 3.0f);
        DrawCube(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, RED);
        DrawCubeWires(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, GREEN);
        DrawLine3D(Vector3 {-20.0f, 0.0f, -20.0f}, {-20.0f, 0.0f, 20.0f}, RED);
        DrawLine3D(Vector3 {-20.0f, 0.0f, -20.0f}, {20.0f, 0.0f, -20.0f}, RED);
        DrawLine3D(Vector3 {20.0f, 0.0f, -20.0f}, {20.0f, 0.0f, 20.0f}, RED);
        DrawLine3D(Vector3 {-20.0f, 0.0f, 20.0f}, {20.0f, 0.0f, 20.0f}, RED);

        EndMode3D();
        //HELP INSTRUCTIONS
        
        if (IsKeyDown(KEY_H)) {
            DrawText("Use arrow keys to move the camera", 10, 10, 20, DARKGRAY);
            DrawText("Hold ALT(OPTION FOR MAC) + UP/DOWN to change camera height", 10, 40, 20, DARKGRAY);
            DrawText("Hold SHIFT to move faster", 10, 70, 20, DARKGRAY);
            DrawText("Hold SPACE to move slower", 10, 100, 20, DARKGRAY);
            DrawText("Use WASD to move the cube", 10, 130, 20, DARKGRAY);
            DrawText("Press R to reset camera and cube position", 10, 160, 20, DARKGRAY);
        } else {
            DrawText("Press H for help", 10, 10, 20, DARKGRAY);
        }
        EndDrawing();

    }
    CloseWindow();

    return 0;
}