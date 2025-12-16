/**
 * AIStyleTransfer.h
 *
 * Real-time AI style transfer integration for EWOCvj2
 * Uses ONNX Runtime with GPU acceleration for neural style transfer
 *
 * Features:
 * - Multiple style model support (.onnx files)
 * - Async processing with PBO transfers
 * - GPU-accelerated inference (CUDA/DirectML)
 * - Thread-safe operation
 * - Zero-copy where possible
 *
 * License: GPL3
 */

#ifndef AAISTYLETRANSFER_H
#define AAISTYLETRANSFER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glew.h>

// Forward declarations for ONNX Runtime (to avoid including heavy headers in .h file)
namespace Ort {
    class Env;
    class Session;
    class SessionOptions;
    class MemoryInfo;
    class Value;
}

namespace fs = std::filesystem;

/**
 * Minimal FBO structure for texture input/output
 */
struct FBOstruct {
    GLuint framebuffer = 0;
    GLuint texture = 0;
    int width = 0;
    int height = 0;
};

/**
 * Style model metadata
 */
struct StyleModel {
    std::string name;           // Display name (derived from filename)
    fs::path path;              // Full path to .onnx file
    int inputWidth = 0;         // Expected input width (0 = any)
    int inputHeight = 0;        // Expected input height (0 = any)
    bool loaded = false;        // Whether model is currently loaded
    bool needsInputNormalization = false;  // Whether to normalize input to [-1, 1] (auto-detected)
};

/**
 * Processing job for async queue
 */
struct ProcessingJob {
    int frameIndex = 0;      // Which frame buffer to use
    int width = 0;
    int height = 0;
    float* inputBuffer = nullptr;   // Pre-downloaded input data
    float* outputBuffer = nullptr;  // Where to write output
    std::atomic<bool>* frameReady = nullptr;  // Signal when done
};

/**
 * AIStyleTransfer - Main class for neural style transfer
 *
 * Usage:
 *   AIStyleTransfer styleTransfer;
 *   if (styleTransfer.initialize()) {
 *       styleTransfer.loadStyles("./models/styles/");
 *       styleTransfer.setStyle(0);
 *       styleTransfer.render(inputFBO, outputFBO);
 *   }
 */
class AIStyleTransfer {
public:
    AIStyleTransfer();
    ~AIStyleTransfer();

    /**
     * Initialize ONNX Runtime environment and check dependencies
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Load all .onnx style models from directory
     * @param modelsPath Path to directory containing .onnx files
     * @return Number of models loaded
     */
    int loadStyles(const std::string& modelsPath);

    /**
     * Set active style model
     * @param styleIndex Index of style to use (0-based)
     * @return true if style loaded successfully
     */
    bool setStyle(int styleIndex);

    /**
     * Get current active style index
     * @return Index of current style (-1 if none active)
     */
    int getCurrentStyle() const { return currentStyleIndex; }

    /**
     * Get list of available style names
     * @return Vector of style names
     */
    std::vector<std::string> getAvailableStyles() const;

    /**
     * Apply style transfer to input FBO
     * Main render method - applies AI style transfer
     *
     * @param input Input FBO containing source texture
     * @param output Output FBO for result (will be created if needed)
     * @param async If true, returns immediately (non-blocking). Check isProcessing()
     * @return true if processing started/completed successfully
     */
    bool render(const FBOstruct& input, FBOstruct& output, bool async = false);

    /**
     * Check if async processing is in progress
     * @return true if currently processing
     */
    bool isProcessing() const { return processing.load(); }

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
     * Enable/disable GPU acceleration
     * Must be called before initialize()
     */
    void setGPUAcceleration(bool enable) { useGPU = enable; }

    /**
     * Set processing resolution (input will be scaled to this)
     * Lower resolution = faster but lower quality
     * Set to 0,0 to use input resolution (default)
     */
    void setProcessingResolution(int width, int height) {
        processingWidth = width;
        processingHeight = height;
    }

    /**
     * Enable/disable passthrough mode (bypass AI processing)
     */
    void setPassthrough(bool enable) { passthrough = enable; }

private:
    // ONNX Runtime members
    std::unique_ptr<Ort::Env> ortEnv;
    std::unique_ptr<Ort::Session> ortSession;
    std::unique_ptr<Ort::SessionOptions> ortSessionOptions;
    std::unique_ptr<Ort::MemoryInfo> ortMemoryInfo;

    // Style models
    std::vector<StyleModel> styleModels;
    int currentStyleIndex = -1;

    // OpenGL resources for PBO transfers (triple-buffered)
    GLuint downloadPBOs[3] = {0, 0, 0};      // PBOs for async texture download
    GLuint uploadPBOs[3] = {0, 0, 0};        // PBOs for async texture upload
    unsigned char* downloadMapPtr[3] = {nullptr, nullptr, nullptr};  // Persistent mapped download buffers
    unsigned char* uploadMapPtr[3] = {nullptr, nullptr, nullptr};    // Persistent mapped upload buffers
    int pboIndex = 0;
    int pboSize = 0;

    // Processing buffers (triple-buffered)
    std::vector<float> inputBuffers[3];   // CPU buffers for model input
    std::vector<float> outputBuffers[3];  // CPU buffers for model output

    // Temp FBOs for rescaling
    FBOstruct scaledInputFBO = {0, 0, 0, 0};
    FBOstruct scaledOutputFBO = {0, 0, 0, 0};

    // Triple-buffering state
    int currentFrame = 0;
    GLsync downloadFences[3] = {nullptr, nullptr, nullptr};  // GPU sync for downloads
    GLsync uploadFences[3] = {nullptr, nullptr, nullptr};    // GPU sync for uploads
    std::atomic<bool> frameReady[3] = {false, false, false}; // CPU processing complete
    int lastOutputFrame = -1;  // Last frame uploaded to output

    // Threading for async processing
    std::unique_ptr<std::thread> workerThread;
    std::queue<ProcessingJob> jobQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> processing{false};
    std::atomic<bool> shouldTerminate{false};

    // Configuration
    bool useGPU = true;
    int processingWidth = 0;      // 0 = use input resolution
    int processingHeight = 0;
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
    bool loadModel(const fs::path& modelPath);
    bool inferenceInternal(const float* input, float* output, int width, int height);
    void workerThreadFunc();
    bool setupPBOs(int width, int height);
    void cleanupPBOs();
    bool createTempFBO(FBOstruct& fbo, int width, int height);
    void deleteFBO(FBOstruct& fbo);

    // Async PBO transfer methods
    bool startAsyncDownload(GLuint fbo, int width, int height, int frameIndex);
    bool finishAsyncDownload(int frameIndex, int width, int height);
    bool startAsyncUpload(const float* buffer, int width, int height, int frameIndex);
    bool finishAsyncUpload(GLuint texture, int width, int height, int frameIndex);

    // Legacy sync methods (deprecated)
    bool downloadTextureToBuffer(GLuint texture, int width, int height, std::vector<float>& buffer);
    bool downloadTextureToBuffer(GLuint fbo, GLuint texture, int width, int height, std::vector<float>& buffer);
    bool uploadBufferToTexture(const std::vector<float>& buffer, int width, int height, GLuint texture);

    void setError(const std::string& error);
    void updateStats(float inferenceTime);

    // Texture format conversion helpers
    void rgbaToRgb(const std::vector<float>& rgba, std::vector<float>& rgb, int width, int height);
    void rgbToRgba(const std::vector<float>& rgb, std::vector<float>& rgba, int width, int height);
    void normalizeInput(std::vector<float>& data, int width, int height);
    void denormalizeOutput(std::vector<float>& data, int width, int height);
    void hwcToChw(const std::vector<float>& hwc, std::vector<float>& chw, int width, int height);
    void chwToHwc(const std::vector<float>& chw, std::vector<float>& hwc, int width, int height);
    void hwcToChw4(const std::vector<float>& hwc, std::vector<float>& chw, int width, int height);
    void chw4ToHwc(const std::vector<float>& chw, std::vector<float>& hwc, int width, int height);
};

#endif // AAISTYLETRANSFER_H

