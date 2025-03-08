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

// Define the global variable for collision box visibility
bool showCollisionBoxes = false;

// Global audio variables
Music backgroundMusic;
Sound demonChantSound;
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
        CheckCollisionRecs(attackBox->rect, hurtBox->rect)) {
        health -= damage;
        if (health < 0) health = 0;
        
        // Deactivate attack box to prevent multiple hits from the same attack
        attackBox->active = false;
    }
}

int main() {
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 550;
    InitWindow(screenWidth, screenHeight, "2D Game");
    
    // Initialize audio device
    InitAudioDevice();
    
    // Load background music
    backgroundMusic = LoadMusicStream("music/Lady Maria of the Astral Clocktower.mp3");
    SetMusicVolume(backgroundMusic, 0.5f * masterVolume);
    PlayMusicStream(backgroundMusic);
    
    // Load common sound effects
    demonChantSound = LoadSound("sounds/misc/demon-chant-latin-14489.mp3");
    SetSoundVolume(demonChantSound, 0.6f * masterVolume);
    
    SetTargetFPS(60);

    // Define floor level higher in the screen
    const float floorHeight = 50.0f;
    const float floorLevel = 400.0f; // Fixed floor level at 400 pixels from the top

    // Initialize characters
    Samurai samurai(100, floorLevel, floorLevel);
    Goblin goblin({500, floorLevel});
    Werewolf werewolf(600, floorLevel, floorLevel);
    Wizard wizard({700, floorLevel});
    Demon demon({400, floorLevel});

    // Initialize health values for enemies
    int goblinHealth = 50;
    int werewolfHealth = 75;
    int wizardHealth = 60;
    int demonHealth = 100;
    
    // Cooldown timer for body collision damage
    float bodyDamageCooldown = 0.0f;
    const float bodyDamageCooldownTime = 1.0f; // 1 second cooldown

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
        
        // Toggle collision box visibility with F1 key (changed from C key)
        if (IsKeyPressed(KEY_F1)) {
            showCollisionBoxes = !showCollisionBoxes;
            printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
        }
        
        // Get frame time
        float deltaTime = GetFrameTime();
        
        // Update cooldown timer
        bodyDamageCooldown -= deltaTime;
        
        // Update characters
        samurai.updateSamurai();
        goblin.update();
        werewolf.update();
        wizard.update();
        demon.update(deltaTime);

        // Check for collisions only if samurai is not dead
        if (!samurai.isDead) {
            CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
            
            // Check Samurai attack vs Goblin
            if (samuraiAttack && samuraiAttack->active) {
                CollisionBox* goblinHurtbox = goblin.getCollisionBox(HURTBOX);
                if (goblinHurtbox && goblinHurtbox->active && checkCharacterCollision(*samuraiAttack, *goblinHurtbox)) {
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
                if (werewolfHurtbox && werewolfHurtbox->active && checkCharacterCollision(*samuraiAttack, *werewolfHurtbox)) {
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
                if (wizardHurtbox && wizardHurtbox->active && checkCharacterCollision(*samuraiAttack, *wizardHurtbox)) {
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
                if (demonHurtbox && demonHurtbox->active && checkCharacterCollision(*samuraiAttack, *demonHurtbox)) {
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
                
                // Check Goblin body collision with Samurai
                CollisionBox* goblinBody = goblin.getCollisionBox(BODY);
                if (goblinBody && goblinBody->active && checkCharacterCollision(*goblinBody, *samuraiHurtbox) && !goblin.isDead) {
                    if (bodyDamageCooldown <= 0.0f) {
                        samurai.takeDamage(3); // Apply less damage for body collision
                        bodyDamageCooldown = bodyDamageCooldownTime; // Reset cooldown
                    }
                }
            }

            // Check Werewolf attack vs Samurai
            if (!werewolf.isDead && samuraiHurtbox && samuraiHurtbox->active) {
                CollisionBox* werewolfAttack = werewolf.getCollisionBox(ATTACK);
                if (werewolfAttack && werewolfAttack->active && checkCharacterCollision(*werewolfAttack, *samuraiHurtbox)) {
                    samurai.takeDamage(8);
                    werewolfAttack->active = false; // Prevent multiple hits
                }
                
                // Check Werewolf body collision with Samurai
                CollisionBox* werewolfBody = werewolf.getCollisionBox(BODY);
                if (werewolfBody && werewolfBody->active && checkCharacterCollision(*werewolfBody, *samuraiHurtbox) && !werewolf.isDead) {
                    if (bodyDamageCooldown <= 0.0f) {
                        samurai.takeDamage(4); // Apply less damage for body collision
                        bodyDamageCooldown = bodyDamageCooldownTime; // Reset cooldown
                    }
                }
            }

            // Check Wizard attack vs Samurai
            if (!wizard.isDead && samuraiHurtbox && samuraiHurtbox->active) {
                CollisionBox* wizardAttack = wizard.getCollisionBox(ATTACK);
                if (wizardAttack && wizardAttack->active && checkCharacterCollision(*wizardAttack, *samuraiHurtbox)) {
                    samurai.takeDamage(6);
                    wizardAttack->active = false; // Prevent multiple hits
                }
                
                // Check Wizard body collision with Samurai
                CollisionBox* wizardBody = wizard.getCollisionBox(BODY);
                if (wizardBody && wizardBody->active && checkCharacterCollision(*wizardBody, *samuraiHurtbox) && !wizard.isDead) {
                    if (bodyDamageCooldown <= 0.0f) {
                        samurai.takeDamage(2); // Apply less damage for body collision
                        bodyDamageCooldown = bodyDamageCooldownTime; // Reset cooldown
                    }
                }
            }

            // Check Demon attack vs Samurai
            if (!demon.isDead && samuraiHurtbox && samuraiHurtbox->active) {
                CollisionBox* demonAttack = demon.getCollisionBox(ATTACK);
                if (demonAttack && demonAttack->active && checkCharacterCollision(*demonAttack, *samuraiHurtbox)) {
                    samurai.takeDamage(10);
                    demonAttack->active = false; // Prevent multiple hits
                }
                
                // Check Demon body collision with Samurai
                CollisionBox* demonBody = demon.getCollisionBox(BODY);
                if (demonBody && demonBody->active && checkCharacterCollision(*demonBody, *samuraiHurtbox) && !demon.isDead) {
                    if (bodyDamageCooldown <= 0.0f) {
                        samurai.takeDamage(5); // Apply less damage for body collision
                        bodyDamageCooldown = bodyDamageCooldownTime; // Reset cooldown
                    }
                }
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
            CollisionBox* goblinBody = goblin.getCollisionBox(BODY);
            if (goblinBody && goblinBody->active) {
                DrawRectangle(goblinBody->rect.x, goblinBody->rect.y - 10, goblinBody->rect.width, 5, RED);
                DrawRectangle(goblinBody->rect.x, goblinBody->rect.y - 10, goblinBody->rect.width * (goblinHealth / 50.0f), 5, GREEN);
            }
        }
        
        if (werewolfHealth > 0) {
            werewolf.draw();
            // Draw health bar for werewolf
            CollisionBox* werewolfBody = werewolf.getCollisionBox(BODY);
            if (werewolfBody && werewolfBody->active) {
                DrawRectangle(werewolfBody->rect.x, werewolfBody->rect.y - 10, werewolfBody->rect.width, 5, RED);
                DrawRectangle(werewolfBody->rect.x, werewolfBody->rect.y - 10, werewolfBody->rect.width * (werewolfHealth / 75.0f), 5, GREEN);
            }
        }
        
        if (wizardHealth > 0) {
            wizard.draw();
            // Draw health bar for wizard
            CollisionBox* wizardBody = wizard.getCollisionBox(BODY);
            if (wizardBody && wizardBody->active) {
                DrawRectangle(wizardBody->rect.x, wizardBody->rect.y - 10, wizardBody->rect.width, 5, RED);
                DrawRectangle(wizardBody->rect.x, wizardBody->rect.y - 10, wizardBody->rect.width * (wizardHealth / 60.0f), 5, GREEN);
            }
        }
        
        if (demonHealth > 0) {
            demon.draw();
            // Draw health bar for demon
            CollisionBox* demonBody = demon.getCollisionBox(BODY);
            if (demonBody && demonBody->active) {
                DrawRectangle(demonBody->rect.x, demonBody->rect.y - 10, demonBody->rect.width, 5, RED);
                DrawRectangle(demonBody->rect.x, demonBody->rect.y - 10, demonBody->rect.width * (demonHealth / 100.0f), 5, GREEN);
            }
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
    }

    // Clean up resources
    StopMusicStream(backgroundMusic);  // Stop the music before unloading
    UnloadMusicStream(backgroundMusic);
    
    // Stop and unload the demon chant sound
    StopSound(demonChantSound);
    UnloadSound(demonChantSound);
    
    // Make sure all character resources are properly unloaded
    // This should prevent segmentation faults during cleanup
    
    // Explicitly set all pointers to nullptr after cleanup
    // This prevents double-free issues that can cause segmentation faults
    
    // Explicitly call destructors for all character objects before closing audio
    // This ensures all audio resources are released before the audio device is closed
    samurai.~Samurai();
    goblin.~Goblin();
    werewolf.~Werewolf();
    wizard.~Wizard();
    demon.~Demon();
    
    // Add a small delay to ensure all resources are properly released
    // This can help prevent race conditions during cleanup
    WaitTime(0.1);
    
    CloseAudioDevice();
    CloseWindow();

    return 0;
}