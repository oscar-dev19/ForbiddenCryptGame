#include <raylib.h>
#include <vector>

enum Direction {
    LEFT = -1,
    RIGHT = 1
};

enum CurrentState {
    DEAD = 0,
    ATTACK = 1,
    HURT = 2,
    IDLE = 3,
    JUMP = 4,
    RUN = 5,
    PARRY = 6,
    WALK = 7
};

enum AnimationType {
    REPEATING,
    ONESHOT
};

struct Animation {
    int firstFrame;
    int lastFrame;
    int currentFrame;
    int offset;
    float speed;
    float timeLeft;
    AnimationType type;
};

class Samurai {
    public:
        Rectangle rect;
        Vector2 velocity;
        Texture2D spriteSheet;
        Direction direction;
        CurrentState state;
        std::vector<Animation> animations;
        std::vector<Texture2D> sprites;

        Samurai(Vector2 position) {
            rect = (Rectangle) {position.x, position.y, 64.0f, 64.0f};
            velocity = (Vector2) {0.0f, 0.0f};
            direction = RIGHT;
            state = IDLE;
            animations = {
                {0, 2, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 3, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 1, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 5, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 11, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 1, 0, 0, 0.1f, 0.1f, REPEATING},
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING}
            };
        }

        ~Samurai() {
            for (auto& sprite : sprites) {
                UnloadTexture(sprite);
            }
        }

        void loadTextures() {
            sprites.resize(8);

            sprites[DEAD] = LoadTexture("assets/Samurai/Dead.png");
            sprites[ATTACK] = LoadTexture("assets/Samurai/Attack_2.png");
            sprites[HURT] = LoadTexture("assets/Samurai/Hurt.png");
            sprites[IDLE] = LoadTexture("assets/Samurai/Idle.png");
            sprites[JUMP] = LoadTexture("assets/Samurai/Jump.png");
            sprites[RUN] = LoadTexture("assets/Samurai/Run.png");
            sprites[PARRY] = LoadTexture("assets/Samurai/Shield.png");
            sprites[WALK] = LoadTexture("assets/Samurai/Walk.png");
        }

        void updateAnimation() {
            Animation& anim = animations[state];
            float deltaTime = GetFrameTime();  // Get the time that has passed since last frame.
        
            anim.timeLeft -= deltaTime;  // Reduce the remaining time for the current frame.
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;  // Reset the timeLeft to the speed value.
        
                anim.currentFrame++;  // Move to the next frame in the animation.
        
                if (anim.currentFrame > anim.lastFrame) {
                    if (anim.type == REPEATING) {
                        anim.currentFrame = anim.firstFrame;  // Reset to the first frame if it's a repeating animation.
                    } else if (anim.type == ONESHOT) {
                        anim.currentFrame = anim.lastFrame;  // Stay at the last frame for oneshot animations.
                    }
                }
            }
        }
        
        Rectangle getAnimationFrame() const {
            const Animation &anim = animations[state];
            int frameWidth = sprites[state].width / (anim.lastFrame + 1);
            int frameHeight = sprites[state].height;
        
            return (Rectangle){
                (float)frameWidth * anim.currentFrame, 0, (float)frameWidth, (float)frameHeight
            };
        }
        
        
        void draw() const {
            Rectangle source = getAnimationFrame();
        
            // Set scale factor (change this to make character bigger)
            float scale = 2.0f;  // Adjust as needed.
        
            // Define destination rectangle.
            Rectangle dest = {
                rect.x, rect.y, 
                rect.width * scale,   // Scale width.
                rect.height * scale   // Scale height.
            };
        
            // Flip the sprite if moving left.
            source.width *= direction;  
        
            // Draw the scaled sprite.
            DrawTexturePro(sprites[state], source, dest, {0, 0}, 0.0f, WHITE);
        }

        void move() {
            velocity.x = 0.0f;
            velocity.y = 0.0f;
        
            // Handle movement.
            if (IsKeyDown(KEY_A)) {
                velocity.x = -200.0f;
                direction = LEFT;
                state = RUN;
            } else if (IsKeyDown(KEY_D)) {
                velocity.x = 200.0f;
                direction = RIGHT;
                state = RUN;
            } else {
                state = IDLE;
            }
        
            // Handle attack.
            if (IsKeyDown(KEY_SPACE)) {
                state = ATTACK;
            }
        
            // Handle jump.
            if (IsKeyDown(KEY_W)) { 
                if (state != JUMP) {  // Prevent continuous jumping animation.
                    state = JUMP;
                    velocity.y = -300.0f;  // Set a negative value for upward velocity.
                }
            }
        
            // Handle parry
            if (IsKeyDown(KEY_E)) { 
                if (state != PARRY) {  // Prevent continuous parry animation.
                    state = PARRY;
                }
            }
        }
        
        
        void applyVelocity() {
            rect.x += velocity.x * GetFrameTime();
            rect.y += velocity.y * GetFrameTime();
        
            // Add gravity effect
            if (rect.y < 400) { // Assuming ground level is y = 400.
                velocity.y += 500.0f * GetFrameTime();  // Gravity pulls the character down.
            } else {
                velocity.y = 0.0f;  // Stop falling when on the ground.
                rect.y = 400;  // Keep the character on the ground.
            }
        }
};