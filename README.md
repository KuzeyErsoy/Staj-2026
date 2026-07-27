# Tactical Radar(Probably will change the name) - C++ & Raylib Project

This is a 3D platformer/collectathon game built using C++ and the [raylib](https://www.raylib.com/) library. 

## Code Architecture Overview

The project is structured into modular C++ classes: the core game loop (`main.cpp`), the player controller (`Player`), the camera manager (`CameraController`), the floating collectibles (`Collectible`), and the environment manager (`Map`).

### 1. `main.cpp`
This file contains the top-level game loop, state machine, and system integration.
- **Game State Machine (`GameState`)**:
  - `STATE_MENU`: Renders main title screen with `[ PRESS ENTER TO START ]`.
  - `STATE_PLAYING`: Manages active 3D gameplay, player physics, map collisions, collectible pickup waves, and real-time HUD rendering.
  - `STATE_LEVEL_UP`: Triggers when reaching EXP threshold or holding unspent level points. Pauses movement and presents an interactive 2D upgrade modal (`[1]` Speed Boost, `[2]` Rapid Dash).
  - `STATE_GAME_OVER`: Triggered when player falls into the void below $Y = -15.0f$, with `[ PRESS R TO RESTART ]` reset functionality.
- **Rendering & UI**:
  - 3D Pass: Renders ground plane, map terrain, player model, and 3D collectibles inside `BeginMode3D()`.
  - 2D Pass: Renders level status, EXP progress bar, Tactical Radar HUD, help menu (`H`), and state-specific overlays.

### 2. `Radar.cpp` & `Radar.h`
Encapsulates the 2D Tactical Mini-Map (Radar) HUD overlay.
- **Position & Layout**: Top-right corner 2D circular reticle with concentric radar rings and cardinal direction indicators (N, S, E, W).
- **Coordinate Mapping**: Maps 3D world $(X, Z)$ positions of the player and active collectibles onto 2D radar coordinates.
- **Visual Blips**:
  - **Player Blip**: Bright cyan dot with a dynamic directional vector line indicating camera facing angle.
  - **Collectible Blips**: Glowing gold/yellow blips for active experience boxes that update dynamically upon pickup or respawn.

### 3. `CameraController.cpp` & `CameraController.h`
Encapsulates 3D camera modes and controls.
- **Third-Person Mouse Orbit**: Uses mouse delta movement to rotate camera yaw and pitch smoothly around the player.
- **Strategic View (`KEY_T`)**: Toggles a high-angle strategic view looking down at the player, allowing manual camera panning using arrow keys.
- **Camera Reset (`KEY_R`)**: Resets camera and player position.

### 4. `Player.cpp` & `Player.h`
Defines the `Player` class, managing character movement, physics, leveling, and rendering.
- **Camera-Relative Movement**: Moves WASD in the direction the camera is currently facing using `cameraYaw` trigonometry.
- **Dash Skill (`Left Shift`)**: Rapid forward dash burst with a 2.0s cooldown timer.
- **Leveling & Experience System**:
  - Accumulates EXP from collecting 3D boxes.
  - Automatically levels up when reaching `maxExp`, rewarding `levelPoints` for future skill choices.
  - Tracks upgrade multipliers (`moveSpeedMultiplier`, `dashCooldownMultiplier`) upgraded via the Level-Up UI modal.
- **Physics & Jumping**: Smooth gravity, grounded detection, and jumping (`SPACE`) that works both on the ground plane and on elevated parkour platforms.

### 5. `Collectible.cpp` & `Collectible.h`
Object-oriented class managing 3D experience boxes.
- **Visual Effects**: 3D cube rotation animation and vertical sine-wave floating oscillation.
- **AABB Collision**: Provides `GetBounds()` for automatic pickup when touched by the player.
- **Dynamic Respawning**: Implements `Respawn(newPos)` to re-populate collected boxes at random parkour coordinates across the map.

### 6. `Map.cpp` & `Map.h`
Defines the `Map` class, managing 3D terrain blocks, level loading, and collision resolution.
- **Data-Driven Level Loader (`LoadLevelFromFile`)**:
  - Loads 3D block layouts dynamically from human-readable text files (e.g. `resources/map/level1.txt`).
  - Converts text grid symbols (`L`, `T`, elevation height numbers) into 3D world coordinates ($X, Z$) and instantiates `BLOCK_LONG` and `BLOCK_TALL` models.
- **Collision Handling (`HandleCollisions()`)**:
  - Checks Axis-Aligned Bounding Box (AABB) overlap between player and map blocks.
  - Includes a step allowance (`stepAllowance = 0.5f`) enabling players to jump onto and walk across elevated platforms smoothly.
  - Applies horizontal wall pushback for side collisions and head-bump checks for under-block impacts.