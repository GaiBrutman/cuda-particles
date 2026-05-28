#include "simulation/particle_system.cuh"
#include <thrust/device_ptr.h>
#include <thrust/fill.h>

void accumulateForces(ParticleSystem& ps) {
    // H/V line demo: no external forces — zero the force buffer via Thrust.
    // Add gravity or other forces here when needed:
    //   thrust::transform(vel_ptr, vel_ptr + ps.count, force_ptr, GravityFunctor{});
    auto ptr = thrust::device_pointer_cast(ps.forces);
    thrust::fill(ptr, ptr + ps.count, make_float4(0.0f, 0.0f, 0.0f, 0.0f));
}
