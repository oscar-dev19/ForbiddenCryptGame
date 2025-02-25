#include <raylib.h>
#include <vector>
#include <iostream>

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

    Texture2D spriteSheet;  // Single sprite sheet texture
    std::vector<AnimationDemon> animations;

    Demon(Vector2 position) {
        rect = { position.x, position.y, 64.0f, 64.0f };
        velocity = { 0.0f, 0.0f };
        direction = RIGHT_DEMON;
        state = IDLE_DEMON;

        spriteSheet = LoadTexture("assets/Demon/spritesheets/demon_slime_FREE_v1.0_288x160_spritesheet.png");  // Load the sprite sheet
        
        // Debug: Check if the texture loads correctly
        std::cout << "Sprite Sheet Loaded: " << spriteSheet.width << "x" << spriteSheet.height << std::endl;

        // Define animations with first and last frame (dynamically handle frames per row)
        animations = {
            { 0, 5, 0, 0.1f, 0.1f, REPEATING_DEMON },   // Animation for Idle
            { 0, 11, 0, 0.1f, 0.1f, REPEATING_DEMON },  // Animation for Walk
            { 0, 14, 0, 0.1f, 0.1f, ONESHOT_DEMON },   // Animation for Cleave
            { 0, 4, 0, 0.1f, 0.1f, ONESHOT_DEMON },    // Animation for Take Hit
            { 0, 21, 0, 0.1f, 0.1f, ONESHOT_DEMON }    // Animation for Death
        };
    }

    ~Demon() {
        UnloadTexture(spriteSheet);  // Unload sprite sheet
    }

    void updateAnimation() {
        AnimationDemon& anim = animations[state];
        float deltaTime = GetFrameTime();
        anim.timeLeft -= deltaTime;

        if (anim.timeLeft <= 0) {
            anim.timeLeft = anim.speed;
            anim.currentFrame++;

            if (anim.currentFrame > anim.lastFrame) {
                if (anim.type == REPEATING_DEMON) {
                    anim.currentFrame = anim.firstFrame; // Loop back to first frame
                } else {
                    anim.currentFrame = anim.lastFrame; // Hold on the last frame
                    hasFinishedAttack = true; // Animation is complete
                }
            }
        }
    }

    Rectangle getAnimationFrame() const {
        const AnimationDemon& anim = animations[state];

        // Calculate the width and height of each frame dynamically based on sprite sheet size
        int frameHeight = spriteSheet.height / 5;  // Assuming 5 rows (one for each animation)

        // Calculate number of columns dynamically based on animation range
        int frameWidth = spriteSheet.width / (anim.lastFrame + 1 - anim.firstFrame); // Calculate based on first and last frame

        // Calculate the X and Y offsets based on the current animation row and frame
        int row = state;  // The row corresponds to the current animation state
        return {
            (float)(frameWidth * (anim.currentFrame - anim.firstFrame)),  // X offset of the frame
            (float)(frameHeight * row),                                   // Y offset (based on animation row)
            (float)frameWidth,                                             // Frame width
            (float)frameHeight                                            // Frame height
        };
    }

    void draw() const {
        Rectangle source = getAnimationFrame();
        float scale = 2.0f;

        Rectangle dest = { rect.x, rect.y, rect.width * scale, rect.height * scale };

        if (direction == LEFT_DEMON) {
            source.width = -source.width;  // Flip frame for left movement
            source.x += source.width;  // Adjust the X offset after flipping
        }

        DrawTexturePro(spriteSheet, source, dest, { 0, 0 }, 0.0f, WHITE);
    }

    void move() {
        if (!hasFinishedAttack) return;

        float moveSpeed = 300.0f;
        velocity.x = 0.0f;

        if (IsKeyDown(KEY_H)) {
            velocity.x = -moveSpeed;
            direction = LEFT_DEMON;
            state = WALK_DEMON;
        }
        else if (IsKeyDown(KEY_K)) {
            velocity.x = moveSpeed;
            direction = RIGHT_DEMON;
            state = WALK_DEMON;
        }
        else {
            state = IDLE_DEMON;
        }

        if (IsKeyPressed(KEY_KP_4) && hasFinishedAttack) {
            state = ATTACK_DEMON;
            hasFinishedAttack = false;
            animations[ATTACK_DEMON].currentFrame = animations[ATTACK_DEMON].firstFrame;
        }
    }

    void applyVelocity() {
        float deltaTime = GetFrameTime();
        rect.x += velocity.x * deltaTime;
        rect.y += velocity.y * deltaTime;
    }
};