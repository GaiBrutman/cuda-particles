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
    // TODO: scatter particles across [0,width] x [0,height]
}
