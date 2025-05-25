#include "raylib.h"
#include "raymath.h"
#include "CollisionSystem.h"
#include "Samurai.h"
#include "Demon.h"
#include <iostream>
#include <vector>
#include <stdlib.h> // For exit()
#include <unistd.h> // For getcwd()
#include <limits.h> // For PATH_MAX
#include <cstdio>
#include "StartScreen.h"

#include <functional>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Define RAYTMX_IMPLEMENTATION to include the implementation of the library
#define RAYTMX_IMPLEMENTATION
#include "raytmx.h"

// Define the global variable for collision box visibility
bool showCollisionBoxes = false;

// Global audio variables
Music backgroundMusic = { 0 };
Music menuMusic = { 0 };
float masterVolume = 0.7f;

// Global background texture

// Dialogue System for Room3
bool showDialogue = false;
std::string dialogueText;
float dialogueTimer = 0.0f;
float dialogueDuration = 4.0f; // seconds

std::vector<std::string> randomDialogues = {
    "Sorry, the demon you seek is in another portal!",
    "You've reached the wrong realm, try again!",
    "Wrong portal, warrior. Your demon lies elsewhere.",
    "Nope, no demons here. Just regrets.",
    "You must seek the next portal, brave samurai."
};

void triggerRoom3Dialogue() {
    // Set dialogue state
    showDialogue = true;
    
    // Select a random dialogue message
    dialogueText = randomDialogues[GetRandomValue(0, randomDialogues.size() - 1)];
    
    // Reset timer to start counting up
    dialogueTimer = 0.0f;
    
    // Print debug information
    printf("Dialogue triggered: %s\n", dialogueText.c_str());
}

Texture2D backgroundTexture = { 0 };
Camera2D camera = { 0 };

TmxMap* map = NULL;

enum GameState {START_SCREEN, MAIN_GAME, EXIT};
bool isPaused = false;
bool isComplete = false;
bool gameover = false;

// Fade transition on map switch
bool isTransitioning = false;
float transitionAlpha = 0.0f;
bool transitionFadeIn = false;

std::function<void()> transitionAction;
void startTransition(std::function<void()> action) {
    isTransitioning = true;
    transitionAlpha = 0.0f;
    transitionFadeIn = false;
    transitionAction = action;
}

// Helper function to check collision between two collision boxes
bool checkCharacterCollision(const CollisionBox& box1, const CollisionBox& box2) {
    if (box1.active && box2.active) {
        return CheckCollisionRecs(box1.rect, box2.rect);
    }
    return false;
}

// Helper function to handle attack collision and damage
void handleAttackCollision(CollisionBox* attackBox, CollisionBox* hurtBox, int& health, int damage) {
    if (attackBox && hurtBox && attackBox->active && hurtBox->active && 
        checkCharacterCollision(*attackBox, *hurtBox)) {
        health -= damage;
        if (health < 0) health = 0;
        
        // Deactivate attack box to prevent multiple hits from the same attack
        attackBox->active = false;
    }
}

// Custom exit function that bypasses normal cleanup
void safeExit() {
    // Unload audio resources
    if (backgroundMusic.ctxData != NULL) {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
    }
    
    // Unload the background texture
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
    }

    // Clean up Raylib
    CloseAudioDevice();
    CloseWindow();
    
    // Exit directly without going through normal cleanup
    _Exit(0); // Use _Exit instead of exit to bypass any atexit handlers
}

// Function to draw collision boxes for debugging
void drawCollisionBox(const CollisionBox& box) {
    if (!box.active) return;
    
    Color color;
    switch (box.type) {
        case BODY:
            color = BLUE;
            break;
        case ATTACK:
            color = RED;
            break;
        case HURTBOX:
            color = GREEN;
            break;
        default:
            color = YELLOW;
            break;
    }
    
    // Draw the collision box as a rectangle outline
    DrawRectangleLines(box.rect.x, box.rect.y, box.rect.width, box.rect.height, color);
    
    // Draw a small label to identify the box type
    const char* label = "";
    switch (box.type) {
        case BODY: label = "BODY"; break;
        case ATTACK: label = "ATTACK"; break;
        case HURTBOX: label = "HURT"; break;
        default: label = "UNKNOWN"; break;
    }
    
    DrawText(label, box.rect.x, box.rect.y - 15, 10, color);
}

// Function to check if a file exists
bool fileExists(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void loadLevel() {
    map = LoadTMX("maps/LevelDesign.tmx");
    if (!map) {
        printf("Failed to Load TMX File.\n");
        exit (1);
    } else {
        printf("Loaded TMX File.");
    }

    // Loop through tilesets (assuming map->tilesets is a pointer to an array of TmxTileset)
    for (int i = 0; i < map->tilesetsLength; i++) {
        TmxTileset* tileset = &map->tilesets[i];  // Accessing tileset by pointer

        // Check if the image source is valid (not empty)
        if (tileset->image.source[0] != '\0') {  // Using image.source to check validity
            Texture2D tilesetTexture = LoadTexture("maps/16 x16 Purple Dungeon Sprite Sheet.png");
            if (tilesetTexture.id == 0) {
                std::cout << "Error loading tileset image" << std::endl;
            }
        }
    }
}

void renderLevel() {
    if (map) {
        DrawTMX(map, &camera, 0, 0, WHITE);
    }
}

void checkTileCollisions(TmxMap* map, Samurai& player) {
    for (unsigned int i = 0; i < map->layersLength; i++) {
      TraceLog(LOG_DEBUG, "current layer is %d: %s", i, map->layers[i].name);
      if (strcmp(map->layers[i].name, "Object Layer 1") == 0 && map->layers[i].type == LAYER_TYPE_OBJECT_GROUP) 
      {
          TmxObject col;
          if (CheckCollisionTMXObjectGroupRec(map->layers[i].exact.objectGroup, player.getRect(), &col))
          {
            TraceLog(LOG_DEBUG, "We've made contact!");
            
            Vector2 newVel = player.getVelocity();
            newVel.y = 0;
            player.setVelocity(newVel);

            Rectangle newRect = player.getRect();
            newRect.y = (col.aabb.y - newRect.height);
            player.setRect(newRect);

            if (player.isJumping()) {
                player.land();
            }
          }
      }
    }
    return;
} 

int main() 
{
    // Print current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
    
    // Set up error handling
    SetTraceLogLevel(LOG_WARNING);
    
    // Initialize window
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "2D Game");

    // Define floor level to match where the non-zero tiles (floor tiles) are in Room1.tmx
    // This value is used for all characters to ensure consistent vertical positioning
    const float floorLevel = 10000.0f; // Exact floor level matching the non-zero floor tiles in TMX
    const float floorHeight = 50.0f; // Height of the floor rectangle if needed
    
    // Loading the Background.
    Texture2D background = LoadTexture("maps/Dungeon_brick_wall_purple.png.png");

    // Background Scale Factors.
    float scalebgx = (float)screenWidth / (float)background.width;
    float scalebgy = (float)screenHeight / (float)background.height;
    float scalebg = (scalebgx < scalebgy) ? scalebgx : scalebgy;
    scalebg /= 4.5f;

    // Positions for Background Positions.
    float bgposX = ((screenWidth - background.width * scalebg) / 2) - 600;
    float bgposY = ((screenHeight - background.height * scalebg) / 2) - 210;
    
    // Horizontal and Vertical Sliders for Background.
    int scaledW = background.width * scalebg;
    int scaledH = background.height * scalebg;
    int tilesX = (screenWidth / scaledW) + 50;
    int tilesY = (screenHeight / scaledH) + 15;
    
    // Initialize audio device before loading music
    InitAudioDevice();

    // Load Music.
    backgroundMusic = LoadMusicStream("music/03. Hunter's Dream.mp3");
    menuMusic = LoadMusicStream("music/Soul Of Cinder.mp3");
    
    SetTargetFPS(60);
    
    // Initialize camera
    camera.target = (Vector2){ 100, 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 3.3f;  // Zoom in for better visibility.

    // Initialize characters using stack allocation - all characters now use the same floorLevel
    Samurai samurai(510, 2223, floorLevel);
    
    // Don't delete this. This is for teleporting to the second main level.
    //samuraiRect.x >= 18760 && samuraiRect.x <= 18840 && samuraiRect.y >= 3660
    //Samurai samurai(18760, 3660, floorLevel);
    
    // Initialize dash sound volume to match master volume
    samurai.setDashSoundVolume(0.8f * masterVolume);

    StartScreen startScreen;
    GameState gameState = START_SCREEN;

    loadLevel();

    // Start playing menu music initially
    PlayMusicStream(menuMusic);
    SetMusicVolume(menuMusic, 0.5f * masterVolume);
    bool isPlayingMenuMusic = true;

    // Check which map are we in
    bool mapSwitchedToRoom2 = false;

    bool mapSwitchedToRoom3 = false;
    bool mapSwitchedToRoom4 = false;
    
    bool mapSwitchedToMainLevel2 = false;
    
    //New map for main level 2
    bool mapSwitchedToRoom5 = false;
    bool mapSwitchedToRoom6 = false;
    bool mapSwitchedToRoom7 = false;
    bool mapSwitchedToRoom8 = false;

    
    // Create a demon for Room2
    Demon* demon = nullptr;

    // Game loop
    while (!WindowShouldClose()) {
        // Update currently playing music
        UpdateMusicStream(isPlayingMenuMusic ? menuMusic : backgroundMusic);

        // Handle game state updates based on current game state
        switch(gameState) {

            case EXIT: {
                // Handle exit (save state, cleanup, etc.)
                safeExit();
                break;
            }
            case START_SCREEN: {
                if (!isPlayingMenuMusic) {
                    PlayMusicStream(menuMusic);
                    isPlayingMenuMusic = true;
                }

                startScreen.Update();

                if (startScreen.ShouldStartGame()) {
                    gameState = MAIN_GAME;  // Transition to main game
                }
                if (startScreen.ShouldExitGame()) {
                    gameState = EXIT;  // Exit the game
                }

                BeginDrawing();
                startScreen.Draw();  // Draw the start screen
                EndDrawing();
                break;
            }

            case MAIN_GAME: {
                if (isPlayingMenuMusic) {
                    PlayMusicStream(backgroundMusic);
                    isPlayingMenuMusic = false;
                }

                if (IsKeyPressed(KEY_P)) {
                    isPaused = !isPaused;
                }
                
                // Toggle music with M key
                if (IsKeyPressed(KEY_M)) {
                    if (IsMusicStreamPlaying(backgroundMusic)) {
                        PauseMusicStream(backgroundMusic);
                    } else {
                        ResumeMusicStream(backgroundMusic);
                    }
                }

                // Toggle collision box visibility with F1 key
                if (IsKeyPressed(KEY_F1)) {
                    showCollisionBoxes = !showCollisionBoxes;
                    printf("Collision boxes visibility: %s\n", showCollisionBoxes ? "ON" : "OFF");
                }

                // Get frame time for updates
                float deltaTime = GetFrameTime();

                if (!isPaused && !isComplete) {
                    // Update samurai character
                    samurai.updateSamurai();
                }

                // You are in the first main level
                if(!mapSwitchedToMainLevel2)
                {
                    samurai.deathBarrier();
                }
       
                // You are now in the second main level. Wow.
                if (mapSwitchedToMainLevel2)
                {
                    samurai.secondDeathBarrier();
                }

                // Get samurai position for collision detection
                Vector2 samuraiPos = {0, 0};
                CollisionBox* samuraiBody = samurai.getCollisionBox(BODY);
                if (samuraiBody && samuraiBody->active) {
                    samuraiPos.x = samuraiBody->rect.x + samuraiBody->rect.width / 2;
                    samuraiPos.y = samuraiBody->rect.y + samuraiBody->rect.height / 2;
                }
                
                // Check for collisions between Samurai's attack and enemies
                CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
                
                // Check for enemy attacks hitting Samurai
                CollisionBox* samuraiHurtbox = samurai.getCollisionBox(HURTBOX);

                checkTileCollisions(map, samurai);

                // Update camera to follow player, ensuring it stays within map boundaries
                Rectangle samuraiRect = samurai.getRect();
                
                float samuraiCenterX = samuraiRect.x + samuraiRect.width / 2;
                float samuraiCenterY = samuraiRect.y + samuraiRect.height / 2;
                
                if (!samurai.checkDeath()) {
                    camera.target = (Vector2){ samuraiPos.x, samuraiPos.y };
                    // Add some camera boundary checks to avoid the camera going out of bounds:
                    float halfScreenWidth = screenWidth / (2.0f * camera.zoom);
                    float halfScreenHeight = screenHeight / (2.0f * camera.zoom);
    
                    // Clamp X and Y positions with easing towards boundaries
                    camera.target.x = Lerp(camera.target.x, Clamp(camera.target.x, halfScreenWidth, map->width * map->tileWidth - halfScreenWidth), 0.1f);
                    camera.target.y = Lerp(camera.target.y, Clamp(camera.target.y, halfScreenHeight, map->height * map->tileHeight - halfScreenHeight), 0.1f);
    
                    // Ensure camera doesn't go out of bounds
                    if (camera.target.x < halfScreenWidth) camera.target.x = halfScreenWidth + 100;
                    if (camera.target.y < halfScreenHeight) camera.target.y = halfScreenHeight;
    
                    // Camera zoom controls
                    if (camera.target.x < halfScreenWidth) camera.target.x = halfScreenWidth;
                    if (camera.target.x > map->width * 16 - halfScreenWidth) camera.target.x = map->width * 16 - halfScreenWidth;
                    if (camera.target.y < halfScreenHeight) camera.target.y = halfScreenHeight;
                    if (camera.target.y > map->height * 16 - halfScreenHeight) camera.target.y = map->height * 16 - halfScreenHeight;
                } else {
                    // Stop moving the camera when the player is dead
                    camera.target = camera.target; // Keeps the camera locked in place
                }
                
                // Switching map :o

                // Main Level to Room2
                if (!mapSwitchedToMainLevel2 && !mapSwitchedToRoom2 && samuraiRect.x >= 920 && samuraiRect.x <= 930 && samuraiRect.y == 1502) 
                {
                    // Debug output to confirm portal detection
                    printf("Portal to Room2 detected! Player position: %.2f, %.2f\n", samuraiRect.x, samuraiRect.y);
                    startTransition([&]() 
                    {
                        mapSwitchedToRoom2 = true;

                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                    map = LoadTMX("maps/Room2.tmx");
                    if (!map) 
                    {
                        std::cerr << "Failed to load Room2.tmx!" << std::endl;
                    }

                    Rectangle newPos = samurai.getRect();

                    newPos.x = 540;  
                    newPos.y = 2222;
                    samurai.setRect(newPos);

                    camera.target = { newPos.x, newPos.y };
                    
                    // Create demon in Room2
                    if (demon == nullptr) {
                        Vector2 demonPos = { 1000.0f, 2165.0f }; // Position the demon in Room2 at same ground level as samurai
                        demon = new Demon(demonPos, 50.0f, 500);
                        std::cout << "Demon spawned in Room2" << std::endl;
                    }
                    });
                }

                
                // Room2 to Main Level 
                if (!mapSwitchedToMainLevel2 && mapSwitchedToRoom2 && samuraiRect.x >= 530 && samuraiRect.x <= 540 && samuraiRect.y >= 2170 && samuraiRect.y <= 2180) 
                {
                    // Debug output to confirm return portal detection
                    printf("Return portal detected! Player position: %.2f, %.2f\n", samuraiRect.x, samuraiRect.y);
                    startTransition([&]() 
                    {
                        mapSwitchedToRoom2 = false;
                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                        map = LoadTMX("maps/LevelDesign.tmx");
                        if (!map) 
                        {
                            std::cerr << "Failed to load LevelDesign.tmx!" << std::endl;
                        }

                        Rectangle newPos = samurai.getRect();
                        newPos.x = 920;
                        newPos.y = 1519.5;
                        samurai.setRect(newPos);

                        camera.target = { newPos.x, newPos.y };

                    });  
                }
                
                // Main Level to Room 3
                if (!mapSwitchedToMainLevel2 && !mapSwitchedToRoom3 && samuraiRect.x >= 5415 && samuraiRect.x <= 5435 && samuraiRect.y >= 877 && samuraiRect.y <= 878)

                {
                    printf("Portal to Room3 detected! Player position: %.2f, %.2f\n", samuraiRect.x, samuraiRect.y);
                    startTransition([&]() 
                    {

                        mapSwitchedToRoom3 = true; 

                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                        map = LoadTMX("maps/Room3.tmx"); // Load Room 3
                        if (!map) 
                        {
                            printf("Failed to load Room3.tmx\n");
                            safeExit(); // Exit if loading fails
                        } 
                        else 
                        {
                            printf("Loaded Room3.tmx successfully.\n");
                        }
                        
                        Rectangle newPos = samurai.getRect();
                        newPos.x = 1560; 
                        newPos.y = 2190.25;
                        samurai.setRect(newPos);
                    });
                }
                
                // Room 3 to Main Level 
                if (!mapSwitchedToMainLevel2 && mapSwitchedToRoom3 && samuraiRect.x >= 1540 && samuraiRect.x <= 1570 && samuraiRect.y >= 2173 && samuraiRect.y <= 2175) 
                {
                    printf("Portal back to LevelDesign detected! Player position: %.2f, %.2f\n", samuraiRect.x, samuraiRect.y);
                    startTransition([&]() 
                    {

                        mapSwitchedToRoom3 = false;
                        if (map) 
                        {
                            UnloadTMX(map);
                        }
                        map = LoadTMX("maps/LevelDesign.tmx"); // Load the main level
                        if (!map) 
                        {
                            printf("Failed to Load TMX File: LevelDesign.tmx\n");
                            safeExit(); 
                        } 
                        else 
                        {
                            printf("Loaded TMX File: LevelDesign.tmx\n");
                        }

                        Rectangle newPos = samurai.getRect();
                        newPos.x = 5895;
                        newPos.y = 892;
                        samurai.setRect(newPos);

                        camera.target = { newPos.x, newPos.y };
                    });  
                }
                
                // Main Level to Room 4 
                if (!mapSwitchedToMainLevel2 && !mapSwitchedToRoom4 && samuraiRect.x >= 8300 && samuraiRect.x <= 8320 && samuraiRect.y >= 2173 && samuraiRect.y <= 2176) 
                {
                    startTransition([&]() 
                    {
                        mapSwitchedToRoom4 = true;

                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                    map = LoadTMX("maps/Room4.tmx");
                    if (!map) 
                    {
                        std::cerr << "Failed to load Room4.tmx!" << std::endl;
                    }

                    Rectangle newPos = samurai.getRect();
                    newPos.x = 665;
                    newPos.y = 2222;
                    samurai.setRect(newPos);

                    camera.target = { newPos.x, newPos.y };
                    });  
                }
                
                // Room 4 to Main Level
                if (!mapSwitchedToMainLevel2 && mapSwitchedToRoom4 && samuraiRect.x >= 3050 && samuraiRect.x <= 3070 && samuraiRect.y >= 2170.00) 
                {
                    startTransition([&]() 
                    {
                        mapSwitchedToRoom4 = false;

                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                    map = LoadTMX("maps/LevelDesign.tmx");
                    if (!map) 
                    {
                        std::cerr << "Failed to load LevelDesign.tmx!" << std::endl;
                    }

                    Rectangle newPos = samurai.getRect();

                    newPos.x = 9385;
                    newPos.y = 2062.25;
                    samurai.setRect(newPos);

                    camera.target = { newPos.x, newPos.y };
                    });  
                }

                // Main Level to Main Level 2
                if (!mapSwitchedToMainLevel2 && samuraiRect.x >= 18760 && samuraiRect.x <= 18840 && samuraiRect.y >= 3660) 
                {
                    startTransition([&]() 
                    {
                        mapSwitchedToMainLevel2 = true;

                        if (map) 
                        {
                            UnloadTMX(map);
                        }

                    map = LoadTMX("maps/LevelDesign2.tmx");
                    if (!map) 
                    {
                        std::cerr << "Failed to load LevelDesign2.tmx!" << std::endl;
                    }

                    Rectangle newPos = samurai.getRect();
                    newPos.x = 200;
                    newPos.y = 1500;
                    samurai.setRect(newPos);

                    camera.target = { newPos.x, newPos.y };
                    });  
                }
                
                if (mapSwitchedToMainLevel2 && !mapSwitchedToRoom5 && 
                    samuraiRect.x > 4400 && samuraiRect.x < 4430 && 
                    samuraiRect.y > 2760 && samuraiRect.y < 2780) 
                   {
                    startTransition([&]() {
                    mapSwitchedToRoom5 = true;

                    // Unload current map
                    if (map) {
                     UnloadTMX(map);
                    }

                    // Load new TMX map (e.g., Room5)
                    
                    map = LoadTMX("maps/Lv2RoomOne.tmx");
                    if (!map) {
                        std::cerr << "Failed to load Room5.tmx!" << std::endl;
                    }
                    

                    // Set Samurai position to destination portal
                    Rectangle newPos = samurai.getRect();
                    newPos.x = 0;  // Adjust to coordinates of portal in Room5
                    newPos.y = 224;
                    samurai.setRect(newPos);

                     // Recenter camera
                    camera.target = { newPos.x, newPos.y };
                     });
                }

                // New portal (e.g., Room5 -> Level2)
                if (mapSwitchedToMainLevel2 && mapSwitchedToRoom5 &&
                    samuraiRect.x > 1000 && samuraiRect.x < 1100 &&
                    samuraiRect.y > 1200 && samuraiRect.y < 1300) 
                {
             
                    startTransition([&]() {
                    mapSwitchedToRoom5 = false;

                 if (map) {
                    UnloadTMX(map);
                }

                    map = LoadTMX("maps/LevelDesign2.tmx");

                    Rectangle newPos = samurai.getRect();
                    newPos.x = 3820;   // back to original portal
                    newPos.y = 1218.77;
                    samurai.setRect(newPos);

                    camera.target = { newPos.x, newPos.y };
                    });
                }


                // New portal (e.g., Level2 -> Room6)
                if (mapSwitchedToMainLevel2 && !mapSwitchedToRoom6 && 
                    samuraiRect.x > 5600 && samuraiRect.x < 5700 && 
                    samuraiRect.y > 3300 && samuraiRect.y < 3400) 
                   {
                    startTransition([&]() {
                    mapSwitchedToRoom6 = true;

                    // Unload current map
                    if (map) {
                     UnloadTMX(map);
                    }

                    // Load new TMX map 
                    
                    map = LoadTMX("maps/Lv2RoomTwo.tmx");
                    if (!map) {
                        std::cerr << "Failed to load Lv2RoomTwo.tmx!" << std::endl;
                    }
                    

                    // Set Samurai position to destination portal
                    Rectangle newPos = samurai.getRect();
                    newPos.x = 0;  // Adjust to coordinates of portal in Room5
                    newPos.y = 224;
                    samurai.setRect(newPos);

                     // Recenter camera
                    camera.target = { newPos.x, newPos.y };
                     });
                }


               // New portal (e.g., Room6 -> Level2)
                if (mapSwitchedToMainLevel2 && mapSwitchedToRoom6 && 
                    samuraiRect.x > 1600 && samuraiRect.x < 1610 && 
                    samuraiRect.y > 3300 && samuraiRect.y < 3500) 
                   {
                    startTransition([&]() {
                    mapSwitchedToRoom6 = false;

                    // Unload current map
                    if (map) {
                     UnloadTMX(map);
                    }

                    // Load new TMX map 
                    
                    map = LoadTMX("maps/LevelDesign2.tmx");
                    if (!map) {
                        std::cerr << "Failed to load Room5.tmx!" << std::endl;
                    }
                    

                    // Set Samurai position to destination portal
                    Rectangle newPos = samurai.getRect();
                    newPos.x = 8390;  // Adjust to coordinates of portal in Room6
                    newPos.y = 1313.78;
                    samurai.setRect(newPos);

                     // Recenter camera
                    camera.target = { newPos.x, newPos.y };
                     });
                }

                                
                                   
                // New portal (e.g., Level2 -> Room7)
                if (mapSwitchedToMainLevel2 && !mapSwitchedToRoom7 && 
                    samuraiRect.x > 7500 && samuraiRect.x < 7580 && 
                    samuraiRect.y > 2900 && samuraiRect.y < 3000) 
                   {
                    startTransition([&]() {
                    mapSwitchedToRoom5 = true;

                    // Unload current map
                    if (map) {
                     UnloadTMX(map);
                    }

                    // Load new TMX map (e.g., Room5)
                    
                    map = LoadTMX("maps/Lv2Room3.tmx");
                    if (!map) {
                        std::cerr << "Failed to load Room5.tmx!" << std::endl;
                    }
                    

                    // Set Samurai position to destination portal
                    Rectangle newPos = samurai.getRect();
                    newPos.x = 0;  // Adjust to coordinates of portal in Room5
                    newPos.y = 224;
                    samurai.setRect(newPos);

                     // Recenter camera
                    camera.target = { newPos.x, newPos.y };
                     });
                }

                // New portal (e.g., Level2 -> Room8)
                if (mapSwitchedToMainLevel2 && !mapSwitchedToRoom7 && 
                    samuraiRect.x > 9100 && samuraiRect.x < 9200 && 
                    samuraiRect.y > 2000 && samuraiRect.y < 2100) 
                   {
                    startTransition([&]() {
                    mapSwitchedToRoom5 = true;

                    // Unload current map
                    if (map) {
                     UnloadTMX(map);
                    }

                    // Load new TMX map (e.g., Room5)
                    
                    map = LoadTMX("maps/Lv2Room4.tmx");
                    if (!map) {
                        std::cerr << "Failed to load Room5.tmx!" << std::endl;
                    }
                    

                    // Set Samurai position to destination portal
                    Rectangle newPos = samurai.getRect();
                    newPos.x = 0;  // Adjust to coordinates of portal in Room5
                    newPos.y = 224;
                    samurai.setRect(newPos);

                     // Recenter camera
                    camera.target = { newPos.x, newPos.y };
                     });
                }
                
                if(samurai.checkDeath()) {
                    gameover = true;
                }
                

                if(mapSwitchedToMainLevel2 && samurai.getRect().x >= 12610 && samurai.getRect().x <= 12655 && samuraiRect.y >= 2304) {
                    isComplete = true;
                }
                
                // Begin drawing
                BeginDrawing();
                ClearBackground(BLACK);
                
                // 2D camera mode for proper drawing
                BeginMode2D(camera);
                
                // Draw Background.
                for (int x = 0; x < tilesX; x++) {
                    for (int y = 0; y < tilesY; y++) {
                        float posX = x * scaledW;
                        float posY = y * scaledH;
                        
                        DrawTextureEx(background, Vector2{posX + bgposX, posY + bgposY}, 0.0f, scalebg, GRAY);
                    }
                }
                
                renderLevel();
                
                // Draw Samurai.
                samurai.draw();
                
                // Update and draw demon if in Room2
                if (mapSwitchedToRoom2 && demon != nullptr) {
                    // Update demon animation
                    demon->updateAnimation();
                    
                    // Update demon AI behavior
                    if (!demon->isDead && !isPaused) {
                        // Get distance to player
                        Rectangle demonRect = demon->rect;
                        Vector2 demonPos = { demonRect.x + demonRect.width/2, demonRect.y + demonRect.height/2 };
                        Vector2 samuraiPos = { samuraiRect.x + samuraiRect.width/2, samuraiRect.y + samuraiRect.height/2 };
                        float distance = Vector2Distance(demonPos, samuraiPos);
                        
                        // Chase player if within range
                        if (distance < demon->chaseRange && distance > demon->attackRange) {
                            demon->state = WALK_DEMON;
                            demon->direction = (samuraiPos.x < demonPos.x) ? LEFT_DEMON : RIGHT_DEMON;
                            
                            // Move toward player
                            float moveDir = (demon->direction == LEFT_DEMON) ? -1.0f : 1.0f;
                            demon->velocity.x = moveDir * demon->moveSpeed * 100.0f;
                        } 
                        // Attack if close enough
                        else if (distance <= demon->attackRange) {
                            if (!demon->isAttacking) {
                                demon->attack();
                            }
                        }
                        // Idle if too far
                        else {
                            demon->state = IDLE_DEMON;
                            demon->velocity.x = 0;
                        }
                        
                        // Apply velocity
                        demon->applyVelocity();
                    }
                    
                    // Draw the demon
                    demon->draw();
                    
                    // Check for collision between Samurai's attack and demon
                    CollisionBox* samuraiAttack = samurai.getCollisionBox(ATTACK);
                    if (samuraiAttack && samuraiAttack->active) {
                        for (auto& box : demon->collisionBoxes) {
                            if (box.type == HURTBOX && box.active) {
                                if (checkCharacterCollision(*samuraiAttack, box)) {
                                    demon->takeDamage(25); // Samurai deals 25 damage
                                    //samuraiAttack->active = false; // Prevent multiple hits
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Check for collision between demon's attack and Samurai
                    CollisionBox* samuraiHurtbox = samurai.getCollisionBox(HURTBOX);
                    for (auto& box : demon->collisionBoxes) {
                        if (box.type == ATTACK && box.active) {
                            if (samuraiHurtbox && samuraiHurtbox->active) {
                                if (checkCharacterCollision(box, *samuraiHurtbox)) {
                                    // Check if samurai is blocking to reduce damage
                                    if (samurai.isBlocking()) {
                                        // Apply damage reduction when blocking (half damage)
                                        int reducedDamage = static_cast<int>(15 * samurai.getBlockDamageReduction());
                                        samurai.takeDamage(reducedDamage);
                                        std::cout << "Blocked attack! Reduced damage: " << reducedDamage << std::endl;
                                    } else {
                                        samurai.takeDamage(15); // Full damage when not blocking
                                    }
                                    box.active = false; // Prevent multiple hits
                                    break;
                                }
                            }
                        }
                    }
                }

                std::cout << "X: " << samurai.getRect().x << std::endl;
                std::cout << "Y: " << samurai.getRect().y << std::endl;

                // End camera mode and finalize drawing
                EndMode2D();
                
                // Draw dialogue textbox after 2D mode
                if (showDialogue) {
                    // Update dialogue timer
                    dialogueTimer += GetFrameTime();
                    
                    // Create a visually appealing dialogue box with fixed screen coordinates (not affected by camera)
                    int boxWidth = 800;
                    int boxHeight = 120; // Slightly taller for better visibility
                    int boxX = GetScreenWidth() / 2 - boxWidth / 2;
                    int boxY = 100; // Position at top of screen for better visibility

                    // Draw the dialogue box with a semi-transparent background and thicker border
                    DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(BLACK, 0.9f));
                    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
                    DrawRectangleLines(boxX+1, boxY+1, boxWidth-2, boxHeight-2, WHITE); // Double border for emphasis
                    
                    // Draw a title for the dialogue box
                    DrawText("RESIDENT GNOME", boxX + boxWidth/2 - MeasureText("MYSTERIOUS VOICE", 20)/2, boxY + 15, 20, GOLD);
                    
                    // Draw the dialogue text centered in the box
                    DrawText(dialogueText.c_str(), boxX + 20, boxY + 50, 24, WHITE);

                    // Print debug info when F2 is pressed
                    if (IsKeyPressed(KEY_F2)) {
                        printf("Dialogue active: %s (Timer: %.2f/%.2f)\n", 
                               dialogueText.c_str(), dialogueTimer, dialogueDuration);
                    }

                    // Hide dialogue after duration expires
                    if (dialogueTimer >= dialogueDuration) {
                        showDialogue = false;
                        dialogueTimer = 0.0f;
                        printf("Dialogue ended.\n");
                    }
                }



                // UI controls instructions
                int instructionsY = screenHeight - 1050;
                int lineHeight = 25;

                DrawText("GAME CONTROLS:", 10, instructionsY, 20, WHITE);
                DrawText("W or Up: Jump ", 10, instructionsY + lineHeight, 20, WHITE);
                DrawText("A/D or Left/Right: Move", 10, instructionsY + lineHeight*2, 20, WHITE);
                DrawText("Space: Attack", 10, instructionsY + lineHeight*3, 20, WHITE);
                DrawText("Double-tap A/D: Dash", 10, instructionsY + lineHeight*4, 20, WHITE);
                DrawText("M: Toggle music", 10, instructionsY + lineHeight*5, 20, WHITE);
                DrawText("P: Pause", 10, instructionsY + lineHeight*6, 20, WHITE);
                
                if (isPaused) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
                    DrawText("PAUSED", GetScreenWidth()/2 - 50, GetScreenHeight()/2 - 10, 30, WHITE);
                    DrawText("Press 'P' to resume", GetScreenWidth()/2 - 100, GetScreenHeight()/2 + 30, 20, WHITE);

                    // Define the exit button area
                    Rectangle exitButton = { static_cast<float>(GetScreenWidth()) / 2 - 75, 
                        static_cast<float>(GetScreenHeight()) / 2 + 60, 
                        150.0f, 40.0f };


                    // Draw the exit button
                    DrawRectangleRec(exitButton, DARKGRAY);
                    DrawText("Exit", GetScreenWidth()/2 - 20, GetScreenHeight()/2 + 70, 20, WHITE);

                    // Check if the exit button is clicked
                    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        safeExit();
                    }

                    samurai.pauseSounds();
                } else if(isComplete) {
                    // Draw completion screen with improved UI layout
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); // Darker background for emphasis

                    // Centered text
                    const int centerX = GetScreenWidth() / 2;
                    const int centerY = GetScreenHeight() / 2;

                    DrawText("GAME COMPLETED!", centerX - MeasureText("GAME COMPLETED!", 40) / 2, centerY - 100, 40, GOLD);
                    DrawText("Congratulations!", centerX - MeasureText("Congratulations!", 30) / 2, centerY - 50, 30, WHITE);
                    DrawText("Press 'E' to exit", centerX - MeasureText("Press 'E' to exit", 20) / 2, centerY + 20, 20, LIGHTGRAY);

                    // Define the exit button
                    Rectangle exitButton = { static_cast<float>(centerX - 100), 
                                            static_cast<float>(centerY + 60), 
                                            200.0f, 50.0f };

                    // Draw the exit button with a hover effect
                    Color exitButtonColor = CheckCollisionPointRec(GetMousePosition(), exitButton) ? LIGHTGRAY : DARKGRAY;
                    DrawRectangleRec(exitButton, exitButtonColor);
                    DrawText("Exit", centerX - MeasureText("Exit", 25) / 2, centerY + 75, 25, WHITE);

                    // Check if the exit button is clicked
                    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (startScreen.ShouldStartGame()) {
                            safeExit();
                        }
                    }

                    // Handle key press for exiting
                    if (IsKeyPressed(KEY_E)) {
                        if (startScreen.ShouldStartGame()) {
                            safeExit();
                        }
                    }
                } else if(gameover) {
                    // Draw Game Over screen
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); // Dark background overlay

                    // Center coordinates
                    const int centerX = GetScreenWidth() / 2;
                    const int centerY = GetScreenHeight() / 2;

                    // Draw Game Over text
                    DrawText("GAME OVER", centerX - MeasureText("GAME OVER", 50) / 2, centerY - 100, 50, RED);
                    DrawText("Better luck next time!", centerX - MeasureText("Better luck next time!", 25) / 2, centerY - 50, 25, WHITE);

                    // Define the Exit button
                    Rectangle exitButton = { static_cast<float>(centerX - 100), 
                                            static_cast<float>(centerY + 30), 
                                            200.0f, 50.0f };

                    // Draw the Exit button with hover effect
                    Color exitButtonColor = CheckCollisionPointRec(GetMousePosition(), exitButton) ? LIGHTGRAY : DARKGRAY;
                    DrawRectangleRec(exitButton, exitButtonColor);
                    DrawText("Exit", centerX - MeasureText("Exit", 25) / 2, centerY + 45, 25, WHITE);

                    // Check if the Exit button is clicked
                    if (CheckCollisionPointRec(GetMousePosition(), exitButton) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        safeExit(); // Close the game window
                    }

                    // Handle key press for exiting
                    if (IsKeyPressed(KEY_E)) {
                        safeExit();
                    }
                }
                else 
                {
                    samurai.resumeSound();
                }
                
                if (isTransitioning) 
                {
                    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, transitionAlpha));
                    if (!transitionFadeIn) 
                    {
                        transitionAlpha += 0.02f;
                        if (transitionAlpha >= 1.0f) 
                        {
                            transitionAlpha = 1.0f;

                            if (transitionAction) 
                            {
                                transitionAction();  // run the map change
                            }

                            transitionFadeIn = true;
                        }
                    } 
                    else 
                    {
                        transitionAlpha -= 0.02f;
                        if (transitionAlpha <= 0.0f) 
                        {
                            transitionAlpha = 0.0f;
                            isTransitioning = false;
                            
                            // Check if we just entered Room3 and trigger dialogue if needed
                            if (mapSwitchedToRoom3 && !showDialogue) {
                                triggerRoom3Dialogue();
                                printf("Dialogue triggered after transition: %s\n", dialogueText.c_str());
                            }
                        }
                    }
                }
                
                EndDrawing();


                break;
            }
        }
    }
}
