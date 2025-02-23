#include <raylib.h>
#include <iostream>
#include  "Samurai.h"

int main() {
    InitWindow(800, 600, "2D  Game");

    // Creating Samurai.
    Samurai samurai((Vector2) {400, 300});
    samurai.loadTextures();

    // Set FPS at 60.
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update.
        samurai.move();
        samurai.applyVelocity();
        samurai.updateAnimation();

        // Drawing.
        BeginDrawing();
        ClearBackground(GREEN);
        samurai.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}