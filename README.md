# Tactical Radar

**Real-time autonomous military drone fleet logistics simulation and 3D combat arena built from scratch with C++17 and raylib.**

My vision started through a tactical radar simulation but shifted into a 3d realistic drone simulation with swarm AI logic. Nine AI-controlled drones coordinate across a 1 km x 1 km operational theater to detect, classify, engage, resupply, and recover targets without human intervention. The simulation models radio-frequency signal propagation, threat-magnitude force allocation, mid-air rendezvous logistics, ballistic micro-missiles, defensive countermeasure flares, rotor downwash dust physics, and multispectral vision modes (FLIR thermal / NVG night vision) -- all rendered in real-time 3D with a full military heads-up display.

**All drone operations are hard coded protocols which you can see the protocol scenerios in Scenario.md**

---

## The Parkour Game (Learning Foundation)

The project also includes a 3D parkour platformer game that was developed first as a learning vehicle for understanding drone control systems, procedural audio, and radar rendering before scaling up to the full fleet simulation.

---

## Table of Contents

1. [Simulation Features](#simulation-features)
2. [Controls Reference](#controls-reference)
3. [Architecture Overview](#architecture-overview)
4. [The Parkour Game (Learning Foundation)](#the-parkour-game-learning-foundation)
5. [Prerequisites](#prerequisites)
6. [Building and Running](#building-and-running)
7. [Project Structure](#project-structure)
8. [License](#license)

---

## Simulation Features

### Autonomous 9-Drone Fleet

The fleet consists of three specialized roles, each governed by independent AI decision logic:

| Role | Units | Speed | HP | Primary Function |
| :--- | :---: | :---: | :---: | :--- |
| **Recon** (Scout) | 2 | 32 m/s | 160 | High-speed sector sweep, target detection and reporting |
| **Task Force** (Strike) | 5 | 16 m/s | 200 | Pincer-formation engagement, laser and micro-missile strikes |
| **Support** (Supplier/Medic) | 2 | 16 m/s | 300 | Mid-air fuel/ammo resupply, crash-site SAR recovery and repair |

**Recon Drones** autonomously patrol the map in expanding sector sweeps. Upon detecting an enemy cluster, they file a contact report to the command terminal. The system calculates a threat magnitude value -- 0.5 per armed hostile, 0.25 per unarmed hostile -- and dispatches the minimum required Task Force drones in a flanking pincer formation.

**Task Force Drones** approach targets from calculated encirclement angles, engage with sustained laser fire and lock-on guided micro-missiles, and request resupply from Support drones when fuel or ammunition drops below threshold. When a drone is critically damaged and crashes, the system flags a Search and Rescue (SAR) mission.

**Support Drones** carry large cargo reserves of fuel and ammunition. They perform mid-air rendezvous resupply at the calculated midpoint between themselves and the requesting drone. For SAR missions, Task Force drones first clear the perimeter of hostiles, and then the Support drone lands at the crash site to repair the downed unit to full operational status. Support drones are mandatorily dispatched alongside any strike mission targeting armed enemy clusters.

### Signal Intelligence and Radio Telemetry

Every drone maintains a live radio link back to the command base. The Signal-to-Noise Ratio (SNR) degrades logarithmically with distance from the hangar:

```
SNR (dB) = 45.0 - 20.0 * log10(max(1.0, distance / 20.0))
```

This drives real-time telemetry values including signal quality (0.0 to 1.0), latency (12 ms at base, increasing with range), and downlink bitrate (1.5 to 12.0 Mbps). When a drone's FPV camera feed is displayed, the signal quality manifests visually as static noise, horizontal glitch lines, and chromatic aberration. Critically low SNR triggers a flashing warning overlay, and destroyed drones display a full test-card signal-loss pattern.

### Multispectral Vision Modes

Three vision modes are available, each affecting the entire 3D scene and all HUD elements:

- **Normal**: Standard daylight rendering with full-color drone navigation LEDs, ground spotlights, and terrain.
- **FLIR Thermal (White-Hot)**: Inverted thermal palette. Enemy heat signatures glow white/orange with temperature readouts. Raster scanlines, a Mil-Dot reticle, and bearing/elevation ladders overlay the viewport. Drone engines emit thermal bloom.
- **NVG Gen-3 (Green Phosphor)**: Intensified green-phosphor night vision with characteristic grain noise. All lighting shifts to phosphor-green spectrum.

### Tactical Radar Map

Pressing `M` opens a fullscreen interactive tactical radar map with the following capabilities:

- **Asset Filter Bar**: Five clickable filter buttons (or hotkeys `1` through `5`) to isolate All Assets, Recon Only, Task Force Only, Support Only, or Hostiles Only. Each button displays a live unit count.
- **SNR Coverage Rings**: Four concentric range rings at 200 m (Green/Optimal), 400 m (Yellow/Standard), 600 m (Orange/Degraded), and 800 m (Red/Fringe), with annotated signal specifications.
- **Flight Trajectory Vectors**: Directional lines drawn from each active drone to its assigned target or rendezvous point.
- **Target Analysis Card**: Left-clicking an enemy cluster opens a detailed intelligence panel showing coordinates, force composition (armed vs. unarmed breakdown), threat magnitude, recommended strike doctrine, and a live list of dispatched assets with range and estimated time of arrival.
- **Drone Quick Card**: Left-clicking any drone marker opens its instant telemetry readout (HP, fuel, ammo, SNR, speed, assigned mission).
- **Precision Cursor**: Real-time world coordinate and range readout under the mouse reticle.

### Weapons and Countermeasures

- **Laser Engagement**: Task Force drones sustain a 40 damage/second laser beam on locked targets. Support drones project a green resupply/repair beam.
- **Guided Micro-Missiles**: Task Force drones launch lock-on ballistic micro-missiles with parabolic flight trajectories, engine trail particles, and area-of-effect explosion detonation.
- **Defensive Flares**: All drones can deploy infrared countermeasure flare salvos (manual or automatic), producing chaff/flare particle bursts that dissipate over time.

### Atmospheric Physics and Rotor Downwash

Drones flying below 8 meters altitude generate ground-level rotor downwash effects. Intensity scales inversely with altitude. A dynamic atmospheric wind vector drifts dust particles downwind. Concentric expanding dust shockwave rings appear beneath the rotors with a central pressure footprint. The effect adapts to the active vision mode: thermal friction glow in FLIR, phosphor scattering in NVG, and realistic sand-colored clouds in normal view.

### Heads-Up Display Panels

The military HUD provides multiple toggleable panels:

| Panel | Toggle | Description |
| :--- | :---: | :--- |
| Fleet Telemetry | `F1` | Global fleet metrics: active drones, docked count, known targets, eliminated count |
| Resource Status | `F2` | Per-drone fuel, ammo, and HP bar summaries |
| Command Terminal | `F3` | Scrolling operational log of AI decisions, target detections, and mission status changes |
| Camera Banner | `F4` | Active camera mode identifier and simulation time/speed indicator |
| PiP Nose Camera | `F6` / `V` | Picture-in-Picture live video feed from a selected drone's forward camera |
| Drone Inspector | `I` | Deep telemetry card for a single drone: position, velocity, heading, fuel, ammo, HP, SNR, state, and action log |
| Tactical Map | `M` | Fullscreen interactive radar map (see above) |
| Debriefing | `F7` / `G` | Post-operation performance scorecard: mission time, missiles fired, flares deployed, fuel burned, drones repaired, clusters eliminated |
| Cinematic Mode | `F8` | Hides all HUD panels for clean 3D observation |

### Operation Waves and Debriefing

Enemy clusters spawn in escalating waves. When all active clusters are cleared, a wave-clear announcement triggers and the next wave can be initiated. The debriefing panel tracks cumulative mission statistics across waves.

---

## Controls Reference

### Fleet Simulation Mode

| Key | Action |
| :--- | :--- |
| `F7` / `O` | Enter Fleet Simulation from main menu |
| `ESC` | Close fullscreen map, or return to main menu |
| `0` / `C` | Free command camera (WASD + Shift + Mouse look) |
| `1` - `9` | Lock camera to drone 1 through 9 |
| `TAB` | Cycle through camera modes sequentially |
| `P` / `SPACE` | Pause / Resume simulation |
| `+` / `-` | Increase (2x, 4x, 8x) / Decrease (0.5x, 0.25x) simulation speed |
| `T` | Toggle FLIR Thermal vision mode |
| `N` | Toggle NVG Night Vision mode |
| `M` | Toggle fullscreen Tactical Radar Map |
| `I` | Toggle Drone Inspector panel |
| `F1` - `F4` | Toggle individual HUD panels |
| `F6` / `V` | Toggle PiP nose camera window |
| `[` / `]` | Cycle PiP target drone (Auto / #1 - #9) |
| `F7` / `G` | Toggle Debriefing panel |
| `F8` | Toggle all panels (Cinematic mode) |
| `X` | Deploy defensive flares (selected / PiP drone) |
| `K` | Spawn armed enemy cluster (red, counter-attacks) |
| `L` | Spawn unarmed enemy cluster (orange, flees) |
| `J` | Spawn heavy mixed enemy cluster |
| `F11` | Toggle fullscreen window |

### Tactical Map (while `M` map is open)

| Key / Action | Function |
| :--- | :--- |
| `1` - `5` (or click buttons) | Filter displayed assets by role or hostiles |
| Left-click enemy cluster | Open Target Analysis Card |
| Left-click drone marker | Open Drone Quick Card |
| Right-click or `X` button | Dismiss analysis/quick card |

### Parkour Game Mode

| Key | Action |
| :--- | :--- |
| `ENTER` | Start game from main menu |
| `WASD` | Player movement |
| `SPACE` | Jump (double jump when unlocked) |
| `SHIFT` | Dash |
| `E` | Toggle companion drone mode (Follow-Attack / Free Flight) |
| `T` | Enter Drone Test Arena |
| `F5` / `F9` | Quick Save / Quick Load |
| `L` (menu) | Load saved game |
| `1` - `4` (level up) | Select upgrade: Speed / Dash / Double Jump / Max HP |
| `R` | Restart after game over |

---

## Architecture Overview

The simulation is built as a single-binary C++17 application, compiled with CMake and linked against raylib (included as a Git submodule). The codebase is organized into two distinct subsystems and a shared entry point.

### System Architecture Diagram

```
main.cpp  (State Machine: MENU | PLAYING | FLEET_SIM | GAME_OVER | VICTORY)
    |
    +---> DroneSystemManager          [DroneSystem.h / .cpp]
    |         |
    |         +---> HangarBase        [fleet/HangarBase.h / .cpp]
    |         |       9 helipad dock positions, base refueling/rearming
    |         |
    |         +---> Drone (abstract)  [fleet/Drone.h / .cpp]
    |         |       +-- ReconDrone         (sector sweep, detection, reporting)
    |         |       +-- TaskForceDrone     (pincer engagement, missiles, laser)
    |         |       +-- SupportDrone       (resupply, SAR, forward loiter)
    |         |
    |         +---> ClusterManager    [fleet/ClusterManager.h / .cpp]
    |         |       Enemy cluster spawning, threat calculation, hostile AI
    |         |
    |         +---> MissileSystem     [fleet/MissileSystem.h / .cpp]
    |         |       Ballistic micro-missiles, trail particles, AoE detonation
    |         |
    |         +---> FleetCamera       [fleet/FleetCamera.h / .cpp]
    |         |       Free camera + 9 orbital tracking camera modes
    |         |
    |         +---> FleetHUD          [fleet/FleetHUD.h / .cpp]
    |                 All 2D panels, tactical map, PiP render, FLIR overlay,
    |                 SNR coverage rings, target analysis cards, debriefing
    |
    +---> Player, Map, Drone (game)   [oyun/*.h / .cpp]
    |       3D parkour game systems (see below)
    |
    +---> Shared Services
            +-- AudioManager          [oyun/AudioManager.h / .cpp]
            |     Procedural waveform sound synthesis (no audio files)
            +-- ParticleSystem        [oyun/ParticleSystem.h / .cpp]
                  Impact, dash trail, pickup, and downwash dust particles
```

### Core Subsystem Responsibilities

**DroneSystemManager** (`DroneSystem.h/.cpp`) is the top-level coordinator for the fleet simulation. It owns the fleet of 9 drones (stored as `std::unique_ptr<Drone>` in a polymorphic vector), the hangar base, cluster manager, missile system, fleet camera, and HUD. On each frame it orchestrates: time-scaled fleet updates, camera movement (with fullscreen-map cursor lock), off-screen PiP render pass, 3D scene drawing, and 2D HUD overlay.

**FleetTypes** (`fleet/FleetTypes.h`) defines all shared enumerations (`DroneRole`, `DroneState`, `ClusterStatus`, `VisionMode`) and data structures (`Enemy`, `EnemyCluster`, `TelemetryContext`, `FleetContext`, `CountermeasureParticle`, `DownwashParticle`). The `FleetContext` struct is passed to every drone update call to provide access to sibling drones, the cluster manager, missile system, hangar, and telemetry -- avoiding circular dependencies.

**Drone** (`fleet/Drone.h/.cpp`) is the abstract base class. It manages shared state (position, velocity, yaw/pitch/roll, fuel, ammo, HP, SNR link, visual FX) and provides default implementations for 3D rendering (procedural quadcopter model, navigation LEDs, ground spotlights, flares, downwash). Three concrete subclasses implement role-specific AI:
- `ReconDrone`: Sector-index sweep, scanning timer, contact reporting.
- `TaskForceDrone`: Target-lock, flanking angle computation, laser engagement, missile launch cooldown, support request logic.
- `SupportDrone`: Mid-air rendezvous intercept, resupply beam, SAR landing/repair, forward loiter positioning, mandatory escort logic.

**ClusterManager** (`fleet/ClusterManager.h/.cpp`) handles enemy lifecycle: spawning clusters of armed and unarmed hostiles at random positions, computing per-cluster threat magnitudes, managing hostile counter-attack fire, processing panic/flee AI for unarmed units, and clearing destroyed clusters.

**MissileSystem** (`fleet/MissileSystem.h/.cpp`) tracks in-flight micro-missiles with proportional-navigation guidance, parabolic arcs, engine trail particle emission, impact detection against cluster enemies, area-of-effect explosion detonation, and particle cleanup.

**FleetHUD** (`fleet/FleetHUD.h/.cpp`) is the largest single module (~1550 lines). It implements all 2D overlay rendering: telemetry panel, resource status bars, active missions panel, command terminal log, camera banner, PiP camera window with render-to-texture, FPV signal degradation overlay, FLIR scanline overlay with thermal crosshairs, NVG grain overlay, drone inspector deep-telemetry card, fullscreen tactical radar map with SNR coverage rings, asset filter bar, trajectory vectors, target analysis cards, drone quick cards, precision cursor, and the debriefing scorecard.

**FleetCamera** (`fleet/FleetCamera.h/.cpp`) provides two camera paradigms: a WASD + mouse free-flight command camera, and nine orbital tracking cameras that follow individual drones with mouse-controlled yaw/pitch orbit.

**HangarBase** (`fleet/HangarBase.h/.cpp`) defines the physical base with nine helipad dock positions arranged in a grid, a protective energy shield dome, runway lighting, and per-frame fuel/ammo/HP refill logic for docked drones.

### Rendering Pipeline

Each frame executes a two-pass rendering pipeline:

1. **Off-screen PiP Pass**: The PiP nose camera renders the 3D scene from the selected drone's forward-facing viewpoint into a `RenderTexture2D` framebuffer.
2. **Main Pass**: The primary 3D scene is drawn from the active camera (free or tracking), followed by the full 2D HUD overlay which composites the PiP texture, all panels, and the tactical map.

When the fullscreen tactical map is open, the system enables the OS cursor for click interaction, suppresses 3D camera mouse-delta input, and routes all mouse events to the map's interactive elements. Closing the map restores hidden-cursor free-look mode.

### Audio System

All sound effects are generated procedurally at runtime using waveform synthesis -- no audio files are loaded. The `AudioManager` singleton synthesizes distinct sounds for dashing, collecting items, jumping, taking damage, leveling up, UI clicks, and flare deployment.

---

## The Parkour Game (Learning Foundation)

Before building the fleet simulation, a complete 3D cyber-parkour platformer was developed in the `oyun/` (Turkish for "game") directory. This game served as the engineering foundation for understanding raylib's 3D rendering pipeline, collision detection, camera systems, and drone flight mechanics.

### Game Overview

The player controls a third-person character across procedurally loaded block-based levels. The objective is to collect scattered items, defeat patrolling enemies, level up through an upgrade system, and reach the exit portal to advance through three progressively harder stages.

### Core Systems (oyun/)

| Module | Description |
| :--- | :--- |
| `Player` | Third-person character with WASD movement, momentum-based physics, air dash with cooldown, double jump (unlockable), wall sliding, HP/invincibility, and an XP/level/skill-point progression system |
| `Map` | Data-driven level loader that reads block layouts from text files (`resources/map/level*.txt`) and builds 3D geometry from OBJ models with texture mapping |
| `CameraController` | Third-person orbital camera with mouse-driven yaw/pitch, player tracking, collision shake effects, and drone free-flight camera override |
| `Drone` (game) | Companion drone AI with two modes: Follow-Attack (orbits player, auto-targets nearest enemy with a laser) and Free Flight (full manual WASD + mouse flight control) |
| `Enemy` | Patrol enemies with waypoint movement, player collision damage, and drone laser vulnerability |
| `Collectible` | Floating animated pickup items that grant XP and dynamically respawn in waves |
| `Radar` | 2D minimap HUD overlay showing player, enemies, collectibles, and the companion drone as colored blips |
| `ParticleSystem` | 3D particle effects for dash trails, impact bursts, pickup sparkles, and rotor downwash dust |
| `AudioManager` | Procedural waveform sound synthesis engine (see above) |
| `SaveSystem` | Binary quick-save/load system preserving level index and player state |

### Progression and Upgrades

On leveling up, the player chooses one of four upgrades:
1. Movement Speed increase
2. Dash Cooldown reduction
3. Double Jump unlock
4. Max HP increase

A Drone Test Arena (press `T`) provides an infinite enemy spawner for testing the companion drone's combat AI in isolation. This test arena was the direct precursor to the fleet simulation's cluster engagement system.

---

## Prerequisites

- **CMake** 3.14 or later
- **C++17** compatible compiler:
  - macOS: Xcode Command Line Tools (Apple Clang)
  - Linux: GCC 8+ or Clang 7+
  - Windows: MSVC 2017+ or MinGW-w64
- **Git** (for cloning with submodules)
- **OpenGL 3.3+** capable GPU and drivers

### Platform-Specific Dependencies

**macOS** (no additional packages required):
```bash
xcode-select --install
```

**Linux (Debian/Ubuntu)**:
```bash
sudo apt install build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
    libxcursor-dev libxinerama-dev libwayland-dev \
    libxkbcommon-dev
```

**Linux (Fedora)**:
```bash
sudo dnf install gcc-c++ cmake git mesa-libGL-devel \
    libX11-devel libXrandr-devel libXi-devel \
    libXcursor-devel libXinerama-devel wayland-devel \
    libxkbcommon-devel
```

**Windows**: Install Visual Studio 2017 or later with the "Desktop development with C++" workload, or install MinGW-w64 and CMake separately. Git for Windows is required.

---

## Building and Running

```bash
# 1. Clone the repository with the raylib submodule
git clone --recursive https://github.com/KuzeyErsoy/Staj-2026.git
cd Staj-2026

# 2. Configure the build
cmake -B build -S .

# 3. Compile
cmake --build build

# 4. Run
./build/Staj26          # macOS / Linux
# or
.\build\Debug\Staj26.exe  # Windows (MSVC)
```

If you cloned without `--recursive`, initialize the submodule manually:

```bash
git submodule update --init --recursive
```

### Quick Launch Reference

- **Main Menu**: Press `ENTER` to start the parkour game, `F7` or `O` to enter the fleet simulation, `T` for the drone test arena.
- **Fleet Simulation**: Use `WASD` + mouse for free camera, number keys `1`-`9` to track individual drones, `M` for the tactical map, `K`/`L`/`J` to spawn enemies.
- **Exit**: Press `ESC` to return to the main menu (closes fullscreen map first if open).

---

## Project Structure

```
Staj-2026/
+-- main.cpp                  Application entry point and state machine
+-- DroneSystem.h / .cpp      Fleet simulation coordinator
+-- CmakeLists.txt            Build configuration
+-- Scenario.md               Operational scenario reference (Turkish)
+-- TODO.md                   Development roadmap and technical reference
+-- fleet/                    Fleet simulation subsystem
|   +-- FleetTypes.h            Shared enums and data structures
|   +-- Drone.h / .cpp          Base drone + 3 role subclasses
|   +-- ClusterManager.h / .cpp Enemy cluster lifecycle and threat AI
|   +-- MissileSystem.h / .cpp  Guided micro-missiles and explosions
|   +-- HangarBase.h / .cpp     Base, helipads, and refueling
|   +-- FleetCamera.h / .cpp    Free + tracking camera modes
|   +-- FleetHUD.h / .cpp       Military HUD, tactical map, PiP, overlays
+-- oyun/                     Parkour game subsystem
|   +-- Player.h / .cpp         Character controller and progression
|   +-- Map.h / .cpp            Data-driven level loader
|   +-- Drone.h / .cpp          Companion drone AI (follow/free-flight)
|   +-- Enemy.h / .cpp          Patrol enemy AI
|   +-- Collectible.h / .cpp    XP pickup items
|   +-- CameraController.h/.cpp Third-person orbital camera
|   +-- Radar.h / .cpp          2D minimap overlay
|   +-- ParticleSystem.h / .cpp 3D particle effects
|   +-- AudioManager.h / .cpp   Procedural sound synthesis
|   +-- SaveSystem.h / .cpp     Binary save/load
+-- resources/                Game assets
|   +-- character/              Player model and textures
|   +-- map/                    Level data files and block models
+-- thirdparty/
    +-- raylib/                 raylib (Git submodule)
```

**Total codebase**: ~7,500 lines of C++ across 36 source files.

---

## License

This project was developed as an internship project. raylib is licensed under the Zlib license.