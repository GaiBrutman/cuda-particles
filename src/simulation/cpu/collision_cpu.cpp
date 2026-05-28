#include "simulation/particle_system.cuh"
#include <cmath>

void resolveCollisions(ParticleSystem& ps) {
    float d2 = (2.0f * PARTICLE_RADIUS) * (2.0f * PARTICLE_RADIUS);
    for (int i = 0; i < ps.count; ++i) {
        for (int j = i + 1; j < ps.count; ++j) {
            float dx    = ps.positions[j].x - ps.positions[i].x;
            float dy    = ps.positions[j].y - ps.positions[i].y;
            float dist2 = dx * dx + dy * dy;
            if (dist2 < d2 && dist2 > 1e-8f) {
                float dist = std::sqrt(dist2);
                float nx   = dx / dist;
                float ny   = dy / dist;
                float dot  = (ps.velocities[i].x - ps.velocities[j].x) * nx
                           + (ps.velocities[i].y - ps.velocities[j].y) * ny;
                if (dot > 0.0f) {
                    ps.velocities[i].x -= dot * nx;
                    ps.velocities[i].y -= dot * ny;
                    ps.velocities[j].x += dot * nx;
                    ps.velocities[j].y += dot * ny;
                }
            }
        }
    }
}
