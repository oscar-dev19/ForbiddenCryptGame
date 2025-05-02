#ifndef START_SCREEN_H
#define START_SCREEN_H

#include "raylib.h"

/**
 * @class StartScreen
 * @brief Represents the start screen of the game with interactive buttons.
 *
 * This class manages the start screen of the game, displaying a title along with "Play" 
 * and "Exit" buttons. It provides user interaction by detecting mouse hover and clicks 
 * on these buttons. The buttons visually change on hover, creating a Dark Souls-inspired 
 * atmospheric design.
 */
class StartScreen {
public:
    /**
     * @brief Constructs the StartScreen object.
     *
     * Initializes the button positions, colors, and game state variables.
     */
    StartScreen() {
        // Centering the buttons on the screen
        playButton = { 760, 400, 400, 80 };
        exitButton = { 760, 500, 400, 80 };
        startGame = false;
        exitGame = false;
    }

    /**
     * @brief Updates the button states based on user interaction.
     *
     * Detects mouse hover and clicks, changing button colors accordingly.
     * Sets game state variables when buttons are clicked.
     */
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

    /**
     * @brief Renders the start screen elements.
     *
     * Draws the game title, buttons, and button labels with a visually appealing theme.
     */
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

    /**
     * @brief Checks if the player has chosen to start the game.
     * @return True if the "Play" button was clicked, otherwise false.
     */
    bool ShouldStartGame() { return startGame; }

    /**
     * @brief Checks if the player has chosen to exit the game.
     * @return True if the "Exit" button was clicked, otherwise false.
     */
    bool ShouldExitGame() { return exitGame; }

private:
    Rectangle playButton;    ///< Rectangle defining the "Play" button's position and size.
    Rectangle exitButton;    ///< Rectangle defining the "Exit" button's position and size.
    Color playButtonColor = (Color){ 64, 64, 64, 255 }; ///< Default color for the "Play" button.
    Color exitButtonColor = (Color){ 64, 64, 64, 255 }; ///< Default color for the "Exit" button.
    bool startGame;  ///< Flag indicating whether the game should start.
    bool exitGame;   ///< Flag indicating whether the game should exit.
};

#endif // START_SCREEN_H