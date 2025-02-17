#include <raylib.h>
#include <iostream>

int main() {
    InitWindow(1280, 720, "2D  Game");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}