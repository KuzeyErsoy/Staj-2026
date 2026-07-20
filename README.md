# Tactical Radar - C++ & Raylib Project

This is a 3D platformer/collectathon game built using C++ and the [raylib](https://www.raylib.com/) library. 

## Code Architecture Overview

The project is structured into three main components: the main game loop (`main.cpp`), the player controller (`Player.h`/`Player.cpp`), and the map/environment manager (`Map.h`/`Map.cpp`).

### 1. `main.cpp`
This file contains the core game loop and the integration of all systems.
- **Initialization**: Sets up the 800x800 window, initializes the 3D camera, and instantiates the `Player` and `Map` objects.
- **Camera System**: Implements a free-roaming 3D camera that can be controlled independently of the player using arrow keys, Shift, Space, and Alt. It also features a "focus mode" (toggled with `T`) that locks the camera onto the player.
- **Collectibles**: Manages an array of 20 randomly placed red cubes (`Collectible` struct). Implements distance-based interaction logic where the player can press `E` to "eat" a box and increase their score when within range.
- **Collision Integration**: Resolves player collisions with the red collectible boxes, preventing the player from passing through them.
- **Rendering & UI**: Handles the 3D drawing phase (rendering the map, player, and boxes) and 2D UI overlay (score, interaction prompts, and a help menu accessible via `H`).

### 2. `Player.cpp` & `Player.h`
These files define the `Player` class, which handles character movement, physics, and rendering.
- **Properties**: Maintains the player's 3D position, velocity, rotation, bounding box, and grounded state.
- **Model Loading**: Loads the character's `.obj` model and texture from the `resources/character/` directory during initialization.
- **Movement Logic (`Update()`)**: 
  - Reads WASD input to move the character on the X and Z axes.
  - Calculates the correct facing angle based on the movement direction using `atan2f`.
  - Applies a simple gravity system affecting the Y velocity and allows jumping with `SPACE` when grounded.
  - Enforces hard map boundaries to keep the player within a 100x100 area.
- **Collision Box (`GetBounds()`)**: Provides an Axis-Aligned Bounding Box (AABB) used by other classes (like `Map` and `main.cpp`) to handle collisions.

### 3. `Map.cpp` & `Map.h`
These files define the `Map` class, responsible for the 3D environment layout and terrain collisions.
- **Blocks System**: The map is constructed from three types of modular 3D blocks (`BLOCK_LONG`, `BLOCK_SLOPE`, `BLOCK_TALL`). It loads the respective `.obj` models and applies a shared colormap texture.
- **Level Design**: The constructor manually places blocks to build a level featuring flat ground, ramps, and elevated platforms supported by taller blocks.
- **Collision Handling (`HandleCollisions()`)**: 
  - Iterates through all blocks in the map and checks for AABB collisions with the player's bounding box.
  - If a collision occurs while the player is falling (negative Y velocity) and they are above the block, it snaps the player to the top of the block, enabling them to walk on it.
  - Otherwise, it acts as a wall, preventing the player from walking through the sides of the blocks by reverting their X and Z positions to the previous frame.