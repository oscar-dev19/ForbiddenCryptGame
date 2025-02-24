#include <raylib.h>
#include <vector>

// Direction of the Character.
enum Direction {
    LEFT = -1,
    RIGHT = 1
};

// States for the Character. (Note: not all will be used).
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

// Determines whether an animation is ran once or repeats.
enum AnimationType {
    REPEATING,
    ONESHOT
};

// Represents an Animation.
struct Animation {
    int firstFrame; // First Frame of Animation.
    int lastFrame; // Last Frame of Animation.
    int currentFrame; // Current Frame Displayed.
    int offset; // Frame Offset (if needed).
    float speed; // Speed of Animation.
    float timeLeft; // Time Left for next frame.
    AnimationType type; // Type of Animation.
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
            groundLevel = position.y; // Store the initial Y position as the ground level.
            animations = {
                {0, 2, 0, 0, 0.1f, 0.1f, REPEATING}, // Dead.
                {0, 3, 0, 0, 0.1f, 0.1f, REPEATING}, // Attack.
                {0, 1, 0, 0, 0.1f, 0.1f, REPEATING}, // Hurt.
                {0, 5, 0, 0, 0.1f, 0.1f, REPEATING}, // Idle.
                {0, 11, 0, 0, 0.1f, 0.1f, REPEATING}, // Jump.
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING}, // Run.
                {0, 1, 0, 0, 0.1f, 0.1f, REPEATING}, // Parry
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING} // Walk.
            };
        }

        ~Samurai() {
            for (auto& sprite : sprites) {
                UnloadTexture(sprite); // Unload each texture from memory.
            }
        }

        void loadTextures() {
            sprites.resize(8);
            
            // Load Sprite Textures.
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

            // Determines if we move to next frame.
            anim.timeLeft -= deltaTime;  
            if (anim.timeLeft <= 0) {
                anim.timeLeft = anim.speed;  

                anim.currentFrame++; // Move to next frame.

                // Loop Animation, Otherwises stop at Last Frame
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
            // Calculate Frame Dimensions.
            int frameWidth = sprites[state].width / (anim.lastFrame + 1);
            int frameHeight = sprites[state].height;

            return (Rectangle){ // Return Rectangle for the current frame.
                (float)frameWidth * anim.currentFrame, 0, (float)frameWidth, (float)frameHeight
            };
        }
        
        void draw() const {
            Rectangle source = getAnimationFrame();
            float scale = 2.0f; // Adjust Character Size.

            Rectangle dest = { // Rectangle for Sprite.
                rect.x, rect.y, 
                rect.width * scale,   
                rect.height * scale   
            };

            // Flip Sprite.
            source.width *= direction;  

            // Draw Sprite.
            DrawTexturePro(sprites[state], source, dest, {0, 0}, 0.0f, WHITE);
        }

        void move() {
            // Default horizontal velocity.
            float moveSpeed = 300.0f;
            velocity.x = 0.0f;
        
            if (IsKeyDown(KEY_A)) {
                velocity.x = -moveSpeed;  // Adjust Runnning Velocity Right.
                direction = LEFT;
                if (rect.y >= groundLevel) state = RUN;
            } else if (IsKeyDown(KEY_D)) {
                velocity.x = moveSpeed;  // Adjust Running Velocity Left.
                direction = RIGHT;
                if (rect.y >= groundLevel) state = RUN;
            } else {
                if (rect.y >= groundLevel) state = IDLE;
            }
        
            if (IsKeyDown(KEY_SPACE)) {
                state = ATTACK;
            }
        
            if (IsKeyDown(KEY_W) && rect.y >= groundLevel) {
                state = JUMP;
                velocity.y = -250.0f;  // Adjust Velocity-Y Vector.
                velocity.x *= 0.5f;  // Adjust Velocity-X Vector.
            }
        
            if (IsKeyDown(KEY_E)) { 
                if (state != PARRY) {  
                    state = PARRY;
                }
            }
        
            // If character is in the air, keep JUMP animation active.
            if (rect.y < groundLevel) {
                state = JUMP;
            } else if (state == JUMP && rect.y >= groundLevel) {
                state = IDLE;
            }
        }
        
        void applyVelocity() {
            rect.x += velocity.x * GetFrameTime(); // Apply Horizontal Velocity.
            rect.y += velocity.y * GetFrameTime(); // Apply Vertical Velocity.
            
            // Characters in the air.
            if (rect.y < groundLevel) { 
                velocity.y += 500.0f * GetFrameTime(); // Apply Gravity.
            } else {
                velocity.y = 0.0f;  
                rect.y = groundLevel;  
            }
        }
};
