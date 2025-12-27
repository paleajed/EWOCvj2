/**
 * VideoUpscaler.cpp
 *
 * Implementation of offline video upscaling using EDVR and FlashVSR
 *
 * License: GPL3
 */

#include "VideoUpscaler.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <regex>
#include <set>

// FFmpeg includes for HAP encoding
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
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

    // Read output from pipe
    output.clear();
    char buffer[256];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    // Wait for process to finish
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
#include <signal.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

// ===========================================================================
// Constructor / Destructor
// ===========================================================================

VideoUpscaler::VideoUpscaler() {
    processing.store(false);
    shouldStop.store(false);
    initialized = false;
}

VideoUpscaler::~VideoUpscaler() {
    // Stop processing if active
    shouldStop.store(true);

    // Wait for processing thread
    if (processingThread && processingThread->joinable()) {
        processingThread->join();
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

bool VideoUpscaler::initialize() {
    if (initialized) {
        return true;
    }

    std::cerr << "[VideoUpscaler] Initializing..." << std::endl;

    // Check for user-specified Python path via environment variable
    const char* envPython = getenv("EWOCVJ2_PYTHON");

    // Find Python executable
#ifdef _WIN32
    std::vector<std::string> pythonPaths;
    if (envPython && strlen(envPython) > 0) {
        pythonPaths.push_back(envPython);
        std::cerr << "[VideoUpscaler] Using Python from EWOCVJ2_PYTHON: " << envPython << std::endl;
    }
    const char* username = getenv("USERNAME");
    std::string usernameStr = username ? username : "User";
    pythonPaths.insert(pythonPaths.end(), {
        "python",
        "python3",
        "C:\\Python313\\python.exe",
        "C:\\Python312\\python.exe",
        "C:\\Python311\\python.exe",
        "C:\\Python310\\python.exe",
        "C:\\Users\\" + usernameStr + "\\AppData\\Local\\Programs\\Python\\Python313\\python.exe",
        "C:\\Users\\" + usernameStr + "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe",
        "C:\\Users\\" + usernameStr + "\\AppData\\Local\\Programs\\Python\\Python311\\python.exe"
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
        std::string cmd = "\"" + path + "\" --version";
        std::string output;
        int exitCode;
        if (runCommandSilent(cmd, output, exitCode) && exitCode == 0) {
            pythonExecutable = path;
            foundPython = true;
            std::cerr << "[VideoUpscaler] Found Python: " << path << std::endl;
            break;
        }
#else
        std::string cmd = "\"" + path + "\" --version >/dev/null 2>&1";
        if (system(cmd.c_str()) == 0) {
            pythonExecutable = path;
            foundPython = true;
            std::cerr << "[VideoUpscaler] Found Python: " << path << std::endl;
            break;
        }
#endif
    }

    if (!foundPython) {
        setError("Python 3 not found. Please install Python 3.8+ from python.org");
        std::cerr << "[VideoUpscaler] ERROR: " << lastError << std::endl;
        return false;
    }

    // Check for PyTorch
#ifdef _WIN32
    std::string checkCmd = "\"" + pythonExecutable + "\" -c \"import torch; print(torch.__version__)\"";
    std::string result;
    int torchExitCode;
    bool torchFound = false;

    if (runCommandSilent(checkCmd, result, torchExitCode)) {
        if (result.find('.') != std::string::npos &&
            result.find("ModuleNotFoundError") == std::string::npos &&
            result.find("ImportError") == std::string::npos &&
            torchExitCode == 0) {
            torchFound = true;
        }
    }

    if (!torchFound) {
        setError("PyTorch not found. Install with: pip install torch torchvision");
        std::cerr << "[VideoUpscaler] ERROR: " << lastError << std::endl;
        return false;
    }
#else
    std::string checkCmd = "\"" + pythonExecutable + "\" -c \"import torch; print(torch.__version__)\"";
    if (system(checkCmd.c_str()) != 0) {
        setError("PyTorch not found. Install with: pip install torch torchvision");
        std::cerr << "[VideoUpscaler] ERROR: " << lastError << std::endl;
        return false;
    }
#endif
    std::cerr << "[VideoUpscaler] PyTorch detected" << std::endl;

    // FFmpeg/libav is linked directly - no need to find executables
    std::cerr << "[VideoUpscaler] Using libav API for video processing" << std::endl;

    // Setup directories
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) {
        std::string basePath = std::string(path) + "\\EWOCvj2";
        modelsDir = basePath + "\\models\\upscale";
        scriptsDir = basePath + "\\scripts";
    } else {
        modelsDir = "C:\\ProgramData\\EWOCvj2\\models\\upscale";
        scriptsDir = "C:\\ProgramData\\EWOCvj2\\scripts";
    }

    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    tempDir = std::string(tempPath) + "EWOCvj2\\video_upscale";
#else
    modelsDir = "/usr/local/share/EWOCvj2/models/upscale";
    scriptsDir = "/usr/local/share/EWOCvj2/scripts";
    tempDir = "/tmp/EWOCvj2_video_upscale";
#endif

    // Create directories
    try {
        fs::create_directories(modelsDir);
        fs::create_directories(scriptsDir);
        fs::create_directories(tempDir);
    } catch (const std::exception& e) {
        setError(std::string("Failed to create directories: ") + e.what());
        std::cerr << "[VideoUpscaler] ERROR: " << lastError << std::endl;
        return false;
    }

    std::cerr << "[VideoUpscaler] Models dir: " << modelsDir << std::endl;
    std::cerr << "[VideoUpscaler] Scripts dir: " << scriptsDir << std::endl;
    std::cerr << "[VideoUpscaler] Temp dir: " << tempDir << std::endl;

    // Deploy upscaling scripts
    if (!deployScripts()) {
        setError("Failed to deploy upscaling scripts");
        std::cerr << "[VideoUpscaler] ERROR: " << lastError << std::endl;
        return false;
    }

    initialized = true;
    std::cerr << "[VideoUpscaler] Initialization successful" << std::endl;
    return true;
}

// ===========================================================================
// Public API
// ===========================================================================

std::string VideoUpscaler::upscale(const std::string& inputPath,
                                    Quality quality,
                                    ScaleFactor scaleFactor,
                                    const Config& config) {
    std::string outputPath = generateOutputPath(inputPath, scaleFactor);

    if (upscaleToPath(inputPath, outputPath, quality, scaleFactor, config)) {
        return outputPath;
    }
    return "";
}

bool VideoUpscaler::upscaleToPath(const std::string& inputPath,
                                   const std::string& outputPath,
                                   Quality quality,
                                   ScaleFactor scaleFactor,
                                   const Config& config) {
    if (!initialized) {
        setError("Upscaler not initialized. Call initialize() first.");
        return false;
    }

    if (processing.load()) {
        setError("Processing already in progress");
        return false;
    }

    // Validate input file exists
    if (!fs::exists(inputPath)) {
        setError("Input file does not exist: " + inputPath);
        return false;
    }

    std::cerr << "[VideoUpscaler] Starting upscale:" << std::endl;
    std::cerr << "[VideoUpscaler]   Input: " << inputPath << std::endl;
    std::cerr << "[VideoUpscaler]   Output: " << outputPath << std::endl;
    std::cerr << "[VideoUpscaler]   Quality: " << qualityToString(quality) << std::endl;
    std::cerr << "[VideoUpscaler]   Scale: " << scaleFactorToString(scaleFactor) << std::endl;

    // Clear error and reset progress
    clearError();
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress = Progress();
        progress.status = "Starting...";
    }
    currentOutputPath = outputPath;

    // Join previous processing thread if it exists
    if (processingThread && processingThread->joinable()) {
        processingThread->join();
    }

    // Start processing thread
    shouldStop.store(false);
    processing.store(true);
    processingThread = std::make_unique<std::thread>(
        &VideoUpscaler::processingThreadFunc, this,
        inputPath, outputPath, quality, scaleFactor, config
    );

    return true;
}

void VideoUpscaler::stop() {
    if (!processing.load()) {
        return;
    }

    std::cerr << "[VideoUpscaler] Stopping processing..." << std::endl;
    shouldStop.store(true);

#ifdef _WIN32
    if (processHandle) {
        TerminateProcess((HANDLE)processHandle, 1);
    }
#else
    if (childPid > 0) {
        kill(childPid, SIGTERM);
    }
#endif
}

VideoUpscaler::Progress VideoUpscaler::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

VideoUpscaler::VideoInfo VideoUpscaler::getVideoInfo(const std::string& videoPath) {
    VideoInfo info;

    if (!fs::exists(videoPath)) {
        return info;
    }

    // Use libav API to get video information
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[VideoUpscaler] Could not open video: " << videoPath << std::endl;
        return info;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        avformat_close_input(&formatCtx);
        return info;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIdx < 0) {
        avformat_close_input(&formatCtx);
        return info;
    }

    AVStream* videoStream = formatCtx->streams[videoStreamIdx];
    AVCodecParameters* codecpar = videoStream->codecpar;

    // Get basic info
    info.width = codecpar->width;
    info.height = codecpar->height;

    // Get FPS from stream
    if (videoStream->r_frame_rate.den > 0) {
        info.fps = static_cast<float>(videoStream->r_frame_rate.num) /
                   static_cast<float>(videoStream->r_frame_rate.den);
    } else if (videoStream->avg_frame_rate.den > 0) {
        info.fps = static_cast<float>(videoStream->avg_frame_rate.num) /
                   static_cast<float>(videoStream->avg_frame_rate.den);
    }

    // Get duration in seconds
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<float>(formatCtx->duration) / AV_TIME_BASE;
    }

    // Get frame count
    info.frameCount = static_cast<int>(videoStream->nb_frames);
    if (info.frameCount == 0 && info.duration > 0 && info.fps > 0) {
        // Estimate frame count from duration and fps
        info.frameCount = static_cast<int>(info.duration * info.fps);
    }

    // Get codec name
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (codec) {
        info.codec = codec->name;
    }

    // Get pixel format name
    const char* pixFmtName = av_get_pix_fmt_name(static_cast<AVPixelFormat>(codecpar->format));
    if (pixFmtName) {
        info.pixelFormat = pixFmtName;
    }

    avformat_close_input(&formatCtx);
    return info;
}

// ===========================================================================
// Static Helper Methods
// ===========================================================================

std::string VideoUpscaler::generateOutputPath(const std::string& inputPath,
                                               ScaleFactor scaleFactor) {
    fs::path input(inputPath);
    fs::path stem = input.stem();
    fs::path parent = input.parent_path();

    std::string suffix = "_upscaled";
    switch (scaleFactor) {
        case ScaleFactor::CLEANUP: suffix = "_enhanced"; break;
        case ScaleFactor::X2: suffix = "_upscaled_2x"; break;
        case ScaleFactor::X3: suffix = "_upscaled_3x"; break;
        case ScaleFactor::X4: suffix = "_upscaled_4x"; break;
    }

    // HAP codec uses .mov container
    fs::path output = parent / (stem.string() + suffix + "_hap.mov");
    return output.string();
}

std::string VideoUpscaler::qualityToString(Quality quality) {
    switch (quality) {
        case Quality::FAST: return "FAST (EDVR)";
        case Quality::BALANCED: return "BALANCED (EDVR_HQ)";
        case Quality::ULTRA: return "ULTRA (FlashVSR)";
        default: return "Unknown";
    }
}

std::string VideoUpscaler::scaleFactorToString(ScaleFactor scaleFactor) {
    switch (scaleFactor) {
        case ScaleFactor::CLEANUP: return "Cleanup (1x enhanced)";
        case ScaleFactor::X2: return "2x";
        case ScaleFactor::X3: return "3x";
        case ScaleFactor::X4: return "4x";
        default: return "Unknown";
    }
}

int VideoUpscaler::scaleFactorToInt(ScaleFactor scaleFactor) {
    switch (scaleFactor) {
        case ScaleFactor::CLEANUP: return 1;  // Output is same resolution
        case ScaleFactor::X2: return 2;
        case ScaleFactor::X3: return 3;
        case ScaleFactor::X4: return 4;
        default: return 1;
    }
}

std::string VideoUpscaler::getModelName(Quality quality) {
    switch (quality) {
        case Quality::FAST: return "EDVR_M";           // Medium model - faster
        case Quality::BALANCED: return "EDVR_L";       // Large model - better quality
        case Quality::ULTRA: return "FlashVSR_Lite";   // TinyLong + TCDecoder
        default: return "EDVR_M";
    }
}

std::string VideoUpscaler::getScriptName(Quality quality) {
    if (isEDVRQuality(quality)) {
        return "upscale_edvr.py";
    } else {
        return "upscale_flashvsr.py";
    }
}

size_t VideoUpscaler::estimateOutputSize(const std::string& inputPath, ScaleFactor scaleFactor) {
    auto info = getVideoInfo(inputPath);
    if (info.frameCount == 0) {
        return 0;
    }

    // Rough estimate: output size scales with resolution squared
    int scale = scaleFactorToInt(scaleFactor);
    size_t inputSize = fs::file_size(inputPath);

    // Output size roughly scales with area (scale^2), but compression helps
    // Use a factor of scale^1.5 as a rough estimate
    float factor = std::pow(static_cast<float>(scale), 1.5f);
    return static_cast<size_t>(inputSize * factor);
}

size_t VideoUpscaler::estimateVRAM(int width, int height, Quality quality) {
    // Base VRAM for frame buffers (RGB, 3 bytes per pixel)
    size_t frameSize = static_cast<size_t>(width) * height * 3;

    // EDVR uses 5 frames (temporal radius of 2)
    // FlashVSR uses more frames for diffusion

    size_t modelSize = 0;
    size_t framebuffers = 0;

    switch (quality) {
        case Quality::FAST:
            modelSize = 100 * 1024 * 1024;      // ~100MB for EDVR_M
            framebuffers = frameSize * 10;      // Input + output + intermediate
            break;
        case Quality::BALANCED:
            modelSize = 200 * 1024 * 1024;      // ~200MB for EDVR_L
            framebuffers = frameSize * 12;
            break;
        case Quality::ULTRA:
            modelSize = 500 * 1024 * 1024;      // ~500MB for FlashVSR (TinyLong + TCDecoder)
            framebuffers = frameSize * 16;
            break;
    }

    // Add 20% safety margin
    return static_cast<size_t>((modelSize + framebuffers) * 1.2);
}

std::string VideoUpscaler::formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    char buffer[64];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%zu %s", bytes, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    }
    return std::string(buffer);
}

// ===========================================================================
// Processing Thread
// ===========================================================================

void VideoUpscaler::processingThreadFunc(std::string inputPath,
                                          std::string outputPath,
                                          Quality quality,
                                          ScaleFactor scaleFactor,
                                          Config config) {
    std::cerr << "[VideoUpscaler] Processing thread started (parallel mode)" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Create unique temp directory for this job
    std::string jobId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string jobTempDir = tempDir + "/" + jobId;
    std::string framesInputDir = jobTempDir + "/frames_in";
    std::string framesOutputDir = jobTempDir + "/frames_out";

#ifdef _WIN32
    for (char& c : jobTempDir) if (c == '/') c = '\\';
    for (char& c : framesInputDir) if (c == '/') c = '\\';
    for (char& c : framesOutputDir) if (c == '/') c = '\\';
#endif

    try {
        fs::create_directories(framesInputDir);
        fs::create_directories(framesOutputDir);
    } catch (const std::exception& e) {
        setError(std::string("Failed to create temp directories: ") + e.what());
        processing.store(false);
        return;
    }

    VideoInfo videoInfo;

    try {
        // Stage 1: Extract frames from input video
        updateProgress({0, 0, 0.0f, 0.0f, 0.0f, "Extracting frames...", "Extracting"});

        if (!extractFrames(inputPath, framesInputDir, videoInfo)) {
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        if (shouldStop.load()) {
            std::cerr << "[VideoUpscaler] Processing cancelled during extraction" << std::endl;
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        std::cerr << "[VideoUpscaler] Extracted " << videoInfo.frameCount << " frames" << std::endl;

        // Initialize chunked progress tracking
        totalVideoFrames = videoInfo.frameCount;
        cumulativeFramesProcessed = 0;
        lastChunkTotal = 0;
        lastChunkFrame = 0;
        highWaterMarkPercent = 0.0f;

        // Stage 2: Start parallel processing
        // - Audio extraction runs in its own thread
        // - Python upscaling runs async
        // - HAP encoding runs in parallel, encoding frames as they appear

        updateProgress({0, videoInfo.frameCount, 0.0f, 0.0f, 0.0f, "Starting upscaling...", "Upscaling"});

        // Generate WAV path
        std::string wavPath = outputPath;
        size_t lastDot = wavPath.rfind('.');
        if (lastDot != std::string::npos) {
            wavPath = wavPath.substr(0, lastDot) + ".wav";
        } else {
            wavPath += ".wav";
        }

        // Start audio extraction in a separate thread (parallel)
        std::atomic<bool> audioComplete{false};
        std::atomic<bool> audioSuccess{false};
        std::thread audioThread;

        if (config.preserveAudio) {
            audioThread = std::thread([this, inputPath, wavPath, &audioComplete, &audioSuccess]() {
                std::cerr << "[VideoUpscaler] Audio extraction thread started" << std::endl;
                audioSuccess = extractAudioToWav(inputPath, wavPath);
                audioComplete = true;
                std::cerr << "[VideoUpscaler] Audio extraction thread finished" << std::endl;
            });
        } else {
            audioComplete = true;
        }

        // Launch Python upscaling asynchronously
        if (!launchPythonUpscalingAsync(framesInputDir, framesOutputDir, quality, scaleFactor, config)) {
            std::cerr << "[VideoUpscaler] Failed to launch Python upscaling" << std::endl;
            if (audioThread.joinable()) audioThread.join();
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        std::cerr << "[VideoUpscaler] Python process launched, handle valid: "
                  << (processHandle != nullptr ? "yes" : "no") << std::endl;

        if (shouldStop.load()) {
            std::cerr << "[VideoUpscaler] Processing cancelled before encoding" << std::endl;
            if (audioThread.joinable()) audioThread.join();
            waitForPythonProcess();
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        // Start parallel HAP encoding - this will monitor the output directory
        // and encode frames as they appear from the Python upscaling process
        bool encodeSuccess = encodeFramesParallelHAP(framesOutputDir, outputPath,
                                                      videoInfo, videoInfo.frameCount);

        // Wait for Python process to finish (should already be done by this point)
        int pythonExitCode = waitForPythonProcess();
        if (pythonExitCode != 0 && lastError.empty()) {
            setError("Python upscaling process failed (exit code: " + std::to_string(pythonExitCode) + ")");
        }

        // Wait for audio extraction to complete
        if (audioThread.joinable()) {
            audioThread.join();
        }

        // Close stdout/stderr handles
#ifdef _WIN32
        if (stdoutReadHandle) {
            CloseHandle((HANDLE)stdoutReadHandle);
            stdoutReadHandle = nullptr;
        }
        if (stderrReadHandle) {
            CloseHandle((HANDLE)stderrReadHandle);
            stderrReadHandle = nullptr;
        }
#endif

        if (!encodeSuccess && lastError.empty()) {
            setError("HAP encoding failed");
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        if (!fs::exists(outputPath)) {
            setError("Output file not created");
            processing.store(false);
            cleanupTemp(jobTempDir);
            return;
        }

        // Success
        auto endTime = std::chrono::high_resolution_clock::now();
        float totalTime = std::chrono::duration<float>(endTime - startTime).count();

        std::cerr << "[VideoUpscaler] Processing complete!" << std::endl;
        std::cerr << "[VideoUpscaler] Total time: " << totalTime << " seconds" << std::endl;
        std::cerr << "[VideoUpscaler] Output: " << outputPath << std::endl;
        if (config.preserveAudio && audioSuccess) {
            std::cerr << "[VideoUpscaler] Audio: " << wavPath << std::endl;
        }

        updateProgress({videoInfo.frameCount, videoInfo.frameCount, 100.0f,
                       static_cast<float>(videoInfo.frameCount) / totalTime,
                       0.0f, "Complete!", "Done"});

    } catch (const std::exception& e) {
        setError(std::string("Processing exception: ") + e.what());
        std::cerr << "[VideoUpscaler] Exception: " << e.what() << std::endl;
    }

    // Cleanup temp files
    cleanupTemp(jobTempDir);

    processing.store(false);
    std::cerr << "[VideoUpscaler] Processing thread finished" << std::endl;
}

// ===========================================================================
// Frame Extraction
// ===========================================================================

bool VideoUpscaler::extractFrames(const std::string& videoPath,
                                   const std::string& outputDir,
                                   VideoInfo& info) {
    // Get video info first
    info = getVideoInfo(videoPath);
    if (info.width == 0 || info.height == 0) {
        setError("Failed to get video information");
        return false;
    }

    std::cerr << "[VideoUpscaler] Video: " << info.width << "x" << info.height
              << " @ " << info.fps << " fps, " << info.frameCount << " frames" << std::endl;

    // Open input video using libav
    AVFormatContext* inputCtx = nullptr;
    if (avformat_open_input(&inputCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        setError("Could not open video file");
        return false;
    }

    if (avformat_find_stream_info(inputCtx, nullptr) < 0) {
        setError("Could not find stream information");
        avformat_close_input(&inputCtx);
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < inputCtx->nb_streams; i++) {
        if (inputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIdx < 0) {
        setError("No video stream found");
        avformat_close_input(&inputCtx);
        return false;
    }

    AVStream* videoStream = inputCtx->streams[videoStreamIdx];
    AVCodecParameters* codecpar = videoStream->codecpar;

    // Find and open decoder
    const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        setError("Could not find video decoder");
        avformat_close_input(&inputCtx);
        return false;
    }

    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        setError("Could not open video decoder");
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    // Find PNG encoder
    const AVCodec* pngEncoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!pngEncoder) {
        setError("PNG encoder not found");
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    AVCodecContext* pngCtx = avcodec_alloc_context3(pngEncoder);
    pngCtx->width = codecpar->width;
    pngCtx->height = codecpar->height;
    pngCtx->pix_fmt = AV_PIX_FMT_RGB24;
    pngCtx->time_base = {1, 1};

    if (avcodec_open2(pngCtx, pngEncoder, nullptr) < 0) {
        setError("Could not open PNG encoder");
        avcodec_free_context(&pngCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    // Set up SwsContext for conversion to RGB24
    struct SwsContext* swsCtx = sws_getContext(
        codecpar->width, codecpar->height, static_cast<AVPixelFormat>(codecpar->format),
        codecpar->width, codecpar->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        setError("Could not create sws context");
        avcodec_free_context(&pngCtx);
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    // Allocate frames
    AVFrame* decFrame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = codecpar->width;
    rgbFrame->height = codecpar->height;
    av_frame_get_buffer(rgbFrame, 32);

    AVPacket* pkt = av_packet_alloc();
    AVPacket* pngPkt = av_packet_alloc();

    int frameCount = 0;
    std::cerr << "[VideoUpscaler] Extracting frames using libav API..." << std::endl;

    while (av_read_frame(inputCtx, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIdx) {
            if (avcodec_send_packet(decCtx, pkt) >= 0) {
                while (avcodec_receive_frame(decCtx, decFrame) >= 0) {
                    // Convert to RGB24
                    sws_scale(swsCtx, decFrame->data, decFrame->linesize,
                              0, decFrame->height, rgbFrame->data, rgbFrame->linesize);

                    rgbFrame->pts = frameCount;

                    // Encode to PNG
                    if (avcodec_send_frame(pngCtx, rgbFrame) >= 0) {
                        while (avcodec_receive_packet(pngCtx, pngPkt) >= 0) {
                            // Write PNG to file
                            char filename[512];
                            snprintf(filename, sizeof(filename), "%s/frame_%08d.png",
                                     outputDir.c_str(), frameCount + 1);
#ifdef _WIN32
                            for (char* c = filename; *c; c++) if (*c == '/') *c = '\\';
#endif
                            FILE* outFile = fopen(filename, "wb");
                            if (outFile) {
                                fwrite(pngPkt->data, 1, pngPkt->size, outFile);
                                fclose(outFile);
                            }
                            av_packet_unref(pngPkt);
                        }
                    }
                    frameCount++;

                    // Update progress periodically (0% during extraction phase)
                    if (frameCount % 100 == 0 || (info.frameCount > 0 && frameCount == info.frameCount)) {
                        updateProgress({frameCount, info.frameCount, 0.0f, 0.0f, 0.0f,
                                       "Extracting frames...", "Extracting"});
                    }
                }
            }
        }
        av_packet_unref(pkt);

        if (shouldStop.load()) break;
    }

    // Cleanup
    av_packet_free(&pngPkt);
    av_packet_free(&pkt);
    av_frame_free(&rgbFrame);
    av_frame_free(&decFrame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&pngCtx);
    avcodec_free_context(&decCtx);
    avformat_close_input(&inputCtx);

    if (frameCount == 0) {
        setError("No frames extracted from video");
        return false;
    }

    std::cerr << "[VideoUpscaler] Extracted " << frameCount << " frames" << std::endl;
    info.frameCount = frameCount;
    return true;
}

// ===========================================================================
// Python Upscaling
// ===========================================================================

bool VideoUpscaler::launchPythonUpscaling(const std::string& inputDir,
                                           const std::string& outputDir,
                                           Quality quality,
                                           ScaleFactor scaleFactor,
                                           const Config& config) {
    // Generate config JSON
    std::string configPath = inputDir + "/../config.json";
#ifdef _WIN32
    for (char& c : configPath) if (c == '/') c = '\\';
#endif

    if (!generateConfigJSON(configPath, inputDir, outputDir, quality, scaleFactor, config)) {
        return false;
    }

    // Get script path
    std::string scriptName = getScriptName(quality);
    std::string scriptPath = scriptsDir + "/" + scriptName;
#ifdef _WIN32
    for (char& c : scriptPath) if (c == '/') c = '\\';
#endif

    // Check if script exists
    if (!fs::exists(scriptPath)) {
        setError("Upscaling script not found: " + scriptPath);
        return false;
    }

    // Launch Python process
    std::string cmd = "\"" + pythonExecutable + "\" -u \"" + scriptPath +
                      "\" --config \"" + configPath + "\"";

    std::cerr << "[VideoUpscaler] Launching: " << cmd << std::endl;

#ifdef _WIN32
    // Set PYTHONIOENCODING to avoid Unicode encoding errors on Windows
    // FlashVSR prints ASCII art with special characters that cp1252 can't handle
    SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
    SetEnvironmentVariableA("PYTHONUTF8", "1");

    // Create pipes for stdout/stderr
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE stdoutWrite, stderrWrite;
    if (!CreatePipe((HANDLE*)&stdoutReadHandle, &stdoutWrite, &saAttr, 0) ||
        !SetHandleInformation((HANDLE)stdoutReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stdout pipe");
        return false;
    }

    if (!CreatePipe((HANDLE*)&stderrReadHandle, &stderrWrite, &saAttr, 0) ||
        !SetHandleInformation((HANDLE)stderrReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stderr pipe");
        CloseHandle((HANDLE)stdoutReadHandle);
        CloseHandle(stdoutWrite);
        return false;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to launch Python upscaling process");
        CloseHandle((HANDLE)stdoutReadHandle);
        CloseHandle(stdoutWrite);
        CloseHandle((HANDLE)stderrReadHandle);
        CloseHandle(stderrWrite);
        return false;
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    processHandle = pi.hProcess;
    CloseHandle(pi.hThread);

    // Monitor progress
    monitorProgress();

    return !shouldStop.load() && lastError.empty();
#else
    // Unix implementation
    int result = system(cmd.c_str());
    return (result == 0);
#endif
}

bool VideoUpscaler::generateConfigJSON(const std::string& configPath,
                                        const std::string& inputDir,
                                        const std::string& outputDir,
                                        Quality quality,
                                        ScaleFactor scaleFactor,
                                        const Config& config) {
    std::ofstream file(configPath);
    if (!file.is_open()) {
        setError("Failed to create config file: " + configPath);
        return false;
    }

    // Helper to prepare paths for JSON (forward slashes work on all platforms in Python)
    auto preparePathForJson = [](const std::string& str) -> std::string {
        std::string result;
        for (char c : str) {
            if (c == '"') {
                // Skip quotes
            } else if (c == '\\') {
                result += '/';
            } else {
                result += c;
            }
        }
        return result;
    };

    int scale = scaleFactorToInt(scaleFactor);
    bool isCleanup = (scaleFactor == ScaleFactor::CLEANUP);

    file << "{\n";
    file << "  \"input_dir\": \"" << preparePathForJson(inputDir) << "\",\n";
    file << "  \"output_dir\": \"" << preparePathForJson(outputDir) << "\",\n";
    file << "  \"model_name\": \"" << getModelName(quality) << "\",\n";
    file << "  \"scale_factor\": " << (isCleanup ? 2 : scale) << ",\n";
    file << "  \"cleanup_mode\": " << (isCleanup ? "true" : "false") << ",\n";
    file << "  \"use_gpu\": " << (config.useGPU ? "true" : "false") << ",\n";
    file << "  \"gpu_id\": " << config.gpuId << ",\n";
    file << "  \"tile_size\": " << config.tileSize << ",\n";
    file << "  \"temporal_radius\": " << config.temporalRadius << "\n";
    file << "}\n";

    file.close();
    std::cerr << "[VideoUpscaler] Config JSON created: " << configPath << std::endl;
    return true;
}

void VideoUpscaler::monitorProgress() {
#ifdef _WIN32
    char buffer[4096];
    DWORD bytesRead;

    while (!shouldStop.load()) {
        // Check if process is still running
        DWORD exitCode;
        if (GetExitCodeProcess((HANDLE)processHandle, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                std::cerr << "[VideoUpscaler] Process exited with code: " << exitCode << std::endl;
                if (exitCode != 0) {
                    setError("Upscaling process failed (exit code: " + std::to_string(exitCode) + ")");
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

                std::istringstream stream(output);
                std::string line;
                while (std::getline(stream, line)) {
                    parseProgressLine(line);
                    std::cerr << "[Upscaling] " << line << std::endl;
                }
            }
        }

        // Read stderr
        if (PeekNamedPipe((HANDLE)stderrReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (ReadFile((HANDLE)stderrReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                        &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string output(buffer);

                std::istringstream stream(output);
                std::string line;
                while (std::getline(stream, line)) {
                    std::cerr << "[Upscaling ERROR] " << line << std::endl;

                    // Check for CUDA/PyTorch out of memory errors
                    if (line.find("CUDA out of memory") != std::string::npos ||
                        line.find("OutOfMemoryError") != std::string::npos) {
                        setError("CUDA out of memory. Try reducing tile size or use CPU mode.");
                        shouldStop.store(true);
                    }
                }
            }
        }

        Sleep(100);
    }

    CloseHandle((HANDLE)stdoutReadHandle);
    CloseHandle((HANDLE)stderrReadHandle);
    stdoutReadHandle = nullptr;
    stderrReadHandle = nullptr;
#endif
}

bool VideoUpscaler::parseProgressLine(const std::string& line) {
    // Parse lines like: "[Frame 450/2000] fps: 12.34 ETA: 120.3s"
    //
    // FlashVSR outputs TWO types:
    // - [Inference X/Y] - Internal AI iterations (small numbers, causes fluctuation)
    // - [Frame X/Y] - Actual saved frames (cumulative, smooth progress)
    //
    // We ONLY use [Frame] lines for progress to avoid bar fluctuation.

    std::regex frameRegex("\\[Frame\\s+(\\d+)/(\\d+)\\]");
    std::smatch frameMatch;

    if (std::regex_search(line, frameMatch, frameRegex)) {
        int currentFrame = std::stoi(frameMatch[1].str());
        int totalFrames = std::stoi(frameMatch[2].str());

        Progress newProgress;
        newProgress.currentFrame = currentFrame;
        newProgress.totalFrames = totalFrames;

        float percentComplete = (totalFrames > 0) ?
            (static_cast<float>(currentFrame) / totalFrames * 100.0f) : 0.0f;

        newProgress.status = "Upscaling...";
        newProgress.currentStage = "Upscaling";

        // Never let progress go backwards
        if (percentComplete > highWaterMarkPercent) {
            highWaterMarkPercent = percentComplete;
        }
        newProgress.percentComplete = highWaterMarkPercent;

        // Parse FPS if present
        std::regex fpsRegex("fps:\\s*([0-9.]+)");
        std::smatch fpsMatch;
        if (std::regex_search(line, fpsMatch, fpsRegex)) {
            newProgress.fps = std::stof(fpsMatch[1].str());
        }

        // Parse ETA if present
        std::regex etaRegex("ETA:\\s*([0-9.]+)s");
        std::smatch etaMatch;
        if (std::regex_search(line, etaMatch, etaRegex)) {
            newProgress.estimatedTimeRemaining = std::stof(etaMatch[1].str());
        }

        updateProgress(newProgress);
        return true;
    }

    return false;
}

// ===========================================================================
// Helper Methods
// ===========================================================================

bool VideoUpscaler::deployScripts() {
    // Check if EDVR script exists
    std::string edvrScript = scriptsDir + "/upscale_edvr.py";
    std::string flashvsrScript = scriptsDir + "/upscale_flashvsr.py";

#ifdef _WIN32
    for (char& c : edvrScript) if (c == '/') c = '\\';
    for (char& c : flashvsrScript) if (c == '/') c = '\\';
#endif

    if (fs::exists(edvrScript) && fs::exists(flashvsrScript)) {
        std::cerr << "[VideoUpscaler] Upscaling scripts found" << std::endl;
        return true;
    }

    // Scripts not found - warn user
    std::cerr << "[VideoUpscaler] WARNING: Upscaling scripts not found at " << scriptsDir << std::endl;
    std::cerr << "[VideoUpscaler] Please deploy upscale_edvr.py and upscale_flashvsr.py" << std::endl;

    return true; // Don't fail, user will deploy manually
}

void VideoUpscaler::cleanupTemp(const std::string& tempDirPath) {
    try {
        if (fs::exists(tempDirPath)) {
            fs::remove_all(tempDirPath);
            std::cerr << "[VideoUpscaler] Cleaned up temp directory: " << tempDirPath << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[VideoUpscaler] Warning: Failed to clean up temp dir: " << e.what() << std::endl;
    }
}

void VideoUpscaler::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
}

void VideoUpscaler::updateProgress(const Progress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
}

void VideoUpscaler::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ===========================================================================
// Parallel HAP Encoding
// ===========================================================================

bool VideoUpscaler::isPythonProcessRunning() {
#ifdef _WIN32
    if (!processHandle) return false;
    DWORD exitCode;
    if (GetExitCodeProcess((HANDLE)processHandle, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
#else
    if (childPid <= 0) return false;
    int status;
    pid_t result = waitpid(childPid, &status, WNOHANG);
    return (result == 0);
#endif
}

int VideoUpscaler::waitForPythonProcess() {
#ifdef _WIN32
    if (!processHandle) return 0;  // Already waited/cleaned up - not an error
    WaitForSingleObject((HANDLE)processHandle, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess((HANDLE)processHandle, &exitCode);
    CloseHandle((HANDLE)processHandle);
    processHandle = nullptr;
    return static_cast<int>(exitCode);
#else
    if (childPid <= 0) return 0;  // Already waited/cleaned up - not an error
    int status;
    waitpid(childPid, &status, 0);
    childPid = -1;
    return WEXITSTATUS(status);
#endif
}

bool VideoUpscaler::extractAudioToWav(const std::string& inputPath,
                                       const std::string& wavPath) {
    std::cerr << "[VideoUpscaler] Extracting audio to WAV: " << wavPath << std::endl;

    AVFormatContext* inputCtx = nullptr;
    if (avformat_open_input(&inputCtx, inputPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[VideoUpscaler] Could not open input for audio extraction" << std::endl;
        return false;
    }

    if (avformat_find_stream_info(inputCtx, nullptr) < 0) {
        avformat_close_input(&inputCtx);
        return false;
    }

    // Find audio stream
    int audioStreamIdx = -1;
    for (unsigned int i = 0; i < inputCtx->nb_streams; i++) {
        if (inputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIdx = static_cast<int>(i);
            break;
        }
    }

    if (audioStreamIdx < 0) {
        std::cerr << "[VideoUpscaler] No audio stream found" << std::endl;
        avformat_close_input(&inputCtx);
        return false;
    }

    AVStream* audioStream = inputCtx->streams[audioStreamIdx];
    AVCodecParameters* codecpar = audioStream->codecpar;

    // Open audio decoder
    const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        avformat_close_input(&inputCtx);
        return false;
    }

    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    // Open WAV output file
    FILE* wavFile = fopen(wavPath.c_str(), "wb");
    if (!wavFile) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&inputCtx);
        return false;
    }

    // Write WAV header (will update at end)
    int numChannels = codecpar->ch_layout.nb_channels;
    int sampleRate = codecpar->sample_rate;
    uint8_t wavHeader[44] = {0};
    memcpy(wavHeader, "RIFF", 4);
    memcpy(wavHeader + 8, "WAVE", 4);
    memcpy(wavHeader + 12, "fmt ", 4);
    *(uint32_t*)(wavHeader + 16) = 16;              // PCM chunk size
    *(uint16_t*)(wavHeader + 20) = 1;               // PCM format
    *(uint16_t*)(wavHeader + 22) = static_cast<uint16_t>(numChannels);
    *(uint32_t*)(wavHeader + 24) = static_cast<uint32_t>(sampleRate);
    *(uint32_t*)(wavHeader + 28) = static_cast<uint32_t>(sampleRate * numChannels * 2);
    *(uint16_t*)(wavHeader + 32) = static_cast<uint16_t>(numChannels * 2);
    *(uint16_t*)(wavHeader + 34) = 16;              // 16 bits per sample
    memcpy(wavHeader + 36, "data", 4);
    fwrite(wavHeader, 1, 44, wavFile);

    // Decode and write audio samples
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int64_t samplesWritten = 0;

    while (av_read_frame(inputCtx, pkt) >= 0) {
        if (pkt->stream_index == audioStreamIdx) {
            if (avcodec_send_packet(decCtx, pkt) >= 0) {
                while (avcodec_receive_frame(decCtx, frame) >= 0) {
                    int samplesPerChannel = frame->nb_samples;
                    int channels = frame->ch_layout.nb_channels;

                    for (int i = 0; i < samplesPerChannel; i++) {
                        for (int ch = 0; ch < channels; ch++) {
                            int16_t sample = 0;

                            switch (frame->format) {
                                case AV_SAMPLE_FMT_S16:
                                case AV_SAMPLE_FMT_S16P: {
                                    int16_t* data = (int16_t*)frame->data[frame->format == AV_SAMPLE_FMT_S16P ? ch : 0];
                                    sample = data[frame->format == AV_SAMPLE_FMT_S16P ? i : i * channels + ch];
                                    break;
                                }
                                case AV_SAMPLE_FMT_FLT:
                                case AV_SAMPLE_FMT_FLTP: {
                                    float* data = (float*)frame->data[frame->format == AV_SAMPLE_FMT_FLTP ? ch : 0];
                                    float fval = data[frame->format == AV_SAMPLE_FMT_FLTP ? i : i * channels + ch];
                                    sample = static_cast<int16_t>(fval * 32767.0f);
                                    break;
                                }
                                case AV_SAMPLE_FMT_S32:
                                case AV_SAMPLE_FMT_S32P: {
                                    int32_t* data = (int32_t*)frame->data[frame->format == AV_SAMPLE_FMT_S32P ? ch : 0];
                                    int32_t ival = data[frame->format == AV_SAMPLE_FMT_S32P ? i : i * channels + ch];
                                    sample = static_cast<int16_t>(ival >> 16);
                                    break;
                                }
                                default:
                                    sample = 0;
                            }

                            fwrite(&sample, sizeof(int16_t), 1, wavFile);
                            samplesWritten++;
                        }
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Update WAV header with correct sizes
    uint32_t fileSize = static_cast<uint32_t>(44 + samplesWritten * 2 - 8);
    uint32_t dataSize = static_cast<uint32_t>(samplesWritten * 2);
    fseek(wavFile, 4, SEEK_SET);
    fwrite(&fileSize, sizeof(uint32_t), 1, wavFile);
    fseek(wavFile, 40, SEEK_SET);
    fwrite(&dataSize, sizeof(uint32_t), 1, wavFile);
    fclose(wavFile);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&decCtx);
    avformat_close_input(&inputCtx);

    std::cerr << "[VideoUpscaler] Audio extracted successfully" << std::endl;
    return true;
}

bool VideoUpscaler::launchPythonUpscalingAsync(const std::string& inputDir,
                                                const std::string& outputDir,
                                                Quality quality,
                                                ScaleFactor scaleFactor,
                                                const Config& config) {
    // Generate config JSON
    std::string configPath = inputDir + "/../config.json";
#ifdef _WIN32
    for (char& c : configPath) if (c == '/') c = '\\';
#endif

    if (!generateConfigJSON(configPath, inputDir, outputDir, quality, scaleFactor, config)) {
        return false;
    }

    // Get script path
    std::string scriptName = getScriptName(quality);
    std::string scriptPath = scriptsDir + "/" + scriptName;
#ifdef _WIN32
    for (char& c : scriptPath) if (c == '/') c = '\\';
#endif

    if (!fs::exists(scriptPath)) {
        setError("Upscaling script not found: " + scriptPath);
        return false;
    }

    std::string cmd = "\"" + pythonExecutable + "\" -u \"" + scriptPath +
                      "\" --config \"" + configPath + "\"";

    std::cerr << "[VideoUpscaler] Launching async: " << cmd << std::endl;

#ifdef _WIN32
    // Set PYTHONIOENCODING to avoid Unicode encoding errors on Windows
    // FlashVSR prints ASCII art with special characters that cp1252 can't handle
    SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
    SetEnvironmentVariableA("PYTHONUTF8", "1");
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE stdoutWrite, stderrWrite;
    if (!CreatePipe((HANDLE*)&stdoutReadHandle, &stdoutWrite, &saAttr, 0) ||
        !SetHandleInformation((HANDLE)stdoutReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stdout pipe");
        return false;
    }

    if (!CreatePipe((HANDLE*)&stderrReadHandle, &stderrWrite, &saAttr, 0) ||
        !SetHandleInformation((HANDLE)stderrReadHandle, HANDLE_FLAG_INHERIT, 0)) {
        setError("Failed to create stderr pipe");
        CloseHandle((HANDLE)stdoutReadHandle);
        CloseHandle(stdoutWrite);
        return false;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to launch Python upscaling process");
        CloseHandle((HANDLE)stdoutReadHandle);
        CloseHandle(stdoutWrite);
        CloseHandle((HANDLE)stderrReadHandle);
        CloseHandle(stderrWrite);
        return false;
    }

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    processHandle = pi.hProcess;
    CloseHandle(pi.hThread);

    return true;
#else
    // Unix: use fork/exec
    int stdoutPipes[2], stderrPipes[2];
    pipe(stdoutPipes);
    pipe(stderrPipes);

    childPid = fork();
    if (childPid == 0) {
        // Child process
        close(stdoutPipes[0]);
        close(stderrPipes[0]);
        dup2(stdoutPipes[1], STDOUT_FILENO);
        dup2(stderrPipes[1], STDERR_FILENO);
        close(stdoutPipes[1]);
        close(stderrPipes[1]);

        execlp(pythonExecutable.c_str(), pythonExecutable.c_str(),
               "-u", scriptPath.c_str(), "--config", configPath.c_str(), NULL);
        _exit(1);
    }

    close(stdoutPipes[1]);
    close(stderrPipes[1]);
    stdoutPipe[0] = stdoutPipes[0];
    stderrPipe[0] = stderrPipes[0];

    return true;
#endif
}

bool VideoUpscaler::encodeFramesParallelHAP(const std::string& framesOutputDir,
                                             const std::string& outputPath,
                                             const VideoInfo& info,
                                             int totalFrames) {
    std::cerr << "[VideoUpscaler] Starting parallel HAP encoding for " << totalFrames << " frames" << std::endl;

    // Set up HAP encoder
    const AVCodec* hapCodec = avcodec_find_encoder_by_name("hap");
    if (!hapCodec) {
        setError("HAP codec not found - FFmpeg may be compiled without snappy support");
        return false;
    }

    // Determine output frame dimensions from first available frame
    int frameWidth = 0, frameHeight = 0;

    // Wait for at least one frame to appear
    // FlashVSR processes all frames as a batch before outputting, so this can take a long time
    std::string firstFramePath;
    int waitCount = 0;
    int maxWaitWithoutPython = 300;  // 30 seconds after Python finishes
    int waitAfterPythonDone = 0;

    while (!shouldStop.load()) {
        char frameName[64];
        snprintf(frameName, sizeof(frameName), "frame_%08d.png", 1);
        std::string framePath = framesOutputDir + "/" + frameName;
#ifdef _WIN32
        for (char& c : framePath) if (c == '/') c = '\\';
#endif
        if (fs::exists(framePath)) {
            firstFramePath = framePath;
            break;
        }

        // Read Python stdout for progress updates while waiting
#ifdef _WIN32
        if (stdoutReadHandle) {
            DWORD available;
            if (PeekNamedPipe((HANDLE)stdoutReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
                char buffer[4096];
                DWORD bytesRead;
                if (ReadFile((HANDLE)stdoutReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                            &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    std::string output(buffer);
                    std::istringstream stream(output);
                    std::string line;
                    while (std::getline(stream, line)) {
                        parseProgressLine(line);
                        std::cerr << "[Upscaling] " << line << std::endl;
                    }
                }
            }
        }
        // Also read stderr for error messages
        if (stderrReadHandle) {
            DWORD available;
            if (PeekNamedPipe((HANDLE)stderrReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
                char buffer[4096];
                DWORD bytesRead;
                if (ReadFile((HANDLE)stderrReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                            &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    std::string output(buffer);
                    std::istringstream stream(output);
                    std::string line;
                    while (std::getline(stream, line)) {
                        std::cerr << "[Upscaling ERROR] " << line << std::endl;
                    }
                }
            }
        }
#endif

        // Check if Python is still running
        bool pythonRunning = isPythonProcessRunning();

        if (!pythonRunning) {
            // Python finished - drain remaining output
            if (waitAfterPythonDone == 0) {
                std::cerr << "[VideoUpscaler] Python process ended, draining remaining output..." << std::endl;
                // Give a moment for buffers to flush
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                // Read any remaining stdout
                if (stdoutReadHandle) {
                    DWORD available;
                    while (PeekNamedPipe((HANDLE)stdoutReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
                        char buffer[4096];
                        DWORD bytesRead;
                        if (ReadFile((HANDLE)stdoutReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                                    &bytesRead, NULL) && bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            std::cerr << "[Upscaling FINAL] " << buffer << std::endl;
                        } else break;
                    }
                }
                // Read any remaining stderr
                if (stderrReadHandle) {
                    DWORD available;
                    while (PeekNamedPipe((HANDLE)stderrReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
                        char buffer[4096];
                        DWORD bytesRead;
                        if (ReadFile((HANDLE)stderrReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                                    &bytesRead, NULL) && bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            std::cerr << "[Upscaling FINAL ERROR] " << buffer << std::endl;
                        } else break;
                    }
                }
            }
            waitAfterPythonDone++;
            if (waitAfterPythonDone > maxWaitWithoutPython) {
                std::cerr << "[VideoUpscaler] Python finished but no frames appeared" << std::endl;
                break;
            }
        } else {
            // Python still running - reset the post-python counter
            waitAfterPythonDone = 0;
        }

        // Log progress periodically (only if no Python output recently)
        if (waitCount % 100 == 0) {
            std::cerr << "[VideoUpscaler] Waiting for first upscaled frame... (Python "
                      << (pythonRunning ? "running" : "finished") << ")" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }

    if (firstFramePath.empty()) {
        setError("Timeout waiting for first upscaled frame - Python may have failed");
        return false;
    }

    // Wait for first frame to be fully written (check file size stability)
    std::cerr << "[VideoUpscaler] Waiting for first frame to be fully written..." << std::endl;
    uintmax_t firstFrameLastSize = 0;
    int firstFrameStableCount = 0;
    for (int waitIter = 0; waitIter < 100 && firstFrameStableCount < 5; waitIter++) {
        try {
            uintmax_t currentSize = fs::file_size(firstFramePath);
            if (currentSize == firstFrameLastSize && currentSize > 1000) {  // At least 1KB
                firstFrameStableCount++;
            } else {
                firstFrameStableCount = 0;
                firstFrameLastSize = currentSize;
            }
        } catch (...) {
            firstFrameStableCount = 0;
        }
        if (firstFrameStableCount < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::cerr << "[VideoUpscaler] First frame size stable at " << firstFrameLastSize << " bytes" << std::endl;
    std::cerr << "[VideoUpscaler] Probing dimensions from: " << firstFramePath << std::endl;

    // Get frame dimensions from first frame using libav
    AVFormatContext* probeCtx = nullptr;
    const AVInputFormat* probeFormat = av_find_input_format("image2");
    int probeResult = avformat_open_input(&probeCtx, firstFramePath.c_str(), probeFormat, nullptr);
    if (probeResult >= 0) {
        if (avformat_find_stream_info(probeCtx, nullptr) >= 0) {
            std::cerr << "[VideoUpscaler] Probe found " << probeCtx->nb_streams << " streams" << std::endl;
            for (unsigned int i = 0; i < probeCtx->nb_streams; i++) {
                if (probeCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    frameWidth = probeCtx->streams[i]->codecpar->width;
                    frameHeight = probeCtx->streams[i]->codecpar->height;
                    std::cerr << "[VideoUpscaler] Probed dimensions: " << frameWidth << "x" << frameHeight << std::endl;
                    break;
                }
            }
        } else {
            std::cerr << "[VideoUpscaler] WARNING: Could not find stream info" << std::endl;
        }
        avformat_close_input(&probeCtx);
    } else {
        char errBuf[256];
        av_strerror(probeResult, errBuf, sizeof(errBuf));
        std::cerr << "[VideoUpscaler] WARNING: Could not open frame for probing: " << errBuf << std::endl;
    }

    if (frameWidth == 0 || frameHeight == 0 || frameWidth < 64 || frameHeight < 64) {
        std::cerr << "[VideoUpscaler] WARNING: Invalid dimensions (" << frameWidth << "x" << frameHeight
                  << "), falling back to original * scale" << std::endl;
        // Fallback to original dimensions * 4 (or 2 for 2x scale)
        int scaleFactor = 4;  // Default, could be passed in
        frameWidth = info.width * scaleFactor;
        frameHeight = info.height * scaleFactor;
        std::cerr << "[VideoUpscaler] Fallback dimensions: " << frameWidth << "x" << frameHeight << std::endl;
    }

    std::cerr << "[VideoUpscaler] Upscaled frame size: " << frameWidth << "x" << frameHeight << std::endl;

    // Apply HAP alignment (width % 32 == 0, height % 4 == 0)
    int alignedWidth = frameWidth;
    int alignedHeight = frameHeight;
    if (alignedWidth % 32 != 0) {
        alignedWidth += (32 - alignedWidth % 32);
    }
    if (alignedHeight % 4 != 0) {
        alignedHeight += (4 - alignedHeight % 4);
    }

    std::cerr << "[VideoUpscaler] HAP aligned size: " << alignedWidth << "x" << alignedHeight << std::endl;

    // Set up encoder context
    AVCodecContext* encCtx = avcodec_alloc_context3(hapCodec);
    encCtx->width = alignedWidth;
    encCtx->height = alignedHeight;
    encCtx->time_base = {1, 1000};
    encCtx->pix_fmt = hapCodec->pix_fmts[0];  // Usually RGBA
    encCtx->sample_aspect_ratio = {1, 1};

    if (avcodec_open2(encCtx, hapCodec, nullptr) < 0) {
        setError("Failed to open HAP encoder");
        avcodec_free_context(&encCtx);
        return false;
    }

    // Set up output format context
    AVFormatContext* outputCtx = nullptr;
    avformat_alloc_output_context2(&outputCtx, av_guess_format("mov", nullptr, "video/mov"),
                                    nullptr, outputPath.c_str());
    if (!outputCtx) {
        setError("Failed to create output format context");
        avcodec_free_context(&encCtx);
        return false;
    }

    AVStream* outStream = avformat_new_stream(outputCtx, hapCodec);
    outStream->time_base = {1, static_cast<int>(info.fps * 1000)};
    outStream->r_frame_rate = {static_cast<int>(info.fps * 1000), 1000};
    avcodec_parameters_from_context(outStream->codecpar, encCtx);

    if (avio_open(&outputCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        setError("Failed to open output file");
        avformat_free_context(outputCtx);
        avcodec_free_context(&encCtx);
        return false;
    }

    if (avformat_write_header(outputCtx, nullptr) < 0) {
        setError("Failed to write header");
        avio_close(outputCtx->pb);
        avformat_free_context(outputCtx);
        avcodec_free_context(&encCtx);
        return false;
    }

    // Prepare SwsContext for scaling/conversion
    struct SwsContext* swsCtx = sws_getContext(
        frameWidth, frameHeight, AV_PIX_FMT_RGBA,
        alignedWidth, alignedHeight, encCtx->pix_fmt,
        SWS_LANCZOS, nullptr, nullptr, nullptr);

    // Prepare frames
    AVFrame* inputFrame = av_frame_alloc();
    inputFrame->format = AV_PIX_FMT_RGBA;
    inputFrame->width = frameWidth;
    inputFrame->height = frameHeight;
    av_frame_get_buffer(inputFrame, 32);

    AVFrame* encFrame = av_frame_alloc();
    encFrame->format = encCtx->pix_fmt;
    encFrame->width = alignedWidth;
    encFrame->height = alignedHeight;

    AVPacket* pkt = av_packet_alloc();

    // Process frames as they appear
    int framesEncoded = 0;
    auto encodeStartTime = std::chrono::high_resolution_clock::now();
    std::vector<float> frameTimes;
    bool pythonFinished = false;  // Track if we've already confirmed Python finished
    int pythonExitCode = 0;
    int consecutiveMissingFrames = 0;  // Track consecutive missing frames after Python finished

    // Reset progress for HAP encoding phase (appears as "second pass" to user)
    highWaterMarkPercent = 0.0f;
    updateProgress({0, totalFrames, 0.0f, 0.0f, 0.0f, "Encoding HAP...", "Encoding"});
    std::cerr << "[VideoUpscaler] Starting HAP encoding: " << totalFrames << " frames" << std::endl;

    while (framesEncoded < totalFrames && !shouldStop.load()) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        int nextFrame = framesEncoded + 1;
        char frameName[64];
        snprintf(frameName, sizeof(frameName), "frame_%08d.png", nextFrame);
        std::string framePath = framesOutputDir + "/" + frameName;
#ifdef _WIN32
        for (char& c : framePath) if (c == '/') c = '\\';
#endif

        // Wait for frame to exist
        int frameWaitCount = 0;
        while (!fs::exists(framePath) && frameWaitCount < 1200 && !shouldStop.load()) {
            // Read Python output while waiting (but don't update progress - we're in HAP encoding phase)
#ifdef _WIN32
            if (stdoutReadHandle) {
                DWORD available;
                if (PeekNamedPipe((HANDLE)stdoutReadHandle, NULL, 0, NULL, &available, NULL) && available > 0) {
                    char buffer[4096];
                    DWORD bytesRead;
                    if (ReadFile((HANDLE)stdoutReadHandle, buffer, std::min(available, (DWORD)sizeof(buffer) - 1),
                                &bytesRead, NULL) && bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        std::string output(buffer);
                        std::istringstream stream(output);
                        std::string line;
                        while (std::getline(stream, line)) {
                            // Don't call parseProgressLine - we're showing HAP encoding progress now
                            std::cerr << "[Upscaling] " << line << std::endl;
                        }
                    }
                }
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            frameWaitCount++;
        }

        if (shouldStop.load()) break;

        if (!fs::exists(framePath)) {
            // Check if Python process died (only check once)
            if (!pythonFinished && !isPythonProcessRunning()) {
                pythonExitCode = waitForPythonProcess();
                pythonFinished = true;
                std::cerr << "[VideoUpscaler] Python process finished with exit code: " << pythonExitCode << std::endl;
                if (pythonExitCode != 0) {
                    setError("Python upscaling process failed (exit code: " + std::to_string(pythonExitCode) + ")");
                    break;
                }
                // Python finished successfully - scan directory to see what frames exist
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                int existingFrameCount = 0;
                std::string firstMissing, lastFound;
                for (int i = 1; i <= totalFrames; i++) {
                    char checkName[64];
                    snprintf(checkName, sizeof(checkName), "frame_%08d.png", i);
                    std::string checkPath = framesOutputDir + "/" + checkName;
#ifdef _WIN32
                    for (char& c : checkPath) if (c == '/') c = '\\';
#endif
                    if (fs::exists(checkPath)) {
                        existingFrameCount++;
                        lastFound = checkName;
                    } else if (firstMissing.empty()) {
                        firstMissing = checkName;
                    }
                }
                std::cerr << "[VideoUpscaler] Directory scan: " << existingFrameCount << "/" << totalFrames
                          << " frames exist" << std::endl;
                if (!firstMissing.empty()) {
                    std::cerr << "[VideoUpscaler] First missing: " << firstMissing << std::endl;
                }
                if (!lastFound.empty()) {
                    std::cerr << "[VideoUpscaler] Last found: " << lastFound << std::endl;
                }
                // List first few files in directory for debugging
                std::cerr << "[VideoUpscaler] Output directory: " << framesOutputDir << std::endl;
                int fileCount = 0;
                try {
                    for (const auto& entry : fs::directory_iterator(framesOutputDir)) {
                        if (fileCount < 5) {
                            std::cerr << "[VideoUpscaler]   File: " << entry.path().filename().string() << std::endl;
                        }
                        fileCount++;
                    }
                    std::cerr << "[VideoUpscaler] Total files in directory: " << fileCount << std::endl;
                } catch (...) {
                    std::cerr << "[VideoUpscaler] Could not list directory" << std::endl;
                }
            }
            // Frame might be the last one, check if we're done
            if (framesEncoded >= totalFrames - 1) break;

            // If Python is done but frame still doesn't exist, we have a problem
            if (pythonFinished) {
                consecutiveMissingFrames++;
                if (consecutiveMissingFrames <= 3) {
                    std::cerr << "[VideoUpscaler] WARNING: Frame " << nextFrame << " not found after Python finished" << std::endl;
                }
                // After 10 consecutive missing frames, give up
                if (consecutiveMissingFrames >= 10) {
                    std::cerr << "[VideoUpscaler] ERROR: Too many consecutive missing frames, stopping" << std::endl;
                    setError("Too many frames missing from Python output");
                    break;
                }
                // Skip this frame number and try the next
                framesEncoded++;  // Move to next frame
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            continue;
        }

        // Frame exists - reset missing counter
        consecutiveMissingFrames = 0;

        // Wait for file to be fully written (check file size stability)
        uintmax_t lastSize = 0;
        int stableCount = 0;
        for (int waitIter = 0; waitIter < 50 && stableCount < 3; waitIter++) {
            try {
                uintmax_t currentSize = fs::file_size(framePath);
                if (currentSize == lastSize && currentSize > 0) {
                    stableCount++;
                } else {
                    stableCount = 0;
                    lastSize = currentSize;
                }
            } catch (...) {
                stableCount = 0;
            }
            if (stableCount < 3) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }

        // Suppress libav warnings during PNG reading
        int oldLogLevel = av_log_get_level();
        av_log_set_level(AV_LOG_ERROR);

        // Load PNG frame using libav with image2 format
        AVFormatContext* imgCtx = nullptr;
        const AVInputFormat* imgFormat = av_find_input_format("image2");

        bool frameLoaded = false;
        int retryCount = 0;
        const int maxRetries = 3;

        while (!frameLoaded && retryCount < maxRetries && !shouldStop.load()) {
            imgCtx = nullptr;
            if (avformat_open_input(&imgCtx, framePath.c_str(), imgFormat, nullptr) >= 0) {
                if (avformat_find_stream_info(imgCtx, nullptr) >= 0 && imgCtx->nb_streams > 0) {
                    AVStream* imgStream = imgCtx->streams[0];

                    // Find PNG decoder directly
                    const AVCodec* imgDecoder = avcodec_find_decoder(AV_CODEC_ID_PNG);
                    if (imgDecoder) {
                        AVCodecContext* imgDecCtx = avcodec_alloc_context3(imgDecoder);
                        avcodec_parameters_to_context(imgDecCtx, imgStream->codecpar);

                        if (avcodec_open2(imgDecCtx, imgDecoder, nullptr) >= 0) {
                            AVPacket* imgPkt = av_packet_alloc();
                            AVFrame* decodedFrame = av_frame_alloc();

                            if (av_read_frame(imgCtx, imgPkt) >= 0) {
                                if (avcodec_send_packet(imgDecCtx, imgPkt) >= 0) {
                                    if (avcodec_receive_frame(imgDecCtx, decodedFrame) >= 0) {
                                        // Convert to HAP format
                                        struct SwsContext* imgSwsCtx = sws_getContext(
                                            decodedFrame->width, decodedFrame->height,
                                            (AVPixelFormat)decodedFrame->format,
                                            alignedWidth, alignedHeight, encCtx->pix_fmt,
                                            SWS_LANCZOS, nullptr, nullptr, nullptr);

                                        if (imgSwsCtx) {
                                            av_image_alloc(encFrame->data, encFrame->linesize,
                                                           alignedWidth, alignedHeight, encCtx->pix_fmt, 32);

                                            sws_scale(imgSwsCtx, decodedFrame->data, decodedFrame->linesize,
                                                      0, decodedFrame->height, encFrame->data, encFrame->linesize);

                                            // Set PTS
                                            int64_t pts = static_cast<int64_t>(framesEncoded) * 1000 / static_cast<int64_t>(info.fps);
                                            encFrame->pts = pts;

                                            // Encode frame
                                            if (avcodec_send_frame(encCtx, encFrame) >= 0) {
                                                while (avcodec_receive_packet(encCtx, pkt) >= 0) {
                                                    av_packet_rescale_ts(pkt, encCtx->time_base, outStream->time_base);
                                                    pkt->stream_index = 0;
                                                    av_interleaved_write_frame(outputCtx, pkt);
                                                    av_packet_unref(pkt);
                                                }
                                            }

                                            av_freep(&encFrame->data[0]);
                                            sws_freeContext(imgSwsCtx);
                                            framesEncoded++;
                                            frameLoaded = true;  // Success!
                                        }
                                    }
                                }
                                av_packet_unref(imgPkt);
                            }

                            av_frame_free(&decodedFrame);
                            av_packet_free(&imgPkt);
                        }
                        avcodec_free_context(&imgDecCtx);
                    }
                }
                avformat_close_input(&imgCtx);
            }

            if (!frameLoaded) {
                retryCount++;
                if (retryCount < maxRetries) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }

        // Restore log level
        av_log_set_level(oldLogLevel);

        // Calculate timing and update progress
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
        frameTimes.push_back(frameTime);

        // Rolling average of last 100 frames
        float avgTime = 0;
        int count = 0;
        for (int i = std::max(0, (int)frameTimes.size() - 100); i < (int)frameTimes.size(); i++) {
            avgTime += frameTimes[i];
            count++;
        }
        avgTime /= count;

        float fps = avgTime > 0 ? 1.0f / avgTime : 0;
        float eta = (totalFrames - framesEncoded) * avgTime;
        float percent = static_cast<float>(framesEncoded) / totalFrames * 100.0f;

        updateProgress({framesEncoded, totalFrames, percent, fps, eta, "Encoding HAP...", "Encoding"});

        // Console output every 100 frames or at completion
        if (framesEncoded % 100 == 0 || framesEncoded == totalFrames) {
            std::cerr << "[VideoUpscaler] [HAP " << framesEncoded << "/" << totalFrames
                      << "] " << std::fixed << std::setprecision(1) << percent << "% fps: "
                      << std::setprecision(2) << fps << " ETA: " << std::setprecision(1) << eta << "s" << std::endl;
        }
    }

    // Flush encoder
    avcodec_send_frame(encCtx, nullptr);
    while (avcodec_receive_packet(encCtx, pkt) >= 0) {
        av_packet_rescale_ts(pkt, encCtx->time_base, outStream->time_base);
        pkt->stream_index = 0;
        av_interleaved_write_frame(outputCtx, pkt);
        av_packet_unref(pkt);
    }

    // Write trailer and cleanup
    av_write_trailer(outputCtx);
    avio_close(outputCtx->pb);

    av_packet_free(&pkt);
    av_frame_free(&inputFrame);
    av_frame_free(&encFrame);
    if (swsCtx) sws_freeContext(swsCtx);
    avformat_free_context(outputCtx);
    avcodec_free_context(&encCtx);

    std::cerr << "[VideoUpscaler] HAP encoding complete: " << framesEncoded << " frames" << std::endl;

    return framesEncoded == totalFrames;
}
