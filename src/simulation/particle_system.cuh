#pragma once

#include "cuda_types.h"
#include <cstdio>
#include <cstdlib>

#ifdef __CUDACC__
#define CUDA_CHECK(call)                                                  \
    do {                                                                  \
        cudaError_t err = (call);                                         \
        if (err != cudaSuccess) {                                         \
            fprintf(stderr, "CUDA error %s:%d \xe2\x80\x94 %s\n",        \
                    __FILE__, __LINE__, cudaGetErrorString(err));         \
            std::exit(EXIT_FAILURE);                                      \
        }                                                                 \
    } while (0)
#endif

constexpr int   BLOCK_SIZE      = 256;
constexpr float PARTICLE_RADIUS = 3.0f;

struct ParticleSystem {
    float4* positions;   // xyzw = x, y, z, mass  — owned by IRenderer
    float4* velocities;  // xyzw = vx, vy, vz, lifespan
    float4* forces;      // xyzw = fx, fy, fz, unused
    int     count;
};

// Lifecycle — positions must be set by caller via renderer->getMappedPositionBuffer()
void particleSystemAlloc(ParticleSystem& ps, int count);
void particleSystemFree(ParticleSystem& ps);
void particleSystemInit(ParticleSystem& ps, int width, int height);

// Per-frame pipeline steps
void accumulateForces(ParticleSystem& ps);
void integrateParticles(ParticleSystem& ps, float dt, int width, int height);
void resolveCollisions(ParticleSystem& ps);
