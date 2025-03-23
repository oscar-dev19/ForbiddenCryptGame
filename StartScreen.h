#ifndef START_SCREEN_H
#define START_SCREEN_H

#include "raylib.h"

class StartScreen {
public:
    StartScreen() {
        // Centering the buttons on the screen
        playButton = { 760, 400, 400, 80 };
        exitButton = { 760, 500, 400, 80 };
        startGame = false;
        exitGame = false;
    }

    void Update() {
        Vector2 mouse = GetMousePosition();

        // Play button interaction
        if (CheckCollisionPointRec(mouse, playButton)) {
            playButtonColor = (Color){ 255, 223, 100, 255 }; // Faint gold for hover
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                startGame = true;
            }
        } else {
            playButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone texture
        }

        // Exit button interaction
        if (CheckCollisionPointRec(mouse, exitButton)) {
            exitButtonColor = (Color){ 255, 0, 0, 255 }; // Blood red for hover
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                exitGame = true;
            }
        } else {
            exitButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone texture
        }
    }

    void Draw() {
        // Dark Souls-like background with misty shadows
        ClearBackground((Color){ 20, 20, 30, 255 });

        // Draw title centered on the screen
        const char* title = "THE FORSAKEN CRYPT";
        int titleWidth = MeasureText(title, 60); // Measure text width
        int titleX = (GetScreenWidth() - titleWidth) / 2; // Center the text
        DrawText(title, titleX, 150, 60, (Color){ 255, 255, 255, 255 });

        // Draw buttons with glowing effects
        DrawRectangleRec(playButton, playButtonColor);
        DrawRectangleLinesEx(playButton, 4, (Color){ 50, 50, 50, 255 }); // Faded border
        DrawRectangleRec(exitButton, exitButtonColor);
        DrawRectangleLinesEx(exitButton, 4, (Color){ 50, 50, 50, 255 }); // Faded border

        // Draw button text with pale contrast for better readability
        DrawText("PLAY", playButton.x + 150, playButton.y + 25, 32, (Color){ 240, 240, 240, 255 });
        DrawText("EXIT", exitButton.x + 150, exitButton.y + 25, 32, (Color){ 240, 240, 240, 255 });
    }

    bool ShouldStartGame() { return startGame; }
    bool ShouldExitGame() { return exitGame; }

private:
    Rectangle playButton;
    Rectangle exitButton;
    Color playButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone color
    Color exitButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone color
    bool startGame;
    bool exitGame;
};

#endif // START_SCREEN_H