#include "cpu_renderer.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

void CpuRenderer::init(int w, int h, int maxParticles) {
    m_width     = w;
    m_height    = h;
    m_positions = static_cast<float4*>(std::malloc(maxParticles * sizeof(float4)));
    m_pixels    = static_cast<uchar4*>(std::malloc(w * h * sizeof(uchar4)));
}

float4* CpuRenderer::getMappedPositionBuffer() { return m_positions; }
void    CpuRenderer::unmapPositionBuffer()      {}

void CpuRenderer::render(int count, float /*time*/) {
    // Clear to opaque black
    uchar4 black{0, 0, 0, 255};
    std::fill(m_pixels, m_pixels + m_width * m_height, black);

    for (int i = 0; i < count; ++i) {
        int cx = (int)m_positions[i].x;
        int cy = (int)m_positions[i].y;

        // Color: horizontal-movers orange, vertical-movers cyan
        bool isVertical = (i >= count / 2);
        uchar4 color = isVertical ? uchar4{0, 200, 255, 255} : uchar4{255, 160, 30, 255};

        // 3×3 splat
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < m_width && py >= 0 && py < m_height)
                    m_pixels[py * m_width + px] = color;
            }
        }
    }
}

const uchar4* CpuRenderer::getFramePixels() { return m_pixels; }

void CpuRenderer::shutdown() {
    std::free(m_positions);
    std::free(m_pixels);
    m_positions = nullptr;
    m_pixels    = nullptr;
}
