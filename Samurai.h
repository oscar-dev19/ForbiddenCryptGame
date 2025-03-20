#pragma once

#include "raylib.h"
#include "CollisionSystem.h"
#include <vector>
#include <cmath>   // For sinf
#include <cstdlib> // For system
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX
#include <cstdio>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Define PI if not already defined
#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Direction of the Character.
enum Direction {
    LEFT = -1,
    RIGHT = 1
};

// States for the Character. (Note: not all will be used).
enum CurrentState {
    DEAD_STATE = 0,
    ATTACK_STATE = 1,
    HURT_STATE = 2,
    IDLE_STATE = 3,
    JUMP_STATE = 4,
    RUN_STATE = 5
};

// Animation types.
enum AnimationType {
    LOOP,    // Animation loops continuously.
    ONESHOT  // Animation plays once and stops at the last frame.
};

// Animation data structure.
struct Animation {
    int firstFrame;    // First frame of the animation.
    int lastFrame;     // Last frame of the animation.
    int currentFrame;  // Current frame being displayed.
    float timer;       // Timer for frame changes.
    float frameTime;   // Time per frame.
    float speed;       // Animation speed.
    AnimationType type; // Type of animation (loop or oneshot).
};

class Samurai {
private:
    Rectangle rect; // Character's rectangle for position and size.
    Vector2 velocity; // Velocity of the character for movement.
    Direction direction; // Current facing direction of the character.
    CurrentState state; // Current state of the character (e.g., idle, attack, etc.).
    std::vector<Animation> animations; // List of animations for different states.
    std::vector<Texture2D> sprites; // List of textures for each state.
    float groundLevel; // The Y-coordinate of the ground level.

    // Define Health Variables
    int maxHealth = 100; // Maximum health value.
    int currentHealth = 100; // Current health of the samurai.
    bool wasInAir = false; // Flag to indicate if the character was in the air.
    
    // Double jump variables
    bool canDoubleJump = false; // Flag to track if double jump is available
    bool hasDoubleJumped = false; // Flag to track if double jump has been used
    float doubleJumpHeight = -60.0f; // Reduced from -80.0f to prevent flying off screen
    float singleJumpMaxHeight; // Maximum height for a single jump
    float doubleJumpMaxHeight; // Maximum height for a double jump
    
    // Double dash variables
    float dashSpeed = 15.0f; // Speed multiplier when dashing
    float dashDuration = 0.3f; // How long the dash lasts in seconds
    float dashTimer = 0.0f; // Timer to track current dash duration
    float dashCooldown = 0.5f; // Cooldown period between dashes in seconds
    float dashCooldownTimer = 0.0f; // Timer to track cooldown
    float lastAKeyPressTime = 0.0f; // Time when A key was last pressed
    float lastDKeyPressTime = 0.0f; // Time when D key was last pressed
    float doubleTapTimeThreshold = 0.3f; // Maximum time between taps to count as double tap
    bool canDash = true; // Flag to determine if dash is available
    float dashSoundVolume = 0.8f; // Volume for dash sound (0.0 to 1.0)
    
    // Invincibility frames variables
    bool isInvincible = false; // Flag to indicate if the character is currently invincible
    float invincibilityTimer = 0.0f; // Timer to track invincibility duration
    const float invincibilityDuration = 1.5f; // 1.5 seconds of invincibility after taking damage

    // Define Sound Varaibles.
    Sound attackSound;
    Sound jumpSound;
    Sound hurtSound;
    Sound runSound;
    Sound deadSound;
    Sound landSound;
    Sound dashSound; // New sound for dash ability

    // Collision boxes for different purposes
    std::vector<CollisionBox> collisionBoxes;

    // Helper method to update the animation frame.
    void updateAnimation(float deltaTime) {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size()) {
            state = IDLE_STATE;
        }
        
        Animation &anim = animations[state];
        anim.timer += deltaTime;
        
        // Handle special case for HURT_STATE
        if (state == HURT_STATE && anim.currentFrame >= anim.lastFrame) {
            state = IDLE_STATE;
            animations[state].currentFrame = 0;
            anim.timer = 0;
        }
        
        // Update animation frame based on timer
        if (anim.timer >= anim.frameTime) {
            anim.timer = 0;
            
            // For JUMP_STATE, don't increment frame if we're at the last frame
            if (state == JUMP_STATE && anim.currentFrame >= anim.lastFrame) {
                // Keep the last frame until landing
            } else {
                // Increment frame
                anim.currentFrame++;
                
                // Handle frame looping or state transitions
                if (anim.currentFrame > anim.lastFrame) {
                    if (anim.type == LOOP) {
                        anim.currentFrame = 0;
                    } else if (anim.type == ONESHOT) {
                        anim.currentFrame = anim.lastFrame;
                        
                        // For ATTACK_STATE, transition back to IDLE_STATE when animation completes
                        if (state == ATTACK_STATE) {
                            state = IDLE_STATE;
                            animations[state].currentFrame = 0;
                        }
                    }
                }
            }
        }
    }

    // Helper method to get the current animation frame rectangle.
    Rectangle getAnimationFrame() const {
        // Safety check for valid state
        if (state < 0 || state >= sprites.size() || state >= animations.size()) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Get the sprite dimensions
        int spriteWidth = sprites[state].width;
        
        // Safety check for valid sprite
        if (spriteWidth <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Calculate the number of frames in the sprite sheet
        int framesPerRow = spriteWidth / 128;
        
        // Safety check for valid frames calculation
        if (framesPerRow <= 0) {
            return Rectangle{0, 0, 128, 128}; // Return a default frame
        }
        
        // Get the current frame from the animation
        int currentFrame = animations[state].currentFrame;
        
        // Safety check for valid frame
        if (currentFrame < 0) {
            currentFrame = 0; // Reset to first frame
        }
        
        // Ensure current frame doesn't exceed the last frame
        if (currentFrame > animations[state].lastFrame) {
            currentFrame = animations[state].lastFrame;
        }
        
        // Calculate the position of the frame in the sprite sheet
        int frameX = (currentFrame % framesPerRow) * 128;
        int frameY = (currentFrame / framesPerRow) * 128;
        
        return Rectangle{(float)frameX, (float)frameY, 128, 128};
    }

    // Helper method to handle movement input.
    void move(float deltaTime) {
        // Update dash cooldown timer
        if (!canDash) {
            dashCooldownTimer -= deltaTime;
            if (dashCooldownTimer <= 0.0f) {
                canDash = true;
                dashCooldownTimer = 0.0f;
            }
        }
        
        // Update dash timer if currently dashing
        if (isDashing) {
            dashTimer -= deltaTime;
            if (dashTimer <= 0.0f) {
                isDashing = false;
                dashTimer = 0.0f;
            }
        }
        
        // Reset velocity if not jumping.
        if (rect.y >= groundLevel) {
            velocity.y = 0;
            rect.y = groundLevel;  // Ensure character is on ground.
            
            // Reset double jump flags when landed
            canDoubleJump = false;
            hasDoubleJumped = false;
            
            // Only transition from JUMP_STATE to IDLE_STATE when landing
            if (state == JUMP_STATE) {
                state = IDLE_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame
                if (wasInAir) {
                    if (landSound.frameCount > 0) {
                        PlaySound(landSound);
                    }
                    wasInAir = false;
                }
            }
        }

        // Check for jump input.
        if (IsKeyPressed(KEY_W)) {
            // First jump (from ground)
            if (rect.y >= groundLevel) {
                velocity.y = -100.0f;  // Apply upward velocity.
                if (jumpSound.frameCount > 0) {
                    PlaySound(jumpSound);
                }
                wasInAir = true;
                canDoubleJump = true; // Enable double jump after first jump
                
                // Set state to JUMP_STATE and reset animation
                if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                    state = JUMP_STATE;
                    animations[state].currentFrame = 0;  // Reset animation frame
                }
            }
            // Double jump (in mid-air)
            else if (canDoubleJump && !hasDoubleJumped) {
                velocity.y = doubleJumpHeight;  // Apply double jump velocity
                if (jumpSound.frameCount > 0) {
                    PlaySound(jumpSound);
                }
                hasDoubleJumped = true; // Mark double jump as used
                canDoubleJump = false; // Prevent further jumps
                
                // Reset jump animation for the double jump
                if (state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                    animations[JUMP_STATE].currentFrame = 0;  // Reset animation frame
                }
            }
        }

        // Apply gravity.
        if (rect.y < groundLevel) {
            velocity.y += 0.5f;  // Gravity effect.
            
            // Maintain JUMP_STATE while in the air unless in special states
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = JUMP_STATE;
                animations[state].currentFrame = 0;  // Reset animation frame
            }
        }
        
        // Current time for double tap detection
        float currentTime = GetTime();
        
        // Handle left/right movement with double tap dash
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
            // Check for double tap (if the last press was recent enough)
            if (canDash && (currentTime - lastAKeyPressTime) <= doubleTapTimeThreshold) {
                // Initiate dash to the left
                isDashing = true;
                dashTimer = dashDuration;
                canDash = false;
                dashCooldownTimer = dashCooldown;
                direction = LEFT;
                
                // Play dash sound effect
                playDashSound();
            }
            
            // Update the last key press time
            lastAKeyPressTime = currentTime;
        }
        
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
            // Check for double tap (if the last press was recent enough)
            if (canDash && (currentTime - lastDKeyPressTime) <= doubleTapTimeThreshold) {
                // Initiate dash to the right
                isDashing = true;
                dashTimer = dashDuration;
                canDash = false;
                dashCooldownTimer = dashCooldown;
                direction = RIGHT;
                
                // Play dash sound effect
                playDashSound();
            }
            
            // Update the last key press time
            lastDKeyPressTime = currentTime;
        }
        
        // Handle movement based on key press and dash state
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            // Apply dash speed if dashing, otherwise normal speed
            velocity.x = isDashing && direction == LEFT ? -dashSpeed : -5.0f;
            direction = LEFT;
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;  // Set to run state.
            }
        } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            // Apply dash speed if dashing, otherwise normal speed
            velocity.x = isDashing && direction == RIGHT ? dashSpeed : 5.0f;
            direction = RIGHT;
            if (state != JUMP_STATE && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
                state = RUN_STATE;  // Set to run state.
            }
        } else {
            // When no movement keys are pressed but still dashing
            if (isDashing) {
                velocity.x = (direction == RIGHT) ? dashSpeed : -dashSpeed;
            } else {
                velocity.x = 0;  // Stop horizontal movement if not dashing
            }
            
            if (state == RUN_STATE && !isDashing) {
                state = IDLE_STATE;  // Return to idle if not running and not dashing
            }
        }

        // Check for attack input.
        if (IsKeyPressed(KEY_SPACE) && state != ATTACK_STATE && state != HURT_STATE && state != DEAD_STATE) {
            state = ATTACK_STATE;  // Set to attack state.
            animations[state].currentFrame = 0;  // Reset animation frame.
            if (attackSound.frameCount > 0) {
                PlaySound(attackSound);
            }
        }

        // Apply velocity to position.
        applyVelocity();
    }

    // Helper method to apply velocity to position.
    void applyVelocity() {
        rect.x += velocity.x;  // Update horizontal position.
        rect.y += velocity.y;  // Update vertical position.

        // Ensure character stays within map bounds instead of screen bounds
        // Map dimensions are 128 tiles * 16 pixels = 2048 pixels wide
        const float mapWidth = 128 * 16;
        
        if (rect.x < 0) rect.x = 0;
        if (rect.x > mapWidth - rect.width) rect.x = mapWidth - rect.width;
        
        // Apply different maximum height limits depending on whether double jump was used
        float currentMaxHeight = hasDoubleJumped ? doubleJumpMaxHeight : singleJumpMaxHeight;
        
        // Ensure character doesn't go too high - limit the maximum jump height
        if (rect.y < currentMaxHeight) {
            rect.y = currentMaxHeight;
            velocity.y = 0; // Stop upward movement when hitting the ceiling
        }
    }

    // Helper method to handle taking damage.
    void heal(int healingAmount) {
        currentHealth += healingAmount;
        if (currentHealth > maxHealth) {
            currentHealth = maxHealth;  // Cap health at maximum.
        }
    }

    // Helper method to check for healing input.
    void checkForHealing() {
        if (IsKeyPressed(KEY_H)) {
            heal(10);  // Heal by 10 points.
        }
    }

    // Helper method to check for damage input (for testing).
    void checkForDamage() {
        if (IsKeyPressed(KEY_K)) {
            takeDamage(10);  // Take 10 damage.
        }
    }

    // Helper method to update the collision boxes of the character.
    void updateCollisionBoxes() {
        // Only update collision boxes if they exist.
        if (!collisionBoxes.empty()) {
            
            // Update the body collision box
            if (collisionBoxes.size() > 0) {
                // Base body box
                float bodyOffsetX = 15.0f * SPRITE_SCALE;
                float bodyOffsetY = 15.0f * SPRITE_SCALE;
                float bodyWidth = rect.width - (30.0f * SPRITE_SCALE);
                float bodyHeight = rect.height - (15.0f * SPRITE_SCALE);
                
                // If dashing, extend the collision box in the direction of movement
                if (isDashing) {
                    // Extend the box in the direction of dash
                    if (direction == RIGHT) {
                        bodyWidth += 10.0f * SPRITE_SCALE;
                    } else {
                        bodyOffsetX -= 10.0f * SPRITE_SCALE;
                        bodyWidth += 10.0f * SPRITE_SCALE;
                    }
                }
                
                collisionBoxes[0].rect = {rect.x + bodyOffsetX, rect.y + bodyOffsetY, bodyWidth, bodyHeight};
                collisionBoxes[0].active = (state != DEAD_STATE);
            }
            
            // Update attack collision box
            if (collisionBoxes.size() > 1) {
                float attackOffsetX = (direction == RIGHT) ? rect.width - (16.0f * SPRITE_SCALE) : 0;
                float attackOffsetY = 24.0f * SPRITE_SCALE;
                float attackSize = 32.0f * SPRITE_SCALE;
                
                collisionBoxes[1].rect = {rect.x + attackOffsetX, rect.y + attackOffsetY, attackSize, attackSize};
                collisionBoxes[1].active = (state == ATTACK_STATE && animations[state].currentFrame >= 2 && animations[state].currentFrame <= 4);
                
                // Update hurtbox collision box
                if (collisionBoxes.size() > 2) {
                    float hurtboxOffsetX = 20.0f * SPRITE_SCALE;
                    float hurtboxOffsetY = 20.0f * SPRITE_SCALE;
                    float hurtboxWidth = rect.width - (40.0f * SPRITE_SCALE);
                    float hurtboxHeight = rect.height - (24.0f * SPRITE_SCALE);
                    
                    collisionBoxes[2].rect = {rect.x + hurtboxOffsetX, rect.y + hurtboxOffsetY, hurtboxWidth, hurtboxHeight};
                    collisionBoxes[2].active = (state != DEAD_STATE);
                }
            }
        }
    }

    // Helper function to play the dash sound with proper volume
    void playDashSound() {
        if (dashSound.frameCount > 0) {
            // Play the sound (volume already set through setSoundVolumes)
            PlaySound(dashSound);
            
            // Debug output
            printf("Playing dash sound (volume: %.2f)\n", dashSoundVolume);
        } else {
            printf("Dash sound not loaded properly!\n");
        }
    }

    // Helper function to try downloading the dash sound file
    bool tryDownloadDashSound() {
        printf("Attempting to download dash sound file...\n");
        
        // Create sounds directory if it doesn't exist
        if (!DirectoryExists("sounds")) {
            #if defined(_WIN32)
                system("mkdir sounds");
            #else
                system("mkdir -p sounds");
            #endif
        }
        
        // Try to download the file using curl (if available)
        #if defined(_WIN32)
            system("curl -s -o sounds/whoosh\\ \\(phaser\\).wav https://freesound.org/data/previews/320/320654_5260872-lq.mp3 > nul 2>&1");
        #else
            system("curl -s -o 'sounds/whoosh (phaser).wav' https://freesound.org/data/previews/320/320654_5260872-lq.mp3 > /dev/null 2>&1");
        #endif
        
        // Check if the download was successful
        dashSound = LoadSound("sounds/whoosh (phaser).wav");
        if (dashSound.frameCount > 0) {
            printf("Successfully downloaded dash sound file!\n");
            return true;
        }
        
        printf("Failed to download dash sound file.\n");
        return false;
    }

    // Helper function to create a simple sound file if the dash sound is not found
    void createDefaultDashSound() {
        // Create a sounds directory if it doesn't exist
        if (!DirectoryExists("2DGame/assets/sounds")) {
            printf("Creating sounds directory...\n");
            bool success = false;
            #if defined(_WIN32)
                success = (system("mkdir 2DGame\\assets\\sounds") == 0);
            #else
                success = (system("mkdir -p 2DGame/assets/sounds") == 0);
            #endif
            
            if (!success) {
                printf("Failed to create sounds directory!\n");
                return;
            }
        }
        
        // Create a simple wave file with a whoosh sound
        Wave wave = { 0 };
        wave.frameCount = 22050;    // 0.5 seconds at 44100 Hz
        wave.sampleRate = 44100;
        wave.sampleSize = 16;
        wave.channels = 1;
        
        // Allocate memory for wave data
        wave.data = malloc(wave.frameCount * wave.channels * wave.sampleSize/8);
        
        // Generate a simple whoosh sound (sine wave with decreasing frequency)
        float frequency = 800.0f;
        float volume = 0.5f;
        
        for (unsigned int i = 0; i < wave.frameCount; i++) {
            // Decrease frequency over time for whoosh effect
            frequency = 800.0f - (float)i * 700.0f / (float)wave.frameCount;
            if (frequency < 100.0f) frequency = 100.0f;
            
            // Decrease volume over time
            volume = 0.5f - (float)i * 0.5f / (float)wave.frameCount;
            
            // Calculate sample value
            short sample = (short)(32000.0f * volume * sinf(2.0f * PI * frequency * (float)i / (float)wave.sampleRate));
            
            // Write sample to wave data
            ((short *)wave.data)[i] = sample;
        }
        
        // Export wave as sound file
        printf("Creating default dash sound file...\n");
        ExportWave(wave, "2DGame/assets/sounds/whoosh (phaser).wav");
        
        // Load the newly created sound
        dashSound = LoadSound("2DGame/assets/sounds/whoosh (phaser).wav");
        
        // Free wave data
        UnloadWave(wave);
    }

    // Helper function to set volume for all sound effects
    void setSoundVolumes(float volume) {
        // Clamp volume between 0.0 and 1.0
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        
        // Store the dash sound volume for future reference
        dashSoundVolume = volume;
        
        // Set volume for all sound effects
        if (attackSound.frameCount > 0) SetSoundVolume(attackSound, volume);
        if (jumpSound.frameCount > 0) SetSoundVolume(jumpSound, volume);
        if (hurtSound.frameCount > 0) SetSoundVolume(hurtSound, volume);
        if (runSound.frameCount > 0) SetSoundVolume(runSound, volume);
        if (deadSound.frameCount > 0) SetSoundVolume(deadSound, volume);
        if (landSound.frameCount > 0) SetSoundVolume(landSound, volume);
        if (dashSound.frameCount > 0) SetSoundVolume(dashSound, volume);
    }

    void loadTextures() {
        // Clear any existing textures
        for (auto& sprite : sprites) {
            if (sprite.id != 0) {
                UnloadTexture(sprite);
            }
        }
        
        printf("Loading Samurai textures...\n");
        
        // Get current working directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            printf("Error getting current working directory\n");
            return;
        }
        
        // Try different path formats
        const char* pathFormats[] = {
            "%s/2DGame/assets/Samurai/%s",    // Absolute with 2DGame prefix
            "%s/assets/Samurai/%s",           // Absolute without 2DGame prefix
            "2DGame/assets/Samurai/%s",       // Relative with 2DGame prefix
            "assets/Samurai/%s"               // Relative without 2DGame prefix
        };
        
        // Resize sprites vector
        sprites.resize(6);
        
        // List of filenames to load
        const char* fileNames[] = {
            "Dead.png", "Attack_1.png", "Hurt.png", 
            "Idle.png", "Jump.png", "Run.png"
        };
        
        // Corresponding sprite indices
        int spriteIndices[] = {
            DEAD_STATE, ATTACK_STATE, HURT_STATE,
            IDLE_STATE, JUMP_STATE, RUN_STATE
        };
        
        int loadedCount = 0;
        
        // Try to load each texture with different path formats
        for (int i = 0; i < 6; i++) {
            bool loaded = false;
            
            // Try each path format
            for (int p = 0; p < 4 && !loaded; p++) {
                char fullPath[PATH_MAX];
                if (p < 2) {
                    // Absolute paths
                    snprintf(fullPath, sizeof(fullPath), pathFormats[p], cwd, fileNames[i]);
                } else {
                    // Relative paths
                    snprintf(fullPath, sizeof(fullPath), pathFormats[p], fileNames[i]);
                }
                
                // Check if file exists before loading
                if (FileExists(fullPath)) {
                    sprites[spriteIndices[i]] = LoadTexture(fullPath);
                    
                    if (sprites[spriteIndices[i]].id != 0) {
                        printf("Loaded %s: %dx%d\n", 
                               fileNames[i], 
                               sprites[spriteIndices[i]].width, 
                               sprites[spriteIndices[i]].height);
                        loaded = true;
                        loadedCount++;
                        break;
                    }
                }
            }
            
            if (!loaded) {
                printf("Failed to load texture: %s\n", fileNames[i]);
            }
        }
        
        // Print summary
        printf("Samurai textures loaded: %d/6\n", loadedCount);
    }

public:
    bool isDead = false; // Flag to indicate if the samurai is dead.
    bool showCollisionBoxes = false; // Flag to enable/disable collision box drawing
    bool isDashing = false; // Flag to indicate if the character is currently dashing

    // Accessors for double jump height
    float getDoubleJumpHeight() const {
        return -doubleJumpHeight; // Return positive value for easier understanding
    }
    
    void setDoubleJumpHeight(float height) {
        doubleJumpHeight = -height; // Store as negative value for upward velocity
    }
    
    // Accessor for dash sound volume (kept for compatibility)
    void setDashSoundVolume(float volume) {
        // Set volume for all sounds to maintain consistency
        setSoundVolumes(volume);
    }
    
    float getDashSoundVolume() const {
        return dashSoundVolume;
    }

    // Constructor initializing the Samurai's properties and animations
    Samurai(float x, float y, float groundLevel) {
        rect = (Rectangle){x, y, 64.0f * SPRITE_SCALE, 64.0f * SPRITE_SCALE}; // Scale the sprite size
        velocity = (Vector2){0.0f, 0.0f}; // Initialize velocity.
        direction = RIGHT; // Default facing direction.
        state = IDLE_STATE; // Start in idle state.
        this->groundLevel = groundLevel; // Set initial ground level.
        currentHealth = maxHealth = 100;
        isDead = false;
        wasInAir = false;
        
        // Set maximum jump heights
        singleJumpMaxHeight = groundLevel - 150.0f; // 150 pixels above ground level
        doubleJumpMaxHeight = groundLevel - 300.0f; // 300 pixels above ground level (double height)

        // Load textures for each state.
        loadTextures();

        // Initialize animations for each state.
        animations = {
            {0, 2, 0, 0, 0.2f, 0.2f, ONESHOT},    // DEAD_STATE - Changed to use all 3 frames (0, 1, 2)
            {0, 5, 0, 0, 0.1f, 0.1f, ONESHOT},  // ATTACK_STATE
            {0, 2, 0, 0, 0.2f, 0.2f, ONESHOT},  // HURT_STATE
            {0, 5, 0, 0, 0.2f, 0.2f, LOOP},     // IDLE_STATE
            {0, 11, 0, 0, 0.1f, 0.1f, ONESHOT}, // JUMP_STATE - 12 frames (0-11) based on 1536/128 = 12
            {0, 7, 0, 0, 0.1f, 0.1f, LOOP}      // RUN_STATE
        };

        // Load sound effects with error checking
        attackSound = LoadSound("2DGame/assets/sounds/attack.mp3");
        jumpSound = LoadSound("2DGame/assets/sounds/jump.mp3");
        hurtSound = LoadSound("2DGame/assets/sounds/hurt.mp3");
        runSound = LoadSound("2DGame/assets/sounds/run.mp3");
        deadSound = LoadSound("2DGame/assets/sounds/dead.mp3");
        landSound = LoadSound("2DGame/assets/sounds/land.mp3");
        
        // Try to load the dash sound from various possible locations
        dashSound = LoadSound("2DGame/assets/sounds/whoosh (phaser).wav");
        if (dashSound.frameCount > 0) {
            printf("Loaded dash sound from: 2DGame/assets/sounds/whoosh (phaser).wav\n");
        } else {
            // Try alternative paths
            dashSound = LoadSound("2DGame/assets/sounds/misc/whoosh (phaser).wav");
            if (dashSound.frameCount > 0) {
                printf("Loaded dash sound from: 2DGame/assets/sounds/misc/whoosh (phaser).wav\n");
            } else {
                dashSound = LoadSound("2DGame/assets/sounds/whoosh(phaser).wav");
                if (dashSound.frameCount > 0) {
                    printf("Loaded dash sound from: 2DGame/assets/sounds/whoosh(phaser).wav\n");
                } else {
                    // If all paths fail, try without spaces in filename
                    dashSound = LoadSound("2DGame/assets/sounds/misc/whoosh(phaser).wav");
                    if (dashSound.frameCount > 0) {
                        printf("Loaded dash sound from: 2DGame/assets/sounds/misc/whoosh(phaser).wav\n");
                    } else {
                        printf("Warning: Could not load dash sound effect! Trying to download...\n");
                        
                        // Try to download the sound file
                        if (!tryDownloadDashSound()) {
                            // If download fails, create a default sound
                            printf("Creating a default dash sound...\n");
                            createDefaultDashSound();
                        }
                    }
                }
            }
        }

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

        // Initialize dash variables
        isDashing = false;
        dashTimer = 0.0f;
        canDash = true;
        dashCooldownTimer = 0.0f;
        dashSoundVolume = 0.8f; // Set default volume to 80%
    }

    // Destructor to clean up resources
    ~Samurai() {
        for (auto& sprite : sprites) {
            UnloadTexture(sprite); // Unload textures from memory when done.
        }
        
        // Unload sounds - make sure they're valid before unloading
        if (attackSound.frameCount > 0) UnloadSound(attackSound);
        if (jumpSound.frameCount > 0) UnloadSound(jumpSound);
        if (hurtSound.frameCount > 0) UnloadSound(hurtSound);
        if (runSound.frameCount > 0) UnloadSound(runSound);
        if (deadSound.frameCount > 0) UnloadSound(deadSound);
        if (landSound.frameCount > 0) UnloadSound(landSound);
        if (dashSound.frameCount > 0) UnloadSound(dashSound);
    }

    // Draw the character.
    void draw() const {
        if (sprites.size() <= state) {
            return; // Safety check
        }
        
        // Get the frame width for rendering
        float frameWidth = sprites[state].width / (animations[state].lastFrame + 1);
        
        // Calculate source rectangle for the current frame
        Rectangle source = {
            animations[state].currentFrame * frameWidth,
            0.0f,
            direction == RIGHT ? frameWidth : -frameWidth,
            (float)sprites[state].height
        };
        
        // Calculate destination rectangle for rendering
        Rectangle dest = {
            rect.x,
            rect.y,
            rect.width,
            rect.height
        };
        
        // Draw dash trail effect when dashing
        if (isDashing) {
            // Draw a few fading copies of the character behind the main sprite
            for (int i = 1; i <= 3; i++) {
                float offsetX = (direction == RIGHT) ? -i * 10.0f : i * 10.0f;
                Rectangle trailDest = {
                    rect.x + offsetX,
                    rect.y,
                    rect.width,
                    rect.height
                };
                
                // Calculate alpha based on distance (further = more transparent)
                float alpha = 0.7f - (i * 0.2f);
                Color trailTint = {255, 255, 255, (unsigned char)(alpha * 255)};
                
                DrawTexturePro(sprites[state], source, trailDest, (Vector2){0, 0}, 0.0f, trailTint);
            }
        }
        
        // Apply visual effect for invincibility frames
        Color tint = WHITE;
        if (isInvincible) {
            // Flash the character by alternating transparency
            float flashFrequency = 10.0f; // Higher value = faster flashing
            if (fmodf(invincibilityTimer * flashFrequency, 1.0f) > 0.5f) {
                tint = (Color){255, 255, 255, 128}; // Half transparent
            }
        }
        
        // Draw the sprite
        DrawTexturePro(sprites[state], source, dest, (Vector2){0, 0}, 0.0f, tint);
        
        // Draw collision boxes for debugging
        if (showCollisionBoxes) {
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
        
        // Draw health bar above the character instead of top left corner
        float healthBarWidth = rect.width;
        float healthBarHeight = 5.0f;
        float healthPercentage = (float)currentHealth / maxHealth;
        
        // Draw health bar background (red)
        DrawRectangle(
            rect.x, 
            rect.y - healthBarHeight - 5, 
            healthBarWidth, 
            healthBarHeight, 
            RED
        );
        
        // Draw current health (green)
        DrawRectangle(
            rect.x, 
            rect.y - healthBarHeight - 5, 
            healthBarWidth * healthPercentage, 
            healthBarHeight, 
            GREEN
        );
    }

    // Update the Samurai's state and position
    void updateSamurai() {
        float deltaTime = GetFrameTime();
        
        // Always update animation regardless of whether the samurai is dead or alive
        updateAnimation(deltaTime);
        
        // Update invincibility timer if active
        if (isInvincible) {
            invincibilityTimer -= deltaTime;
            if (invincibilityTimer <= 0.0f) {
                isInvincible = false;
                invincibilityTimer = 0.0f;
            }
        }
        
        // Only handle movement if the samurai is alive
        if (!isDead) {
            move(deltaTime);
        }
        
        // Always update collision boxes
        updateCollisionBoxes();
        
        // Always check for healing and damage
        checkForHealing();
        checkForDamage();
    }

    // Get the Samurai's rectangle for collision detection
    Rectangle getRect() const {
        return rect;
    }

    // Get the Samurai's current health
    int getHealth() const {
        return currentHealth;
    }

    // Get a collision box of a specific type
    CollisionBox* getCollisionBox(CollisionBoxType type) {
        for (auto& box : collisionBoxes) {
            if (box.type == type) {
                return &box;
            }
        }
        return nullptr;
    }

    // Take damage when hit by an enemy
    void takeDamage(int damage) {
        // Skip damage if currently invincible
        if (isInvincible) return;
            
        if (!isDead && state != HURT_STATE) {
            currentHealth -= damage;
            if (currentHealth <= 0) {
                currentHealth = 0;
                state = DEAD_STATE;
                isDead = true;
                animations[state].currentFrame = 0;  // Reset death animation to start from the beginning
                animations[state].timer = 0;  // Reset timer for smooth animation
                if (deadSound.frameCount > 0) {
                    PlaySound(deadSound);
                }
            } else {
                state = HURT_STATE;
                animations[state].currentFrame = 0;  // Reset hurt animation.
                if (hurtSound.frameCount > 0) {
                    PlaySound(hurtSound);
                }
                
                // Activate invincibility frames
                isInvincible = true;
                invincibilityTimer = invincibilityDuration;
            }
        }
    }
};