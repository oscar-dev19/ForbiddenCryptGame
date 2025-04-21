#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "raylib.h"
#include <vector>

/**
 * @file CollisionSystem.h
 * @brief Defines the collision detection system used in the game.
 *
 * This file includes common collision structures and functions that handle
 * collision detection between characters, attacks, and environmental objects.
 */

// Global scale factor for all sprites and collision boxes
const float SPRITE_SCALE = 1.5f;

// Global flag to toggle collision box visibility
extern bool showCollisionBoxes;

/**
 * @enum CollisionBoxType
 * @brief Defines the different types of collision boxes used in the game.
 */
enum CollisionBoxType {
    BODY,       ///< Main body collision for movement and general collisions.
    ATTACK,     ///< Attack hitbox for detecting when attacks hit enemies.
    HURTBOX     ///< Vulnerable area where the character can be hit.
};

/**
 * @struct CollisionBox
 * @brief Represents a collision box used for character and attack detection.
 */
struct CollisionBox {
    Rectangle rect;      ///< The rectangle defining the collision box.
    CollisionBoxType type; ///< Type of collision box.
    bool active;         ///< Whether the collision box is currently active.

    /**
     * @brief Constructs a CollisionBox.
     * @param r The rectangle defining the collision box.
     * @param t The type of the collision box (BODY, ATTACK, HURTBOX).
     * @param a Whether the collision box is active (default: true).
     */
    CollisionBox(Rectangle r, CollisionBoxType t, bool a = true) 
        : rect(r), type(t), active(a) {}
};

/**
 * @brief Checks for a collision between two active collision boxes.
 *
 * This function determines if two active collision boxes overlap.
 * If either box is inactive, it returns false.
 *
 * @param box1 The first collision box.
 * @param box2 The second collision box.
 * @return True if the boxes collide, false otherwise.
 */
bool checkCollision(const CollisionBox& box1, const CollisionBox& box2) {
    if (box1.active && box2.active) {
        return CheckCollisionRecs(box1.rect, box2.rect);
    }
    return false;
}

#endif // COLLISION_SYSTEM_H