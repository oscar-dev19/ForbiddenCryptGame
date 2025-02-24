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
        float groundLevel; // Store the initial ground level

        Samurai(Vector2 position) {
            rect = (Rectangle) {position.x, position.y, 64.0f, 64.0f};
            velocity = (Vector2) {0.0f, 0.0f};
            direction = RIGHT;
            state = IDLE;
            groundLevel = position.y; // Store the initial Y position as the ground level
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
            float deltaTime = GetFrameTime();  

            anim.timeLeft -= deltaTime;  
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;  

                anim.currentFrame++;  

                if (anim.currentFrame > anim.lastFrame) {
                    if (anim.type == REPEATING) {
                        anim.currentFrame = anim.firstFrame;  
                    } else if (anim.type == ONESHOT) {
                        anim.currentFrame = anim.lastFrame;  
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
            float scale = 2.0f;  

            Rectangle dest = {
                rect.x, rect.y, 
                rect.width * scale,   
                rect.height * scale   
            };

            source.width *= direction;  

            DrawTexturePro(sprites[state], source, dest, {0, 0}, 0.0f, WHITE);
        }

        void move() {
            velocity.x = 0.0f;
            
            if (IsKeyDown(KEY_A)) {
                velocity.x = -200.0f;
                direction = LEFT;
                if (rect.y >= groundLevel) state = RUN; // Only switch to RUN if on the ground
            } else if (IsKeyDown(KEY_D)) {
                velocity.x = 200.0f;
                direction = RIGHT;
                if (rect.y >= groundLevel) state = RUN;
            } else {
                if (rect.y >= groundLevel) state = IDLE;  // Set to IDLE when no movement keys are pressed
            }
        
            if (IsKeyDown(KEY_SPACE)) {
                state = ATTACK;
            }
        
            if (IsKeyDown(KEY_W) && rect.y >= groundLevel) {  
                state = JUMP;
                velocity.y = -300.0f;  
            }
        
            if (IsKeyDown(KEY_E)) { 
                if (state != PARRY) {  
                    state = PARRY;
                }
            }
        
            // **Fix: If the character is in the air, keep JUMP animation active**
            if (rect.y < groundLevel) {
                state = JUMP;
            } else if (state == JUMP && rect.y >= groundLevel) {
                state = IDLE; // Only switch to IDLE if fully landed
            }
        }
        
        
        
        void applyVelocity() {
            rect.x += velocity.x * GetFrameTime();
            rect.y += velocity.y * GetFrameTime();
        
            if (rect.y < groundLevel) { 
                velocity.y += 500.0f * GetFrameTime();  
            } else {
                velocity.y = 0.0f;  
                rect.y = groundLevel;  
            }
        }
};
