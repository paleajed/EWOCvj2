/**
 * RealESRGANInstaller.h
 *
 * Downloads and installs Real-ESRGAN ncnn models for image/video upscaling:
 * - realesrgan-x4plus (general purpose 4x upscaling)
 * - realesrgan-x4plus-anime (anime-optimized 4x)
 * - realesr-animevideov3 (anime video 2x/3x/4x)
 *
 * Models are downloaded from the official Real-ESRGAN-ncnn-vulkan repository
 *
 * License: GPL3
 */

#ifndef REALESRGAN_INSTALLER_H
#define REALESRGAN_INSTALLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

/**
 * RealESRGAN model component identifiers
 */
enum class RealESRGANComponent {
    REALESRGAN_X4PLUS = 0,        // General purpose 4x upscaling (~33MB)
    REALESRGAN_X4PLUS_ANIME = 1,  // Anime-optimized 4x (~9MB)
    ANIMEVIDEOV3_ALL = 2,         // Anime video models 2x/3x/4x (~4MB total)
    ALL_MODELS = 3,               // All available models
    COMPONENT_COUNT = 4
};

/**
 * Download/installation progress for RealESRGAN
 */
struct RealESRGANInstallProgress {
    enum class State {
        IDLE = 0,
        CHECKING = 1,
        DOWNLOADING = 2,
        VERIFYING = 3,
        COMPLETE = 4,
        FAILED = 5,
        CANCELLED = 6
    };

    State state = State::IDLE;
    std::string status = "Idle";
    std::string currentFile = "";

    // Download progress
    int64_t bytesDownloaded = 0;
    int64_t bytesTotal = 0;
    float downloadSpeed = 0.0f;       // bytes/second

    // Overall progress
    int filesCompleted = 0;
    int filesTotal = 0;
    float percentComplete = 0.0f;

    // Time estimates
    float elapsedTime = 0.0f;
    float estimatedTimeRemaining = 0.0f;

    // Error info
    std::string errorMessage = "";
    int errorCode = 0;
};

/**
 * RealESRGAN model file descriptor
 */
struct RealESRGANModelFile {
    std::string url;              // Source URL
    std::string localPath;        // Destination path (relative to models dir)
    std::string description;      // Human-readable description
    int64_t expectedSize = 0;     // Expected file size in bytes
    bool required = true;         // If false, skip on failure
};

/**
 * Installation configuration for RealESRGAN
 */
struct RealESRGANInstallConfig {
    std::string modelsDir = "";               // Models directory (e.g., C:/ProgramData/EWOCvj2/models/upscale)
    std::string tempDir = "";                 // Temporary download directory
    bool verifyDownloads = true;              // Verify file sizes
    bool cleanupOnFailure = true;             // Remove partial downloads on failure
    bool resumeDownloads = true;              // Resume interrupted downloads
    int maxRetries = 3;                       // Retry failed downloads
    int connectionTimeout = 30000;            // Connection timeout (ms)
    int downloadTimeout = 300000;             // Download timeout per file (ms)
};

/**
 * RealESRGANInstaller - Downloads and installs Real-ESRGAN ncnn models
 *
 * Usage:
 *   RealESRGANInstaller installer;
 *   RealESRGANInstallConfig config;
 *   config.modelsDir = "C:/ProgramData/EWOCvj2/models/upscale";
 *
 *   installer.setProgressCallback([](const RealESRGANInstallProgress& p) {
 *       std::cout << p.status << " " << p.percentComplete << "%" << std::endl;
 *   });
 *
 *   // Install all models
 *   if (installer.installAllModels(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 */
class RealESRGANInstaller {
public:
    RealESRGANInstaller();
    ~RealESRGANInstaller();

    // Prevent copying
    RealESRGANInstaller(const RealESRGANInstaller&) = delete;
    RealESRGANInstaller& operator=(const RealESRGANInstaller&) = delete;

    // === Installation Methods ===

    /**
     * Install RealESRGAN-x4plus model (general purpose 4x upscaling)
     * Download size: ~33MB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installX4Plus(const RealESRGANInstallConfig& config);

    /**
     * Install RealESRGAN-x4plus-anime model (anime-optimized 4x)
     * Download size: ~9MB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installX4PlusAnime(const RealESRGANInstallConfig& config);

    /**
     * Install realesr-animevideov3 models (2x, 3x, 4x for anime video)
     * Download size: ~4MB total
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAnimeVideoV3(const RealESRGANInstallConfig& config);

    /**
     * Install all available models
     * Download size: ~46MB total
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAllModels(const RealESRGANInstallConfig& config);

    // === Status ===

    /**
     * Check if installation is in progress
     */
    bool isInstalling() const { return installing.load(); }

    /**
     * Cancel ongoing installation
     */
    void cancelInstallation();

    /**
     * Get current installation progress
     */
    RealESRGANInstallProgress getProgress() const;

    /**
     * Set progress callback (called from download thread)
     */
    void setProgressCallback(std::function<void(const RealESRGANInstallProgress&)> callback);

    // === Verification ===

    /**
     * Check if RealESRGAN-x4plus is installed
     * @param modelsDir Models directory
     */
    static bool isX4PlusInstalled(const std::string& modelsDir);

    /**
     * Check if RealESRGAN-x4plus-anime is installed
     * @param modelsDir Models directory
     */
    static bool isX4PlusAnimeInstalled(const std::string& modelsDir);

    /**
     * Check if realesr-animevideov3 models are installed
     * @param modelsDir Models directory
     */
    static bool isAnimeVideoV3Installed(const std::string& modelsDir);

    /**
     * Check if all models are installed
     * @param modelsDir Models directory
     */
    static bool isAllModelsInstalled(const std::string& modelsDir);

    /**
     * Get required disk space for a component (bytes)
     */
    static int64_t getRequiredDiskSpace(RealESRGANComponent component);

    /**
     * Get download size for a component (bytes)
     */
    static int64_t getDownloadSize(RealESRGANComponent component);

    /**
     * Get human-readable size string
     */
    static std::string formatSize(int64_t bytes);

    /**
     * Get default models directory
     * Returns: C:/ProgramData/EWOCvj2/models/upscale on Windows
     */
    static std::string getDefaultModelsDir();

    // === Uninstallation ===

    /**
     * Remove RealESRGAN-x4plus model
     */
    bool uninstallX4Plus(const std::string& modelsDir);

    /**
     * Remove RealESRGAN-x4plus-anime model
     */
    bool uninstallX4PlusAnime(const std::string& modelsDir);

    /**
     * Remove realesr-animevideov3 models
     */
    bool uninstallAnimeVideoV3(const std::string& modelsDir);

    /**
     * Remove all RealESRGAN models
     */
    bool uninstallAll(const std::string& modelsDir);

    // === Error Handling ===

    std::string getLastError() const;
    void clearError();

private:
    // State
    std::atomic<bool> installing{false};
    std::atomic<bool> shouldCancel{false};
    RealESRGANInstallConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> installThread;

    // Progress
    mutable std::mutex progressMutex;
    RealESRGANInstallProgress progress;
    std::function<void(const RealESRGANInstallProgress&)> progressCallback;

    // Error
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Download URLs ===
    // Models from Real-ESRGAN-ncnn-vulkan repository
    // https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan

    // RealESRGAN-x4plus (general purpose)
    static constexpr const char* X4PLUS_BIN_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesrgan-x4plus.bin";
    static constexpr int64_t X4PLUS_BIN_SIZE = 33424520LL;

    static constexpr const char* X4PLUS_PARAM_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesrgan-x4plus.param";
    static constexpr int64_t X4PLUS_PARAM_SIZE = 116029LL;

    // RealESRGAN-x4plus-anime
    static constexpr const char* X4PLUS_ANIME_BIN_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesrgan-x4plus-anime.bin";
    static constexpr int64_t X4PLUS_ANIME_BIN_SIZE = 8943500LL;

    static constexpr const char* X4PLUS_ANIME_PARAM_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesrgan-x4plus-anime.param";
    static constexpr int64_t X4PLUS_ANIME_PARAM_SIZE = 30290LL;

    // realesr-animevideov3-x2
    static constexpr const char* ANIMEVIDEO_X2_BIN_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x2.bin";
    static constexpr int64_t ANIMEVIDEO_X2_BIN_SIZE = 1247368LL;

    static constexpr const char* ANIMEVIDEO_X2_PARAM_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x2.param";
    static constexpr int64_t ANIMEVIDEO_X2_PARAM_SIZE = 3173LL;

    // realesr-animevideov3-x3
    static constexpr const char* ANIMEVIDEO_X3_BIN_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x3.bin";
    static constexpr int64_t ANIMEVIDEO_X3_BIN_SIZE = 1247368LL;

    static constexpr const char* ANIMEVIDEO_X3_PARAM_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x3.param";
    static constexpr int64_t ANIMEVIDEO_X3_PARAM_SIZE = 3173LL;

    // realesr-animevideov3-x4
    static constexpr const char* ANIMEVIDEO_X4_BIN_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x4.bin";
    static constexpr int64_t ANIMEVIDEO_X4_BIN_SIZE = 1247368LL;

    static constexpr const char* ANIMEVIDEO_X4_PARAM_URL =
        "https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/raw/master/models/realesr-animevideov3-x4.param";
    static constexpr int64_t ANIMEVIDEO_X4_PARAM_SIZE = 3077LL;

    // === Private Methods ===

    // Installation threads
    void installX4PlusThread(RealESRGANInstallConfig config);
    void installX4PlusAnimeThread(RealESRGANInstallConfig config);
    void installAnimeVideoV3Thread(RealESRGANInstallConfig config);
    void installAllModelsThread(RealESRGANInstallConfig config);

    // Download helpers
    bool downloadFile(const RealESRGANModelFile& file);
    bool downloadFileWithResume(const std::string& url, const std::string& localPath,
                                 int64_t expectedSize = 0);
    bool verifyFile(const std::string& path, int64_t expectedSize);

    // Utility
    bool createDirectories(const std::string& path);
    bool deleteFile(const std::string& path);
    bool fileExists(const std::string& path);
    int64_t getFileSize(const std::string& path);
    int64_t getFreeDiskSpace(const std::string& path);

    // Progress
    void updateProgress(const RealESRGANInstallProgress& newProgress);
    void setError(const std::string& error);

    // Build file lists
    std::vector<RealESRGANModelFile> getX4PlusFiles();
    std::vector<RealESRGANModelFile> getX4PlusAnimeFiles();
    std::vector<RealESRGANModelFile> getAnimeVideoV3Files();
    std::vector<RealESRGANModelFile> getAllModelFiles();
};

#endif // REALESRGAN_INSTALLER_H
