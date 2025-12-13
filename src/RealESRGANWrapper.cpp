/**
 * RealESRGANWrapper.cpp
 *
 * Implementation of real-time Real-ESRGAN upscaling for EWOCvj2
 * Uses native Real-ESRGAN library with ncnn/Vulkan
 *
 * License: GPL3
 */

#include "RealESRGANWrapper.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

// Real-ESRGAN and ncnn includes
#include "realesrgan.h"
#include "mat.h"
#include "gpu.h"

/**
 * Implementation wrapper for Real-ESRGAN
 * Uses pimpl pattern to hide ncnn/Real-ESRGAN headers from public interface
 */
class RealESRGANImpl {
public:
    ::RealESRGAN* upsampler = nullptr;
    int gpuId = 0;
    bool tta_mode = false;

    RealESRGANImpl() {
        upsampler = nullptr;
    }

    ~RealESRGANImpl() {
        if (upsampler) {
            delete upsampler;
            upsampler = nullptr;
        }
    }
};


RealESRGANUpscaler::RealESRGANUpscaler() {
    // Initialize stats
    lastInferenceTime = 0.0f;
    avgInferenceTime = 0.0f;
    lastFPS = 0.0f;
    frameCount = 0;

    esrgan = std::make_unique<RealESRGANImpl>();
}

RealESRGANUpscaler::~RealESRGANUpscaler() {
    // Cleanup handled by unique_ptr
}

bool RealESRGANUpscaler::initialize() {
    if (initialized) {
        return true;
    }

    try {
        // Enumerate GPUs and select the best one (prefer NVIDIA)
        int gpuId = -1;  // -1 = CPU

        if (useGPU) {
            int gpuCount = ncnn::get_gpu_count();
            std::cerr << "[RealESRGAN] Found " << gpuCount << " Vulkan device(s)" << std::endl;

            // List all GPUs and prefer NVIDIA
            int nvidiaGpu = -1;
            for (int i = 0; i < gpuCount; i++) {
                const ncnn::GpuInfo& info = ncnn::get_gpu_info(i);
                std::cerr << "[RealESRGAN] GPU " << i << ": " << info.device_name() << std::endl;

                // Check if this is an NVIDIA GPU
                std::string deviceName = info.device_name();
                if (deviceName.find("NVIDIA") != std::string::npos ||
                    deviceName.find("GeForce") != std::string::npos ||
                    deviceName.find("RTX") != std::string::npos) {
                    nvidiaGpu = i;
                    std::cerr << "[RealESRGAN] Found NVIDIA GPU at index " << i << std::endl;
                }
            }

            // Use NVIDIA GPU if found, otherwise use first GPU
            if (nvidiaGpu >= 0) {
                gpuId = nvidiaGpu;
                std::cerr << "[RealESRGAN] Selected NVIDIA GPU " << gpuId << std::endl;
            } else if (gpuCount > 0) {
                gpuId = 0;
                std::cerr << "[RealESRGAN] No NVIDIA GPU found, using GPU 0" << std::endl;
            } else {
                std::cerr << "[RealESRGAN] No Vulkan GPUs found, falling back to CPU" << std::endl;
                gpuId = -1;
            }
        }

        // Create Real-ESRGAN instance with selected GPU
        esrgan->upsampler = new ::RealESRGAN(gpuId, false); // tta_mode = false for speed

        if (!esrgan->upsampler) {
            setError("Failed to create Real-ESRGAN instance");
            std::cerr << "[RealESRGAN] " << lastError << std::endl;
            return false;
        }

        esrgan->gpuId = gpuId;
        initialized = true;

        if (gpuId >= 0) {
            const ncnn::GpuInfo& info = ncnn::get_gpu_info(gpuId);
            std::cerr << "[RealESRGAN] Initialized with Vulkan GPU: " << info.device_name() << std::endl;
        } else {
            std::cerr << "[RealESRGAN] Initialized with CPU inference" << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Initialization failed: ") + e.what());
        std::cerr << "[RealESRGAN] " << lastError << std::endl;
        return false;
    }
}


int RealESRGANUpscaler::loadModels(const std::string& modelsPath) {
    if (!initialized) {
        setError("Not initialized. Call initialize() first.");
        return 0;
    }

    upscaleModels.clear();
    currentModelIndex = -1;

    try {
        fs::path modelDir(modelsPath);

        // Check if directory exists
        if (!fs::exists(modelDir) || !fs::is_directory(modelDir)) {
            setError("Models directory does not exist: " + modelsPath);
            std::cerr << "[RealESRGAN] " << lastError << std::endl;
            return 0;
        }

        // Scan for .param files and find matching .bin files
        for (const auto& entry : fs::directory_iterator(modelDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".param") {
                fs::path paramPath = entry.path();
                fs::path binPath = paramPath;
                binPath.replace_extension(".bin");

                // Check if corresponding .bin file exists
                if (!fs::exists(binPath)) {
                    std::cerr << "[RealESRGAN] Skipping " << paramPath.filename()
                              << " - no matching .bin file" << std::endl;
                    continue;
                }

                UpscaleModel model;
                model.paramPath = paramPath;
                model.binPath = binPath;
                model.name = paramPath.stem().string();
                model.loaded = false;

                // Try to detect scale factor from filename
                std::string name_lower = model.name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                if (name_lower.find("x2") != std::string::npos || name_lower.find("-2") != std::string::npos) {
                    model.scaleFactor = 2;
                } else if (name_lower.find("x3") != std::string::npos || name_lower.find("-3") != std::string::npos) {
                    model.scaleFactor = 3;
                } else if (name_lower.find("x4") != std::string::npos || name_lower.find("-4") != std::string::npos) {
                    model.scaleFactor = 4;
                } else if (name_lower.find("x8") != std::string::npos || name_lower.find("-8") != std::string::npos) {
                    model.scaleFactor = 8;
                } else {
                    model.scaleFactor = 4; // Default to 4x
                }

                upscaleModels.push_back(model);

                std::cerr << "[RealESRGAN] Found model: " << model.name
                          << " (" << model.scaleFactor << "x)" << std::endl;
            }
        }

        std::cerr << "[RealESRGAN] Loaded " << upscaleModels.size() << " upscaling models" << std::endl;
        return static_cast<int>(upscaleModels.size());

    } catch (const std::exception& e) {
        setError(std::string("Failed to load models: ") + e.what());
        std::cerr << "[RealESRGAN] " << lastError << std::endl;
        return 0;
    }
}

bool RealESRGANUpscaler::setModel(int modelIndex) {
    if (!initialized) {
        setError("Not initialized");
        return false;
    }

    if (modelIndex < 0 || modelIndex >= static_cast<int>(upscaleModels.size())) {
        setError("Invalid model index");
        return false;
    }

    if (loadModel(upscaleModels[modelIndex])) {
        currentModelIndex = modelIndex;
        upscaleModels[modelIndex].loaded = true;
        std::cerr << "[RealESRGAN] Activated model: " << upscaleModels[modelIndex].name
                  << " (" << upscaleModels[modelIndex].scaleFactor << "x)" << std::endl;
        return true;
    }

    return false;
}

std::vector<std::string> RealESRGANUpscaler::getAvailableModels() const {
    std::vector<std::string> names;
    names.reserve(upscaleModels.size());
    for (const auto& model : upscaleModels) {
        names.push_back(model.name);
    }
    return names;
}

int RealESRGANUpscaler::getScaleFactor() const {
    if (currentModelIndex >= 0 && currentModelIndex < static_cast<int>(upscaleModels.size())) {
        return upscaleModels[currentModelIndex].scaleFactor;
    }
    return 1; // No upscaling if no model loaded
}

bool RealESRGANUpscaler::renderBuffer(const unsigned char* inputBuffer, int inputWidth, int inputHeight,
                                       unsigned char*& outputBuffer, int& outputWidth, int& outputHeight) {
    // Check if initialized
    if (!initialized || !esrgan || !esrgan->upsampler) {
        setError("Real-ESRGAN not initialized");
        return false;
    }

    // Check if model is loaded
    if (currentModelIndex < 0 || currentModelIndex >= upscaleModels.size()) {
        setError("No model loaded");
        return false;
    }

    // Passthrough mode - just copy input to output
    if (passthrough) {
        outputWidth = inputWidth;
        outputHeight = inputHeight;
        size_t bufferSize = inputWidth * inputHeight * 3;
        outputBuffer = new unsigned char[bufferSize];
        memcpy(outputBuffer, inputBuffer, bufferSize);
        return true;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        int scaleFactor = getScaleFactor();
        outputWidth = inputWidth * scaleFactor;
        outputHeight = inputHeight * scaleFactor;

        // Convert input from interleaved BGR to planar BGR
        // RealESRGAN process() expects interleaved bytes that it converts to planar via from_pixels()
        // But looking at the output being planar, let's pass it interleaved and let process() handle it
        if (!inputBuffer) {
            setError("Input buffer is null");
            return false;
        }

        // Create Mat from raw interleaved BGR bytes
        // For interleaved BGR: elemsize=1 (each element is 1 byte), elempack=3 (packed as BGR)
        // process() reads channels from elempack
        ncnn::Mat inMat(inputWidth, inputHeight, (void*)inputBuffer, (size_t)1, 3);

        if (inMat.empty()) {
            setError("Failed to create input Mat");
            return false;
        }

        std::cerr << "[RealESRGAN] Input Mat: w=" << inMat.w << " h=" << inMat.h
                  << " elemsize=" << inMat.elemsize << " elempack=" << inMat.elempack << std::endl;

        // Keep original tile size for all images - let the default tiny tiles work
        int originalTileSize = esrgan->upsampler->tilesize;
        int originalPrepadding = esrgan->upsampler->prepadding;

        // Allocate output BYTE buffer (process() writes bytes, not floats!)
        size_t bufferSize = (size_t)outputWidth * outputHeight * 3;
        outputBuffer = new unsigned char[bufferSize];

        // Initialize to a test pattern to see what gets written
        memset(outputBuffer, 128, bufferSize);  // Fill with gray

        std::cerr << "[RealESRGAN] Allocated buffer: " << bufferSize << " bytes ("
                  << outputWidth << "x" << outputHeight << "x3)" << std::endl;

        // Create output Mat wrapping the byte buffer
        // For interleaved BGR: elemsize=1 (each element is 1 byte), elempack=3 (BGR channels)
        ncnn::Mat outMat(outputWidth, outputHeight, (void*)outputBuffer, (size_t)1, 3);

        std::cerr << "[RealESRGAN] Output Mat: w=" << outMat.w << " h=" << outMat.h
                  << " elemsize=" << outMat.elemsize << " elempack=" << outMat.elempack
                  << " total_bytes=" << (outMat.w * outMat.h * outMat.elemsize) << std::endl;

        // Process with Real-ESRGAN (with tiny tiles for best performance)
        int result = esrgan->upsampler->process(inMat, outMat);

        if (result != 0) {
            setError("Real-ESRGAN processing failed");
            return false;
        }

        // Debug: Check output buffer (should already be interleaved BGR from to_pixels())
        std::cerr << "[RealESRGAN] First 30 output bytes after process(): ";
        for (int i = 0; i < 30; i++) {
            std::cerr << (int)outputBuffer[i] << " ";
        }
        std::cerr << std::endl;

        // Update statistics
        auto endTime = std::chrono::high_resolution_clock::now();
        float inferenceTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        updateStats(inferenceTime);
        return true;

    } catch (const std::exception& e) {
        setError(std::string("Buffer render failed: ") + e.what());
        std::cerr << "[RealESRGAN] " << lastError << std::endl;
        return false;
    }
}

bool RealESRGANUpscaler::loadModel(const UpscaleModel& model) {
    if (!esrgan->upsampler) {
        setError("Real-ESRGAN not initialized");
        return false;
    }

    try {
        std::cerr << "[RealESRGAN] Loading model: " << model.name << std::endl;

        // Load the model
        #ifdef _WIN32
            int result = esrgan->upsampler->load(model.paramPath.wstring(), model.binPath.wstring());
        #else
            int result = esrgan->upsampler->load(model.paramPath.string(), model.binPath.string());
        #endif

        if (result != 0) {
            setError("Failed to load model: " + model.name);
            std::cerr << "[RealESRGAN] " << lastError << std::endl;
            return false;
        }

        // Set tile size - use 100 as default for best performance on small images
        // MUST be > 0 to avoid division by zero in Real-ESRGAN
        // Tiny tiles (64-100) = each GPU operation is faster
        // Medium tiles (200-400) = balanced
        // Large tiles (800+) = for large images only
        int effectiveTileSize = (tileSize > 0) ? tileSize : 100;
        esrgan->upsampler->tilesize = effectiveTileSize;

        // Set prepadding (overlap between tiles to avoid seams)
        // Should be smaller than tilesize
        esrgan->upsampler->prepadding = std::min(10, effectiveTileSize / 4);

        // Set scale factor
        esrgan->upsampler->scale = model.scaleFactor;

        std::cerr << "[RealESRGAN] Tile size: " << effectiveTileSize << "x" << effectiveTileSize << ", prepadding: 10" << std::endl;

        std::cerr << "[RealESRGAN] Model loaded successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        setError(std::string("Failed to load model: ") + e.what());
        std::cerr << "[RealESRGAN] " << lastError << std::endl;
        return false;
    }
}

void RealESRGANUpscaler::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
}

void RealESRGANUpscaler::updateStats(float inferenceTime) {
    lastInferenceTime = inferenceTime;

    // Running average
    const float alpha = 0.1f;  // Smoothing factor
    avgInferenceTime = avgInferenceTime * (1.0f - alpha) + inferenceTime * alpha;

    // Calculate FPS
    lastFPS = inferenceTime > 0.0f ? 1000.0f / inferenceTime : 0.0f;

    frameCount++;
}
