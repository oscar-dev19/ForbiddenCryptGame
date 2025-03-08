#ifndef DEMON_H
#define DEMON_H

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>
#include <iostream>
#include <string>
// Remove or comment out the filesystem include if it's not supported
// #include <filesystem>
#include <algorithm>  // For sorting files by name

// Comment out the namespace alias since we won't be using filesystem
// namespace fs = std::filesystem;

// Collision box types for different purposes

enum DirectionDemon {
    LEFT_DEMON = -1,
    RIGHT_DEMON = 1
};

enum CurrentStateDemon {
    IDLE_DEMON = 0,
    WALK_DEMON = 1,
    ATTACK_DEMON = 2,
    HURT_DEMON = 3,
    DEAD_DEMON = 4
};

enum AnimationTypeDemon {
    REPEATING_DEMON,
    ONESHOT_DEMON
};

struct AnimationDemon {
    int firstFrame, lastFrame, currentFrame;
    float speed, timeLeft;
    AnimationTypeDemon type;
};

class Demon {
    public:
        Rectangle rect;
        Vector2 velocity;
        DirectionDemon direction;
        CurrentStateDemon state;
        bool isAttacking = false;
        bool hasFinishedAttack = true;
        bool isDead = false;

        // Health related to damage and death
        int health;

        std::vector<AnimationDemon> animations;
        std::vector<Texture2D> sprites; // The texture sheets used for animations
        
        // Sound variables
        Sound attackSound;
        Sound hurtSound;
        Sound deadSound;
        Sound chantSound;
        float chantTimer = 0.0f;
        float chantInterval = 10.0f; // Play chant every 10 seconds

        // Collision boxes for different purposes
        std::vector<CollisionBox> collisionBoxes;

        // Constructor initializing the Demon's properties and animations
        Demon(Vector2 position) {
            std::cout << "Initializing Demon at position: " << position.x << ", " << position.y << std::endl;
            rect = { position.x, position.y, 144.0f * SPRITE_SCALE, 80.0f * SPRITE_SCALE };
            velocity = { 0, 0 };
            direction = RIGHT_DEMON;
            state = IDLE_DEMON;
            health = 100; // Initialize health

            // Initialize animations for different states with correct frame counts
            animations = {
                { 0, 21, 0, 0.1f, 0.1f, REPEATING_DEMON }, // IDLE_DEMON - 22 frames
                { 0, 7, 0, 0.1f, 0.1f, REPEATING_DEMON },  // WALK_DEMON - 8 frames
                { 0, 11, 0, 0.1f, 0.1f, ONESHOT_DEMON },   // ATTACK_DEMON - 12 frames
                { 0, 5, 0, 0.2f, 0.2f, ONESHOT_DEMON },    // HURT_DEMON - 6 frames
                { 0, 21, 0, 0.2f, 0.2f, ONESHOT_DEMON }    // DEAD_DEMON - 22 frames
            };

            // Load textures for each state
            std::cout << "Loading Demon textures..." << std::endl;
            try {
                Texture2D texture = LoadTexture("assets/Demon/spritesheets/demon_slime_FREE_v1.0_288x160_spritesheet.png");
                if (texture.id == 0) {
                    std::cout << "Error: Failed to load Demon texture" << std::endl;
                    // Create a small placeholder texture to prevent crashes
                    Image placeholder = GenImageColor(288, 160, RED);
                    texture = LoadTextureFromImage(placeholder);
                    UnloadImage(placeholder);
                }
                sprites.push_back(texture);
                std::cout << "Demon texture loaded. Width: " << sprites[0].width << ", Height: " << sprites[0].height << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Exception loading Demon texture: " << e.what() << std::endl;
                // Create a small placeholder texture to prevent crashes
                Image placeholder = GenImageColor(288, 160, RED);
                Texture2D texture = LoadTextureFromImage(placeholder);
                UnloadImage(placeholder);
                sprites.push_back(texture);
            }

            // Initialize collision boxes with scaled dimensions
            float bodyOffsetX = 36.0f * SPRITE_SCALE;
            float bodyOffsetY = 20.0f * SPRITE_SCALE;
            float bodyWidth = rect.width - (72.0f * SPRITE_SCALE);
            float bodyHeight = rect.height - (20.0f * SPRITE_SCALE);
            
            float attackOffsetX = rect.width - (36.0f * SPRITE_SCALE);
            float attackOffsetY = 30.0f * SPRITE_SCALE;
            float attackSize = 72.0f * SPRITE_SCALE;
            float attackHeight = 40.0f * SPRITE_SCALE;
            
            float hurtboxOffsetX = 45.0f * SPRITE_SCALE;
            float hurtboxOffsetY = 25.0f * SPRITE_SCALE;
            float hurtboxWidth = rect.width - (90.0f * SPRITE_SCALE);
            float hurtboxHeight = rect.height - (30.0f * SPRITE_SCALE);
            
            collisionBoxes = {
                CollisionBox({rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight}, BODY),
                CollisionBox({rect.x + attackOffsetX, rect.y + attackOffsetY, attackSize, attackHeight}, ATTACK, false),
                CollisionBox({rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight}, HURTBOX)
            };
            
            std::cout << "Demon collision boxes initialized. Count: " << collisionBoxes.size() << std::endl;

            // Load sounds
            chantSound = LoadSound("sounds/misc/demon-chant-latin-14489.mp3");
            hurtSound = LoadSound("sounds/samurai/female-hurt-2-94301.wav"); // Using samurai hurt sound as fallback
            deadSound = LoadSound("sounds/samurai/female-death.wav"); // Using samurai death sound as fallback
            
            // Set sound volume
            SetSoundVolume(chantSound, 0.7f);
            SetSoundVolume(hurtSound, 0.7f);
            SetSoundVolume(deadSound, 0.7f);
        }

        // Destructor to clean up resources
        ~Demon() {
            for (auto& sprite : sprites) {
                if (sprite.id != 0) {
                    UnloadTexture(sprite);
                }
            }
            
            // Unload sounds if they were loaded
            if (attackSound.frameCount > 0) UnloadSound(attackSound);
            if (hurtSound.frameCount > 0) UnloadSound(hurtSound);
            if (deadSound.frameCount > 0) UnloadSound(deadSound);
            if (chantSound.frameCount > 0) UnloadSound(chantSound);
        }

        void updateAnimation() {
            // Safety check for valid state
            if (state < 0 || state >= animations.size()) {
                state = IDLE_DEMON; // Reset to idle if state is invalid
            }
            
            AnimationDemon& anim = animations[state];
            float deltaTime = GetFrameTime();

            anim.timeLeft -= deltaTime;
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;

                if (anim.currentFrame < anim.lastFrame) {
                    anim.currentFrame++;
                } else {
                    if (anim.type == REPEATING_DEMON) {
                        anim.currentFrame = 0;
                    } else if (state == ATTACK_DEMON) {
                        state = IDLE_DEMON;
                        isAttacking = false;
                        hasFinishedAttack = true;
                    } else if (state == HURT_DEMON) {
                        state = IDLE_DEMON;
                    } else if (state == DEAD_DEMON) {
                        // Stay on the last frame if dead
                        anim.currentFrame = anim.lastFrame;
                    }
                }
            }
        }
        
        Rectangle getAnimationFrame() const {
            // Safety check for valid state
            if (state < 0 || state >= animations.size()) {
                return Rectangle{0.0f, 0.0f, 288.0f, 160.0f}; // Return a default frame
            }
            
            const AnimationDemon& anim = animations[state];
            
            // Safety check for valid sprite
            if (sprites.empty() || sprites[0].id == 0) {
                return Rectangle{0.0f, 0.0f, 288.0f, 160.0f}; // Return a default frame
            }
            
            // Calculate frame dimensions based on sprite sheet layout
            // The demon sprite sheet has 22 columns and 5 rows
            int frameWidth = 288;
            int frameHeight = 160;
            
            // Calculate the row and column for the current frame
            int row = state;
            int col = anim.currentFrame;
            
            // Safety check for valid frame
            if (row < 0 || row >= 5 || col < 0 || col > anim.lastFrame) {
                return Rectangle{0.0f, 0.0f, static_cast<float>(frameWidth), static_cast<float>(frameHeight)}; // Return the first frame
            }
            
            return Rectangle{
                static_cast<float>(col * frameWidth),
                static_cast<float>(row * frameHeight),
                static_cast<float>(frameWidth),
                static_cast<float>(frameHeight)
            };
        }

        void draw() const {
            // Safety check for valid state and sprite
            if (state < 0 || state >= animations.size() || sprites.empty() || sprites[0].id == 0) {
                return; // Don't draw if state is invalid or texture not loaded
            }
            
            Rectangle source = getAnimationFrame();
            Rectangle dest = { rect.x, rect.y, rect.width, rect.height };
            Vector2 origin = { 0, 0 };
            float rotation = 0.0f;

            if (direction == RIGHT_DEMON) {
                DrawTexturePro(sprites[0], source, dest, origin, rotation, WHITE);
            } else {
                Rectangle flippedSource = { source.x + source.width, source.y, -source.width, source.height };
                DrawTexturePro(sprites[0], flippedSource, dest, origin, rotation, WHITE);
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
                // Random movement
                if (GetRandomValue(0, 100) < 2) {
                    direction = (DirectionDemon)GetRandomValue(-1, 1);
                    if (direction == 0) direction = RIGHT_DEMON;
                    
                    // Change state to walking or idle randomly
                    state = (GetRandomValue(0, 1) == 0) ? IDLE_DEMON : WALK_DEMON;
                }
                
                // Random attack
                if (GetRandomValue(0, 200) < 1) {
                    attack();
                }
                
                // Apply movement based on direction and state
                if (state == WALK_DEMON) {
                    velocity.x = 50.0f * (float)direction;
                } else {
                    velocity.x = 0;
                }
            } else {
                velocity.x = 0; // Stop moving when attacking or dead
            }
        }

        void attack() {
            if (!isAttacking && !isDead) {
                state = ATTACK_DEMON;
                isAttacking = true;
                hasFinishedAttack = false;
                
                // Activate attack collision box
                for (auto& box : collisionBoxes) {
                    if (box.type == ATTACK) {
                        // Position the attack box based on direction
                        if (direction == RIGHT_DEMON) {
                            box.rect.x = rect.x + rect.width - (36.0f * SPRITE_SCALE);
                        } else {
                            box.rect.x = rect.x - (72.0f * SPRITE_SCALE);
                        }
                        box.active = true;
                        break;
                    }
                }
                
                // Play attack sound if available
                if (attackSound.frameCount > 0) {
                    PlaySound(attackSound);
                }
            }
        }

        void applyVelocity() {
            float deltaTime = GetFrameTime();
            rect.x += velocity.x * deltaTime;
            
            // Update collision boxes positions
            updateCollisionBoxes();
        }

        void updateCollisionBoxes() {
            float bodyOffsetX = 36.0f * SPRITE_SCALE;
            float bodyOffsetY = 20.0f * SPRITE_SCALE;
            
            float attackOffsetX = rect.width - (36.0f * SPRITE_SCALE);
            float attackOffsetY = 30.0f * SPRITE_SCALE;
            
            float hurtboxOffsetX = 45.0f * SPRITE_SCALE;
            float hurtboxOffsetY = 25.0f * SPRITE_SCALE;
            
            for (auto& box : collisionBoxes) {
                if (box.type == BODY) {
                    box.rect.x = rect.x + bodyOffsetX;
                    box.rect.y = rect.y + bodyOffsetY;
                } else if (box.type == ATTACK) {
                    // Position attack box based on direction
                    if (direction == RIGHT_DEMON) {
                        box.rect.x = rect.x + attackOffsetX;
                    } else {
                        box.rect.x = rect.x - box.rect.width;
                    }
                    box.rect.y = rect.y + attackOffsetY;
                    
                    // Only active during attack animation
                    box.active = isAttacking;
                } else if (box.type == HURTBOX) {
                    box.rect.x = rect.x + hurtboxOffsetX;
                    box.rect.y = rect.y + hurtboxOffsetY;
                    
                    // Disable hurtbox if dead
                    box.active = !isDead;
                }
            }
        }

        void takeDamage(int damage) {
            if (!isDead) {
                health -= damage;
                if (health <= 0) {
                    health = 0;
                    isDead = true;
                    state = DEAD_DEMON;
                    
                    // Disable collision boxes except for BODY
                    for (auto& box : collisionBoxes) {
                        if (box.type == ATTACK || box.type == HURTBOX) {
                            box.active = false;
                        }
                    }
                    
                    // Play death sound if available
                    if (deadSound.frameCount > 0) {
                        PlaySound(deadSound);
                    }
                } else {
                    state = HURT_DEMON;
                    
                    // Play hurt sound if available
                    if (hurtSound.frameCount > 0) {
                        PlaySound(hurtSound);
                    }
                }
            }
        }

        void update(float deltaTime) {
            if (!isDead) {
                // Update chant timer
                chantTimer += deltaTime;
                if (chantTimer >= chantInterval) {
                    chantTimer = 0;
                    // Play chant sound if available
                    if (chantSound.frameCount > 0) {
                        PlaySound(chantSound);
                    }
                }
                
                move();
            }
            
            applyVelocity();
            updateAnimation();
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

#endif // DEMON_H