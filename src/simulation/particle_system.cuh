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
constexpr float BASE_SPEED = 1.0f;
constexpr float CAMERA_DIST = 3 * PARTICLE_RADIUS;
constexpr float FOCAL_LENGTH = 600.0f;

struct ParticleSystem {
    float4* positions;   // xyzw = x, y, z, unused  — owned by IRenderer
    float4* axes;        // xyzw = ax, ay, az, speed - never written to after init
    int     count;
};

// Lifecycle — positions must be set by caller via renderer->getMappedPositionBuffer()
void particleSystemAlloc(ParticleSystem& ps, int count);
void particleSystemFree(ParticleSystem& ps);
void particleSystemInit(ParticleSystem& ps, int width, int height);

// Per-frame pipeline steps
void integrateParticles(ParticleSystem& ps, float dt);
