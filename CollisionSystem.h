#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "raylib.h"
#include <vector>

// Global scale factor for all sprites and collision boxes
const float SPRITE_SCALE = 3.0f;

// Common collision box type enum to be used by all character classes
enum CollisionBoxType {
    BODY,       // Main body collision for movement and general collisions
    ATTACK,     // Attack hitbox for detecting when attacks hit enemies
    HURTBOX     // Vulnerable area where the character can be hit
};

// Common collision box struct to be used by all character classes
struct CollisionBox {
    Rectangle rect;      // The rectangle defining the collision box
    CollisionBoxType type; // Type of collision box
    bool active;         // Whether the collision box is currently active
    
    // Constructor
    CollisionBox(Rectangle r, CollisionBoxType t, bool a = true) 
        : rect(r), type(t), active(a) {}
};

// Helper function to check collision between two collision boxes
bool checkCollision(const CollisionBox& box1, const CollisionBox& box2) {
    if (box1.active && box2.active) {
        return CheckCollisionRecs(box1.rect, box2.rect);
    }
    return false;
}

#endif // COLLISION_SYSTEM_H 