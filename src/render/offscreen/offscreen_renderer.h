#pragma once

#include "render/i_renderer.h"

class OffscreenRenderer : public IRenderer {
public:
    void          init(int w, int h, int maxParticles) override;
    float4*       getMappedPositionBuffer()             override;
    void          unmapPositionBuffer()                 override;
    void          render(int count, float time)         override;
    const uchar4* getFramePixels()                      override;
    void          shutdown()                            override;
};
