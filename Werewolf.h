#ifndef WEREWOLF_H
#define WEREWOLF_H

#include "raylib.h"
#include "CollisionSystem.h"
#include "CharacterAI.h"
#include <vector>

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
    WALK_WOLF
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
        animations = {
            {0, 6, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF}, // DEAD_WOLF - 7 frames
            {0, 7, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF}, // ATTACK_SWIPE - 8 frames
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF}, // ATTACK_RUN - 6 frames
            {0, 2, 0, 0, 0.1f, 0.1f, ONESHOT_WOLF}, // HURT_WOLF - 3 frames
            {0, 6, 0, 0, 0.1f, 0.1f, REPEATING_WOLF}, // IDLE_WOLF - 7 frames
            {0, 8, 0, 0, 0.1f, 0.1f, REPEATING_WOLF}, // JUMP_WOLF - 9 frames
            {0, 7, 0, 0, 0.1f, 0.1f, REPEATING_WOLF}, // RUN_WOLF - 8 frames
            {0, 9, 0, 0, 0.1f, 0.1f, REPEATING_WOLF}  // WALK_WOLF - 10 frames
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
        
        // Resize and load all textures
        sprites.resize(8);
        sprites[DEAD_WOLF] = LoadTexture("assets/Werewolf/dead/werewolf_dead.png");
        sprites[ATTACK_SWIPE] = LoadTexture("assets/Werewolf/attack/werewolf_attack.png");
        sprites[ATTACK_RUN] = LoadTexture("assets/Werewolf/attack/werewolf_attack_run.png");
        sprites[HURT_WOLF] = LoadTexture("assets/Werewolf/hurt/werewolf_hurt.png");
        sprites[IDLE_WOLF] = LoadTexture("assets/Werewolf/idle/werewolf_idle.png");
        sprites[JUMP_WOLF] = LoadTexture("assets/Werewolf/jump/werewolf_jump.png");
        sprites[RUN_WOLF] = LoadTexture("assets/Werewolf/run/werewolf_run.png");
        sprites[WALK_WOLF] = LoadTexture("assets/Werewolf/walk/werewolf_walk.png");
        
        // Print debug info about loaded textures
        for (int i = 0; i < sprites.size(); i++) {
            printf("Werewolf texture %d: %dx%d\n", i, sprites[i].width, sprites[i].height);
        }
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
        // Safety check for valid state
        if (state < 0 || state >= sprites.size() || state >= animations.size()) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        const AnimationWolf& anim = animations[state];
        
        // Safety check for valid sprite
        if (sprites[state].id == 0 || sprites[state].width <= 0 || sprites[state].height <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        int frameWidth = sprites[state].width / (anim.lastFrame + 1);
        int frameHeight = sprites[state].height;
        
        // Safety check for valid frame dimensions
        if (frameWidth <= 0 || frameHeight <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
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
                // Chase the target
                state = RUN_WOLF;
                
                // Calculate direction vector towards target
                Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, wolfCenter));
                velocity.x = directionVector.x * moveSpeed;
                
                // Random chance to jump during pursuit
                if (isOnGround && GetRandomValue(0, 100) < 2) {
                    velocity.y = JUMP_FORCE;
                    isOnGround = false;
                    state = JUMP_WOLF;
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
        rect.y += velocity.y;

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
        
        // Check if werewolf has landed on the ground
        if (rect.y >= groundLevel - rect.height) {
            rect.y = groundLevel - rect.height;
            velocity.y = 0;
            isOnGround = true;
        }
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

    CollisionBox* getCollisionBox(CollisionBoxType type) {
        for (auto& box : collisionBoxes) {
            if (box.type == type) {
                return &box;
            }
        }
        return nullptr;
    }
};

#endif // WEREWOLF_H