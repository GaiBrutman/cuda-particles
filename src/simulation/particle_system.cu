#include "particle_system.cuh"
#include <thrust/device_ptr.h>
#include <thrust/fill.h>

__global__ void initKernel(ParticleSystem ps, int width, int height) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    int   half  = ps.count / 2;
    float speed = 80.0f;

    if (idx < half) {
        float t   = (half > 1) ? (float)idx / (float)(half - 1) : 0.5f;
        float dir = (idx % 2 == 0) ? 1.0f : -1.0f;
        ps.positions[idx]  = make_float4(t * (float)width, (float)height * 0.5f, 0.0f, 1.0f);
        ps.velocities[idx] = make_float4(dir * speed, 0.0f, 0.0f, 0.0f);
    } else {
        int   j     = idx - half;
        int   vhalf = ps.count - half;
        float t     = (vhalf > 1) ? (float)j / (float)(vhalf - 1) : 0.5f;
        float dir   = (j % 2 == 0) ? 1.0f : -1.0f;
        ps.positions[idx]  = make_float4((float)width * 0.5f, t * (float)height, 0.0f, 1.0f);
        ps.velocities[idx] = make_float4(0.0f, dir * speed, 0.0f, 0.0f);
    }
    ps.forces[idx] = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void particleSystemAlloc(ParticleSystem& ps, int count) {
    ps.count     = count;
    ps.positions = nullptr;  // owned by IRenderer; set via getMappedPositionBuffer()
    CUDA_CHECK(cudaMalloc(&ps.velocities, count * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&ps.forces,     count * sizeof(float4)));
}

void particleSystemFree(ParticleSystem& ps) {
    CUDA_CHECK(cudaFree(ps.velocities));
    CUDA_CHECK(cudaFree(ps.forces));
    ps.velocities = ps.forces = nullptr;
}

void particleSystemInit(ParticleSystem& ps, int width, int height) {
    int grid = (ps.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    initKernel<<<grid, BLOCK_SIZE>>>(ps, width, height);
    CUDA_CHECK(cudaDeviceSynchronize());
}
