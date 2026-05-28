#pragma once

#ifdef USE_OPENGL

class ImGuiOverlay {
public:
    void init();
    void render();
    void shutdown();
};

#endif // USE_OPENGL
