#include "raylib.h"

int main() {
    InitWindow(800, 450,"Deneme 1");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        
        DrawText("DENEME 2", 0, 0, 0, RED);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}