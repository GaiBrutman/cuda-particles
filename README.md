# CUDA Particle Simulation

A real-time GPU particle simulation for learning CUDA optimization techniques hands-on. Supports three backends selected at build time:

- **CPU-only** — no CUDA required; plain C++, works anywhere
- **CUDA offscreen** — headless GPU rendering, works on Colab T4
- **CUDA + OpenGL** — live windowed display for local dev with NSight profiling

## Requirements

| | CPU-only | CUDA offscreen | CUDA + OpenGL |
|---|---|---|---|
| CUDA Toolkit | — | 11.0+ | 11.0+ |
| C++ compiler | 17+ | 17+ | 17+ |
| CMake | 3.18+ | 3.18+ | 3.18+ |
| OpenGL / GLFW | — | — | required |
| Python | optional | optional | optional |

## Build

```bash
# CPU-only — no NVCC needed
cmake -B build -DCPU_ONLY=ON
cmake --build build -j$(nproc)

# CUDA offscreen (headless, Colab-compatible)
cmake -B build -DCMAKE_CUDA_ARCHITECTURES=75   # 75 = T4; use 'native' locally
cmake --build build -j$(nproc)

# CUDA + OpenGL (local dev)
cmake -B build -DUSE_OPENGL_BACKEND=ON -DCMAKE_CUDA_ARCHITECTURES=native
cmake --build build -j$(nproc)
```

## Run

```bash
# Dump 300 frames to ./frames/
./build/particles --particles 100 --frames 300 --output ./frames/

# Larger scale
./build/particles --particles 500000 --frames 120
```

| Flag | Default | Notes |
|---|---|---|
| `--particles` | `100` | particle count |
| `--frames` | `300` | number of frames to render |
| `--output` | `./frames/` | output directory for PNG frames |
| `--width` / `--height` | `1280` / `720` | frame dimensions in pixels |
| `--dt` | `0.016` | timestep in seconds |

## Visualize

```bash
# Interactive animation (requires matplotlib + pillow)
python python/visualize.py --frames ./frames --fps 30

# Export GIF or MP4
python python/visualize.py --frames ./frames --fps 30 --output anim.gif
python python/visualize.py --frames ./frames --fps 30 --output anim.mp4  # needs ffmpeg
```

## Google Colab (T4 GPU)

Open `notebooks/particles_colab.ipynb` — it builds and runs everything in-browser on a T4.

Or use the helper script:

```bash
# Build + run + show animation in one command
python python/run_simulation.py --particles 100 --frames 300 --output anim.gif

# CPU-only (no GPU required)
python python/run_simulation.py --cpu-only --particles 100 --frames 300
```

## Project structure

```
src/
├── main.cpp                  # Entry point; selects renderer via #ifdef
├── app/                      # Frame loop, CLI config
├── simulation/               # Simulation — zero renderer dependencies
│   ├── particle_system.cuh   # ParticleSystem SoA struct + CUDA_CHECK macro
│   ├── spatial_grid.cuh      # Uniform grid helpers for O(n) neighbor queries
│   ├── cpu/                  # CPU fallback implementations
│   └── kernels/              # CUDA kernels (integrate, forces, collision)
└── render/
    ├── i_renderer.h          # Abstract interface (the only thing app.cpp touches)
    ├── cpu/                  # CPU renderer — malloc + software rasterizer
    ├── offscreen/            # CUDA → pinned pixel buffer → PNG via stb_image_write
    └── opengl/               # GLFW window, CUDA–GL VBO interop
shaders/                      # GLSL point sprites
third_party/stb/              # stb_image_write (header-only PNG export)
python/                       # visualize.py, run_simulation.py
notebooks/                    # Colab demo notebook
```

## Key design decisions

- **SoA memory layout** — `float4* positions`, `float4* velocities`, `float4* forces` as separate arrays. Threads in a warp read adjacent addresses → fully coalesced 128-byte transactions.
- **`float4` over `float3`** — 128-bit aligned; `float3` forces two memory transactions per access.
- **Renderer owns the position buffer** — `IRenderer::getMappedPositionBuffer()` returns the device pointer that kernels write into directly, with no intermediate copy. OpenGL backend maps a VBO into CUDA address space; offscreen and CPU backends use their own allocations.
- **One kernel per physical effect** — integrate, forces, collision are separate kernels composed in the app loop, not one monolithic kernel.
- **Pinned host memory for readback** — `cudaMallocHost` in the offscreen renderer gives PCIe DMA transfer speeds for the D2H pixel copy each frame.
- **Thrust for bulk ops** — force buffer zeroing and (future) spatial grid sort use `thrust::fill` / `thrust::sort_by_key` so launch configs are handled automatically.
