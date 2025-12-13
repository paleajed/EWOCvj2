/**
 * RealESRGANWrapper.h
 *
 * Real-time Real-ESRGAN upscaling integration for EWOCvj2
 * Uses native Real-ESRGAN library with ncnn and Vulkan acceleration
 *
 * Features:
 * - Multiple upscaling factors (2x, 4x, etc.)
 * - Vulkan GPU acceleration via ncnn
 * - Thread-safe operation
 * - Tile-based processing for large images
 *
 * License: GPL3
 */

#ifndef REALESRGAN_WRAPPER_H
#define REALESRGAN_WRAPPER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glew.h>

// Use FBOstruct from AIStyleTransfer.h
#include "AIStyleTransfer.h"

// Forward declarations for Real-ESRGAN/ncnn
class RealESRGANImpl;

namespace fs = std::filesystem;

/**
 * Upscaling model metadata
 */
struct UpscaleModel {
    std::string name;           // Display name (derived from filename)
    fs::path paramPath;         // Path to .param file
    fs::path binPath;           // Path to .bin file
    int scaleFactor = 4;        // Upscaling factor (2x, 4x, etc.)
    int tileSize = 0;           // Tile size for processing (0 = auto)
    bool loaded = false;        // Whether model is currently loaded
};

/**
 * RealESRGANUpscaler - Main class for neural upscaling
 * Note: Renamed from RealESRGAN to avoid collision with ncnn's RealESRGAN class
 *
 * Usage:
 *   RealESRGANUpscaler upscaler;
 *   if (upscaler.initialize()) {
 *       upscaler.loadModels("./models/upscale/");
 *       upscaler.setModel(0);  // Select first model
 *       upscaler.render(inputFBO, outputFBO);
 *   }
 */
class RealESRGANUpscaler {
public:
    RealESRGANUpscaler();
    ~RealESRGANUpscaler();

    /**
     * Initialize Real-ESRGAN and Vulkan
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Load all Real-ESRGAN models from directory
     * Looks for .param/.bin file pairs
     * @param modelsPath Path to directory containing model files
     * @return Number of models loaded
     */
    int loadModels(const std::string& modelsPath);

    /**
     * Set active upscaling model
     * @param modelIndex Index of model to use (0-based)
     * @return true if model loaded successfully
     */
    bool setModel(int modelIndex);

    /**
     * Get current active model index
     * @return Index of current model (-1 if none active)
     */
    int getCurrentModel() const { return currentModelIndex; }

    /**
     * Get list of available model names
     * @return Vector of model names
     */
    std::vector<std::string> getAvailableModels() const;

    /**
     * Apply upscaling to raw RGB buffer (for ffmpeg transcoding/HAP encoding)
     * Efficient method that works directly with CPU buffers
     *
     * @param inputBuffer Input RGB buffer (must be width*height*3 bytes)
     * @param inputWidth Input image width
     * @param inputHeight Input image height
     * @param outputBuffer Output RGB buffer (allocated by function, caller must delete[])
     * @param outputWidth Output image width (set by function based on scale factor)
     * @param outputHeight Output image height (set by function based on scale factor)
     * @return true if processing completed successfully
     */
    bool renderBuffer(const unsigned char* inputBuffer, int inputWidth, int inputHeight,
                      unsigned char*& outputBuffer, int& outputWidth, int& outputHeight);

    /**
     * Get last error message
     * @return Error string (empty if no error)
     */
    std::string getLastError() const { return lastError; }

    /**
     * Get inference statistics
     */
    float getLastInferenceTime() const { return lastInferenceTime; }
    float getAverageInferenceTime() const { return avgInferenceTime; }
    float getLastFPS() const { return lastFPS; }

    /**
     * Enable/disable GPU acceleration (Vulkan)
     * Must be called before initialize()
     */
    void setGPUAcceleration(bool enable) { useGPU = enable; }

    /**
     * Enable/disable passthrough mode (bypass upscaling)
     */
    void setPassthrough(bool enable) { passthrough = enable; }

    /**
     * Set tile size for processing (0 = auto)
     * Larger tiles = faster but more VRAM
     * Common values: 0 (auto), 200, 400, 800
     */
    void setTileSize(int size) { tileSize = size; }

    /**
     * Get current upscaling factor
     */
    int getScaleFactor() const;

private:
    // Real-ESRGAN instance (pimpl pattern to hide ncnn headers)
    std::unique_ptr<RealESRGANImpl> esrgan;

    // Upscaling models
    std::vector<UpscaleModel> upscaleModels;
    int currentModelIndex = -1;

    // Configuration
    bool useGPU = true;
    int tileSize = 0;           // 0 = auto
    bool passthrough = false;
    bool initialized = false;

    // Statistics
    float lastInferenceTime = 0.0f;
    float avgInferenceTime = 0.0f;
    float lastFPS = 0.0f;
    int frameCount = 0;

    // Error handling
    mutable std::mutex errorMutex;
    std::string lastError;

    // Private methods
    bool loadModel(const UpscaleModel& model);
    void setError(const std::string& error);
    void updateStats(float inferenceTime);
};

#endif // REALESRGAN_H
