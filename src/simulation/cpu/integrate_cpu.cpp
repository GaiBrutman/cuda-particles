#include "simulation/particle_system.cuh"
#include "cuda_types.h"
#include <cmath>

static float3 rotatePoint3d(float3 p, float3 k, float theta) {
    float cosT = std::cos(theta);
    float sinT = std::sin(theta);

    return p * cosT + cross(k, p) * sinT;
}

void integrateParticles(ParticleSystem& ps, float dt) {
    for (int i = 0; i < ps.count; ++i) {
        float4& pos = ps.positions[i];
        float4& axis = ps.axes[i];

        float3 pos3d = {pos.x, pos.y, pos.z};
        float3 axis3d = {axis.x, axis.y, axis.z};
        
        float3 newPos = rotatePoint3d(pos3d, axis3d, axis.w * dt);

        ps.positions[i].x = newPos.x;
        ps.positions[i].y = newPos.y;
        ps.positions[i].z = newPos.z;
    }
}
