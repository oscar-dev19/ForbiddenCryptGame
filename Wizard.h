#pragma once

#include "raylib.h"
#include "CollisionSystem.h"
#include "CharacterAI.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const float GRAVITY_WIZARD = 800.0f;
const float JUMP_FORCE_WIZARD = -400.0f;
const float GROUND_LEVEL_WIZARD = 400.0f;

enum DirectionWizard {
    LEFT_WIZARD = -1,
    RIGHT_WIZARD = 1
};

enum CurrentStateWizard {
    DEAD_WIZARD = 0,
    ATTACK1_WIZARD = 1,
    ATTACK2_WIZARD = 2,
    HURT_WIZARD = 3,
    IDLE_WIZARD = 4,
    JUMP_WIZARD = 5,
    RUN_WIZARD = 6
};

enum AnimationTypeWizard {
    REPEATING_WIZARD,
    ONESHOT_WIZARD
};

// Fix: Use struct instead of enum
struct AnimationWizard {
    int firstFrame, lastFrame, currentFrame, offset;
    float speed, timeLeft;
    AnimationTypeWizard type;
};

class Wizard {
    public:
        Rectangle rect;
        Vector2 velocity;
        DirectionWizard direction;
        CurrentStateWizard state;
        bool isOnGround = true;
        bool isDead = false;

        std::vector<AnimationWizard> animations;
        std::vector<Texture2D> sprites;
        
        // Sound variables
        Sound attackSound;
        Sound hurtSound;
        Sound deadSound;

        bool isAttacking = false;
        bool hasFinishedAttack = true;
        
        // AI system
        CharacterAI ai;
        float attackRange = 200.0f;  // Wizard has longer range
        float chaseRange = 350.0f;
        float retreatRange = 80.0f;  // Distance at which wizard will retreat
        float moveSpeed = 1.0f;
        
        // Collision boxes for different purposes
        std::vector<CollisionBox> collisionBoxes;

        Wizard(Vector2 position) {
            rect = { position.x, position.y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE };
            velocity = { 0, 0 };
            direction = RIGHT_WIZARD;
            state = IDLE_WIZARD;

            // Initialize animations for different states with correct frame counts
            animations = {
                { 0, 6, 0, 0, 0.2f, 0.2f, ONESHOT_WIZARD },   // DEAD_WIZARD - 7 frames
                { 0, 7, 0, 0, 0.1f, 0.1f, ONESHOT_WIZARD },   // ATTACK1_WIZARD - 8 frames
                { 0, 7, 0, 0, 0.1f, 0.1f, ONESHOT_WIZARD },   // ATTACK2_WIZARD - 8 frames
                { 0, 2, 0, 0, 0.2f, 0.2f, ONESHOT_WIZARD },   // HURT_WIZARD - 3 frames
                { 0, 7, 0, 0, 0.2f, 0.2f, REPEATING_WIZARD }, // IDLE_WIZARD - 8 frames
                { 0, 1, 0, 0, 0.2f, 0.2f, ONESHOT_WIZARD },   // JUMP_WIZARD - 2 frames
                { 0, 7, 0, 0, 0.1f, 0.1f, REPEATING_WIZARD }  // RUN_WIZARD - 8 frames
            };

            // Load sounds
            attackSound = LoadSound("2DGame/assets/sounds/wizard/magic-strike-5856.mp3");
            hurtSound = LoadSound("2DGame/assets/sounds/samurai/female-hurt-2-94301.wav"); // Using samurai hurt sound as fallback
            deadSound = LoadSound("2DGame/assets/sounds/samurai/female-death.wav"); // Using samurai death sound as fallback
            
            // Set sound volume
            SetSoundVolume(attackSound, 0.7f);
            SetSoundVolume(hurtSound, 0.7f);
            SetSoundVolume(deadSound, 0.7f);

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
            
            // Initialize AI with a defensive behavior - wizard keeps distance
            ai.setBehavior(std::make_unique<DefensiveBehavior>(retreatRange, attackRange));
        }

        ~Wizard() {
            // Unload sounds
            UnloadSound(attackSound);
            UnloadSound(hurtSound);
            UnloadSound(deadSound);
            
            // Unload textures
            for (auto& sprite : sprites) {
                if (sprite.id != 0) {
                    UnloadTexture(sprite);
                }
            }
        }

        void loadTextures() {
            // Clear any existing textures
            for (auto& sprite : sprites) {
                if (sprite.id != 0) {
                    UnloadTexture(sprite);
                }
            }
            
            printf("Loading Wizard textures...\n");
            
            // Get current working directory
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                printf("Error getting current working directory\n");
                return;
            }
            
            // Try different path formats
            const char* pathFormats[] = {
                "%s/2DGame/assets/Wizard/Sprites/%s",    // Absolute with 2DGame prefix
                "%s/assets/Wizard/Sprites/%s",           // Absolute without 2DGame prefix
                "2DGame/assets/Wizard/Sprites/%s",       // Relative with 2DGame prefix
                "assets/Wizard/Sprites/%s"               // Relative without 2DGame prefix
            };
            
            // Resize sprites vector
            sprites.resize(7);
            
            // List of filenames to load
            const char* fileNames[] = {
                "Death.png", "Attack1.png", "Attack2.png", "Take hit.png", 
                "Idle.png", "Jump.png", "Run.png"
            };
            
            // Corresponding sprite indices
            int spriteIndices[] = {
                DEAD_WIZARD, ATTACK1_WIZARD, ATTACK2_WIZARD, HURT_WIZARD,
                IDLE_WIZARD, JUMP_WIZARD, RUN_WIZARD
            };
            
            int loadedCount = 0;
            
            // Try to load each texture with different path formats
            for (int i = 0; i < 7; i++) {
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
            printf("Wizard textures loaded: %d/7\n", loadedCount);
        }

        void updateAnimation() {
            // Safety check for valid state
            if (state < 0 || state >= animations.size()) {
                state = IDLE_WIZARD; // Reset to idle if state is invalid
            }
            
            AnimationWizard& anim = animations[state];
            float deltaTime = GetFrameTime();

            anim.timeLeft -= deltaTime;
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;

                if (anim.currentFrame < anim.lastFrame) {
                    anim.currentFrame++;
                } else {
                    if (anim.type == REPEATING_WIZARD) {
                        anim.currentFrame = anim.firstFrame;
                    } else if (state == ATTACK1_WIZARD || state == ATTACK2_WIZARD) {
                        state = IDLE_WIZARD;
                        isAttacking = false;
                        hasFinishedAttack = true;
                    } else if (state == HURT_WIZARD) {
                        state = IDLE_WIZARD;
                    } else if (state == DEAD_WIZARD) {
                        // Stay on the last frame if dead
                        anim.currentFrame = anim.lastFrame;
                    }
                }
            }
        }

        Rectangle getAnimationFrame() const {
            // Safety check for valid state
            if (state < 0 || state >= sprites.size() || state >= animations.size()) {
                return Rectangle{0, 0, 250, 250}; // Return a default frame
            }
            
            const AnimationWizard& anim = animations[state];
            
            // Safety check for valid sprite
            if (sprites[state].id == 0 || sprites[state].width <= 0 || sprites[state].height <= 0) {
                return Rectangle{0, 0, 250, 250}; // Return a default frame
            }
            
            int frameWidth = sprites[state].width / (anim.lastFrame + 1);
            int frameHeight = sprites[state].height;
            
            // Safety check for valid frame dimensions
            if (frameWidth <= 0 || frameHeight <= 0) {
                return Rectangle{0, 0, 250, 250}; // Return a default frame
            }
            
            // Safety check for valid current frame
            if (anim.currentFrame < 0 || anim.currentFrame > anim.lastFrame) {
                return Rectangle{0, 0, (float)frameWidth, (float)frameHeight}; // Return the first frame
            }
    
            return { (float)frameWidth * anim.currentFrame, 0, (float)frameWidth, (float)frameHeight };
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

            if (direction == RIGHT_WIZARD) {
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
                            default: color = YELLOW; break;
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
                    direction = (direction == RIGHT_WIZARD) ? LEFT_WIZARD : RIGHT_WIZARD;
                }

                if (GetRandomValue(0, 100) < 1) {
                    // Choose a random attack
                    int attackType = GetRandomValue(1, 2);
                    switch (attackType) {
                        case 1: state = ATTACK1_WIZARD; break;
                        case 2: state = ATTACK2_WIZARD; break;
                    }
                    isAttacking = true;
                    hasFinishedAttack = false;
                    animations[state].currentFrame = 0;
                    
                    // Play attack sound
                    if (!IsSoundPlaying(attackSound)) {
                        PlaySound(attackSound);
                    }
                } else {
                    // Move in the current direction
                    velocity.x = direction * moveSpeed;
                    state = RUN_WIZARD;
                }
            } else if (isDead) {
                velocity.x = 0;  // Stop moving when dead
                state = DEAD_WIZARD;
            } else {
                velocity.x = 0;  // Stop moving while attacking
            }

            // Apply velocity
            applyVelocity();
        }

        void applyVelocity() {
            rect.x += velocity.x;
            
            // Always keep wizard at the floor level to match other enemies
            // Use the same constant as all other enemies
            const float floorLevel = 380.0f; // Same as in main.cpp
            rect.y = floorLevel - rect.height;
            velocity.y = 0;
            isOnGround = true;

            // Use map boundaries instead of screen bounds
            // Map dimensions are 128 tiles * 16 pixels = 2048 pixels wide
            const float mapWidth = 128 * 16;
            
            if (rect.x < 0) {
                rect.x = 0;
                direction = RIGHT_WIZARD;
            }
            if (rect.x > mapWidth - rect.width) {
                rect.x = mapWidth - rect.width;
                direction = LEFT_WIZARD;
            }
        }

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
                // Update position of body collision box
                if (box.type == BODY) {
                    box.rect.x = rect.x + bodyOffsetX;
                    box.rect.y = rect.y + bodyOffsetY;
                    box.rect.width = bodyWidth;
                    box.rect.height = bodyHeight;
                }
                // Update position of attack collision box
                else if (box.type == ATTACK) {
                    // Only activate attack box during attack animation frames 2-4
                    box.active = ((state == ATTACK1_WIZARD || state == ATTACK2_WIZARD) && 
                                 animations[state].currentFrame >= 2 && 
                                 animations[state].currentFrame <= 4);
                    
                    if (direction == RIGHT_WIZARD) {
                        box.rect.x = rect.x + attackOffsetX;
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
            if (state != DEAD_WIZARD) {
                state = HURT_WIZARD;
                animations[state].currentFrame = 0;
                animations[state].timeLeft = animations[state].speed;
                
                // Play hurt sound if valid
                if (hurtSound.frameCount > 0) {
                    PlaySound(hurtSound);
                }
                
                // Deactivate attack boxes when hurt
                for (auto& box : collisionBoxes) {
                    if (box.type == ATTACK || box.type == HURTBOX) {
                        box.active = false;
                    }
                }
            }
        }
        
        void die() {
            if (state != DEAD_WIZARD) {
                state = DEAD_WIZARD;
                animations[state].currentFrame = 0;
                animations[state].timeLeft = animations[state].speed;
                isDead = true;
                
                // Play death sound if valid
                if (deadSound.frameCount > 0) {
                    PlaySound(deadSound);
                }
                
                // Deactivate all collision boxes when dead
                for (auto& box : collisionBoxes) {
                    box.active = false;
                }
            }
        }
        
        void attack() {
            if (state != ATTACK1_WIZARD && state != ATTACK2_WIZARD && state != DEAD_WIZARD) {
                state = ATTACK1_WIZARD;
                animations[state].currentFrame = 0;
                animations[state].timeLeft = animations[state].speed;
                
                // Activate attack box
                for (auto& box : collisionBoxes) {
                    if (box.type == ATTACK) {
                        box.active = true;
                        
                        // Play attack sound if valid
                        if (attackSound.frameCount > 0) {
                            PlaySound(attackSound);
                        }
                        break;
                    }
                }
            }
        }

        void update(Vector2 targetPos) {
            if (isDead) {
                state = DEAD_WIZARD;
                updateAnimation();
                return;
            }

            // Calculate distance to target
            Vector2 wizardCenter = {rect.x + rect.width/2, rect.y + rect.height/2};
            float distance = Vector2Distance(wizardCenter, targetPos);
            
            // Define optimal distance range for the wizard (they prefer to keep distance)
            float optimalMinDistance = 150.0f;
            float optimalMaxDistance = 200.0f;
            
            // Update state based on AI behavior
            if (!isAttacking && hasFinishedAttack) {
                // Update direction based on target position
                if (targetPos.x < wizardCenter.x) {
                    direction = LEFT_WIZARD;
                } else {
                    direction = RIGHT_WIZARD;
                }
                
                // Determine AI state
                AIState aiState;
                
                if (distance <= retreatRange) {
                    // Too close, need to retreat
                    aiState = AIState::RETREAT;
                } else if (distance >= optimalMinDistance && distance <= attackRange) {
                    // In attack range and not too close - optimal position for attacking
                    aiState = AIState::ATTACK;
                } else if (distance < optimalMinDistance) {
                    // Too close for comfort but not in immediate retreat range
                    aiState = AIState::RETREAT;
                } else if (distance > attackRange && distance <= chaseRange) {
                    // Too far to attack, need to get closer
                    aiState = AIState::CHASE;
                } else {
                    // Too far, stay idle
                    aiState = AIState::IDLE;
                }
                
                // Act based on state
                switch (aiState) {
                    case AIState::RETREAT: {
                        // Move away from target
                        state = RUN_WIZARD;
                        Vector2 directionVector = Vector2Normalize(Vector2Subtract(wizardCenter, targetPos));
                        velocity.x = directionVector.x * moveSpeed;
                        break;
                    }
                    
                    case AIState::ATTACK: {
                        // Attack the target, but stop moving
                        velocity.x = 0;
                        int attackType = GetRandomValue(1, 2);
                        switch (attackType) {
                            case 1: state = ATTACK1_WIZARD; break;
                            case 2: state = ATTACK2_WIZARD; break;
                        }
                        isAttacking = true;
                        hasFinishedAttack = false;
                        animations[state].currentFrame = 0;
                        
                        // Play attack sound
                        if (!IsSoundPlaying(attackSound)) {
                            PlaySound(attackSound);
                        }
                        break;
                    }
                    
                    case AIState::CHASE: {
                        // Move toward target, but stop if we reach optimal distance
                        if (distance > optimalMaxDistance) {
                            state = RUN_WIZARD;
                            Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, wizardCenter));
                            velocity.x = directionVector.x * moveSpeed;
                        } else {
                            // We're at a good distance, stop moving
                            state = IDLE_WIZARD;
                            velocity.x = 0;
                        }
                        break;
                    }
                    
                    case AIState::IDLE:
                    default: {
                        // Stand still
                        state = IDLE_WIZARD;
                        velocity.x = 0;
                        break;
                    }
                }
            }
            
            // Apply velocity
            applyVelocity();
            updateAnimation();
            updateCollisionBoxes();
        }

        void update() {
            if (!isDead) {
                move();  // Fall back to random movement
                updateAnimation();
                updateCollisionBoxes();
            } else {
                state = DEAD_WIZARD;
                updateAnimation(); // Continue death animation
            }
        }

        CollisionBox* getCollisionBox(CollisionBoxType type) {
            for (auto& box : collisionBoxes) {
                if (box.type == type) {
                    return &box;
                }
            }
            return nullptr;
        }
};