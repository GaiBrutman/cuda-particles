#pragma once

#include "config.h"
#include "render/i_renderer.h"

class App {
public:
    App(const Config& config, IRenderer* renderer);
    void run();

private:
    Config    m_config;
    IRenderer* m_renderer;
};
