#include "simulation/particle_system.cuh"
#include "simulation/math_helpers.cuh"

__device__ float3 rotatePoint3d(float3 p, float3 k, float theta) {
    float cosT = cosf(theta);
    float sinT = sinf(theta);

    return p * cosT + cross(k, p) * sinT;
}

__global__ void integrateKernel(ParticleSystem ps, float dt) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    float4 pos = ps.positions[idx];
    float4 axis = __ldg(&ps.axes[idx]);

    float3 pos3d = {pos.x, pos.y, pos.z};
    float3 axis3d = {axis.x, axis.y, axis.z};
    
    float3 newPos = rotatePoint3d(pos3d, axis3d, axis.w * dt);

    ps.positions[idx].x = newPos.x;
    ps.positions[idx].y = newPos.y;
    ps.positions[idx].z = newPos.z;
}

void integrateParticles(ParticleSystem& ps, float dt) {
    int grid = (ps.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    integrateKernel<<<grid, BLOCK_SIZE>>>(ps, dt);
    CUDA_CHECK(cudaGetLastError());
}
