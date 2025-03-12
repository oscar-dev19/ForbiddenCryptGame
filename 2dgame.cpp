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

int main() {
    // Set up error handling
    SetTraceLogLevel(LOG_WARNING);
    
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 550;
    InitWindow(screenWidth, screenHeight, "2D Game");

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

    // Define floor level higher up on the screen (moved up by 100 pixels)
    const float floorHeight = 50.0f;
    const float floorOffset = 120.0f;  // Offset from the bottom to ensure visibility
    const float floorLevel = screenHeight - floorHeight - floorOffset;

    // Load key texture
    keyTexture = LoadTexture("assets/gameObjects/key/key.png");
    if (keyTexture.id == 0) {
        printf("Failed to load key texture!\n");
        CloseWindow();
        return -1;
    }
    keyPosition = { 200, floorLevel - keyTexture.height };

    // Initialize characters using stack allocation
    Samurai samurai(100, floorLevel, floorLevel);
    Goblin goblin((Vector2){500, floorLevel});
    goblin.loadTextures();
    Werewolf werewolf(600, floorLevel, floorLevel);
    werewolf.loadTextures();
    Wizard wizard((Vector2){700, floorLevel});
    wizard.loadTextures();
    
    // Create the demon at normal size (we'll scale it visually when drawing)
    Demon demon((Vector2){400, floorLevel});

    // Initialize health values for enemies
    int goblinHealth = 50;
    int werewolfHealth = 75;
    int wizardHealth = 60;
    int demonHealth = 150; // Increased demon health to match its larger appearance

    // Game loop
    while (!WindowShouldClose()) {
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
        }
        
        if (IsKeyPressed(KEY_MINUS)) { // - key
            masterVolume -= 0.1f;
            if (masterVolume < 0.0f) masterVolume = 0.0f;
            SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
            SetSoundVolume(demonChantSound, 0.6f * masterVolume);
        }
        
        // Double jump height control with PAGE UP and PAGE DOWN keys
        if (IsKeyPressed(KEY_PAGE_UP)) {
            // Increase double jump height with a maximum limit
            float currentHeight = samurai.getDoubleJumpHeight();
            if (currentHeight < 100.0f) { // Increased maximum limit to match the doubled max height
                samurai.setDoubleJumpHeight(currentHeight + 5.0f);
            }
        }
        if (IsKeyPressed(KEY_PAGE_DOWN)) {
            // Decrease double jump height (with minimum limit)
            float currentHeight = samurai.getDoubleJumpHeight();
            if (currentHeight > 40.0f) { // Don't go too low
                samurai.setDoubleJumpHeight(currentHeight - 5.0f);
            }
        }
        
        // Adjust dash sound volume with [ and ] keys
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            // Decrease dash sound volume
            float currentVolume = samurai.getDashSoundVolume();
            samurai.setDashSoundVolume(currentVolume - 0.1f);
        }
        
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            // Increase dash sound volume
            float currentVolume = samurai.getDashSoundVolume();
            samurai.setDashSoundVolume(currentVolume + 0.1f);
        }
        
        // Toggle collision box visibility with F1 key
        if (IsKeyPressed(KEY_F1)) {
            showCollisionBoxes = !showCollisionBoxes;
            printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
        }
        
        // Get frame time
        float deltaTime = GetFrameTime();
        
        // Update characters
        samurai.updateSamurai();
        goblin.update();
        werewolf.update();
        wizard.update();
        demon.update(deltaTime);

        // Check collision between samurai and key
        CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
        if (samuraiBody && samuraiBody->active) {
            if (CheckCollisionRecs(samuraiBody->rect, keyCollisionRect) && !keyCollected) {
                // Play key collection sound
                PlaySound(keySound);
                keyCollected = true;
                
                // Display message with shadow effect
                DrawText("Key Collected!", screenWidth/2 - 48, screenHeight/2 + 2, 20, BLACK); // Shadow
                DrawText("Key Collected!", screenWidth/2 - 50, screenHeight/2, 20, GOLD);      // Main text
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
        
        // Update camera to follow player
        Rectangle samuraiRect = samurai.getRect();
        camera.target = (Vector2){ samuraiRect.x, samuraiRect.y };
        
        // Camera zoom controls
        camera.zoom += ((float)GetMouseWheelMove() * 0.1f);
        if (camera.zoom < 0.5f) camera.zoom = 0.5f;
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Begin 2D camera mode
        BeginMode2D(camera);
        
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
        
        // Draw background floor (only if needed)
        DrawRectangle(0, floorLevel, screenWidth * 2, floorHeight, DARKGRAY);

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
        
        // If the key is not collected, draw it
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

        // Draw UI elements (not affected by camera)
        // Draw collected key icon if collected AND samurai is alive
        if (keyCollected && !samurai.isDead) {
            // Source rectangle for first frame of key
            Rectangle keyIconSource = {
                0.0f,                     // First frame (x = 0)
                0.0f,                     // y position in sprite sheet
                (float)KEY_FRAME_WIDTH,   // width of one frame
                (float)KEY_FRAME_HEIGHT   // height of frame
            };
            
            // Position the key icon in the UI area
            Rectangle keyIconDest = {
                20,                  // left side of screen
                20,                  // top of screen
                24.0f,               // smaller size for UI
                24.0f                // smaller size for UI
            };
            
            // Draw the key icon in the UI
            DrawTexturePro(
                keyTexture,
                keyIconSource,
                keyIconDest,
                (Vector2){0, 0},
                0.0f,
                WHITE
            );
        }

        // Draw instructions with shadow
        DrawText("Controls: A/D to move, SPACE to attack, W to jump", 12, 32, 20, BLACK); // Shadow
        DrawText("Controls: A/D to move, SPACE to attack, W to jump", 10, 30, 20, WHITE); // Main text
        
        // Double jump instructions 
        DrawText("Press W in mid-air to double jump", 12, 52, 20, BLACK); // Shadow
        DrawText("Press W in mid-air to double jump", 10, 50, 20, WHITE); // Main text
        
        // Double dash instructions
        DrawText("Double-tap A or D to dash in that direction", 12, 72, 20, BLACK); // Shadow
        DrawText("Double-tap A or D to dash in that direction", 10, 70, 20, WHITE); // Main text
        
        // Double jump height adjustment instructions
        DrawText("PAGE UP/DOWN to adjust double jump height", 12, 92, 20, BLACK); // Shadow
        DrawText("PAGE UP/DOWN to adjust double jump height", 10, 90, 20, WHITE); // Main text

        // Draw UI elements with shadow effect
        // Health text
        DrawText(TextFormat("Samurai Health: %d", samurai.getHealth()), 12, 112, 20, BLACK); // Shadow
        DrawText(TextFormat("Samurai Health: %d", samurai.getHealth()), 10, 110, 20, WHITE); // Main text
        
        // Double jump height display
        DrawText(TextFormat("Double Jump Height: %.1f", samurai.getDoubleJumpHeight()), 12, 132, 20, BLACK); // Shadow
        DrawText(TextFormat("Double Jump Height: %.1f", samurai.getDoubleJumpHeight()), 10, 130, 20, WHITE); // Main text
        
        // Score display
        static int score = 0;
        DrawText(TextFormat("Score: %d", score), 12, 152, 20, BLACK); // Shadow
        DrawText(TextFormat("Score: %d", score), 10, 150, 20, WHITE); // Main text
        
        // F1 help text
        DrawText("Press F1 to toggle collision boxes", 12, 172, 20, BLACK); // Shadow
        DrawText("Press F1 to toggle collision boxes", 10, 170, 20, WHITE); // Main text

        // Draw audio controls help text with shadow
        DrawText("Audio Controls:", 12, screenHeight - 98, 16, BLACK); // Shadow
        DrawText("Audio Controls:", 10, screenHeight - 100, 16, WHITE); // Main text
        
        DrawText("M - Toggle Music", 12, screenHeight - 78, 16, BLACK); // Shadow
        DrawText("M - Toggle Music", 10, screenHeight - 80, 16, WHITE); // Main text
        
        DrawText("+ / - - Volume Control", 12, screenHeight - 58, 16, BLACK); // Shadow
        DrawText("+ / - - Volume Control", 10, screenHeight - 60, 16, WHITE); // Main text
        
        DrawText("[ / ] - Dash Sound Volume", 12, screenHeight - 38, 16, BLACK); // Shadow
        DrawText("[ / ] - Dash Sound Volume", 10, screenHeight - 40, 16, WHITE); // Main text
        
        DrawText(TextFormat("Master Volume: %.1f", masterVolume), 12, screenHeight - 18, 16, BLACK); // Shadow
        DrawText(TextFormat("Master Volume: %.1f", masterVolume), 10, screenHeight - 20, 16, WHITE); // Main text
        
        // Display dash sound volume
        float dashVolume = samurai.getDashSoundVolume();
        DrawText(TextFormat("Dash Volume: %.1f", dashVolume), 250, screenHeight - 18, 16, BLACK); // Shadow
        DrawText(TextFormat("Dash Volume: %.1f", dashVolume), 248, screenHeight - 20, 16, WHITE); // Main text

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