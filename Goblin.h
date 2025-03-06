#ifndef GOBLIN_H
#define GOBLIN_H

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>

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
    
    // Collision boxes for different purposes
    std::vector<CollisionBox> collisionBoxes;

    // Constructor initializing the Goblin's properties and animations
    Goblin(Vector2 position) {
        rect = { position.x, position.y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE };
        velocity = { 0, 0 };
        direction = RIGHT_GOBLIN;
        state = IDLE_GOBLIN;

        // Initialize animations for different states
        animations = {
            { 0, 3, 0, 0, 0.2f, 0.2f, REPEATING_GOBLIN }, // DEAD_GOBLIN
            { 0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },   // ATTACK_CLUB
            { 0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },   // ATTACK_STOMP
            { 0, 5, 0, 0, 0.1f, 0.1f, ONESHOT_GOBLIN },   // ATTACK_AOE
            { 0, 5, 0, 0, 0.2f, 0.2f, REPEATING_GOBLIN }, // IDLE_GOBLIN
            { 0, 7, 0, 0, 0.1f, 0.1f, REPEATING_GOBLIN }  // WALK_GOBLIN
        };

        // Load textures for each state
        loadTextures();

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
    ~Goblin() {
        for (auto& sprite : sprites) {
            UnloadTexture(sprite); // Unload all textures
        }
    }

    void loadTextures() {
        sprites.resize(6);
        sprites[DEAD_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Die/Hobgoblin KO.png");
        sprites[ATTACK_CLUB] = LoadTexture("assets/Goblin/Hobgoblin Attack 1 & 2/Hobgoblin Attack 1 and 2.png");
        sprites[ATTACK_STOMP] = LoadTexture("assets/Goblin/Hobgoblin Attack 3/Hobgoblin Attack 3.png");
        sprites[ATTACK_AOE] = LoadTexture("assets/Goblin/Hobgoblin Attack 4/Hobgoblin Fourth Attack.png");
        sprites[IDLE_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Idle/GoblinK Idle.png");
        sprites[WALK_GOBLIN] = LoadTexture("assets/Goblin/Hobgoblin Walk/Hobgoblin Walk.png");
    }

    void updateAnimation() {
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
                }
            }
        }
    }

    Rectangle getAnimationFrame() const {
        const AnimationGoblin& anim = animations[state];
        int frameWidth = sprites[state].width / (anim.lastFrame + 1);  // Calculate width of each frame
        int frameHeight = sprites[state].height;  // Height of the frame

        return (Rectangle) {
            (float)frameWidth * anim.currentFrame,
            0,
            (float)frameWidth,
            (float)frameHeight
        };
    }

    void draw() const {
        if (isDead && animations[DEAD_GOBLIN].currentFrame == animations[DEAD_GOBLIN].lastFrame) {
            return; // Don't draw if dead and animation finished
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
                direction = (direction == RIGHT_GOBLIN) ? LEFT_GOBLIN : RIGHT_GOBLIN;
            }

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
                velocity.x = direction * 1.0f;
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

    void applyVelocity() {
        rect.x += velocity.x;
        rect.y += velocity.y;

        // Keep within screen bounds
        if (rect.x < 0) {
            rect.x = 0;
            direction = RIGHT_GOBLIN;
        }
        if (rect.x > GetScreenWidth() - rect.width) {
            rect.x = GetScreenWidth() - rect.width;
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
            move();
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