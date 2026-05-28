#include "app.h"
#include "simulation/particle_system.cuh"
#include "render/offscreen/frame_exporter.h"
#include <cstdio>

#ifdef PROFILING
#include <chrono>
#include <algorithm>

using Clock = std::chrono::steady_clock;

static double ms_between(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
#endif

App::App(const Config& config, IRenderer* renderer)
    : m_config(config), m_renderer(renderer) {}

void App::run() {
    m_renderer->init(m_config.width, m_config.height, m_config.particleCount);

    ParticleSystem ps;
    particleSystemAlloc(ps, m_config.particleCount);
    ps.positions = m_renderer->getMappedPositionBuffer();  // renderer owns the buffer

    particleSystemInit(ps, m_config.width, m_config.height);

    FrameExporter exporter(m_config.outputDir);

#ifdef PROFILING
    double sum_sim = 0, sum_render = 0, sum_readback = 0, sum_save = 0;
    double min_total = 1e9, max_total = 0;

    // CUDA note: integrateParticles/render launch kernels asynchronously.
    // Their wall-clock cost here is just launch latency (~us).
    // getFramePixels() issues a synchronous D2H cudaMemcpy which drains all
    // prior GPU work first — so 'readback' captures actual GPU compute time.
    std::printf("%-6s  %8s %8s %10s %8s   (all ms)\n",
                "frame", "sim", "render", "readback", "save");
    auto wall_start = Clock::now();
#endif

    for (int frame = 0; frame < m_config.frames; ++frame) {
#ifdef PROFILING
        auto t0 = Clock::now();
#endif
        integrateParticles(ps, m_config.dt);

#ifdef PROFILING
        auto t1 = Clock::now();
#endif
        m_renderer->unmapPositionBuffer();
        m_renderer->render(m_config.particleCount, frame * m_config.dt);

#ifdef PROFILING
        auto t2 = Clock::now();
#endif
        const uchar4* pixels = m_renderer->getFramePixels();  // blocks until GPU done

#ifdef PROFILING
        auto t3 = Clock::now();
#endif
        if (pixels)
            exporter.save(pixels, m_config.width, m_config.height, frame);

#ifdef PROFILING
        auto t4 = Clock::now();

        double d_sim      = ms_between(t0, t1);
        double d_render   = ms_between(t1, t2);
        double d_readback = ms_between(t2, t3);
        double d_save     = ms_between(t3, t4);
        double d_total    = ms_between(t0, t4);

        sum_sim      += d_sim;
        sum_render   += d_render;
        sum_readback += d_readback;
        sum_save     += d_save;
        min_total     = std::min(min_total, d_total);
        max_total     = std::max(max_total, d_total);

        std::printf("%-6d  %8.3f %8.3f %10.3f %8.3f\n",
                    frame, d_sim, d_render, d_readback, d_save);
#endif
    }

#ifdef PROFILING
    double wall_s = ms_between(wall_start, Clock::now()) / 1000.0;
    int    n      = m_config.frames;

    std::printf("\n=== summary: %d frames  %.2fs  %.1f fps ===\n",
                n, wall_s, n / wall_s);
    std::printf("%-12s %8s\n", "phase", "avg(ms)");
    std::printf("%-12s %8.3f\n", "sim",      sum_sim      / n);
    std::printf("%-12s %8.3f\n", "render",   sum_render   / n);
    std::printf("%-12s %8.3f\n", "readback", sum_readback / n);
    std::printf("%-12s %8.3f\n", "save",     sum_save     / n);
    std::printf("%-12s %8.3f  (min %.3f  max %.3f)\n",
                "frame", (sum_sim + sum_render + sum_readback + sum_save) / n,
                min_total, max_total);
#endif

    ps.positions = nullptr;  // owned by renderer — do not free
    particleSystemFree(ps);
    m_renderer->shutdown();
}
