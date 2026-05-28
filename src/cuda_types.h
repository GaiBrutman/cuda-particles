#pragma once

#ifdef CPU_ONLY

#include <cmath>

struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct uchar4 { unsigned char x, y, z, w; };

// ─── float3 math helpers ──────────────────────────────────────────────────────

inline float3 operator+(float3 a, float3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline float3 operator-(float3 a, float3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline float3 operator*(float3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float3 operator*(float s, float3 a) { return a * s; }

inline float dot(float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float  length   (float3 a)           { return std::sqrtf(dot(a, a)); }
inline float3 normalize(float3 a) { return a * (1.0f / length(a)); }

inline float3 cross    (float3 a, float3 b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}
#else
#include <cuda_runtime.h>
#endif
