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

        // Health related to damage and death
        int health;
        bool isDead = false;

        std::vector<AnimationDemon> animations;
        std::vector<Texture2D> sprites; // The texture sheets used for animations
        
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

            // Initialize animations for different states
            animations = {
                { 0, 5, 0, 0.2f, 0.2f, REPEATING_DEMON }, // IDLE_DEMON
                { 0, 5, 0, 0.1f, 0.1f, REPEATING_DEMON }, // WALK_DEMON
                { 0, 5, 0, 0.1f, 0.1f, ONESHOT_DEMON },   // ATTACK_DEMON
                { 0, 2, 0, 0.2f, 0.2f, ONESHOT_DEMON },   // HURT_DEMON
                { 0, 5, 0, 0.2f, 0.2f, ONESHOT_DEMON }    // DEAD_DEMON
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
                std::cout << "Exception loading texture: " << e.what() << std::endl;
                // Create a small placeholder texture to prevent crashes
                Image placeholder = GenImageColor(288, 160, RED);
                Texture2D texture = LoadTextureFromImage(placeholder);
                UnloadImage(placeholder);
                sprites.push_back(texture);
            }

            // Initialize collision boxes with scaled dimensions
            float bodyOffsetX = 36.0f * SPRITE_SCALE;
            float bodyOffsetY = 20.0f * SPRITE_SCALE;
            float bodyWidth = std::max(10.0f, rect.width - (72.0f * SPRITE_SCALE));
            float bodyHeight = std::max(10.0f, rect.height - (20.0f * SPRITE_SCALE));
            
            float attackOffsetX = rect.width - (36.0f * SPRITE_SCALE);
            float attackOffsetY = 30.0f * SPRITE_SCALE;
            float attackSize = 72.0f * SPRITE_SCALE;
            float attackHeight = 40.0f * SPRITE_SCALE;
            
            float hurtboxOffsetX = 45.0f * SPRITE_SCALE;
            float hurtboxOffsetY = 25.0f * SPRITE_SCALE;
            float hurtboxWidth = std::max(10.0f, rect.width - (90.0f * SPRITE_SCALE));
            float hurtboxHeight = std::max(10.0f, rect.height - (30.0f * SPRITE_SCALE));
            
            try {
                collisionBoxes = {
                    CollisionBox({rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight}, BODY),
                    CollisionBox({rect.x + attackOffsetX, rect.y + attackOffsetY, attackSize, attackHeight}, ATTACK, false),
                    CollisionBox({rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight}, HURTBOX)
                };
                std::cout << "Demon collision boxes initialized. Count: " << collisionBoxes.size() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Exception initializing collision boxes: " << e.what() << std::endl;
                // Create default collision boxes to prevent crashes
                collisionBoxes = {
                    CollisionBox({rect.x, rect.y, rect.width, rect.height}, BODY),
                    CollisionBox({rect.x, rect.y, rect.width/2, rect.height/2}, ATTACK, false),
                    CollisionBox({rect.x, rect.y, rect.width/2, rect.height/2}, HURTBOX)
                };
            }
        }

        // Destructor to clean up resources
        ~Demon() {
            try {
                for (auto& sprite : sprites) {
                    if (sprite.id != 0) {
                        UnloadTexture(sprite);
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in Demon destructor: " << e.what() << std::endl;
            }
        }

        void updateAnimation() {
            // Safety check for valid state
            if (state < 0 || state >= animations.size()) {
                std::cout << "Invalid state in updateAnimation: " << state << std::endl;
                state = IDLE_DEMON; // Reset to a valid state
            }
            
            AnimationDemon& anim = animations[state];
            anim.timeLeft -= GetFrameTime();
            
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;
                
                if (anim.type == REPEATING_DEMON) {
                    anim.currentFrame = (anim.currentFrame + 1) % (anim.lastFrame + 1);
                } else if (anim.type == ONESHOT_DEMON) {
                    if (anim.currentFrame < anim.lastFrame) {
                        anim.currentFrame++;
                    } else {
                        // Animation has finished
                        if (state == ATTACK_DEMON) {
                            isAttacking = false;
                            hasFinishedAttack = true;
                            state = IDLE_DEMON;
                            animations[state].currentFrame = 0;
                        } else if (state == HURT_DEMON) {
                            state = IDLE_DEMON;
                            animations[state].currentFrame = 0;
                        } else if (state == DEAD_DEMON) {
                            // Keep in dead state
                        }
                    }
                }
            }
        }

        Rectangle getAnimationFrame() const {
            // Safety check for valid state
            if (state < 0 || state >= animations.size()) {
                return { 0, 0, 288, 160 }; // Return a default frame with valid dimensions
            }
            
            const AnimationDemon& anim = animations[state];
            
            // Safety check for valid current frame
            if (anim.currentFrame < 0 || anim.currentFrame > anim.lastFrame) {
                return { 0, (float)state * 160, 288, 160 }; // Return the first frame of the current state
            }
            
            // Calculate the frame width and height based on the spritesheet
            int frameWidth = 288;  // Width of each frame in the spritesheet
            int frameHeight = 160; // Height of each frame in the spritesheet
            
            // Safety check for sprites
            if (sprites.empty() || sprites[0].id == 0) {
                return { 0, 0, (float)frameWidth, (float)frameHeight }; // Return a default frame
            }
            
            // Calculate the position in the spritesheet
            float x = (float)frameWidth * anim.currentFrame;
            float y = (float)frameHeight * state;
            
            // Ensure we don't exceed texture bounds
            if (x >= sprites[0].width) {
                x = 0;
            }
            
            if (y >= sprites[0].height) {
                y = 0;
            }
            
            // Return the source rectangle for the current animation frame
            return { x, y, (float)frameWidth, (float)frameHeight };
        }

        void draw() const {
            // Don't draw if dead and animation has finished
            if (isDead && state == DEAD_DEMON && 
                animations[DEAD_DEMON].currentFrame >= animations[DEAD_DEMON].lastFrame) {
                return;
            }
            
            // Safety check for sprites
            if (sprites.empty() || sprites[0].id == 0) {
                DrawRectangle(rect.x, rect.y, rect.width, rect.height, RED);
                return;
            }
            
            // Get the source rectangle for the current animation frame
            Rectangle source;
            try {
                source = getAnimationFrame();
            } catch (...) {
                // If there's any error, draw a placeholder
                DrawRectangle(rect.x, rect.y, rect.width, rect.height, RED);
                return;
            }
            
            // Safety check for valid source dimensions
            if (source.width <= 0 || source.height <= 0 || 
                source.x < 0 || source.y < 0 || 
                source.x + source.width > sprites[0].width || 
                source.y + source.height > sprites[0].height) {
                DrawRectangle(rect.x, rect.y, rect.width, rect.height, RED);
                return;
            }
            
            // Draw the sprite with the correct direction
            Rectangle dest = {rect.x, rect.y, rect.width, rect.height};
            Vector2 origin = {0, 0};
            float rotation = 0.0f;
            
            // If facing left, flip the sprite horizontally
            if (direction == LEFT_DEMON) {
                dest.width *= -1;  // Negative width will flip the sprite
            }
            
            // Draw the sprite
            DrawTexturePro(sprites[0], source, dest, origin, rotation, WHITE);
            
            // Draw collision boxes if enabled
            if (showCollisionBoxes) {
                for (const auto& box : collisionBoxes) {
                    if (box.active) {
                        Color boxColor;
                        switch (box.type) {
                            case BODY: boxColor = BLUE; break;
                            case ATTACK: boxColor = RED; break;
                            case HURTBOX: boxColor = GREEN; break;
                            default: boxColor = PURPLE; break;
                        }
                        DrawRectangleLines(box.rect.x, box.rect.y, box.rect.width, box.rect.height, boxColor);
                    }
                }
            }
        }

        void move() {
            // Simple AI movement
            if (!isDead && state != HURT_DEMON && state != ATTACK_DEMON) {
                // Random chance to change direction
                if (GetRandomValue(0, 100) < 1) {
                    direction = (direction == RIGHT_DEMON) ? LEFT_DEMON : RIGHT_DEMON;
                }
                
                // Random chance to attack
                if (!isAttacking && GetRandomValue(0, 100) < 1) {
                    isAttacking = true;
                    hasFinishedAttack = false;
                    state = ATTACK_DEMON;
                    animations[state].currentFrame = 0;
                    return;
                }
                
                // Move in current direction
                if (GetRandomValue(0, 100) < 30) {
                    velocity.x = 2.0f * direction;
                    state = WALK_DEMON;
                } else {
                    velocity.x = 0;
                    state = IDLE_DEMON;
                }
            }
        }

        void applyVelocity() {
            rect.x += velocity.x;
            
            // Keep within screen bounds
            if (rect.x < 0) {
                rect.x = 0;
                direction = RIGHT_DEMON;
            }
            if (rect.x > GetScreenWidth() - rect.width) {
                rect.x = GetScreenWidth() - rect.width;
                direction = LEFT_DEMON;
            }
        }

        void takeDamage(int damage) {
            if (!isDead && state != HURT_DEMON) {
                health -= damage;
                if (health <= 0) {
                    health = 0;
                    isDead = true;
                    state = DEAD_DEMON;
                    animations[state].currentFrame = 0;
                    velocity.x = 0;
                    
                    // Deactivate collision boxes
                    for (auto& box : collisionBoxes) {
                        if (box.type == ATTACK || box.type == HURTBOX) {
                            box.active = false;
                        }
                    }
                } else {
                    state = HURT_DEMON;
                    animations[state].currentFrame = 0;
                    isAttacking = false;
                }
            }
        }

        void updateCollisionBoxes() {
            if (collisionBoxes.empty()) {
                std::cout << "Warning: No collision boxes to update" << std::endl;
                return;
            }
            
            try {
                for (auto& box : collisionBoxes) {
                    // Update position based on character position
                    if (box.type == BODY) {
                        float bodyOffsetX = 36.0f * SPRITE_SCALE;
                        float bodyOffsetY = 20.0f * SPRITE_SCALE;
                        box.rect.x = rect.x + bodyOffsetX;
                        box.rect.y = rect.y + bodyOffsetY;
                    } else if (box.type == ATTACK) {
                        float attackOffsetX = (direction == RIGHT_DEMON) ? 
                                            (rect.width - (36.0f * SPRITE_SCALE)) : 0;
                        float attackOffsetY = 30.0f * SPRITE_SCALE;
                        box.rect.x = rect.x + attackOffsetX;
                        box.rect.y = rect.y + attackOffsetY;
                        
                        // Only activate attack box during attack animation
                        box.active = (state == ATTACK_DEMON);
                    } else if (box.type == HURTBOX) {
                        float hurtboxOffsetX = 45.0f * SPRITE_SCALE;
                        float hurtboxOffsetY = 25.0f * SPRITE_SCALE;
                        box.rect.x = rect.x + hurtboxOffsetX;
                        box.rect.y = rect.y + hurtboxOffsetY;
                        
                        // Deactivate hurtbox if dead
                        box.active = !isDead;
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "Exception in updateCollisionBoxes: " << e.what() << std::endl;
            }
        }

        void update() {
            try {
                // Update animation
                updateAnimation();
                
                // Update collision boxes
                updateCollisionBoxes();
                
                // If dead, don't do anything else
                if (isDead) {
                    state = DEAD_DEMON;
                    return;
                }
                
                // Simple AI behavior
                move();
                applyVelocity();
            } catch (const std::exception& e) {
                std::cout << "Exception in Demon update: " << e.what() << std::endl;
            }
        }

        CollisionBox* getCollisionBox(CollisionBoxType type) {
            // Safety check for empty collision boxes
            if (collisionBoxes.empty()) {
                std::cout << "Warning: No collision boxes available" << std::endl;
                return nullptr;
            }
            
            // Update collision box positions based on current position and direction
            for (auto& box : collisionBoxes) {
                if (box.type == type) {
                    return &box;
                }
            }
            
            // If no matching collision box is found
            return nullptr;
        }
};

#endif // DEMON_H