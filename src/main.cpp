#include "app/config.h"
#include "app/app.h"
#include "render/i_renderer.h"

#ifdef CPU_ONLY
#include "render/cpu/cpu_renderer.h"
#elif defined(USE_OPENGL)
#include "render/opengl/gl_renderer.h"
#else
#include "render/offscreen/offscreen_renderer.h"
#endif

#include <memory>

int main(int argc, char** argv) {
    Config config; // TODO: parse argv

    std::unique_ptr<IRenderer> renderer;

#ifdef CPU_ONLY
    renderer = std::make_unique<CpuRenderer>();
#elif defined(USE_OPENGL)
    renderer = std::make_unique<GlRenderer>();
#else
    renderer = std::make_unique<OffscreenRenderer>();
#endif

    App app(config, renderer.get());
    app.run();
    return 0;
}
