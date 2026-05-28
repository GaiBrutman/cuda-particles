#include "particle_system.cuh"
#include "math_helpers.cuh"

static constexpr float PI = CUDART_PI_F;

__device__ unsigned int hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

__device__ float getRandomUnitFloat(unsigned int x) {
    return (float)(hash(x)) / (float)UINT_MAX;
}

__device__ float3 getRandomPointOnUnitSphere(unsigned int seed)
{
    float u1 = getRandomUnitFloat(seed);
    float u2 = getRandomUnitFloat(seed+1);

    float theta = 2 * PI * u1;
    float phi   = acosf(1.0f - 2.0f * u2);
 
    return make_float3(
        sinf(phi) * cosf(theta),
        sinf(phi) * sinf(theta),
        cosf(phi)
    );
}

__device__ float3 perpendicular(float3 k, float3 p) {
    return normalize(k - dot(k, p) * p);
}


__global__ void initKernel(ParticleSystem ps, int width, int height) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= ps.count) return;

    float3 pos = getRandomPointOnUnitSphere(idx+1);

    float angularSpeed = (0.5 + getRandomUnitFloat(idx)) * BASE_SPEED;

    // Calculate orbital axis
    float3 k = getRandomPointOnUnitSphere(idx+3);
    float3 k_prep = perpendicular(k, pos);

    pos *= PARTICLE_RADIUS;

    ps.positions[idx] = make_float4(pos.x, pos.y, pos.z, 0.0f);
    ps.axes[idx] = make_float4(k_prep.x, k_prep.y, k_prep.z, angularSpeed);
}

void particleSystemAlloc(ParticleSystem& ps, int count) {
    ps.count     = count;
    ps.positions = nullptr;  // owned by IRenderer; set via getMappedPositionBuffer()
    CUDA_CHECK(cudaMalloc(&ps.axes, count * sizeof(float4)));
}

void particleSystemFree(ParticleSystem& ps) {
    CUDA_CHECK(cudaFree(ps.axes));
    ps.axes = nullptr;
}

void particleSystemInit(ParticleSystem& ps, int width, int height) {
    int grid = (ps.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    initKernel<<<grid, BLOCK_SIZE>>>(ps, width, height);
    CUDA_CHECK(cudaDeviceSynchronize());
}
