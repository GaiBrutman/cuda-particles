#include "cpu_renderer.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "simulation/particle_system.cuh"

void CpuRenderer::init(int w, int h, int maxParticles) {
    m_width     = w;
    m_height    = h;
    m_positions = static_cast<float4*>(std::malloc(maxParticles * sizeof(float4)));
    m_pixels    = static_cast<uchar4*>(std::malloc(w * h * sizeof(uchar4)));
}

float4* CpuRenderer::getMappedPositionBuffer() { return m_positions; }
void    CpuRenderer::unmapPositionBuffer()      {}

void CpuRenderer::render(int count, float /*time*/) {
    uchar4 black{0, 0, 0, 255};
    std::fill(m_pixels, m_pixels + m_width * m_height, black);

    int half = count / 2;
    for (int i = 0; i < count; ++i) {
        // Brightness by depth
        float b = std::clamp(m_positions[i].z / PARTICLE_RADIUS * 0.5f + 0.5f, 0.0f, 1.0f);

        uchar4 color;
        if (i < half) {
            color = {(unsigned char)(255 * b), (unsigned char)(160 * b), (unsigned char)(30 * b), 255};
        } else {
            color = {0, (unsigned char)(200 * b), (unsigned char)(255 * b), 255};
        }

        int cx = (m_positions[i].x / (m_positions[i].z + CAMERA_DIST)) * FOCAL_LENGTH + m_width/2;
        int cy = (m_positions[i].y / (m_positions[i].z + CAMERA_DIST)) * FOCAL_LENGTH + m_height/2;

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
