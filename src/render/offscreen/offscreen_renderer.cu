#include "offscreen_renderer.h"
#include "color_mapper.cuh"
#include "simulation/particle_system.cuh"

// ── device kernels ────────────────────────────────────────────────────────────

__global__ void clearKernel(uchar4* pixels, int numPixels) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numPixels) return;
    pixels[idx] = uchar4{0, 0, 0, 255};
}

__global__ void colorMapKernel(const float4* __restrict__ positions,
                                const float4* __restrict__ velocities,
                                uchar4*                    pixels,
                                int count, int halfCount,
                                int width, int height) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;

    float4 pos   = positions[idx];
    float4 vel   = __ldg(&velocities[idx]);
    uchar4 color = velocityToColorDevice(vel, idx, halfCount);

    int cx = (int)pos.x;
    int cy = (int)pos.y;

    // 3×3 splat — inner loop fully unrolled by compiler
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int px = cx + dx;
            int py = cy + dy;
            if ((unsigned)px < (unsigned)width && (unsigned)py < (unsigned)height)
                pixels[py * width + px] = color;
        }
    }
}

// ── OffscreenRenderer ─────────────────────────────────────────────────────────

void OffscreenRenderer::init(int w, int h, int maxParticles) {
    m_width  = w;
    m_height = h;
    CUDA_CHECK(cudaMalloc    (&d_positions, maxParticles * sizeof(float4)));
    CUDA_CHECK(cudaMalloc    (&d_pixels,    w * h * sizeof(uchar4)));
    CUDA_CHECK(cudaMallocHost(&h_pixels,    w * h * sizeof(uchar4)));
}

float4* OffscreenRenderer::getMappedPositionBuffer() { return d_positions; }
void    OffscreenRenderer::unmapPositionBuffer()      {}

void OffscreenRenderer::setVelocityBuffer(const float4* velocities) {
    d_velocities = velocities;
}

void OffscreenRenderer::render(int count, float /*time*/) {
    int numPixels = m_width * m_height;

    int clearGrid = (numPixels + BLOCK_SIZE - 1) / BLOCK_SIZE;
    clearKernel<<<clearGrid, BLOCK_SIZE>>>(d_pixels, numPixels);
    CUDA_CHECK(cudaGetLastError());

    if (count > 0 && d_velocities) {
        int mapGrid = (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        colorMapKernel<<<mapGrid, BLOCK_SIZE>>>(
            d_positions, d_velocities, d_pixels,
            count, count / 2, m_width, m_height);
        CUDA_CHECK(cudaGetLastError());
    }
    m_count = count;
}

const uchar4* OffscreenRenderer::getFramePixels() {
    CUDA_CHECK(cudaMemcpy(h_pixels, d_pixels,
                          m_width * m_height * sizeof(uchar4),
                          cudaMemcpyDeviceToHost));
    return h_pixels;
}

void OffscreenRenderer::shutdown() {
    CUDA_CHECK(cudaFree    (d_positions));
    CUDA_CHECK(cudaFree    (d_pixels));
    CUDA_CHECK(cudaFreeHost(h_pixels));
    d_positions = nullptr;
    d_pixels    = nullptr;
    h_pixels    = nullptr;
}
