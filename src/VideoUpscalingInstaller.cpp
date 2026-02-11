/**
 * VideoUpscalingInstaller.cpp
 *
 * Implementation of video upscaling installer for EDVR and FlashVSR models.
 * Includes Python 3.12 installation, PyTorch with CUDA support, and model downloads.
 *
 * License: GPL3
 */

#include "VideoUpscalingInstaller.h"
#include "InstallVerification.h"

// Helper to get programData without including program.h (avoids OpenGL header conflicts)
extern std::string getProgramDataPath();

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
        std::string errMsg = "Installation already in progress";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = VideoUpscalingInstallProgress::State::FAILED;
        progress.errorMessage = errMsg;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[VideoUpscalingInstaller] %s\n", errMsg.c_str());
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

    // Helper lambda to check if a Python path is specifically version 3.12.x
    // Uses direct CreateProcessA and checks exit code (critical for rejecting Windows Store stubs)
    auto isPython312 = [](const std::string& path) -> bool {
        if (!fs::exists(path)) return false;

#ifdef _WIN32
        // Create pipes for capturing stdout
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return false;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        ZeroMemory(&pi, sizeof(pi));

        std::string cmd = "\"" + path + "\" --version";
        std::string cmdCopy = cmd;

        if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(hReadPipe);
            CloseHandle(hWritePipe);
            return false;
        }

        CloseHandle(hWritePipe);

        // Read output
        std::string output;
        char buffer[256];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        CloseHandle(hReadPipe);

        WaitForSingleObject(pi.hProcess, 5000);

        // CRITICAL: Check exit code - Windows Store stubs return non-zero
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0) return false;

        // Must be specifically Python 3.12.x - reject 3.11, 3.13, etc.
        return output.find("Python 3.12") != std::string::npos;
#else
        // Unix implementation
        std::string cmd = "\"" + path + "\" --version 2>&1";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;

        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        int result = pclose(pipe);

        if (result != 0) return false;

        return output.find("Python 3.12") != std::string::npos;
#endif
    };

    // Check EWOCVJ2_PYTHON environment variable first
    std::string envPath = getEnvironmentVariable();
    if (!envPath.empty() && isPython312(envPath)) {
        pythonPath = envPath;
        return true;
    }

#ifdef _WIN32
    // Check Windows Registry for Python 3.12 installation
    // This is the most reliable method as registry is updated immediately after install
    auto checkRegistryPath = [&isPython312](HKEY hKeyRoot, const char* subKey) -> std::string {
        HKEY hKey;
        if (RegOpenKeyExA(hKeyRoot, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char installPath[MAX_PATH];
            DWORD pathSize = sizeof(installPath);
            DWORD type;
            if (RegQueryValueExA(hKey, nullptr, nullptr, &type, (LPBYTE)installPath, &pathSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                std::string pythonExe = std::string(installPath) + "python.exe";
                if (isPython312(pythonExe)) {
                    return pythonExe;
                }
            }
            RegCloseKey(hKey);
        }
        return "";
    };

    // Check HKEY_LOCAL_MACHINE first (all users install)
    std::string regPath = checkRegistryPath(HKEY_LOCAL_MACHINE, "SOFTWARE\\Python\\PythonCore\\3.12\\InstallPath");
    if (!regPath.empty()) {
        pythonPath = regPath;
        return true;
    }

    // Check HKEY_CURRENT_USER (current user install)
    regPath = checkRegistryPath(HKEY_CURRENT_USER, "SOFTWARE\\Python\\PythonCore\\3.12\\InstallPath");
    if (!regPath.empty()) {
        pythonPath = regPath;
        return true;
    }
#endif

    // Fall back to known installation locations
    std::vector<std::string> pythonPaths = {
        getDefaultPythonDir() + "\\python.exe"  // C:\Python312\python.exe
    };

#ifdef _WIN32
    // Add user-specific Python 3.12 path
    const char* username = getenv("USERNAME");
    if (username) {
        pythonPaths.push_back(std::string("C:\\Users\\") + username +
            "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe");
    }
#endif

    for (const auto& path : pythonPaths) {
        if (isPython312(path)) {
            pythonPath = path;
            return true;
        }
    }

    pythonPath.clear();
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

    // First check manifest for verified installation
    auto result = InstallVerification::verifyInstallation(modelsDir, "edvr_models");
    if (result.isValid()) {
        return true;
    }

    // Fall back to file checking with size verification (backwards compatibility)
    // This catches interrupted installations where files exist but are incomplete
    if (!result.manifestExists) {
        try {
            // Check file existence AND size (within 5% tolerance)
            auto verifyFileSize = [&modelsDir](const char* filename, int64_t expectedSize) -> bool {
                fs::path p = fs::path(modelsDir) / filename;
                if (!fs::exists(p)) return false;
                int64_t actualSize = static_cast<int64_t>(fs::file_size(p));
                int64_t minSize = static_cast<int64_t>(expectedSize * 0.95);
                return actualSize >= minSize;
            };

            if (!verifyFileSize(EDVR_L_SR_VIMEO_FILENAME, EDVR_L_SR_VIMEO_SIZE)) return false;
            if (!verifyFileSize(EDVR_M_SR_FILENAME, EDVR_M_SR_SIZE)) return false;
        } catch (...) {
            return false;
        }
        return true;
    }

    return false;
}

bool VideoUpscalingInstaller::isFlashVSRInstalled(const std::string& modelsDir) {
    if (modelsDir.empty()) return false;

    // First check manifest for verified installation
    auto result = InstallVerification::verifyInstallation(modelsDir, "flashvsr_models");
    if (result.isValid()) {
        return true;
    }

    // Fall back to file checking with size verification (backwards compatibility)
    // This catches interrupted installations where files exist but are incomplete
    if (!result.manifestExists) {
        std::string flashDir = modelsDir + "/FlashVSR-v1.1";

        try {
            // Check file existence AND size (within 5% tolerance)
            auto verifyFileSize = [](const std::string& path, int64_t expectedSize) -> bool {
                if (!fs::exists(path)) return false;
                int64_t actualSize = static_cast<int64_t>(fs::file_size(path));
                int64_t minSize = static_cast<int64_t>(expectedSize * 0.95);
                return actualSize >= minSize;
            };

            if (!verifyFileSize(flashDir + "/LQ_proj_in.ckpt", FLASHVSR_LQ_PROJ_SIZE)) return false;
            if (!verifyFileSize(flashDir + "/TCDecoder.ckpt", FLASHVSR_TCDECODER_SIZE)) return false;
            if (!verifyFileSize(flashDir + "/diffusion_pytorch_model_streaming_dmd.safetensors", FLASHVSR_DIFFUSION_SIZE)) return false;
        } catch (...) {
            return false;
        }

        return true;
    }

    return false;
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
    return getProgramDataPath() + "/EWOCvj2/models/upscale";
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
            return EDVR_L_SR_VIMEO_SIZE + EDVR_M_SR_SIZE;  // ~96MB total
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
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Step 1/4: Checking Python installation...";
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
    prog.status = "Step 1/4: Downloading Python 3.12...";
    prog.currentItem = "python-3.12.8-amd64.exe";
    prog.stepsCompleted = 1;
    prog.percentComplete = 25.0f;
    updateProgress(prog);

    std::string installerPath = tempDir + "\\python-3.12.8-amd64.exe";
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
    prog.status = "Step 2/4: Installing Python 3.12...";
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
    prog.status = "Step 3/4: Verifying Python installation...";
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
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Step 1/3: Checking Python installation...";
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
    prog.status = "Step 2/3: Installing PyTorch with CUDA support...";
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
    prog.status = "Step 3/3: Verifying PyTorch installation...";
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
    prog.status = "Starting...";
    updateProgress(prog);

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
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = VideoUpscalingInstallProgress::State::CHECKING;
    prog.status = "Checking EDVR models...";
    prog.filesTotal = 2;
    prog.filesCompleted = 0;
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

    // Get Python path (required for gdown)
    // Use locking to wait if another installer is currently installing Python
    std::string pythonPath;
    {
        auto isInstalledFn = [&pythonPath]() {
            return isPythonInstalled(pythonPath);
        };

        // Don't install Python ourselves - just wait for another installer to finish if lock is held
        auto installFn = []() -> bool {
            return false;  // We won't install, just waited for lock
        };

        VideoUpscalingInstaller* self = this;
        VideoUpscalingInstallProgress* progPtr = &prog;

        bool pythonReady = installPrerequisiteWithLock(
            PrerequisiteIds::PYTHON312,
            isInstalledFn,
            installFn,
            5000,
            [self, progPtr](const std::string&) {
                progPtr->status = "Prerequisites: Waiting for Python installation by another installer...";
                self->updateProgress(*progPtr);
            }
        );

        if (!pythonReady) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = "Python not installed - required for downloading EDVR models";
            prog.errorMessage = "Please install Python first (or wait for another installer to complete)";
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    // Create directory
    createDirectories(modelsDir);

    // EDVR models to download from Google Drive
    struct EDVRModel {
        const char* gdriveId;
        const char* filename;
        const char* description;
        int64_t expectedSize;
    };

    EDVRModel models[] = {
        {EDVR_L_SR_VIMEO_GDRIVE_ID, EDVR_L_SR_VIMEO_FILENAME, "EDVR-L Vimeo90K (BALANCED)", EDVR_L_SR_VIMEO_SIZE},
        {EDVR_M_SR_GDRIVE_ID, EDVR_M_SR_FILENAME, "EDVR-M (FAST)", EDVR_M_SR_SIZE}
    };

    for (int i = 0; i < 2; i++) {
        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        std::string localPath = modelsDir + "/" + models[i].filename;

        // Check if already exists
        if (verifyFile(localPath, models[i].expectedSize)) {
            prog.filesCompleted = i + 1;
            prog.percentComplete = ((i + 1) / 2.0f) * 100.0f;
            updateProgress(prog);
            continue;
        }

        prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
        prog.status = std::string("Downloading ") + models[i].description + " via gdown...";
        prog.currentFile = localPath;
        prog.currentItem = models[i].filename;
        prog.percentComplete = (i / 2.0f) * 100.0f;
        updateProgress(prog);

        if (!downloadFromGoogleDrive(models[i].gdriveId, localPath, pythonPath)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = std::string("Failed to download ") + models[i].description;
            prog.errorMessage = getLastError();
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Verify download
        if (!verifyFile(localPath, models[i].expectedSize)) {
            prog.state = VideoUpscalingInstallProgress::State::FAILED;
            prog.status = std::string("Downloaded file size mismatch: ") + models[i].filename;
            prog.errorMessage = "File may be corrupted or incomplete";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.filesCompleted = i + 1;
    }

    // Write installation manifest
    InstallManifest manifest;
    manifest.componentId = "edvr_models";
    manifest.componentName = "EDVR Video Upscaling Models";
    manifest.version = "1.0";
    manifest.complete = true;
    manifest.addFile(EDVR_L_SR_VIMEO_FILENAME, EDVR_L_SR_VIMEO_SIZE);
    manifest.addFile(EDVR_M_SR_FILENAME, EDVR_M_SR_SIZE);
    InstallVerification::writeManifest(modelsDir, manifest);

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "EDVR models installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installFlashVSRThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

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

    // Write installation manifest
    InstallManifest manifest;
    manifest.componentId = "flashvsr_models";
    manifest.componentName = "FlashVSR Video Upscaling Models";
    manifest.version = "1.1";
    manifest.complete = true;
    manifest.addFile("FlashVSR-v1.1/LQ_proj_in.ckpt", FLASHVSR_LQ_PROJ_SIZE);
    manifest.addFile("FlashVSR-v1.1/TCDecoder.ckpt", FLASHVSR_TCDECODER_SIZE);
    manifest.addFile("FlashVSR-v1.1/diffusion_pytorch_model_streaming_dmd.safetensors", FLASHVSR_DIFFUSION_SIZE);
    InstallVerification::writeManifest(modelsDir, manifest);

    prog.state = VideoUpscalingInstallProgress::State::COMPLETE;
    prog.status = "FlashVSR models installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void VideoUpscalingInstaller::installAllThread(VideoUpscalingInstallConfig config) {
    VideoUpscalingInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.stepsCompleted = 0;

    // Calculate total steps based on what's being installed
    // EDVR needs: Python, PyTorch, packages, EDVR models (4 steps)
    // FlashVSR needs: just FlashVSR models (1 step, no Python required)
    int totalSteps = 0;
    if (config.installEDVR) totalSteps += 4;  // Python + PyTorch + packages + EDVR
    if (config.installFlashVSR) totalSteps += 1;  // FlashVSR only
    prog.stepsTotal = totalSteps;

    std::string pythonPath;
    int currentStep = 0;

    // Python, PyTorch, and packages are ONLY needed for EDVR (uses gdown for Google Drive downloads)
    // FlashVSR uses direct HTTP downloads and doesn't need Python
    if (config.installEDVR) {
        // Step: Install Python (with lock to prevent multiple installers)
        currentStep++;
        prog.state = VideoUpscalingInstallProgress::State::CHECKING;
        prog.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Checking Python...";
        updateProgress(prog);

        // Use locking to ensure only one installer installs Python at a time
        {
            VideoUpscalingInstallConfig configCopy = config;
            VideoUpscalingInstaller* self = this;
            std::atomic<bool>* cancelFlag = &shouldCancel;
            VideoUpscalingInstallProgress* progPtr = &prog;
            int stepNum = currentStep;
            int stepTotal = totalSteps;

            auto isInstalledFn = [&pythonPath]() {
                return isPythonInstalled(pythonPath);
            };

            auto installFn = [self, configCopy, cancelFlag, progPtr, &pythonPath, stepNum, stepTotal]() -> bool {
                progPtr->status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Installing Python 3.12...";
                self->updateProgress(*progPtr);

                std::string pythonDir = configCopy.pythonInstallDir.empty() ? getDefaultPythonDir() : configCopy.pythonInstallDir;
                std::string tempDir = configCopy.tempDir.empty() ? (pythonDir + "\\temp") : configCopy.tempDir;
                self->createDirectories(tempDir);

                // Download
                progPtr->state = VideoUpscalingInstallProgress::State::DOWNLOADING;
                progPtr->currentItem = "python-3.12.8-amd64.exe";
                self->updateProgress(*progPtr);

                std::string installerPath = tempDir + "\\python-3.12.8-amd64.exe";
                if (!self->downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
                    return false;
                }

                if (cancelFlag->load()) {
                    self->deleteFile(installerPath);
                    return false;
                }

                // Install
                progPtr->state = VideoUpscalingInstallProgress::State::INSTALLING;
                progPtr->status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Installing Python 3.12...";
                self->updateProgress(*progPtr);

                if (!self->runPythonInstaller(installerPath, pythonDir)) {
                    return false;
                }

                pythonPath = pythonDir + "\\python.exe";
                if (configCopy.setSystemEnvVar) {
                    setEnvironmentVariable(pythonPath, true);
                }

                self->deleteFile(installerPath);
                return true;
            };

            bool pythonReady = installPrerequisiteWithLock(
                PrerequisiteIds::PYTHON312,
                isInstalledFn,
                installFn,
                5000,
                [self, progPtr, stepNum, stepTotal](const std::string&) {
                    progPtr->status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Waiting for Python installation by another installer...";
                    self->updateProgress(*progPtr);
                }
            );

            if (!pythonReady && !shouldCancel.load()) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to install Python";
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }

            // Re-check pythonPath if it wasn't set (another installer might have installed it)
            if (pythonPath.empty()) {
                isPythonInstalled(pythonPath);
            }
        }

        prog.stepsCompleted = currentStep;
        prog.percentComplete = (static_cast<float>(currentStep) / totalSteps) * 100.0f;

        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Step: Install PyTorch (with lock to prevent multiple installers)
        currentStep++;
        {
            VideoUpscalingInstaller* self = this;
            std::string pythonPathCopy = pythonPath;
            VideoUpscalingInstallConfig configCopy = config;
            std::atomic<bool>* cancelFlag = &shouldCancel;
            VideoUpscalingInstallProgress* progPtr = &prog;
            int stepNum = currentStep;
            int stepTotal = totalSteps;

            auto isInstalledFn = [pythonPathCopy]() {
                return isPyTorchInstalled(pythonPathCopy);
            };

            auto installFn = [self, pythonPathCopy, configCopy, cancelFlag, progPtr, stepNum, stepTotal]() -> bool {
                progPtr->state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
                progPtr->status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Installing PyTorch with CUDA...";
                progPtr->currentItem = "torch, torchvision, torchaudio";
                self->updateProgress(*progPtr);

                if (cancelFlag->load()) {
                    return false;
                }

                std::string indexUrl = self->getPyTorchIndexUrl(configCopy.cudaVersion);
                std::vector<std::string> packages = {"torch", "torchvision", "torchaudio"};

                if (!self->runPipInstallMultiple(pythonPathCopy, packages, indexUrl)) {
                    return false;
                }

                return true;
            };

            bool pytorchReady = installPrerequisiteWithLock(
                PrerequisiteIds::PYTORCH_CUDA,
                isInstalledFn,
                installFn,
                5000,
                [self, progPtr, stepNum, stepTotal](const std::string&) {
                    progPtr->status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Waiting for PyTorch installation by another installer...";
                    self->updateProgress(*progPtr);
                }
            );

            if (!pytorchReady && !shouldCancel.load()) {
                prog.state = VideoUpscalingInstallProgress::State::FAILED;
                prog.status = "Failed to install PyTorch";
                prog.errorMessage = getLastError();
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.stepsCompleted = currentStep;
        prog.percentComplete = (static_cast<float>(currentStep) / totalSteps) * 100.0f;

        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Step: Install Python packages (including gdown for Google Drive downloads)
        currentStep++;
        prog.state = VideoUpscalingInstallProgress::State::INSTALLING_PACKAGES;
        prog.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Installing Python packages...";
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

        prog.stepsCompleted = currentStep;
        prog.percentComplete = (static_cast<float>(currentStep) / totalSteps) * 100.0f;

        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    // Step: Install EDVR models via gdown (Google Drive)
    std::string modelsDir = config.modelsDir.empty() ? getDefaultModelsDir() : config.modelsDir;

    if (config.installEDVR) {
        currentStep++;

        if (!isEDVRInstalled(modelsDir)) {
            prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
            prog.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Downloading EDVR models via gdown...";
            updateProgress(prog);

            createDirectories(modelsDir);

            // EDVR models to download from Google Drive
            struct EDVRModel {
                const char* gdriveId;
                const char* filename;
                const char* description;
                int64_t expectedSize;
            };

            EDVRModel models[] = {
                {EDVR_L_SR_VIMEO_GDRIVE_ID, EDVR_L_SR_VIMEO_FILENAME, "EDVR-L Vimeo90K (BALANCED)", EDVR_L_SR_VIMEO_SIZE},
                {EDVR_M_SR_GDRIVE_ID, EDVR_M_SR_FILENAME, "EDVR-M (FAST)", EDVR_M_SR_SIZE}
            };

            for (int i = 0; i < 2; i++) {
                if (shouldCancel.load()) {
                    prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
                    prog.status = "Installation cancelled";
                    updateProgress(prog);
                    installing.store(false);
                    return;
                }

                std::string localPath = modelsDir + "/" + models[i].filename;

                // Skip if already exists
                if (verifyFile(localPath, models[i].expectedSize)) {
                    continue;
                }

                prog.currentItem = models[i].description;
                prog.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Downloading " + models[i].description + "...";
                updateProgress(prog);

                if (!downloadFromGoogleDrive(models[i].gdriveId, localPath, pythonPath)) {
                    prog.state = VideoUpscalingInstallProgress::State::FAILED;
                    prog.status = std::string("Failed to download ") + models[i].description;
                    prog.errorMessage = getLastError();
                    updateProgress(prog);
                    installing.store(false);
                    return;
                }
            }
        }

        prog.stepsCompleted = currentStep;
        prog.percentComplete = (static_cast<float>(currentStep) / totalSteps) * 100.0f;

        if (shouldCancel.load()) {
            prog.state = VideoUpscalingInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    // Step: Install FlashVSR models (no Python required - direct HTTP downloads)
    if (config.installFlashVSR) {
        currentStep++;

        if (!isFlashVSRInstalled(modelsDir)) {
            prog.state = VideoUpscalingInstallProgress::State::DOWNLOADING;
            prog.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Downloading FlashVSR models...";
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

        prog.stepsCompleted = currentStep;
        prog.percentComplete = (static_cast<float>(currentStep) / totalSteps) * 100.0f;
    }

    // Write installation manifests for models (if installed)
    if (config.installEDVR) {
        InstallManifest manifest;
        manifest.componentId = "edvr_models";
        manifest.componentName = "EDVR Video Upscaling Models";
        manifest.version = "1.0";
        manifest.complete = true;
        manifest.addFile(EDVR_L_SR_VIMEO_FILENAME, EDVR_L_SR_VIMEO_SIZE);
        manifest.addFile(EDVR_M_SR_FILENAME, EDVR_M_SR_SIZE);
        InstallVerification::writeManifest(modelsDir, manifest);
    }

    if (config.installFlashVSR) {
        InstallManifest manifest;
        manifest.componentId = "flashvsr_models";
        manifest.componentName = "FlashVSR Video Upscaling Models";
        manifest.version = "1.1";
        manifest.complete = true;
        manifest.addFile("FlashVSR-v1.1/LQ_proj_in.ckpt", FLASHVSR_LQ_PROJ_SIZE);
        manifest.addFile("FlashVSR-v1.1/TCDecoder.ckpt", FLASHVSR_TCDECODER_SIZE);
        manifest.addFile("FlashVSR-v1.1/diffusion_pytorch_model_streaming_dmd.safetensors", FLASHVSR_DIFFUSION_SIZE);
        InstallVerification::writeManifest(modelsDir, manifest);
    }

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

    // Enable automatic redirect following (GitHub releases use multiple redirects)
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

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
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to send request (error: " + std::to_string(err) + ")");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to receive response (error: " + std::to_string(err) + ")");
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
    auto lastProgressUpdate = std::chrono::steady_clock::now();

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

        // Update progress (throttled to avoid UI spam)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastProgressUpdate).count() > 0.25f) {
            VideoUpscalingInstallProgress prog;
            {
                std::lock_guard<std::mutex> lock(progressMutex);
                prog = progress;
            }
            prog.bytesDownloaded = totalBytesRead;
            prog.bytesTotal = expectedSize;

            // Calculate percentage from bytes
            if (expectedSize > 0) {
                prog.percentComplete = (static_cast<float>(totalBytesRead) / expectedSize) * 100.0f;
            }

            // Update status with download progress
            std::string sizeStr;
            if (expectedSize > 0) {
                sizeStr = " (" + std::to_string(totalBytesRead / (1024 * 1024)) + " / " +
                          std::to_string(expectedSize / (1024 * 1024)) + " MB)";
            } else {
                sizeStr = " (" + std::to_string(totalBytesRead / (1024 * 1024)) + " MB)";
            }
            prog.status = "Downloading" + sizeStr;

            updateProgress(prog);
            lastProgressUpdate = now;
        }
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

bool VideoUpscalingInstaller::downloadFromGoogleDrive(const std::string& fileId,
                                                       const std::string& localPath,
                                                       const std::string& pythonPath) {
    // Use gdown to download from Google Drive
    // --fuzzy: handles virus scan warning for large files
    // --continue: resume partial downloads

    // Fix paths for Windows - convert forward slashes to backslashes
    std::string fixedPath = localPath;
    std::string fixedPythonPath = pythonPath;
#ifdef _WIN32
    for (char& c : fixedPath) {
        if (c == '/') c = '\\';
    }
    for (char& c : fixedPythonPath) {
        if (c == '/') c = '\\';
    }
#endif

    // Build the command for debugging
    std::string debugCmd = "\"" + fixedPythonPath + "\" -m gdown --fuzzy --id " + fileId + " -O \"" + fixedPath + "\"";

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        setError("Failed to create pipe for gdown");
        return false;
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {0};

    // Run Python directly without cmd.exe wrapper to avoid quote parsing issues
    std::string cmdLine = "\"" + fixedPythonPath + "\" -m gdown --fuzzy --id " + fileId + " -O \"" + fixedPath + "\"";
    char* cmdBuf = _strdup(cmdLine.c_str());

    BOOL success = CreateProcessA(fixedPythonPath.c_str(), cmdBuf, nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    free(cmdBuf);

    if (!success) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        setError("Failed to start gdown process (error " + std::to_string(GetLastError()) + ")");
        return false;
    }

    CloseHandle(hWritePipe);

    // Read output in real-time and parse progress
    char buffer[256];
    DWORD bytesRead;
    std::string lineBuffer;
    std::string lastOutput;  // Keep last output for error reporting
    std::string allOutput;   // Keep all output for debugging

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        if (shouldCancel.load()) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
            deleteFile(localPath);
            return false;
        }

        buffer[bytesRead] = '\0';
        allOutput += buffer;  // Capture everything
        lineBuffer += buffer;

        // Parse gdown progress output (e.g., "Downloading... 45.3% (38.1M/84.2M)")
        // or "Downloading... 38.1MB/84.2MB"
        size_t pos;
        while ((pos = lineBuffer.find('\r')) != std::string::npos ||
               (pos = lineBuffer.find('\n')) != std::string::npos) {
            std::string line = lineBuffer.substr(0, pos);
            lineBuffer = lineBuffer.substr(pos + 1);

            // Keep last non-empty line for error reporting
            if (!line.empty()) {
                lastOutput = line;
            }

            // Look for percentage in the line
            size_t pctPos = line.find('%');
            if (pctPos != std::string::npos) {
                // Find the number before %
                size_t numStart = pctPos;
                while (numStart > 0 && (isdigit(line[numStart - 1]) || line[numStart - 1] == '.')) {
                    numStart--;
                }
                if (numStart < pctPos) {
                    try {
                        float pct = std::stof(line.substr(numStart, pctPos - numStart));
                        VideoUpscalingInstallProgress prog;
                        {
                            std::lock_guard<std::mutex> lock(progressMutex);
                            prog = progress;
                        }
                        prog.status = "Downloading: " + line;
                        prog.percentComplete = pct;
                        updateProgress(prog);
                    } catch (...) {}
                }
            } else if (line.find("Downloading") != std::string::npos) {
                // Update status with current line
                VideoUpscalingInstallProgress prog;
                {
                    std::lock_guard<std::mutex> lock(progressMutex);
                    prog = progress;
                }
                prog.status = line;
                updateProgress(prog);
            }
        }
    }

    // Wait for process
    DWORD waitResult = WaitForSingleObject(pi.hProcess, currentConfig.downloadTimeout);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        setError("gdown timed out");
        return false;
    }

    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    if (dwExitCode != 0) {
        std::string errMsg = "gdown failed with exit code " + std::to_string(dwExitCode);
        errMsg += " | cmd: " + debugCmd;
        if (!lastOutput.empty()) {
            errMsg += " | output: " + lastOutput;
        } else if (!allOutput.empty()) {
            // Truncate if too long
            std::string truncated = allOutput.substr(0, 300);
            errMsg += " | output: " + truncated;
        }
        setError(errMsg);
        return false;
    }

    // Verify file was created
    if (!fileExists(localPath)) {
        setError("gdown completed but file not found: " + localPath);
        return false;
    }

    return true;
#else
    // Unix: use popen with real-time reading
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        setError("Failed to start gdown process");
        return false;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        // Update progress with current line
        VideoUpscalingInstallProgress prog;
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            prog = progress;
        }
        prog.status = "Downloading: " + line;
        updateProgress(prog);
    }

    int exitCode = pclose(pipe);
    if (exitCode != 0) {
        setError("gdown failed with exit code " + std::to_string(exitCode));
        return false;
    }

    if (!fileExists(localPath)) {
        setError("gdown completed but file not found: " + localPath);
        return false;
    }

    return true;
#endif
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
    // Retry loop handles file access issues (antivirus scanning, file system delays)
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = "cmd.exe";
    std::string args = "/c \"" + cmd + "\"";
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;

    bool launched = false;
    DWORD lastErr = 0;
    for (int attempt = 0; attempt < 5 && !launched; attempt++) {
        if (attempt > 0) {
            Sleep(1000);
        }
        if (ShellExecuteExA(&sei)) {
            launched = true;
        } else {
            lastErr = GetLastError();
            if (lastErr != ERROR_SHARING_VIOLATION &&
                lastErr != ERROR_LOCK_VIOLATION &&
                lastErr != ERROR_ACCESS_DENIED) {
                break;
            }
        }
    }

    if (!launched) {
        setError("Failed to launch Python installer (error " + std::to_string(lastErr) + ")");
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
    std::string cmd = "\"" + pythonPath + "\" -m pip install --user --no-cache-dir --upgrade " + package;
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

    std::string cmd = "\"" + pythonPath + "\" -m pip install --user --no-cache-dir --upgrade " + packageList;
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
    // EDVR models are downloaded via gdown from Google Drive
    // URLs are Google Drive direct links (actual download uses gdown with file IDs)
    std::string gdriveUrlL = std::string("https://drive.google.com/uc?id=") + EDVR_L_SR_VIMEO_GDRIVE_ID;
    std::string gdriveUrlM = std::string("https://drive.google.com/uc?id=") + EDVR_M_SR_GDRIVE_ID;

    return {
        {gdriveUrlL, EDVR_L_SR_VIMEO_FILENAME,
         "EDVR-L 4x SR Vimeo90K", EDVR_L_SR_VIMEO_SIZE, true},
        {gdriveUrlM, EDVR_M_SR_FILENAME,
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
