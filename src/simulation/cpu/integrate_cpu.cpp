#include "simulation/particle_system.cuh"

void integrateParticles(ParticleSystem& ps, float dt, int width, int height) {
    for (int i = 0; i < ps.count; ++i) {
        float4& pos = ps.positions[i];
        float4& vel = ps.velocities[i];

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // Bounce off walls
        if (pos.x < 0.0f)          { pos.x =  0.0f;          vel.x = -vel.x; }
        if (pos.x > (float)width)  { pos.x = (float)width;   vel.x = -vel.x; }
        if (pos.y < 0.0f)          { pos.y =  0.0f;          vel.y = -vel.y; }
        if (pos.y > (float)height) { pos.y = (float)height;  vel.y = -vel.y; }
    }
}
