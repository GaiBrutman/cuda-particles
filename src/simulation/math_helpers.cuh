#pragma once

__device__ inline float3 operator+(float3 a, float3 b) { return make_float3(a.x+b.x, a.y+b.y, a.z+b.z); }
__device__ inline float3 operator-(float3 a, float3 b) { return make_float3(a.x-b.x, a.y-b.y, a.z-b.z); }
__device__ inline float3 operator*(float3 a, float  s) { return make_float3(a.x*s,   a.y*s,   a.z*s  ); }
__device__ inline float3 operator*(float  s, float3 a) { return a * s; }
 
__device__ __forceinline__ float dot(float3 a, float3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
 
// rnorm3df(x,y,z) is a single hardware instruction: 1 / sqrt(x²+y²+z²)
// Faster and more numerically stable than sqrtf + division.
__device__ __forceinline__ float3 normalize(float3 a) {
    float inv = rnorm3df(a.x, a.y, a.z);
    return a * inv;
}
 
__device__ __forceinline__ float3 cross(float3 a, float3 b) {
    return make_float3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}
