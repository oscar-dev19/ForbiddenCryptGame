#include "raylib.h"
#include "raymath.h"
#include "CollisionSystem.h"
#include "Samurai.h"
#include <iostream>
#include <vector>
#include <stdlib.h> // For exit()
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX
#include <cstdio>
#include "StartScreen.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Define RAYTMX_IMPLEMENTATION to include the implementation of the library
#define RAYTMX_IMPLEMENTATION
#include "raytmx.h"

// Define the global variable for collision box visibility
bool showCollisionBoxes = false;

// Global audio variables
Music backgroundMusic = { 0 };
Music menuMusic = { 0 };
float masterVolume = 0.7f;

// Global background texture
Texture2D backgroundTexture = { 0 };
Camera2D camera = { 0 };

TmxMap* map = NULL;

enum GameState {START_SCREEN, MAIN_GAME, EXIT};
bool isPaused = false;
bool isComplete = false;
bool goMain = false;

// Helper function to check collision between two collision boxes
bool checkCharacterCollision(const CollisionBox& box1, const CollisionBox& box2) {
    if (box1.active && box2.active) {
        return CheckCollisionRecs(box1.rect, box2.rect);
    }
    return false;
}

// Helper function to handle attack collision and damage
void handleAttackCollision(CollisionBox* attackBox, CollisionBox* hurtBox, int& health, int damage) {
    if (attackBox && hurtBox && attackBox->active && hurtBox->active && 
        checkCharacterCollision(*attackBox, *hurtBox)) {
        health -= damage;
        if (health < 0) health = 0;
        
        // Deactivate attack box to prevent multiple hits from the same attack
        attackBox->active = false;
    }
}

// Custom exit function that bypasses normal cleanup
void safeExit() {
    // Unload audio resources
    if (backgroundMusic.ctxData != NULL) {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
    }
    
    // Unload the background texture
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
    }
    
    // Clean up Raylib
    CloseAudioDevice();
    CloseWindow();
    
    // Exit directly without going through normal cleanup
    _Exit(0); // Use _Exit instead of exit to bypass any atexit handlers
}

// Function to draw collision boxes for debugging
void drawCollisionBox(const CollisionBox& box) {
    if (!box.active) return;
    
    Color color;
    switch (box.type) {
        case BODY:
            color = BLUE;
            break;
        case ATTACK:
            color = RED;
            break;
        case HURTBOX:
            color = GREEN;
            break;
        default:
            color = YELLOW;
            break;
    }
    
    // Draw the collision box as a rectangle outline
    DrawRectangleLines(box.rect.x, box.rect.y, box.rect.width, box.rect.height, color);
    
    // Draw a small label to identify the box type
    const char* label = "";
    switch (box.type) {
        case BODY: label = "BODY"; break;
        case ATTACK: label = "ATTACK"; break;
        case HURTBOX: label = "HURT"; break;
        default: label = "UNKNOWN"; break;
    }
    
    DrawText(label, box.rect.x, box.rect.y - 15, 10, color);
}

// Function to check if a file exists
bool fileExists(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void loadLevel() {
    map = LoadTMX("maps/LevelDesign.tmx");
    if (!map) {
        printf("Failed to Load TMX File.\n");
        exit (1);
    } else {
        printf("Loaded TMX File.");
    }

    // Loop through tilesets (assuming map->tilesets is a pointer to an array of TmxTileset)
    for (int i = 0; i < map->tilesetsLength; i++) {
        TmxTileset* tileset = &map->tilesets[i];  // Accessing tileset by pointer

        // Check if the image source is valid (not empty)
        if (tileset->image.source[0] != '\0') {  // Using image.source to check validity
            Texture2D tilesetTexture = LoadTexture("maps/16 x16 Purple Dungeon Sprite Sheet.png");
            if (tilesetTexture.id == 0) {
                std::cout << "Error loading tileset image" << std::endl;
            }
        }
    }
}

void renderLevel() {
    if (map) {
        DrawTMX(map, &camera, 0, 0, WHITE);
    }
}

void checkTileCollisions(TmxMap* map, Samurai& player) {
    for (unsigned int i = 0; i < map->layersLength; i++) {
      TraceLog(LOG_DEBUG, "current layer is %d: %s", i, map->layers[i].name);
      if (strcmp(map->layers[i].name, "Object Layer 1") == 0 && map->layers[i].type == LAYER_TYPE_OBJECT_GROUP) 
      {
          TmxObject col;
          if (CheckCollisionTMXObjectGroupRec(map->layers[i].exact.objectGroup, player.getRect(), &col))
          {
            TraceLog(LOG_DEBUG, "We've made contact!");
            
            Vector2 newVel = player.getVelocity();
            newVel.y = 0;
            player.setVelocity(newVel);

            Rectangle newRect = player.getRect();
            newRect.y = (col.aabb.y - newRect.height);
            player.setRect(newRect);

            if (player.isJumping()) {
                player.land();
            }
          }
      }
    }
    return;
} 

int main() {
    // Print current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
    
    // Set up error handling
    SetTraceLogLevel(LOG_WARNING);
    
    // Initialize window
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "2D Game");

    // Define floor level to match where the non-zero tiles (floor tiles) are in Room1.tmx
    // This value is used for all characters to ensure consistent vertical positioning
    const float floorLevel = 5000.0f; // Exact floor level matching the non-zero floor tiles in TMX
    const float floorHeight = 50.0f; // Height of the floor rectangle if needed
    
    // Loading the Background.
    Texture2D background = LoadTexture("maps/Dungeon_brick_wall_purple.png.png");

    // Background Scale Factors.
    float scalebgx = (float)screenWidth / (float)background.width;
    float scalebgy = (float)screenHeight / (float)background.height;
    float scalebg = (scalebgx < scalebgy) ? scalebgx : scalebgy;
    scalebg /= 4.5f;

    // Positions for Background Positions.
    float bgposX = ((screenWidth - background.width * scalebg) / 2) - 600;
    float bgposY = ((screenHeight - background.height * scalebg) / 2) - 210;
    
    // Horizontal and Vertical Sliders for Background.
    int scaledW = background.width * scalebg;
    int scaledH = background.height * scalebg;
    int tilesX = (screenWidth / scaledW) + 40;
    int tilesY = (screenHeight / scaledH) + 10;
    
    // Initialize audio device before loading music
    InitAudioDevice();

    // Load Music.
    backgroundMusic = LoadMusicStream("music/Lady Maria of the Astral Clocktower.mp3");
    menuMusic = LoadMusicStream("music/Soul Of Cinder.mp3");
    
    SetTargetFPS(60);
    
    // Initialize camera
    camera.target = (Vector2){ 100, 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 3.3f;  // Zoom in for better visibility.

    // Initialize characters using stack allocation - all characters now use the same floorLevel
    Samurai samurai(400, 2223, floorLevel);
    
    // Initialize dash sound volume to match master volume
    samurai.setDashSoundVolume(0.8f * masterVolume);

    StartScreen startScreen;
    GameState gameState = START_SCREEN;

    loadLevel();

    // Start playing menu music initially
    PlayMusicStream(menuMusic);
    SetMusicVolume(menuMusic, 0.5f * masterVolume);
    bool isPlayingMenuMusic = true;

    // Game loop
    while (!WindowShouldClose()) {
        // Update currently playing music
        UpdateMusicStream(isPlayingMenuMusic ? menuMusic : backgroundMusic);

        // Handle game state updates based on current game state
        switch(gameState) {
            case START_SCREEN: {
                goMain = false;
                if (!isPlayingMenuMusic) {
                    PlayMusicStream(menuMusic);
                    isPlayingMenuMusic = true;
                }

                startScreen.Update();

                if (startScreen.ShouldStartGame()) {
                    gameState = MAIN_GAME;  // Transition to main game
                }
                if (startScreen.ShouldExitGame()) {
                    gameState = EXIT;  // Exit the game
                }

                BeginDrawing();
                startScreen.Draw();  // Draw the start screen
                EndDrawing();
                break;
            }

            case MAIN_GAME: {
                if (isPlayingMenuMusic) {
                    PlayMusicStream(backgroundMusic);
                    isPlayingMenuMusic = false;
                }

                if (IsKeyPressed(KEY_P)) {
                    isPaused = !isPaused;
                }
                
                // Toggle music with M key
                if (IsKeyPressed(KEY_M)) {
                    if (IsMusicStreamPlaying(backgroundMusic)) {
                        PauseMusicStream(backgroundMusic);
                    } else {
                        ResumeMusicStream(backgroundMusic);
                    }
                }

                // Toggle collision box visibility with F1 key
                if (IsKeyPressed(KEY_F1)) {
                    showCollisionBoxes = !showCollisionBoxes;
                    printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
                }

                // Get frame time for updates
                float deltaTime = GetFrameTime();

                if (!isPaused && !isComplete) {
                    // Update samurai character
                    samurai.updateSamurai();
                }

                samurai.deathBarrier();
                std::cout << "X: " << samurai.getRect().x << std::endl;
                std::cout << "Y: " <<samurai.getRect().y << std::endl;

                // Get samurai position for collision detection
                Vector2 samuraiPos = {0, 0};
                CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
                if (samuraiBody && samuraiBody->active) {
                    samuraiPos.x = samuraiBody->rect.x + samuraiBody->rect.width / 2;
                    samuraiPos.y = samuraiBody->rect.y + samuraiBody->rect.height / 2;
                }
                
                // Check for collisions between Samurai's attack and enemies
                CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
                
                // Check for enemy attacks hitting Samurai
                CollisionBox* samuraiHurtbox = samurai.getCollisionBox(HURTBOX);

                checkTileCollisions(map, samurai);

                // Update camera to follow player, ensuring it stays within map boundaries
                Rectangle samuraiRect = samurai.getRect();
                
                if (!samurai.checkDeath()) {
                    camera.target = (Vector2){ samuraiPos.x, samuraiPos.y };
                    // Add some camera boundary checks to avoid the camera going out of bounds:
                    float halfScreenWidth = screenWidth / (2.0f * camera.zoom);
                    float halfScreenHeight = screenHeight / (2.0f * camera.zoom);
    
                    // Clamp X and Y positions with easing towards boundaries
                    camera.target.x = Lerp(camera.target.x, Clamp(camera.target.x, halfScreenWidth, map->width * map->tileWidth - halfScreenWidth), 0.1f);
                    camera.target.y = Lerp(camera.target.y, Clamp(camera.target.y, halfScreenHeight, map->height * map->tileHeight - halfScreenHeight), 0.1f);
    
                    // Ensure camera doesn't go out of bounds
                    if (camera.target.x < halfScreenWidth) camera.target.x = halfScreenWidth + 100;
                    if (camera.target.y < halfScreenHeight) camera.target.y = halfScreenHeight;
    
                    // Camera zoom controls
                    camera.zoom += ((float)GetMouseWheelMove() * 0.1f);
                    if (camera.target.x < halfScreenWidth) camera.target.x = halfScreenWidth;
                    if (camera.target.x > map->width * 16 - halfScreenWidth) camera.target.x = map->width * 16 - halfScreenWidth;
                    if (camera.target.y < halfScreenHeight) camera.target.y = halfScreenHeight;
                    if (camera.target.y > map->height * 16 - halfScreenHeight) camera.target.y = map->height * 16 - halfScreenHeight;
                } else {
                    // Stop moving the camera when the player is dead
                    camera.target = camera.target; // Keeps the camera locked in place
                }

                if(samurai.getRect().x >= 10980) {
                    isComplete = true;
                }

                // Begin drawing
                BeginDrawing();
                ClearBackground(BLACK);
                
                // 2D camera mode for proper drawing
                BeginMode2D(camera);

                // Draw Background.
                for (int x = 0; x < tilesX; x++) {
                    for (int y = 0; y < tilesY; y++) {
                        float posX = x * scaledW;
                        float posY = y * scaledH;

                        DrawTextureEx(background, Vector2{posX + bgposX, posY + bgposY}, 0.0f, scalebg, GRAY);
                    }
                }
                
                renderLevel();

                // Draw Samurai.
                samurai.draw();

                // End camera mode and finalize drawing
                EndMode2D();

                // UI controls instructions
                int instructionsY = screenHeight - 1050;
                int lineHeight = 25;

                DrawText("GAME CONTROLS:", 10, instructionsY, 20, WHITE);
                DrawText("W or Up: Jump ", 10, instructionsY + lineHeight, 20, WHITE);
                DrawText("A/D or Left/Right: Move", 10, instructionsY + lineHeight*2, 20, WHITE);
                DrawText("Space: Attack", 10, instructionsY + lineHeight*3, 20, WHITE);
                DrawText("Double-tap A/D: Dash", 10, instructionsY + lineHeight*4, 20, WHITE);
                DrawText("F1: Toggle collision boxes", 10, instructionsY + lineHeight*5, 20, WHITE);
                DrawText("M: Toggle music", 10, instructionsY + lineHeight*6, 20, WHITE);
                DrawText("Mouse Wheel: Zoom", 10, instructionsY + lineHeight*7, 20, WHITE);
                DrawText("P: Pause", 10, instructionsY + lineHeight*8, 20, WHITE);

                if (isPaused) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
                    DrawText("PAUSED", GetScreenWidth()/2 - 50, GetScreenHeight()/2 - 10, 30, WHITE);
                    DrawText("Press 'P' to resume", GetScreenWidth()/2 - 100, GetScreenHeight()/2 + 30, 20, WHITE);

                    // Define the exit button area
                    Rectangle exitButton = { static_cast<float>(GetScreenWidth()) / 2 - 75, 
                        static_cast<float>(GetScreenHeight()) / 2 + 60, 
                        150.0f, 40.0f };


                    // Draw the exit button
                    DrawRectangleRec(exitButton, DARKGRAY);
                    DrawText("Exit", GetScreenWidth()/2 - 20, GetScreenHeight()/2 + 70, 20, WHITE);

                    // Check if the exit button is clicked
                    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        safeExit();
                    }

                    samurai.pauseSounds();
                } else if(isComplete) {
                    // Draw completion screen with improved UI layout
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); // Darker background for emphasis

                    // Centered text
                    const int centerX = GetScreenWidth() / 2;
                    const int centerY = GetScreenHeight() / 2;

                    DrawText("GAME COMPLETED!", centerX - MeasureText("GAME COMPLETED!", 40) / 2, centerY - 100, 40, GOLD);
                    DrawText("Congratulations!", centerX - MeasureText("Congratulations!", 30) / 2, centerY - 50, 30, WHITE);
                    DrawText("Press 'E' to exit", centerX - MeasureText("Press 'E' to exit", 20) / 2, centerY + 20, 20, LIGHTGRAY);

                    // Define the exit button
                    Rectangle exitButton = { static_cast<float>(centerX - 100), 
                                            static_cast<float>(centerY + 60), 
                                            200.0f, 50.0f };

                    // Draw the exit button with a hover effect
                    Color exitButtonColor = CheckCollisionPointRec(GetMousePosition(), exitButton) ? LIGHTGRAY : DARKGRAY;
                    DrawRectangleRec(exitButton, exitButtonColor);
                    DrawText("Exit", centerX - MeasureText("Exit", 25) / 2, centerY + 75, 25, WHITE);

                    // Check if the exit button is clicked
                    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (startScreen.ShouldStartGame()) {
                            safeExit();
                        }
                    }

                    // Handle key press for exiting
                    if (IsKeyPressed(KEY_E)) {
                        if (startScreen.ShouldStartGame()) {
                            safeExit();
                        }
                    }
                } else {
                    samurai.resumeSound();
                }

                EndDrawing();

                break;
            }

            case EXIT: {
                // Handle exit (save state, cleanup, etc.)
                safeExit();
                break;
            }
        }
    }

    // This code should never be reached because we call safeExit() when WindowShouldClose() is true
    return 0;
}