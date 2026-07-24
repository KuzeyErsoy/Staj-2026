# Tactical Radar(Probably will change the name) - C++ & Raylib Project

This is a 3D platformer/collectathon game built using C++ and the [raylib](https://www.raylib.com/) library. 

## Code Architecture Overview

The project is structured into modular C++ classes: the core game loop (`main.cpp`), the player controller (`Player`), the camera manager (`CameraController`), the floating collectibles (`Collectible`), and the environment manager (`Map`).

### 1. `main.cpp`
This file contains the top-level game loop and integrates all sub-systems.
- **Initialization**: Sets up the 800x800 window, initializes `Player`, `Map`, `CameraController`, and a `std::vector` of `Collectible` items.
- **Game Loop (`Update` & `Draw`)**:
  - Updates camera and passes camera Yaw angle to `Player` for camera-relative movement.
  - Updates player movement, dash cooldowns, map collisions, and collectible interactions.
  - Handles dynamic collectible respawning when remaining active boxes fall to 5 or lower.
- **Rendering & UI**:
  - 3D Pass: Renders map, player model, and 3D collectibles inside `BeginMode3D()`.
  - 2D Pass: Renders top-left Level & Level Point text (`LEVEL: X`, `LEVEL POINT: Y`), bottom EXP progress bar, and help menu (`H`).

### 2. `CameraController.cpp` & `CameraController.h`
Encapsulates 3D camera modes and controls.
- **Third-Person Mouse Orbit**: Uses mouse delta movement to rotate camera yaw and pitch smoothly around the player.
- **Strategic View (`KEY_T`)**: Toggles a high-angle strategic view looking down at the player, allowing manual camera panning using arrow keys.
- **Camera Reset (`KEY_R`)**: Resets camera and player position.

### 3. `Player.cpp` & `Player.h`
Defines the `Player` class, managing character movement, physics, leveling, and rendering.
- **Camera-Relative Movement**: Moves WASD in the direction the camera is currently facing using `cameraYaw` trigonometry.
- **Dash Skill (`Left Shift`)**: Rapid forward dash burst with a 2.0s cooldown timer.
- **Leveling & Experience System**:
  - Accumulates EXP from collecting 3D boxes.
  - Automatically levels up when reaching `maxExp`, rewarding `levelPoints` for future skill choices.
  - Tracks upgrade multipliers (`moveSpeedMultiplier`, `dashCooldownMultiplier`).
- **Physics & Jumping**: Smooth gravity, grounded detection, and jumping (`SPACE`) that works both on the ground plane and on elevated parkour platforms.

### 4. `Collectible.cpp` & `Collectible.h`
Object-oriented class managing 3D experience boxes.
- **Visual Effects**: 3D cube rotation animation and vertical sine-wave floating oscillation.
- **AABB Collision**: Provides `GetBounds()` for automatic pickup when touched by the player.
- **Dynamic Respawning**: Implements `Respawn(newPos)` to re-populate collected boxes at random parkour coordinates across the map.

### 5. `Map.cpp` & `Map.h`
Defines the `Map` class, managing 3D terrain blocks and collision resolution.
- **Parkour Environment**: Multi-tiered layout constructed from modular `BLOCK_LONG` and `BLOCK_TALL` 3D OBJ models.
- **Collision Handling (`HandleCollisions()`)**:
  - Checks Axis-Aligned Bounding Box (AABB) overlap between player and map blocks.
  - Includes a step allowance (`stepAllowance = 0.5f`) enabling players to jump onto and walk across elevated platforms smoothly.
  - Applies horizontal wall pushback for side collisions and head-bump checks for under-block impacts.