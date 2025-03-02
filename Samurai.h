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
    WALK = 6
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

        // Define Health Variables
        int maxHealth = 100;
        int currentHealth = 100;
        bool isDead = false;

        Samurai(Vector2 position) {
            rect = (Rectangle) {position.x, position.y, 64.0f, 64.0f};
            velocity = (Vector2) {0.0f, 0.0f};
            direction = RIGHT;
            state = IDLE;
            groundLevel = position.y; // Store the initial Y position as the ground level.
            animations = {
                {0, 2, 0, 0, 0.1f, 0.1f, ONESHOT}, // Dead.
                {0, 3, 0, 0, 0.1f, 0.1f, ONESHOT}, // Attack.
                {0, 1, 0, 0, 0.1f, 0.1f, ONESHOT}, // Hurt.
                {0, 5, 0, 0, 0.1f, 0.1f, REPEATING}, // Idle.
                {0, 11, 0, 0, 0.1f, 0.1f, ONESHOT}, // Jump.
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING}, // Run.
                {0, 7, 0, 0, 0.1f, 0.1f, REPEATING} // Walk.
            };
        }

        ~Samurai() {
            for (auto& sprite : sprites) {
                UnloadTexture(sprite); // Unload each texture from memory.
            }
        }

        void loadTextures() {
            sprites.resize(7);
            
            // Load Sprite Textures.
            sprites[DEAD] = LoadTexture("assets/Samurai/Dead.png");
            sprites[ATTACK] = LoadTexture("assets/Samurai/Attack_2.png");
            sprites[HURT] = LoadTexture("assets/Samurai/Hurt.png");
            sprites[IDLE] = LoadTexture("assets/Samurai/Idle.png");
            sprites[JUMP] = LoadTexture("assets/Samurai/Jump.png");
            sprites[RUN] = LoadTexture("assets/Samurai/Run.png");
            sprites[WALK] = LoadTexture("assets/Samurai/Walk.png");
        }

        void updateAnimation() {
            Animation& anim = animations[state];
            float deltaTime = GetFrameTime();
            
            // If in HURT state, prevent switching until the hurt animation finishes
            if (state == HURT) {
                if (anim.currentFrame == anim.lastFrame) {
                    // After the hurt animation finishes, switch back to IDLE or other state
                    if (currentHealth > 0) {
                        state = IDLE;  // Reset to idle state after hurt animation completes
                    }
                }
            }
            
            // Rest of the animation handling
            if (anim.currentFrame == anim.firstFrame && anim.timeLeft == anim.speed) {
                anim.timeLeft = anim.speed;
            }
            
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
            float moveSpeed = 300.0f;
            velocity.x = 0.0f;
            
            if (state == ATTACK || state == HURT) return; // Prevent movement during attack or hurt states
            
            if (IsKeyDown(KEY_A)) {
                velocity.x = -moveSpeed;
                direction = LEFT;
                if (rect.y >= groundLevel && state != JUMP) state = RUN;
            } else if (IsKeyDown(KEY_D)) {
                velocity.x = moveSpeed;
                direction = RIGHT;
                if (rect.y >= groundLevel && state != JUMP) state = RUN;
            } else {
                if (rect.y >= groundLevel && state != JUMP) state = IDLE;
            }
        
            if (IsKeyPressed(KEY_SPACE)) { // Attack plays fully
                state = ATTACK;
                animations[state].currentFrame = animations[state].firstFrame;
            }
        
            if (IsKeyPressed(KEY_W) && rect.y >= groundLevel) {
                state = JUMP;
                animations[JUMP].currentFrame = animations[JUMP].firstFrame; // Reset jump animation
                velocity.y = -250.0f;
                velocity.x *= 0.5f;
            }
        
            if (rect.y < groundLevel) {
                state = JUMP; // Ensure jump animation plays while airborne
            } else if (state == JUMP && rect.y >= groundLevel) {
                state = IDLE;
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
        
            if (state == ATTACK && animations[state].currentFrame == animations[state].lastFrame) {
                state = IDLE;
            }
        }

        void takeDamage(int damage) {
            if (isDead) return; // Don't take damage if already dead
            
            currentHealth -= damage;
            if (currentHealth <= 0) {
                currentHealth = 0;
                isDead = true;
                state = DEAD; // Set character to dead state
            } else {
                state = HURT; // Change state to hurt when damage is taken
                animations[HURT].currentFrame = animations[HURT].firstFrame; // Reset hurt animation to first frame
                animations[HURT].timeLeft = animations[HURT].speed; // Reset animation time so it plays from the start
            }
        }
        
        
        // Draw the health bar, including healing
        void drawHealthBar() {
            float healthPercentage = (float)currentHealth / maxHealth;
            DrawRectangle(20, 20, 200, 20, DARKGRAY); // Background
            DrawRectangle(20, 20, (int)(200 * healthPercentage), 20, RED); // Health bar (now green when healed)
            DrawText(TextFormat("Health: %d/%d", currentHealth, maxHealth), 20, 45, 20, WHITE);
        }

        void updateSamurai() {
            if (!isDead) {
                move(); // Allow movement if not dead
                applyVelocity();
            } else {
                velocity.x = 0;
                velocity.y = 0;
            }
            
            updateAnimation();
            checkForHealing(); // Call healing check every frame
            checkForDamage();
        }

        // Function to heal the samurai
        void heal(int healingAmount) {
            if (isDead) return; // Don't heal if dead

            currentHealth += healingAmount;
            if (currentHealth > maxHealth) {
                currentHealth = maxHealth; // Cap health at max health
            }
        }

        // Example of a healing item use (this can be triggered on key press or by items in the game)
        void checkForHealing() {
            if (IsKeyPressed(KEY_H)) { // Press 'H' to heal
                heal(20); // Heal 20 health points
            }
        }

        //Testing DELETE LATER!!!
        void checkForDamage() {
            if (IsKeyPressed(KEY_K)) {  // Press 'K' to simulate the enemy attack
                takeDamage(20);          // Apply damage to the samurai
            }
        }
};