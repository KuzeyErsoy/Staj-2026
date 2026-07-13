#include "raylib.h"

int main() {
    InitWindow(800, 450,"Deneme1");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        
        DrawText("DENEME2", 0, 0, 0, RED);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}