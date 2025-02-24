#include <raylib.h>
#include <iostream>
#include "Samurai.h"
#include "Goblin.h"
#include "Werewolf.h"

int main() {
    InitWindow(800, 600, "2D  Game");

    // Creating Samurai.
    Samurai samurai((Vector2) {400, 300});
    samurai.loadTextures();

    // Creating Goblin.
    Goblin goblin((Vector2) {400, 300});
    goblin.loadTextures();

    // Creating Werewolf.
    Werewolf werewolf((Vector2 {400, 300}));
    werewolf.loadTextures();

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

        werewolf.move();
        werewolf.applyVelocity();
        werewolf.updateAnimation();

        // Drawing.
        BeginDrawing();
        ClearBackground(GREEN);
        samurai.draw();
        goblin.draw();
        werewolf.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}