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
            sprites.push_back(LoadTexture("assets/Demon/spritesheets/demon_slime_FREE_v1.0_288x160_spritesheet.png"));

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
        }

        ~Demon() {
            for (Texture2D& texture : sprites) {
                UnloadTexture(texture);
            }
        }

        void updateAnimation() {
            AnimationDemon& anim = animations[state];
            float deltaTime = GetFrameTime();

            anim.timeLeft -= deltaTime;
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;

                if (anim.currentFrame < anim.lastFrame) {
                    anim.currentFrame++;
                } else {
                    if (anim.type == REPEATING_DEMON) {
                        anim.currentFrame = anim.firstFrame;
                    } else if (state == ATTACK_DEMON) {
                        state = IDLE_DEMON;
                        isAttacking = false;
                        hasFinishedAttack = true;
                    } else if (state == HURT_DEMON) {
                        state = IDLE_DEMON;
                    }
                }
            }
        }

        Rectangle getAnimationFrame() const {
            const AnimationDemon& anim = animations[state];
            
            // Calculate the frame width and height based on the spritesheet
            int frameWidth = 288;  // Width of each frame in the spritesheet
            int frameHeight = 160; // Height of each frame in the spritesheet
            
            return { (float)frameWidth * anim.currentFrame, (float)frameHeight * state, (float)frameWidth, (float)frameHeight };
        }

        void draw() const {
            if (isDead && animations[DEAD_DEMON].currentFrame == animations[DEAD_DEMON].lastFrame) {
                return; // Don't draw if dead and animation finished
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

        void move() {
            // Simple AI movement logic
            if (!isAttacking && hasFinishedAttack && !isDead) {
                // Random chance to change direction or attack
                if (GetRandomValue(0, 100) < 2) {
                    direction = (direction == RIGHT_DEMON) ? LEFT_DEMON : RIGHT_DEMON;
                }

                if (GetRandomValue(0, 100) < 1) {
                    // Attack
                    state = ATTACK_DEMON;
                    isAttacking = true;
                    hasFinishedAttack = false;
                    animations[state].currentFrame = 0;
                } else {
                    // Move in the current direction
                    velocity.x = direction * 1.2f;  // Demon moves at medium speed
                    state = WALK_DEMON;
                }
            } else if (isDead) {
                velocity.x = 0;  // Stop moving when dead
                state = DEAD_DEMON;
            } else {
                velocity.x = 0;  // Stop moving while attacking
            }

            // Apply velocity
            applyVelocity();
        }

        void applyVelocity() {
            rect.x += velocity.x;
            rect.y += velocity.y;

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
            // Define scaled offsets and dimensions
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
            
            for (auto& box : collisionBoxes) {
                if (box.type == BODY) {
                    box.rect.x = rect.x + bodyOffsetX;
                    box.rect.y = rect.y + bodyOffsetY;
                    box.rect.width = bodyWidth;
                    box.rect.height = bodyHeight;
                } else if (box.type == ATTACK) {
                    // Only activate attack box during attack animation frames 2-4
                    box.active = (state == ATTACK_DEMON && animations[state].currentFrame >= 2 && animations[state].currentFrame <= 4);
                    
                    if (direction == RIGHT_DEMON) {
                        box.rect.x = rect.x + attackOffsetX;
                        box.rect.y = rect.y + attackOffsetY;
                        box.rect.width = attackSize;
                        box.rect.height = attackHeight;
                    } else {
                        box.rect.x = rect.x - attackSize;
                        box.rect.y = rect.y + attackOffsetY;
                        box.rect.width = attackSize;
                        box.rect.height = attackHeight;
                    }
                } else if (box.type == HURTBOX) {
                    box.rect.x = rect.x + hurtboxOffsetX;
                    box.rect.y = rect.y + hurtboxOffsetY;
                    box.rect.width = hurtboxWidth;
                    box.rect.height = hurtboxHeight;
                    box.active = !isDead;  // Deactivate hurtbox when dead
                }
            }
        }

        void update() {
            if (!isDead || (isDead && animations[DEAD_DEMON].currentFrame < animations[DEAD_DEMON].lastFrame)) {
                move();
                updateAnimation();
                updateCollisionBoxes();
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

#endif // DEMON_H