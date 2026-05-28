#include "config.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

Config Config::fromArgs(int argc, char** argv) {
    Config c;
    for (int i = 1; i < argc - 1; ++i) {
        const char* key = argv[i];
        const char* val = argv[i + 1];
        if      (std::strcmp(key, "--particles") == 0) { c.particleCount = std::atoi(val); ++i; }
        else if (std::strcmp(key, "--frames")    == 0) { c.frames        = std::atoi(val); ++i; }
        else if (std::strcmp(key, "--width")     == 0) { c.width         = std::atoi(val); ++i; }
        else if (std::strcmp(key, "--height")    == 0) { c.height        = std::atoi(val); ++i; }
        else if (std::strcmp(key, "--dt")        == 0) { c.dt            = std::atof(val); ++i; }
        else if (std::strcmp(key, "--output")    == 0) { c.outputDir     = val;            ++i; }
        else if (std::strcmp(key, "--backend")   == 0) {                                   ++i; }
        else { std::fprintf(stderr, "unknown arg: %s\n", key); }
    }
    return c;
}
