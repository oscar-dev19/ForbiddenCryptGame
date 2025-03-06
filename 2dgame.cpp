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

    // Game loop
    while (!WindowShouldClose()) {
        // Check for F1 key press to toggle collision box visibility
        if (IsKeyPressed(KEY_F1)) {
            showCollisionBoxes = !showCollisionBoxes;
        }
        
        // Update characters
        samurai.updateSamurai();
        goblin.update();
        werewolf.update();
        wizard.update();
        demon.update();

        // Check for collisions
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

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();

    return 0;
}