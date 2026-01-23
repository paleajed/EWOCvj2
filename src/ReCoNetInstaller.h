/**
 * ReCoNetInstaller.h
 *
 * Downloads and installs Python 3.12 and dependencies for ReCoNet style transfer training:
 * - Python 3.12 (if not installed)
 * - PyTorch with CUDA 12.8 support
 * - torchvision, torchaudio
 * - numpy, Pillow, scikit-image, opencv-python, onnx
 *
 * Sets EWOCVJ2_PYTHON environment variable to the Python executable path
 *
 * License: GPL3
 */

#ifndef RECONET_INSTALLER_H
#define RECONET_INSTALLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

/**
 * ReCoNet installation component identifiers
 */
enum class ReCoNetComponent {
    PYTHON_312 = 0,           // Python 3.12 interpreter
    PYTORCH_CUDA = 1,         // PyTorch with CUDA 12.8 support
    PYTHON_PACKAGES = 2,      // Additional Python packages (numpy, Pillow, etc.)
    ALL_COMPONENTS = 3,       // Everything
    COMPONENT_COUNT = 4
};

/**
 * Download/installation progress for ReCoNet
 */
struct ReCoNetInstallProgress {
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

    // Download progress
    int64_t bytesDownloaded = 0;
    int64_t bytesTotal = 0;
    float downloadSpeed = 0.0f;       // bytes/second

    // Overall progress
    int stepsCompleted = 0;
    int stepsTotal = 0;
    float percentComplete = 0.0f;

    // Time estimates
    float elapsedTime = 0.0f;
    float estimatedTimeRemaining = 0.0f;

    // Error info
    std::string errorMessage = "";
    int errorCode = 0;
};

/**
 * Installation configuration for ReCoNet
 */
struct ReCoNetInstallConfig {
    std::string pythonInstallDir = "";        // Python installation directory (default: C:\Python312)
    std::string tempDir = "";                 // Temporary download directory
    bool setSystemEnvVar = true;              // Set EWOCVJ2_PYTHON as system environment variable
    bool installCudaSupport = true;           // Install PyTorch with CUDA support
    std::string cudaVersion = "12.8";         // CUDA version (12.8, 12.6, 11.8)
    int connectionTimeout = 30000;            // Connection timeout (ms)
    int downloadTimeout = 600000;             // Download timeout (ms)
    int pipTimeout = 1800000;                 // Pip install timeout (ms) - 30 minutes for PyTorch
};

/**
 * ReCoNetInstaller - Downloads and installs Python and dependencies for ReCoNet training
 *
 * Usage:
 *   ReCoNetInstaller installer;
 *   ReCoNetInstallConfig config;
 *
 *   installer.setProgressCallback([](const ReCoNetInstallProgress& p) {
 *       std::cout << p.status << " " << p.percentComplete << "%" << std::endl;
 *   });
 *
 *   // Install everything
 *   if (installer.installAll(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 */
class ReCoNetInstaller {
public:
    ReCoNetInstaller();
    ~ReCoNetInstaller();

    // Prevent copying
    ReCoNetInstaller(const ReCoNetInstaller&) = delete;
    ReCoNetInstaller& operator=(const ReCoNetInstaller&) = delete;

    // === Installation Methods ===

    /**
     * Install Python 3.12 if not already installed
     * Downloads from python.org and installs to pythonInstallDir
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPython(const ReCoNetInstallConfig& config);

    /**
     * Install PyTorch with CUDA support
     * Requires Python to be installed first
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPyTorch(const ReCoNetInstallConfig& config);

    /**
     * Install additional Python packages (numpy, Pillow, etc.)
     * Requires Python to be installed first
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installPythonPackages(const ReCoNetInstallConfig& config);

    /**
     * Install everything (Python + PyTorch + packages)
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAll(const ReCoNetInstallConfig& config);

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
    ReCoNetInstallProgress getProgress() const;

    /**
     * Set progress callback (called from install thread)
     */
    void setProgressCallback(std::function<void(const ReCoNetInstallProgress&)> callback);

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
     * Check if all required packages are installed
     * @param pythonPath Path to python.exe
     * @return true if all packages are available
     */
    static bool arePackagesInstalled(const std::string& pythonPath);

    /**
     * Check if everything is installed and ready
     * @return true if ReCoNet training is ready
     */
    static bool isFullyInstalled();

    /**
     * Check if training dataset (COCO content images) has been downloaded
     * @return true if at least 200 images exist in the content directory
     */
    static bool isDatasetDownloaded();

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
     * Get required disk space for installation (bytes)
     */
    static int64_t getRequiredDiskSpace(ReCoNetComponent component);

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

    // === Error Handling ===

    std::string getLastError() const;
    void clearError();

private:
    // State
    std::atomic<bool> installing{false};
    std::atomic<bool> shouldCancel{false};
    ReCoNetInstallConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> installThread;

    // Progress
    mutable std::mutex progressMutex;
    ReCoNetInstallProgress progress;
    std::function<void(const ReCoNetInstallProgress&)> progressCallback;

    // Error
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Download URLs ===

    // Python 3.12.8 installer (64-bit Windows)
    static constexpr const char* PYTHON_312_URL =
        "https://www.python.org/ftp/python/3.12.8/python-3.12.8-amd64.exe";
    static constexpr int64_t PYTHON_312_SIZE = 25500000LL;  // ~25MB

    // PyTorch pip index for CUDA 12.8
    static constexpr const char* PYTORCH_INDEX_CU128 =
        "https://download.pytorch.org/whl/cu128";

    // PyTorch pip index for CUDA 12.6
    static constexpr const char* PYTORCH_INDEX_CU126 =
        "https://download.pytorch.org/whl/cu126";

    // PyTorch pip index for CUDA 11.8
    static constexpr const char* PYTORCH_INDEX_CU118 =
        "https://download.pytorch.org/whl/cu118";

    // === Required Packages ===

    // Core PyTorch packages (installed with CUDA index)
    static constexpr const char* PYTORCH_PACKAGES[] = {
        "torch",
        "torchvision",
        "torchaudio"
    };

    // Additional packages for ReCoNet training
    static constexpr const char* ADDITIONAL_PACKAGES[] = {
        "numpy",
        "Pillow",
        "scikit-image",
        "opencv-python",
        "tensorboard",
        "onnx"
    };

    // === Private Methods ===

    // Installation threads
    void installPythonThread(ReCoNetInstallConfig config);
    void installPyTorchThread(ReCoNetInstallConfig config);
    void installPythonPackagesThread(ReCoNetInstallConfig config);
    void installAllThread(ReCoNetInstallConfig config);

    // Download helpers
    bool downloadFile(const std::string& url, const std::string& localPath,
                      int64_t expectedSize = 0);

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

    /**
     * Run a Python script and parse its output for progress messages
     * Lines starting with the specified prefix will be used to update progress.status
     * @param pythonPath Path to python executable
     * @param scriptPath Path to the Python script
     * @param statusPrefix Prefix to look for in output lines (e.g., "COCO Content:")
     * @param timeoutMs Timeout in milliseconds
     * @return true if script ran successfully (exit code 0)
     */
    bool runScriptWithProgress(const std::string& pythonPath, const std::string& scriptPath,
                               const std::string& statusPrefix, int timeoutMs = 600000);

    // Utility
    bool createDirectories(const std::string& path);
    bool fileExists(const std::string& path);
    int64_t getFileSize(const std::string& path);
    int64_t getFreeDiskSpace(const std::string& path);

    // Progress
    void updateProgress(const ReCoNetInstallProgress& newProgress);
    void setError(const std::string& error);

    // Get pip index URL for CUDA version
    std::string getPyTorchIndexUrl(const std::string& cudaVersion);
};

#endif // RECONET_INSTALLER_H
