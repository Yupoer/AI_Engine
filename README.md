# AI Engine

[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-5586A4?style=flat&logo=opengl)](https://www.opengl.org/)
[![CMake](https://img.shields.io/badge/CMake-Build-064F8C?style=flat&logo=cmake)](https://cmake.org/)

> A real-time 3D rendering engine with an integrated AI behaviour system, featuring multiple autonomous ball agents with FSM-driven logic, bounding-sphere agent-agent collision, AABB wall collision, Phong shading, and an ImGui runtime control panel — all driven by a custom OpenGL + GLFW pipeline.

## Introduction

AI Engine extends a core OpenGL renderer with **autonomous agent behaviour**, making each ball in the scene an independent entity that perceives the environment and reacts to collisions through a **Finite State Machine (FSM) combined with Fuzzy Logic**. It addresses the challenge of combining real-time graphics with lightweight AI by embedding per-object state machines directly into the rendering loop — no separate thread required.

Built with a component-oriented C++ design, geometry (`DrawBall`), collision volumes (`AABB`, `BoundingSphere`), and AI logic are cleanly separated, enabling fast iteration on behaviour without touching the render pipeline.

## Table of Contents

- [Getting Started](#getting-started)
  - [System Requirements](#system-requirements)
  - [Prerequisites](#prerequisites)
  - [Quick Start](#quick-start)
  - [Manual Build](#manual-build)
  - [Troubleshooting](#troubleshooting)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [AI Behaviour System](#ai-behaviour-system)
- [Design Decisions & Trade-offs](#design-decisions--trade-offs)
- [Project Layout](#project-layout)
- [License](#license)

## Getting Started

### System Requirements

* **OS:** Windows 10/11 (x64)
* **GPU:** OpenGL 4.6-capable GPU (NVIDIA GTX 1050+ or AMD RX 570+ recommended)
* **RAM:** 4 GB minimum
* **Disk:** 500 MB free space

### Prerequisites

* **CMake** 3.15+
* **Visual Studio 2019/2022** with C++ Desktop workload
* **GLEW** and **GLFW3** — pre-built DLLs included in `build/Release/`

### Quick Start

1. **Clone the repository**

   ```bash
   git clone https://github.com/Yupoer/AI_Engine.git
   cd AI_Engine
   ```

2. **Configure and build**

   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Run the application**

   ```bash
   .\Release\3DRender.exe
   ```

   > **Note:** `glew32.dll` and `glfw3.dll` must reside in the same directory as the executable. Shader files (`fragmentShaderSource.frag`, `vertexShaderSource.vert`) and `picSource/` textures must also be co-located.

### Manual Build

Open `build/3DRender.sln` in Visual Studio and build the `3DRender` target in **Release** configuration.

### Troubleshooting

1. **Agents not moving:** Ensure the simulation runs at >30 FPS — AI state updates are frame-rate dependent. Reduce agent count if performance drops.
2. **All agents clumping together:** This is expected initial behaviour as agents begin in overlapping positions. The FSM transitions to a separation state after the first collision response.
3. **Missing DLL error:** Copy `glew32.dll` and `glfw3.dll` from `build/Release/` next to the executable.
4. **Shader error on startup:** Confirm `.frag` / `.vert` files are in the same folder as the `.exe`.

## Key Features

* **FSM + Fuzzy Logic AI**: Each ball agent operates under a **Finite State Machine** with fuzzy membership functions determining state transitions — producing nuanced, non-binary behaviour responses to proximity and velocity.
* **Autonomous Ball Agents**: Each ball is an independent AI entity with its own velocity, direction, and collision-response logic — producing emergent group behaviour without a central coordinator.
* **Bounding Sphere Collision**: Sphere-to-sphere intersection tests for fast, rotation-invariant broad-phase collision detection between ball agents (O(1) per pair).
* **AABB Wall Collision**: Axis-Aligned Bounding Box tests for accurate ball-to-wall boundary detection, ensuring agents stay within the scene bounds.
* **Phong Lighting Model**: Per-fragment ambient, diffuse, and specular shading applied to all ball geometries via GLSL fragment shader.
* **STL Model Import**: Custom vertex-array converter (`stl2VA.exe`, `stl2array.exe`) converts `.stl` files to inline C++ arrays at build time, eliminating runtime parsing.
* **ImGui Runtime Panel**: Real-time control of agent count, speed multiplier, and collision visualisation toggles via an integrated **Dear ImGui** overlay.
* **Pre-baked Model Data**: `model_data.h` contains pre-converted vertex arrays for instant loading — zero disk I/O at runtime.

## Architecture

The system separates AI logic, rendering, and collision into independent concerns:

```
main.cpp  (Render + AI Loop)
  ├── DrawBall        — per-ball VAO management & draw calls
  ├── BoundingSphere  — agent-agent collision (sphere-sphere distance test)
  ├── AABB            — wall boundary collision
  ├── Shader          — GLSL shader loader
  ├── Camera          — view + projection matrices
  ├── Dear ImGui      — runtime controls
  └── model_data.h    — pre-baked ball vertex arrays
```

### Update Loop Explanation

**Per-frame AI Update:**
1. For each agent: evaluate FSM state using fuzzy proximity/velocity inputs
2. Integrate velocity → update world position based on current FSM state
3. **Bounding Sphere** test against all other agents — on collision, compute reflection vector and exchange momentum
4. **AABB** test against scene boundaries — on boundary hit, invert the relevant velocity component
5. Upload updated model matrix to GPU via uniform

**Render Pass:**
1. Clear colour + depth buffers
2. For each agent: bind VAO → set uniforms (MVP, colour, light) → `glDrawArrays`
3. Overlay ImGui panel

**Why this architecture?**
- **In-loop AI:** Keeping AI updates inside the render loop avoids thread synchronisation overhead — safe and sufficient for tens of agents at 60 FPS
- **Pre-baked vertices:** Eliminates STL parsing at runtime; `model_data.h` is generated once by the converter tools
- **Bounding Sphere for agent-agent:** Spheres are rotation-invariant, making the intersection test a single distance comparison — ideal for uniformly-shaped ball agents

## AI Behaviour System

Each ball agent maintains the following state:

| Property | Type | Description |
|----------|------|-------------|
| **position** | `glm::vec3` | World-space centre of the agent |
| **velocity** | `glm::vec3` | Direction × speed vector |
| **radius** | `float` | Used for both rendering scale and BoundingSphere radius |
| **FSM state** | `enum` | Current behavioural state (e.g., Wander, Flee, Chase) |
| **fuzzy inputs** | `float` | Proximity and relative velocity to nearest neighbour |

**FSM States:**
- **Wander**: Agent moves in a semi-random direction, gradually steering toward unexplored space
- **Flee**: Triggered when a neighbour enters close proximity — velocity is reflected away from the threat
- **Chase**: Triggered by fuzzy membership when a neighbour is at mid-range — agent steers toward target

**Collision Response:**
- **Agent vs Agent (BoundingSphere):** When `distance(a.pos, b.pos) < a.radius + b.radius`, velocities are reflected along the collision normal — simulating elastic collision.
- **Agent vs Wall (AABB):** When an agent's bounding box intersects a scene wall, the perpendicular velocity component is negated, producing a bounce.

## Design Decisions & Trade-offs

* **Why FSM + Fuzzy Logic over a pure rule system?** Hard thresholds produce abrupt, unnatural state switches. Fuzzy membership functions allow agents to blend between states smoothly — e.g., partially fleeing while partially wandering — producing more realistic emergent behaviour.
* **Why BoundingSphere over AABB for agent-agent collision?** Ball agents are spherical and never rotate relative to their local frame. Sphere-sphere intersection requires only a distance check vs. sum of radii — cheaper and more accurate than an AABB for round objects.
* **Why pre-bake model data into a header?** Runtime STL parsing requires file I/O and memory allocation per model load. Pre-converting to a `const float[]` array in `model_data.h` gives zero-overhead loading and enables the compiler to place geometry in read-only memory.
* **Why run AI in the render loop instead of a separate thread?** With tens of agents, the AI update is microseconds per frame. A separate thread would introduce mutex locks around the transform buffer — adding latency and complexity for negligible gain at this scale.
* **Why ImGui for controls?** Dear ImGui requires no external UI framework, integrates in < 10 lines of setup, and allows real-time slider adjustments without recompiling — ideal for rapid behaviour tuning.

## Project Layout

```plaintext
.
├── AABB.h                       # Axis-Aligned Bounding Box (wall collision)
├── BoundingSphere.h             # Bounding Sphere (agent-agent collision)
├── Camera.cpp / .h              # FPS-style camera controller
├── DrawBall.cpp / .h            # Ball geometry draw calls & VAO management
├── Shader.cpp / .h              # GLSL shader loader & linker
├── main.cpp                     # Application entry, FSM AI update, render loop
├── ball.h                       # Hardcoded ball vertex array (fallback)
├── model_data.h                 # Pre-baked STL vertex arrays (main geometry)
├── fragmentShaderSource.frag    # Fragment shader (Phong lighting)
├── vertexShaderSource.vert      # Vertex shader (MVP transform)
├── stb_image.h                  # Single-header texture loader
├── ball.stl                     # Source STL model for ball geometry
├── stl2VA.exe                   # STL-to-vertex-array converter
├── stl2array.exe                # Alternative STL converter tool
├── stl_to_vertex_array.cpp      # Converter source code
├── imgui/                       # Dear ImGui (GLFW + OpenGL3 backend)
├── picSource/                   # Texture images (.jpg) and model files
├── build/                       # CMake build output (VS solution + Release exe)
└── CMakeLists.txt               # Build system configuration
```

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.
