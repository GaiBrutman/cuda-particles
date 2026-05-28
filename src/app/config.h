#pragma once

#include <string>

struct Config {
    int         width         = 1280;
    int         height        = 720;
    int         particleCount = 100;
    int         frames        = 300;
    float       dt            = 0.016f;
    std::string outputDir     = "./frames/";
};
