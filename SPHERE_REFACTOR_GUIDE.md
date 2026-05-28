# Particle Sphere Refactor Guide

This guide walks you through converting the current 2D bouncing-lines simulation
into the 3D orbiting-sphere described in `particle-sphere-goal.md`.
You do the work. This file explains what to do, why it matters for GPU performance,
and where to look for answers. Read every section fully before writing a line of code.

---

## Before You Start: Read the Codebase

Do not skim. Open every file listed and understand it end-to-end.
You will be modifying most of them.

**Must read:**
- [src/simulation/particle_system.cuh](src/simulation/particle_system.cuh) — struct layout and CUDA_CHECK macro
- [src/simulation/particle_system.cu](src/simulation/particle_system.cu) — `initKernel`, allocation
- [src/simulation/kernels/integrate.cu](src/simulation/kernels/integrate.cu) — the main per-frame kernel
- [src/simulation/kernels/forces.cu](src/simulation/kernels/forces.cu) — what happens before integration
- [src/app/app.cpp](src/app/app.cpp) — the frame loop; how the pieces connect
- [src/render/offscreen/offscreen_renderer.cu](src/render/offscreen/offscreen_renderer.cu) — how positions become pixels
- [src/render/offscreen/color_mapper.cuh](src/render/offscreen/color_mapper.cuh) — per-particle coloring

**Questions to answer before proceeding:**
1. Where does `ps.positions` actually get allocated? (Hint: it is *not* in `particleSystemAlloc`.)
2. Why does `app.cpp` call `unmapPositionBuffer()` before `render()`?
3. What does `__ldg()` do and why is it used in `integrateKernel`?
4. What is the w-component of `ps.positions` used for in the current code?
5. Why does `clearKernel` exist as a separate kernel rather than using `cudaMemset`?

Answering these will surface the assumptions baked into the current design.
You need to break several of them.

---

## Phase 1 — Redesign the Data Layout

**Goal:** Replace the current `ParticleSystem` struct with one that matches the
orbital-sphere model.

### What the new model needs per particle

| Data | Type | Notes |
|------|------|-------|
| position `(x, y, z)` | `float3` / `float4` | On sphere surface, always distance R from origin |
| orbital axis `(ax, ay, az)` | `float3` / `float4` | Fixed unit vector; never written after init |
| angular speed | `float` | Radians per timestep; fixed after init |

The current `velocities` and `forces` arrays are no longer needed.
The orbital axis is the only "secondary" data; speed can pack into its w-component.

### Your task

Modify `particle_system.cuh` to:
- Remove `velocities` and `forces` pointers
- Add an `axes` array: `float4* axes` where `w = speed`
- Remove or repurpose the `w` component of `positions` (no mass needed)
- Update `particleSystemAlloc` and `particleSystemFree` accordingly

### Why float4 instead of float3?

CUDA memory transactions are **128 bits wide** (one cache line = 32 bytes = 8 floats).
A `float3` is 12 bytes — not a power-of-two size. Loading it requires two transactions
and leaves 4 bytes unused in every cache line.  A `float4` is 16 bytes: two per cache
line, perfectly aligned, one transaction.

Verify this claim: read the CUDA Best Practices Guide section on "Aligned Memory Accesses."
Search for "float3 vs float4 performance" in the CUDA forums.  You will find concrete
benchmark numbers.

The unused `w` field is not waste — it is the price of alignment. Pack useful data into it
(axis speed, for example).

### Coalesced access — the most important concept in this project

When a warp of 32 threads executes a memory load, the GPU combines adjacent addresses
into a single transaction if they are contiguous and aligned. This is *coalescing*.

With SoA (Structure of Arrays), thread 0 reads `positions[0]`, thread 1 reads
`positions[1]`, ..., thread 31 reads `positions[31]`. These are contiguous → one
transaction for the warp.

With AoS (Array of Structures), thread 0 reads `particles[0].x`, thread 1 reads
`particles[1].x`, but `particles[1].x` is `sizeof(Particle)` bytes away from
`particles[0].x`, not 4 bytes. No coalescing → up to 32 separate transactions.

Draw the memory layout for both cases on paper. Then look at the existing `ParticleSystem`
and confirm it is already SoA. Your new layout must stay SoA.

---

## Phase 2 — New Initialization Kernel

**Goal:** At startup, place every particle on the surface of a sphere of radius R with
a random position and a random orbital axis.

### Mathematics you need

**Random point on a unit sphere (uniform distribution):**
```
theta = 2π * u1          // u1, u2 ∈ [0,1) uniform random
phi   = acos(1 - 2*u2)
x = sin(phi) * cos(theta)
y = sin(phi) * sin(theta)
z = cos(phi)
```
Do *not* use rejection sampling inside a cube — it wastes threads and introduces divergence.
This closed-form method assigns unique, non-redundant work to every thread.

**Random unit vector for the orbital axis:**
Use the same formula with different random inputs. The axis must be perpendicular to the
position vector — otherwise the particle will drift off the sphere.

To enforce perpendicularity, generate a random vector **k** on the sphere, then project
out the component along **p**:
```
k_perp = normalize(k - dot(k, p) * p)
```
This is the Gram-Schmidt step. Understand why it works before coding it.

**What is dot(k, p) if p is on the sphere surface?**
Think about it: if **p** is a point on the sphere and **k** is a random unit vector, their
dot product measures how "aligned" they are.  After subtracting `dot(k,p)*p`, what is the
geometric meaning of `k_perp`?

### Random number generation on GPU

CUDA has no stdlib `rand()` in device code.  Use **cuRAND** or a simple hash-based PRNG.

For this project, a per-thread hash PRNG is sufficient and has zero overhead:
```cuda
__device__ unsigned int hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}
```
Call `hash(idx)`, `hash(idx + count)`, `hash(idx + 2*count)`, etc. to get independent
streams per thread without any state. Convert to `[0,1)` float: `(float)(hash(x)) / (float)UINT_MAX`.

### Your task

Replace `initKernel` in `particle_system.cu` with one that:
1. Generates a random position on the sphere surface at radius `R` (make R a `constexpr`)
2. Generates a random axis perpendicular to that position
3. Assigns a random angular speed in a small range (e.g. 0.5–1.5 × base_speed)
4. Writes `positions[idx]` and `axes[idx]`

Update `particleSystemInit` to pass whatever new parameters your kernel needs.

---

## Phase 3 — The Orbit Kernel

**Goal:** Replace `integrateKernel` (Euler integration) with a kernel that applies
Rodrigues' rotation to each particle every frame.

### Rodrigues' rotation formula

For position **p**, unit axis **k**, angle θ:
```
p_new = p·cos(θ) + (k × p)·sin(θ) + k·(k·p)·(1 − cos(θ))
```
Because **p** ⊥ **k** by construction, `k·p = 0`, so the last term vanishes:
```
p_new = p·cos(θ) + (k × p)·sin(θ)
```
Implement the cross product and dot product yourself — no library needed.
Each is 3-6 arithmetic ops. CUDA's FMA (fused multiply-add) will handle them efficiently.

### Read-only cache: __ldg()

The `axes` array is written once (at init) and read every frame, never written again.
This is exactly what the **read-only data cache** (L1 texture cache path) is for.

Use `__ldg(&ps.axes[idx])` to load the axis through the read-only cache. This bypasses
the regular L1 cache (which assumes data may be written) and uses a dedicated path with
higher bandwidth for read-only data.

**Experiment:** Implement the kernel with `ps.axes[idx]` first, profile it with
`nvprof` or NSight, then switch to `__ldg()` and compare. You want to see
"L1 texture cache hit rate" increase. This is a real, measurable optimization.

### Compute vs. memory bound

The current `integrateKernel` is **memory-bound**: it does a few multiplications
per float4 read, so the bottleneck is memory bandwidth.

The orbit kernel is **compute-bound**: Rodrigues' rotation is ~15 FLOPs per particle
on ~8 bytes read and ~8 bytes written. The arithmetic intensity is much higher.

Look up "roofline model" for CUDA. Draw the roofline for your GPU (find theoretical
FLOP/s and memory bandwidth in the spec sheet). Plot where the current kernel falls
and where the orbit kernel will fall. This tells you whether further memory or compute
optimization matters more.

### Your task

Create `src/simulation/kernels/orbit.cu` (or replace `integrate.cu`):
```
orbitKernel(float4* positions, const float4* axes, int count, float dt)
```
- One thread per particle
- Load position and axis (axis via `__ldg`)
- Apply Rodrigues' rotation with angle `= axes[idx].w * dt`
- Write new position back
- Guard: `if (idx >= count) return;`

Remove `forces.cu` and `collision.cu` — they are not needed.
Update `CMakeLists.txt` to remove them and add `orbit.cu`.
Update `app.cpp` to call `orbitParticles(ps, dt)` instead of `accumulateForces` +
`integrateParticles` + `resolveCollisions`.

---

## Phase 4 — 3D Projection in the Renderer

**Goal:** Map 3D positions on the sphere to 2D screen pixels.

The current `colorMapKernel` treats `pos.x` and `pos.y` as pixel coordinates directly.
That worked for the 2D simulation. For the 3D sphere you need a projection.

### Simple perspective projection

```
screen_x = (pos.x / (pos.z + camera_dist)) * focal_length + width/2
screen_y = (pos.y / (pos.z + camera_dist)) * focal_length + height/2
```
Choose `camera_dist` so the sphere fits nicely on screen (e.g. `3 * R`).
Choose `focal_length` to control apparent sphere size.

### Depth-based rendering

Multiple particles will project to the same pixel. With millions of particles this
is expected and desirable — it creates the dense shell appearance.

For a splat renderer (writing pixels atomically), the last writer wins. You can accept
this or use `atomicExch` to "sort" by depth, but for a sphere of uniform density
the visual difference is minimal. Start simple.

Later challenge: can you colour particles by their `pos.z` value (depth) to give the
sphere a 3D shaded appearance without any lighting calculation?

### Your task

Modify `colorMapKernel` in `offscreen_renderer.cu`:
1. Add perspective projection math
2. Clamp to screen bounds before writing
3. Change the splat size if needed (3×3 splat may look too large or too small at
   different particle counts — experiment)

The renderer does not need to know anything about the simulation physics.
It only sees `float4* positions`. That is the IRenderer contract — do not break it.

---

## Phase 5 — Coloring by Orbital Axis

**Goal:** Replace velocity-based coloring with something meaningful for the sphere.

Ideas (pick one, justify your choice):
- **Axis direction**: map `(ax, ay, az)` to `(r, g, b)`. Particles orbiting in similar
  planes will share colours, creating visible "bands" that rotate.
- **Depth (z)**: front-facing particles bright, back-facing dim. Classic sphere shading.
- **Speed**: faster particles brighter. Simple, always works.
- **Orbit phase**: compute the angle of the particle along its orbit and map to hue.
  This requires storing the initial reference vector, which adds complexity.

The colour mapper is in `color_mapper.cuh`. It is a `__device__` inline function.
The renderer passes it the position and whatever secondary data you choose.

**Question to answer:** the current `colorMapKernel` takes `velocities` as a separate
parameter. After your refactor, `velocities` no longer exists. What do you pass instead?
How do you update the renderer's `setVelocityBuffer` plumbing (or remove it)?

---

## Phase 6 — Occupancy and Block Size Tuning

Once the simulation is working visually, measure and optimize.

### Occupancy

Occupancy = (active warps on SM) / (maximum warps an SM can hold).

Higher occupancy is not always better, but low occupancy (< 50%) often indicates
wasted SM resources.

To measure occupancy:
```bash
nvprof --metrics achieved_occupancy ./particles --backend offscreen --frames 10
```
Or use NSight Compute's occupancy section.

### Block size experiment

BLOCK_SIZE is `constexpr int BLOCK_SIZE = 256` in `particle_system.cuh`.

Try 64, 128, 256, 512. For each, record:
- Achieved occupancy
- Kernel time (ns)
- Memory throughput

The orbit kernel has moderate register usage (Rodrigues = ~10 registers). The theoretical
occupancy vs. block size curve can be read from the occupancy calculator
(`cuda_occupancy_calculator.xls` from NVIDIA or the NSight Compute UI).

Rule of thumb: start at 256. Go smaller if occupancy is register-limited. Go larger
only if you have shared memory use that amortises the overhead.

### Register pressure

Add `--ptxas-options=-v` to your NVCC flags in CMakeLists.txt:
```cmake
target_compile_options(particles PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--ptxas-options=-v>)
```
This prints register usage per kernel. Rodrigues' rotation should need ~16-24 registers.
If it exceeds 32, the compiler may spill registers to local memory (slow).

Inspect with: look for lines like `ptxas info: Used N registers, ...`.

---

## Phase 7 — CUDA Streams (Stretch Goal)

Once the simulation runs smoothly, try pipelining.

The current frame loop is:
```
orbit kernel → (sync) → render kernel → (sync) → memcpy D→H → save PNG
```
Every arrow is sequential. The GPU sits idle during the PNG write.

With two streams and double-buffering you can overlap:
```
stream A:  orbit N+1 →               render N+1 → ...
stream B:            → memcpy N → PNG save N
```

The GPU never idles: while stream A runs the orbit kernel for frame N+1, stream B is
doing the host-side PNG write for frame N.

This requires two position buffers and careful synchronisation with `cudaStreamWaitEvent`.
Research `cudaEvent_t` and `cudaStreamWaitEvent` before attempting this.

**Question to answer first:** why can you not simply launch both streams without any
synchronisation? What would go wrong if the orbit kernel for frame N+1 started while
the memcpy for frame N had not finished?

---

## Checkpoints

Work through phases in order. Each checkpoint should compile and produce visible output
before you proceed.

- [ ] Phase 1 complete: `make` succeeds with new struct; no existing tests broken
- [ ] Phase 2 complete: `--frames 1` produces a PNG with dots scattered on a circle (ortho projection)
- [ ] Phase 3 complete: `--frames 60` produces 60 PNGs; dots rotate, none drift off sphere
- [ ] Phase 4 complete: sphere looks like a sphere with perspective; not a flat disc
- [ ] Phase 5 complete: coloring is meaningful and visually interesting
- [ ] Phase 6 complete: you have occupancy numbers and can explain them
- [ ] Phase 7 (optional): pipelined render, measurable throughput improvement

---

## Debugging Tips

**Particles fly off the sphere:** Rodrigues' formula has a sign error or axis is not
unit-length. Add a debug kernel that computes `|pos| - R` for every particle and prints
the max deviation. Should be < 1e-4 after many frames (float precision limit).

**All particles converge to one point:** Axis generation is not perpendicular to position.
Check your Gram-Schmidt step — specifically the normalisation.

**Black frames / no pixels:** Projection math sends all particles off-screen. Print one
projected coordinate to stdout from a debug CPU path and sanity-check it.

**Kernel launch fails silently:** You added CUDA_CHECK to every launch, right?
`CUDA_CHECK(cudaGetLastError())` after every kernel invocation — not optional.

---

## Resources

- *CUDA C++ Best Practices Guide* — chapters on Memory Optimizations, Execution Configuration
- *Programming Massively Parallel Processors* (Kirk & Hwu) — chapters 5–7 cover memory hierarchy
- Rodrigues' rotation formula derivation: Wikipedia is sufficient; verify the perpendicularity simplification yourself
- NSight Compute user guide — occupancy analysis and roofline tool

---

*This guide does not write the code for you. It tells you what to build, why each
decision matters for GPU performance, and where to look when you are stuck.*
*By the time all checkpoints are green, you will have directly applied coalesced memory,
read-only cache, occupancy tuning, and (optionally) stream pipelining to a real workload.*
