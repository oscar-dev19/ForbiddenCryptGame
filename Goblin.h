#ifndef GOBLIN_H
#define GOBLIN_H

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>
#include "CharacterAI.h" // Include the CharacterAI header

// Directions for Goblin movement (LEFT_GOBLIN or RIGHT_GOBLIN)
enum DirectionGoblin {
    LEFT_GOBLIN = -1,
    RIGHT_GOBLIN = 1
};

// Possible states for the Goblin (Idle, Walking, Attacking, Dead)
enum CurrentStateGoblin {
    DEAD_GOBLIN = 0,
    ATTACK_CLUB = 1,
    ATTACK_STOMP = 2,
    ATTACK_AOE = 3,
    IDLE_GOBLIN = 4,
    WALK_GOBLIN = 5
};

// Animation types for Goblin (Repeating or One-shot)
enum AnimationTypeGoblin {
    REPEATING_GOBLIN,
    ONESHOT_GOBLIN
};

// Struct that holds animation data for each state
struct AnimationGoblin {
    int firstFrame;      // First frame of the animation
    int lastFrame;       // Last frame of the animation
    int currentFrame;    // Current frame being displayed
    int offset;          // Offset for animation positioning
    float speed;         // Speed of animation (time per frame)
    float timeLeft;      // Remaining time until frame change
    AnimationTypeGoblin type;  // Type of animation (repeating or one-shot)
};

class Goblin {
public:
    Rectangle rect;          // Goblin's position and size
    Vector2 velocity;        // Goblin's movement velocity
    Texture2D spriteSheet;   // Goblin's sprite sheet
    DirectionGoblin direction; // Goblin's facing direction
    CurrentStateGoblin state; // Current state of the Goblin (idle, walking, attacking)
    std::vector<AnimationGoblin> animations; // List of animations
    std::vector<Texture2D> sprites; // List of sprite sheets for each state

    bool isAttacking = false;    // Flag to check if Goblin is attacking
    bool hasFinishedAttack = true;  // Flag to check if the attack animation is finished
    bool isDead = false;         // Flag to check if Goblin is dead
    
    // AI system
    CharacterAI ai;
    float attackRange = 100.0f;
    float chaseRange = 300.0f;
    float moveSpeed = 1.0f;
    
    // Collision boxes for different purposes
    std::vector<CollisionBox> collisionBoxes;

    // Constructor initializing the Goblin's properties and animations
    Goblin(Vector2 position) {
        rect = { position.x, position.y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE };
        velocity = { 0, 0 };
        direction = RIGHT_GOBLIN;
        state = IDLE_GOBLIN;

        // Initialize animations for different states with correct frame counts
        animations = {
            { 0, 4, 0, 0, 0.2f, 0.2f, ONESHOT_GOBLIN },   // DEAD_GOBLIN - 5 frames
            { 0, 9, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },   // ATTACK_CLUB - 10 frames
            { 0, 23, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },  // ATTACK_STOMP - 24 frames
            { 0, 8, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },   // ATTACK_AOE - 9 frames
            { 0, 6, 0, 0, 0.2f, 0.2f, REPEATING_GOBLIN }, // IDLE_GOBLIN - 7 frames
            { 0, 7, 0, 0, 0.1f, 0.1f, REPEATING_GOBLIN }  // WALK_GOBLIN - 8 frames
        };

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
        
        // Initialize AI with an aggressive behavior
        ai.setBehavior(std::make_unique<AggressiveBehavior>(attackRange, chaseRange));
    }

    // Destructor to clean up resources
    ~Goblin() {
        for (auto& sprite : sprites) {
            if (sprite.id != 0) {
                UnloadTexture(sprite); // Unload all textures
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
        
        // Resize and load all textures
        sprites.resize(6);
        sprites[DEAD_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Die/Hobgoblin KO.png");
        sprites[ATTACK_CLUB] = LoadTexture("assets/Goblin/Hobgoblin Attack 1 & 2/Hobgoblin Attack 1 and 2.png");
        sprites[ATTACK_STOMP] = LoadTexture("assets/Goblin/Hobgoblin Attack 3/Hobgoblin Attack 3.png");
        sprites[ATTACK_AOE] = LoadTexture("assets/Goblin/Hobgoblin Attack 4/Hobgoblin Fourth Attack.png");
        sprites[IDLE_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Idle/GoblinK Idle.png");
        sprites[WALK_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Walk/Hobgoblin Walk.png");
        
        // Print debug info about loaded textures
        for (int i = 0; i < sprites.size(); i++) {
            printf("Goblin texture %d: %dx%d\n", i, sprites[i].width, sprites[i].height);
        }
    }

    void updateAnimation() {
        // Safety check for valid state
        if (state < 0 || state >= animations.size()) {
            state = IDLE_GOBLIN; // Reset to idle if state is invalid
        }
        
        AnimationGoblin& anim = animations[state];
        float deltaTime = GetFrameTime();

        anim.timeLeft -= deltaTime;
        if (anim.timeLeft <= 0) {
            anim.timeLeft = anim.speed;

            if (anim.currentFrame < anim.lastFrame) {
                anim.currentFrame++;
            } else {
                if (anim.type == REPEATING_GOBLIN) {
                    anim.currentFrame = anim.firstFrame;
                } else if (state == ATTACK_CLUB || state == ATTACK_STOMP || state == ATTACK_AOE) {
                    state = IDLE_GOBLIN;
                    isAttacking = false;
                    hasFinishedAttack = true;
                } else if (state == DEAD_GOBLIN) {
                    // Stay on the last frame if dead
                    anim.currentFrame = anim.lastFrame;
                }
            }
        }
    }

    Rectangle getAnimationFrame() const {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size() || state >= animations.size()) {
            return Rectangle{0, 0, 100, 100}; // Return a default frame
        }
        
        const AnimationGoblin& anim = animations[state];
        
        // Safety check for valid sprite
        if (sprites[state].id == 0 || sprites[state].width <= 0 || sprites[state].height <= 0) {
            return Rectangle{0, 0, 100, 100}; // Return a default frame
        }
        
        int frameWidth = sprites[state].width / (anim.lastFrame + 1);  // Calculate width of each frame
        int frameHeight = sprites[state].height;  // Height of the frame
        
        // Safety check for valid frame dimensions
        if (frameWidth <= 0 || frameHeight <= 0) {
            return Rectangle{0, 0, 100, 100}; // Return a default frame
        }
        
        // Safety check for valid current frame
        if (anim.currentFrame < 0 || anim.currentFrame > anim.lastFrame) {
            return Rectangle{0, 0, (float)frameWidth, (float)frameHeight}; // Return the first frame
        }

        return (Rectangle) {
            (float)frameWidth * anim.currentFrame,  // X position in the sprite sheet
            0,                                      // Y position in the sprite sheet
            (float)frameWidth,                      // Width of the frame
            (float)frameHeight                      // Height of the frame
        };
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

        if (direction == RIGHT_GOBLIN) {
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
            if (GetRandomValue(0, 100) < 1) {
                // Choose a random attack
                int attackType = GetRandomValue(1, 3);
                switch (attackType) {
                    case 1: state = ATTACK_CLUB; break;
                    case 2: state = ATTACK_STOMP; break;
                    case 3: state = ATTACK_AOE; break;
                }
                isAttacking = true;
                hasFinishedAttack = false;
                animations[state].currentFrame = 0;
            } else {
                // Move in the current direction
                velocity.x = direction * moveSpeed;
                state = WALK_GOBLIN;
            }
        } else if (isDead) {
            velocity.x = 0;  // Stop moving when dead
            state = DEAD_GOBLIN;
        } else {
            velocity.x = 0;  // Stop moving while attacking
        }

        // Apply velocity
        applyVelocity();
    }

    // New method to update with targeting behavior using the target position
    void updateWithTarget(Vector2 targetPos) {
        if (isDead) {
            state = DEAD_GOBLIN;
            updateAnimation();
            return;
        }

        // Calculate distance to target
        Rectangle goblinRect = {rect.x, rect.y, rect.width, rect.height};
        Vector2 goblinCenter = {goblinRect.x + goblinRect.width/2, goblinRect.y + goblinRect.height/2};
        float distanceToTarget = Vector2Distance(goblinCenter, targetPos);
        
        // Define a minimum distance to keep from the target
        float minDistance = 50.0f;
        
        // Update state based on AI behavior
        if (!isAttacking && hasFinishedAttack) {
            // Update direction based on target position
            if (targetPos.x < goblinCenter.x) {
                direction = LEFT_GOBLIN;
            } else {
                direction = RIGHT_GOBLIN;
            }
            
            if (distanceToTarget <= attackRange) {
                // Attack when in range
                int attackType = GetRandomValue(1, 3);
                switch (attackType) {
                    case 1: state = ATTACK_CLUB; break;
                    case 2: state = ATTACK_STOMP; break;
                    case 3: state = ATTACK_AOE; break;
                }
                isAttacking = true;
                hasFinishedAttack = false;
                animations[state].currentFrame = 0;
            }
            else if (distanceToTarget <= chaseRange) {
                // Chase the target but maintain minimum distance
                state = WALK_GOBLIN;
                
                if (distanceToTarget > minDistance) {
                    // Calculate direction vector towards target
                    Vector2 directionVector = Vector2Normalize(Vector2Subtract(targetPos, goblinCenter));
                    velocity.x = directionVector.x * moveSpeed;
                } else {
                    // Stop if we're already at minimum distance
                    velocity.x = 0;
                    state = IDLE_GOBLIN;
                }
            }
            else {
                // Idle when out of range
                state = IDLE_GOBLIN;
                velocity.x = 0;
            }
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
            direction = RIGHT_GOBLIN;
        }
        if (rect.x > mapWidth - rect.width) {
            rect.x = mapWidth - rect.width;
            direction = LEFT_GOBLIN;
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
                box.active = (isAttacking && animations[state].currentFrame >= 2 && animations[state].currentFrame <= 4);
                
                if (direction == RIGHT_GOBLIN) {
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
        if (!isDead) {
            // Implement health reduction logic here
            if (GetRandomValue(0, 100) < 30) {
                state = DEAD_GOBLIN;
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
            move();  // Fall back to random movement if no target is provided
            updateAnimation();
            updateCollisionBoxes();
        } else {
            state = DEAD_GOBLIN;
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

#endif // GOBLIN_H