#include "simulation/particle_system.cuh"

void resolveCollisions(ParticleSystem& ps) {
    for (int i = 0; i < ps.count; ++i) {
        // TODO: O(n²) naive or spatial-grid collision response
    }
}
