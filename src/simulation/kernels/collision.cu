#include "simulation/particle_system.cuh"

// Tiled O(n²) collision with shared memory — correct for small n (< ~10K).
// For large n, replace with spatial-grid neighbor lookup (see spatial_grid.cuh).
__global__ void collisionKernel(ParticleSystem ps, float diameter) {
    __shared__ float4 tile[BLOCK_SIZE];

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    float4 pos = ps.positions[idx];
    float4 vel = ps.velocities[idx];
    float  d2  = diameter * diameter;

    for (int tileStart = 0; tileStart < ps.count; tileStart += BLOCK_SIZE) {
        int j = tileStart + threadIdx.x;
        tile[threadIdx.x] = (j < ps.count)
            ? ps.positions[j]
            : make_float4(1e9f, 1e9f, 0.0f, 0.0f);
        __syncthreads();

        int tileEnd = min(BLOCK_SIZE, ps.count - tileStart);
        #pragma unroll 4
        for (int t = 0; t < tileEnd; ++t) {
            if (tileStart + t == idx) continue;

            float dx    = tile[t].x - pos.x;
            float dy    = tile[t].y - pos.y;
            float dist2 = dx * dx + dy * dy;

            if (dist2 < d2 && dist2 > 1e-8f) {
                float dist = sqrtf(dist2);
                float nx   = dx / dist;
                float ny   = dy / dist;
                float dot  = vel.x * nx + vel.y * ny;
                if (dot > 0.0f) {  // only separate approaching pairs
                    vel.x -= 2.0f * dot * nx;
                    vel.y -= 2.0f * dot * ny;
                }
            }
        }
        __syncthreads();
    }

    ps.velocities[idx] = vel;
}

void resolveCollisions(ParticleSystem& ps) {
    int grid = (ps.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    collisionKernel<<<grid, BLOCK_SIZE>>>(ps, 2.0f * PARTICLE_RADIUS);
    CUDA_CHECK(cudaGetLastError());
}
