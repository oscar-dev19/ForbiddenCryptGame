#ifndef WIZARD_H
#define WIZARD_H

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>

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
            attackSound = LoadSound("sounds/wizard/magic-strike-5856.mp3");
            hurtSound = LoadSound("sounds/samurai/female-hurt-2-94301.wav"); // Using samurai hurt sound as fallback
            deadSound = LoadSound("sounds/samurai/female-death.wav"); // Using samurai death sound as fallback
            
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

        void loadTextures()  {
            // Clear any existing textures
            for (auto& sprite : sprites) {
                if (sprite.id != 0) {
                    UnloadTexture(sprite);
                }
            }
            
            // Resize and load all textures
            sprites.resize(7);
            sprites[DEAD_WIZARD] = LoadTexture("assets/Wizard/Sprites/Death.png");
            sprites[ATTACK1_WIZARD] = LoadTexture("assets/Wizard/Sprites/Attack1.png");
            sprites[ATTACK2_WIZARD] = LoadTexture("assets/Wizard/Sprites/Attack2.png");
            sprites[HURT_WIZARD] = LoadTexture("assets/Wizard/Sprites/Take hit.png");
            sprites[IDLE_WIZARD] = LoadTexture("assets/Wizard/Sprites/Idle.png");
            sprites[JUMP_WIZARD] = LoadTexture("assets/Wizard/Sprites/Jump.png");
            sprites[RUN_WIZARD] = LoadTexture("assets/Wizard/Sprites/Run.png");
            
            // Print debug info about loaded textures
            for (int i = 0; i < sprites.size(); i++) {
                printf("Wizard texture %d: %dx%d\n", i, sprites[i].width, sprites[i].height);
            }
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
                } else {
                    // Move in the current direction
                    velocity.x = direction * 1.0f;  // Wizard moves slower
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
            rect.y += velocity.y;

            // Keep within screen bounds
            if (rect.x < 0) {
                rect.x = 0;
                direction = RIGHT_WIZARD;
            }
            if (rect.x > GetScreenWidth() - rect.width) {
                rect.x = GetScreenWidth() - rect.width;
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

        void update() {
            if (!isDead) {
                move();
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

#endif // WIZARD_H