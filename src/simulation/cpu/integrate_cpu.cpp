#include "simulation/particle_system.cuh"

void integrateParticles(ParticleSystem& ps, float dt, int width, int height) {
    for (int i = 0; i < ps.count; ++i) {
        // TODO: Euler integration — pos += vel * dt, apply boundary
    }
}
