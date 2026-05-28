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

void CpuRenderer::setVelocityBuffer(const float4* velocities) {
    m_velocities = velocities;
}

void CpuRenderer::render(int count, float /*time*/) {
    uchar4 black{0, 0, 0, 255};
    std::fill(m_pixels, m_pixels + m_width * m_height, black);

    int half = count / 2;
    for (int i = 0; i < count; ++i) {
        // Brightness by speed
        float speed = 0.0f;
        if (m_velocities) {
            float vx = m_velocities[i].x;
            float vy = m_velocities[i].y;
            speed = std::sqrt(vx * vx + vy * vy);
        }
        float b = std::min(speed / 160.0f, 1.0f) * 0.7f + 0.3f;

        uchar4 color;
        if (i < half) {
            color = {(unsigned char)(255 * b), (unsigned char)(160 * b), (unsigned char)(30 * b), 255};
        } else {
            color = {0, (unsigned char)(200 * b), (unsigned char)(255 * b), 255};
        }

        int cx = (int)m_positions[i].x;
        int cy = (int)m_positions[i].y;
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
