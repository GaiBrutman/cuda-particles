#pragma once

#ifdef USE_OPENGL

#include "render/i_renderer.h"

class GlRenderer : public IRenderer {
public:
    void    init(int w, int h, int maxParticles) override;
    float4* getMappedPositionBuffer()             override;
    void    unmapPositionBuffer()                 override;
    void    render(int count, float time)         override;
    void    shutdown()                            override;
};

#endif // USE_OPENGL
