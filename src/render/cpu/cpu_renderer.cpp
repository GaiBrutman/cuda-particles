#include "cpu_renderer.h"
#include <cstdlib>

void CpuRenderer::init(int w, int h, int maxParticles) {
    m_width     = w;
    m_height    = h;
    m_positions = static_cast<float4*>(std::malloc(maxParticles * sizeof(float4)));
    m_pixels    = static_cast<uchar4*>(std::malloc(w * h * sizeof(uchar4)));
}

float4* CpuRenderer::getMappedPositionBuffer() { return m_positions; }
void    CpuRenderer::unmapPositionBuffer()      {}

void CpuRenderer::render(int count, float time) {
    // TODO: CPU rasterize particles into m_pixels (splat point sprites)
}

const uchar4* CpuRenderer::getFramePixels() { return m_pixels; }

void CpuRenderer::shutdown() {
    std::free(m_positions);
    std::free(m_pixels);
    m_positions = nullptr;
    m_pixels    = nullptr;
}
