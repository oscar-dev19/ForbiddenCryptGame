#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include "CollisionSystem.h"

/**
 * @class Character
 * @brief Abstract base class for all in-game characters.
 *
 * This class defines common properties and behaviors for all characters,
 * including movement, health management, collision detection, and rendering.
 */
class Character {
protected:
    Rectangle rect;  ///< Character's position and size.
    float speed;     ///< Movement speed of the character.
    int health;      ///< Current health of the character.
    int maxHealth;   ///< Maximum health of the character.

    // Health bar properties
    const float HEALTH_BAR_WIDTH = 50.0f;
    const float HEALTH_BAR_HEIGHT = 5.0f;
    const float HEALTH_BAR_Y_OFFSET = -20.0f;

public:
    /**
     * @brief Constructs a Character with a position, speed, and health.
     * @param position Initial position of the character.
     * @param baseSpeed Movement speed of the character.
     * @param startingHealth Initial health of the character.
     */
    Character(Vector2 position, float baseSpeed, int startingHealth) 
        : speed(baseSpeed), health(startingHealth), maxHealth(startingHealth) {
        rect = { position.x, position.y, 64.0f, 64.0f };
    }

    /**
     * @brief Virtual destructor for the Character class.
     */
    virtual ~Character() {}

    // Pure virtual functions to be implemented by derived classes
    /**
     * @brief Draws the character.
     */
    virtual void draw() const = 0;

    /**
     * @brief Handles the character's attack action.
     */
    virtual void attack() = 0;

    /**
     * @brief Applies damage to the character.
     * @param damage Amount of damage taken.
     */
    virtual void takeDamage(int damage) = 0;

    /**
     * @brief Handles character death.
     */
    virtual void die() = 0;

    /**
     * @brief Updates the character's state.
     */
    virtual void update() = 0;

    /**
     * @brief Updates the character's state based on a target position.
     * @param targetPos The position of the target (e.g., enemy or player).
     */
    virtual void updateWithTarget(Vector2 targetPos) = 0;

    /**
     * @brief Retrieves the collision box of a specific type.
     * @param type The type of collision box (BODY, ATTACK, HURTBOX).
     * @return Pointer to the requested collision box.
     */
    virtual CollisionBox* getCollisionBox(CollisionBoxType type) const = 0;

    /**
     * @brief Draws the character's health bar above their sprite.
     */
    void drawHealthBar() const {
        float healthPercent = (float)health / maxHealth;
        float barX = rect.x + (rect.width - HEALTH_BAR_WIDTH) / 2;
        float barY = rect.y + HEALTH_BAR_Y_OFFSET;
        
        // Draw background (empty health bar)
        DrawRectangle(barX, barY, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, GRAY);
        
        // Draw current health
        DrawRectangle(barX, barY, HEALTH_BAR_WIDTH * healthPercent, HEALTH_BAR_HEIGHT, RED);
        
        // Draw border
        DrawRectangleLines(barX, barY, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, BLACK);
    }

    // Getters and setters

    /**
     * @brief Gets the character's bounding rectangle.
     * @return The rectangle representing the character's position and size.
     */
    Rectangle getRect() const { return rect; }

    /**
     * @brief Gets the character's movement speed.
     * @return The movement speed.
     */
    float getSpeed() const { return speed; }

    /**
     * @brief Gets the character's current health.
     * @return The current health value.
     */
    int getHealth() const { return health; }

    /**
     * @brief Gets the character's maximum health.
     * @return The maximum health value.
     */
    int getMaxHealth() const { return maxHealth; }

    /**
     * @brief Sets the character's position.
     * @param x The new X coordinate.
     * @param y The new Y coordinate.
     */
    void setPosition(float x, float y) {
        rect.x = x;
        rect.y = y;
    }

    /**
     * @brief Moves the character by a given offset.
     * @param dx The change in X position.
     * @param dy The change in Y position.
     */
    void moveBy(float dx, float dy) {
        rect.x += dx;
        rect.y += dy;
    }

    // Get components of the character's bounding box for rendering

    /**
     * @brief Gets the X coordinate of the character.
     * @return The X position.
     */
    float getX() const { return rect.x; }

    /**
     * @brief Gets the Y coordinate of the character.
     * @return The Y position.
     */
    float getY() const { return rect.y; }

    /**
     * @brief Gets the width of the character's bounding box.
     * @return The width value.
     */
    float getWidth() const { return rect.width; }

    /**
     * @brief Gets the height of the character's bounding box.
     * @return The height value.
     */
    float getHeight() const { return rect.height; }
};

#endif // CHARACTER_H