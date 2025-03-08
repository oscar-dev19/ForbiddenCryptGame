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
    printf("Exiting safely...\n");
    
    // Turn off collision boxes before exiting to prevent segfault
    showCollisionBoxes = false;
    
    // Unload audio resources directly
    if (backgroundMusic.stream.buffer) {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
    }
    
    if (demonChantSound.stream.buffer) {
        StopSound(demonChantSound);
        UnloadSound(demonChantSound);
    }
    
    // Close audio device and window
    CloseAudioDevice();
    CloseWindow();
    
    // Exit directly without going through normal cleanup
    _Exit(0); // Use _Exit instead of exit to bypass any atexit handlers
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
    
    SetTargetFPS(60);

    // Define floor level higher up on the screen (moved up by 100 pixels)
    const float floorHeight = 50.0f;
    const float floorLevel = screenHeight - floorHeight - 100.0f;

    // Initialize characters using stack allocation
    Samurai samurai(100, floorLevel, floorLevel);
    Goblin goblin((Vector2){500, floorLevel});
    goblin.loadTextures();
    Werewolf werewolf(600, floorLevel, floorLevel);
    werewolf.loadTextures();
    Wizard wizard((Vector2){700, floorLevel});
    wizard.loadTextures();
    Demon demon((Vector2){400, floorLevel});

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
        samurai.updateSamurai();
        goblin.update();
        werewolf.update();
        wizard.update();
        demon.update(deltaTime);

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
                samurai.takeDamage(10);
                demonAttack->active = false; // Prevent multiple hits
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw background
        DrawRectangle(0, floorLevel, screenWidth, floorHeight, DARKGRAY);

        // Draw characters
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
            demon.draw();
            // Draw health bar for demon
            DrawRectangle(demon.rect.x, demon.rect.y - 10, demon.rect.width, 5, RED);
            DrawRectangle(demon.rect.x, demon.rect.y - 10, demon.rect.width * (demonHealth / 100.0f), 5, GREEN);
        }

        // Draw Samurai health bar
        samurai.drawHealthBar();

        // Draw instructions
        DrawText("Controls: A/D to move, J to attack, SPACE to jump", 10, 30, 20, BLACK);
        DrawText("Press F1 to toggle collision boxes", 10, 50, 20, BLACK);

        // Draw UI elements
        DrawText(TextFormat("Samurai Health: %d", samurai.getHealth()), 10, 10, 20, RED);
        
        // Draw audio controls help text
        DrawText("Audio Controls:", 10, screenHeight - 80, 16, WHITE);
        DrawText("M - Toggle Music", 10, screenHeight - 60, 16, WHITE);
        DrawText("+ / - - Volume Control", 10, screenHeight - 40, 16, WHITE);
        DrawText(TextFormat("Volume: %.1f", masterVolume), 10, screenHeight - 20, 16, WHITE);

        EndDrawing();
        
        // Check if window should close
        if (WindowShouldClose()) {
            // Use our custom exit function instead of letting the loop end naturally
            safeExit();
        }
    }

    // This code should never be reached because we call safeExit() when WindowShouldClose() is true
    return 0;
}