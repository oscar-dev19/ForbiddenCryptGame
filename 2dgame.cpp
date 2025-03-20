#include "raylib.h"
#include "raymath.h"
#include "CollisionSystem.h"
#include "Samurai.h"
#include "Goblin.h"
#include "Werewolf.h"
#include "Wizard.h"
#include "Demon.h"
#include <iostream>
#include <vector>
#include <stdlib.h> // For exit()
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX
#include <cstdio>

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

// Define a scale factor for the demon (visual only)
float demonScaleFactor = 1.3f;

// Global background texture
Texture2D backgroundTexture = { 0 };
Camera2D camera = { 0 };

// Global TMX map variable
TmxMap* tmxMap = NULL;

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
    
    // Unload the TMX map
    if (tmxMap != NULL) {
        UnloadTMX(tmxMap);
        tmxMap = NULL;
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

// Custom draw function for the demon to apply scaling
void drawScaledDemon(const Demon& demon) {
    // Get the current animation frame
    Rectangle frameRec = demon.getAnimationFrame();
    
    // Calculate the scaled dimensions
    float scaledWidth = demon.rect.width * demonScaleFactor;
    float scaledHeight = demon.rect.height * demonScaleFactor;
    
    // Calculate position adjustment to keep the demon's feet on the ground
    float posYAdjustment = (scaledHeight - demon.rect.height);
    
    // Draw the demon with scaling
    DrawTexturePro(
        demon.sprites[0],                                // Texture
        frameRec,                                        // Source rectangle
        (Rectangle){
            demon.rect.x,                                // X position (unchanged)
            demon.rect.y - posYAdjustment,               // Y position (adjusted for height)
            scaledWidth,                                 // Scaled width
            scaledHeight                                 // Scaled height
        },
        (Vector2){0, 0},                                 // Origin
        0.0f,                                            // Rotation
        WHITE                                            // Tint
    );
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
    const int screenWidth = 800;
    const int screenHeight = 550;
    InitWindow(screenWidth, screenHeight, "2D Game");

    // Define floor level to match where the non-zero tiles (floor tiles) are in Room1.tmx
    // This value is used for all characters to ensure consistent vertical positioning
    const float floorLevel = 380.0f; // Exact floor level matching the non-zero floor tiles in TMX
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

    // Load background texture
    // Try to load from maps directory first
    backgroundTexture = LoadTexture("maps/64 x 64 Blue Dungeon Sprite Sheet.png");
    if (backgroundTexture.id == 0) {
        // If failed, try assets directory
        backgroundTexture = LoadTexture("assets/background/dungeon_background.png");
        if (backgroundTexture.id == 0) {
            printf("Failed to load background texture! Creating a default one.\n");
            // Create a default background texture with a simple pattern
            Image checkerImage = GenImageChecked(800, 600, 64, 64, DARKGRAY, GRAY);
            backgroundTexture = LoadTextureFromImage(checkerImage);
            UnloadImage(checkerImage);
        }
    }
    
    // Initialize camera
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.5f;  // Zoom in for better visibility

    // Keep the array of map paths for fallback loading
    const char* mapPaths[] = {
        "maps/Room1.tmx",
        "maps/Room3and4.tmx",
        "maps/Dungeon.tmx"
    };
    int numMapPaths = sizeof(mapPaths) / sizeof(mapPaths[0]);
    
    // Try to load any available map
    tmxMap = NULL;
    bool mapLoaded = false;
    for (int i = 0; i < numMapPaths && !mapLoaded; i++) {
        tmxMap = LoadTMX(mapPaths[i]);
        if (tmxMap != NULL) {
            mapLoaded = true;
            printf("Successfully loaded TMX map: %s\n", mapPaths[i]);
        } else {
            printf("Failed to load TMX map: %s\n", mapPaths[i]);
        }
    }
    
    if (!mapLoaded) {
        printf("Failed to load any TMX map. Exiting...\n");
        exit(1);
    }
    
    // Keep the tileset checking code but remove debug prints
    const char* requiredTilesets[] = {
        "maps/16 x16 Purple Dungeon Sprite Sheet copy.png"
    };
    int numRequiredTilesets = sizeof(requiredTilesets) / sizeof(requiredTilesets[0]);
    
    // Ensure needed tileset files exist
    for (int i = 0; i < numRequiredTilesets; i++) {
        if (!fileExists(requiredTilesets[i])) {
            // Keep the TSX file creation code
            FILE* file = fopen(requiredTilesets[i], "w");
            if (file) {
                fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
                fprintf(file, "<tileset version=\"1.10\" tiledversion=\"1.10.2\" name=\"16 x16 Purple Dungeon Sprite Sheet\" tilewidth=\"16\" tileh\n");
                fprintf(file, "eight=\"16\" tilecount=\"60\" columns=\"6\">\n");
                fprintf(file, " <image source=\"16 x16 Purple Dungeon Sprite Sheet.png\" width=\"96\" height=\"160\"/>\n");
                fprintf(file, "</tileset>\n");
                fclose(file);
            } else {
                printf("Failed to create tileset file: %s\n", requiredTilesets[i]);
            }
        }
    }
    
    // Calculate the map dimensions in pixels based on the loaded TMX map
    const int mapTileSize = 16;
    int mapWidth = 128;  // Default map width in tiles
    int mapHeight = 32;  // Default map height in tiles
    
    // Try to get actual dimensions from the loaded TMX map
    if (tmxMap != NULL && tmxMap->fileName != NULL) {
        // Get map dimensions from the loaded TMX file
        mapWidth = tmxMap->width;
        mapHeight = tmxMap->height;
    } else {
        printf("Using default map dimensions: %d x %d tiles\n", mapWidth, mapHeight);
    }
    
    const float mapWidthPixels = mapWidth * mapTileSize;
    const float mapHeightPixels = mapHeight * mapTileSize;
    
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
    Samurai samurai(100, floorLevel, floorLevel);
    
    // Initialize dash sound volume to match master volume
    samurai.setDashSoundVolume(0.8f * masterVolume);
    
    // Position enemies across the map to demonstrate full map usage
    // Each enemy is placed at different distances from the start but at the same floor level
    // This ensures all enemies are aligned on the same horizontal plane
    
    // For each enemy, explicitly set Y position = floorLevel - height
    // This ensures the bottom of all enemies aligns with floorLevel
    float goblinHeight = 64.0f * SPRITE_SCALE;
    float werewolfHeight = 64.0f * SPRITE_SCALE;
    float wizardHeight = 64.0f * SPRITE_SCALE;
    float demonHeight = 64.0f * SPRITE_SCALE;
    
    Goblin goblin((Vector2){500, floorLevel - goblinHeight});
    goblin.loadTextures();
    
    Werewolf werewolf(1000, floorLevel - werewolfHeight, floorLevel);  // Further away
    werewolf.loadTextures();
    
    Wizard wizard((Vector2){1500, floorLevel - wizardHeight});  // Even further
    wizard.loadTextures();
    
    // Create the demon at the same floor level at the far end of the map
    Demon demon((Vector2){1800, floorLevel - demonHeight});
    
    // Print initialization positions for debugging
    printf("Floor level set to: %.1f\n", floorLevel);
    printf("All characters initialized at floor level: %.1f\n", floorLevel);
    printf("Samurai Y: %.1f, Goblin Y: %.1f, Werewolf Y: %.1f, Wizard Y: %.1f, Demon Y: %.1f\n", 
           samurai.getRect().y, goblin.rect.y, werewolf.rect.y, wizard.rect.y, demon.rect.y);

    // Initialize health values for enemies
    int goblinHealth = 50;
    int werewolfHealth = 75;
    int wizardHealth = 60;
    int demonHealth = 150; // Increased demon health to match its larger appearance

    // Game loop
    while (!WindowShouldClose()) {
        // First frame debug: print all character Y positions 
        static bool firstFrame = true;
        if (firstFrame) {
            printf("Initial positions:\n");
            printf("Floor level: %.1f\n", floorLevel);
            printf("Samurai Y: %.1f (bottom: %.1f)\n", 
                   samurai.getRect().y, samurai.getRect().y + samurai.getRect().height);
            printf("Goblin Y: %.1f (bottom: %.1f)\n", 
                   goblin.rect.y, goblin.rect.y + goblin.rect.height);
            printf("Werewolf Y: %.1f (bottom: %.1f)\n", 
                   werewolf.rect.y, werewolf.rect.y + werewolf.rect.height);
            printf("Wizard Y: %.1f (bottom: %.1f)\n", 
                   wizard.rect.y, wizard.rect.y + wizard.rect.height);
            printf("Demon Y: %.1f (bottom: %.1f)\n", 
                   demon.rect.y, demon.rect.y + demon.rect.height);
            firstFrame = false;
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
            // Apply master volume to dash sound
            samurai.setDashSoundVolume(0.8f * masterVolume);
        }
        
        if (IsKeyPressed(KEY_MINUS)) { // - key
            masterVolume -= 0.1f;
            if (masterVolume < 0.0f) masterVolume = 0.0f;
            SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
            SetSoundVolume(demonChantSound, 0.6f * masterVolume);
            // Apply master volume to dash sound
            samurai.setDashSoundVolume(0.8f * masterVolume);
        }
        
        // Set double jump height to a fixed value of 45
        samurai.setDoubleJumpHeight(45.0f);
        
        // Toggle collision box visibility with F1 key
        if (IsKeyPressed(KEY_F1)) {
            showCollisionBoxes = !showCollisionBoxes;
            printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
        }
        
        // Get frame time
        float deltaTime = GetFrameTime();
        
        // Update characters
        samurai.updateSamurai();

        // Get samurai position for enemies to target
        Vector2 samuraiPos = {0, 0};
        CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
        if (samuraiBody && samuraiBody->active) {
            samuraiPos.x = samuraiBody->rect.x + samuraiBody->rect.width / 2;
            samuraiPos.y = samuraiBody->rect.y + samuraiBody->rect.height / 2;
        }

        // Update enemies with the player's position as target
        goblin.updateWithTarget(samuraiPos);
        werewolf.updateWithTarget(samuraiPos);
        wizard.update(samuraiPos);
        demon.update(deltaTime, samuraiPos);

        // Check collision between samurai and key
        if (samuraiBody && samuraiBody->active) {
            if (CheckCollisionRecs(samuraiBody->rect, keyCollisionRect) && !keyCollected) {
                // Play key collection sound
                PlaySound(keySound);
                keyCollected = true;
                
                // Remove the large center "Key Collected!" text - don't display any messages here
            }
        }

        // Check for collisions between Samurai's attack and enemies
        CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
        
        // Check Samurai attack vs Goblin
        if (!goblin.isDead && samuraiAttack && samuraiAttack->active) {
            CollisionBox* goblinHurtbox = goblin.getCollisionBox(HURTBOX);
            if (goblinHurtbox && checkCharacterCollision(*samuraiAttack, *goblinHurtbox)) {
                goblinHealth -= 10;
                if (goblinHealth <= 0) {
                    goblin.takeDamage(10); // This will set the goblin to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Werewolf
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* werewolfHurtbox = werewolf.getCollisionBox(HURTBOX);
            if (werewolfHurtbox && checkCharacterCollision(*samuraiAttack, *werewolfHurtbox)) {
                werewolfHealth -= 10;
                if (werewolfHealth <= 0) {
                    werewolf.takeDamage(10); // This will set the werewolf to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Wizard
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* wizardHurtbox = wizard.getCollisionBox(HURTBOX);
            if (wizardHurtbox && checkCharacterCollision(*samuraiAttack, *wizardHurtbox)) {
                wizardHealth -= 10;
                if (wizardHealth <= 0) {
                    wizard.takeDamage(10); // This will set the wizard to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Demon
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* demonHurtbox = demon.getCollisionBox(HURTBOX);
            if (demonHurtbox && checkCharacterCollision(*samuraiAttack, *demonHurtbox)) {
                demonHealth -= 10;
                if (demonHealth <= 0) {
                    demon.takeDamage(10); // This will set the demon to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check for enemy attacks hitting Samurai
        CollisionBox* samuraiHurtbox = samurai.getCollisionBox(HURTBOX);
        
        // Check Goblin attack vs Samurai
        if (!goblin.isDead && samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* goblinAttack = goblin.getCollisionBox(ATTACK);
            if (goblinAttack && goblinAttack->active && checkCharacterCollision(*goblinAttack, *samuraiHurtbox)) {
                samurai.takeDamage(5);
                goblinAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Werewolf attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* werewolfAttack = werewolf.getCollisionBox(ATTACK);
            if (werewolfAttack && werewolfAttack->active && checkCharacterCollision(*werewolfAttack, *samuraiHurtbox)) {
                samurai.takeDamage(8);
                werewolfAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Wizard attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* wizardAttack = wizard.getCollisionBox(ATTACK);
            if (wizardAttack && wizardAttack->active && checkCharacterCollision(*wizardAttack, *samuraiHurtbox)) {
                samurai.takeDamage(6);
                wizardAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Demon attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* demonAttack = demon.getCollisionBox(ATTACK);
            if (demonAttack && demonAttack->active && checkCharacterCollision(*demonAttack, *samuraiHurtbox)) {
                samurai.takeDamage(15); // Increased damage from the larger demon
                demonAttack->active = false; // Prevent multiple hits
            }
        }

        // Update key animation
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
        
        // Update TMX animations
        if (tmxMap != NULL && tmxMap->fileName != NULL) {
            AnimateTMX(tmxMap);
        }
        
        // Update camera to follow player
        Rectangle samuraiRect = samurai.getRect();
        camera.target = (Vector2){ samuraiRect.x, samuraiRect.y };
        
        // Ensure camera doesn't go beyond map boundaries
        float halfScreenWidth = screenWidth / (2.0f * camera.zoom);
        float halfScreenHeight = screenHeight / (2.0f * camera.zoom);
        
        // Calculate camera bounds, allowing movement throughout the entire map
        if (camera.target.x < halfScreenWidth) camera.target.x = halfScreenWidth;
        if (camera.target.x > mapWidthPixels - halfScreenWidth) camera.target.x = mapWidthPixels - halfScreenWidth;
        if (camera.target.y < halfScreenHeight) camera.target.y = halfScreenHeight;
        if (camera.target.y > mapHeightPixels - halfScreenHeight) camera.target.y = mapHeightPixels - halfScreenHeight;
        
        // Camera zoom controls
        camera.zoom += ((float)GetMouseWheelMove() * 0.1f);
        if (camera.zoom < 0.5f) camera.zoom = 0.5f;
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Begin 2D camera mode
        BeginMode2D(camera);
        
        // Draw the TMX map if loaded, otherwise fall back to the tiled background
        if (tmxMap != NULL && tmxMap->fileName != NULL) {
            // Remove all debug prints about drawing TMX map
            
            // Draw the TMX map
            DrawTMX(tmxMap, &camera, 0, 0, WHITE);
        } else {
            // Fall back to the original tiled background
            // Draw the background texture
            // Tiled approach to make it fill the viewable area
            float tileWidth = backgroundTexture.width;
            float tileHeight = backgroundTexture.height;
            
            // Calculate how many tiles needed based on camera zoom and window size
            int tilesX = (int)((screenWidth / camera.zoom) / tileWidth) + 2;  // +2 for safety
            int tilesY = (int)((screenHeight / camera.zoom) / tileHeight) + 2;
            
            // Calculate starting positions based on camera target to create a parallax effect
            float startX = (int)(camera.target.x / 2 - (tilesX * tileWidth) / 2);
            float startY = (int)(camera.target.y / 2 - (tilesY * tileHeight) / 2);
            
            // Draw the tiled background
            for (int y = 0; y < tilesY; y++) {
                for (int x = 0; x < tilesX; x++) {
                    DrawTexture(
                        backgroundTexture, 
                        startX + x * tileWidth, 
                        startY + y * tileHeight, 
                        WHITE
                    );
                }
            }
        }
        
        // Draw background floor (only if needed)
        DrawRectangle(0, floorLevel, mapWidthPixels, floorHeight, DARKGRAY);

        // Draw characters - moved inside camera mode
        samurai.draw();
        
        // Only draw enemies if they're not dead
        if (goblinHealth > 0) {
            goblin.draw();
            // Draw health bar for goblin
            DrawRectangle(goblin.rect.x, goblin.rect.y - 10, goblin.rect.width, 5, RED);
            DrawRectangle(goblin.rect.x, goblin.rect.y - 10, goblin.rect.width * (goblinHealth / 50.0f), 5, GREEN);
        }
        
        if (werewolfHealth > 0) {
            werewolf.draw();
            // Draw health bar for werewolf
            DrawRectangle(werewolf.rect.x, werewolf.rect.y - 10, werewolf.rect.width, 5, RED);
            DrawRectangle(werewolf.rect.x, werewolf.rect.y - 10, werewolf.rect.width * (werewolfHealth / 75.0f), 5, GREEN);
        }
        
        if (wizardHealth > 0) {
            wizard.draw();
            // Draw health bar for wizard
            DrawRectangle(wizard.rect.x, wizard.rect.y - 10, wizard.rect.width, 5, RED);
            DrawRectangle(wizard.rect.x, wizard.rect.y - 10, wizard.rect.width * (wizardHealth / 60.0f), 5, GREEN);
        }
        
        if (demonHealth > 0) {
            // Use our custom draw function instead of demon.draw()
            drawScaledDemon(demon);
            
            // Draw health bar for demon (scaled to match visual size)
            float scaledWidth = demon.rect.width * demonScaleFactor;
            DrawRectangle(demon.rect.x, demon.rect.y - 10, scaledWidth, 5, RED);
            DrawRectangle(demon.rect.x, demon.rect.y - 10, scaledWidth * (demonHealth / 150.0f), 5, GREEN);
        }
        
        // If the key is not collected, draw it in its original position
        if (!keyCollected) {
            // Source rectangle for current frame of key
            Rectangle keySource = {
                (float)(keyCurrentFrame * KEY_FRAME_WIDTH),  // x position in sprite sheet
                0.0f,                              // y position in sprite sheet
                (float)KEY_FRAME_WIDTH,           // width of one frame
                (float)KEY_FRAME_HEIGHT           // height of frame
            };
            
            // Draw the key with animation
            DrawTexturePro(
                keyTexture,
                keySource,
                (Rectangle){
                    keyPosition.x,
                    keyPosition.y,
                    (float)KEY_FRAME_WIDTH,
                    (float)KEY_FRAME_HEIGHT
                },
                (Vector2){0, 0},
                keyRotation,
                WHITE
            );
        } else {
            // Key is collected, draw it above the samurai
            CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
            if (samuraiBody && samuraiBody->active) {
                // Source rectangle for current frame of key
                Rectangle keySource = {
                    (float)(keyCurrentFrame * KEY_FRAME_WIDTH),  // x position in sprite sheet
                    0.0f,                              // y position in sprite sheet
                    (float)KEY_FRAME_WIDTH,           // width of one frame
                    (float)KEY_FRAME_HEIGHT           // height of frame
                };
                
                // Draw the key floating above the samurai
                DrawTexturePro(
                    keyTexture,
                    keySource,
                    (Rectangle){
                        samuraiBody->rect.x + samuraiBody->rect.width/2 - KEY_FRAME_WIDTH/2, // Center over samurai
                        samuraiBody->rect.y - KEY_FRAME_HEIGHT - 10, // Above the samurai with a small gap
                        (float)KEY_FRAME_WIDTH,
                        (float)KEY_FRAME_HEIGHT
                    },
                    (Vector2){0, 0},
                    keyRotation,
                    WHITE
                );
            }
        }
        
        // Draw collision boxes if enabled
        if (showCollisionBoxes) {
            // Draw key collision box
            if (!keyCollected) {
                DrawRectangleLines(
                    keyCollisionRect.x,
                    keyCollisionRect.y,
                    keyCollisionRect.width,
                    keyCollisionRect.height,
                    PURPLE  // Use purple to distinguish from other collision boxes
                );
            }
            
            // Draw collision boxes for each character type
            // For Samurai
            CollisionBox* bodyBox = samurai.getCollisionBox(BODY);
            if (bodyBox) drawCollisionBox(*bodyBox);
            
            CollisionBox* attackBox = samurai.getCollisionBox(ATTACK);
            if (attackBox) drawCollisionBox(*attackBox);
            
            CollisionBox* hurtBox = samurai.getCollisionBox(HURTBOX);
            if (hurtBox) drawCollisionBox(*hurtBox);
            
            // For Goblin
            if (goblinHealth > 0) {
                bodyBox = goblin.getCollisionBox(BODY);
                if (bodyBox) drawCollisionBox(*bodyBox);
                
                attackBox = goblin.getCollisionBox(ATTACK);
                if (attackBox) drawCollisionBox(*attackBox);
                
                hurtBox = goblin.getCollisionBox(HURTBOX);
                if (hurtBox) drawCollisionBox(*hurtBox);
            }
            
            // For Werewolf
            if (werewolfHealth > 0) {
                bodyBox = werewolf.getCollisionBox(BODY);
                if (bodyBox) drawCollisionBox(*bodyBox);
                
                attackBox = werewolf.getCollisionBox(ATTACK);
                if (attackBox) drawCollisionBox(*attackBox);
                
                hurtBox = werewolf.getCollisionBox(HURTBOX);
                if (hurtBox) drawCollisionBox(*hurtBox);
            }
            
            // For Wizard
            if (wizardHealth > 0) {
                bodyBox = wizard.getCollisionBox(BODY);
                if (bodyBox) drawCollisionBox(*bodyBox);
                
                attackBox = wizard.getCollisionBox(ATTACK);
                if (attackBox) drawCollisionBox(*attackBox);
                
                hurtBox = wizard.getCollisionBox(HURTBOX);
                if (hurtBox) drawCollisionBox(*hurtBox);
            }
            
            // For Demon
            if (demonHealth > 0) {
                bodyBox = demon.getCollisionBox(BODY);
                if (bodyBox) drawCollisionBox(*bodyBox);
                
                attackBox = demon.getCollisionBox(ATTACK);
                if (attackBox) drawCollisionBox(*attackBox);
                
                hurtBox = demon.getCollisionBox(HURTBOX);
                if (hurtBox) drawCollisionBox(*hurtBox);
            }
        }
        
        // End 2D camera mode
        EndMode2D();

        // Draw UI elements on top of everything
        EndMode2D();

        // Move keyboard instructions to bottom left corner
        int instructionsY = screenHeight - 210; // Starting Y position for instructions
        int lineHeight = 25; // Height of each line of text
        
        DrawText("GAME CONTROLS:", 10, instructionsY, 20, DARKGRAY);
        DrawText("W or Up: Jump (press twice for double jump)", 10, instructionsY + lineHeight, 20, DARKGRAY);
        DrawText("A/D or Left/Right: Move", 10, instructionsY + lineHeight*2, 20, DARKGRAY);
        DrawText("Space: Attack", 10, instructionsY + lineHeight*3, 20, DARKGRAY);
        DrawText("Double-tap A/D: Dash", 10, instructionsY + lineHeight*4, 20, DARKGRAY);
        DrawText("F1: Toggle collision boxes", 10, instructionsY + lineHeight*5, 20, DARKGRAY);
        DrawText("M: Toggle music", 10, instructionsY + lineHeight*6, 20, DARKGRAY);
        DrawText("Mouse Wheel: Zoom", 10, instructionsY + lineHeight*7, 20, DARKGRAY);

        EndDrawing();
        
        // Check if window should close
        if (WindowShouldClose()) {
            // Use our custom exit function instead of letting the loop end naturally
            safeExit();
        }
    }

    // Unload key texture
    UnloadTexture(keyTexture);

    // Unload sounds
    UnloadSound(keySound);

    // This code should never be reached because we call safeExit() when WindowShouldClose() is true
    return 0;
}