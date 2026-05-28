#pragma once

#include "cuda_types.h"
#include "simulation/particle_system.cuh"

__device__ inline float clampf_(float v, float lo, float hi) {
    return fminf(fmaxf(v, lo), hi);
}

// Maps particle depth to RGBA.
// first half movers (idx < halfCount): orange spectrum.
// second half movers   (idx >= halfCount): cyan spectrum.
__device__ inline uchar4 depthToColorDevice(float depth, int idx, int halfCount) {
    unsigned char r, g, b;
    float brightness = clampf_(depth / PARTICLE_RADIUS * 0.5f + 0.5f, 0.0f, 1.0f);
    
    if (idx < halfCount) {
        r = (unsigned char)(255.0f * brightness);
        g = (unsigned char)(160.0f * brightness);
        b = (unsigned char)( 30.0f * brightness);
    } else {
        r = 0;
        g = (unsigned char)(200.0f * brightness);
        b = (unsigned char)(255.0f * brightness);
    }

    return make_uchar4(r, g, b, 255);
}
