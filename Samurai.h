#ifndef SAMURAI_H
#define SAMURAI_H

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>

// Direction of the Character.
enum Direction {
    LEFT = -1,
    RIGHT = 1
};

// States for the Character. (Note: not all will be used).
enum CurrentState {
    DEAD_STATE = 0,
    ATTACK_STATE = 1,
    HURT_STATE = 2,
    IDLE_STATE = 3,
    JUMP_STATE = 4,
    RUN_STATE = 5
};

// Determines whether an animation is ran once or repeats.
enum AnimationType {
    REPEATING,
    ONESHOT
};

// Represents an Animation.
struct Animation {
    int firstFrame; // First Frame of Animation.
    int lastFrame; // Last Frame of Animation.
    int currentFrame; // Current Frame Displayed.
    int offset; // Frame Offset (if needed).
    float speed; // Speed of Animation.
    float timeLeft; // Time Left for next frame.
    AnimationType type; // Type of Animation.
};

class Samurai {
private:
    Rectangle rect; // Character's rectangle for position and size.
    Vector2 velocity; // Velocity of the character for movement.
    Direction direction; // Current facing direction of the character.
    CurrentState state; // Current state of the character (e.g., idle, attack, etc.).
    std::vector<Animation> animations; // List of animations for different states.
    std::vector<Texture2D> sprites; // List of textures for each state.
    float groundLevel; // The Y-coordinate of the ground level.
    bool isRunning = false; // Flag for checking if the player is running or not.

    // Define Health Variables
    int maxHealth = 100; // Maximum health value.
    int currentHealth = 100; // Current health of the samurai.
    bool isDead = false; // Flag to indicate if the samurai is dead.
    bool wasInAir = false; // Flag to indicate if the character was in the air.

    // Define Sound Varaibles.
    Sound attackSound;
    Sound jumpSound;
    Sound hurtSound;
    Sound runSound;
    Sound deadSound;
    Sound landSound;

    // Collision boxes for different purposes
    std::vector<CollisionBox> collisionBoxes;

    // Helper method to update the animation frame.
    void updateAnimation() {
        Animation& anim = animations[state];
        float deltaTime = GetFrameTime();

        // Prevent animation switch while in the HURT state.
        if (state == HURT_STATE) {
            if (anim.currentFrame == anim.lastFrame) {
                if (currentHealth > 0) {
                    state = IDLE_STATE;  // Switch to idle once hurt animation finishes.
                }
                else {
                    state = DEAD_STATE;  // Switch to dead if health is depleted.
                    isDead = true;
                    PlaySound(deadSound);
                }
            }
        }

        // Update animation frame based on time.
        anim.timeLeft -= deltaTime;
        if (anim.timeLeft <= 0) {
            anim.timeLeft = anim.speed;  // Reset timer.

            // Increment frame or loop back to first frame.
            if (anim.currentFrame < anim.lastFrame) {
                anim.currentFrame++;
            } else {
                if (anim.type == REPEATING) {
                    anim.currentFrame = anim.firstFrame;  // Loop animation.
                } else if (state == ATTACK_STATE) {
                    state = IDLE_STATE;  // Return to idle after attack.
                }
            }
        }
    }

    // Helper method to get the current animation frame rectangle.
    Rectangle getAnimationFrame() const {
        const Animation &anim = animations[state];
        // Calculate Frame Dimensions.
        int frameWidth = sprites[state].width / (anim.lastFrame + 1);
        int frameHeight = sprites[state].height;

        return (Rectangle){ // Return Rectangle for the current frame.
            (float)(frameWidth * anim.currentFrame),
            0,
            (float)frameWidth,
            (float)frameHeight
        };
    }

    // Helper method to handle movement input.
    void move() {
        // Reset velocity if not jumping.
        if (rect.y >= groundLevel) {
            velocity.y = 0;
            rect.y = groundLevel;  // Ensure character is on ground.
        }

        // Check for jump input.
        if (IsKeyPressed(KEY_SPACE) && rect.y >= groundLevel) {
            velocity.y = -10.0f;  // Apply upward velocity.
            PlaySound(jumpSound);
            wasInAir = true;
        }

        // Apply gravity.
        if (rect.y < groundLevel) {
            velocity.y += 0.5f;  // Gravity effect.
        } else if (wasInAir) {
            PlaySound(landSound);
            wasInAir = false;
        }

        // Check for horizontal movement input.
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            velocity.x = -5.0f;  // Move left.
            direction = LEFT;
            if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;
            }
            isRunning = true;
        } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            velocity.x = 5.0f;  // Move right.
            direction = RIGHT;
            if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;
            }
            isRunning = true;
        } else {
            velocity.x = 0;  // Stop horizontal movement.
            if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = IDLE_STATE;
            }
            isRunning = false;
        }

        // Check for attack input.
        if (IsKeyPressed(KEY_J)) {
            if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = ATTACK_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame.
                PlaySound(attackSound);
            }
        }

        // Apply velocity to position.
        applyVelocity();
    }

    // Helper method to apply velocity to position.
    void applyVelocity() {
        rect.x += velocity.x;  // Update horizontal position.
        rect.y += velocity.y;  // Update vertical position.

        // Ensure character stays within screen bounds.
        if (rect.x < 0) rect.x = 0;
        if (rect.x > GetScreenWidth() - rect.width) rect.x = GetScreenWidth() - rect.width;
    }

    // Helper method to handle taking damage.
    void heal(int healingAmount) {
        currentHealth += healingAmount;
        if (currentHealth > maxHealth) {
            currentHealth = maxHealth;  // Cap health at maximum.
        }
    }

    // Helper method to check for healing input.
    void checkForHealing() {
        if (IsKeyPressed(KEY_H)) {
            heal(10);  // Heal by 10 points.
        }
    }

    // Helper method to check for damage input (for testing).
    void checkForDamage() {
        if (IsKeyPressed(KEY_K)) {
            takeDamage(10);  // Take 10 damage.
        }
    }

    // Helper method to update collision boxes based on character state.
    void updateCollisionBoxes() {
        // Define scaled offsets and dimensions
        float bodyOffsetX = 16.0f * SPRITE_SCALE;
        float bodyOffsetY = 16.0f * SPRITE_SCALE;
        float bodyWidth = rect.width - (32.0f * SPRITE_SCALE);
        float bodyHeight = rect.height - (16.0f * SPRITE_SCALE);
        
        float attackOffsetX = rect.width - (16.0f * SPRITE_SCALE);
        float attackOffsetY = 24.0f * SPRITE_SCALE;
        float attackSize = 32.0f * SPRITE_SCALE;
        
        float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
        float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
        float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
        float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
        
        for (auto& box : collisionBoxes) {
            if (box.type == BODY) {
                // Update body collision box position based on character position
                box.rect.x = rect.x + bodyOffsetX;
                box.rect.y = rect.y + bodyOffsetY;
                box.rect.width = bodyWidth;
                box.rect.height = bodyHeight;
            } else if (box.type == ATTACK) {
                // Update attack collision box position based on character position and direction
                if (direction == RIGHT) {
                    box.rect.x = rect.x + attackOffsetX;
                } else {
                    box.rect.x = rect.x - attackSize;
                }
                box.rect.y = rect.y + attackOffsetY;
                box.rect.width = attackSize;
                box.rect.height = attackSize;
                
                // Activate attack box only during attack animation
                if (state == ATTACK_STATE && animations[state].currentFrame >= 2 && animations[state].currentFrame <= 4) {
                    box.active = true;
                } else {
                    box.active = false;
                }
            } else if (box.type == HURTBOX) {
                // Update hurtbox position based on character position
                box.rect.x = rect.x + hurtboxOffsetX;
                box.rect.y = rect.y + hurtboxOffsetY;
                box.rect.width = hurtboxWidth;
                box.rect.height = hurtboxHeight;
                
                // Deactivate hurtbox during certain frames of hurt animation
                if (state == HURT_STATE && animations[state].currentFrame >= 1) {
                    box.active = false;
                } else if (state != DEAD_STATE) {
                    box.active = true;
                } else {
                    box.active = false;
                }
            }
        }
    }

public:
    // Constructor initializing the Samurai's properties and animations
    Samurai(float x, float y, float groundLevel) {
        rect = (Rectangle){x, y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE}; // Scale the sprite size
        velocity = (Vector2){0.0f, 0.0f}; // Initialize velocity.
        direction = RIGHT; // Default facing direction.
        state = IDLE_STATE; // Start in idle state.
        this->groundLevel = groundLevel; // Set initial ground level.

        // Initialize animations for different states.
        animations = {
            {0, 3, 0, 0, 0.2f, 0.2f, REPEATING}, // DEAD_STATE
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT},   // ATTACK_STATE
            {0, 2, 0, 0, 0.2f, 0.2f, ONESHOT},   // HURT_STATE
            {0, 5, 0, 0, 0.2f, 0.2f, REPEATING}, // IDLE_STATE
            {0, 5, 0, 0, 0.2f, 0.2f, ONESHOT},   // JUMP_STATE
            {0, 7, 0, 0, 0.1f, 0.1f, REPEATING}  // RUN_STATE
        };

        // Load textures for each state.
        sprites = {
            LoadTexture("assets/Samurai/Dead.png"),
            LoadTexture("assets/Samurai/Attack_1.png"),
            LoadTexture("assets/Samurai/Hurt.png"),
            LoadTexture("assets/Samurai/Idle.png"),
            LoadTexture("assets/Samurai/Jump.png"),
            LoadTexture("assets/Samurai/Run.png")
        };

        // Load sounds
        attackSound = LoadSound("assets/sounds/attack.wav");
        jumpSound = LoadSound("assets/sounds/jump.wav");
        hurtSound = LoadSound("assets/sounds/hurt.wav");
        runSound = LoadSound("assets/sounds/run.wav");
        deadSound = LoadSound("assets/sounds/dead.wav");
        landSound = LoadSound("assets/sounds/land.wav");

        // Initialize collision boxes with scaled dimensions
        float bodyOffsetX = 16.0f * SPRITE_SCALE;
        float bodyOffsetY = 16.0f * SPRITE_SCALE;
        float bodyWidth = rect.width - (32.0f * SPRITE_SCALE);
        float bodyHeight = rect.height - (16.0f * SPRITE_SCALE);
        
        float attackOffsetX = rect.width - (16.0f * SPRITE_SCALE);
        float attackOffsetY = 24.0f * SPRITE_SCALE;
        float attackSize = 32.0f * SPRITE_SCALE;
        
        float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
        float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
        float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
        float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
        
        collisionBoxes = {
            CollisionBox({rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight}, BODY),
            CollisionBox({rect.x + attackOffsetX, rect.y + attackOffsetY, attackSize, attackSize}, ATTACK, false),
            CollisionBox({rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight}, HURTBOX)
        };
    }

    // Destructor to clean up resources
    ~Samurai() {
        for (auto& sprite : sprites) {
            UnloadTexture(sprite); // Unload textures from memory when done.
        }
        UnloadSound(attackSound);
        UnloadSound(jumpSound);
        UnloadSound(hurtSound);
        UnloadSound(runSound);
        UnloadSound(deadSound);
        UnloadSound(landSound);
    }

    // Draw the Samurai on the screen
    void draw() const {
        Rectangle source = getAnimationFrame(); // Get the current animation frame.
        Rectangle dest = {rect.x, rect.y, rect.width, rect.height}; // Destination rectangle.
        Vector2 origin = {0, 0}; // Origin for rotation and scaling.
        float rotation = 0.0f; // No rotation.

        // Draw the sprite with appropriate flipping based on direction.
        if (direction == RIGHT) {
            DrawTexturePro(sprites[state], source, dest, origin, rotation, WHITE);
        } else {
            // Flip horizontally for left direction.
            Rectangle flippedSource = {source.x + source.width, source.y, -source.width, source.height};
            DrawTexturePro(sprites[state], flippedSource, dest, origin, rotation, WHITE);
        }

        // Draw collision boxes for debugging
        for (const auto& box : collisionBoxes) {
            if (box.active) {
                Color color;
                switch (box.type) {
                    case BODY: color = BLUE; break;
                    case ATTACK: color = RED; break;
                    case HURTBOX: color = GREEN; break;
                }
                DrawRectangleLines(box.rect.x, box.rect.y, box.rect.width, box.rect.height, color);
            }
        }
    }

    // Draw the health bar for the Samurai
    void drawHealthBar() {
        float healthPercentage = (float)currentHealth / maxHealth;
        DrawRectangle(10, 10, 200, 20, GRAY); // Background of health bar.
        DrawRectangle(10, 10, 200 * healthPercentage, 20, RED); // Filled portion of health bar.
        DrawRectangleLines(10, 10, 200, 20, BLACK); // Border of health bar.
    }

    // Update the Samurai's state and position
    void updateSamurai() {
        if (!isDead) {
            move(); // Handle movement input.
            updateAnimation(); // Update animation frame.
            updateCollisionBoxes(); // Update collision boxes.
        }
        checkForHealing(); // Check if the healing key is pressed.
        checkForDamage(); // Check if the damage key is pressed.
    }

    // Get the Samurai's rectangle for collision detection
    Rectangle getRect() const {
        return rect;
    }

    // Get the Samurai's current health
    int getHealth() const {
        return currentHealth;
    }

    // Check if the Samurai is dead
    bool getIsDead() const {
        return isDead;
    }

    // Get a collision box of a specific type
    CollisionBox* getCollisionBox(CollisionBoxType type) {
        for (auto& box : collisionBoxes) {
            if (box.type == type) {
                return &box;
            }
        }
        return nullptr;
    }

    // Take damage when hit by an enemy
    void takeDamage(int damage) {
        if (!isDead && state != HURT_STATE) {
            currentHealth -= damage;
            if (currentHealth <= 0) {
                currentHealth = 0;
                state = DEAD_STATE;
                isDead = true;
                PlaySound(deadSound);
            } else {
                state = HURT_STATE;
                animations[state].currentFrame = 0;  // Reset hurt animation.
                PlaySound(hurtSound);
            }
        }
    }
};

#endif // SAMURAI_H