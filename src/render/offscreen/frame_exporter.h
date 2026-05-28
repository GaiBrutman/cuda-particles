#pragma once

#include "cuda_types.h"
#include <string>

class FrameExporter {
public:
    explicit FrameExporter(const std::string& outputDir);
    void save(const uchar4* pixels, int width, int height, int frameIndex);

private:
    std::string m_outputDir;
};
