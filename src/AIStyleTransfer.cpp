/**
 * AIStyleTransfer.cpp
 *
 * Implementation of real-time AI style transfer for EWOCvj2
 *
 * License: GPL3
 */

#include "AIStyleTransfer.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

// ONNX Runtime includes
#include <onnxruntime_cxx_api.h>


AIStyleTransfer::AIStyleTransfer() {
    // Initialize stats
    lastInferenceTime = 0.0f;
    avgInferenceTime = 0.0f;
    lastFPS = 0.0f;
    frameCount = 0;
}

AIStyleTransfer::~AIStyleTransfer() {
    // Signal worker thread to terminate
    shouldTerminate.store(true);
    queueCV.notify_all();

    if (workerThread && workerThread->joinable()) {
        workerThread->join();
    }

    // Cleanup OpenGL resources
    cleanupPBOs();
    deleteFBO(scaledInputFBO);
    deleteFBO(scaledOutputFBO);

    // ONNX Runtime resources will be cleaned up by unique_ptr
}

bool AIStyleTransfer::initialize() {
    if (initialized) {
        return true;
    }

    try {
        // Initialize ONNX Runtime environment
        ortEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "EWOCvj2_StyleTransfer");

        // Create session options
        ortSessionOptions = std::make_unique<Ort::SessionOptions>();
        ortSessionOptions->SetIntraOpNumThreads(4);
        ortSessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Setup GPU acceleration if requested
        if (useGPU) {
            try {
                #ifdef _WIN32
                    // Use DirectML on Windows (ONNX Runtime 1.23.0+)
                    ortSessionOptions->AppendExecutionProvider("DML");
                    std::cerr << "[AIStyleTransfer] Using DirectML GPU acceleration" << std::endl;
                #else
                    // Use CUDA on Linux
                    ortSessionOptions->AppendExecutionProvider("CUDA");
                    std::cerr << "[AIStyleTransfer] Using CUDA GPU acceleration" << std::endl;
                #endif
            } catch (const Ort::Exception& e) {
                std::cerr << "[AIStyleTransfer] GPU acceleration unavailable: " << e.what()
                          << ", falling back to CPU" << std::endl;
                useGPU = false;
            }
        } else {
            std::cerr << "[AIStyleTransfer] Using CPU inference" << std::endl;
        }

        // Create memory info
        ortMemoryInfo = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)
        );

        // Start worker thread for async processing
        workerThread = std::make_unique<std::thread>(&AIStyleTransfer::workerThreadFunc, this);

        initialized = true;
        std::cerr << "[AIStyleTransfer] Initialized successfully" << std::endl;
        return true;

    } catch (const Ort::Exception& e) {
        setError(std::string("ONNX Runtime initialization failed: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return false;
    } catch (const std::exception& e) {
        setError(std::string("Initialization failed: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return false;
    }
}


int AIStyleTransfer::loadStyles(const std::string& modelsPath) {
    if (!initialized) {
        setError("Not initialized. Call initialize() first.");
        return 0;
    }

    styleModels.clear();
    currentStyleIndex = -1;

    try {
        fs::path modelDir(modelsPath);

        // Check if directory exists
        if (!fs::exists(modelDir) || !fs::is_directory(modelDir)) {
            setError("Models directory does not exist: " + modelsPath);
            std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
            return 0;
        }

        // Scan for .onnx files
        for (const auto& entry : fs::directory_iterator(modelDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".onnx") {
                StyleModel model;
                model.path = entry.path();
                model.name = entry.path().stem().string();
                model.loaded = false;
                styleModels.push_back(model);

                std::cerr << "[AIStyleTransfer] Found style: " << model.name << std::endl;
            }
        }

        std::cerr << "[AIStyleTransfer] Loaded " << styleModels.size() << " style models" << std::endl;
        return static_cast<int>(styleModels.size());

    } catch (const std::exception& e) {
        setError(std::string("Failed to load styles: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return 0;
    }
}

bool AIStyleTransfer::setStyle(int styleIndex) {
    if (!initialized) {
        setError("Not initialized");
        return false;
    }

    if (styleIndex < 0 || styleIndex >= static_cast<int>(styleModels.size())) {
        setError("Invalid style index");
        return false;
    }

    // Wait for any in-progress processing
    while (processing.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (loadModel(styleModels[styleIndex].path)) {
        currentStyleIndex = styleIndex;
        styleModels[styleIndex].loaded = true;
        std::cerr << "[AIStyleTransfer] Activated style: " << styleModels[styleIndex].name << std::endl;
        return true;
    }

    return false;
}

std::vector<std::string> AIStyleTransfer::getAvailableStyles() const {
    std::vector<std::string> names;
    names.reserve(styleModels.size());
    for (const auto& model : styleModels) {
        names.push_back(model.name);
    }
    return names;
}

bool AIStyleTransfer::render(const FBOstruct& input, FBOstruct& output, bool async) {
    if (!initialized) {
        setError("Not initialized");
        return false;
    }

    // Passthrough mode - just copy input to output
    if (passthrough || currentStyleIndex < 0 || !ortSession) {
        // Simple texture copy using FBO blit
        glBindFramebuffer(GL_READ_FRAMEBUFFER, input.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
        glBlitFramebuffer(0, 0, input.width, input.height,
                         0, 0, output.width, output.height,
                         GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    // Determine processing resolution
    int procWidth = processingWidth > 0 ? processingWidth : input.width;
    int procHeight = processingHeight > 0 ? processingHeight : input.height;

    // Ensure output FBO exists and has correct size
    if (output.framebuffer == 0 || output.width != input.width || output.height != input.height) {
        if (!createTempFBO(output, input.width, input.height)) {
            setError("Failed to create output FBO");
            return false;
        }
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Determine if we need to scale the input
        bool needsScaling = (procWidth != input.width || procHeight != input.height);

        FBOstruct inputToUse = input;

        // Create scaled input FBO if needed
        if (needsScaling) {
            if (scaledInputFBO.width != procWidth || scaledInputFBO.height != procHeight) {
                deleteFBO(scaledInputFBO);
                if (!createTempFBO(scaledInputFBO, procWidth, procHeight)) {
                    setError("Failed to create scaled input FBO");
                    return false;
                }
            }

            // Scale input to processing resolution using FBO blit
            glBindFramebuffer(GL_READ_FRAMEBUFFER, input.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scaledInputFBO.framebuffer);
            glBlitFramebuffer(0, 0, input.width, input.height,
                             0, 0, procWidth, procHeight,
                             GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            inputToUse = scaledInputFBO;
        }

        // Allocate buffers if needed
        size_t inputSize = static_cast<size_t>(procWidth) * procHeight * 3;
        size_t outputSize = inputSize;
        if (inputBuffer.size() != inputSize) {
            inputBuffer.resize(inputSize);
        }
        if (outputBuffer.size() != outputSize) {
            outputBuffer.resize(outputSize);
        }

        // Download texture to CPU buffer
        if (!downloadTextureToBuffer(inputToUse.framebuffer, inputToUse.texture, procWidth, procHeight, inputBuffer)) {
            setError("Failed to download input texture");
            return false;
        }


        // Normalize input (models typically expect [0,1] or [-1,1])
        normalizeInput(inputBuffer, procWidth, procHeight);

        // Convert from HWC (interleaved) to CHW (planar) format for model
        std::vector<float> inputCHW;
        hwcToChw(inputBuffer, inputCHW, procWidth, procHeight);

        // Run inference (output will be in CHW format)
        std::vector<float> outputCHW(inputCHW.size());
        if (!inferenceInternal(inputCHW.data(), outputCHW.data(), procWidth, procHeight)) {
            // Error already set by inferenceInternal
            return false;
        }

        // Convert output from CHW (planar) back to HWC (interleaved)
        chwToHwc(outputCHW, outputBuffer, procWidth, procHeight);

        // Denormalize output
        denormalizeOutput(outputBuffer, procWidth, procHeight);

        // Handle output scaling if needed
        if (needsScaling) {
            // Create scaled output FBO if needed
            if (scaledOutputFBO.width != procWidth || scaledOutputFBO.height != procHeight) {
                deleteFBO(scaledOutputFBO);
                if (!createTempFBO(scaledOutputFBO, procWidth, procHeight)) {
                    setError("Failed to create scaled output FBO");
                    return false;
                }
            }

            // Upload result to scaled output texture
            if (!uploadBufferToTexture(outputBuffer, procWidth, procHeight, scaledOutputFBO.texture)) {
                setError("Failed to upload output texture");
                return false;
            }

            // Scale back up to output resolution using FBO blit
            glBindFramebuffer(GL_READ_FRAMEBUFFER, scaledOutputFBO.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
            glBlitFramebuffer(0, 0, procWidth, procHeight,
                             0, 0, output.width, output.height,
                             GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            // Upload result directly to output texture
            if (!uploadBufferToTexture(outputBuffer, procWidth, procHeight, output.texture)) {
                setError("Failed to upload output texture");
                return false;
            }
        }

        // Update statistics
        auto endTime = std::chrono::high_resolution_clock::now();
        float inferenceTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        updateStats(inferenceTime);

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Render failed: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return false;
    }
}

bool AIStyleTransfer::loadModel(const fs::path& modelPath) {
    try {
        std::cerr << "[AIStyleTransfer] Loading model: " << modelPath << std::endl;

        #ifdef _WIN32
            // Windows requires wstring path
            ortSession = std::make_unique<Ort::Session>(*ortEnv, modelPath.wstring().c_str(), *ortSessionOptions);
        #else
            // Linux uses string path
            ortSession = std::make_unique<Ort::Session>(*ortEnv, modelPath.string().c_str(), *ortSessionOptions);
        #endif

        // Get input/output info
        size_t numInputNodes = ortSession->GetInputCount();
        size_t numOutputNodes = ortSession->GetOutputCount();

        std::cerr << "[AIStyleTransfer] Model loaded. Inputs: " << numInputNodes
                  << ", Outputs: " << numOutputNodes << std::endl;

        // Get expected input dimensions from model
        if (numInputNodes > 0) {
            Ort::TypeInfo inputTypeInfo = ortSession->GetInputTypeInfo(0);
            auto tensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
            std::vector<int64_t> inputShape = tensorInfo.GetShape();

            std::cerr << "[AIStyleTransfer] Input shape: [";
            for (size_t i = 0; i < inputShape.size(); i++) {
                std::cerr << inputShape[i];
                if (i < inputShape.size() - 1) std::cerr << ", ";
            }
            std::cerr << "]" << std::endl;

            // Assume NCHW format: [batch, channels, height, width]
            if (inputShape.size() == 4) {
                int modelHeight = static_cast<int>(inputShape[2]);
                int modelWidth = static_cast<int>(inputShape[3]);

                // Set processing resolution to match model's expected input
                processingHeight = modelHeight;
                processingWidth = modelWidth;

                std::cerr << "[AIStyleTransfer] Model expects input size: "
                          << modelWidth << "x" << modelHeight << std::endl;
                std::cerr << "[AIStyleTransfer] Auto-set processing resolution to match" << std::endl;
            }
        }

        return true;

    } catch (const Ort::Exception& e) {
        setError(std::string("Failed to load model: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return false;
    }
}

bool AIStyleTransfer::inferenceInternal(const float* input, float* output, int width, int height) {
    if (!ortSession) {
        setError("No model loaded");
        return false;
    }

    try {
        // Get input and output names from model
        Ort::AllocatorWithDefaultOptions allocator;

        // Get input name
        Ort::AllocatedStringPtr inputNameAlloc = ortSession->GetInputNameAllocated(0, allocator);
        const char* inputName = inputNameAlloc.get();

        // Get output name
        Ort::AllocatedStringPtr outputNameAlloc = ortSession->GetOutputNameAllocated(0, allocator);
        const char* outputName = outputNameAlloc.get();

        std::cerr << "[AIStyleTransfer] Model input name: " << inputName << ", output name: " << outputName << std::endl;

        // Create input tensor shape [1, 3, height, width] (NCHW format common for PyTorch)
        std::vector<int64_t> inputShape = {1, 3, height, width};
        size_t inputSize = static_cast<size_t>(width) * height * 3;

        // Create input tensor
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            *ortMemoryInfo,
            const_cast<float*>(input),
            inputSize,
            inputShape.data(),
            inputShape.size()
        );

        // Input/output names from model
        const char* inputNames[] = {inputName};
        const char* outputNames[] = {outputName};

        // Run inference
        auto outputTensors = ortSession->Run(
            Ort::RunOptions{nullptr},
            inputNames,
            &inputTensor,
            1,
            outputNames,
            1
        );

        // Copy output data
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        std::copy(outputData, outputData + inputSize, output);

        return true;

    } catch (const Ort::Exception& e) {
        setError(std::string("Inference failed: ") + e.what());
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;
        return false;
    }
}

void AIStyleTransfer::workerThreadFunc() {
    while (!shouldTerminate.load()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this] { return !jobQueue.empty() || shouldTerminate.load(); });

        if (shouldTerminate.load()) {
            break;
        }

        if (!jobQueue.empty()) {
            ProcessingJob job = jobQueue.front();
            jobQueue.pop();
            lock.unlock();

            processing.store(true);

            // Process job here (async implementation)
            // For now, this is a placeholder for future async implementation

            processing.store(false);
            if (job.completed) {
                job.completed->store(true);
            }
        }
    }
}

bool AIStyleTransfer::setupPBOs(int width, int height) {
    int newPBOSize = width * height * 4 * sizeof(float);  // RGBA float

    if (pbos[0] == 0 || pboSize != newPBOSize) {
        cleanupPBOs();

        pboSize = newPBOSize;
        glGenBuffers(2, pbos);

        for (int i = 0; i < 2; i++) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, nullptr, GL_STREAM_READ);
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        std::cerr << "[AIStyleTransfer] Created PBOs for " << width << "x" << height << std::endl;
    }

    return true;
}

void AIStyleTransfer::cleanupPBOs() {
    if (pbos[0] != 0) {
        glDeleteBuffers(2, pbos);
        pbos[0] = pbos[1] = 0;
        pboSize = 0;
    }
}

bool AIStyleTransfer::createTempFBO(FBOstruct& fbo, int width, int height) {
    // Delete existing FBO if dimensions changed
    if (fbo.framebuffer != 0 && (fbo.width != width || fbo.height != height)) {
        deleteFBO(fbo);
    }

    if (fbo.framebuffer == 0) {
        // Generate framebuffer
        glGenFramebuffers(1, &fbo.framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo.framebuffer);

        // Generate texture
        glGenTextures(1, &fbo.texture);
        glBindTexture(GL_TEXTURE_2D, fbo.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.texture, 0);

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[AIStyleTransfer] Failed to create FBO: " << status << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        fbo.width = width;
        fbo.height = height;
    }

    return true;
}

void AIStyleTransfer::deleteFBO(FBOstruct& fbo) {
    if (fbo.framebuffer != 0) {
        glDeleteFramebuffers(1, &fbo.framebuffer);
        fbo.framebuffer = 0;
    }
    if (fbo.texture != 0) {
        glDeleteTextures(1, &fbo.texture);
        fbo.texture = 0;
    }
    fbo.width = 0;
    fbo.height = 0;
}

bool AIStyleTransfer::downloadTextureToBuffer(GLuint texture, int width, int height, std::vector<float>& buffer) {
    // This is a fallback - should be called with FBO version
    std::cerr << "[AIStyleTransfer] Warning: downloadTextureToBuffer called without FBO" << std::endl;
    return false;
}


bool AIStyleTransfer::downloadTextureToBuffer(GLuint fbo, GLuint texture, int width, int height, std::vector<float>& buffer) {
    // Clear any previous errors
    while (glGetError() != GL_NO_ERROR);

    // Save current bindings
    GLint currentReadFBO;
    GLint currentPBO;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentReadFBO);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &currentPBO);

    std::cerr << "[AIStyleTransfer] Download: FBO=" << fbo << " texture=" << texture
              << " size=" << width << "x" << height << std::endl;
    std::cerr << "[AIStyleTransfer] Current read FBO=" << currentReadFBO
              << " PBO=" << currentPBO << std::endl;

    // Unbind any pixel pack buffer
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] Error after unbinding PBO: " << err << std::endl;
    }

    // Allocate buffer for RGBA byte data
    buffer.resize(static_cast<size_t>(width) * height * 3);
    std::vector<unsigned char> rgbaBuffer(static_cast<size_t>(width) * height * 4);

    // Use the provided FBO directly
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] Error after binding read FBO: " << err << std::endl;
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] Error after glReadBuffer: " << err << std::endl;
    }

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    std::cerr << "[AIStyleTransfer] Framebuffer status: " << status
              << (status == GL_FRAMEBUFFER_COMPLETE ? " (COMPLETE)" : " (INCOMPLETE!)") << std::endl;

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[AIStyleTransfer] Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, currentPBO);
        return false;
    }

    // Get texture info
    GLint texWidth, texHeight, texFormat;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &texFormat);
    std::cerr << "[AIStyleTransfer] Texture info: " << texWidth << "x" << texHeight
              << " format=" << texFormat << std::endl;

    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] Error after getting texture info: " << err << std::endl;
    }

    // Read pixels as UNSIGNED_BYTE (matches GL_RGBA8)
    std::cerr << "[AIStyleTransfer] About to call glReadPixels..." << std::endl;
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());

    GLenum error = glGetError();
    std::cerr << "[AIStyleTransfer] After glReadPixels, error: " << error << std::endl;

    // Restore bindings
    glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, currentPBO);

    if (error != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] OpenGL error in downloadTextureToBuffer: " << error << std::endl;
        return false;
    }

    // Convert RGBA unsigned byte to RGB float [0,1]
    for (size_t i = 0; i < width * height; ++i) {
        buffer[i * 3 + 0] = rgbaBuffer[i * 4 + 0] / 255.0f;
        buffer[i * 3 + 1] = rgbaBuffer[i * 4 + 1] / 255.0f;
        buffer[i * 3 + 2] = rgbaBuffer[i * 4 + 2] / 255.0f;
    }

    return true;
}

bool AIStyleTransfer::uploadBufferToTexture(const std::vector<float>& buffer, int width, int height, GLuint texture) {
    // Convert RGB float to RGBA unsigned byte
    std::vector<unsigned char> rgbaBuffer(static_cast<size_t>(width) * height * 4);

    for (size_t i = 0; i < width * height; ++i) {
        rgbaBuffer[i * 4 + 0] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 0], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 1] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 1], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 2] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 2], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 3] = 255;
    }

    // Upload to texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "[AIStyleTransfer] OpenGL error in uploadBufferToTexture: " << error << std::endl;
        return false;
    }

    return true;
}

void AIStyleTransfer::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
}

void AIStyleTransfer::updateStats(float inferenceTime) {
    lastInferenceTime = inferenceTime;

    // Running average
    const float alpha = 0.1f;  // Smoothing factor
    avgInferenceTime = avgInferenceTime * (1.0f - alpha) + inferenceTime * alpha;

    // Calculate FPS
    lastFPS = inferenceTime > 0.0f ? 1000.0f / inferenceTime : 0.0f;

    frameCount++;
}

void AIStyleTransfer::rgbaToRgb(const std::vector<float>& rgba, std::vector<float>& rgb, int width, int height) {
    size_t pixels = static_cast<size_t>(width) * height;
    rgb.resize(pixels * 3);

    for (size_t i = 0; i < pixels; i++) {
        rgb[i * 3 + 0] = rgba[i * 4 + 0];  // R
        rgb[i * 3 + 1] = rgba[i * 4 + 1];  // G
        rgb[i * 3 + 2] = rgba[i * 4 + 2];  // B
        // Alpha channel discarded
    }
}

void AIStyleTransfer::rgbToRgba(const std::vector<float>& rgb, std::vector<float>& rgba, int width, int height) {
    size_t pixels = static_cast<size_t>(width) * height;
    rgba.resize(pixels * 4);

    for (size_t i = 0; i < pixels; i++) {
        rgba[i * 4 + 0] = rgb[i * 3 + 0];  // R
        rgba[i * 4 + 1] = rgb[i * 3 + 1];  // G
        rgba[i * 4 + 2] = rgb[i * 3 + 2];  // B
        rgba[i * 4 + 3] = 1.0f;            // A (full opacity)
    }
}


void AIStyleTransfer::normalizeInput(std::vector<float>& data, int width, int height) {
    // ImageNet normalization (standard for PyTorch models)
    // Input is in [0, 1], normalize to ImageNet stats
    const float mean[3] = {0.485f, 0.456f, 0.406f};
    const float std[3] = {0.229f, 0.224f, 0.225f};

    size_t pixels = static_cast<size_t>(width) * height;
    for (size_t i = 0; i < pixels; ++i) {
        data[i * 3 + 0] = (data[i * 3 + 0] - mean[0]) / std[0];  // R
        data[i * 3 + 1] = (data[i * 3 + 1] - mean[1]) / std[1];  // G
        data[i * 3 + 2] = (data[i * 3 + 2] - mean[2]) / std[2];  // B
    }
}

void AIStyleTransfer::denormalizeOutput(std::vector<float>& data, int width, int height) {
    // Output is typically in [0, 255] range for style transfer models
    // Convert to [0, 1] for OpenGL
    for (auto& val : data) {
        val = val / 255.0f;
        val = std::clamp(val, 0.0f, 1.0f);
    }
}

void AIStyleTransfer::hwcToChw(const std::vector<float>& hwc, std::vector<float>& chw, int width, int height) {
    int pixels = width * height;
    chw.resize(pixels * 3);
    for (int i = 0; i < pixels; ++i) {
        chw[i] = hwc[i * 3 + 0];
        chw[pixels + i] = hwc[i * 3 + 1];
        chw[pixels * 2 + i] = hwc[i * 3 + 2];
    }
}

void AIStyleTransfer::chwToHwc(const std::vector<float>& chw, std::vector<float>& hwc, int width, int height) {
    int pixels = width * height;
    hwc.resize(pixels * 3);
    for (int i = 0; i < pixels; ++i) {
        hwc[i * 3 + 0] = chw[i];
        hwc[i * 3 + 1] = chw[pixels + i];
        hwc[i * 3 + 2] = chw[pixels * 2 + i];
    }
}
