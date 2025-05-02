#ifndef CHARACTER_AI_H
#define CHARACTER_AI_H

#include "raylib.h"
#include "Character.h"
#include <memory>
#include <cmath>

// We don't need to define these functions as they're already in raymath.h
// which is included in the files that include this header


// AI States
enum class AIState {
    IDLE,
    CHASE,
    ATTACK,
    RETREAT
};

// Base behavior class
class AIBehavior {
protected:
    float attackRange;
    float chaseRange;
    float retreatRange;

public:
    AIBehavior(float aRange, float cRange, float rRange = 0.0f)
        : attackRange(aRange), chaseRange(cRange), retreatRange(rRange) {}
    virtual ~AIBehavior() = default;
    virtual AIState determineState(float distance) = 0;
};

// Aggressive behavior
class AggressiveBehavior : public AIBehavior {
public:
    AggressiveBehavior(float aRange, float cRange, float rRange = 0.0f)
        : AIBehavior(aRange, cRange, rRange) {}

    AIState determineState(float distance) override {
        if (distance <= attackRange) return AIState::ATTACK;
        if (distance <= chaseRange) return AIState::CHASE;
        return AIState::IDLE;
    }
};

// Defensive behavior
class DefensiveBehavior : public AIBehavior {
public:
    DefensiveBehavior(float safeDistance, float attackRange)
        : AIBehavior(attackRange, safeDistance, safeDistance) {}

    AIState determineState(float distance) override {
        if (distance <= retreatRange) return AIState::RETREAT;
        if (distance <= attackRange) return AIState::ATTACK;
        return AIState::IDLE;
    }
};

// Main AI class
class CharacterAI {
private:
    std::unique_ptr<AIBehavior> behavior;
    AIState currentState;

public:
    CharacterAI() : currentState(AIState::IDLE) {}

    void setBehavior(std::unique_ptr<AIBehavior> newBehavior) {
        behavior = std::move(newBehavior);
    }

    void update(Character* character, Vector2 targetPos, float deltaTime) {
        if (!behavior || !character) return;

        // Calculate distance to target
        Rectangle charRect = character->getRect();
        float distance = Vector2Distance(
            {charRect.x + charRect.width/2, charRect.y + charRect.height/2},
            targetPos
        );

        // Update state based on behavior
        currentState = behavior->determineState(distance);

        // Move character based on state
        Vector2 direction = {0, 0};
        float speed = character->getSpeed() * deltaTime;

        switch (currentState) {
            case AIState::CHASE:
                direction = Vector2Normalize(Vector2Subtract(targetPos, {charRect.x, charRect.y}));
                character->moveBy(direction.x * speed, direction.y * speed);
                break;

            case AIState::RETREAT:
                direction = Vector2Normalize(Vector2Subtract({charRect.x, charRect.y}, targetPos));
                character->moveBy(direction.x * speed, direction.y * speed);
                break;

            case AIState::ATTACK:
                character->attack();
                break;

            default:
                break;
        }
    }

    AIState getCurrentState() const { return currentState; }
};

#endif // CHARACTER_AI_H