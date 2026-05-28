#pragma once

#include "cuda_types.h"

// Maps particle velocity to RGBA.
// Horizontal movers (idx < halfCount): orange spectrum.
// Vertical movers   (idx >= halfCount): cyan spectrum.
// Brightness scales with speed (min 30%, max 100%).
__device__ inline uchar4 velocityToColorDevice(float4 vel, int idx, int halfCount) {
    float speed      = sqrtf(vel.x * vel.x + vel.y * vel.y);
    float brightness = fminf(speed / 160.0f, 1.0f) * 0.7f + 0.3f;

    unsigned char r, g, b;
    if (idx < halfCount) {
        r = (unsigned char)(255.0f * brightness);
        g = (unsigned char)(160.0f * brightness);
        b = (unsigned char)( 30.0f * brightness);
    } else {
        r = 0;
        g = (unsigned char)(200.0f * brightness);
        b = (unsigned char)(255.0f * brightness);
    }
    return uchar4{r, g, b, 255};
}
