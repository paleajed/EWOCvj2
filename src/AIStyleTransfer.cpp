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

#include "program.h"


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

    // Wait for any pending model load to complete
    if (modelLoadThread && modelLoadThread->joinable()) {
        modelLoadThread->join();
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
        // Priority: TensorRT (NVIDIA RTX) -> DirectML (any GPU) -> CUDA (Linux) -> CPU
        if (useGPU) {
            bool gpuEnabled = false;

            #ifdef _WIN32
            // Windows: Try TensorRT RTX first (fastest for NVIDIA RTX GPUs)
            try {
                ortSessionOptions->AppendExecutionProvider("NvTensorRtRtx");
                std::cerr << "[AIStyleTransfer] Using TensorRT RTX GPU acceleration" << std::endl;
                gpuEnabled = true;
            } catch (const Ort::Exception& e) {
                std::cerr << "[AIStyleTransfer] TensorRT RTX unavailable: " << e.what() << std::endl;
            }

            // Fallback: DirectML (works with NVIDIA, AMD, Intel via DirectX 12)
            if (!gpuEnabled) {
                try {
                    ortSessionOptions->AppendExecutionProvider("DML");
                    std::cerr << "[AIStyleTransfer] Using DirectML GPU acceleration" << std::endl;
                    gpuEnabled = true;
                } catch (const Ort::Exception& e) {
                    std::cerr << "[AIStyleTransfer] DirectML unavailable: " << e.what() << std::endl;
                }
            }
            #else
            // Linux: Use CUDA for NVIDIA GPUs
            try {
                ortSessionOptions->AppendExecutionProvider("CUDA");
                std::cerr << "[AIStyleTransfer] Using CUDA GPU acceleration" << std::endl;
                gpuEnabled = true;
            } catch (const Ort::Exception& e) {
                std::cerr << "[AIStyleTransfer] CUDA unavailable: " << e.what() << std::endl;
            }
            #endif

            if (!gpuEnabled) {
                std::cerr << "[AIStyleTransfer] No GPU acceleration available, using CPU" << std::endl;
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

                mainprogram->aistylepaths.push_back(model.path.string());

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

void AIStyleTransfer::setStyleAsync(int styleIndex) {
    if (!initialized) {
        setError("Not initialized");
        return;
    }

    if (styleIndex < 0 || styleIndex >= static_cast<int>(styleModels.size())) {
        setError("Invalid style index");
        return;
    }

    // If already loading, ignore (or could queue)
    if (modelLoading.load()) {
        std::cerr << "[AIStyleTransfer] Model load already in progress, ignoring request" << std::endl;
        return;
    }

    // Wait for previous load thread to finish if exists
    if (modelLoadThread && modelLoadThread->joinable()) {
        modelLoadThread->join();
    }

    pendingStyleIndex.store(styleIndex);
    modelLoading.store(true);

    std::cerr << "[AIStyleTransfer] Starting async load for style: " << styleModels[styleIndex].name << std::endl;

    // Launch loading thread
    modelLoadThread = std::make_unique<std::thread>([this, styleIndex]() {
        // Wait for any in-progress processing
        while (processing.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (loadModel(styleModels[styleIndex].path)) {
            currentStyleIndex = styleIndex;
            styleModels[styleIndex].loaded = true;
            std::cerr << "[AIStyleTransfer] Async load complete: " << styleModels[styleIndex].name << std::endl;
        } else {
            std::cerr << "[AIStyleTransfer] Async load failed: " << styleModels[styleIndex].name << std::endl;
        }

        modelLoading.store(false);
        pendingStyleIndex.store(-1);
    });
}

std::vector<std::string> AIStyleTransfer::getAvailableStyles() const {
    std::vector<std::string> names;
    names.reserve(styleModels.size());
    for (const auto& model : styleModels) {
        names.push_back(model.name);
    }
    return names;
}

bool AIStyleTransfer::render(const FBOstruct& input, FBOstruct& output) {
    if (!initialized) {
        setError("Not initialized");
        return false;
    }

    // Save current GL pixel store state (might be polluted by other code)
    GLint savedPackAlignment, savedPackRowLength, savedPackSkipPixels, savedPackSkipRows;
    GLint savedUnpackAlignment, savedUnpackRowLength, savedUnpackSkipPixels, savedUnpackSkipRows;
    glGetIntegerv(GL_PACK_ALIGNMENT, &savedPackAlignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &savedPackRowLength);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &savedPackSkipPixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &savedPackSkipRows);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &savedUnpackAlignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &savedUnpackRowLength);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &savedUnpackSkipPixels);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &savedUnpackSkipRows);

    // Reset to OpenGL defaults to avoid stride issues from other code
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    // Passthrough mode - just copy input to output
    // BUT: if we have a valid AI frame, show that instead of passthrough during model loading
    bool needsPassthrough = passthrough || currentStyleIndex < 0 || !ortSession;

    if (needsPassthrough) {
        // Check if we have a valid AI frame to show instead of passthrough
        // This prevents flashing during model loading or temporary session unavailability
        if (lastOutputFrame >= 0 && scaledOutputFBO.framebuffer != 0 && letterboxW > 0 && letterboxH > 0) {
            // Show last valid AI frame - extract from letterbox region
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, scaledOutputFBO.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
            glBlitFramebuffer(letterboxX, letterboxY, letterboxX + letterboxW, letterboxY + letterboxH,
                             0, 0, output.width, output.height,
                             GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            // No valid AI frame yet - true passthrough
            glBindFramebuffer(GL_READ_FRAMEBUFFER, input.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
            glBlitFramebuffer(0, 0, input.width, input.height,
                             0, 0, output.width, output.height,
                             GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Restore pixel store state before returning
        glPixelStorei(GL_PACK_ALIGNMENT, savedPackAlignment);
        glPixelStorei(GL_PACK_ROW_LENGTH, savedPackRowLength);
        glPixelStorei(GL_PACK_SKIP_PIXELS, savedPackSkipPixels);
        glPixelStorei(GL_PACK_SKIP_ROWS, savedPackSkipRows);
        glPixelStorei(GL_UNPACK_ALIGNMENT, savedUnpackAlignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, savedUnpackRowLength);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, savedUnpackSkipPixels);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, savedUnpackSkipRows);

        return true;
    }

    // Determine processing resolution
    // IMPORTANT: Width must be multiple of 4 for RGB alignment (width×3 must be 4-byte aligned)
    int maxDimension = 256;
    int procWidth, procHeight;

    if (processingWidth > 0 && processingHeight > 0) {
        // Model has fixed dimensions (set by loadModel) - use them
        // This ensures square-trained models process square input
        procWidth = (processingWidth / 4) * 4;  // Round down to multiple of 4
        procHeight = processingHeight;
    } else {
        // Model has dynamic dimensions - force SQUARE processing at maxDimension
        // Style transfer models are typically trained on square images, so
        // processing non-square input causes distorted style patterns.
        // The input will be scaled to square, processed, then scaled back.
        procWidth = (maxDimension / 4) * 4;  // Round to multiple of 4
        procHeight = maxDimension;
    }

    // Setup persistent mapped PBOs for async transfers
    setupPBOs(procWidth, procHeight);

    // Determine if we need to scale the input
    bool needsScaling = (procWidth != input.width || procHeight != input.height);
    FBOstruct inputToUse = input;

    if (needsScaling) {
        if (scaledInputFBO.width != procWidth || scaledInputFBO.height != procHeight) {
            deleteFBO(scaledInputFBO);
            if (!createTempFBO(scaledInputFBO, procWidth, procHeight)) {
                setError("Failed to create scaled input FBO");
                // Restore pixel store state before returning
                glPixelStorei(GL_PACK_ALIGNMENT, savedPackAlignment);
                glPixelStorei(GL_PACK_ROW_LENGTH, savedPackRowLength);
                glPixelStorei(GL_PACK_SKIP_PIXELS, savedPackSkipPixels);
                glPixelStorei(GL_PACK_SKIP_ROWS, savedPackSkipRows);
                glPixelStorei(GL_UNPACK_ALIGNMENT, savedUnpackAlignment);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, savedUnpackRowLength);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, savedUnpackSkipPixels);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, savedUnpackSkipRows);
                return false;
            }
        }

        // Calculate letterbox region to maintain aspect ratio
        // This prevents style distortion on non-square inputs
        float inputAspect = (float)input.width / (float)input.height;
        float procAspect = (float)procWidth / (float)procHeight;

        if (inputAspect > procAspect) {
            // Input is wider - pillarbox (black bars top/bottom)
            letterboxW = procWidth;
            letterboxH = (int)(procWidth / inputAspect);
            letterboxX = 0;
            letterboxY = (procHeight - letterboxH) / 2;
        } else {
            // Input is taller - letterbox (black bars left/right)
            letterboxH = procHeight;
            letterboxW = (int)(procHeight * inputAspect);
            letterboxX = (procWidth - letterboxW) / 2;
            letterboxY = 0;
        }

        // Clear to black first (for letterbox/pillarbox bars)
        glBindFramebuffer(GL_FRAMEBUFFER, scaledInputFBO.framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Blit input to letterbox region maintaining aspect ratio
        glBindFramebuffer(GL_READ_FRAMEBUFFER, input.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scaledInputFBO.framebuffer);
        glBlitFramebuffer(0, 0, input.width, input.height,
                         letterboxX, letterboxY, letterboxX + letterboxW, letterboxY + letterboxH,
                         GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        inputToUse = scaledInputFBO;
    } else {
        // No scaling needed - content fills entire processing area
        letterboxX = 0;
        letterboxY = 0;
        letterboxW = procWidth;
        letterboxH = procHeight;
    }

    // ========================================================================
    // TRIPLE-BUFFERED ASYNC PIPELINE
    // ========================================================================

    int currentIdx = currentFrame % 3;
    int prevIdx = (currentFrame - 1 + 3) % 3;
    int readyIdx = (currentFrame - 2 + 3) % 3;

    // Step 1: Start async download for current frame
    startAsyncDownload(inputToUse.framebuffer, procWidth, procHeight, currentIdx);

    // Step 2: Check if frame from 2 frames ago has finished processing
    if (frameReady[readyIdx].load()) {
        // Create scaledOutputFBO if needed (for processed resolution)
        if (scaledOutputFBO.width != procWidth || scaledOutputFBO.height != procHeight) {
            deleteFBO(scaledOutputFBO);
            if (!createTempFBO(scaledOutputFBO, procWidth, procHeight)) {
                setError("Failed to create scaled output FBO");
                // Restore pixel store state before returning
                glPixelStorei(GL_PACK_ALIGNMENT, savedPackAlignment);
                glPixelStorei(GL_PACK_ROW_LENGTH, savedPackRowLength);
                glPixelStorei(GL_PACK_SKIP_PIXELS, savedPackSkipPixels);
                glPixelStorei(GL_PACK_SKIP_ROWS, savedPackSkipRows);
                glPixelStorei(GL_UNPACK_ALIGNMENT, savedUnpackAlignment);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, savedUnpackRowLength);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, savedUnpackSkipPixels);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, savedUnpackSkipRows);
                return false;
            }
        }

        // Upload AI result to scaledOutputFBO using async PBO
        startAsyncUpload(outputBuffers[readyIdx].data(), procWidth, procHeight, readyIdx);
        finishAsyncUpload(scaledOutputFBO.texture, procWidth, procHeight, readyIdx);

        // Wait for upload to complete before marking as ready
        // This ensures the texture is fully written before we blit from it
        WaitBuffer(uploadFences[readyIdx]);

        frameReady[readyIdx].store(false);
        lastOutputFrame = readyIdx;
    }

    // Always output: show last completed AI frame, or passthrough if no frames ready yet
    if (lastOutputFrame >= 0) {
        // Ensure no PBOs are bound (they can interfere with blit)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // Blit from letterbox region in scaledOutputFBO to output
        // This extracts only the styled content, not the black bars
        glBindFramebuffer(GL_READ_FRAMEBUFFER, scaledOutputFBO.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
        glBlitFramebuffer(letterboxX, letterboxY, letterboxX + letterboxW, letterboxY + letterboxH,
                         0, 0, output.width, output.height,
                         GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    } else {
        // No AI frames ready yet (first few frames) - passthrough input
        glBindFramebuffer(GL_READ_FRAMEBUFFER, input.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output.framebuffer);
        glBlitFramebuffer(0, 0, input.width, input.height,
                         0, 0, output.width, output.height,
                         GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Step 3: Finish async download from previous frame and queue for processing
    // IMPORTANT: Only process if buffer is not still being used by worker thread
    // This prevents overwriting inputBuffers while worker is reading them
    if (currentFrame >= 1 && !bufferInUse[prevIdx].load()) {
        if (finishAsyncDownload(prevIdx, procWidth, procHeight)) {
            // Mark buffer as in use BEFORE queuing
            bufferInUse[prevIdx].store(true);

            // Queue job for worker thread
            ProcessingJob job;
            job.frameIndex = prevIdx;
            job.width = procWidth;
            job.height = procHeight;
            job.frameReady = &frameReady[prevIdx];

            std::lock_guard<std::mutex> lock(queueMutex);
            jobQueue.push(job);
            queueCV.notify_one();
        }
    }

    currentFrame++;

    // Restore original GL pixel store state
    glPixelStorei(GL_PACK_ALIGNMENT, savedPackAlignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, savedPackRowLength);
    glPixelStorei(GL_PACK_SKIP_PIXELS, savedPackSkipPixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, savedPackSkipRows);
    glPixelStorei(GL_UNPACK_ALIGNMENT, savedUnpackAlignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, savedUnpackRowLength);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, savedUnpackSkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, savedUnpackSkipRows);

    return true;
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

                // Check if model requires specific dimensions (not dynamic)
                if (modelHeight > 0 && modelWidth > 0) {
                    // Model has fixed input dimensions - auto-set processing resolution
                    processingHeight = modelHeight;
                    processingWidth = modelWidth;
                    std::cerr << "[AIStyleTransfer] Model requires fixed input: "
                              << modelWidth << "x" << modelHeight << std::endl;
                    std::cerr << "[AIStyleTransfer] Will use Lanczos downscaling to match" << std::endl;
                } else {
                    // Model accepts dynamic dimensions
                    std::cerr << "[AIStyleTransfer] Model accepts dynamic input dimensions" << std::endl;
                }
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

        // Create input tensor shape [1, 3, height, width] (NCHW format, 3 channels for RGB)
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
        std::string errorMsg = e.what();
        setError(std::string("Inference failed: ") + errorMsg);
        std::cerr << "[AIStyleTransfer] " << lastError << std::endl;

        // Check for out of memory errors
        if (errorMsg.find("memory") != std::string::npos ||
            errorMsg.find("Memory") != std::string::npos ||
            errorMsg.find("OOM") != std::string::npos ||
            errorMsg.find("out of") != std::string::npos ||
            errorMsg.find("allocation") != std::string::npos) {
            outOfMemoryError.store(true);
        }
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

            // Process inference on worker thread
            int frameIdx = job.frameIndex;
            int w = job.width;
            int h = job.height;

            auto startTime = std::chrono::high_resolution_clock::now();

            // Normalize input if model needs it ([-1, 1] for ReCoNet-style models)
            // Main thread already did format conversion (RGBA uint8 → RGB float)
            if (currentStyleIndex >= 0 && currentStyleIndex < static_cast<int>(styleModels.size())) {
                if (styleModels[currentStyleIndex].needsInputNormalization) {
                    normalizeInput(inputBuffers[frameIdx], w, h);
                }
            }

            // Convert HWC → CHW for ONNX model (3 channels: RGB)
            size_t expectedInputSize = static_cast<size_t>(w) * h * 3;
            std::vector<float> inputCHW;
            hwcToChw(inputBuffers[frameIdx], inputCHW, w, h);

            // Run inference
            std::vector<float> outputCHW(expectedInputSize);
            bool success = inferenceInternal(inputCHW.data(), outputCHW.data(), w, h);

            if (success) {
                // Convert CHW → HWC (3 channels: RGB)
                chwToHwc(outputCHW, outputBuffers[frameIdx], w, h);

                // Auto-detect output range and denormalize accordingly
                // Sample first pixel to determine model output range
                if (outputBuffers[frameIdx].size() >= 3) {
                    float maxVal = std::max({outputBuffers[frameIdx][0],
                                            outputBuffers[frameIdx][1],
                                            outputBuffers[frameIdx][2]});
                    float minVal = std::min({outputBuffers[frameIdx][0],
                                            outputBuffers[frameIdx][1],
                                            outputBuffers[frameIdx][2]});

                    if (maxVal > 10.0f || minVal < -10.0f) {
                        // Model outputs [0, 255] range - divide by 255
                        // These models expect [0, 1] input (no normalization)
                        for (auto& val : outputBuffers[frameIdx]) {
                            val = std::clamp(val / 255.0f, 0.0f, 1.0f);
                        }
                        if (currentStyleIndex >= 0 && currentStyleIndex < static_cast<int>(styleModels.size())) {
                            styleModels[currentStyleIndex].needsInputNormalization = false;
                        }
                    } else if (minVal < -0.1f) {
                        // Model outputs [-1, 1] range (ReCoNet style)
                        // These models expect [-1, 1] input (needs normalization)
                        for (auto& val : outputBuffers[frameIdx]) {
                            val = std::clamp((val + 1.0f) / 2.0f, 0.0f, 1.0f);
                        }
                        if (currentStyleIndex >= 0 && currentStyleIndex < static_cast<int>(styleModels.size())) {
                            styleModels[currentStyleIndex].needsInputNormalization = true;
                        }
                    }
                    // else: already in [0, 1] range, no conversion needed
                }

                // Output stays in RGB float - main thread will do format conversion
                auto endTime = std::chrono::high_resolution_clock::now();
                float inferenceTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

                // Print timing occasionally
                static int printCounter = 0;
                if (++printCounter % 60 == 0) {
                    std::cerr << "[AIStyleTransfer] Worker inference: " << inferenceTime << "ms" << std::endl;
                }

                // Signal ready for upload and release buffer
                if (job.frameReady) {
                    job.frameReady->store(true);
                }
                bufferInUse[frameIdx].store(false);
            } else {
                // Passthrough input to output as fallback (copy RGB float data)
                outputBuffers[frameIdx] = inputBuffers[frameIdx];
                if (job.frameReady) {
                    job.frameReady->store(true);  // Still mark as ready to show something
                }
                bufferInUse[frameIdx].store(false);
            }

            processing.store(false);
        }
    }
}

bool AIStyleTransfer::setupPBOs(int width, int height) {
    int newPBOSize = width * height * 4;  // RGBA bytes (unsigned char)

    if (downloadPBOs[0] == 0 || pboSize != newPBOSize) {
        cleanupPBOs();

        pboSize = newPBOSize;

        // Create download PBOs with persistent mapping (texture → CPU, GPU writes)
        glGenBuffers(3, downloadPBOs);
        for (int i = 0; i < 3; i++) {
            GLbitfield flags = GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            glBindBuffer(GL_PIXEL_PACK_BUFFER, downloadPBOs[i]);
            glBufferStorage(GL_PIXEL_PACK_BUFFER, pboSize, nullptr, flags);
            downloadMapPtr[i] = (unsigned char*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pboSize, flags);

            if (!downloadMapPtr[i]) {
                return false;
            }
        }

        // Create upload PBOs with persistent mapping (CPU → texture, CPU writes)
        glGenBuffers(3, uploadPBOs);
        for (int i = 0; i < 3; i++) {
            GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uploadPBOs[i]);
            glBufferStorage(GL_PIXEL_UNPACK_BUFFER, pboSize, nullptr, flags);
            uploadMapPtr[i] = (unsigned char*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, pboSize, flags);

            if (!uploadMapPtr[i]) {
                return false;
            }
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    return true;
}

void AIStyleTransfer::cleanupPBOs() {
    // Unmap and delete download PBOs
    if (downloadPBOs[0] != 0) {
        for (int i = 0; i < 3; i++) {
            if (downloadMapPtr[i]) {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, downloadPBOs[i]);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                downloadMapPtr[i] = nullptr;
            }
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glDeleteBuffers(3, downloadPBOs);
        downloadPBOs[0] = downloadPBOs[1] = downloadPBOs[2] = 0;
    }

    // Unmap and delete upload PBOs
    if (uploadPBOs[0] != 0) {
        for (int i = 0; i < 3; i++) {
            if (uploadMapPtr[i]) {
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uploadPBOs[i]);
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                uploadMapPtr[i] = nullptr;
            }
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glDeleteBuffers(3, uploadPBOs);
        uploadPBOs[0] = uploadPBOs[1] = uploadPBOs[2] = 0;
    }

    // Delete sync fences
    for (int i = 0; i < 3; i++) {
        if (downloadFences[i]) {
            glDeleteSync(downloadFences[i]);
            downloadFences[i] = nullptr;
        }
        if (uploadFences[i]) {
            glDeleteSync(uploadFences[i]);
            uploadFences[i] = nullptr;
        }
    }

    pboSize = 0;
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

        // Generate texture (use GL_RGBA8 to match PBO upload format)
        glGenTextures(1, &fbo.texture);
        glBindTexture(GL_TEXTURE_2D, fbo.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.texture, 0);

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
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
    return false;
}


bool AIStyleTransfer::downloadTextureToBuffer(GLuint fbo, GLuint texture, int width, int height, std::vector<float>& buffer) {
    // Clear any previous errors
    while (glGetError() != GL_NO_ERROR);

    // Save current bindings and pixel store state
    GLint currentReadFBO;
    GLint currentPBO;
    GLint currentPackAlignment, currentPackRowLength, currentPackSkipPixels, currentPackSkipRows;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentReadFBO);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &currentPBO);
    glGetIntegerv(GL_PACK_ALIGNMENT, &currentPackAlignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &currentPackRowLength);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &currentPackSkipPixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &currentPackSkipRows);

    // Set pixel store for tightly packed RGBA data
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    // Unbind any pixel pack buffer
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Allocate buffer for RGBA byte data
    buffer.resize(static_cast<size_t>(width) * height * 3);
    std::vector<unsigned char> rgbaBuffer(static_cast<size_t>(width) * height * 4);

    // Use the provided FBO directly
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, currentPBO);
        glPixelStorei(GL_PACK_ALIGNMENT, currentPackAlignment);
        glPixelStorei(GL_PACK_ROW_LENGTH, currentPackRowLength);
        glPixelStorei(GL_PACK_SKIP_PIXELS, currentPackSkipPixels);
        glPixelStorei(GL_PACK_SKIP_ROWS, currentPackSkipRows);
        return false;
    }

    // Read pixels as UNSIGNED_BYTE (matches GL_RGBA8)
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());

    // Restore bindings and pixel store state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, currentPBO);
    glPixelStorei(GL_PACK_ALIGNMENT, currentPackAlignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, currentPackRowLength);
    glPixelStorei(GL_PACK_SKIP_PIXELS, currentPackSkipPixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, currentPackSkipRows);

    // Convert RGBA unsigned byte to RGB float [0,1]
    for (size_t i = 0; i < width * height; ++i) {
        buffer[i * 3 + 0] = rgbaBuffer[i * 4 + 0] / 255.0f;
        buffer[i * 3 + 1] = rgbaBuffer[i * 4 + 1] / 255.0f;
        buffer[i * 3 + 2] = rgbaBuffer[i * 4 + 2] / 255.0f;
    }

    return true;
}

bool AIStyleTransfer::uploadBufferToTexture(const std::vector<float>& buffer, int width, int height, GLuint texture) {
    // Save current pixel store state
    GLint currentUnpackAlignment, currentUnpackRowLength, currentUnpackSkipPixels, currentUnpackSkipRows;
    GLint currentUnpackBuffer;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &currentUnpackAlignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &currentUnpackRowLength);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &currentUnpackSkipPixels);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &currentUnpackSkipRows);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentUnpackBuffer);

    // Set pixel store for tightly packed RGBA data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Convert RGB float to RGBA unsigned byte
    std::vector<unsigned char> rgbaBuffer(static_cast<size_t>(width) * height * 4);

    for (size_t i = 0; i < width * height; ++i) {
        rgbaBuffer[i * 4 + 0] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 0], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 1] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 1], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 2] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 2], 0.0f, 1.0f) * 255.0f);
        rgbaBuffer[i * 4 + 3] = 255;
    }

    // Upload to texture (using glTexSubImage2D to avoid reallocating)
    glBindTexture(GL_TEXTURE_2D, texture);

    // Check if texture needs to be allocated first
    GLint texWidth = 0, texHeight = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

    if (texWidth != width || texHeight != height) {
        // Texture not allocated or wrong size - allocate it
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());
    } else {
        // Texture already allocated - just update contents
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());
    }

    // Restore pixel store state
    glPixelStorei(GL_UNPACK_ALIGNMENT, currentUnpackAlignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, currentUnpackRowLength);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, currentUnpackSkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, currentUnpackSkipRows);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentUnpackBuffer);

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
    // ReCoNet models expect [-1, 1] range
    // Input is in [0, 1], convert to [-1, 1]
    for (auto& val : data) {
        val = val * 2.0f - 1.0f;  // [0, 1] -> [-1, 1]
    }
}

void AIStyleTransfer::denormalizeOutput(std::vector<float>& data, int width, int height) {
    // ReCoNet models output [-1, 1] range (Tanh activation)
    // Convert to [0, 1] for OpenGL
    for (auto& val : data) {
        val = (val + 1.0f) / 2.0f;  // [-1, 1] -> [0, 1]
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

void AIStyleTransfer::hwcToChw4(const std::vector<float>& hwc, std::vector<float>& chw, int width, int height) {
    int pixels = width * height;
    chw.resize(pixels * 4);
    for (int i = 0; i < pixels; ++i) {
        chw[i] = hwc[i * 4 + 0];                    // R
        chw[pixels + i] = hwc[i * 4 + 1];           // G
        chw[pixels * 2 + i] = hwc[i * 4 + 2];       // B
        chw[pixels * 3 + i] = hwc[i * 4 + 3];       // A
    }
}

void AIStyleTransfer::chw4ToHwc(const std::vector<float>& chw, std::vector<float>& hwc, int width, int height) {
    int pixels = width * height;
    hwc.resize(pixels * 4);
    for (int i = 0; i < pixels; ++i) {
        hwc[i * 4 + 0] = chw[i];                    // R
        hwc[i * 4 + 1] = chw[pixels + i];           // G
        hwc[i * 4 + 2] = chw[pixels * 2 + i];       // B
        hwc[i * 4 + 3] = chw[pixels * 3 + i];       // A
    }
}

// ============================================================================
// Async PBO Transfer Methods (Persistent Mapped Buffers)
// ============================================================================
//
// Uses the same pattern as mixer.cpp for zero-copy async transfers:
//
// 1. Persistent Mapped Buffers (created once in setupPBOs):
//    - glBufferStorage() with GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
//    - glMapBufferRange() once at creation - stays mapped forever
//    - No glMapBuffer/glUnmapBuffer overhead per frame!
//
// 2. Fence Sync Pattern:
//    - LockBuffer() = glFenceSync() to mark GPU command completion point
//    - WaitBuffer() = glClientWaitSync() to wait for fence (non-blocking with triple-buffer)
//
// 3. Zero-Copy Transfers:
//    - GPU writes directly to downloadMapPtr[] (glReadPixels → PBO → mapped memory)
//    - Main thread reads from downloadMapPtr[], does format conversion (OpenGL context)
//    - Worker thread does inference (~140ms) on RGB float data
//    - Main thread writes results to uploadMapPtr[] (OpenGL context)
//    - GPU reads directly from uploadMapPtr[] (glTexSubImage2D from PBO)
//
// 4. Main Thread Work (still fast):
//    - finishAsyncDownload: Wait fence + format conversion RGBA→RGB (~1-2ms)
//    - startAsyncUpload: Format conversion RGB→RGBA (~1-2ms)
//    - Total main thread overhead: ~2-4ms (vs ~150ms with blocking glMapBuffer)
//
// Result: Truly async - no blocking glMapBuffer(), format conversions on main thread
//         (OpenGL context requirement), inference on worker thread!
//
// ============================================================================

bool AIStyleTransfer::startAsyncDownload(GLuint fbo, int width, int height, int frameIndex) {
    // Initiate async download from texture to PBO (persistent mapped buffer)
    // GPU writes directly to mapped buffer - zero copy!

    // Ensure no row padding (critical for correct pixel layout) - SET BEFORE BINDING
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);  // 0 = use width (no custom stride)
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Bind PBO for this frame
    glBindBuffer(GL_PIXEL_PACK_BUFFER, downloadPBOs[frameIndex]);

    // Start async transfer (NULL pointer = write to PBO, not CPU memory)
    // GPU writes to persistent mapped buffer in background
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Insert fence to signal when download completes (using mixer.cpp pattern)
    LockBuffer(downloadFences[frameIndex]);

    // Unbind
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return true;
}

bool AIStyleTransfer::finishAsyncDownload(int frameIndex, int width, int height) {
    // Wait for GPU to finish writing to PBO (non-blocking if triple-buffer working correctly)
    WaitBuffer(downloadFences[frameIndex]);

    // Convert RGBA → RGB for model input (models expect 3 channels)
    // Direct read from persistent mapped buffer - NO glMapBuffer blocking!
    size_t pixelCount = static_cast<size_t>(width) * height;
    inputBuffers[frameIndex].resize(pixelCount * 3);  // RGB (3 channels)

    unsigned char* pboData = downloadMapPtr[frameIndex];
    if (!pboData) {
        return false;
    }

    // Convert RGBA unsigned byte → RGB float [0,1] (drop alpha channel)
    for (size_t i = 0; i < pixelCount; ++i) {
        inputBuffers[frameIndex][i * 3 + 0] = pboData[i * 4 + 0] / 255.0f;  // R
        inputBuffers[frameIndex][i * 3 + 1] = pboData[i * 4 + 1] / 255.0f;  // G
        inputBuffers[frameIndex][i * 3 + 2] = pboData[i * 4 + 2] / 255.0f;  // B
        // Alpha channel dropped - model expects RGB only
    }

    return true;
}

bool AIStyleTransfer::startAsyncUpload(const float* buffer, int width, int height, int frameIndex) {
    // Convert RGB → RGBA for texture upload (add opaque alpha)
    // Direct write to persistent mapped buffer - NO glMapBuffer blocking!

    size_t pixelCount = static_cast<size_t>(width) * height;

    unsigned char* pboData = uploadMapPtr[frameIndex];
    if (!pboData) {
        return false;
    }

    // Convert RGB float [0,1] → RGBA unsigned byte (add alpha=1.0)
    for (size_t i = 0; i < pixelCount; ++i) {
        pboData[i * 4 + 0] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 0], 0.0f, 1.0f) * 255.0f);  // R
        pboData[i * 4 + 1] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 1], 0.0f, 1.0f) * 255.0f);  // G
        pboData[i * 4 + 2] = static_cast<unsigned char>(std::clamp(buffer[i * 3 + 2], 0.0f, 1.0f) * 255.0f);  // B
        pboData[i * 4 + 3] = 255;  // Alpha = 1.0 (opaque)
    }

    return true;
}

bool AIStyleTransfer::finishAsyncUpload(GLuint texture, int width, int height, int frameIndex) {
    // Initiate async upload from PBO to texture (persistent mapped buffer)
    // GPU reads from persistent mapped buffer - zero copy!

    // Ensure no row padding (critical for correct pixel layout) - SET BEFORE BINDING
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);  // 0 = use width (no custom stride)
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glBindTexture(GL_TEXTURE_2D, texture);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uploadPBOs[frameIndex]);

    // Clear any previous errors
    while (glGetError() != GL_NO_ERROR);

    // Upload from PBO to existing texture (texture already allocated in createTempFBO)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Insert fence to signal when upload completes (using mixer.cpp pattern)
    LockBuffer(uploadFences[frameIndex]);

    // Unbind
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}
