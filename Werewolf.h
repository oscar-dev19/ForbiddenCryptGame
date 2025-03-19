#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "raylib.h"
#include "CollisionSystem.h"
#include "CharacterAI.h"

// Constants for physics
const float GRAVITY = 800.0f;
const float JUMP_FORCE = -400.0f;
const float GROUND_LEVEL = 400.0f; // Updated to match the floor level

enum DirectionWolf {
    LEFT_WOLF = -1,
    RIGHT_WOLF = 1
};

enum CurrentStateWolf {
    DEAD_WOLF = 0,
    ATTACK_SWIPE,
    ATTACK_RUN,
    HURT_WOLF,
    IDLE_WOLF,
    JUMP_WOLF,
    RUN_WOLF,
    WALK_WOLF,
    RUN_ATTACK_WOLF
};

enum AnimationTypeWolf {
    REPEATING_WOLF,
    ONESHOT_WOLF
};

struct AnimationWolf {
    int firstFrame, lastFrame, currentFrame, offset;
    float speed, timeLeft;
    AnimationTypeWolf type;
};

class Werewolf {
public:
    Rectangle rect;
    Vector2 velocity;
    DirectionWolf direction;
    CurrentStateWolf state;
    bool isOnGround = true;
    bool isDead = false;

    std::vector<AnimationWolf> animations;
    std::vector<Texture2D> sprites;

    bool isAttacking = false;
    bool hasFinishedAttack = true;
    
    // AI system
    CharacterAI ai;
    float attackRange = 120.0f;
    float chaseRange = 400.0f;
    float moveSpeed = 1.5f;
    
    // Collision boxes for different purposes
    std::vector<CollisionBox> collisionBoxes;

    float groundLevel;

    Werewolf(float x, float y, float groundLevel) {
        rect = (Rectangle){x, y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE}; // Scale the sprite size
        velocity = (Vector2){0.0f, 0.0f}; // Initialize velocity.
        direction = LEFT_WOLF; // Default facing direction.
        state = IDLE_WOLF; // Start in idle state.
        this->groundLevel = groundLevel; // Set initial ground level.

        // Initialize animations for different states with correct frame counts
        animations.resize(9); // Resize to include RUN_ATTACK_WOLF
        animations = {
            {0, 3, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF},    // DEAD_WOLF - 4 frames in Dead.png
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF},    // ATTACK_SWIPE - 6 frames in Attack_1.png
            {0, 3, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF},    // ATTACK_RUN - 4 frames in Attack_2.png
            {0, 2, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF},    // HURT_WOLF - 3 frames in Hurt.png
            {0, 3, 0, 0, 0.1f, 0.1f, REPEATING_WOLF},  // IDLE_WOLF - 4 frames in Idle.png
            {0, 3, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF},    // JUMP_WOLF - 4 frames in Jump.png
            {0, 5, 0, 0, 0.1f, 0.1f, REPEATING_WOLF},  // RUN_WOLF - 6 frames in Run.png
            {0, 7, 0, 0, 0.1f, 0.1f, REPEATING_WOLF},  // WALK_WOLF - 8 frames in walk.png
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF}     // RUN_ATTACK_WOLF - 6 frames in Run+Attack.png
        };

        // Initialize collision boxes with appropriate dimensions
        float bodyOffsetX = 16.0f * SPRITE_SCALE;
        float bodyOffsetY = 16.0f * SPRITE_SCALE;
        float bodyWidth = rect.width - (32.0f * SPRITE_SCALE);
        float bodyHeight = rect.height - (16.0f * SPRITE_SCALE);
        
        float attackOffsetY = 24.0f * SPRITE_SCALE;
        float attackSize = 32.0f * SPRITE_SCALE;
        
        float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
        float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
        float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
        float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
        
        collisionBoxes = {
            CollisionBox({rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight}, BODY),
            CollisionBox({rect.x + rect.width, rect.y + attackOffsetY, attackSize, attackSize}, ATTACK, false),
            CollisionBox({rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight}, HURTBOX)
        };
        
        // Initialize AI with an aggressive behavior
        ai.setBehavior(std::make_unique<AggressiveBehavior>(attackRange, chaseRange));
    }

    ~Werewolf() {
        for (auto& sprite : sprites) {
            UnloadTexture(sprite);
        }
    }

    void loadTextures() {
        // Clear any existing textures
        for (auto& sprite : sprites) {
            if (sprite.id != 0) {
                UnloadTexture(sprite);
            }
        }
        
        printf("Loading Werewolf textures...\n");
        
        // Get current working directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            printf("Error getting current working directory\n");
            return;
        }
        
        // Try different path formats
        const char* pathFormats[] = {
            "%s/2DGame/assets/Werewolf/%s",    // Absolute with 2DGame prefix
            "%s/assets/Werewolf/%s",           // Absolute without 2DGame prefix
            "2DGame/assets/Werewolf/%s",       // Relative with 2DGame prefix
            "assets/Werewolf/%s"               // Relative without 2DGame prefix
        };
        
        // Resize sprites vector
        sprites.resize(9);
        
        // List of filenames to load
        const char* fileNames[] = {
            "Dead.png", "Attack_1.png", "Attack_2.png", "Hurt.png", 
            "Idle.png", "Jump.png", "Run.png", "walk.png", "Run+Attack.png"
        };
        
        // Corresponding sprite indices
        int spriteIndices[] = {
            DEAD_WOLF, ATTACK_SWIPE, ATTACK_RUN, HURT_WOLF,
            IDLE_WOLF, JUMP_WOLF, RUN_WOLF, WALK_WOLF, RUN_ATTACK_WOLF
        };
        
        int loadedCount = 0;
        
        // Try to load each texture with different path formats
        for (int i = 0; i < 9; i++) {
            bool loaded = false;
            
            // Try each path format
            for (int p = 0; p < 4 && !loaded; p++) {
                char fullPath[PATH_MAX];
                if (p < 2) {
                    // Absolute paths
                    snprintf(fullPath, sizeof(fullPath), pathFormats[p], cwd, fileNames[i]);
                } else {
                    // Relative paths
                    snprintf(fullPath, sizeof(fullPath), pathFormats[p], fileNames[i]);
                }
                
                // Check if file exists before loading
                if (FileExists(fullPath)) {
                    sprites[spriteIndices[i]] = LoadTexture(fullPath);
                    
                    if (sprites[spriteIndices[i]].id != 0) {
                        printf("Loaded %s: %dx%d\n", 
                               fileNames[i], 
                               sprites[spriteIndices[i]].width, 
                               sprites[spriteIndices[i]].height);
                        loaded = true;
                        loadedCount++;
                        break;
                    }
                }
            }
            
            if (!loaded) {
                printf("Failed to load texture: %s\n", fileNames[i]);
            }
        }
        
        // Print summary
        printf("Werewolf textures loaded: %d/9\n", loadedCount);
    }

    void updateAnimation() {
        // Safety check for valid state
        if (state < 0 || state >= animations.size()) {
            state = IDLE_WOLF; // Reset to idle if state is invalid
        }
        
        AnimationWolf& anim = animations[state];
        float deltaTime = GetFrameTime();

        anim.timeLeft -= deltaTime;
        if (anim.timeLeft <= 0) {
            anim.timeLeft = anim.speed;

            if (anim.currentFrame < anim.lastFrame) {
                anim.currentFrame++;
            } else {
                if (anim.type == REPEATING_WOLF) {
                    anim.currentFrame = anim.firstFrame;
                } else if (state == ATTACK_SWIPE || state == ATTACK_RUN) {
                    state = IDLE_WOLF;
                    isAttacking = false;
                    hasFinishedAttack = true;
                } else if (state == HURT_WOLF) {
                    state = IDLE_WOLF; // Return to idle after hurt animation
                } else if (state == DEAD_WOLF) {
                    // Stay on the last frame if dead
                    anim.currentFrame = anim.lastFrame;
                } else if (state == JUMP_WOLF && isOnGround) {
                    state = IDLE_WOLF; // Return to idle after landing
                }
            }
        }
    }

    Rectangle getAnimationFrame() const {
        Rectangle frame = {0, 0, 0, 0}; // Default empty frame
        
        // Check if state is valid
        if (state < 0 || state >= sprites.size()) {
            static std::vector<bool> reportedInvalidStates(sprites.size() + 1, false);
            if (!reportedInvalidStates[state < 0 ? 0 : state]) {
                printf("Error: Invalid state %d (sprites size: %zu)\n", state, sprites.size());
                reportedInvalidStates[state < 0 ? 0 : state] = true;
            }
            return frame;
        }
        
        // Check if animations vector is properly sized
        if (animations.size() <= state) {
            static std::vector<bool> reportedMissingAnimations(sprites.size(), false);
            if (!reportedMissingAnimations[state]) {
                printf("Error: Animation not defined for state %d (animations size: %zu)\n", state, animations.size());
                reportedMissingAnimations[state] = true;
            }
            return frame;
        }
        
        // Check if sprite is loaded
        if (sprites[state].id == 0) {
            static std::vector<bool> reportedMissingSprites(sprites.size(), false);
            if (!reportedMissingSprites[state]) {
                printf("Error: Sprite for state %d is not loaded (ID: 0)\n", state);
                reportedMissingSprites[state] = true;
            }
            return frame;
        }
        
        // For Werewolf, each sprite is a single frame, not a spritesheet
        // So we return the entire sprite as the frame
        frame.x = 0;
        frame.y = 0;
        frame.width = sprites[state].width;
        frame.height = sprites[state].height;
        
        return frame;
    }

    void draw() const {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size() || sprites[state].id == 0) {
            return; // Don't draw if state is invalid or texture not loaded
        }
        
        Rectangle source = getAnimationFrame();
        Rectangle dest = { rect.x, rect.y, rect.width, rect.height };
        Vector2 origin = { 0, 0 };
        float rotation = 0.0f;

        if (direction == RIGHT_WOLF) {
            DrawTexturePro(sprites[state], source, dest, origin, rotation, WHITE);
        } else {
            Rectangle flippedSource = { source.x + source.width, source.y, -source.width, source.height };
            DrawTexturePro(sprites[state], flippedSource, dest, origin, rotation, WHITE);
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
                        default: color = PURPLE; break;
                    }
                    DrawRectangleLines(box.rect.x, box.rect.y, box.rect.width, box.rect.height, color);
                }
            }
        }
    }

    void move() {
        // Simple AI movement logic
        if (!isAttacking && hasFinishedAttack && !isDead) {
            // Random chance to change direction or attack
            if (GetRandomValue(0, 100) < 2) {
                direction = (direction == RIGHT_WOLF) ? LEFT_WOLF : RIGHT_WOLF;
            }

            if (GetRandomValue(0, 100) < 1) {
                // Choose a random attack
                int attackType = GetRandomValue(1, 2);
                switch (attackType) {
                    case 1: state = ATTACK_SWIPE; break;
                    case 2: state = ATTACK_RUN; break;
                }
                isAttacking = true;
                hasFinishedAttack = false;
                animations[state].currentFrame = 0;
            } else {
                // Move in the current direction
                velocity.x = direction * moveSpeed;  // Werewolf moves faster than goblin
                state = RUN_WOLF;
            }
        } else if (isDead) {
            velocity.x = 0;  // Stop moving when dead
            state = DEAD_WOLF;
        } else {
            velocity.x = 0;  // Stop moving while attacking
        }

        // Apply velocity
        applyVelocity();
    }

    void updateWithTarget(Vector2 targetPos) {
        if (isDead) {
            state = DEAD_WOLF;
            updateAnimation();
            return;
        }

        // Calculate distance to target
        float wolfCenterX = rect.x + rect.width/2;
        float wolfCenterY = rect.y + rect.height/2;
        Vector2 wolfCenter = {wolfCenterX, wolfCenterY};
        float distanceToTarget = Vector2Distance(wolfCenter, targetPos);
        
        // Define a minimum distance to keep from the target
        float minDistance = 70.0f;
        
        // Update state based on AI behavior
        if (!isAttacking && hasFinishedAttack) {
            // Update direction based on target position
            if (targetPos.x < wolfCenter.x) {
                direction = LEFT_WOLF;
            } else {
                direction = RIGHT_WOLF;
            }
            
            if (distanceToTarget <= attackRange) {
                // Attack when in range
                int attackType = GetRandomValue(1, 2);
                switch (attackType) {
                    case 1: state = ATTACK_SWIPE; break;
                    case 2: state = ATTACK_RUN; break;
                }
                isAttacking = true;
                hasFinishedAttack = false;
                animations[state].currentFrame = 0;
                
                // If using running attack, move toward the target
                if (state == ATTACK_RUN) {
                    Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, wolfCenter));
                    velocity.x = directionVector.x * moveSpeed * 2.0f; // Faster during running attack
                }
            }
            else if (distanceToTarget <= chaseRange) {
                // Chase the target but maintain minimum distance
                state = RUN_WOLF;
                
                if (distanceToTarget > minDistance) {
                    // Calculate direction vector towards target
                    Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, wolfCenter));
                    velocity.x = directionVector.x * moveSpeed;
                    
                    // Removed random jumping behavior to keep all enemies on the same level
                    // No more jumping during pursuit to maintain consistent floor level with other enemies
                } else {
                    // Stop if we're already at minimum distance
                    velocity.x = 0;
                    state = IDLE_WOLF;
                }
            }
            else {
                // Idle when out of range
                state = IDLE_WOLF;
                velocity.x = 0;
            }
        }
        
        // Apply gravity
        if (!isOnGround) {
            velocity.y += GRAVITY * GetFrameTime();
        }
        
        // Apply velocity
        applyVelocity();
        updateAnimation();
        updateCollisionBoxes();
    }

    void applyVelocity() {
        rect.x += velocity.x;
        
        // Always keep werewolf at the ground level to ensure consistent positioning with other enemies
        rect.y = groundLevel - rect.height;
        velocity.y = 0;
        isOnGround = true;

        // Use map boundaries instead of screen bounds
        // Map dimensions are 128 tiles * 16 pixels = 2048 pixels wide
        const float mapWidth = 128 * 16;
        
        if (rect.x < 0) {
            rect.x = 0;
            direction = RIGHT_WOLF;
        }
        if (rect.x > mapWidth - rect.width) {
            rect.x = mapWidth - rect.width;
            direction = LEFT_WOLF;
        }
        
        // Check if werewolf has landed on the ground - no longer needed as we always set position to ground level
        // if (rect.y >= groundLevel - rect.height) {
        //    rect.y = groundLevel - rect.height;
        //    velocity.y = 0;
        //    isOnGround = true;
        // }
    }

    void updateCollisionBoxes() {
        // Safety check for empty collision boxes
        if (collisionBoxes.empty()) {
            return;
        }
        
        // Define scaled offsets and dimensions
        float bodyOffsetX = 16.0f * SPRITE_SCALE;
        float bodyOffsetY = 16.0f * SPRITE_SCALE;
        float bodyWidth = rect.width - (32.0f * SPRITE_SCALE);
        float bodyHeight = rect.height - (16.0f * SPRITE_SCALE);
        
        float attackOffsetY = 24.0f * SPRITE_SCALE;
        float attackSize = 32.0f * SPRITE_SCALE;
        
        float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
        float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
        float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
        float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
        
        for (auto& box : collisionBoxes) {
            // Update position of body collision box
            if (box.type == BODY) {
                box.rect.x = rect.x + bodyOffsetX;
                box.rect.y = rect.y + bodyOffsetY;
                box.rect.width = bodyWidth;
                box.rect.height = bodyHeight;
            }
            // Update position of attack collision box
            else if (box.type == ATTACK) {
                // Safety check for valid animation state
                if (state < 0 || state >= animations.size()) {
                    box.active = false;
                    continue;
                }
                
                // Only activate attack box during attack animation
                box.active = (state == ATTACK_SWIPE && animations[state].currentFrame >= 1 && animations[state].currentFrame <= 3);
                
                if (direction == RIGHT_WOLF) {
                    box.rect.x = rect.x + rect.width - attackSize;
                    box.rect.y = rect.y + attackOffsetY;
                    box.rect.width = attackSize;
                    box.rect.height = attackSize;
                } else {
                    box.rect.x = rect.x - attackSize;
                    box.rect.y = rect.y + attackOffsetY;
                    box.rect.width = attackSize;
                    box.rect.height = attackSize;
                }
            }
            // Update position of hurtbox
            else if (box.type == HURTBOX) {
                box.rect.x = rect.x + hurtboxOffsetX;
                box.rect.y = rect.y + hurtboxOffsetY;
                box.rect.width = hurtboxWidth;
                box.rect.height = hurtboxHeight;
                box.active = !isDead;  // Deactivate hurtbox when dead
            }
        }
    }

    void takeDamage(int damage) {
        if (!isDead) {
            // Set to hurt state temporarily
            state = HURT_WOLF;
            animations[state].currentFrame = 0;
            
            // Random chance to die
            if (GetRandomValue(0, 100) < 20) {
                state = DEAD_WOLF;
                isDead = true;
                animations[state].currentFrame = 0;
                velocity.x = 0;
                
                // Deactivate collision boxes
                for (auto& box : collisionBoxes) {
                    if (box.type == ATTACK || box.type == HURTBOX) {
                        box.active = false;
                    }
                }
            }
        }
    }

    void update() {
        if (!isDead) {
            // Apply gravity
            if (!isOnGround) {
                velocity.y += GRAVITY * GetFrameTime();
            }
            
            move();  // Fall back to random movement if no target is provided
            updateAnimation();
            updateCollisionBoxes();
        } else {
            state = DEAD_WOLF;
            updateAnimation(); // Continue death animation
        }
    }

    CollisionBox* getCollisionBox(CollisionBoxType type) const {
        for (auto& box : collisionBoxes) {
            if (box.type == type) {
                return const_cast<CollisionBox*>(&box);
            }
        }
        return nullptr;
    }
};