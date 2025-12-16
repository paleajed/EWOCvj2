/**
 * ComputeStyleTransfer.h
 *
 * GPU-accelerated style transfer using OpenGL Compute Shaders
 * Runs entire neural network on GPU - zero CPU transfers
 *
 * Performance target: 1-5ms per frame (200-1000 FPS)
 *
 * License: GPL3
 */

#ifndef COMPUTESTYLETRANSFER_H
#define COMPUTESTYLETRANSFER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "UniformCache.h"

#ifdef _WIN32
#include "GL/glew.h"
#else
#include <GL/glew.h>
#endif

/**
 * GPU-accelerated style transfer using compute shaders
 *
 * Implements ReCoNet architecture entirely in GLSL compute shaders:
 * - All layers run on GPU
 * - Weights stored in SSBOs (Shader Storage Buffer Objects)
 * - Zero CPU-GPU transfers during inference
 * - Supports dynamic input resolution
 */
class ComputeStyleTransfer {
public:
    ComputeStyleTransfer();
    ~ComputeStyleTransfer();

    /**
     * Load ONNX model and compile compute shaders
     *
     * @param modelPath Path to .onnx model file
     * @return true if loaded successfully
     */
    bool loadModel(const std::string& modelPath);

    /**
     * Run style transfer on GPU texture
     *
     * @param inputTexture Input GL texture (RGB/RGBA)
     * @param outputTexture Output GL texture (can be different size)
     * @param inputWidth Input image width
     * @param inputHeight Input image height
     * @param outputWidth Output image width
     * @param outputHeight Output image height
     * @return true if inference succeeded
     */
    bool process(GLuint inputTexture, GLuint outputTexture, int inputWidth, int inputHeight, int outputWidth, int outputHeight);

    /**
     * Get last error message
     */
    std::string getLastError() const { return lastError; }

    /**
     * Check if model is loaded
     */
    bool isLoaded() const { return modelLoaded; }

    /**
     * Get inference time of last process() call (milliseconds)
     */
    float getLastInferenceTime() const { return lastInferenceTimeMs; }

private:
    // === Model state ===
    bool modelLoaded = false;
    std::string lastError;
    float lastInferenceTimeMs = 0.0f;
    int maxBatchSize = 8;  // Adaptive batch size based on GPU capability

    // === Compute shader programs ===
    GLuint normalizeProgram = 0;        // Input normalization [0,1] -> [-1,1]
    GLuint denormalizeProgram = 0;      // Output denormalization [-1,1] -> [0,1]
    GLuint lanczos3Program = 0;         // Lanczos3 resampling (up/downscaling)

    // ReCoNet layer programs
    GLuint conv2dProgram = 0;              // Standard convolution
    GLuint instanceNormStatsProgram = 0;   // Instance normalization - Pass 1 (stats)
    GLuint instanceNormApplyProgram = 0;   // Instance normalization - Pass 2 (apply)
    GLuint reluProgram = 0;                // ReLU activation
    GLuint tanhProgram = 0;                // Tanh activation
    GLuint clampNormalizeProgram = 0;      // Clamp [0,255] → [0,1]
    GLuint copyArrayTo2DProgram = 0;       // Copy array layer 0 to 2D texture
    GLuint transposedConvProgram = 0;      // Transposed convolution (upsampling)
    GLuint residualAddProgram = 0;         // Residual connection addition

    // === GPU buffers for weights ===
    struct LayerWeights {
        GLuint weightBuffer = 0;  // SSBO for conv weights
        GLuint biasBuffer = 0;    // SSBO for biases
        int inChannels = 0;
        int outChannels = 0;
        int kernelSize = 0;
        int stride = 1;
        int padding = 0;
    };

    std::map<std::string, LayerWeights> layerWeights;

    // Instance norm parameters (gamma, beta)
    struct InstanceNormParams {
        GLuint gammaBuffer = 0;
        GLuint betaBuffer = 0;
        int channels = 0;
    };
    std::map<std::string, InstanceNormParams> instanceNormParams;

    // Temporary stats buffer for instance normalization (reused across layers)
    GLuint statsBuffer = 0;
    int statsBufferSize = 0;

    // === Intermediate textures ===
    struct TempTexture {
        GLuint texture = 0;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool isRegular2D = false;  // Track if it's regular 2D vs array
    };

    std::vector<TempTexture> tempTextures;

    // === Uniform caches for performance ===
    std::unique_ptr<UniformCache> conv2dCache;
    std::unique_ptr<UniformCache> instanceNormStatsCache;
    std::unique_ptr<UniformCache> instanceNormApplyCache;
    std::unique_ptr<UniformCache> transposedConvCache;
    std::unique_ptr<UniformCache> residualAddCache;
    std::unique_ptr<UniformCache> clampNormalizeCache;

    // === Private methods ===

    /**
     * Compile all compute shader programs
     */
    bool compileShaders();

    /**
     * Compile single compute shader from source
     */
    GLuint compileComputeShader(const char* source, const std::string& shaderName);

    /**
     * Load weights from ONNX model into GPU buffers
     */
    bool loadWeightsFromONNX(const std::string& modelPath);

    /**
     * Load weights from pre-extracted binary file (no ONNX dependency)
     */
    bool loadWeightsFromBinary(const std::string& weightsPath);

    /**
     * Upload weight tensor to GPU SSBO
     */
    void uploadWeightToSSBO(const std::string& name,
                           const std::vector<float>& data,
                           const std::vector<int64_t>& dims);

    /**
     * Create or resize temporary texture
     */
    bool getOrCreateTempTexture(int index, int width, int height, int channels, GLuint& outTexture, bool forceRegular2D = false);

    /**
     * Free all GPU resources
     */
    void cleanup();

    /**
     * Run Conv2D layer
     */
    bool runConv2D(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                   int width, int height);

    /**
     * Run Instance Normalization
     */
    bool runInstanceNorm(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                         int width, int height, int channels);

    /**
     * Run ReLU activation
     */
    bool runReLU(GLuint inputTex, GLuint outputTex, int width, int height, int channels);

    /**
     * Run Tanh activation
     */
    bool runTanh(GLuint inputTex, GLuint outputTex, int width, int height, int channels);

    /**
     * Apply tanh for soft clamping to [0,1] values (darkens output)
     */
    bool runTanhSoftClamp(GLuint inputTex, GLuint outputTex, int width, int height, int channels);

    /**
     * Clamp [0,255] and normalize to [0,1]
     */
    bool runClampAndNormalize(GLuint inputTex, GLuint outputTex, int width, int height, int channels);

    /**
     * Run Transposed Conv2D (upsampling)
     */
    bool runTransposedConv(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                          int inputWidth, int inputHeight, int outputWidth, int outputHeight);

    /**
     * Run residual addition (x + residual)
     */
    bool runResidualAdd(GLuint inputTex, GLuint residualTex, GLuint outputTex,
                       int width, int height, int channels);

    /**
     * Normalize input texture [-1, 1]
     */
    bool runNormalize(GLuint inputTex, GLuint outputTex, int width, int height);

    /**
     * Denormalize output texture [0, 1]
     */
    bool runDenormalize(GLuint inputTex, GLuint outputTex, int width, int height);

    /**
     * Copy from texture array layer 0 to regular 2D texture
     */
    bool runCopyArrayToRegular(GLuint inputArrayTex, GLuint outputTex, int width, int height);

    /**
     * Lanczos3 resampling (high-quality up/downscaling)
     */
    bool runLanczos3(GLuint inputTex, GLuint outputTex, int inputWidth, int inputHeight, int outputWidth, int outputHeight);

    /**
     * Set error message
     */
    void setError(const std::string& error);
};

#endif // COMPUTESTYLETRANSFER_H
