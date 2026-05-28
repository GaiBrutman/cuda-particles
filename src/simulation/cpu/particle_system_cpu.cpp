#include "simulation/particle_system.cuh"
#include <cstdlib>

void particleSystemAlloc(ParticleSystem& ps, int count) {
    ps.count      = count;
    ps.positions  = static_cast<float4*>(std::malloc(count * sizeof(float4)));
    ps.velocities = static_cast<float4*>(std::malloc(count * sizeof(float4)));
    ps.forces     = static_cast<float4*>(std::malloc(count * sizeof(float4)));
}

void particleSystemFree(ParticleSystem& ps) {
    std::free(ps.positions);
    std::free(ps.velocities);
    std::free(ps.forces);
    ps.positions = ps.velocities = ps.forces = nullptr;
}

void particleSystemInit(ParticleSystem& ps, int width, int height) {
    int half = ps.count / 2;
    float speed = 80.0f;  // pixels per second

    // Horizontal line: evenly spaced along x, y = height/2
    for (int i = 0; i < half; ++i) {
        float t = (float)i / (float)(half - 1);
        ps.positions[i]  = { t * (float)width, (float)height * 0.5f, 0.0f, 1.0f };
        float dir = (i % 2 == 0) ? 1.0f : -1.0f;
        ps.velocities[i] = { dir * speed, 0.0f, 0.0f, 0.0f };
        ps.forces[i]     = { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    // Vertical line: evenly spaced along y, x = width/2
    for (int i = half; i < ps.count; ++i) {
        int j = i - half;
        int vhalf = ps.count - half;
        float t = (float)j / (float)(vhalf - 1);
        ps.positions[i]  = { (float)width * 0.5f, t * (float)height, 0.0f, 1.0f };
        float dir = (j % 2 == 0) ? 1.0f : -1.0f;
        ps.velocities[i] = { 0.0f, dir * speed, 0.0f, 0.0f };
        ps.forces[i]     = { 0.0f, 0.0f, 0.0f, 0.0f };
    }
}
