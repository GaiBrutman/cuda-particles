#pragma once

#include <cuda_runtime.h>
#include <string>

class FrameExporter {
public:
    explicit FrameExporter(const std::string& outputDir);
    void save(const uchar4* pixels, int width, int height, int frameIndex);
};
