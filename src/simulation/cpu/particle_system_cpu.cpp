#include "simulation/particle_system.cuh"
#include <cstdlib>

void particleSystemAlloc(ParticleSystem& ps, int count) {
    ps.count      = count;
    ps.positions  = nullptr;  // owned by IRenderer; set via getMappedPositionBuffer()
    ps.velocities = static_cast<float4*>(std::malloc(count * sizeof(float4)));
    ps.forces     = static_cast<float4*>(std::malloc(count * sizeof(float4)));
}

void particleSystemFree(ParticleSystem& ps) {
    std::free(ps.velocities);
    std::free(ps.forces);
    ps.velocities = ps.forces = nullptr;
}

void particleSystemInit(ParticleSystem& ps, int width, int height) {
    int   half  = ps.count / 2;
    float speed = 80.0f;

    for (int i = 0; i < half; ++i) {
        float t   = (half > 1) ? (float)i / (float)(half - 1) : 0.5f;
        float dir = (i % 2 == 0) ? 1.0f : -1.0f;
        ps.positions[i]  = {t * (float)width, (float)height * 0.5f, 0.0f, 1.0f};
        ps.velocities[i] = {dir * speed, 0.0f, 0.0f, 0.0f};
        ps.forces[i]     = {0.0f, 0.0f, 0.0f, 0.0f};
    }
    for (int i = half; i < ps.count; ++i) {
        int   j     = i - half;
        int   vhalf = ps.count - half;
        float t     = (vhalf > 1) ? (float)j / (float)(vhalf - 1) : 0.5f;
        float dir   = (j % 2 == 0) ? 1.0f : -1.0f;
        ps.positions[i]  = {(float)width * 0.5f, t * (float)height, 0.0f, 1.0f};
        ps.velocities[i] = {0.0f, dir * speed, 0.0f, 0.0f};
        ps.forces[i]     = {0.0f, 0.0f, 0.0f, 0.0f};
    }
}
