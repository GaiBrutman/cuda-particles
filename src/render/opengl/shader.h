#pragma once

#ifdef USE_OPENGL

#include <string>

class Shader {
public:
    unsigned int id = 0;
    void load(const std::string& vertPath, const std::string& fragPath);
    void use() const;
};

#endif // USE_OPENGL
