# The Forsaken Crypt

A Dark Souls-inspired dungeon crawler game where a samurai warrior battles demons in a mysterious crypt. Navigate through multiple rooms, engage in combat with enemies, and survive the challenges that await.

## Table of Contents
- [Game Overview](#game-overview)
- [Features](#features)
- [Installation](#installation)
- [Controls](#controls)
- [Game Mechanics](#game-mechanics)
- [Technical Implementation](#technical-implementation)
- [Assets](#assets)
- [Development](#development)

## Game Overview

The Forsaken Crypt is an atmospheric 2D action game where players control a samurai character navigating through a series of dungeon rooms while battling demon enemies. The game features a Dark Souls-inspired aesthetic with challenging combat and exploration elements.

## Features

- **Atmospheric Start Screen**: Dark Souls-inspired menu with interactive buttons that change appearance on hover
- **Character System**: Fully implemented character classes with inheritance for different entity types
- **Combat System**: Attack and defense mechanics with collision detection
- **Health Management**: Visual health bars for player and enemies
- **Multiple Rooms**: Navigate through different dungeon rooms with unique layouts
- **Enemy AI**: Demons with intelligent behavior that track and attack the player
- **Sound Effects**: Combat sounds, movement audio, and atmospheric music
- **Dialogue System**: Random dialogue messages in specific game areas
- **Room Transitions**: Smooth transitions between different areas of the dungeon

## Installation

### Prerequisites

- C++ compiler (GCC or Clang recommended)
- Raylib 4.5.0 or higher
- Make build system

### Building from Source

1. Clone the repository:
   ```
   git clone [repository-url]
   cd ForbiddenCryptGame
   ```

2. Compile the game using the provided Makefile:
   ```
   make
   ```

3. Run the compiled executable:
   ```
   ./game
   ```

### Build Options

The Makefile supports various platforms and configurations. By default, it builds for desktop platforms. You can customize the build by setting environment variables:

```
make PLATFORM=PLATFORM_DESKTOP
```

Supported platforms include:
- PLATFORM_DESKTOP
- PLATFORM_RPI
- PLATFORM_ANDROID
- PLATFORM_WEB

## Controls

- **Movement**: WASD or Arrow keys
- **Attack**: Left mouse button
- **Block/Shield**: Right mouse button
- **Jump**: Spacebar
- **Toggle Collision Boxes (Debug)**: F1
- **Pause Game**: Escape

## Game Mechanics

### Character System

The game implements a robust character system with a base `Character` class that defines common properties and behaviors:

- Health management with visual health bars
- Movement and positioning
- Collision detection with different collision box types (body, attack, hurtbox)
- Virtual methods for character-specific behaviors

Derived character classes include:

- **Samurai**: The player character with attack combos, blocking, and jumping abilities
- **Demon**: Enemy characters with AI-driven behavior that track and attack the player

### Combat

- **Attack System**: Characters can perform attacks that activate collision boxes
- **Damage System**: Health reduction when attack boxes collide with hurtboxes
- **Visual Feedback**: Animation effects for attacks, blocks, and damage

### Level Design

- Multiple interconnected rooms created with Tiled Map Editor
- Room transitions with fade effects
- Collision detection with map tiles for solid obstacles

## Technical Implementation

### Architecture

The game is built using a component-based architecture with the following key components:

- **Game Loop**: Managed in the main file (2dgame.cpp)
- **Character System**: Abstract base class with derived implementations
- **Collision System**: Handles different types of collision detection
- **Start Screen**: Manages the game's menu interface
- **Audio System**: Handles background music and sound effects

### Libraries

- **Raylib**: Core game development framework providing graphics, input, and audio functionality
- **RayTMX**: Extension for loading and rendering Tiled map files
- **RayMath**: Vector and matrix operations for game calculations

### Rendering

- Sprite-based rendering for characters and environment
- Camera system for following the player character
- Health bar UI elements
- Debug visualization for collision boxes

## Assets

### Graphics

- Character sprites for Samurai and Demon entities
- Tileset for dungeon environments

### Audio

- Background music tracks inspired by Dark Souls
- Character-specific sound effects for movement, attacks, and damage

## Development

### Debug Features

- Toggle collision box visibility for debugging hitboxes
- Console output for important game events

### Future Enhancements

- Additional enemy types
- More levels and room designs
- Character progression system
- Boss encounters
- Enhanced visual effects

---

*The Forsaken Crypt* was developed as a project for CS 485 at California State University San Marcos.