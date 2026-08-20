/**
 * ReCoNetTrainer.cpp
 *
 * Implementation of ReCoNet Style Transfer Trainer
 *
 * License: GPL3
 */
#include "ReCoNetTrainer.h"
#include "ReCoNetInstaller.h"
#include "AIStyleTransfer.h"
#include "program.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#include "ImageLoader.h"

extern "C" {
#include <libswscale/swscale.h>
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>

// Helper function to run a command without showing a console window
// Returns true if CreateProcess succeeded, exitCode and output are set
static bool runCommandSilent(const std::string& cmd, std::string& output, int& exitCode) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }

    // Ensure the read handle is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess needs a mutable copy of the command line
    std::string cmdCopy = cmd;

    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    // Close write end of pipe in parent process
    CloseHandle(hWritePipe);

    // Read output from pipe, stopping once process exits (avoids blocking on
    // inherited handles kept open by grandchild processes like torch workers)
    output.clear();
    char buffer[256];
    DWORD bytesRead;
    for (;;) {
        // Check if the process has exited
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            // Drain any remaining data
            DWORD avail = 0;
            while (PeekNamedPipe(hReadPipe, NULL, 0, NULL, &avail, NULL) && avail > 0) {
                if (!ReadFile(hReadPipe, buffer, std::min(avail, (DWORD)(sizeof(buffer) - 1)), &bytesRead, NULL))
                    break;
                buffer[bytesRead] = '\0';
                output += buffer;
            }
            break;
        }
        // Process still running — read whatever is available
        DWORD avail = 0;
        if (PeekNamedPipe(hReadPipe, NULL, 0, NULL, &avail, NULL) && avail > 0) {
            if (!ReadFile(hReadPipe, buffer, std::min(avail, (DWORD)(sizeof(buffer) - 1)), &bytesRead, NULL))
                break;
            buffer[bytesRead] = '\0';
            output += buffer;
        } else {
            Sleep(10);
        }
    }

    // Wait for process to finish (already done above, but needed for exit code)
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    exitCode = static_cast<int>(dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return true;
}

#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#endif

// External global
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
    // Use ReCoNetInstaller's method which checks both getenv() AND Windows registry
    std::string envPythonStr = ReCoNetInstaller::getEnvironmentVariable();

    // Find Python executable - MUST be Python 3.12.x specifically
#ifdef _WIN32
    // Try common Python 3.12 locations (prioritize 3.12, no 3.11 or 3.13)
    std::vector<std::string> pythonPaths;
    if (!envPythonStr.empty()) {
        pythonPaths.push_back(envPythonStr);
        std::cerr << "[ReCoNetTrainer] Using Python from EWOCVJ2_PYTHON: " << envPythonStr << std::endl;
    }
    const char* username = getenv("USERNAME");
    std::string usernameStr = username ? username : "";
    pythonPaths.insert(pythonPaths.end(), {
        "C:\\Python312\\python.exe",
        "C:\\Users\\" + usernameStr + "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe"
    });
#else
    std::vector<std::string> pythonPaths;
    if (!envPythonStr.empty()) {
        pythonPaths.push_back(envPythonStr);
    }
    // Prefer EWOCvj2-managed Python 3.12 (has PyTorch + all packages) over system Python
    const char* homeDir = getenv("HOME");
    if (homeDir) {
#ifdef __APPLE__
        pythonPaths.push_back(std::string(homeDir) + "/Library/Application Support/EWOCvj2/python312/bin/python3.12");
#else
        pythonPaths.push_back(std::string(homeDir) + "/.local/share/EWOCvj2/python312/bin/python3.12");
#endif
    }
    pythonPaths.insert(pythonPaths.end(), {
        "/opt/homebrew/bin/python3.12",  // Homebrew on Apple Silicon
        "/usr/local/bin/python3.12",
        "/usr/bin/python3.12",
        "python3",
        "python",
        "/usr/bin/python3",
        "/usr/local/bin/python3"
    });
#endif

    bool foundPython = false;
    for (const auto& path : pythonPaths) {
#ifdef _WIN32
        // Try to run python --version silently (no console window)
        std::string cmd = "\"" + path + "\" --version";
        std::string output;
        int exitCode;
        if (runCommandSilent(cmd, output, exitCode) && exitCode == 0) {
            pythonExecutable = path;
            foundPython = true;
            std::cerr << "[ReCoNetTrainer] Found Python: " << path << std::endl;
            break;
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
        setError("Python 3.12 not found. Use the ReCoNet Installer to install Python 3.12.");
        std::cerr << "[ReCoNetTrainer] ERROR: " << lastError << std::endl;
        return false;
    }

    // Check for PyTorch silently (no console window)
#ifdef _WIN32
    std::string checkCmd = "\"" + pythonExecutable + "\" -c \"import torch; print(torch.__version__)\"";
    std::string result;
    int torchExitCode;
    bool torchFound = false;

    if (runCommandSilent(checkCmd, result, torchExitCode)) {
        std::cerr << "[ReCoNetTrainer] PyTorch check output: " << result;

        // Check if output looks like a version number
        if (result.find('.') != std::string::npos &&
            result.find("ModuleNotFoundError") == std::string::npos &&
            result.find("ImportError") == std::string::npos &&
            torchExitCode == 0) {
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
    modelsDir  = mainprogram->programData + "/EWOCvj2/models/styles";
    scriptsDir = mainprogram->programData + "/EWOCvj2/scripts";
#ifdef _WIN32
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    tempDir = std::string(tempPath) + "EWOCvj2\\reconet_training";
#else
    tempDir = mainprogram->temppath + "/EWOCvj2_reconet_training";
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

    // Join previous training thread if it exists (prevents std::terminate on destruction)
    if (trainingThread && trainingThread->joinable()) {
        trainingThread->join();
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
        // Collect original image paths (for high-res style loading in Python)
        std::vector<std::string> originalImagePaths;
        for (size_t i = 0; i < bin->elements.size(); i++) {
            StylePreparationElement* elem = bin->elements[i];
            if (!elem->abspath.empty()) {
                originalImagePaths.push_back(elem->abspath);
            }
        }
        std::cerr << "[ReCoNetTrainer] Collected " << originalImagePaths.size() << " original style image paths" << std::endl;

        // Preprocess images (for content-resolution fallback)
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

        if (!generateConfigJSON(config, imagePaths, originalImagePaths, modelName, configPath)) {
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
                if (exists(finalModelPath)) {
                    safe_remove(finalModelPath);
                }
                std::filesystem::copy_file(outputOnnxPath, finalModelPath,
                                          std::filesystem::copy_options::overwrite_existing);
                std::cerr << "[ReCoNetTrainer] Model saved: " << finalModelPath << std::endl;
                updateProgress({config.getIterations(), config.getIterations(), 0, 0, 0,
                              "Training complete!", 0});

                // Add new style to available styles list
                mainstyleroom->updatelists();

                std::string modelsdir = mainprogram->programData + "/EWOCvj2/models/styles/";
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

    training.store(false);
    std::cerr << "[ReCoNetTrainer] Training thread finished" << std::endl;
}

// ===========================================================================
// Image Preprocessing
// ===========================================================================

bool ReCoNetTrainer::preprocessImages(StylePreparationBin* bin, const Config& config,
                                      std::vector<std::string>& outImagePaths) {
    // Use inspirationResolution for preprocessing - smallest side will be this value
    int targetMinDimension = config.inspirationResolution;
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

        // Preprocess image - output dimensions calculated based on aspect ratio
        int outWidth = 0, outHeight = 0;
        std::vector<uint8_t> pixels = preprocessSingleImage(elem->abspath, targetMinDimension, outWidth, outHeight);

        if (pixels.empty() || outWidth <= 0 || outHeight <= 0) {
            setError("Failed to preprocess image: " + elem->abspath);
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

        if (!savePreprocessedImage(pixels, outWidth, outHeight, outputPath)) {
            setError("Failed to save preprocessed image: " + outputPath);
            return false;
        }

        outImagePaths.push_back(outputPath);

        processedCount++;
    }

    std::cerr << "[ReCoNetTrainer] Preprocessed " << processedCount << " images" << std::endl;
    return processedCount > 0;
}

std::vector<uint8_t> ReCoNetTrainer::preprocessSingleImage(const std::string& imagePath,
                                             int targetMinDimension, int& outWidth, int& outHeight) {
    // Initialize output parameters to safe defaults
    outWidth = 0;
    outHeight = 0;

    // Load image with FFmpeg
    int srcWidth, srcHeight;
    auto imgData = ImageLoader::loadImageRGBA(imagePath, &srcWidth, &srcHeight);

    if (imgData.empty() || srcWidth <= 0 || srcHeight <= 0) {
        std::cerr << "[ReCoNetTrainer] Failed to load image: " << imagePath << std::endl;
        return {};
    }

    // Calculate output dimensions: smallest side = targetMinDimension, maintain aspect ratio
    int targetWidth, targetHeight;
    if (srcWidth <= srcHeight) {
        // Width is smaller or equal - scale width to target, height proportionally
        targetWidth = targetMinDimension;
        targetHeight = (int)((float)srcHeight / (float)srcWidth * targetMinDimension);
    } else {
        // Height is smaller - scale height to target, width proportionally
        targetHeight = targetMinDimension;
        targetWidth = (int)((float)srcWidth / (float)srcHeight * targetMinDimension);
    }

    // Clamp to reasonable maximum to avoid memory issues with extreme aspect ratios
    const int maxDimension = 4096;
    if (targetWidth > maxDimension) {
        targetHeight = (int)((float)targetHeight * maxDimension / targetWidth);
        targetWidth = maxDimension;
    }
    if (targetHeight > maxDimension) {
        targetWidth = (int)((float)targetWidth * maxDimension / targetHeight);
        targetHeight = maxDimension;
    }

    // Ensure minimum dimensions
    if (targetWidth < 1) targetWidth = 1;
    if (targetHeight < 1) targetHeight = 1;

    // Return computed dimensions to caller
    outWidth = targetWidth;
    outHeight = targetHeight;

    std::cerr << "[ReCoNetTrainer] Resizing " << srcWidth << "x" << srcHeight
              << " -> " << targetWidth << "x" << targetHeight << std::endl;

    // CPU resize via sws_scale (Lanczos) - no GL context needed, so this runs
    // fine on a background thread (SDL/GL window+context creation must happen
    // on the main thread on macOS, which this preprocessing used to violate).
    SwsContext* swsCtx = sws_getContext(
        srcWidth, srcHeight, AV_PIX_FMT_RGBA,
        targetWidth, targetHeight, AV_PIX_FMT_RGB24,
        SWS_LANCZOS, nullptr, nullptr, nullptr);
    if (!swsCtx) {
        std::cerr << "[ReCoNetTrainer] Failed to create sws context for " << imagePath << std::endl;
        return {};
    }

    std::vector<uint8_t> outPixels(static_cast<size_t>(targetWidth) * targetHeight * 3);

    const uint8_t* srcSlices[1] = { imgData.data() };
    int srcStrides[1] = { srcWidth * 4 };
    uint8_t* dstSlices[1] = { outPixels.data() };
    int dstStrides[1] = { targetWidth * 3 };

    sws_scale(swsCtx, srcSlices, srcStrides, 0, srcHeight, dstSlices, dstStrides);
    sws_freeContext(swsCtx);

    return outPixels;
}

bool ReCoNetTrainer::savePreprocessedImage(const std::vector<uint8_t>& pixels, int width, int height,
                                           const std::string& outputPath) {
    // Validate parameters
    if (width <= 0 || height <= 0) {
        std::cerr << "[ReCoNetTrainer] Invalid dimensions: " << width << "x" << height << std::endl;
        return false;
    }

    if (pixels.size() != static_cast<size_t>(width) * height * 3) {
        std::cerr << "[ReCoNetTrainer] Pixel buffer size mismatch for " << outputPath << std::endl;
        return false;
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

    // Save using stb_image_write (thread-safe, no DevIL issues). sws_scale
    // writes rows top-down already, same as PNG expects - no flip needed.
    int result = stbi_write_png(outputPath.c_str(), width, height, 3, pixels.data(), width * 3);

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
                                        const std::vector<std::string>& originalImagePaths,
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
    file << "  \"temporal_weight\": " << config.temporalWeight << ",\n";
    file << "  \"video_dataset\": \"" << preparePathForJson(config.videoDataset) << "\",\n";
    file << "  \"sequence_length\": " << config.sequenceLength << ",\n";
    file << "  \"use_gpu\": " << (config.useGPU ? "true" : "false") << ",\n";
    file << "  \"content_dataset\": \"" << preparePathForJson(config.contentDataset) << "\",\n";

    // Advanced: Per-layer style weights (VGG19 layers)
    file << "  \"style_layer_weights\": {\n";
    file << "    \"relu1_1\": " << config.styleWeightRelu1 << ",\n";
    file << "    \"relu2_1\": " << config.styleWeightRelu2 << ",\n";
    file << "    \"relu3_1\": " << config.styleWeightRelu3 << ",\n";
    file << "    \"relu4_1\": " << config.styleWeightRelu4 << ",\n";
    file << "    \"relu5_1\": " << config.styleWeightRelu5 << "\n";
    file << "  },\n";

    // Preprocessed image paths (legacy, for content-resolution style if needed)
    file << "  \"image_paths\": [\n";
    for (size_t i = 0; i < imagePaths.size(); i++) {
        file << "    \"" << preparePathForJson(imagePaths[i]) << "\"";
        if (i < imagePaths.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    // Original high-res image paths for style (preserves brush stroke detail!)
    file << "  \"original_style_paths\": [\n";
    for (size_t i = 0; i < originalImagePaths.size(); i++) {
        file << "    \"" << preparePathForJson(originalImagePaths[i]) << "\"";
        if (i < originalImagePaths.size() - 1) file << ",";
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
    int stdoutPipe[2], stderrPipe[2];
    if (pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0) {
        setError("Failed to create pipes for training process");
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        setError("Failed to fork training process");
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        close(stderrPipe[0]); close(stderrPipe[1]);
        return false;
    }

    if (pid == 0) {
        // Child: wire up pipes and exec
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        close(stderrPipe[0]); close(stderrPipe[1]);
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }

    // Parent: close write ends, store read ends
    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    childPid      = pid;
    stdoutReadFd  = stdoutPipe[0];
    stderrReadFd  = stderrPipe[0];
    // Make reads non-blocking so the monitor loop can interleave stdout+stderr
    fcntl(stdoutReadFd, F_SETFL, O_NONBLOCK);
    fcntl(stderrReadFd, F_SETFL, O_NONBLOCK);
    return true;
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

                // Print stderr lines and check for OOM errors
                std::istringstream stream(output);
                std::string line;
                while (std::getline(stream, line)) {
                    std::cerr << "[Training ERROR] " << line << std::endl;

                    // Check for CUDA/PyTorch out of memory errors
                    if (line.find("CUDA out of memory") != std::string::npos ||
                        line.find("OutOfMemoryError") != std::string::npos ||
                        line.find("out of memory") != std::string::npos) {
                        mainprogram->infostr = "Out of VRAM! Reduce training resolution or batch size.";
                        setError("CUDA out of memory. Reduce resolution or batch size.");
                        shouldStop.store(true);
                    }
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
#else
    char buffer[4096];
    std::string stdoutBuf, stderrBuf;

    auto drainLines = [&](std::string& buf, bool isErr) {
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (isErr) {
                std::cerr << "[Training ERROR] " << line << std::endl;
                if (line.find("CUDA out of memory") != std::string::npos ||
                    line.find("OutOfMemoryError")   != std::string::npos ||
                    line.find("out of memory")       != std::string::npos) {
                    mainprogram->infostr = "Out of VRAM! Reduce training resolution or batch size.";
                    setError("CUDA out of memory. Reduce resolution or batch size.");
                    shouldStop.store(true);
                }
            } else {
                std::cerr << "[Training] " << line << std::endl;
                parseProgressLine(line);
            }
        }
    };

    while (true) {
        struct pollfd fds[2];
        fds[0] = { stdoutReadFd, POLLIN, 0 };
        fds[1] = { stderrReadFd, POLLIN, 0 };
        poll(fds, 2, 100);   // 100 ms timeout so shouldStop is checked regularly

        // Drain stdout
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(stdoutReadFd, buffer, sizeof(buffer) - 1);
            if (n > 0) { buffer[n] = '\0'; stdoutBuf += buffer; drainLines(stdoutBuf, false); }
        }
        // Drain stderr
        if (fds[1].revents & POLLIN) {
            ssize_t n = read(stderrReadFd, buffer, sizeof(buffer) - 1);
            if (n > 0) { buffer[n] = '\0'; stderrBuf += buffer; drainLines(stderrBuf, true); }
        }

        // Check if process exited
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);
        if (result == childPid) {
            // Drain remaining output
            ssize_t n;
            while ((n = read(stdoutReadFd, buffer, sizeof(buffer) - 1)) > 0)
                { buffer[n] = '\0'; stdoutBuf += buffer; drainLines(stdoutBuf, false); }
            while ((n = read(stderrReadFd, buffer, sizeof(buffer) - 1)) > 0)
                { buffer[n] = '\0'; stderrBuf += buffer; drainLines(stderrBuf, true); }

            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            std::cerr << "[ReCoNetTrainer] Process exited with code: " << exitCode << std::endl;
            if (exitCode != 0 && !shouldStop.load())
                setError("Training process failed (exit code: " + std::to_string(exitCode) + ")");
            break;
        }

        if (shouldStop.load()) {
            kill(childPid, SIGTERM);
            waitpid(childPid, nullptr, 0);
            break;
        }
    }

    close(stdoutReadFd);
    close(stderrReadFd);
    childPid     = -1;
    stdoutReadFd = -1;
    stderrReadFd = -1;
#endif
}

bool ReCoNetTrainer::parseProgressLine(const std::string& line) {
    // Parse lines like: "[Iteration 450/2000] Loss: 12.34 ... Remain: 120.3s"
    size_t iterPos = line.find("[Iteration");
    if (iterPos == std::string::npos) {
        return false;
    }

    int currentIter = 0, totalIter = 0;
    float loss = 0.0f;
    float remainingTime = 0.0f;

    // Extract iteration numbers
    sscanf(line.c_str() + iterPos, "[Iteration %d/%d]", &currentIter, &totalIter);

    // Extract loss
    size_t lossPos = line.find("Loss:");
    if (lossPos != std::string::npos) {
        sscanf(line.c_str() + lossPos, "Loss: %f", &loss);
    }

    // Extract remaining time (format: "Remain: 120.3s")
    size_t remainPos = line.find("Remain:");
    if (remainPos != std::string::npos) {
        sscanf(line.c_str() + remainPos, "Remain: %f", &remainingTime);
    }

    // Update progress
    Progress newProgress;
    newProgress.currentIteration = currentIter;
    newProgress.totalIterations = totalIter;
    newProgress.totalLoss = loss;
    newProgress.estimatedTimeRemaining = remainingTime;
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
    std::cerr << "[ReCoNetTrainer] Error: " << error << std::endl;
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

