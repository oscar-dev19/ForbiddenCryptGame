#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include "CollisionSystem.h"

class Character {
protected:
    Rectangle rect;
    float speed;
    int health;
    int maxHealth;
    const float HEALTH_BAR_WIDTH = 50.0f;
    const float HEALTH_BAR_HEIGHT = 5.0f;
    const float HEALTH_BAR_Y_OFFSET = -20.0f;

public:
    // Constructor with position, base speed, and health
    Character(Vector2 position, float baseSpeed, int startingHealth) 
        : speed(baseSpeed), health(startingHealth), maxHealth(startingHealth) {
        rect = { position.x, position.y, 64.0f, 64.0f };
    }

    // Virtual destructor
    virtual ~Character() {}

    // Pure virtual functions
    virtual void draw() const = 0;
    virtual void attack() = 0;
    virtual void takeDamage(int damage) = 0;
    virtual void die() = 0;
    virtual void update() = 0;
    virtual void updateWithTarget(Vector2 targetPos) = 0;
    virtual CollisionBox* getCollisionBox(CollisionBoxType type) const = 0;

    // Draw health bar
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
    Rectangle getRect() const { return rect; }
    float getSpeed() const { return speed; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    
    void setPosition(float x, float y) {
        rect.x = x;
        rect.y = y;
    }
    
    void moveBy(float dx, float dy) {
        rect.x += dx;
        rect.y += dy;
    }
    
    // Get components of rect for drawing
    float getX() const { return rect.x; }
    float getY() const { return rect.y; }
    float getWidth() const { return rect.width; }
    float getHeight() const { return rect.height; }
};

#endif // CHARACTER_H 