#include "simulation/particle_system.cuh"

void accumulateForces(ParticleSystem& ps) {
    for (int i = 0; i < ps.count; ++i) {
        // TODO: gravity + other forces → ps.forces[i]
    }
}
