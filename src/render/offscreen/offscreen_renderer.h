#pragma once

#include "render/i_renderer.h"

class OffscreenRenderer : public IRenderer {
public:
    void          init(int w, int h, int maxParticles)        override;
    float4*       getMappedPositionBuffer()                    override;
    void          unmapPositionBuffer()                        override;
    void          setVelocityBuffer(const float4* velocities)  override;
    void          render(int count, float time)                override;
    const uchar4* getFramePixels()                             override;
    void          shutdown()                                   override;

private:
    float4*        d_positions   = nullptr;   // device — position buffer
    const float4*  d_velocities  = nullptr;   // device — not owned, set by App
    uchar4*        d_pixels      = nullptr;   // device — RGBA frame buffer
    uchar4*        h_pixels      = nullptr;   // pinned host — for D2H readback
    int            m_width       = 0;
    int            m_height      = 0;
    int            m_count       = 0;
};
