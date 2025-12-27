/**
 * VideoUpscaler.h
 *
 * Offline Video Upscaling using PyTorch-based models
 * Supports EDVR and FlashVSR for high-quality video super-resolution
 *
 * Quality Modes:
 * - FAST: EDVR (standard model, fastest)
 * - BALANCED: EDVR_HQ (larger model, better quality)
 * - ULTRA: FlashVSR (diffusion-based with TCDecoder, best quality for consumer GPUs)
 *
 * Scale Factors:
 * - 2x, 3x, 4x upscaling supported
 * - CLEANUP mode: 2x upscale + Lanczos3 downscale (same resolution, enhanced)
 *
 * License: GPL3
 */

#ifndef VIDEO_UPSCALER_H
#define VIDEO_UPSCALER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

/**
 * Configuration options for video upscaling
 * Defined outside class to allow use as default argument
 */
struct VideoUpscalerConfig {
    bool useGPU = true;             // Use CUDA acceleration
    int gpuId = 0;                  // GPU device ID
    int tileSize = 0;               // Tile size for processing (0 = auto)
    int temporalRadius = 2;         // Frames before/after for temporal info (EDVR uses 5 frames total)
    bool preserveAudio = true;      // Copy audio stream to output
    std::string outputFormat = "";  // Output format (empty = same as input)
    std::string outputCodec = "";   // Output codec (empty = auto select)
    int outputCRF = 18;             // Output quality (lower = better, 0-51)
};

/**
 * VideoUpscaler - Offline video upscaling using EDVR and FlashVSR
 *
 * Usage:
 *   VideoUpscaler upscaler;
 *   if (upscaler.initialize()) {
 *       std::string outputPath = upscaler.upscale(
 *           "input.mp4",
 *           VideoUpscaler::Quality::BALANCED,
 *           VideoUpscaler::ScaleFactor::X4
 *       );
 *       while (upscaler.isProcessing()) {
 *           auto progress = upscaler.getProgress();
 *           // Update UI with progress.percentComplete
 *       }
 *   }
 */
class VideoUpscaler {
public:
    /**
     * Quality presets - determines which model to use
     */
    enum class Quality {
        FAST,       // EDVR standard - fastest, good quality
        BALANCED,   // EDVR_HQ - better quality, slower
        ULTRA       // FlashVSR with TCDecoder - diffusion-based, best quality
    };

    /**
     * Scale factor for upscaling
     */
    enum class ScaleFactor {
        CLEANUP,    // Same resolution (2x up, Lanczos3 down) - quality enhancement
        X2,         // 2x upscaling
        X3,         // 3x upscaling
        X4          // 4x upscaling (native for EDVR)
    };

    /**
     * Processing progress information
     */
    struct Progress {
        int currentFrame = 0;
        int totalFrames = 0;
        float percentComplete = 0.0f;
        float fps = 0.0f;               // Processing speed (frames per second)
        float estimatedTimeRemaining = 0.0f; // seconds
        std::string status = "Idle";
        std::string currentStage = "";  // "Extracting", "Upscaling", "Encoding"
    };

    /**
     * Video information structure
     */
    struct VideoInfo {
        int width = 0;
        int height = 0;
        int frameCount = 0;
        float fps = 0.0f;
        float duration = 0.0f;          // seconds
        std::string codec;
        std::string pixelFormat;
    };

    /**
     * Configuration options (typedef to external struct)
     */
    using Config = VideoUpscalerConfig;

    /**
     * Constructor
     */
    VideoUpscaler();

    /**
     * Destructor
     */
    ~VideoUpscaler();

    /**
     * Initialize the upscaler
     * Checks for Python, PyTorch, FFmpeg, and required models
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Start video upscaling (async)
     * Spawns background thread for processing
     *
     * @param inputPath Path to input video file
     * @param quality Quality preset (determines model)
     * @param scaleFactor Upscaling factor
     * @param config Optional configuration
     * @return Output video path (with "_upscaled" suffix), or empty string on error
     */
    std::string upscale(const std::string& inputPath,
                        Quality quality,
                        ScaleFactor scaleFactor,
                        const VideoUpscalerConfig& config = VideoUpscalerConfig());

    /**
     * Start video upscaling with custom output path (async)
     *
     * @param inputPath Path to input video file
     * @param outputPath Path for output video file
     * @param quality Quality preset (determines model)
     * @param scaleFactor Upscaling factor
     * @param config Optional configuration
     * @return true if processing started successfully
     */
    bool upscaleToPath(const std::string& inputPath,
                       const std::string& outputPath,
                       Quality quality,
                       ScaleFactor scaleFactor,
                       const VideoUpscalerConfig& config = VideoUpscalerConfig());

    /**
     * Stop ongoing processing
     * Signals processing thread to terminate gracefully
     */
    void stop();

    /**
     * Check if processing is currently in progress
     * @return true if processing thread is active
     */
    bool isProcessing() const {
        return processing.load();
    }

    /**
     * Get current processing progress
     * Thread-safe, can be called from UI thread
     * @return Progress structure with current status
     */
    Progress getProgress() const;

    /**
     * Get information about a video file
     * @param videoPath Path to video file
     * @return VideoInfo structure (empty on error)
     */
    VideoInfo getVideoInfo(const std::string& videoPath);

    /**
     * Get last error message
     * @return Error string, empty if no error
     */
    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(errorMutex);
        return lastError;
    }

    /**
     * Get output path for a given input path
     * Appends "_upscaled" before the file extension
     * @param inputPath Input video path
     * @param scaleFactor Scale factor (affects suffix)
     * @return Generated output path
     */
    static std::string generateOutputPath(const std::string& inputPath,
                                          ScaleFactor scaleFactor);

    /**
     * Get human-readable name for quality preset
     */
    static std::string qualityToString(Quality quality);

    /**
     * Get human-readable name for scale factor
     */
    static std::string scaleFactorToString(ScaleFactor scaleFactor);

    /**
     * Get numeric scale factor value
     */
    static int scaleFactorToInt(ScaleFactor scaleFactor);

    /**
     * Check if a quality preset uses EDVR
     */
    static bool isEDVRQuality(Quality quality) {
        return quality == Quality::FAST || quality == Quality::BALANCED;
    }

    /**
     * Check if a quality preset uses FlashVSR
     */
    static bool isFlashVSRQuality(Quality quality) {
        return quality == Quality::ULTRA;
    }

    /**
     * Estimate output file size
     * @param inputPath Input video path
     * @param scaleFactor Target scale factor
     * @return Estimated size in bytes (rough estimate)
     */
    size_t estimateOutputSize(const std::string& inputPath, ScaleFactor scaleFactor);

    /**
     * Estimate VRAM required for processing
     * @param width Video width
     * @param height Video height
     * @param quality Quality preset
     * @return VRAM requirement in bytes
     */
    static size_t estimateVRAM(int width, int height, Quality quality);

    /**
     * Format bytes for display
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 GB")
     */
    static std::string formatBytes(size_t bytes);

private:
    // === Threading ===
    std::unique_ptr<std::thread> processingThread;
    std::atomic<bool> processing{false};
    std::atomic<bool> shouldStop{false};

    // === Progress tracking ===
    mutable std::mutex progressMutex;
    Progress progress;
    std::string currentOutputPath;

    // === Chunked progress tracking ===
    int totalVideoFrames = 0;           // Total frames in entire video
    int cumulativeFramesProcessed = 0;  // Frames completed in previous chunks
    int lastChunkTotal = 0;             // Total frames in current chunk
    int lastChunkFrame = 0;             // Last frame number seen in current chunk (to detect wrap-around)
    float highWaterMarkPercent = 0.0f;  // Never let progress go backwards

    // === Configuration ===
    bool initialized = false;
    std::string pythonExecutable;
    std::string scriptsDir;
    std::string tempDir;
    std::string modelsDir;

    // === Error handling ===
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Windows subprocess handles ===
#ifdef _WIN32
    void* processHandle = nullptr;
    void* stdoutReadHandle = nullptr;
    void* stderrReadHandle = nullptr;
#else
    pid_t childPid = -1;
    int stdoutPipe[2] = {-1, -1};
    int stderrPipe[2] = {-1, -1};
#endif

    // === Private methods ===

    /**
     * Processing thread function
     * Runs FFmpeg extraction, Python upscaling, and FFmpeg encoding
     */
    void processingThreadFunc(std::string inputPath,
                              std::string outputPath,
                              Quality quality,
                              ScaleFactor scaleFactor,
                              Config config);

    /**
     * Extract frames from video using FFmpeg
     * @param videoPath Input video path
     * @param outputDir Directory for extracted frames
     * @param info Video info (populated by this function)
     * @return true if extraction successful
     */
    bool extractFrames(const std::string& videoPath,
                       const std::string& outputDir,
                       VideoInfo& info);

    /**
     * Launch Python upscaling process
     * @param inputDir Directory with input frames
     * @param outputDir Directory for output frames
     * @param quality Quality preset
     * @param scaleFactor Scale factor
     * @param config Processing configuration
     * @return true if processing successful
     */
    bool launchPythonUpscaling(const std::string& inputDir,
                               const std::string& outputDir,
                               Quality quality,
                               ScaleFactor scaleFactor,
                               const Config& config);

    /**
     * Monitor Python process progress
     * Reads stdout and parses progress updates
     */
    void monitorProgress();

    /**
     * Parse progress line from Python output
     * @param line Output line to parse
     * @return true if line was a progress update
     */
    bool parseProgressLine(const std::string& line);

    /**
     * Generate configuration JSON for Python script
     */
    bool generateConfigJSON(const std::string& configPath,
                            const std::string& inputDir,
                            const std::string& outputDir,
                            Quality quality,
                            ScaleFactor scaleFactor,
                            const Config& config);

    /**
     * Deploy Python upscaling scripts
     * Copies/creates required Python scripts
     */
    bool deployScripts();

    /**
     * Clean up temporary files
     * @param tempDir Temporary directory to clean
     */
    void cleanupTemp(const std::string& tempDir);

    /**
     * Set error message (thread-safe)
     */
    void setError(const std::string& error);

    /**
     * Update progress (thread-safe)
     */
    void updateProgress(const Progress& newProgress);

    /**
     * Clear error message
     */
    void clearError();

    /**
     * Get model name for quality preset
     */
    static std::string getModelName(Quality quality);

    /**
     * Get Python script name for quality preset
     */
    static std::string getScriptName(Quality quality);

    /**
     * Extract audio from video to WAV file
     * @param inputPath Input video path
     * @param wavPath Output WAV file path
     * @return true if audio extracted successfully (false if no audio or error)
     */
    bool extractAudioToWav(const std::string& inputPath,
                           const std::string& wavPath);

    /**
     * Launch Python upscaling process asynchronously (non-blocking)
     * @param inputDir Directory with input frames
     * @param outputDir Directory for output frames
     * @param quality Quality preset
     * @param scaleFactor Scale factor
     * @param config Processing configuration
     * @return true if process launched successfully
     */
    bool launchPythonUpscalingAsync(const std::string& inputDir,
                                    const std::string& outputDir,
                                    Quality quality,
                                    ScaleFactor scaleFactor,
                                    const Config& config);

    /**
     * Encode frames to HAP video in parallel as they appear
     * @param framesOutputDir Directory where upscaled frames will appear
     * @param outputPath Output HAP video path
     * @param info Original video info (for fps, dimensions)
     * @param totalFrames Total expected frame count
     * @return true if encoding successful
     */
    bool encodeFramesParallelHAP(const std::string& framesOutputDir,
                                  const std::string& outputPath,
                                  const VideoInfo& info,
                                  int totalFrames);

    /**
     * Check if Python process is still running
     * @return true if process is running
     */
    bool isPythonProcessRunning();

    /**
     * Wait for Python process to complete
     * @return Exit code of Python process
     */
    int waitForPythonProcess();
};

#endif // VIDEO_UPSCALER_H
