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

// Helper function to check collision between two collision boxes
bool checkCharacterCollision(const CollisionBox& box1, const CollisionBox& box2) {
    if (box1.active && box2.active) {
        return CheckCollisionRecs(box1.rect, box2.rect);
    }
    return false;
}

// Helper function to handle attack collision and damage
void handleAttackCollision(CollisionBox* attackBox, CollisionBox* hurtBox, int& health, int damage) {
    if (attackBox && hurtBox && checkCharacterCollision(*attackBox, *hurtBox)) {
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
    SetTargetFPS(60);

    // Define floor level at the bottom of the screen
    const float floorHeight = 50.0f;
    const float floorLevel = screenHeight - floorHeight;

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

    // Game loop
    while (!WindowShouldClose()) {
        // Update characters
        samurai.updateSamurai();
        goblin.update();
        werewolf.update();
        wizard.update();
        demon.update();

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

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();

    return 0;
}