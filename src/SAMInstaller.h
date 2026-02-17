/**
 * SAMInstaller.h
 *
 * Downloads and installs SAM 3 (Segment Anything Model 3) for ComfyUI
 * SAM 3 is Meta's unified model for detection, segmentation, and tracking
 * using open-vocabulary text prompts.
 *
 * Requires ComfyUI base installation (handled automatically if missing).
 *
 * License: GPL3
 */

#ifndef SAM_INSTALLER_H
#define SAM_INSTALLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include "ComfyUIInstaller.h"

/**
 * SAM 3 installation progress
 */
struct SAMInstallProgress {
    enum class State {
        IDLE = 0,
        CHECKING = 1,
        DOWNLOADING = 2,
        INSTALLING_NODES = 3,
        VERIFYING = 4,
        COMPLETE = 5,
        FAILED = 6,
        CANCELLED = 7
    };

    State state = State::IDLE;
    std::string status = "Idle";
    int64_t bytesDownloaded = 0;
    int64_t bytesTotal = 0;
    float percentComplete = 0.0f;
    std::string errorMessage = "";
};

/**
 * SAM 3 installation configuration
 */
struct SAMInstallConfig {
    std::string installDir = "";           // ComfyUI base directory
    std::string tempDir = "";              // Temporary download directory
    int connectionTimeout = 30000;         // Connection timeout (ms)
    int downloadTimeout = 0;               // Download timeout per file (ms), 0 = no timeout
};

/**
 * SAMInstaller - Downloads and installs SAM 3 segmentation stack for ComfyUI
 *
 * Usage:
 *   SAMInstaller installer;
 *   SAMInstallConfig config;
 *   config.installDir = "C:/ProgramData/EWOCvj2/ComfyUI";
 *
 *   installer.setProgressCallback([](const SAMInstallProgress& p) {
 *       std::cout << p.status << " " << p.percentComplete << "%" << std::endl;
 *   });
 *
 *   if (installer.installAll(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 */
class SAMInstaller {
public:
    SAMInstaller();
    ~SAMInstaller();

    // Prevent copying
    SAMInstaller(const SAMInstaller&) = delete;
    SAMInstaller& operator=(const SAMInstaller&) = delete;

    // === Installation ===

    /**
     * Install everything: ComfyUI base (if needed) + SAM 3 custom node
     * The SAM 3 model (~3.2-3.4GB) auto-downloads on first ComfyUI run.
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAll(const SAMInstallConfig& config);

    /**
     * Cancel ongoing installation
     */
    void cancelInstallation();

    /**
     * Check if installation is in progress
     */
    bool isInstalling() const { return installing.load(); }

    /**
     * Get current installation progress
     */
    SAMInstallProgress getProgress() const;

    /**
     * Set progress callback (called from install thread)
     */
    void setProgressCallback(std::function<void(const SAMInstallProgress&)> callback);

    // === Verification ===

    /**
     * Check if SAM 3 is fully installed (ComfyUI base + model weights)
     * (ComfyUI-SAM3 custom node is bundled with the application)
     * @param installDir ComfyUI installation directory
     * @return true if ComfyUI base exists AND sam3.pt model is present
     */
    static bool isSAMInstalled(const std::string& installDir);

    /**
     * Check if SAM 3 model weights are downloaded
     * @param installDir ComfyUI installation directory
     * @return true if models/sam3/sam3.pt exists
     */
    static bool isSAMModelDownloaded(const std::string& installDir);

    /**
     * Get required disk space for SAM 3 (bytes)
     * ~3.5GB for model (auto-downloaded on first run)
     */
    static int64_t getRequiredDiskSpace();

    // === Error Handling ===

    std::string getLastError() const;
    void clearError();

    // Progress (public for use by platform callbacks)
    void updateProgress(const SAMInstallProgress& newProgress);
    static std::string formatBytes(int64_t bytes);

private:
    // State
    std::atomic<bool> installing{false};
    std::atomic<bool> shouldCancel{false};
    SAMInstallConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> installThread;

    // Progress
    mutable std::mutex progressMutex;
    SAMInstallProgress progress;
    std::function<void(const SAMInstallProgress&)> progressCallback;

    // Error
    mutable std::mutex errorMutex;
    std::string lastError;

    // SAM 3 model download (HuggingFace: 1038lab/sam3)
    static constexpr const char* SAM3_MODEL_URL =
        "https://huggingface.co/1038lab/sam3/resolve/main/sam3.pt";
    static constexpr const char* SAM3_MODEL_FILENAME = "sam3.pt";
    static constexpr int64_t SAM3_MODEL_SIZE = 3500000000LL;  // ~3.5GB

    // === Private Methods ===

    // Installation thread
    void installAllThread(SAMInstallConfig config);

    // Model download
    bool downloadModel(const std::string& url, const std::string& destPath);

    // Utility
    bool createDirectories(const std::string& path);
    bool fileExists(const std::string& path);

    // Error
    void setError(const std::string& error);
};

#endif // SAM_INSTALLER_H
