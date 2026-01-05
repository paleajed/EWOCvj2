/**
 * VideoUpscalingInstaller.cpp
 *
 * Implementation of video upscaling installer for EDVR and FlashVSR models.
 * Includes Python 3.12 installation, PyTorch with CUDA support, and model downloads.
 *
 * License: GPL3
 */

#include "VideoUpscalingInstaller.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <shellapi.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <curl/curl.h>
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Constructor / Destructor
// ============================================================================

VideoUpscalingInstaller::VideoUpscalingInstaller() {
    progress.state = VideoUpscalingInstallProgress::State::IDLE;
    progress.status = "Idle";
}

VideoUpscalingInstaller::~VideoUpscalingInstaller() {
    cancelInstallation();
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
}

// ============================================================================
// Public Installation Methods
// ============================================================================

bool VideoUpscalingInstaller::installPython(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installPythonThread, this, config);
    return true;
}

bool VideoUpscalingInstaller::installPyTorch(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installPyTorchThread, this, config);
    return true;
}

bool VideoUpscalingInstaller::installPythonPackages(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installPythonPackagesThread, this, config);
    return true;
}

bool VideoUpscalingInstaller::installEDVRModels(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installEDVRThread, this, config);
    return true;
}

bool VideoUpscalingInstaller::installFlashVSR(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installFlashVSRThread, this, config);
    return true;
}

bool VideoUpscalingInstaller::installAll(const VideoUpscalingInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(&VideoUpscalingInstaller::installAllThread, this, config);
    return true;
}

// ============================================================================
// Status Methods
// ============================================================================

void VideoUpscalingInstaller::cancelInstallation() {
    shouldCancel.store(true);
}

VideoUpscalingInstallProgress VideoUpscalingInstaller::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void VideoUpscalingInstaller::setProgressCallback(std::function<void(const VideoUpscalingInstallProgress&)> callback) {
    progressCallback = callback;
}

// ============================================================================
// Static Verification Methods
// ============================================================================

bool VideoUpscalingInstaller::isPythonInstalled(std::string& pythonPath) {
    pythonPath.clear();

    // Check EWOCVJ2_PYTHON environment variable first
    std::string envPath = getEnvironmentVariable();
    if (!envPath.empty()) {
        // Verify it exists and is Python 3.12+
        std::string testCmd = "\"" + envPath + "\" --version";
        std::string output;
        int exitCode;

        VideoUpscalingInstaller temp;
        if (temp.runCommand(testCmd, output, exitCode, 5000) && exitCode == 0) {
            if (output.find("Python 3.12") != std::string::npos ||
                output.find("Python 3.13") != std::string::npos ||
                output.find("Python 3.14") != std::string::npos) {
                pythonPath = envPath;
                return true;
            }
        }
    }

    // Check default installation directory
    std::string defaultDir = getDefaultPythonDir();
    std::string defaultPython = defaultDir + "\\python.exe";

#ifdef _WIN32
    if (GetFileAttributesA(defaultPython.c_str()) != INVALID_FILE_ATTRIBUTES) {
        pythonPath = defaultPython;
        return true;
    }
#else
    struct stat st;
    if (stat(defaultPython.c_str(), &st) == 0) {
        pythonPath = defaultPython;
        return true;
    }
#endif

    // Try to find Python in PATH
    std::string testCmd = "python --version";
    std::string output;
    int exitCode;

    VideoUpscalingInstaller temp;
    if (temp.runCommand(testCmd, output, exitCode, 5000) && exitCode == 0) {
        if (output.find("Python 3.12") != std::string::npos ||
            output.find("Python 3.13") != std::string::npos ||
            output.find("Python 3.14") != std::string::npos) {
            // Get full path
            std::string whereCmd = "where python";
            std::string wherePath;
            if (temp.runCommand(whereCmd, wherePath, exitCode, 5000) && exitCode == 0) {
                // Take first line
                size_t newline = wherePath.find('\n');
                if (newline != std::string::npos) {
                    wherePath = wherePath.substr(0, newline);
                }
                // Trim whitespace
                while (!wherePath.empty() && (wherePath.back() == '\r' || wherePath.back() == '\n' || wherePath.back() == ' ')) {
                    wherePath.pop_back();
                }
                pythonPath = wherePath;
                return true;
            }
        }
    }

    return false;
}

bool VideoUpscalingInstaller::isPyTorchInstalled(const std::string& pythonPath) {
    if (pythonPath.empty()) return false;

    std::string testCmd = "\"" + pythonPath + "\" -c \"import torch; print(torch.cuda.is_available())\"";
    std::string output;
    int exitCode;

    VideoUpscalingInstaller temp;
    if (temp.runCommand(testCmd, output, exitCode, 30000) && exitCode == 0) {
        return output.find("True") != std::string::npos;
    }
    return false;
}

bool VideoUpscalingInstaller::arePythonPackagesInstalled(const std::string& pythonPath) {
    if (pythonPath.empty()) return false;

    // Check additional packages for video upscaling
    const char* packages[] = {"safetensors", "einops", "basicsr"};
    VideoUpscalingInstaller temp;

    for (const char* pkg : packages) {
        if (!temp.checkPackageInstalled(pythonPath, pkg)) {
            return false;
        }
    }
    return true;
}

bool VideoUpscalingInstaller::isEDVRInstalled(const std::string& modelsDir) {
    if (modelsDir.empty()) return false;

    // Check all EDVR model files
    const char* files[] = {
        "EDVR_L_deblur_REDS_official-ca46bd8c.pth",
        "EDVR_L_deblurcomp_REDS_official-0e988e5c.pth",
        "EDVR_L_x4_SR_REDS_official-9f5f5039.pth",
        "EDVR_L_x4_SR_Vimeo90K_official-162b54e4.pth",
        "EDVR_L_x4_SRblur_REDS_official-983d7b8e.pth",
        "EDVR_M_woTSA_x4_SR_REDS_official-1edf645c.pth",
        "EDVR_M_x4_SR_REDS_official-32075921.pth"
    };

    for (const char* file : files) {
        std::string path = modelsDir + "/" + file;
        try {
            if (!fs::exists(path)) {
                return false;
            }
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool VideoUpscalingInstaller::isFlashVSRInstalled(const std::string& modelsDir) {
    if (modelsDir.empty()) return false;

    // Check all FlashVSR model files
    std::string flashDir = modelsDir + "/FlashVSR-v1.1";

    try {
        if (!fs::exists(flashDir + "/LQ_proj_in.ckpt")) return false;
        if (!fs::exists(flashDir + "/TCDecoder.ckpt")) return false;
        if (!fs::exists(flashDir + "/diffusion_pytorch_model_streaming_dmd.safetensors")) return false;
    } catch (...) {
        return false;
    }

    return true;
}

bool VideoUpscalingInstaller::isFullyInstalled(const std::string& modelsDir) {
    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) return false;
    if (!isPyTorchInstalled(pythonPath)) return false;
    if (!arePythonPackagesInstalled(pythonPath)) return false;
    if (!isEDVRInstalled(modelsDir)) return false;
    if (!isFlashVSRInstalled(modelsDir)) return false;
    return true;
}

std::string VideoUpscalingInstaller::getPythonPath() {
    std::string path;
    isPythonInstalled(path);
    return path;
}

std::string VideoUpscalingInstaller::getDefaultPythonDir() {
#ifdef _WIN32
    return "C:\\Python312";
#else
    return "/usr/local/python312";
#endif
}

std::string VideoUpscalingInstaller::getDefaultModelsDir() {
#ifdef _WIN32
    return "C:/ProgramData/EWOCvj2/models/upscale";
#else
    return "/var/lib/EWOCvj2/models/upscale";
#endif
}

int64_t VideoUpscalingInstaller::getRequiredDiskSpace(VideoUpscalingComponent component) {
    switch (component) {
        case VideoUpscalingComponent::PYTHON_312:
            return 150LL * 1024 * 1024;  // ~150MB installed
        case VideoUpscalingComponent::PYTORCH_CUDA:
            return 3LL * 1024 * 1024 * 1024;  // ~3GB for PyTorch with CUDA
        case VideoUpscalingComponent::PYTHON_PACKAGES:
            return 500LL * 1024 * 1024;  // ~500MB for additional packages
        case VideoUpscalingComponent::EDVR_MODELS:
            return EDVR_L_DEBLUR_SIZE + EDVR_L_DEBLURCOMP_SIZE + EDVR_L_SR_REDS_SIZE +
                   EDVR_L_SR_VIMEO_SIZE + EDVR_L_SRBLUR_SIZE + EDVR_M_WOTSA_SIZE + EDVR_M_SR_SIZE;
        case VideoUpscalingComponent::FLASHVSR_MODELS:
            return FLASHVSR_LQ_PROJ_SIZE + FLASHVSR_TCDECODER_SIZE + FLASHVSR_DIFFUSION_SIZE;
        case VideoUpscalingComponent::ALL_COMPONENTS:
            return getRequiredDiskSpace(VideoUpscalingComponent::PYTHON_312) +
                   getRequiredDiskSpace(VideoUpscalingComponent::PYTORCH_CUDA) +
                   getRequiredDiskSpace(VideoUpscalingComponent::PYTHON_PACKAGES) +
                   getRequiredDiskSpace(VideoUpscalingComponent::EDVR_MODELS) +
                   getRequiredDiskSpace(VideoUpscalingComponent::FLASHVSR_MODELS);
        default:
            return 0;
    }
}

std::string VideoUpscalingInstaller::formatSize(int64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return buffer;
}

// ============================================================================
// Environment Methods
// ============================================================================

bool VideoUpscalingInstaller::setEnvironmentVariable(const std::string& pythonPath, bool systemWide) {
#ifdef _WIN32
    if (systemWide) {
        // Set system environment variable (requires admin)
        HKEY hKey;
        LONG result = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
            0, KEY_SET_VALUE, &hKey);

        if (result != ERROR_SUCCESS) {
            // Try user environment instead
            result = RegOpenKeyExA(
                HKEY_CURRENT_USER,
                "Environment",
                0, KEY_SET_VALUE, &hKey);
        }

        if (result == ERROR_SUCCESS) {
            result = RegSetValueExA(hKey, "EWOCVJ2_PYTHON", 0, REG_SZ,
                                    (const BYTE*)pythonPath.c_str(),
                                    static_cast<DWORD>(pythonPath.length() + 1));
            RegCloseKey(hKey);

            // Broadcast environment change
            DWORD_PTR dwResult;
            SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                               (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &dwResult);

            return result == ERROR_SUCCESS;
        }
        return false;
    } else {
        // Set user environment variable
        return SetEnvironmentVariableA("EWOCVJ2_PYTHON", pythonPath.c_str()) != 0;
    }
#else
    // On Unix, we'd typically write to .bashrc or similar
    return setenv("EWOCVJ2_PYTHON", pythonPath.c_str(), 1) == 0;
#endif
}

std::string VideoUpscalingInstaller::getEnvironmentVariable() {
#ifdef _WIN32
    char buffer[MAX_PATH];

    // Try process environment first
    DWORD len = GetEnvironmentVariableA("EWOCVJ2_PYTHON", buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return buffer;
    }

    // Try user registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = MAX_PATH;
        DWORD type;
        if (RegQueryValueExA(hKey, "EWOCVJ2_PYTHON", nullptr, &type, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return buffer;
        }
        RegCloseKey(hKey);
    }

    // Try system registry
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = MAX_PATH;
        DWORD type;
        if (RegQueryValueExA(hKey, "EWOCVJ2_PYTHON", nullptr, &type, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return buffer;
        }
        RegCloseKey(hKey);
    }

    return "";
#else
    const char* value = getenv("EWOCVJ2_PYTHON");
    return value ? value : "";
#endif
}

// ============================================================================
// Uninstallation Methods
// ============================================================================

bool VideoUpscalingInstaller::uninstallEDVR(const std::string& modelsDir) {
    const char* files[] = {
        "EDVR_L_deblur_REDS_official-ca46bd8c.pth",
        "EDVR_L_deblurcomp_REDS_official-0e988e5c.pth",
        "EDVR_L_x4_SR_REDS_official-9f5f5039.pth",
        "EDVR_L_x4_SR_Vimeo90K_official-162b54e4.pth",
        "EDVR_L_x4_SRblur_REDS_official-983d7b8e.pth",
        "EDVR_M_woTSA_x4_SR_REDS_official-1edf645c.pth",
        "EDVR_M_x4_SR_REDS_official-32075921.pth"
    };

    bool success = true;
    for (const char* file : files) {
        std::string path = modelsDir + "/" + file;
        if (!deleteFile(path)) {
            success = false;
        }
    }
    return success;
}

bool VideoUpscalingInstaller::uninstallFlashVSR(const std::string& modelsDir) {
    std::string flashDir = modelsDir + "/FlashVSR-v1.1";

    bool success = true;
    success &= deleteFile(flashDir + "/LQ_proj_in.ckpt");
    success &= deleteFile(flashDir + "/TCDecoder.ckpt");
    success &= deleteFile(flashDir + "/diffusion_pytorch_model_streaming_dmd.safetensors");

    return success;
}

bool VideoUpscalingInstaller::uninstallAllModels(const std::string& modelsDir) {
    bool success = true;
    success &= uninstallEDVR(modelsDir);
    success &= uninstallFlashVSR(modelsDir);
    return success;
}

// ============================================================================
// Error Handling
// ============================================================================

std::string VideoUpscalingInstaller::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void VideoUpscalingInstaller::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ============================================================================
// Private Installation Thread Methods
// ============================================================================

void VideoUpscalingInstaller::installPythonThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking Python installation...";
    prog.stepsTotal = 4;
    prog.stepsCompleted = 0;
    updateProgress(prog);

    // Check if already installed
    std::string pythonPath;
    if (isPythonInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
        prog.status = "Python 3.12 already installed";
        prog.percentComplete = 100.0f;
        prog.stepsCompleted = prog.stepsTotal;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Setup directories
    std::string pythonDir = config.pythonInstallDir.empty() ? getDefaultPythonDir() : config.pythonInstallDir;
    std::string tempDir = config.tempDir.empty() ? (pythonDir + "\\temp") : config.tempDir;
    createDirectories(tempDir);

    // Download Python installer
    prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
    prog.status = "Downloading Python 3.12...";
    prog.currentItem = "python-3.12.7-amd64.exe";
    prog.stepsCompleted = 1;
    prog.percentComplete = 25.0f;
    updateProgress(prog);

    std::string installerPath = tempDir + "\\python-3.12.7-amd64.exe";
    if (!downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Failed to download Python installer";
        prog.errorMessage = getLastError();
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (shouldCancel.load()) {
        deleteFile(installerPath);
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Run installer
    prog.state = VideoUpscalingInstallProgress::State::INSTALLING;
    prog.status = "Installing Python 3.12...";
    prog.stepsCompleted = 2;
    prog.percentComplete = 50.0f;
    updateProgress(prog);

    if (!runPythonInstaller(installerPath, pythonDir)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Failed to install Python";
        prog.errorMessage = getLastError();
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Verify installation
    prog.state = VideoUpscalingInstallProgress::State::VERIFYING;
    prog.status = "Verifying Python installation...";
    prog.stepsCompleted = 3;
    prog.percentComplete = 75.0f;
    updateProgress(prog);

    if (!verifyPythonInstallation(pythonDir)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Python installation verification failed";
        prog.errorMessage = getLastError();
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Set environment variable
    pythonPath = pythonDir + "\\python.exe";
    if (config.setSystemEnvVar) {
        setEnvironmentVariable(pythonPath, true);
    }

    // Cleanup
    deleteFile(installerPath);

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "Python 3.12 installed successfully";
    prog.stepsCompleted = 4;
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installPyTorchThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking Python installation...";
    prog.stepsTotal = 3;
    updateProgress(prog);

    // Check Python is installed
    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Python 3.12 not installed";
        prog.errorMessage = "Please install Python first";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Check if PyTorch already installed
    if (isPyTorchInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
        prog.status = "PyTorch with CUDA already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Install PyTorch
    prog.state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
    prog.status = "Installing PyTorch with CUDA support...";
    prog.currentItem = "torch, torchvision, torchaudio";
    prog.stepsCompleted = 1;
    prog.percentComplete = 33.0f;
    updateProgress(prog);

    std::string indexUrl = getPyTorchIndexUrl(config.cudaVersion);
    std::vector<std::string> packages = {"torch", "torchvision", "torchaudio"};

    if (!runPipInstallMultiple(pythonPath, packages, indexUrl)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Failed to install PyTorch";
        prog.errorMessage = getLastError();
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Verify PyTorch installation
    prog.state = VideoUpscalingInstallProgress::State::VERIFYING;
    prog.status = "Verifying PyTorch installation...";
    prog.stepsCompleted = 2;
    prog.percentComplete = 66.0f;
    updateProgress(prog);

    if (!isPyTorchInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "PyTorch verification failed";
        prog.errorMessage = "CUDA support not available";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "PyTorch installed successfully";
    prog.stepsCompleted = 3;
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installPythonPackagesThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking Python installation...";
    updateProgress(prog);

    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::FAILED;
        prog.status = "Python 3.12 not installed";
        prog.errorMessage = "Please install Python first";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Count packages to install
    std::vector<std::string> packagesToInstall;
    for (const char* pkg : ADDITIONAL_PACKAGES) {
        if (!checkPackageInstalled(pythonPath, pkg)) {
            packagesToInstall.push_back(pkg);
        }
    }

    if (packagesToInstall.empty()) {
        prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
        prog.status = "All packages already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    prog.stepsTotal = static_cast<int>(packagesToInstall.size());
    prog.stepsCompleted = 0;

    // Install each package
    for (size_t i = 0; i < packagesToInstall.size(); i++) {
        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        const std::string& pkg = packagesToInstall[i];
        prog.state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
        prog.status = "Installing " + pkg + "...";
        prog.currentItem = pkg;
        prog.percentComplete = (static_cast<float>(i) / packagesToInstall.size()) * 100.0f;
        updateProgress(prog);

        if (!runPipInstall(pythonPath, pkg)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = "Failed to install " + pkg;
            prog.errorMessage = getLastError();
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.stepsCompleted = static_cast<int>(i + 1);
    }

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "All packages installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installEDVRThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking EDVR models...";
    updateProgress(prog);

    std::string modelsDir = config.modelsDir.empty() ? getDefaultModelsDir() : config.modelsDir;

    // Check if already installed
    if (isEDVRInstalled(modelsDir)) {
        prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
        prog.status = "EDVR models already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Create directory
    createDirectories(modelsDir);

    // Get file list
    auto files = getEDVRFiles();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    // Download each file
    for (size_t i = 0; i < files.size(); i++) {
        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        auto& file = files[i];
        file.localPath = modelsDir + "/" + file.localPath;

        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.status = "Downloading " + file.description + "...";
        prog.currentFile = file.localPath;
        prog.percentComplete = (static_cast<float>(i) / files.size()) * 100.0f;
        updateProgress(prog);

        if (!downloadModelFile(file)) {
            if (file.required) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to download " + file.description;
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.filesCompleted = static_cast<int>(i + 1);
    }

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "EDVR models installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installFlashVSRThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking FlashVSR models...";
    updateProgress(prog);

    std::string modelsDir = config.modelsDir.empty() ? getDefaultModelsDir() : config.modelsDir;
    std::string flashDir = modelsDir + "/FlashVSR-v1.1";

    // Check if already installed
    if (isFlashVSRInstalled(modelsDir)) {
        prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
        prog.status = "FlashVSR models already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Create directory
    createDirectories(flashDir);

    // Get file list
    auto files = getFlashVSRFiles();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    // Download each file
    for (size_t i = 0; i < files.size(); i++) {
        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        auto& file = files[i];
        file.localPath = flashDir + "/" + file.localPath;

        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.status = "Downloading " + file.description + "...";
        prog.currentItem = file.localPath;
        prog.percentComplete = (static_cast<float>(i) / files.size()) * 100.0f;
        updateProgress(prog);

        if (!downloadModelFile(file)) {
            if (file.required) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to download " + file.description;
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.filesCompleted = static_cast<int>(i + 1);
    }

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "FlashVSR models installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installAllThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.stepsTotal = 5;  // Python, PyTorch, packages, EDVR, FlashVSR
    prog.stepsCompleted = 0;

    // Step 1: Install Python
    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Step 1/5: Checking Python...";
    updateProgress(prog);

    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        prog.status = "Step 1/5: Installing Python 3.12...";
        updateProgress(prog);

        std::string pythonDir = config.pythonInstallDir.empty() ? getDefaultPythonDir() : config.pythonInstallDir;
        std::string tempDir = config.tempDir.empty() ? (pythonDir + "\\temp") : config.tempDir;
        createDirectories(tempDir);

        // Download
        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.currentItem = "python-3.12.7-amd64.exe";
        updateProgress(prog);

        std::string installerPath = tempDir + "\\python-3.12.7-amd64.exe";
        if (!downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = "Failed to download Python installer";
            prog.errorMessage = getLastError();
            updateProgress(prog);
            installing.store(false);
            return;
        }

        if (shouldCancel.load()) {
            deleteFile(installerPath);
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Install
        prog.state = VideoUpscalingInstallProgress::State::INSTALLING;
        prog.status = "Step 1/5: Installing Python 3.12...";
        updateProgress(prog);

        if (!runPythonInstaller(installerPath, pythonDir)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = "Failed to install Python";
            prog.errorMessage = getLastError();
            updateProgress(prog);
            installing.store(false);
            return;
        }

        pythonPath = pythonDir + "\\python.exe";
        if (config.setSystemEnvVar) {
            setEnvironmentVariable(pythonPath, true);
        }

        deleteFile(installerPath);
    }

    prog.stepsCompleted = 1;
    prog.percentComplete = 20.0f;

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 2: Install PyTorch
    if (!isPyTorchInstalled(pythonPath)) {
        prog.state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
        prog.status = "Step 2/5: Installing PyTorch with CUDA...";
        prog.currentItem = "torch, torchvision, torchaudio";
        updateProgress(prog);

        std::string indexUrl = getPyTorchIndexUrl(config.cudaVersion);
        std::vector<std::string> packages = {"torch", "torchvision", "torchaudio"};

        if (!runPipInstallMultiple(pythonPath, packages, indexUrl)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = "Failed to install PyTorch";
            prog.errorMessage = getLastError();
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    prog.stepsCompleted = 2;
    prog.percentComplete = 40.0f;

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 3: Install Python packages
    prog.state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
    prog.status = "Step 3/5: Installing Python packages...";
    updateProgress(prog);

    for (const char* pkg : ADDITIONAL_PACKAGES) {
        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        if (!checkPackageInstalled(pythonPath, pkg)) {
            prog.currentItem = pkg;
            updateProgress(prog);

            if (!runPipInstall(pythonPath, pkg)) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to install " + std::string(pkg);
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }
    }

    prog.stepsCompleted = 3;
    prog.percentComplete = 60.0f;

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 4: Install EDVR models
    std::string modelsDir = config.modelsDir.empty() ? getDefaultModelsDir() : config.modelsDir;

    if (config.installEDVR && !isEDVRInstalled(modelsDir)) {
        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.status = "Step 4/5: Downloading EDVR models...";
        updateProgress(prog);

        createDirectories(modelsDir);
        auto edvrFiles = getEDVRFiles();

        for (auto& file : edvrFiles) {
            if (shouldCancel.load()) {
                prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
                prog.status = "Installation cancelled";
                updateProgress(prog);
                installing.store(false);
                return;
            }

            file.localPath = modelsDir + "/" + file.localPath;
            prog.currentItem = file.description;
            updateProgress(prog);

            if (!downloadModelFile(file) && file.required) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to download " + file.description;
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }
    }

    prog.stepsCompleted = 4;
    prog.percentComplete = 80.0f;

    if (shouldCancel.load()) {
        prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 5: Install FlashVSR models
    if (config.installFlashVSR && !isFlashVSRInstalled(modelsDir)) {
        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.status = "Step 5/5: Downloading FlashVSR models...";
        updateProgress(prog);

        std::string flashDir = modelsDir + "/FlashVSR-v1.1";
        createDirectories(flashDir);
        auto flashFiles = getFlashVSRFiles();

        for (auto& file : flashFiles) {
            if (shouldCancel.load()) {
                prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
                prog.status = "Installation cancelled";
                updateProgress(prog);
                installing.store(false);
                return;
            }

            file.localPath = flashDir + "/" + file.localPath;
            prog.currentItem = file.description;
            updateProgress(prog);

            if (!downloadModelFile(file) && file.required) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to download " + file.description;
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }
    }

    prog.stepsCompleted = 5;
    prog.percentComplete = 100.0f;
    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "Video upscaling installation complete";
    updateProgress(prog);

    installing.store(false);
}

// ============================================================================
// Download Helpers
// ============================================================================

bool VideoUpscalingInstaller::downloadFile(const std::string& url, const std::string& localPath,
                                            int64_t expectedSize) {
#ifdef _WIN32
    // Parse URL manually (works on both MSVC and MinGW)
    std::string host, path;
    bool isHttps = false;

    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        setError("Invalid URL: missing scheme");
        return false;
    }

    std::string scheme = url.substr(0, schemeEnd);
    isHttps = (scheme == "https");

    size_t hostStart = schemeEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    if (pathStart == std::string::npos) {
        host = url.substr(hostStart);
        path = "/";
    } else {
        host = url.substr(hostStart, pathStart - hostStart);
        path = url.substr(pathStart);
    }

    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());

    // Open session
    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-VideoUpscalingInstaller/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        setError("Failed to open WinHTTP session");
        return false;
    }

    // Set timeouts
    WinHttpSetTimeouts(hSession, currentConfig.connectionTimeout,
                       currentConfig.connectionTimeout,
                       currentConfig.downloadTimeout,
                       currentConfig.downloadTimeout);

    // Connect
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                        isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
                                        0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        setError("Failed to connect to server");
        return false;
    }

    // Open request
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(),
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to create request");
        return false;
    }

    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to send request");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to receive response");
        return false;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                        WINHTTP_NO_HEADER_INDEX);

    // Handle redirects
    if (statusCode >= 300 && statusCode < 400) {
        wchar_t locationBuffer[2048] = {0};
        DWORD locationSize = sizeof(locationBuffer);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION,
                               WINHTTP_HEADER_NAME_BY_INDEX, locationBuffer, &locationSize,
                               WINHTTP_NO_HEADER_INDEX)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            // Convert redirect URL and recursively call
            std::wstring wRedirect(locationBuffer);
            std::string redirectUrl(wRedirect.begin(), wRedirect.end());
            return downloadFile(redirectUrl, localPath, expectedSize);
        }
    }

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("HTTP error: " + std::to_string(statusCode));
        return false;
    }

    // Open output file
    std::ofstream outFile(localPath, std::ios::binary);
    if (!outFile) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to create output file");
        return false;
    }

    // Read data
    char buffer[8192];
    DWORD bytesRead;
    int64_t totalBytesRead = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (shouldCancel.load()) {
            outFile.close();
            deleteFile(localPath);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        outFile.write(buffer, bytesRead);
        totalBytesRead += bytesRead;

        // Update progress
        VideoUpscalingInstallProgress prog;
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            prog = progress;
        }
        prog.bytesDownloaded = totalBytesRead;
        prog.bytesTotal = expectedSize;
        updateProgress(prog);
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Verify size if expected
    if (expectedSize > 0 && currentConfig.verifyDownloads) {
        int64_t actualSize = getFileSize(localPath);
        // Allow some tolerance for different compression
        if (actualSize < expectedSize * 0.9 || actualSize > expectedSize * 1.1) {
            deleteFile(localPath);
            setError("Downloaded file size mismatch");
            return false;
        }
    }

    return true;
#else
    // Unix implementation using curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        setError("Failed to initialize curl");
        return false;
    }

    FILE* fp = fopen(localPath.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        setError("Failed to create output file");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, currentConfig.downloadTimeout / 1000);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        deleteFile(localPath);
        setError("Download failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    return true;
#endif
}

bool VideoUpscalingInstaller::downloadModelFile(const VideoUpscalingModelFile& file) {
    // Check if file already exists with correct size
    if (fileExists(file.localPath)) {
        if (file.expectedSize > 0 && currentConfig.verifyDownloads) {
            int64_t actualSize = getFileSize(file.localPath);
            if (actualSize >= file.expectedSize * 0.9 && actualSize <= file.expectedSize * 1.1) {
                return true;  // Already downloaded
            }
        } else {
            return true;
        }
    }

    // Download with retry
    for (int attempt = 0; attempt < currentConfig.maxRetries; attempt++) {
        if (shouldCancel.load()) return false;

        if (downloadFile(file.url, file.localPath, file.expectedSize)) {
            return true;
        }

        if (attempt < currentConfig.maxRetries - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    return false;
}

bool VideoUpscalingInstaller::verifyFile(const std::string& path, int64_t expectedSize) {
    if (!fileExists(path)) return false;

    if (expectedSize > 0) {
        int64_t actualSize = getFileSize(path);
        return actualSize >= expectedSize * 0.9 && actualSize <= expectedSize * 1.1;
    }

    return true;
}

// ============================================================================
// Python Installation Helpers
// ============================================================================

bool VideoUpscalingInstaller::runPythonInstaller(const std::string& installerPath, const std::string& installDir) {
#ifdef _WIN32
    // Build command for silent install
    // /quiet - silent install
    // InstallAllUsers=1 - install for all users
    // PrependPath=1 - add to PATH
    // TargetDir - installation directory
    std::string cmd = "\"" + installerPath + "\" /quiet InstallAllUsers=1 PrependPath=1 "
                      "TargetDir=\"" + installDir + "\" Include_pip=1 Include_tcltk=0 Include_test=0";

    // Run with elevated privileges
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = "cmd.exe";
    std::string args = "/c \"" + cmd + "\"";
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        setError("Failed to launch Python installer (admin rights required)");
        return false;
    }

    // Wait for completion
    WaitForSingleObject(sei.hProcess, currentConfig.pipTimeout);

    DWORD exitCode;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);

    return exitCode == 0;
#else
    setError("Python installation not supported on this platform");
    return false;
#endif
}

bool VideoUpscalingInstaller::verifyPythonInstallation(const std::string& installDir) {
    std::string pythonExe = installDir + "\\python.exe";

    if (!fileExists(pythonExe)) {
        setError("Python executable not found");
        return false;
    }

    // Test Python
    std::string cmd = "\"" + pythonExe + "\" --version";
    std::string output;
    int exitCode;

    if (!runCommand(cmd, output, exitCode, 10000) || exitCode != 0) {
        setError("Python verification failed");
        return false;
    }

    return output.find("Python 3.12") != std::string::npos;
}

// ============================================================================
// Pip Operations
// ============================================================================

bool VideoUpscalingInstaller::runPipInstall(const std::string& pythonPath, const std::string& package,
                                             const std::string& indexUrl) {
    std::string cmd = "\"" + pythonPath + "\" -m pip install --upgrade " + package;
    if (!indexUrl.empty()) {
        cmd += " --extra-index-url " + indexUrl;
    }

    std::string output;
    int exitCode;

    if (!runCommand(cmd, output, exitCode, currentConfig.pipTimeout) || exitCode != 0) {
        setError("Failed to install " + package + ": " + output);
        return false;
    }

    return true;
}

bool VideoUpscalingInstaller::runPipInstallMultiple(const std::string& pythonPath,
                                                     const std::vector<std::string>& packages,
                                                     const std::string& indexUrl) {
    std::string packageList;
    for (const auto& pkg : packages) {
        if (!packageList.empty()) packageList += " ";
        packageList += pkg;
    }

    std::string cmd = "\"" + pythonPath + "\" -m pip install --upgrade " + packageList;
    if (!indexUrl.empty()) {
        cmd += " --extra-index-url " + indexUrl;
    }

    std::string output;
    int exitCode;

    if (!runCommand(cmd, output, exitCode, currentConfig.pipTimeout) || exitCode != 0) {
        setError("Failed to install packages: " + output);
        return false;
    }

    return true;
}

bool VideoUpscalingInstaller::checkPackageInstalled(const std::string& pythonPath, const std::string& package) {
    std::string cmd = "\"" + pythonPath + "\" -c \"import " + package + "\"";
    std::string output;
    int exitCode;

    return runCommand(cmd, output, exitCode, 10000) && exitCode == 0;
}

// ============================================================================
// Command Execution
// ============================================================================

bool VideoUpscalingInstaller::runCommand(const std::string& command, std::string& output, int& exitCode,
                                          int timeoutMs) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {0};

    std::string cmdLine = "cmd.exe /c " + command;
    char* cmdBuf = _strdup(cmdLine.c_str());

    BOOL success = CreateProcessA(nullptr, cmdBuf, nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    free(cmdBuf);

    if (!success) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);

    // Read output
    output.clear();
    char buffer[4096];
    DWORD bytesRead;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    // Wait for process
    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        return false;
    }

    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    exitCode = static_cast<int>(dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return true;
#else
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[128];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    exitCode = pclose(pipe);
    return true;
#endif
}

// ============================================================================
// Utility Methods
// ============================================================================

bool VideoUpscalingInstaller::createDirectories(const std::string& path) {
    try {
        fs::create_directories(path);
        return true;
    } catch (...) {
        return false;
    }
}

bool VideoUpscalingInstaller::deleteFile(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

bool VideoUpscalingInstaller::fileExists(const std::string& path) {
    try {
        return fs::exists(path);
    } catch (...) {
        return false;
    }
}

int64_t VideoUpscalingInstaller::getFileSize(const std::string& path) {
    try {
        return static_cast<int64_t>(fs::file_size(path));
    } catch (...) {
        return 0;
    }
}

int64_t VideoUpscalingInstaller::getFreeDiskSpace(const std::string& path) {
    try {
        auto spaceInfo = fs::space(path);
        return static_cast<int64_t>(spaceInfo.available);
    } catch (...) {
        return 0;
    }
}

// ============================================================================
// Progress Management
// ============================================================================

void VideoUpscalingInstaller::updateProgress(const VideoUpscalingInstallProgress& newProgress) {
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress = newProgress;
    }

    if (progressCallback) {
        progressCallback(newProgress);
    }
}

void VideoUpscalingInstaller::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
}

std::string VideoUpscalingInstaller::getPyTorchIndexUrl(const std::string& cudaVersion) {
    if (cudaVersion == "12.8" || cudaVersion == "12.8.1") {
        return PYTORCH_INDEX_CU128;
    } else if (cudaVersion == "12.6") {
        return PYTORCH_INDEX_CU126;
    } else if (cudaVersion == "11.8") {
        return PYTORCH_INDEX_CU118;
    }
    return PYTORCH_INDEX_CU128;  // Default to latest
}

// ============================================================================
// File List Builders
// ============================================================================

std::vector<VideoUpscalingModelFile> VideoUpscalingInstaller::getEDVRFiles() {
    return {
        {EDVR_L_DEBLUR_URL, "EDVR_L_deblur_REDS_official-ca46bd8c.pth",
         "EDVR-L Deblur", EDVR_L_DEBLUR_SIZE, true},
        {EDVR_L_DEBLURCOMP_URL, "EDVR_L_deblurcomp_REDS_official-0e988e5c.pth",
         "EDVR-L Deblur+Comp", EDVR_L_DEBLURCOMP_SIZE, true},
        {EDVR_L_SR_REDS_URL, "EDVR_L_x4_SR_REDS_official-9f5f5039.pth",
         "EDVR-L 4x SR REDS", EDVR_L_SR_REDS_SIZE, true},
        {EDVR_L_SR_VIMEO_URL, "EDVR_L_x4_SR_Vimeo90K_official-162b54e4.pth",
         "EDVR-L 4x SR Vimeo", EDVR_L_SR_VIMEO_SIZE, true},
        {EDVR_L_SRBLUR_URL, "EDVR_L_x4_SRblur_REDS_official-983d7b8e.pth",
         "EDVR-L 4x SR+Blur", EDVR_L_SRBLUR_SIZE, true},
        {EDVR_M_WOTSA_URL, "EDVR_M_woTSA_x4_SR_REDS_official-1edf645c.pth",
         "EDVR-M woTSA", EDVR_M_WOTSA_SIZE, true},
        {EDVR_M_SR_URL, "EDVR_M_x4_SR_REDS_official-32075921.pth",
         "EDVR-M 4x SR", EDVR_M_SR_SIZE, true}
    };
}

std::vector<VideoUpscalingModelFile> VideoUpscalingInstaller::getFlashVSRFiles() {
    return {
        {FLASHVSR_LQ_PROJ_URL, "LQ_proj_in.ckpt",
         "FlashVSR LQ Projector", FLASHVSR_LQ_PROJ_SIZE, true},
        {FLASHVSR_TCDECODER_URL, "TCDecoder.ckpt",
         "FlashVSR TC Decoder", FLASHVSR_TCDECODER_SIZE, true},
        {FLASHVSR_DIFFUSION_URL, "diffusion_pytorch_model_streaming_dmd.safetensors",
         "FlashVSR Diffusion Model", FLASHVSR_DIFFUSION_SIZE, true}
    };
}

std::vector<VideoUpscalingModelFile> VideoUpscalingInstaller::getAllModelFiles() {
    auto files = getEDVRFiles();
    auto flashFiles = getFlashVSRFiles();
    files.insert(files.end(), flashFiles.begin(), flashFiles.end());
    return files;
}
