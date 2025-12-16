/**
 * ReCoNetTrainer.cpp
 *
 * Implementation of ReCoNet Style Transfer Trainer
 *
 * License: GPL3
 */
#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#include "ReCoNetTrainer.h"
#include "program.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#include <IL/il.h>
#include <IL/ilu.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// External global
extern SDL_GLContext glc;
extern class Program *mainprogram;

// ===========================================================================
// Constructor / Destructor
// ===========================================================================

ReCoNetTrainer::ReCoNetTrainer() {
    training.store(false);
    shouldStop.store(false);
    initialized = false;
}

ReCoNetTrainer::~ReCoNetTrainer() {
    // Stop training if active
    shouldStop.store(true);

    // Wait for training thread
    if (trainingThread && trainingThread->joinable()) {
        trainingThread->join();
    }

    // Cleanup OpenGL resources
    if (fullscreenQuadVAO != 0) {
        glDeleteVertexArrays(1, &fullscreenQuadVAO);
        fullscreenQuadVAO = 0;
    }
    if (fullscreenQuadVBO != 0) {
        glDeleteBuffers(1, &fullscreenQuadVBO);
        fullscreenQuadVBO = 0;
    }
    if (resizeShaderProgram != 0) {
        glDeleteProgram(resizeShaderProgram);
        resizeShaderProgram = 0;
    }

    if (trainingContext) {
        SDL_GL_DeleteContext(trainingContext);
    }
    if (trainingWindow) {
        SDL_DestroyWindow(trainingWindow);
    }

#ifdef _WIN32
    // Close process handles
    if (processHandle) CloseHandle((HANDLE)processHandle);
    if (stdoutReadHandle) CloseHandle((HANDLE)stdoutReadHandle);
    if (stderrReadHandle) CloseHandle((HANDLE)stderrReadHandle);
#endif
}

// ===========================================================================
// Initialization
// ===========================================================================

bool ReCoNetTrainer::initialize() {
    if (initialized) {
        return true;
    }

    std::cerr << "[ReCoNetTrainer] Initializing..." << std::endl;

    // Check for user-specified Python path via environment variable
    const char* envPython = getenv("EWOCVJ2_PYTHON");

    // Find Python executable
#ifdef _WIN32
    // Try common Python locations
    std::vector<std::string> pythonPaths;
    if (envPython && strlen(envPython) > 0) {
        pythonPaths.push_back(envPython);
        std::cerr << "[ReCoNetTrainer] Using Python from EWOCVJ2_PYTHON: " << envPython << std::endl;
    }
    pythonPaths.insert(pythonPaths.end(), {
        "python",
        "python3",
        "C:\\Python313\\python.exe",
        "C:\\Python312\\python.exe",
        "C:\\Python311\\python.exe",
        "C:\\Python310\\python.exe",
        "C:\\Users\\" + std::string(getenv("USERNAME")) + "\\AppData\\Local\\Programs\\Python\\Python313\\python.exe",
        "C:\\Users\\" + std::string(getenv("USERNAME")) + "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe",
        "C:\\Users\\" + std::string(getenv("USERNAME")) + "\\AppData\\Local\\Programs\\Python\\Python311\\python.exe"
    });
#else
    std::vector<std::string> pythonPaths;
    if (envPython && strlen(envPython) > 0) {
        pythonPaths.push_back(envPython);
    }
    pythonPaths.insert(pythonPaths.end(), {
        "python3",
        "python",
        "/usr/bin/python3",
        "/usr/local/bin/python3"
    });
#endif

    bool foundPython = false;
    for (const auto& path : pythonPaths) {
#ifdef _WIN32
        // Try to run python --version using _popen (no console window)
        std::string cmd = path + " --version 2>&1";
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (pipe) {
            int exitCode = _pclose(pipe);
            if (exitCode == 0) {
                pythonExecutable = path;
                foundPython = true;
                std::cerr << "[ReCoNetTrainer] Found Python: " << path << std::endl;
                break;
            }
        }
#else
        std::string cmd = "\"" + path + "\" --version >/dev/null 2>&1";
        if (system(cmd.c_str()) == 0) {
            pythonExecutable = path;
            foundPython = true;
            std::cerr << "[ReCoNetTrainer] Found Python: " << path << std::endl;
            break;
        }
#endif
    }

    if (!foundPython) {
        setError("Python 3 not found. Please install Python 3.8+ from python.org");
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }

    // Check for PyTorch by capturing output (using _popen to avoid console window)
#ifdef _WIN32
    // Build command with proper escaping for _popen
    // Use double quotes around the whole command and escape inner quotes
    std::string checkCmd = pythonExecutable + " -c \"import torch; print(torch.__version__)\" 2>&1";

    FILE* pipe = _popen(checkCmd.c_str(), "r");
    bool torchFound = false;

    if (pipe) {
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        int exitCode = _pclose(pipe);

        std::cerr << "[ReCoNetTrainer] PyTorch check output: " << result;

        // Check if output looks like a version number
        if (result.find('.') != std::string::npos &&
            result.find("ModuleNotFoundError") == std::string::npos &&
            result.find("ImportError") == std::string::npos &&
            exitCode == 0) {
            torchFound = true;
        }
    }

    if (!torchFound) {
        setError("PyTorch not found. Install with: pip install torch torchvision onnx");
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }
#else
    std::string checkCmd = "\"" + pythonExecutable + "\" -c \"import torch; print(torch.__version__)\"";
    if (system(checkCmd.c_str()) != 0) {
        setError("PyTorch not found. Install with: pip install torch torchvision onnx");
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }
#endif

    std::cerr << "[ReCoNetTrainer] PyTorch detected" << std::endl;

    // Setup directories
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) {
        std::string basePath = std::string(path) + "\\EWOCvj2";
        modelsDir = basePath + "\\models\\styles";
        scriptsDir = basePath + "\\scripts";
    } else {
        modelsDir = "C:\\ProgramData\\EWOCvj2\\models\\styles";
        scriptsDir = "C:\\ProgramData\\EWOCvj2\\scripts";
    }

    // Temp directory
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    tempDir = std::string(tempPath) + "EWOCvj2\\reconet_training";
#else
    modelsDir = "/usr/local/share/EWOCvj2/models/styles";
    scriptsDir = "/usr/local/share/EWOCvj2/scripts";
    tempDir = "/tmp/EWOCvj2_reconet_training";
#endif

    // Create directories
    try {
        std::filesystem::create_directories(modelsDir);
        std::filesystem::create_directories(scriptsDir);
        std::filesystem::create_directories(tempDir);
        std::filesystem::create_directories(tempDir + "/images");
        std::filesystem::create_directories(tempDir + "/checkpoints");
    } catch (const std::exception& e) {
        setError(std::string("Failed to create directories: ") + e.what());
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }

    std::cerr << "[ReCoNetTrainer] Models dir: " << modelsDir << std::endl;
    std::cerr << "[ReCoNetTrainer] Scripts dir: " << scriptsDir << std::endl;
    std::cerr << "[ReCoNetTrainer] Temp dir: " << tempDir << std::endl;

    // Deploy training scripts
    if (!deployTrainingScripts()) {
        setError("Failed to deploy training scripts");
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }

    initialized = true;
    std::cerr << "[ReCoNetTrainer] Initialization successful" << std::endl;
    return true;
}

// ===========================================================================
// Public API
// ===========================================================================

bool ReCoNetTrainer::startTraining(StylePreparationBin* bin,
                                    const std::string& modelName,
                                    const Config& config) {
    if (!initialized) {
        setError("Trainer not initialized. Call initialize() first.");
        return false;
    }

    if (training.load()) {
        setError("Training already in progress");
        return false;
    }

    if (!bin) {
        setError("Invalid StylePreparationBin");
        return false;
    }

    // Validate model name
    if (!validateModelName(modelName)) {
        setError("Invalid model name. Use only alphanumeric characters and underscores.");
        return false;
    }

    // Count valid images
    int numImages = countValidImages(bin);
    if (numImages < 1) {
        setError("No training images found. Load at least 1 image.");
        return false;
    }

    std::cerr << "[ReCoNetTrainer] Starting training: " << modelName << std::endl;
    std::cerr << "[ReCoNetTrainer] Quality: " << (int)config.quality << std::endl;
    std::cerr << "[ReCoNetTrainer] Resolution: " << config.getResolution() << "x" << config.getResolution() << std::endl;
    std::cerr << "[ReCoNetTrainer] Images: " << numImages << std::endl;

    // Estimate VRAM
    size_t vram = estimateVRAM(numImages, config.getResolution(), config.getResolution());
    std::cerr << "[ReCoNetTrainer] Estimated VRAM: " << formatVRAM(vram) << std::endl;

    // Clear error
    clearError();

    // Reset progress
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress = Progress();
        progress.totalIterations = config.getIterations();
        progress.status = "Starting...";
    }

    // Start training thread
    shouldStop.store(false);
    training.store(true);
    trainingThread = std::make_unique<std::thread>(
        &ReCoNetTrainer::trainingThreadFunc, this, bin, modelName, config
    );

    return true;
}

void ReCoNetTrainer::stopTraining() {
    if (!training.load()) {
        return;
    }

    std::cerr << "[ReCoNetTrainer] Stopping training..." << std::endl;
    shouldStop.store(true);

#ifdef _WIN32
    // Terminate Python process if running
    if (processHandle) {
        TerminateProcess((HANDLE)processHandle, 1);
    }
#endif
}

ReCoNetTrainer::Progress ReCoNetTrainer::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

// ===========================================================================
// VRAM Estimation
// ===========================================================================

size_t ReCoNetTrainer::estimateVRAM(int numImages, int width, int height) {
    // Per-image VRAM (RGBA8 texture)
    size_t imageVRAM = static_cast<size_t>(width) * height * 4;

    // Total for all training images
    size_t imagesTotal = imageVRAM * numImages;

    // Model parameters (~1.7M params * 4 bytes)
    size_t modelVRAM = 7 * 1024 * 1024;

    // Optimizer state (Adam: 2x model size for momentum and velocity)
    size_t optimizerVRAM = modelVRAM * 2;

    // Determine batch size
    int batchSize;
    if (width <= 256) batchSize = 4;
    else if (width <= 512) batchSize = 2;
    else batchSize = 1;

    // Activations during training (batch size * intermediate features * 8x)
    size_t activationsVRAM = imageVRAM * batchSize * 8;

    // Preprocessing overhead (source + scaled texture)
    size_t preprocessVRAM = imageVRAM * 2;

    // Total
    size_t totalVRAM = imagesTotal + modelVRAM + optimizerVRAM +
                       activationsVRAM + preprocessVRAM;

    // Add 20% safety margin
    return static_cast<size_t>(totalVRAM * 1.2);
}

std::string ReCoNetTrainer::formatVRAM(size_t bytes) {
    double gb = bytes / (1024.0 * 1024.0 * 1024.0);
    double mb = bytes / (1024.0 * 1024.0);

    char buffer[64];
    if (gb >= 1.0) {
        snprintf(buffer, sizeof(buffer), "%.1f GB", gb);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f MB", mb);
    }
    return std::string(buffer);
}

// ===========================================================================
// Training Thread
// ===========================================================================

void ReCoNetTrainer::trainingThreadFunc(StylePreparationBin* bin,
                                       std::string modelName,
                                       Config config) {
    std::cerr << "[ReCoNetTrainer] Training thread started" << std::endl;

    try {
        // Create OpenGL context for preprocessing
        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

        trainingWindow = SDL_CreateWindow("ReCoNet Training", 0, 0, 256, 256,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (!trainingWindow) {
            setError(std::string("Failed to create training window: ") + SDL_GetError());
            training.store(false);
            return;
        }

        trainingContext = SDL_GL_CreateContext(trainingWindow);
        if (!trainingContext) {
            setError(std::string("Failed to create OpenGL context: ") + SDL_GetError());
            SDL_DestroyWindow(trainingWindow);
            trainingWindow = nullptr;
            training.store(false);
            return;
        }

        SDL_GL_MakeCurrent(trainingWindow, trainingContext);
        std::cerr << "[ReCoNetTrainer] OpenGL context created" << std::endl;

        // Create fullscreen quad VAO for image preprocessing
        createFullscreenQuad();

        // Create simple resize shader for this thread
        if (!createResizeShader()) {
            setError("Failed to create resize shader");
            training.store(false);
            return;
        }

        // Preprocess images
        std::vector<std::string> imagePaths;
        updateProgress({0, config.getIterations(), 0, 0, 0, "Preprocessing images...", 0});

        if (!preprocessImages(bin, config, imagePaths)) {
            training.store(false);
            return;
        }

        if (shouldStop.load()) {
            std::cerr << "[ReCoNetTrainer] Training cancelled" << std::endl;
            training.store(false);
            return;
        }

        // Generate config JSON (don't use pathtoplatform - it adds quotes)
        std::string configPath = tempDir + "/config.json";
        std::string outputOnnxPath = tempDir + "/output.onnx";
        #ifdef _WIN32
        for (char& c : configPath) if (c == '/') c = '\\';
        for (char& c : outputOnnxPath) if (c == '/') c = '\\';
        #endif

        if (!generateConfigJSON(config, imagePaths, modelName, configPath)) {
            training.store(false);
            return;
        }

        // Launch Python training
        updateProgress({0, config.getIterations(), 0, 0, 0, "Starting Python training...", 0});

        if (!launchPythonTraining(configPath, outputOnnxPath)) {
            training.store(false);
            return;
        }

        // Monitor training progress
        monitorTrainingProgress();

        // Check if training completed successfully
        if (!shouldStop.load() && std::filesystem::exists(outputOnnxPath)) {
            // Copy ONNX model to models directory (don't use pathtoplatform - it adds quotes)
            std::string finalModelPath = modelsDir + "/" + modelName + ".onnx";
            #ifdef _WIN32
            for (char& c : finalModelPath) if (c == '/') c = '\\';
            #endif
            try {
                std::filesystem::copy_file(outputOnnxPath, finalModelPath,
                                          std::filesystem::copy_options::overwrite_existing);
                std::cerr << "[ReCoNetTrainer] Model saved: " << finalModelPath << std::endl;
                updateProgress({config.getIterations(), config.getIterations(), 0, 0, 0,
                              "Training complete!", 0});
            } catch (const std::exception& e) {
                setError(std::string("Failed to copy model file: ") + e.what());
            }
        } else if (shouldStop.load()) {
            updateProgress({0, 0, 0, 0, 0, "Training cancelled", 0});
        } else {
            setError("Training failed - ONNX file not generated");
        }

    } catch (const std::exception& e) {
        setError(std::string("Training exception: ") + e.what());
        std::cerr << "[ReCoNetTrainer] Exception: " << e.what() << std::endl;
    }

    // Cleanup
    if (trainingContext) {
        SDL_GL_DeleteContext(trainingContext);
        trainingContext = nullptr;
    }
    if (trainingWindow) {
        SDL_DestroyWindow(trainingWindow);
        trainingWindow = nullptr;
    }

    training.store(false);
    std::cerr << "[ReCoNetTrainer] Training thread finished" << std::endl;
}

// ===========================================================================
// Image Preprocessing
// ===========================================================================

bool ReCoNetTrainer::preprocessImages(StylePreparationBin* bin, const Config& config,
                                      std::vector<std::string>& outImagePaths) {
    int targetRes = config.getResolution();
    int processedCount = 0;

    for (size_t i = 0; i < bin->elements.size(); i++) {
        StylePreparationElement* elem = bin->elements[i];

        if (elem->abspath.empty()) {
            continue; // Skip empty slots
        }

        if (shouldStop.load()) {
            return false;
        }

        updateProgress({processedCount, config.getIterations(), 0, 0, 0,
                       "Preprocessing image " + std::to_string(processedCount + 1) + "...", 0});

        // Preprocess image
        GLuint texture = preprocessSingleImage(elem->abspath, targetRes, targetRes);

        if (texture == (GLuint)-1) {
            setError("Failed to preprocess image: " + elem->abspath);
            return false;
        }

        // Check for GL errors
        GLenum err = glGetError();
        if (err == GL_OUT_OF_MEMORY) {
            setError("Out of VRAM during preprocessing. Reduce image count or quality.");
            glDeleteTextures(1, &texture);
            return false;
        }

        // Save preprocessed image (don't use pathtoplatform - it adds quotes for shell commands)
        std::string outputPath = tempDir + "/images/img_" + std::to_string(processedCount) + ".png";
        // Convert forward slashes to backslashes on Windows
        #ifdef _WIN32
        for (char& c : outputPath) {
            if (c == '/') c = '\\';
        }
        #endif

        if (!savePreprocessedImage(texture, targetRes, targetRes, outputPath)) {
            setError("Failed to save preprocessed image: " + outputPath);
            glDeleteTextures(1, &texture);
            return false;
        }

        outImagePaths.push_back(outputPath);

        // Delete texture immediately (don't use pool - avoid thread safety issues)
        glDeleteTextures(1, &texture);

        processedCount++;
    }

    std::cerr << "[ReCoNetTrainer] Preprocessed " << processedCount << " images" << std::endl;
    return processedCount > 0;
}

GLuint ReCoNetTrainer::preprocessSingleImage(const std::string& imagePath,
                                             int targetWidth, int targetHeight) {
    // Load image with DevIL
    ILuint ilImage;
    ilGenImages(1, &ilImage);
    ilBindImage(ilImage);

    if (!ilLoadImage(imagePath.c_str())) {
        std::cerr << "[ReCoNetTrainer] Failed to load image: " << imagePath << std::endl;
        ilDeleteImages(1, &ilImage);
        return (GLuint)-1;
    }

    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    int srcWidth = ilGetInteger(IL_IMAGE_WIDTH);
    int srcHeight = ilGetInteger(IL_IMAGE_HEIGHT);
    ILubyte* data = ilGetData();

    if (!data) {
        std::cerr << "[ReCoNetTrainer] Failed to get image data" << std::endl;
        ilDeleteImages(1, &ilImage);
        return (GLuint)-1;
    }

    // Create source texture (don't use pool - avoid thread safety issues)
    GLuint srcTexture;
    glGenTextures(1, &srcTexture);
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, srcWidth, srcHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    ilDeleteImages(1, &ilImage);

    // Create output texture (don't use pool - avoid thread safety issues)
    GLuint dstTexture;
    glGenTextures(1, &dstTexture);
    glBindTexture(GL_TEXTURE_2D, dstTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, targetWidth, targetHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, dstTexture, 0);

    // Check FBO status
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ReCoNetTrainer] FBO incomplete! Status: " << fboStatus << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &srcTexture);
        glDeleteTextures(1, &dstTexture);
        return (GLuint)-1;
    }

    // Render using our Lanczos3 resize shader
    glViewport(0, 0, targetWidth, targetHeight);

    // Disable depth testing and blending for simple copy
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Bind source texture BEFORE using shader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);

    // Use our resize shader
    glUseProgram(resizeShaderProgram);

    // Set input texture uniform to texture unit 0
    GLint texLoc = glGetUniformLocation(resizeShaderProgram, "inputTexture");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);
    } else {
        std::cerr << "[ReCoNetTrainer] ERROR: inputTexture uniform not found!" << std::endl;
    }

    // Set source size uniform for Lanczos3
    GLint sizeLoc = glGetUniformLocation(resizeShaderProgram, "sourceSize");
    if (sizeLoc != -1) {
        glUniform2f(sizeLoc, (float)srcWidth, (float)srcHeight);
    } else {
        std::cerr << "[ReCoNetTrainer] ERROR: sourceSize uniform not found!" << std::endl;
    }

    // Render fullscreen quad using our VAO
    renderFullscreenQuad();

    // Flush to ensure rendering completes
    glFlush();

    GLenum renderErr = glGetError();
    if (renderErr != GL_NO_ERROR) {
        std::cerr << "[ReCoNetTrainer] OpenGL error after rendering: " << renderErr << std::endl;
    }

    // Cleanup and restore state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &srcTexture);  // Delete directly (don't use pool - avoid thread safety issues)

    // Unbind shader program to avoid polluting main thread
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    return dstTexture;
}

bool ReCoNetTrainer::savePreprocessedImage(GLuint texture, int width, int height,
                                           const std::string& outputPath) {
    // Validate parameters
    if (width <= 0 || height <= 0) {
        std::cerr << "[ReCoNetTrainer] Invalid dimensions: " << width << "x" << height << std::endl;
        return false;
    }

    if (texture == 0 || texture == (GLuint)-1) {
        std::cerr << "[ReCoNetTrainer] Invalid texture ID: " << texture << std::endl;
        return false;
    }

    // Download from GL texture as RGB (3 bytes per pixel)
    std::vector<unsigned char> pixels(width * height * 3);

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Check for GL errors
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        std::cerr << "[ReCoNetTrainer] OpenGL error during texture read: " << glErr << std::endl;
        return false;
    }

    // OpenGL textures are bottom-up, but PNG expects top-down
    // Flip the image vertically
    std::vector<unsigned char> flippedPixels(width * height * 3);
    for (int y = 0; y < height; y++) {
        memcpy(&flippedPixels[y * width * 3],
               &pixels[(height - 1 - y) * width * 3],
               width * 3);
    }

    // Check if directory exists
    std::filesystem::path filePath(outputPath);
    std::filesystem::path dirPath = filePath.parent_path();

    if (!std::filesystem::exists(dirPath)) {
        try {
            std::filesystem::create_directories(dirPath);
        } catch (const std::exception& e) {
            std::cerr << "[ReCoNetTrainer] Failed to create directory: " << e.what() << std::endl;
            return false;
        }
    }

    // Save using stb_image_write (thread-safe, no DevIL issues)
    int result = stbi_write_png(outputPath.c_str(), width, height, 3, flippedPixels.data(), width * 3);

    if (!result) {
        std::cerr << "[ReCoNetTrainer] Failed to save PNG: " << outputPath << std::endl;
        return false;
    }

    return true;
}

// ===========================================================================
// Python Training
// ===========================================================================

bool ReCoNetTrainer::generateConfigJSON(const Config& config,
                                        const std::vector<std::string>& imagePaths,
                                        const std::string& modelName,
                                        const std::string& outputPath) {
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        setError("Failed to create config file: " + outputPath);
        return false;
    }

    // Helper to prepare paths for JSON
    auto preparePathForJson = [](const std::string& str) -> std::string {
        std::string result;
        // Remove any quote characters and convert backslashes to forward slashes
        // Forward slashes work in Python on all platforms including Windows
        for (char c : str) {
            if (c == '"') {
                // Skip quotes
            } else if (c == '\\') {
                result += '/';  // Use forward slash instead
            } else {
                result += c;
            }
        }
        return result;
    };

    file << "{\n";
    file << "  \"model_name\": \"" << preparePathForJson(modelName) << "\",\n";
    file << "  \"resolution\": " << config.getResolution() << ",\n";
    file << "  \"batch_size\": " << config.getBatchSize() << ",\n";
    file << "  \"iterations\": " << config.getIterations() << ",\n";
    file << "  \"learning_rate\": " << config.learningRate << ",\n";
    file << "  \"content_weight\": " << config.contentWeight << ",\n";
    file << "  \"style_weight\": " << config.styleWeight << ",\n";
    file << "  \"tv_weight\": " << config.tvWeight << ",\n";
    file << "  \"use_gpu\": " << (config.useGPU ? "true" : "false") << ",\n";
    file << "  \"image_paths\": [\n";

    for (size_t i = 0; i < imagePaths.size(); i++) {
        file << "    \"" << preparePathForJson(imagePaths[i]) << "\"";
        if (i < imagePaths.size() - 1) file << ",";
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    std::cerr << "[ReCoNetTrainer] Config JSON created: " << outputPath << std::endl;
    return true;
}

bool ReCoNetTrainer::launchPythonTraining(const std::string& configPath,
                                          const std::string& outputPath) {
    trainingScriptPath = scriptsDir + "/train_reconet.py";

    // Use -u flag for unbuffered output so we get real-time progress
    std::string cmd = "\"" + pythonExecutable + "\" -u \"" + trainingScriptPath +
                      "\" --config \"" + configPath + "\" --output \"" + outputPath + "\"";

    std::cerr << "[ReCoNetTrainer] Launching: " + cmd << std::endl;

#ifdef _WIN32
    // Create pipes for stdout/stderr
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE stdoutWrite, stderrWrite;
    if (!CreatePipe(&stdoutReadHandle, &stdoutWrite, &saAttr, 0) ||
        !SetHandleInformation(stdoutReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stdout pipe");
        return false;
    }

    if (!CreatePipe(&stderrReadHandle, &stderrWrite, &saAttr, 0) ||
        !SetHandleInformation(stderrReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stderr pipe");
        CloseHandle(stdoutReadHandle);
        CloseHandle(stdoutWrite);
        return false;
    }

    // Setup process
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    // Create process
    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to launch Python training process");
        CloseHandle(stdoutReadHandle);
        CloseHandle(stdoutWrite);
        CloseHandle(stderrReadHandle);
        CloseHandle(stderrWrite);
        return false;
    }

    // Close write ends (child process has them)
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    processHandle = pi.hProcess;
    CloseHandle(pi.hThread);

    return true;
#else
    // Unix implementation - simple system() call for now
    // TODO: Implement proper fork/exec with pipe redirection
    int result = system(cmd.c_str());
    return (result == 0);
#endif
}

void ReCoNetTrainer::monitorTrainingProgress() {
#ifdef _WIN32
    char buffer[4096];
    DWORD bytesRead;

    while (!shouldStop.load()) {
        // Check if process is still running
        DWORD exitCode;
        if (GetExitCodeProcess((HANDLE)processHandle, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                std::cerr << "[ReCoNetTrainer] Process exited with code: " << exitCode << std::endl;
                if (exitCode != 0) {
                    setError("Training process failed (exit code: " + std::to_string(exitCode) + ")");
                }
                break;
            }
        }

        // Read stdout
        DWORD available;
        if (PeekNamedPipe((HANDLE)stdoutReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (ReadFile((HANDLE)stdoutReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                        &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string output(buffer);

                // Parse line by line
                std::istringstream stream(output);
                std::string line;
                while (std::getline(stream, line)) {
                    parseProgressLine(line);
                    std::cerr << "[Training] " << line << std::endl;
                }
            }
        }

        // Read stderr for error messages
        if (PeekNamedPipe((HANDLE)stderrReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (ReadFile((HANDLE)stderrReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                        &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string output(buffer);

                // Print stderr lines
                std::istringstream stream(output);
                std::string line;
                while (std::getline(stream, line)) {
                    std::cerr << "[Training ERROR] " << line << std::endl;
                }
            }
        }

        // Short sleep to avoid busy-waiting
        Sleep(100);
    }

    // Close handles
    CloseHandle((HANDLE)stdoutReadHandle);
    CloseHandle((HANDLE)stderrReadHandle);
    stdoutReadHandle = nullptr;
    stderrReadHandle = nullptr;
#endif
}

bool ReCoNetTrainer::parseProgressLine(const std::string& line) {
    // Parse lines like: "[Iteration 450/2000] Loss: 12.34"
    size_t iterPos = line.find("[Iteration");
    if (iterPos == std::string::npos) {
        return false;
    }

    int currentIter = 0, totalIter = 0;
    float loss = 0.0f;

    // Extract iteration numbers
    sscanf(line.c_str() + iterPos, "[Iteration %d/%d]", &currentIter, &totalIter);

    // Extract loss
    size_t lossPos = line.find("Loss:");
    if (lossPos != std::string::npos) {
        sscanf(line.c_str() + lossPos, "Loss: %f", &loss);
    }

    // Update progress
    Progress newProgress;
    newProgress.currentIteration = currentIter;
    newProgress.totalIterations = totalIter;
    newProgress.totalLoss = loss;
    newProgress.status = "Training...";

    updateProgress(newProgress);
    return true;
}

// ===========================================================================
// Helper Methods
// ===========================================================================

bool ReCoNetTrainer::deployTrainingScripts() {
    // For now, scripts are expected to be pre-deployed or embedded
    // In production, you would embed the Python scripts as string literals
    // and write them to scriptsDir on first run

    std::string trainScript = scriptsDir + "/train_reconet.py";
    std::string modelScript = scriptsDir + "/reconet_model.py";
    std::string lossScript = scriptsDir + "/loss_functions.py";

    // Check if scripts exist
    if (std::filesystem::exists(trainScript) &&
        std::filesystem::exists(modelScript) &&
        std::filesystem::exists(lossScript)) {
        std::cerr << "[ReCoNetTrainer] Training scripts found" << std::endl;
        return true;
    }

    // Scripts not found - for now, just warn
    // TODO: Embed and deploy scripts automatically
    std::cerr << "[ReCoNetTrainer] WARNING: Training scripts not found at " << scriptsDir << std::endl;
    std::cerr << "[ReCoNetTrainer] Scripts must be manually deployed for now" << std::endl;

    return true; // Don't fail initialization, user will deploy manually
}

bool ReCoNetTrainer::validateModelName(const std::string& name) {
    if (name.empty() || name.length() > 64) {
        return false;
    }

    for (char c : name) {
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

int ReCoNetTrainer::countValidImages(StylePreparationBin* bin) {
    int count = 0;
    for (auto* elem : bin->elements) {
        if (!elem->abspath.empty()) {
            count++;
        }
    }
    return count;
}

void ReCoNetTrainer::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
}

void ReCoNetTrainer::updateProgress(const Progress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
}

void ReCoNetTrainer::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

void ReCoNetTrainer::createFullscreenQuad() {
    if (fullscreenQuadVAO != 0) return; // Already created

    // Fullscreen quad vertices (position + texcoords)
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
    };

    glGenVertexArrays(1, &fullscreenQuadVAO);
    glGenBuffers(1, &fullscreenQuadVBO);

    glBindVertexArray(fullscreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // Texcoord attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::cout << "[ReCoNetTrainer] Created fullscreen quad VAO: " << fullscreenQuadVAO
              << " VBO: " << fullscreenQuadVBO << std::endl;
}

void ReCoNetTrainer::renderFullscreenQuad() {
    if (fullscreenQuadVAO == 0) {
        std::cerr << "[ReCoNetTrainer] ERROR: VAO not created!" << std::endl;
        return;
    }

    glBindVertexArray(fullscreenQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

bool ReCoNetTrainer::createResizeShader() {
    // Simple vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // Lanczos3 fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D inputTexture;
        uniform vec2 sourceSize;  // Source texture dimensions

        #define PI 3.14159265359

        // Lanczos3 kernel
        float lanczos3(float x) {
            if (x == 0.0) return 1.0;
            if (abs(x) >= 3.0) return 0.0;
            float pix = PI * x;
            return (sin(pix) / pix) * (sin(pix / 3.0) / (pix / 3.0));
        }

        void main() {
            vec2 texelSize = 1.0 / sourceSize;
            vec2 centerCoord = TexCoord * sourceSize - 0.5;
            vec2 centerPixel = floor(centerCoord);
            vec2 fracCoord = centerCoord - centerPixel;

            vec4 color = vec4(0.0);
            float weightSum = 0.0;

            // Sample 6x6 neighborhood for Lanczos3
            for (int y = -2; y <= 3; y++) {
                for (int x = -2; x <= 3; x++) {
                    vec2 offset = vec2(float(x), float(y));
                    vec2 samplePos = (centerPixel + offset + 0.5) * texelSize;

                    // Calculate Lanczos weight
                    vec2 delta = offset - fracCoord;
                    float weight = lanczos3(delta.x) * lanczos3(delta.y);

                    // Sample and accumulate
                    color += texture(inputTexture, samplePos) * weight;
                    weightSum += weight;
                }
            }

            // Normalize by total weight
            if (weightSum > 0.0) {
                color /= weightSum;
            }

            FragColor = color;
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "[ReCoNetTrainer] Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "[ReCoNetTrainer] Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    // Link shaders
    resizeShaderProgram = glCreateProgram();
    glAttachShader(resizeShaderProgram, vertexShader);
    glAttachShader(resizeShaderProgram, fragmentShader);
    glLinkProgram(resizeShaderProgram);

    glGetProgramiv(resizeShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(resizeShaderProgram, 512, NULL, infoLog);
        std::cerr << "[ReCoNetTrainer] Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "[ReCoNetTrainer] Resize shader created successfully (program ID: " << resizeShaderProgram << ")" << std::endl;
    return true;
}
