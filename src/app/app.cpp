#include "app.h"
#include "simulation/particle_system.cuh"
#include "render/offscreen/frame_exporter.h"
#include <cstdio>

App::App(const Config& config, IRenderer* renderer)
    : m_config(config), m_renderer(renderer) {}

void App::run() {
    m_renderer->init(m_config.width, m_config.height, m_config.particleCount);

    ParticleSystem ps;
    particleSystemAlloc(ps, m_config.particleCount);

    // Alias ps.positions to the renderer's buffer so integrate writes directly into it
    std::free(ps.positions);
    ps.positions = m_renderer->getMappedPositionBuffer();

    particleSystemInit(ps, m_config.width, m_config.height);

    FrameExporter exporter(m_config.outputDir);

    for (int frame = 0; frame < m_config.frames; ++frame) {
        integrateParticles(ps, m_config.dt, m_config.width, m_config.height);
        m_renderer->unmapPositionBuffer();
        m_renderer->render(m_config.particleCount, frame * m_config.dt);

        const uchar4* pixels = m_renderer->getFramePixels();
        if (pixels)
            exporter.save(pixels, m_config.width, m_config.height, frame);

        if (frame % 60 == 0)
            std::printf("frame %d / %d\n", frame, m_config.frames);
    }

    ps.positions = nullptr;  // owned by renderer; don't free
    particleSystemFree(ps);
    m_renderer->shutdown();
}
