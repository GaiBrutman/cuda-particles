# CUDA Particle Sphere Simulation — Project Goal

## Overview

Simulate and render a large number of particles (potentially millions) all orbiting a single central point in 3D space. Every particle moves in a perfect circular orbit at the **same radius** from the center, but each has a **unique, randomly assigned orbital plane**. Together they form a dense, uniform shell — a sphere of continuously moving particles.

## Particle Motion Model

Each particle orbits the center following these rules:

- **Fixed radius**: every particle is always exactly distance `R` from the origin.
- **Circular orbit**: the particle traces a perfect circle on the surface of the sphere.
- **Random orbital plane**: each particle's orbital plane is defined by a randomly oriented axis through the origin. The axis is chosen once at initialization and never changes.
- **Constant angular speed**: each particle advances along its orbit at a fixed angular velocity per timestep. Individual speeds may vary slightly between particles to add visual richness.

Mathematically, updating a particle each timestep is a rotation of its position vector around its orbital axis:

```
new_position = rotate(position, axis, angular_speed * dt)
```

where `rotate` applies Rodrigues' rotation formula.

## Why CUDA

With tens of thousands to millions of particles, updating every position each frame is a massively parallel workload — each particle is completely independent of all others. This maps perfectly onto the GPU threading model:

- One CUDA thread per particle.
- Each thread reads `(x, y, z)` and `(ax, ay, az, speed)` from global memory, applies the rotation, and writes the new position back.
- No inter-thread communication or synchronization needed.

The goal is to sustain a high frame rate (60+ fps) with particle counts that would be infeasible on a CPU.

## Data Layout

Each particle stores:

| Field | Type | Description |
|-------|------|-------------|
| `x, y, z` | `float3` | Current position on the sphere surface |
| `ax, ay, az` | `float3` | Unit vector — orbital axis (fixed at init) |
| `speed` | `float` | Angular velocity (radians per timestep) |

Positions and axes are stored in separate arrays (Structure of Arrays layout) for coalesced memory access on the GPU.

## Rodrigues' Rotation (per-particle kernel)

For a position vector **p**, axis **k** (unit vector), and angle θ:

```
p_new = p·cos(θ) + (k × p)·sin(θ) + k·(k · p)·(1 − cos(θ))
```

Since **p** is always perpendicular to **k** (by construction), the last term is zero, simplifying to:

```
p_new = p·cos(θ) + (k × p)·sin(θ)
```

This is just a handful of multiplications and additions per particle — very cheap per thread.

## Rendering Approach (TBD)

Options under consideration:

1. **CUDA + OpenGL interop** — particle positions live in a CUDA buffer that is shared directly with OpenGL as a vertex buffer. Zero CPU roundtrip; ideal for real-time interactive rendering.
2. **Write frames to disk** — kernel computes positions, outputs a PPM/PNG frame each tick. Simpler setup, useful for offline rendering or profiling.

The preferred approach is OpenGL interop for real-time visualization.

## Success Criteria

- Particle positions update entirely on the GPU with no CPU involvement per frame.
- Visual output shows a clear, uniform spherical shell of moving particles.
- Scales smoothly from 10,000 to 1,000,000+ particles.
- Frame rate remains interactive (≥30 fps) at high particle counts.
