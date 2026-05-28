#include "simulation/particle_system.cuh"
#include <cstdlib>
#include <random>
#include <cmath>

#include "cuda_types.h"

static constexpr float PI = (float)M_PI;

static float getRandomUnitFloat(void)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    return dis(gen);
}

static float3 getRandomPointOnUnitSphere(void)
{
    float u1 = getRandomUnitFloat();
    float u2 = getRandomUnitFloat();

    float theta = 2 * PI * u1;
    float phi = std::acos(1 - 2 * u2);
    float x = std::sin(phi) * std::cos(theta);
    float y = std::sin(phi) * std::sin(theta);
    float z = std::cos(phi);

    return {x, y, z};
}

static float3 perpendicular(float3 k, float3 p) {
    float3 p_hat = normalize(p);
    return normalize(k - dot(k, p_hat) * p_hat);
}

void particleSystemAlloc(ParticleSystem &ps, int count)
{
    ps.count = count;
    ps.positions = nullptr; // owned by IRenderer; set via getMappedPositionBuffer()
    ps.axes = static_cast<float4 *>(std::malloc(count * sizeof(float4)));
}

void particleSystemFree(ParticleSystem &ps)
{
    std::free(ps.axes);
    ps.axes = nullptr;
}

void particleSystemInit(ParticleSystem &ps, int width, int height)
{
    for (int i = 0; i < ps.count; ++i)
    {
        // Calculate initial position
        float3 pos = getRandomPointOnUnitSphere() * PARTICLE_RADIUS;

        ps.positions[i] = {pos.x, pos.y, pos.z, 0.0f};
    
        float angularSpeed = (0.5 + getRandomUnitFloat()) * BASE_SPEED;

        // if (i > 0) {
        //     ps.axes[i] = ps.axes[i-1];
        //     ps.axes[i].w = angularSpeed;
        //     continue;
        // }

        // Calculate orbital axis
        float3 k = getRandomPointOnUnitSphere();
        float3 k_prep = perpendicular(k, pos);

        ps.axes[i] = {k_prep.x, k_prep.y, k_prep.z, angularSpeed};
    }
}
