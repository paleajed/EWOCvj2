/**
 * VideoUpscalingInstaller.h
 *
 * Downloads and installs EDVR and FlashVSR models for video upscaling:
 * - Python 3.12 (if not installed)
 * - PyTorch with CUDA 12.8 support
 * - EDVR models (from BasicSR releases) for FAST/BALANCED quality modes
 * - FlashVSR tiny models (from Hugging Face) for ULTRA quality mode
 * - Python dependencies (safetensors, einops, basicsr, etc.)
 *
 * Models are installed to C:/ProgramData/EWOCvj2/models/upscale
 * Sets EWOCVJ2_PYTHON environment variable to the Python executable path
 *
 * License: GPL3
 */

#ifndef VIDEO_UPSCALING_INSTALLER_H
#define VIDEO_UPSCALING_INSTALLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

/**
 * Video upscaling component identifiers
 */
enum class VideoUpscalingComponent {
    PYTHON_312 = 0,               // Python 3.12 interpreter
    PYTORCH_CUDA = 1,             // PyTorch with CUDA 12.8 support
    PYTHON_PACKAGES = 2,          // Additional Python packages (safetensors, einops, basicsr)
    EDVR_MODELS = 3,              // EDVR models (~96MB total)
    FLASHVSR_MODELS = 4,          // FlashVSR models (~6.4GB total)
    ALL_COMPONENTS = 5,           // Everything
    COMPONENT_COUNT = 6
};

/**
 * Download/installation progress for video upscaling
 */
struct VideoUpscalingInstallProgress {
    enum class State {
        IDLE = 0,
        CHECKING = 1,
        DOWNLOADING = 2,
        INSTALLING = 3,
        INSTALLING_PACKAGES = 4,
        VERIFYING = 5,
        COMPLETE = 6,
        FAILED = 7,
        CANCELLED = 8
    };

    State state = State::IDLE;
    std::string status = "Idle";
    std::string currentItem = "";
    std::string currentFile = "";

    // Download progress
    int64_t bytesDownloaded = 0;
    int64_t bytesTotal = 0;
    float downloadSpeed = 0.0f;       // bytes/second

    // Overall progress
    int stepsCompleted = 0;
    int stepsTotal = 0;
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
 * Model file descriptor for video upscaling
 */
struct VideoUpscalingModelFile {
    std::string url;              // Source URL
    std::string localPath;        // Destination path (relative to models dir)
    std::string description;      // Human-readable description
    int64_t expectedSize = 0;     // Expected file size in bytes
    bool required = true;         // If false, skip on failure
};

/**
 * Installation configuration for video upscaling
 */
struct VideoUpscalingInstallConfig {
    std::string pythonInstallDir = "";        // Python installation directory (default: C:\Python312)
    std::string modelsDir = "";               // Models directory (e.g., C:/ProgramData/EWOCvj2/models/upscale)
    std::string tempDir = "";                 // Temporary download directory
    bool setSystemEnvVar = true;              // Set EWOCVJ2_PYTHON as system environment variable
    bool installCudaSupport = true;           // Install PyTorch with CUDA support
    std::string cudaVersion = "12.8";         // CUDA version (12.8, 12.6, 11.8)
    bool installEDVR = true;                  // Install EDVR models
    bool installFlashVSR = true;              // Install FlashVSR tiny models
    bool verifyDownloads = true;              // Verify file sizes
    bool cleanupOnFailure = true;             // Remove partial downloads on failure
    bool resumeDownloads = true;              // Resume interrupted downloads
    int maxRetries = 3;                       // Retry failed downloads
    int connectionTimeout = 30000;            // Connection timeout (ms)
    int downloadTimeout = 600000;             // Download timeout per file (ms) - longer for large models
    int pipTimeout = 1800000;                 // Pip install timeout (ms) - 30 minutes for PyTorch
};

/**
 * VideoUpscalingInstaller - Downloads and installs Python, PyTorch, EDVR and FlashVSR models
 *
 * Usage:
 *   VideoUpscalingInstaller installer;
 *   VideoUpscalingInstallConfig config;
 *   config.modelsDir = "C:/ProgramData/EWOCvj2/models/upscale";
 *
 *   installer.setProgressCallback([](const VideoUpscalingInstallProgress& p) {
 *       std::cout << p.status << " " << p.percentComplete << "%" << std::endl;
 *   });
 *
 *   // Install all components
 *   if (installer.installAll(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 */
class VideoUpscalingInstaller {
public:
    VideoUpscalingInstaller();
    ~VideoUpscalingInstaller();

    // Prevent copying
    VideoUpscalingInstaller(const VideoUpscalingInstaller&) = delete;
    VideoUpscalingInstaller& operator=(const VideoUpscalingInstaller&) = delete;

    // === Installation Methods ===

    /**
     * Install Python 3.12 if not already installed
     * Downloads from python.org and installs to pythonInstallDir
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPython(const VideoUpscalingInstallConfig& config);

    /**
     * Install PyTorch with CUDA support
     * Requires Python to be installed first
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPyTorch(const VideoUpscalingInstallConfig& config);

    /**
     * Install additional Python packages (safetensors, einops, basicsr)
     * Requires Python to be installed first
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPythonPackages(const VideoUpscalingInstallConfig& config);

    /**
     * Install EDVR models (2 .pth files from BasicSR)
     * Download size: ~96MB total (EDVR_L Vimeo90K + EDVR_M)
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installEDVRModels(const VideoUpscalingInstallConfig& config);

    /**
     * Install FlashVSR models (LQ_proj_in.ckpt + TCDecoder.ckpt + diffusion model)
     * Download size: ~6.4GB total
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installFlashVSR(const VideoUpscalingInstallConfig& config);

    /**
     * Install all components (Python + PyTorch + packages + EDVR + FlashVSR)
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAll(const VideoUpscalingInstallConfig& config);

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
    VideoUpscalingInstallProgress getProgress() const;

    /**
     * Set progress callback (called from install thread)
     */
    void setProgressCallback(std::function<void(const VideoUpscalingInstallProgress&)> callback);

    // === Verification ===

    /**
     * Check if Python 3.12.x is installed (specifically 3.12, not 3.11 or 3.13)
     * @param pythonPath Output: path to python.exe if found
     * @return true if Python 3.12.x is installed
     */
    static bool isPythonInstalled(std::string& pythonPath);

    /**
     * Check if PyTorch is installed with CUDA support
     * @param pythonPath Path to python.exe
     * @return true if PyTorch with CUDA is available
     */
    static bool isPyTorchInstalled(const std::string& pythonPath);

    /**
     * Check if all required Python packages are installed
     * @param pythonPath Path to python.exe
     * @return true if all packages are available
     */
    static bool arePythonPackagesInstalled(const std::string& pythonPath);

    /**
     * Check if all EDVR models are installed
     * @param modelsDir Models directory
     */
    static bool isEDVRInstalled(const std::string& modelsDir);

    /**
     * Check if FlashVSR models are installed
     * @param modelsDir Models directory
     */
    static bool isFlashVSRInstalled(const std::string& modelsDir);

    /**
     * Check if everything is installed and ready
     * @param modelsDir Models directory
     * @return true if video upscaling is ready
     */
    static bool isFullyInstalled(const std::string& modelsDir);

    /**
     * Get the Python executable path (from EWOCVJ2_PYTHON or detected)
     * @return Path to python.exe, or empty string if not found
     */
    static std::string getPythonPath();

    /**
     * Get default Python installation directory
     * Returns: C:\Python312 on Windows
     */
    static std::string getDefaultPythonDir();

    /**
     * Get default models directory
     * Returns: C:/ProgramData/EWOCvj2/models/upscale on Windows
     */
    static std::string getDefaultModelsDir();

    /**
     * Get required disk space for a component (bytes)
     */
    static int64_t getRequiredDiskSpace(VideoUpscalingComponent component);

    /**
     * Get human-readable size string
     */
    static std::string formatSize(int64_t bytes);

    // === Environment ===

    /**
     * Set EWOCVJ2_PYTHON environment variable
     * @param pythonPath Path to python.exe
     * @param systemWide If true, sets system environment variable (requires admin)
     * @return true if successful
     */
    static bool setEnvironmentVariable(const std::string& pythonPath, bool systemWide = true);

    /**
     * Get EWOCVJ2_PYTHON environment variable value
     * @return Path from environment variable, or empty string
     */
    static std::string getEnvironmentVariable();

    // === Uninstallation ===

    /**
     * Remove all EDVR models
     */
    bool uninstallEDVR(const std::string& modelsDir);

    /**
     * Remove FlashVSR models
     */
    bool uninstallFlashVSR(const std::string& modelsDir);

    /**
     * Remove all video upscaling models
     */
    bool uninstallAllModels(const std::string& modelsDir);

    // === Error Handling ===

    std::string getLastError() const;
    void clearError();

private:
    // State
    std::atomic<bool> installing{false};
    std::atomic<bool> shouldCancel{false};
    VideoUpscalingInstallConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> installThread;

    // Progress
    mutable std::mutex progressMutex;
    VideoUpscalingInstallProgress progress;
    std::function<void(const VideoUpscalingInstallProgress&)> progressCallback;

    // Error
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Download URLs ===

    // Python 3.12.8 installer (64-bit Windows)
    // Must be 3.12.x specifically - other versions may cause compatibility issues
    static constexpr const char* PYTHON_312_URL =
        "https://www.python.org/ftp/python/3.12.8/python-3.12.8-amd64.exe";
    static constexpr int64_t PYTHON_312_SIZE = 25500000LL;  // ~25MB

    // PyTorch pip index for CUDA versions
    static constexpr const char* PYTORCH_INDEX_CU128 =
        "https://download.pytorch.org/whl/cu128";
    static constexpr const char* PYTORCH_INDEX_CU126 =
        "https://download.pytorch.org/whl/cu126";
    static constexpr const char* PYTORCH_INDEX_CU118 =
        "https://download.pytorch.org/whl/cu118";

    // EDVR models from Google Drive (xinntao's official repository)
    // Using gdown for download since GitHub releases URLs are no longer available

    // EDVR_L Vimeo90K - for BALANCED quality (better for general video content)
    static constexpr const char* EDVR_L_SR_VIMEO_GDRIVE_ID = "1aVR3lkX6ItCphNLcT7F5bbbC484h4Qqy";
    static constexpr const char* EDVR_L_SR_VIMEO_FILENAME = "EDVR_L_x4_SR_Vimeo90K_official-162b54e4.pth";
    static constexpr int64_t EDVR_L_SR_VIMEO_SIZE = 82850094LL;

    // EDVR_M - for FAST quality
    static constexpr const char* EDVR_M_SR_GDRIVE_ID = "1dd6aFj-5w2v08VJTq5mS9OFsD-wALYD6";
    static constexpr const char* EDVR_M_SR_FILENAME = "EDVR_M_x4_SR_REDS_official-32075921.pth";
    static constexpr int64_t EDVR_M_SR_SIZE = 13228280LL;

    // FlashVSR models from Hugging Face
    // https://huggingface.co/JunhaoZhuang/FlashVSR-v1.1

    static constexpr const char* FLASHVSR_LQ_PROJ_URL =
        "https://huggingface.co/JunhaoZhuang/FlashVSR-v1.1/resolve/main/LQ_proj_in.ckpt";
    static constexpr int64_t FLASHVSR_LQ_PROJ_SIZE = 575694948LL;

    static constexpr const char* FLASHVSR_TCDECODER_URL =
        "https://huggingface.co/JunhaoZhuang/FlashVSR-v1.1/resolve/main/TCDecoder.ckpt";
    static constexpr int64_t FLASHVSR_TCDECODER_SIZE = 189018333LL;

    static constexpr const char* FLASHVSR_DIFFUSION_URL =
        "https://huggingface.co/JunhaoZhuang/FlashVSR-v1.1/resolve/main/diffusion_pytorch_model_streaming_dmd.safetensors";
    static constexpr int64_t FLASHVSR_DIFFUSION_SIZE = 5676070392LL;

    // === Required Packages ===

    // Core PyTorch packages (installed with CUDA index)
    static constexpr const char* PYTORCH_PACKAGES[] = {
        "torch",
        "torchvision",
        "torchaudio"
    };

    // Additional packages for video upscaling
    static constexpr const char* ADDITIONAL_PACKAGES[] = {
        "numpy",
        "Pillow",
        "opencv-python",
        "safetensors",
        "huggingface-hub",
        "einops",
        "basicsr",
        "modelscope",   // Required by diffsynth/FlashVSR
        "ftfy",         // Required by diffsynth/FlashVSR (text processing)
        "gdown"         // For downloading EDVR models from Google Drive
    };

    // === Private Methods ===

    // Installation threads
    void installPythonThread(VideoUpscalingInstallConfig config);
    void installPyTorchThread(VideoUpscalingInstallConfig config);
    void installPythonPackagesThread(VideoUpscalingInstallConfig config);
    void installEDVRThread(VideoUpscalingInstallConfig config);
    void installFlashVSRThread(VideoUpscalingInstallConfig config);
    void installAllThread(VideoUpscalingInstallConfig config);

    // Download helpers
    bool downloadFile(const std::string& url, const std::string& localPath,
                      int64_t expectedSize = 0);
    bool downloadModelFile(const VideoUpscalingModelFile& file);
    bool downloadFileWithResume(const std::string& url, const std::string& localPath,
                                 int64_t expectedSize = 0);
    bool downloadFromGoogleDrive(const std::string& fileId, const std::string& localPath,
                                  const std::string& pythonPath);
    bool verifyFile(const std::string& path, int64_t expectedSize);

    // Python installation
    bool runPythonInstaller(const std::string& installerPath, const std::string& installDir);
    bool verifyPythonInstallation(const std::string& installDir);

    // Pip operations
    bool runPipInstall(const std::string& pythonPath, const std::string& package,
                       const std::string& indexUrl = "");
    bool runPipInstallMultiple(const std::string& pythonPath,
                                const std::vector<std::string>& packages,
                                const std::string& indexUrl = "");
    bool checkPackageInstalled(const std::string& pythonPath, const std::string& package);

    // Command execution
    bool runCommand(const std::string& command, std::string& output, int& exitCode,
                    int timeoutMs = 60000);
    bool runCommandElevated(const std::string& command, int timeoutMs = 300000);

    // Utility
    bool createDirectories(const std::string& path);
    bool deleteFile(const std::string& path);
    bool fileExists(const std::string& path);
    int64_t getFileSize(const std::string& path);
    int64_t getFreeDiskSpace(const std::string& path);

    // Progress
    void updateProgress(const VideoUpscalingInstallProgress& newProgress);
    void setError(const std::string& error);

    // Get pip index URL for CUDA version
    std::string getPyTorchIndexUrl(const std::string& cudaVersion);

    // Build file lists
    std::vector<VideoUpscalingModelFile> getEDVRFiles();
    std::vector<VideoUpscalingModelFile> getFlashVSRFiles();
    std::vector<VideoUpscalingModelFile> getAllModelFiles();
};

#endif // VIDEO_UPSCALING_INSTALLER_H
