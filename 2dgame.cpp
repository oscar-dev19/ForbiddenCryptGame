#include <raylib.h>
#include <iostream>
#include "Samurai.h"
#include "Goblin.h"

int main() {
    InitWindow(800, 600, "2D  Game");

    // Creating Samurai.
    Samurai samurai((Vector2) {400, 300});
    samurai.loadTextures();

    // Creating Goblin.
    Goblin goblin((Vector2) {400, 300});
    goblin.loadTextures();

    // Set FPS at 60.
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update.
        samurai.move();
        samurai.applyVelocity();
        samurai.updateAnimation();

        goblin.move();
        goblin.applyVelocity();
        goblin.updateAnimation();

        // Drawing.
        BeginDrawing();
        ClearBackground(GREEN);
        samurai.draw();
        goblin.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}