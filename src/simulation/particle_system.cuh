#pragma once

#include "cuda_types.h"
#include <cstdio>
#include <cstdlib>

#ifdef __CUDACC__
#define CUDA_CHECK(call)                                                  \
    do {                                                                  \
        cudaError_t err = (call);                                         \
        if (err != cudaSuccess) {                                         \
            fprintf(stderr, "CUDA error %s:%d — %s\n",                   \
                    __FILE__, __LINE__, cudaGetErrorString(err));         \
            std::exit(EXIT_FAILURE);                                      \
        }                                                                 \
    } while (0)
#endif

struct ParticleSystem {
    float4* positions;   // xyzw = x, y, z, mass
    float4* velocities;  // xyzw = vx, vy, vz, lifespan
    float4* forces;      // xyzw = fx, fy, fz, unused
    int     count;
};

void particleSystemAlloc(ParticleSystem& ps, int count);
void particleSystemFree(ParticleSystem& ps);
void particleSystemInit(ParticleSystem& ps, int width, int height);
void integrateParticles(ParticleSystem& ps, float dt, int width, int height);
