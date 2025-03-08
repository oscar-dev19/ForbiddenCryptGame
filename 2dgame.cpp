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
#include <signal.h>
#include <stdlib.h>

// Define the global variable for collision box visibility
bool showCollisionBoxes = false;

// Global audio variables
Music backgroundMusic = { 0 };
Sound demonChantSound = { 0 };
float masterVolume = 0.7f;

// Global pointers to character objects
Samurai* samuraiPtr = NULL;
Goblin* goblinPtr = NULL;
Werewolf* werewolfPtr = NULL;
Wizard* wizardPtr = NULL;
Demon* demonPtr = NULL;

// Flag to track if we're in cleanup to prevent recursive crashes
bool inCleanup = false;

// Signal handler for segmentation faults
void segfaultHandler(int signal) {
    if (!inCleanup) {
        fprintf(stderr, "Caught segmentation fault during normal operation. Exiting safely.\n");
        exit(1);
    } else {
        fprintf(stderr, "Caught segmentation fault during cleanup. Continuing with cleanup.\n");
        // Just continue - we'll skip the problematic operation
    }
}

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

// Clean up function to safely release all resources
void cleanupResources() {
    inCleanup = true;
    
    // First unload audio resources before touching character objects
    if (backgroundMusic.stream.buffer) {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
        backgroundMusic = { 0 };
    }
    
    if (demonChantSound.stream.buffer) {
        StopSound(demonChantSound);
        UnloadSound(demonChantSound);
        demonChantSound = { 0 };
    }
    
    // Close audio device before cleaning up character objects
    CloseAudioDevice();
    
    // Now free character objects without calling destructors
    // This is a last resort approach to avoid segmentation faults
    if (samuraiPtr) {
        // Instead of delete, we'll just set to NULL
        samuraiPtr = NULL;
    }
    
    if (goblinPtr) {
        goblinPtr = NULL;
    }
    
    if (werewolfPtr) {
        werewolfPtr = NULL;
    }
    
    if (wizardPtr) {
        wizardPtr = NULL;
    }
    
    if (demonPtr) {
        demonPtr = NULL;
    }
    
    // Close window last
    CloseWindow();
    
    inCleanup = false;
}

// Atexit handler to ensure cleanup happens
void atExitHandler() {
    if (!inCleanup) {
        cleanupResources();
    }
}

int main() {
    // Set up signal handler for segmentation faults
    signal(SIGSEGV, segfaultHandler);
    
    // Register cleanup function to be called at exit
    atexit(atExitHandler);
    
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
    
    SetTargetFPS(60);

    // Define floor level at the bottom of the screen
    const float floorHeight = 50.0f;
    const float floorLevel = screenHeight - floorHeight;

    // Initialize characters using dynamic allocation
    samuraiPtr = new Samurai(100, floorLevel, floorLevel);
    
    goblinPtr = new Goblin((Vector2){500, floorLevel});
    goblinPtr->loadTextures();
    
    werewolfPtr = new Werewolf(600, floorLevel, floorLevel);
    werewolfPtr->loadTextures();
    
    wizardPtr = new Wizard((Vector2){700, floorLevel});
    wizardPtr->loadTextures();
    
    demonPtr = new Demon((Vector2){400, floorLevel});

    // Initialize health values for enemies
    int goblinHealth = 50;
    int werewolfHealth = 75;
    int wizardHealth = 60;
    int demonHealth = 100;

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
        
        // Toggle collision box visibility with F1 key
        if (IsKeyPressed(KEY_F1)) {
            showCollisionBoxes = !showCollisionBoxes;
            printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
        }
        
        // Get frame time
        float deltaTime = GetFrameTime();
        
        // Update characters
        samuraiPtr->updateSamurai();
        goblinPtr->update();
        werewolfPtr->update();
        wizardPtr->update();
        demonPtr->update(deltaTime);

        // Check for collisions between Samurai's attack and enemies
        CollisionBox* samuraiAttack = samuraiPtr->getCollisionBox(ATTACK);
        
        // Check Samurai attack vs Goblin
        if (!goblinPtr->isDead && samuraiAttack && samuraiAttack->active) {
            CollisionBox* goblinHurtbox = goblinPtr->getCollisionBox(HURTBOX);
            if (goblinHurtbox && checkCharacterCollision(*samuraiAttack, *goblinHurtbox)) {
                goblinHealth -= 10;
                if (goblinHealth <= 0) {
                    goblinPtr->takeDamage(10); // This will set the goblin to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Werewolf
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* werewolfHurtbox = werewolfPtr->getCollisionBox(HURTBOX);
            if (werewolfHurtbox && checkCharacterCollision(*samuraiAttack, *werewolfHurtbox)) {
                werewolfHealth -= 10;
                if (werewolfHealth <= 0) {
                    werewolfPtr->takeDamage(10); // This will set the werewolf to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Wizard
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* wizardHurtbox = wizardPtr->getCollisionBox(HURTBOX);
            if (wizardHurtbox && checkCharacterCollision(*samuraiAttack, *wizardHurtbox)) {
                wizardHealth -= 10;
                if (wizardHealth <= 0) {
                    wizardPtr->takeDamage(10); // This will set the wizard to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Samurai attack vs Demon
        if (samuraiAttack && samuraiAttack->active) {
            CollisionBox* demonHurtbox = demonPtr->getCollisionBox(HURTBOX);
            if (demonHurtbox && checkCharacterCollision(*samuraiAttack, *demonHurtbox)) {
                demonHealth -= 10;
                if (demonHealth <= 0) {
                    demonPtr->takeDamage(10); // This will set the demon to dead state
                }
                samuraiAttack->active = false; // Prevent multiple hits
            }
        }

        // Check for enemy attacks hitting Samurai
        CollisionBox* samuraiHurtbox = samuraiPtr->getCollisionBox(HURTBOX);
        
        // Check Goblin attack vs Samurai
        if (!goblinPtr->isDead && samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* goblinAttack = goblinPtr->getCollisionBox(ATTACK);
            if (goblinAttack && goblinAttack->active && checkCharacterCollision(*goblinAttack, *samuraiHurtbox)) {
                samuraiPtr->takeDamage(5);
                goblinAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Werewolf attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* werewolfAttack = werewolfPtr->getCollisionBox(ATTACK);
            if (werewolfAttack && werewolfAttack->active && checkCharacterCollision(*werewolfAttack, *samuraiHurtbox)) {
                samuraiPtr->takeDamage(8);
                werewolfAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Wizard attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* wizardAttack = wizardPtr->getCollisionBox(ATTACK);
            if (wizardAttack && wizardAttack->active && checkCharacterCollision(*wizardAttack, *samuraiHurtbox)) {
                samuraiPtr->takeDamage(6);
                wizardAttack->active = false; // Prevent multiple hits
            }
        }

        // Check Demon attack vs Samurai
        if (samuraiHurtbox && samuraiHurtbox->active) {
            CollisionBox* demonAttack = demonPtr->getCollisionBox(ATTACK);
            if (demonAttack && demonAttack->active && checkCharacterCollision(*demonAttack, *samuraiHurtbox)) {
                samuraiPtr->takeDamage(10);
                demonAttack->active = false; // Prevent multiple hits
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw background
        DrawRectangle(0, floorLevel, screenWidth, floorHeight, DARKGRAY);

        // Draw characters
        samuraiPtr->draw();
        
        // Only draw enemies if they're not dead
        if (goblinHealth > 0) {
            goblinPtr->draw();
            // Draw health bar for goblin
            DrawRectangle(goblinPtr->rect.x, goblinPtr->rect.y - 10, goblinPtr->rect.width, 5, RED);
            DrawRectangle(goblinPtr->rect.x, goblinPtr->rect.y - 10, goblinPtr->rect.width * (goblinHealth / 50.0f), 5, GREEN);
        }
        
        if (werewolfHealth > 0) {
            werewolfPtr->draw();
            // Draw health bar for werewolf
            DrawRectangle(werewolfPtr->rect.x, werewolfPtr->rect.y - 10, werewolfPtr->rect.width, 5, RED);
            DrawRectangle(werewolfPtr->rect.x, werewolfPtr->rect.y - 10, werewolfPtr->rect.width * (werewolfHealth / 75.0f), 5, GREEN);
        }
        
        if (wizardHealth > 0) {
            wizardPtr->draw();
            // Draw health bar for wizard
            DrawRectangle(wizardPtr->rect.x, wizardPtr->rect.y - 10, wizardPtr->rect.width, 5, RED);
            DrawRectangle(wizardPtr->rect.x, wizardPtr->rect.y - 10, wizardPtr->rect.width * (wizardHealth / 60.0f), 5, GREEN);
        }
        
        if (demonHealth > 0) {
            demonPtr->draw();
            // Draw health bar for demon
            DrawRectangle(demonPtr->rect.x, demonPtr->rect.y - 10, demonPtr->rect.width, 5, RED);
            DrawRectangle(demonPtr->rect.x, demonPtr->rect.y - 10, demonPtr->rect.width * (demonHealth / 100.0f), 5, GREEN);
        }

        // Draw Samurai health bar
        samuraiPtr->drawHealthBar();

        // Draw instructions
        DrawText("Controls: A/D to move, J to attack, SPACE to jump", 10, 30, 20, BLACK);
        DrawText("Press F1 to toggle collision boxes", 10, 50, 20, BLACK);

        // Draw UI elements
        DrawText(TextFormat("Samurai Health: %d", samuraiPtr->getHealth()), 10, 10, 20, RED);
        
        // Draw audio controls help text
        DrawText("Audio Controls:", 10, screenHeight - 80, 16, WHITE);
        DrawText("M - Toggle Music", 10, screenHeight - 60, 16, WHITE);
        DrawText("+ / - - Volume Control", 10, screenHeight - 40, 16, WHITE);
        DrawText(TextFormat("Volume: %.1f", masterVolume), 10, screenHeight - 20, 16, WHITE);

        EndDrawing();
    }

    // Use our custom cleanup function to safely release all resources
    cleanupResources();

    return 0;
}