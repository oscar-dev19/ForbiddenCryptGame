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

// Animation types.
enum AnimationType {
    LOOP,    // Animation loops continuously.
    ONESHOT  // Animation plays once and stops at the last frame.
};

// Animation data structure.
struct Animation {
    int firstFrame;    // First frame of the animation.
    int lastFrame;     // Last frame of the animation.
    int currentFrame;  // Current frame being displayed.
    float timer;       // Timer for frame changes.
    float frameTime;   // Time per frame.
    float speed;       // Animation speed.
    AnimationType type; // Type of animation (loop or oneshot).
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

    // Define Health Variables
    int maxHealth = 100; // Maximum health value.
    int currentHealth = 100; // Current health of the samurai.
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
    void updateAnimation(float deltaTime) {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size()) {
            state = IDLE_STATE;
        }
        
        Animation &anim = animations[state];
        anim.timer += deltaTime;
        
        // Handle special case for HURT_STATE
        if (state == HURT_STATE && anim.currentFrame >= anim.lastFrame) {
            state = IDLE_STATE;
            animations[state].currentFrame = 0;
            anim.timer = 0;
        }
        
        // Update animation frame based on timer
        if (anim.timer >= anim.frameTime) {
            anim.timer = 0;
            
            // For JUMP_STATE, don't increment frame if we're at the last frame
            if (state == JUMP_STATE && anim.currentFrame >= anim.lastFrame) {
                // Keep the last frame until landing
            } else {
                // Increment frame
                anim.currentFrame++;
                
                // Handle frame looping or state transitions
                if (anim.currentFrame > anim.lastFrame) {
                    if (anim.type == LOOP) {
                        anim.currentFrame = 0;
                    } else if (anim.type == ONESHOT) {
                        anim.currentFrame = anim.lastFrame;
                        
                        // For ATTACK_STATE, transition back to IDLE_STATE when animation completes
                        if (state == ATTACK_STATE) {
                            state = IDLE_STATE;
                            animations[state].currentFrame = 0;
                        }
                    }
                }
            }
        }
    }

    // Helper method to get the current animation frame rectangle.
    Rectangle getAnimationFrame() const {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size() || state >= animations.size()) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Get the sprite dimensions
        int spriteWidth = sprites[state].width;
        
        // Safety check for valid sprite
        if (spriteWidth <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Calculate the number of frames in the sprite sheet
        int framesPerRow = spriteWidth / 128;
        
        // Safety check for valid frames calculation
        if (framesPerRow <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Get the current frame from the animation
        int currentFrame = animations[state].currentFrame;
        
        // Safety check for valid frame
        if (currentFrame < 0) {
            currentFrame = 0; // Reset to first frame
        }
        
        // Ensure current frame doesn't exceed the last frame
        if (currentFrame > animations[state].lastFrame) {
            currentFrame = animations[state].lastFrame;
        }
        
        // Calculate the position of the frame in the sprite sheet
        int frameX = (currentFrame % framesPerRow) * 128;
        int frameY = (currentFrame / framesPerRow) * 128;
        
        return Rectangle{(float)frameX, (float)frameY, 128, 128};
    }

    // Helper method to handle movement input.
    void move() {
        // Reset velocity if not jumping.
        if (rect.y >= groundLevel) {
            velocity.y = 0;
            rect.y = groundLevel;  // Ensure character is on ground.
            
            // Only transition from JUMP_STATE to IDLE_STATE when landing
            if (state == JUMP_STATE) {
                state = IDLE_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame
                if (wasInAir) {
                    if (landSound.frameCount > 0) {
                        PlaySound(landSound);
                    }
                    wasInAir = false;
                }
            }
        }

        // Check for jump input.
        if (IsKeyPressed(KEY_SPACE) && rect.y >= groundLevel) {
            velocity.y = -10.0f;  // Apply upward velocity.
            if (jumpSound.frameCount > 0) {
                PlaySound(jumpSound);
            }
            wasInAir = true;
            
            // Set state to JUMP_STATE and reset animation
            if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = JUMP_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame
            }
        }

        // Apply gravity.
        if (rect.y < groundLevel) {
            velocity.y += 0.5f;  // Gravity effect.
            
            // Maintain JUMP_STATE while in the air unless in special states
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = JUMP_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame
            }
        }
        
        // Handle left/right movement
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            velocity.x = -5.0f;  // Move left.
            direction = LEFT;
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;  // Set to run state.
            }
        } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            velocity.x = 5.0f;  // Move right.
            direction = RIGHT;
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;  // Set to run state.
            }
        } else {
            velocity.x = 0;  // Stop horizontal movement.
            if (state == RUN_STATE) {
                state = IDLE_STATE;  // Return to idle if not running.
            }
        }

        // Check for attack input.
        if (IsKeyPressed(KEY_J) && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
            state = ATTACK_STATE;  // Set to attack state.
            animations[state].currentFrame = 0;  // Reset animation frame.
            if (attackSound.frameCount > 0) {
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

    // Update collision boxes based on current position and state.
    void updateCollisionBoxes() {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size()) {
            return;
        }
        
        // Update body collision box
        if (collisionBoxes.size() > 0) {
            float bodyOffsetX = 16.0f * SPRITE_SCALE;
            float bodyOffsetY = 16.0f * SPRITE_SCALE;
            float bodyWidth = rect.width - (32.0f * SPRITE_SCALE);
            float bodyHeight = rect.height - (16.0f * SPRITE_SCALE);
            
            collisionBoxes[0].rect = {rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight};
            collisionBoxes[0].active = (state != DEAD_STATE); // Deactivate body collision when dead
            
            // Update attack collision box
            if (collisionBoxes.size() > 1) {
                float attackOffsetX = (direction == RIGHT) ? rect.width - (16.0f * SPRITE_SCALE) : 0;
                float attackOffsetY = 24.0f * SPRITE_SCALE;
                float attackSize = 32.0f * SPRITE_SCALE;
                
                collisionBoxes[1].rect = {rect.x + attackOffsetX, rect.y + attackOffsetY, attackSize, attackSize};
                collisionBoxes[1].active = (state == ATTACK_STATE && animations[state].currentFrame >= 2 && animations[state].currentFrame <= 4);
                
                // Update hurtbox collision box
                if (collisionBoxes.size() > 2) {
                    float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
                    float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
                    float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
                    float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
                    
                    collisionBoxes[2].rect = {rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight};
                    collisionBoxes[2].active = (state != DEAD_STATE);
                }
            }
        }
    }

public:
    bool isDead = false; // Flag to indicate if the samurai is dead.

    // Constructor initializing the Samurai's properties and animations
    Samurai(float x, float y, float groundLevel) {
        rect = (Rectangle){x, y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE}; // Scale the sprite size
        velocity = (Vector2){0.0f, 0.0f}; // Initialize velocity.
        direction = RIGHT; // Default facing direction.
        state = IDLE_STATE; // Start in idle state.
        this->groundLevel = groundLevel; // Set initial ground level.
        currentHealth = maxHealth = 100;
        isDead = false;
        wasInAir = false;

        // Load textures for each state.
        sprites = {
            LoadTexture("assets/Samurai/Dead.png"),    // DEAD_STATE
            LoadTexture("assets/Samurai/Attack_1.png"), // ATTACK_STATE
            LoadTexture("assets/Samurai/Hurt.png"),    // HURT_STATE
            LoadTexture("assets/Samurai/Idle.png"),    // IDLE_STATE
            LoadTexture("assets/Samurai/Jump.png"),    // JUMP_STATE
            LoadTexture("assets/Samurai/Run.png")      // RUN_STATE
        };

        // Initialize animations for each state.
        animations = {
            {0, 2, 0, 0, 0.2f, 0.2f, ONESHOT},    // DEAD_STATE - Changed to use all 3 frames (0, 1, 2)
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT},  // ATTACK_STATE
            {0, 2, 0, 0, 0.2f, 0.2f, ONESHOT},  // HURT_STATE
            {0, 5, 0, 0, 0.2f, 0.2f, LOOP},     // IDLE_STATE
            {0, 11, 0, 0, 0.1f, 0.1f, ONESHOT}, // JUMP_STATE - 12 frames (0-11) based on 1536/128 = 12
            {0, 7, 0, 0, 0.1f, 0.1f, LOOP}      // RUN_STATE
        };

        // Load sounds
        attackSound = LoadSound("sounds/samurai/sword-sound-2-36274.wav");
        jumpSound = LoadSound("sounds/samurai/female-jump.wav");
        hurtSound = LoadSound("sounds/samurai/female-hurt-2-94301.wav");
        deadSound = LoadSound("sounds/samurai/female-death.wav");
        landSound = LoadSound("sounds/samurai/land2-43790.wav");
        runSound = LoadSound("sounds/samurai/running-on-concrete-268478.wav");

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
        
        // Unload sounds - make sure they're valid before unloading
        if (attackSound.frameCount > 0) UnloadSound(attackSound);
        if (jumpSound.frameCount > 0) UnloadSound(jumpSound);
        if (hurtSound.frameCount > 0) UnloadSound(hurtSound);
        if (runSound.frameCount > 0) UnloadSound(runSound);
        if (deadSound.frameCount > 0) UnloadSound(deadSound);
        if (landSound.frameCount > 0) UnloadSound(landSound);
    }

    // Draw the character.
    void draw() {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size()) {
            state = IDLE_STATE;
        }
        
        // Get the current animation frame.
        Rectangle frame = getAnimationFrame();
        
        // Draw the sprite.
        if (direction == RIGHT) {
            DrawTexturePro(
                sprites[state],
                frame,
                Rectangle{rect.x, rect.y, rect.width, rect.height},
                Vector2{0, 0},
                0.0f,
                WHITE
            );
        } else {
            // Flip the sprite horizontally for left direction.
            Rectangle flippedFrame = {frame.x + frame.width, frame.y, -frame.width, frame.height};
            DrawTexturePro(
                sprites[state],
                flippedFrame,
                Rectangle{rect.x, rect.y, rect.width, rect.height},
                Vector2{0, 0},
                0.0f,
                WHITE
            );
        }
        
        // Draw collision boxes for debugging
        if (showCollisionBoxes) {
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
        // Always update animation regardless of whether the samurai is dead or alive
        updateAnimation(GetFrameTime());
        
        // Only handle movement if the samurai is alive
        if (!isDead) {
            move();
        }
        
        // Always update collision boxes
        updateCollisionBoxes();
        
        // Always check for healing and damage
        checkForHealing();
        checkForDamage();
    }

    // Get the Samurai's rectangle for collision detection
    Rectangle getRect() const {
        return rect;
    }

    // Get the Samurai's current health
    int getHealth() const {
        return currentHealth;
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
                animations[state].currentFrame = 0;  // Reset death animation to start from the beginning
                animations[state].timer = 0;  // Reset timer for smooth animation
                if (deadSound.frameCount > 0) {
                    PlaySound(deadSound);
                }
            } else {
                state = HURT_STATE;
                animations[state].currentFrame = 0;  // Reset hurt animation.
                if (hurtSound.frameCount > 0) {
                    PlaySound(hurtSound);
                }
            }
        }
    }
};

#endif // SAMURAI_H