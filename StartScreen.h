#ifndef START_SCREEN_H
#define START_SCREEN_H

#include "raylib.h"

/**
 * @class StartScreen
 * @brief Represents the start screen of the game with play and exit buttons.
 */
class StartScreen {
public:
    /**
     * @brief Constructor that initializes the start screen with button positions, sizes, and game states.
     */
    StartScreen() {
        // Centering the buttons on the screen
        playButton = { 760, 400, 400, 80 }; // Play button position and size
        exitButton = { 760, 500, 400, 80 }; // Exit button position and size
        startGame = false; // Game not started initially
        exitGame = false;  // Game not exited initially
    }

    /**
     * @brief Updates the button interactions based on mouse input.
     * Handles button hover and click events for play and exit buttons.
     */
    void Update() {
        Vector2 mouse = GetMousePosition(); // Get mouse position

        // Play button interaction: Changes color on hover and starts the game on click
        if (CheckCollisionPointRec(mouse, playButton)) {
            playButtonColor = (Color){ 255, 223, 100, 255 }; // Faint gold for hover
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                startGame = true; // Start game if play button is clicked
            }
        } else {
            playButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone texture
        }

        // Exit button interaction: Changes color on hover and exits the game on click
        if (CheckCollisionPointRec(mouse, exitButton)) {
            exitButtonColor = (Color){ 255, 0, 0, 255 }; // Blood red for hover
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                exitGame = true; // Exit game if exit button is clicked
            }
        } else {
            exitButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone texture
        }
    }

    /**
     * @brief Draws the start screen, including the background, title, and buttons.
     * Renders the title "THE FORSAKEN CRYPT" and the interactive play and exit buttons.
     */
    void Draw() {
        // Dark Souls-like background with misty shadows
        ClearBackground((Color){ 20, 20, 30, 255 });

        // Draw title centered on the screen
        const char* title = "THE FORSAKEN CRYPT";
        int titleWidth = MeasureText(title, 60); // Measure text width
        int titleX = (GetScreenWidth() - titleWidth) / 2; // Center the text
        DrawText(title, titleX, 150, 60, (Color){ 255, 255, 255, 255 }); // Draw the title

        // Draw play button with glowing effects
        DrawRectangleRec(playButton, playButtonColor);
        DrawRectangleLinesEx(playButton, 4, (Color){ 50, 50, 50, 255 }); // Faded border for play button
        // Draw exit button with glowing effects
        DrawRectangleRec(exitButton, exitButtonColor);
        DrawRectangleLinesEx(exitButton, 4, (Color){ 50, 50, 50, 255 }); // Faded border for exit button

        // Draw button text with pale contrast for better readability
        DrawText("PLAY", playButton.x + 150, playButton.y + 25, 32, (Color){ 240, 240, 240, 255 });
        DrawText("EXIT", exitButton.x + 150, exitButton.y + 25, 32, (Color){ 240, 240, 240, 255 });
    }

    /**
     * @brief Checks if the game should start.
     * @return True if the start button was clicked, false otherwise.
     */
    bool ShouldStartGame() { return startGame; }

    /**
     * @brief Checks if the game should exit.
     * @return True if the exit button was clicked, false otherwise.
     */
    bool ShouldExitGame() { return exitGame; }

private:
    /** The play button's rectangle area and its color when hovered or inactive. */
    Rectangle playButton;
    /** The exit button's rectangle area and its color when hovered or inactive. */
    Rectangle exitButton;
    /** The current color of the play button (changes on hover). */
    Color playButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone color
    /** The current color of the exit button (changes on hover). */
    Color exitButtonColor = (Color){ 64, 64, 64, 255 }; // Dark stone color
    /** States for whether the game should start or exit. */
    bool startGame;
    bool exitGame;
};

#endif // START_SCREEN_H