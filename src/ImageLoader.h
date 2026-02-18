#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

struct ImageFrame {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
    double duration_ms = 100.0;
};

struct LoadedImage {
    std::vector<ImageFrame> frames;
    int width = 0;
    int height = 0;
    int channels = 0;
    int bpp = 0;

    int numFrames() const { return (int)frames.size(); }
    const uint8_t* getFrameData(int idx) const {
        if (idx < 0 || idx >= (int)frames.size()) idx = 0;
        return frames[idx].pixels.data();
    }
    int getFrameWidth(int idx = 0) const {
        if (idx < 0 || idx >= (int)frames.size()) idx = 0;
        return frames[idx].width;
    }
    int getFrameHeight(int idx = 0) const {
        if (idx < 0 || idx >= (int)frames.size()) idx = 0;
        return frames[idx].height;
    }
    double getFrameDuration(int idx = 0) const {
        if (idx < 0 || idx >= (int)frames.size()) idx = 0;
        return frames[idx].duration_ms;
    }
};

namespace ImageLoader {
    // Load a single image as RGBA pixels (upper-left origin)
    std::vector<uint8_t> loadImageRGBA(const std::string& path, int* outW, int* outH);

    // Load a single image as RGB pixels (upper-left origin)
    std::vector<uint8_t> loadImageRGB(const std::string& path, int* outW, int* outH);

    // Load a single image as grayscale pixels (upper-left origin)
    std::vector<uint8_t> loadImageGray(const std::string& path, int* outW, int* outH);

    // Get image dimensions without loading pixel data
    bool getImageDimensions(const std::string& path, int* outW, int* outH);

    // Load image with all frames (for animated GIFs)
    std::shared_ptr<LoadedImage> loadImageMultiFrame(const std::string& path);

    // Check if a file is a loadable image
    bool isImage(const std::string& path);

    // Save RGBA pixels as PNG
    bool saveImagePNG(const std::string& path, const uint8_t* pixels, int w, int h, int channels = 4);
}
