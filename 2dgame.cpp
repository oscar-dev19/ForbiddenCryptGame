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
Sound demonChantSound = { 0 };
float masterVolume = 0.7f;

// Global background texture
Texture2D backgroundTexture = { 0 };
Camera2D camera = { 0 };

TmxMap* map = NULL;

// Key variables
Texture2D keyTexture = { 0 };
Vector2 keyPosition = { 0, 0 };
float keyRotation = 0.0f;
float keyFrameTime = 0.0f;
int keyCurrentFrame = 0;
const int KEY_FRAME_COUNT = 17;  // Number of frames in the sprite sheet
const float KEY_FRAME_DURATION = 0.1f;  // Duration for each frame in seconds
const int KEY_FRAME_WIDTH = 32;  // Width of each frame in pixels
const int KEY_FRAME_HEIGHT = 32; // Height of each frame in pixels
Rectangle keyCollisionRect = { 0 }; // Collision rectangle for the key
Sound keySound = { 0 }; // Sound to play when key is collected
bool keyCollected = false; // Flag to track if key has been collected

enum GameState {START_SCREEN, MAIN_GAME, EXIT};
bool isPaused = false;

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
    
    if (demonChantSound.stream.buffer) {
        UnloadSound(demonChantSound);
    }
    
    if (keySound.stream.buffer) {
        UnloadSound(keySound);
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
    map = LoadTMX("maps/Room1.tmx");
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
            std::cout << "Made Contact.\n";

            if (player.isJumping() || player.isFalling()) {
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
    const float floorLevel = 2223.0f; // Exact floor level matching the non-zero floor tiles in TMX
    const float floorHeight = 50.0f; // Height of the floor rectangle if needed
    
    // Update constant values in other files to match our floor level
    // These are compile-time constants, but we're making this note for clarity
    // GROUND_LEVEL and GROUND_LEVEL_WIZARD should be equal to floorLevel for consistency
    
    // Initialize audio device before loading music
    InitAudioDevice();

    // Load background music
    backgroundMusic = LoadMusicStream("music/Lady Maria of the Astral Clocktower.mp3");
    PlayMusicStream(backgroundMusic);  // Start playing the music
    SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
    
    // Load demon chant sound
    demonChantSound = LoadSound("sounds/misc/demon-chant-latin-14489.mp3");
    SetSoundVolume(demonChantSound, 0.6f * masterVolume);
    
    // Load key sound
    keySound = LoadSound("sounds/key/keysound.mp3");
    
    SetTargetFPS(60);
    
    // Initialize camera
    camera.target = (Vector2){ 100, 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 3.3f;  // Zoom in for better visibility

    // Load key texture
    keyTexture = LoadTexture("assets/gameObjects/key/key.png");
    if (keyTexture.id == 0) {
        printf("Failed to load key texture!\n");
        CloseWindow();
        return -1;
    }
    // Place the key further in the map to encourage exploration
    keyPosition = { 1200, floorLevel - keyTexture.height };

    // Initialize characters using stack allocation - all characters now use the same floorLevel
    Samurai samurai(400, floorLevel, floorLevel);
    
    // Initialize dash sound volume to match master volume
    samurai.setDashSoundVolume(0.8f * masterVolume);

    StartScreen startScreen;
    GameState gameState = START_SCREEN;

    loadLevel();

    // Game loop
    while (!WindowShouldClose()) {

        // Handle game state updates based on current game state
        switch(gameState) {
            case START_SCREEN: {
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

                if (IsKeyPressed(KEY_P)) {
                    isPaused = !isPaused;
                }

                // Update music stream
                UpdateMusicStream(backgroundMusic);
                
                // Toggle music with M key
                if (IsKeyPressed(KEY_M)) {
                    if (IsMusicStreamPlaying(backgroundMusic)) {
                        PauseMusicStream(backgroundMusic);
                    } else {
                        ResumeMusicStream(backgroundMusic);
                    }
                }

                // Volume control with + and - keys
                if (IsKeyPressed(KEY_EQUAL)) { // + key
                    masterVolume += 0.1f;
                    if (masterVolume > 1.0f) masterVolume = 1.0f;
                    SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
                    SetSoundVolume(demonChantSound, 0.6f * masterVolume);
                    samurai.setDashSoundVolume(0.8f * masterVolume);
                }

                if (IsKeyPressed(KEY_MINUS)) { // - key
                    masterVolume -= 0.1f;
                    if (masterVolume < 0.0f) masterVolume = 0.0f;
                    SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
                    SetSoundVolume(demonChantSound, 0.6f * masterVolume);
                    samurai.setDashSoundVolume(0.8f * masterVolume);
                }

                // Toggle collision box visibility with F1 key
                if (IsKeyPressed(KEY_F1)) {
                    showCollisionBoxes = !showCollisionBoxes;
                    printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
                }

                // Get frame time for updates
                float deltaTime = GetFrameTime();

                if (!isPaused) {
                    // Update samurai character
                    samurai.updateSamurai();
                }

                // Get samurai position for collision detection
                Vector2 samuraiPos = {0, 0};
                CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
                if (samuraiBody && samuraiBody->active) {
                    samuraiPos.x = samuraiBody->rect.x + samuraiBody->rect.width / 2;
                    samuraiPos.y = samuraiBody->rect.y + samuraiBody->rect.height / 2;
                }

                
                // Check for key collection
                if (samuraiBody && samuraiBody->active) {
                    if (CheckCollisionRecs(samuraiBody->rect, keyCollisionRect) && !keyCollected) {
                        // Play key collection sound
                        PlaySound(keySound);
                        keyCollected = true;
                    }
                }
                
                // Check for collisions between Samurai's attack and enemies
                CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
                
                // Check for enemy attacks hitting Samurai
                CollisionBox* samuraiHurtbox = samurai.getCollisionBox(HURTBOX);

                checkTileCollisions(map, samurai);

                // Update animations and collision rectangles
                keyFrameTime += deltaTime;
                if (keyFrameTime >= KEY_FRAME_DURATION) {
                    keyFrameTime = 0.0f;
                    keyCurrentFrame = (keyCurrentFrame + 1) % KEY_FRAME_COUNT;
                }

                // Update key collision rectangle
                keyCollisionRect = (Rectangle){
                    keyPosition.x + (KEY_FRAME_WIDTH),              // Center horizontally on the scaled key
                    keyPosition.y + (KEY_FRAME_HEIGHT),             // Center vertically on the scaled key
                    (float)KEY_FRAME_WIDTH,                         // Keep original collision size
                    (float)KEY_FRAME_HEIGHT
                };
                // Update camera to follow player, ensuring it stays within map boundaries
                Rectangle samuraiRect = samurai.getRect();
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

                // Begin drawing
                BeginDrawing();
                ClearBackground(BLACK);
                
                // 2D camera mode for proper drawing
                BeginMode2D(camera);

                renderLevel();

                // Draw Samurai.
                samurai.draw();

                if (!keyCollected) {
                    Rectangle keySource = {
                        (float)(keyCurrentFrame * KEY_FRAME_WIDTH), 0.0f, 
                        (float)KEY_FRAME_WIDTH, (float)KEY_FRAME_HEIGHT
                    };
                    DrawTexturePro(keyTexture, keySource, (Rectangle){ keyPosition.x, keyPosition.y, (float)KEY_FRAME_WIDTH, (float)KEY_FRAME_HEIGHT }, (Vector2){0, 0}, keyRotation, WHITE);
                } else {
                    CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
                    if (samuraiBody && samuraiBody->active) {
                        Rectangle keySource = { (float)(keyCurrentFrame * KEY_FRAME_WIDTH), 0.0f, (float)KEY_FRAME_WIDTH, (float)KEY_FRAME_HEIGHT };
                        DrawTexturePro(keyTexture, keySource, (Rectangle){ samuraiBody->rect.x + samuraiBody->rect.width/2 - KEY_FRAME_WIDTH/2, samuraiBody->rect.y - KEY_FRAME_HEIGHT - 10, (float)KEY_FRAME_WIDTH, (float)KEY_FRAME_HEIGHT }, (Vector2){0, 0}, keyRotation, WHITE);
                    }
                }

                // Draw collision boxes if enabled
                if (showCollisionBoxes) {
                    if (!keyCollected) {
                        DrawRectangleLines(keyCollisionRect.x, keyCollisionRect.y, keyCollisionRect.width, keyCollisionRect.height, PURPLE);
                    }
                    drawCollisionBox(*samurai.getCollisionBox(BODY));
                    drawCollisionBox(*samurai.getCollisionBox(ATTACK));
                    drawCollisionBox(*samurai.getCollisionBox(HURTBOX));
                }

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

    // Unload key texture
    UnloadTexture(keyTexture);

    // Unload sounds
    UnloadSound(keySound);

    // Unload map.
    UnloadTMX(map);

    // This code should never be reached because we call safeExit() when WindowShouldClose() is true
    return 0;
}