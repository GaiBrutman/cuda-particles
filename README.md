# CUDA Particle Simulation

A real-time GPU particle simulation for learning CUDA optimization techniques hands-on. Renders either in a live OpenGL window or dumps PNG frames for headless/Colab use.

## Requirements

| | Local (OpenGL) | Headless / Colab |
|---|---|---|
| CUDA Toolkit | 11.0+ | 11.0+ |
| C++ compiler | 17+ | 17+ |
| CMake | 3.18+ | 3.18+ |
| OpenGL / GLFW | required | not needed |
| Python | optional | optional (visualization) |

## Build

```bash
# Offscreen only (works everywhere, no display required)
cmake -B build -DUSE_OPENGL_BACKEND=OFF
cmake --build build -j$(nproc)

# OpenGL + offscreen (local dev)
cmake -B build -DUSE_OPENGL_BACKEND=ON
cmake --build build -j$(nproc)
```

## Run

```bash
# Headless — renders 120 frames to ./frames/
./build/particles --backend offscreen --particles 500000 --frames 120 --output ./frames/

# Real-time window
./build/particles --backend opengl --particles 1000000
```

| Flag | Default | Notes |
|---|---|---|
| `--backend` | `offscreen` | `offscreen` or `opengl` |
| `--particles` | `100000` | |
| `--frames` | `300` | offscreen only |
| `--output` | `./frames/` | offscreen only |
| `--width` / `--height` | `1280` / `720` | |
| `--dt` | `0.016` | timestep in seconds |

## Google Colab

Open `notebooks/particles_colab.ipynb` — it compiles and runs everything in-browser on a T4 GPU. Or use the script directly:

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
    "./particles", "--backend", "offscreen",
    "--particles", "500000", "--frames", "60", "--output", "/tmp/frames/"
], check=True)

for i in range(60):
    display(Image(f"/tmp/frames/frame_{i:04d}.png"))
```

> Use `-arch=sm_75` for Colab T4, or `-arch=native` after checking `nvidia-smi`.

## Project structure

```
src/
├── main.cpp                  # Entry point
├── app/                      # Main loop, CLI config
├── simulation/               # Pure CUDA — no renderer deps
│   └── kernels/              # simulation kernels
└── render/
    ├── i_renderer.h          # Abstract interface (the only thing app.cpp touches)
    ├── offscreen/            # CUDA → pixel buffer → PNG
    └── opengl/               # GLFW window, CUDA–GL VBO interop
shaders/                      # GLSL point sprites
python/                       # Colab helper scripts
notebooks/                    # Colab demo notebook
```

The simulation kernels receive a plain `float4*` device pointer. They don't know — and don't care — whether it came from `cudaMalloc` or a mapped OpenGL VBO.

## Key design decisions

- **SoA memory layout** — `float4* positions`, `float4* velocities`, `float4* forces` as separate arrays. Threads in a warp read adjacent addresses → fully coalesced 128-byte transactions.
- **`float4` over `float3`** — 128-bit aligned; `float3` would force two memory transactions per access.
- **One kernel per physical effect** — compose kernels in the app loop rather than building one monolithic kernel.
- **No CPU roundtrip in the render loop** — positions stay on the GPU. The OpenGL backend maps the VBO directly into CUDA address space via `cudaGraphicsMapResources`.
