#ifndef DEMON_H
#define DEMON_H

#include "raylib.h"
#include "CollisionSystem.h"
#include "CharacterAI.h" // Include the CharacterAI header
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
        Sound explosionSound;
        float chantTimer = 0.0f;
        float chantInterval = 10.0f; // Play chant every 10 seconds

        // AI system
        CharacterAI ai;
        float attackRange = 80.0f;
        float chaseRange = 500.0f;
        float moveSpeed = 0.8f;  // Demon is slower but more powerful

        // Collision boxes for different purposes
        std::vector<CollisionBox> collisionBoxes;

        // Constructor initializing the Demon's properties and animations
        Demon(Vector2 position, float baseSpeed = 150.0f, int startingHealth = 500) {
            std::cout << "Initializing Demon at position: " << position.x << ", " << position.y << std::endl;
            rect = { position.x, position.y, 144.0f * SPRITE_SCALE, 80.0f * SPRITE_SCALE };
            velocity = { 0, 0 };
            direction = RIGHT_DEMON;
            state = IDLE_DEMON;
            health = startingHealth; // Initialize health
            moveSpeed = baseSpeed * 0.01f; // Convert to appropriate scale

            // Initialize animations for different states with correct frame counts
            animations = {
                { 0, 5, 0, 0.1f, 0.1f, REPEATING_DEMON }, // IDLE_DEMON - 6 frames
                { 0, 11, 0, 0.1f, 0.1f, REPEATING_DEMON },  // WALK_DEMON - 12 frames
                { 0, 14, 0, 0.1f, 0.1f, ONESHOT_DEMON },   // ATTACK_DEMON - 15 frames
                { 0, 4, 0, 0.1f, 0.1f, ONESHOT_DEMON },    // HURT_DEMON - 5 frames
                { 0, 21, 0, 0.1f, 0.1f, ONESHOT_DEMON }    // DEAD_DEMON - 22 frames
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

            // Initialize collision boxes with scaled dimensions for the larger Demon
            float bodyOffsetX = 30.0f * SPRITE_SCALE;
            float bodyOffsetY = 20.0f * SPRITE_SCALE;
            float bodyWidth = rect.width - (60.0f * SPRITE_SCALE);
            float bodyHeight = rect.height - (30.0f * SPRITE_SCALE);
            
            float attackOffsetX = rect.width - (20.0f * SPRITE_SCALE);
            float attackOffsetY = 30.0f * SPRITE_SCALE;
            float attackWidth = 32.0f * SPRITE_SCALE;
            float attackHeight = 50.0f * SPRITE_SCALE;
            
            float hurtboxOffsetX = 40.0f * SPRITE_SCALE;
            float hurtboxOffsetY = 25.0f * SPRITE_SCALE;
            float hurtboxWidth = rect.width - (80.0f * SPRITE_SCALE);
            float hurtboxHeight = rect.height - (45.0f * SPRITE_SCALE);
            
            collisionBoxes = {
                CollisionBox({rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight}, BODY),
                CollisionBox({rect.x + attackOffsetX, rect.y + attackOffsetY, attackWidth, attackHeight}, ATTACK, false),
                CollisionBox({rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight}, HURTBOX)
            };
            
            // Initialize AI with an aggressive behavior
            // Create a new AggressiveBehavior and pass it to setBehavior
            ai.setBehavior(std::unique_ptr<AIBehavior>(new AggressiveBehavior(attackRange, chaseRange)));
            
            std::cout << "Demon collision boxes initialized. Count: " << collisionBoxes.size() << std::endl;

            // Load sounds
            chantSound = LoadSound("sounds/misc/demon-chant-latin-14489.mp3");
            hurtSound = LoadSound("sounds/demon/mixkit-fantasy-monster-grunt-1977.wav"); 
            deadSound = LoadSound("sounds/demon/demonic-roar-40349.wav"); 
            explosionSound = LoadSound("sounds/demon/large-explosion-100420.wav");
            attackSound = LoadSound("sounds/demon/sword-clash-1-6917.wav");

            // Set sound volume
            SetSoundVolume(chantSound, 0.7f);
            SetSoundVolume(hurtSound, 0.7f);
            SetSoundVolume(deadSound, 0.7f);
            SetSoundVolume(explosionSound, 0.7f);
            SetSoundVolume(attackSound, 0.7f);
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
            if (explosionSound.frameCount > 0) UnloadSound(explosionSound);
            if (attackSound.frameCount > 0) UnloadSound(attackSound);
        }

        void updateAnimation() {
            std::cout << "Demon Animation State: " << state << std::endl;
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
                    } else {
                        if (state == DEAD_DEMON) {
                            // Stay on the last frame if dead
                            if (anim.currentFrame != 21) {
                                anim.currentFrame = anim.lastFrame;
                            }
                        } else {
                            if (state != DEAD_DEMON) {
                                if (state == ATTACK_DEMON) {
                                    isAttacking = false;
                                    hasFinishedAttack = true;
                                } else if(state == HURT_DEMON) {
                                    if (anim.currentFrame == anim.lastFrame) {
                                        state = ATTACK_DEMON;
                                        AnimationDemon& attackAnim = animations[ATTACK_DEMON];
                                        attackAnim.currentFrame = attackAnim.firstFrame;
                                        attackAnim.timeLeft = attackAnim.speed;
                                        isAttacking = true;
                                        hasFinishedAttack = false;
    
                                        if (attackAnim.currentFrame >= attackAnim.lastFrame - 1) {
                                            
                                        }
                                    }
                                }
                            }
                        }
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

            if (direction == LEFT_DEMON) {
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
                // Random movement logic
                if (GetRandomValue(0, 100) < 2) {
                    direction = (DirectionDemon)GetRandomValue(-1, 1);
                    if (direction == 0) direction = RIGHT_DEMON;
                    
                    // Change state to walking or idle randomly
                    state = (GetRandomValue(0, 1) == 0) ? IDLE_DEMON : WALK_DEMON;
                }
                
                // Random attack logic
                if (GetRandomValue(0, 200) < 1) {
                    attack();
                }
            
                // Apply movement
                if (state == WALK_DEMON) {
                    velocity.x = 50.0f * (float)direction;
                } else {
                    velocity.x = 0;
                }
            } else {
                velocity.x = 0;
            }            
        }

        void attack() {
            if (!isAttacking && !isDead) {
                PlaySound(attackSound);
                state = ATTACK_DEMON;
                animations[state].currentFrame = 0;
                isAttacking = true;
                hasFinishedAttack = false;
                velocity.x = 0;
                
                // Activate attack collision box
                for (auto& box : collisionBoxes) 
                {
                    if (box.type == ATTACK) 
                    {
                        // Position the attack box based on direction
                        CollisionBox* hurtbox = getCollisionBox(HURTBOX);
                        float attackOffset = 0.0f * SPRITE_SCALE; // Adjust this value to bring the box closer/farther

                        if (hurtbox) 
                        {
                            if (direction == RIGHT_DEMON)
                            {
                                // Place attack box just to the right of hurtbox
                                box.rect.x = hurtbox->rect.x + hurtbox->rect.width + attackOffset;
                            } 
                            else 
                            {
                                // Place attack box just to the left of hurtbox
                                box.rect.x = hurtbox->rect.x - box.rect.width - attackOffset;
                            }
                        
                        box.rect.y = hurtbox->rect.y + 5.0f * SPRITE_SCALE; // Optional: slight vertical offset
                        box.active = true;
                        }
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
            
            // Add map boundary checks
            // Map dimensions are 128 tiles * 16 pixels = 2048 pixels wide
            const float mapWidth = 128 * 16;
            
            if (rect.x < 0) {
                rect.x = 0;
                direction = RIGHT_DEMON;
            }
            if (rect.x > mapWidth - rect.width) {
                rect.x = mapWidth - rect.width;
                direction = LEFT_DEMON;
            }

            // Left Hard Cap to ensure Demon stays in bounds.
            if (rect.x < 600) {
                rect.x = 600;
                direction = RIGHT_DEMON;
            }

            
            // Right Hard Cap to ensure Demon stays in bounds.
            if (rect.x > 1270) {
                rect.x = 1270;
                direction = LEFT_DEMON;
            }

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
            
            CollisionBox* hurtbox = getCollisionBox(HURTBOX);
            for (auto& box : collisionBoxes) 
            {
                if (box.type == BODY) 
                {
                    box.rect.x = rect.x + bodyOffsetX;
                    box.rect.y = rect.y + bodyOffsetY;
                } 
                else if (box.type == ATTACK) 
                {
                    if (hurtbox)
                    {
                        // Moving the red attack box either closer or farther
                        float attackOffset = 0.0f * SPRITE_SCALE;
                        
                        if (direction == RIGHT_DEMON) 
                        {
                            box.rect.x = hurtbox->rect.x + hurtbox->rect.width + attackOffset;
                        } 
                        else 
                        {
                            box.rect.x = hurtbox->rect.x - box.rect.width - attackOffset;
                        }
                    }
                    box.rect.y = hurtbox->rect.y + 5.0f * SPRITE_SCALE;
                    
                    if (box.type == ATTACK) {
                        // Only activate the attack box in the last two frames of the attack animation
                        if (state == ATTACK_DEMON) {
                            AnimationDemon& attackAnim = animations[ATTACK_DEMON];

                            if (attackAnim.currentFrame >= attackAnim.lastFrame - 4) {
                                box.active = true;

                                if (hurtbox) {
                                    float attackOffset = 0.0f * SPRITE_SCALE;
                                    if (direction == RIGHT_DEMON) {
                                        box.rect.x = hurtbox->rect.x + hurtbox->rect.width + attackOffset;
                                    } else {
                                        box.rect.x = hurtbox->rect.x - box.rect.width - attackOffset;
                                    }
                                    box.rect.y = hurtbox->rect.y + 5.0f * SPRITE_SCALE;
                                }
                            } else {
                                box.active = false;  // Disable hitbox outside of hit frames
                            }
                        } else {
                            box.active = false;  // Disable if not attacking
                        }
                    }

                } 
                else if (box.type == HURTBOX) 
                {
                    box.rect.x = rect.x + hurtboxOffsetX;
                    box.rect.y = rect.y + hurtboxOffsetY;
                    
                    // Disable hurtbox if dead
                    box.active = !isDead;
                }
            }
        }

        void takeDamage(int damage) {
            if (!isDead) {
                health -= (damage - 24);
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
                        PlaySound(explosionSound);
                        StopSound(attackSound);
                    }
                } else {
                    state = HURT_DEMON;
                    velocity.x = 0;
                    
                    // Play hurt sound if available
                    if (hurtSound.frameCount > 0) {
                        PlaySound(hurtSound);
                        StopSound(attackSound);
                    }
                }
            }
        }

        // Update with targeting behavior
        void update(float deltaTime, Vector2 targetPos) {
            // Update chanting sound timer
            chantTimer += deltaTime;
            if (chantTimer >= chantInterval) {
                chantTimer = 0.0f;
                if (!IsSoundPlaying(chantSound) && !isDead) {
                    PlaySound(chantSound);
                }
            }
            
            if (isDead) {
                state = DEAD_DEMON;
                updateAnimation();
                return;
            }

            // Calculate distance to target
            Vector2 demonCenter = {rect.x + rect.width/2, rect.y + rect.height/2};
            float distance = Vector2Distance(demonCenter, targetPos);
            
            // Define a minimum distance to keep from the target
            // The Demon is larger, so it needs a bigger minimum distance
            float minDistance = 1.0;
            
            // Update state based on AI behavior
            if (!isAttacking && hasFinishedAttack) {
                // Update direction based on target position
                if (targetPos.x < demonCenter.x) {
                    direction = LEFT_DEMON;
                } else {
                    direction = RIGHT_DEMON;
                }
                
                if (distance <= attackRange && distance >= minDistance) {
                    // Attack when in range and not too close
                    state = ATTACK_DEMON;
                    isAttacking = true;
                    hasFinishedAttack = false;
                    animations[state].currentFrame = 0;
                    
                    // Play attack sound
                    if (!IsSoundPlaying(attackSound)) {
                        PlaySound(attackSound);
                    }
                }
                else if (distance < minDistance) {
                    // Too close, take a step back
                    state = WALK_DEMON;
                    Vector2 directionVector = Vector2Normalize(Vector2Subtract(demonCenter, targetPos));
                    velocity.x = directionVector.x * moveSpeed * 0.5f; // Move back slowly
                }
                else if (distance <= chaseRange) {
                    // Chase the target but maintain minimum distance
                    state = WALK_DEMON;
                    
                    // Only move if we're outside the minimum distance
                    if (distance > minDistance) {
                        // Calculate direction vector towards target
                        Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, demonCenter));
                        velocity.x = directionVector.x * moveSpeed;
                    } else {
                        velocity.x = 0;
                        state = IDLE_DEMON;
                    }
                }
                else {
                    // Idle when out of range
                    state = IDLE_DEMON;
                    velocity.x = 0;
                }
            } else if (isAttacking) {
                velocity.x = 0;
            }
            
            // Apply velocity
            rect.x += velocity.x;
            
            // Map boundaries
            const float mapWidth = 128 * 16;
            if (rect.x < 0) rect.x = 0;
            if (rect.x > mapWidth - rect.width) rect.x = mapWidth - rect.width;
            
            updateAnimation();
            updateCollisionBoxes();
        }

        // Original update method for backward compatibility
        void update(float deltaTime) {
            // Update chanting sound timer
            chantTimer += deltaTime;
            if (chantTimer >= chantInterval) {
                chantTimer = 0.0f;
                if (!IsSoundPlaying(chantSound) && !isDead) {
                    PlaySound(chantSound);
                }
            }
            
            if (!isDead) {
                move();  // Fall back to random movement
                updateAnimation();
                updateCollisionBoxes();
            } else {
                state = DEAD_DEMON;
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

#endif // DEMON_H
