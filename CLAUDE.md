# CLAUDE.md — CUDA Particle Simulation

This file is the source of truth for the project's architecture, conventions, and decision rationale.
Read it before touching any file. Update it when you make structural decisions.

---

## Purpose

A real-time (and headless) GPU particle simulation for learning CUDA optimization techniques:
coalesced memory access, shared memory, occupancy tuning, streams, and GPU–display interop.

The project is designed to run in two modes:
- **Offscreen** — headless, works on Google Colab and CI, dumps PNG frames
- **OpenGL** — real-time windowed rendering for local development with NSight profiling

The simulation logic is identical in both modes. Only the renderer differs.

---

## Project Structure

```
cuda-particles/
├── CMakeLists.txt
├── CLAUDE.md                        # This file
├── src/
│   ├── main.cpp                     # Entry point; constructs Config, App, and renderer
│   │
│   ├── app/
│   │   ├── app.cpp/.h               # Main loop; owns simulation + IRenderer
│   │   └── config.cpp/.h            # CLI arg parsing (--backend, --particles, --frames, etc.)
│   │
│   ├── simulation/                  # Pure CUDA — zero rendering dependencies
│   │   ├── particle_system.cuh      # ParticleSystem struct and SoA device layout
│   │   ├── particle_system.cu       # Host-side alloc/free/init
│   │   ├── spatial_grid.cuh         # Uniform spatial hash grid for O(n) neighbor queries
│   │   └── kernels/
│   │       ├── integrate.cu         # Time integration
│   │       ├── forces.cu            # Force accumulation
│   │       └── collision.cu         # Collision detection and response
│   │
│   ├── render/
│   │   ├── i_renderer.h             # Abstract renderer interface — the only thing app.cpp sees
│   │   ├── offscreen/
│   │   │   ├── offscreen_renderer.cu/.h   # CUDA kernel writes positions → pixel buffer
│   │   │   ├── frame_exporter.cpp/.h      # Saves uchar4 buffer as PNG (stb_image_write)
│   │   │   └── color_mapper.cuh           # Device-side velocity/lifespan → RGBA mapping
│   │   └── opengl/
│   │       ├── gl_renderer.cpp/.h         # GLFW window, draw calls
│   │       ├── gl_interop.cu/.h           # cudaGraphics* VBO registration and mapping
│   │       └── shader.cpp/.h              # GLSL loading and compilation
│   │
│   └── ui/
│       └── imgui_overlay.cpp/.h     # Optional: parameter sliders, live stats (OpenGL path only)
│
├── shaders/
│   ├── particle.vert                # Point sprite positioning
│   └── particle.frag                # Color by velocity magnitude or lifespan
│
├── python/
│   ├── run_simulation.py            # Compile + run + display frames in Colab
│   └── visualize.py                 # imageio / matplotlib animation from PNG sequence
│
└── notebooks/
    └── particles_colab.ipynb        # Self-contained Colab demo notebook
```

---

## Architecture

### The Golden Rule

> **The simulation layer never knows which renderer is active.**

`app.cpp` calls `IRenderer*` exclusively. The simulation kernels receive a raw `float4*` device
pointer — they do not know or care whether it came from a plain `cudaMalloc` (offscreen) or a
mapped OpenGL VBO (GL interop).

### Layer Responsibilities

| Layer | Files | Dependencies |
|---|---|---|
| Simulation | `simulation/**` | CUDA only. No GL, no display, no STB. |
| Renderer interface | `render/i_renderer.h` | Nothing — pure virtual C++ |
| Offscreen backend | `render/offscreen/**` | CUDA + stb_image_write |
| OpenGL backend | `render/opengl/**` | CUDA + OpenGL + GLFW + GLAD |
| App / loop | `app/**` | `IRenderer`, `ParticleSystem` |
| UI overlay | `ui/**` | Dear ImGui — only linked in OpenGL builds |

### Frame Loop (app.cpp)

```
┌─────────────────────────────────────────────────┐
│  renderer->getMappedPositionBuffer()            │  ← offscreen: plain d_ptr
│                                                 │    opengl:    mapped VBO ptr
│  integrateKernel<<<>>>( positions, ... )        │
│  forceKernel<<<>>>   ( positions, ... )         │
│                                                 │
│  renderer->unmapPositionBuffer()                │  ← opengl: releases VBO to GL
│  renderer->render( count, time )               │  ← offscreen: color map kernel
│                                                 │    opengl:    glDrawArrays
│  if pixels = renderer->getFramePixels()         │  ← offscreen only
│      frameExporter.save( pixels, frame++ )      │
└─────────────────────────────────────────────────┘
```

### IRenderer Contract

```cpp
class IRenderer {
public:
    virtual void     init(int w, int h, int maxParticles) = 0;
    virtual float4*  getMappedPositionBuffer()             = 0;
    virtual void     unmapPositionBuffer()                 = 0;
    virtual void     render(int count, float time)         = 0;
    virtual const uchar4* getFramePixels() { return nullptr; } // offscreen only
    virtual void     shutdown()                            = 0;
};
```

Adding a new backend (Vulkan, EGL, remote stream) means implementing this interface and adding
one branch in `main.cpp`. Nothing else changes.

---

## Memory Layout

Particles are stored in **Structure of Arrays (SoA)** on the device, not Array of Structures (AoS).

```cpp
// DO THIS (SoA) — coalesced access, threads read adjacent memory
struct ParticleSystem {
    float4 *positions;   // xyzw = x, y, z, mass
    float4 *velocities;  // xyzw = vx, vy, vz, lifespan
    float4 *forces;      // xyzw = fx, fy, fz, unused
    int     count;
};

// NOT THIS (AoS) — each thread strides across struct, kills memory throughput
struct Particle { float x, y, z, mass, vx, vy, vz, life; };
Particle *particles;
```

**Rule:** if you add a new per-particle attribute, add a new array to `ParticleSystem`. Do not
add fields to a struct and store an array of that struct.

---

## CUDA Conventions

### Kernel Naming

- `*Kernel` suffix for all `__global__` functions: `integrateKernel`, `colorMapKernel`
- `*Device` suffix for `__device__` helpers: `gravityForceDevice`, `hashCellDevice`

### Block / Grid Sizing

```cpp
// Standard 1D launch pattern — use this unless there's a specific reason not to
constexpr int BLOCK_SIZE = 256;
int gridSize = (particleCount + BLOCK_SIZE - 1) / BLOCK_SIZE;
myKernel<<<gridSize, BLOCK_SIZE>>>(ps, dt);
```

- Default block size: **256 threads**. Tune with NSight Compute occupancy analysis.
- Always guard with `if (idx >= count) return;` as the first line of every kernel.

### Shared Memory

Use shared memory in kernels that access neighbor data (collision, spatial queries). The pattern:

```cuda
__global__ void collisionKernel(ParticleSystem ps) {
    __shared__ float4 tile[BLOCK_SIZE];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    // Load this block's positions into shared memory
    tile[threadIdx.x] = ps.positions[idx];
    __syncthreads();

    // Now query tile[] without going to global memory repeatedly
}
```

### Error Checking

All CUDA API calls must be wrapped. Use the macro in `particle_system.cuh`:

```cpp
#define CUDA_CHECK(call)                                              \
    do {                                                              \
        cudaError_t err = (call);                                     \
        if (err != cudaSuccess) {                                     \
            fprintf(stderr, "CUDA error %s:%d — %s\n",               \
                    __FILE__, __LINE__, cudaGetErrorString(err));     \
            std::exit(EXIT_FAILURE);                                  \
        }                                                             \
    } while (0)

// Usage
CUDA_CHECK(cudaMalloc(&ps.positions, count * sizeof(float4)));
```

Never call raw CUDA API functions without CUDA_CHECK in production paths.

---

## Backend Selection

```bash
./particles --backend offscreen --particles 500000 --frames 120 --output ./frames/
./particles --backend opengl    --particles 1000000
```

| Flag | Values | Default |
|---|---|---|
| `--backend` | `offscreen`, `opengl` | `offscreen` |
| `--particles` | any int | `100000` |
| `--frames` | any int | `300` (offscreen only) |
| `--output` | path | `./frames/` (offscreen only) |
| `--width` / `--height` | pixels | `1280` / `720` |
| `--dt` | float | `0.016` |

---

## Colab Workflow

```python
# python/run_simulation.py
import subprocess
from IPython.display import Image, display

subprocess.run([
    "nvcc", "-O2", "-arch=sm_75",
    "src/main.cpp", "src/simulation/particle_system.cu",
    "src/simulation/kernels/integrate.cu",
    "src/simulation/kernels/forces.cu",
    "src/render/offscreen/offscreen_renderer.cu",
    "src/render/offscreen/frame_exporter.cpp",
    "-o", "particles"
], check=True)

subprocess.run([
    "./particles",
    "--backend", "offscreen",
    "--particles", "500000",
    "--frames", "60",
    "--output", "/tmp/frames/"
], check=True)

for i in range(60):
    display(Image(f"/tmp/frames/frame_{i:04d}.png"))
```

**Colab notes:**
- Always compile with `-arch=sm_75` (T4 GPU) or use `-arch=native` and check `nvidia-smi`
- The OpenGL backend will not compile on Colab — guard with `#ifdef USE_OPENGL` in CMakeLists
- Frame dumps are the only output; there is no interactive window

---

## Adding a New Renderer Backend

1. Create `src/render/<name>/` directory
2. Implement `IRenderer` fully — all pure virtual methods must be overridden
3. Add a `make_unique<YourRenderer>()` branch in `main.cpp`
4. Add a CMake option `USE_<NAME>_BACKEND` and guard the compile with it
5. Document the new backend's dependencies in this file under a new section

---

## Rules of Thumb

- **Never copy device→host inside the render loop** unless you're in offscreen mode exporting a frame. A GPU→CPU transfer every frame at 1M particles costs ~4ms and defeats the purpose.
- **Never store per-particle data in AoS.** If you find yourself adding a field to a particle struct, stop and add a new `float4*` array instead.
- **Prefer `float4` over `float3`.** CUDA memory transactions are 128-bit aligned; `float3` forces two transactions per access.
- **Keep kernels short and single-purpose.** One kernel = one physical effect. Compose them in the app loop, not by growing a monolithic kernel.
- **Profile before optimizing.** Open NSight Compute, look at achieved memory bandwidth vs theoretical peak, then decide what to fix. Do not guess.
- **The simulation layer has no `#include` from `render/`.** If you find yourself needing to, the abstraction is wrong — fix the interface instead.
- **CUDA_CHECK every API call.** Silent failures in CUDA produce wrong output with no error, which is extremely hard to debug.
