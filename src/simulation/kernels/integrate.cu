#include "simulation/particle_system.cuh"

__global__ void integrateKernel(ParticleSystem ps, float dt, int width, int height) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    float4 pos = ps.positions[idx];
    float4 vel = ps.velocities[idx];
    float4 frc = __ldg(&ps.forces[idx]);

    // Euler: v += (F/m) * dt
    float invMass = 1.0f / pos.w;
    vel.x += frc.x * invMass * dt;
    vel.y += frc.y * invMass * dt;

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Elastic wall bounce (reflect, preserve speed)
    float W = (float)width;
    float H = (float)height;
    if (pos.x < 0.0f) { pos.x =  0.0f; vel.x =  fabsf(vel.x); }
    if (pos.x > W)    { pos.x =  W;    vel.x = -fabsf(vel.x); }
    if (pos.y < 0.0f) { pos.y =  0.0f; vel.y =  fabsf(vel.y); }
    if (pos.y > H)    { pos.y =  H;    vel.y = -fabsf(vel.y); }

    ps.positions[idx]  = pos;
    ps.velocities[idx] = vel;
}

void integrateParticles(ParticleSystem& ps, float dt, int width, int height) {
    int grid = (ps.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    integrateKernel<<<grid, BLOCK_SIZE>>>(ps, dt, width, height);
    CUDA_CHECK(cudaGetLastError());
}
