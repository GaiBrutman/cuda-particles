#include "simulation/particle_system.cuh"

void accumulateForces(ParticleSystem& ps) {
    for (int i = 0; i < ps.count; ++i)
        ps.forces[i] = {0.0f, 0.0f, 0.0f, 0.0f};
}
