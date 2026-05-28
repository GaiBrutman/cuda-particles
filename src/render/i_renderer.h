#pragma once

#include "cuda_types.h"

class IRenderer {
public:
    virtual void          init(int w, int h, int maxParticles)       = 0;
    virtual float4*       getMappedPositionBuffer()                   = 0;
    virtual void          unmapPositionBuffer()                       = 0;
    virtual void          setVelocityBuffer(const float4* velocities) {}
    virtual void          render(int count, float time)               = 0;
    virtual const uchar4* getFramePixels()                            { return nullptr; }
    virtual void          shutdown()                                  = 0;
    virtual ~IRenderer() = default;
};
