#pragma once

#include "render/i_renderer.h"

class CpuRenderer : public IRenderer {
public:
    void          init(int w, int h, int maxParticles)        override;
    float4*       getMappedPositionBuffer()                    override;
    void          unmapPositionBuffer()                        override;
    void          setVelocityBuffer(const float4* velocities)  override;
    void          render(int count, float time)                override;
    const uchar4* getFramePixels()                             override;
    void          shutdown()                                   override;

private:
    float4*        m_positions  = nullptr;
    const float4*  m_velocities = nullptr;  // not owned
    uchar4*        m_pixels     = nullptr;
    int            m_width      = 0;
    int            m_height     = 0;
};
