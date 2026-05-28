#include "frame_exporter.h"
#include "stb_image_write.h"
#include <cstdio>
#include <sys/stat.h>

FrameExporter::FrameExporter(const std::string& outputDir) : m_outputDir(outputDir) {
    mkdir(outputDir.c_str(), 0755);
}

void FrameExporter::save(const uchar4* pixels, int width, int height, int frameIndex) {
    char path[512];
    std::snprintf(path, sizeof(path), "%s/frame_%04d.png", m_outputDir.c_str(), frameIndex);
    stbi_write_png(path, width, height, 4, pixels, width * 4);
}
