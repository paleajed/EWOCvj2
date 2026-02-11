/**
 * ReCoNetInstaller.cpp
 *
 * Implementation of Python and ReCoNet dependencies installation
 *
 * License: GPL3
 */

#include "ReCoNetInstaller.h"
#include "InstallVerification.h"
#include "VideoDatasetDownloader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <shellapi.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#else
#include <curl/curl.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

// ============================================================================
// Constructor / Destructor
// ============================================================================

ReCoNetInstaller::ReCoNetInstaller() {
}

ReCoNetInstaller::~ReCoNetInstaller() {
    cancelInstallation();
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
}

// ============================================================================
// Installation Methods
// ============================================================================

bool ReCoNetInstaller::installPython(const ReCoNetInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    currentConfig = config;
    if (currentConfig.pythonInstallDir.empty()) {
        currentConfig.pythonInstallDir = getDefaultPythonDir();
    }

    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ReCoNetInstaller::installPythonThread, this, currentConfig);

    return true;
}

bool ReCoNetInstaller::installPyTorch(const ReCoNetInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    // Check if Python is installed
    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        setError("Python not installed. Install Python first.");
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ReCoNetInstaller::installPyTorchThread, this, currentConfig);

    return true;
}

bool ReCoNetInstaller::installPythonPackages(const ReCoNetInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        setError("Python not installed. Install Python first.");
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ReCoNetInstaller::installPythonPackagesThread, this, currentConfig);

    return true;
}

bool ReCoNetInstaller::installAll(const ReCoNetInstallConfig& config) {
    if (installing.load()) {
        std::string errMsg = "Installation already in progress";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = ReCoNetInstallProgress::State::FAILED;
        progress.errorMessage = errMsg;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[ReCoNetInstaller] %s\n", errMsg.c_str());
        return false;
    }

    currentConfig = config;
    if (currentConfig.pythonInstallDir.empty()) {
        currentConfig.pythonInstallDir = getDefaultPythonDir();
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(ReCoNetComponent::ALL_COMPONENTS);
    int64_t available = getFreeDiskSpace(currentConfig.pythonInstallDir);
    if (available > 0 && available < required) {
        std::string errMsg = "Insufficient disk space. Required: " + formatSize(required) +
                             ", Available: " + formatSize(available);
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = ReCoNetInstallProgress::State::FAILED;
        progress.errorMessage = errMsg;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[ReCoNetInstaller] %s\n", errMsg.c_str());
        return false;
    }

    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ReCoNetInstaller::installAllThread, this, currentConfig);

    return true;
}

void ReCoNetInstaller::cancelInstallation() {
    shouldCancel.store(true);
}

ReCoNetInstallProgress ReCoNetInstaller::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void ReCoNetInstaller::setProgressCallback(std::function<void(const ReCoNetInstallProgress&)> callback) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progressCallback = std::move(callback);
}

// ============================================================================
// Verification Methods
// ============================================================================

bool ReCoNetInstaller::isPythonInstalled(std::string& pythonPath) {
    // Helper lambda to check if a Python path is specifically version 3.12.x
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
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0) return false;

        // Check if output contains "Python 3.12"
        // Output format: "Python 3.12.8" or similar
        if (output.find("Python 3.12") != std::string::npos) {
            return true;
        }

        return false;
#else
        // Unix implementation
        FILE* pipe = popen((path + " --version 2>&1").c_str(), "r");
        if (!pipe) return false;

        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        int result = pclose(pipe);

        if (result != 0) return false;

        return output.find("Python 3.12") != std::string::npos;
#endif
    };

    // First check EWOCVJ2_PYTHON environment variable - must be Python 3.12
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
        "C:\\Python312\\python.exe"
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

bool ReCoNetInstaller::isPyTorchInstalled(const std::string& pythonPath) {
    if (pythonPath.empty() || !fs::exists(pythonPath)) {
        return false;
    }

    std::string output;
    int exitCode;
    std::string cmd = "\"" + pythonPath + "\" -c \"import torch; print(torch.cuda.is_available())\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = cmd;
    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);

    char buffer[256];
    DWORD bytesRead;
    output.clear();
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    // Check if output contains "True" (CUDA available)
    return (dwExitCode == 0 && output.find("True") != std::string::npos);
}

bool ReCoNetInstaller::arePackagesInstalled(const std::string& pythonPath) {
    if (pythonPath.empty() || !fs::exists(pythonPath)) {
        return false;
    }

    // Check for key packages
    const char* packages[] = {"numpy", "PIL", "skimage", "cv2", "onnx"};

    for (const char* pkg : packages) {
        std::string cmd = "\"" + pythonPath + "\" -c \"import " + pkg + "\"";

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi, sizeof(pi));

        std::string cmdCopy = cmd;
        if (CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 10000);
            DWORD dwExitCode;
            GetExitCodeProcess(pi.hProcess, &dwExitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (dwExitCode != 0) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool ReCoNetInstaller::isFullyInstalled() {
    // First check manifest for verified installation
    std::string pythonDir = getDefaultPythonDir();
    auto result = InstallVerification::verifyInstallation(pythonDir, "reconet_environment");
    if (result.isValid()) {
        // Also check dataset
        if (!isDatasetDownloaded()) {
            return false;
        }
        return true;
    }

    // Fall back to runtime checking (backwards compatibility)
    if (!result.manifestExists) {
        std::string pythonPath;
        if (!isPythonInstalled(pythonPath)) {
            return false;
        }

        if (!isPyTorchInstalled(pythonPath)) {
            return false;
        }

        if (!arePackagesInstalled(pythonPath)) {
            return false;
        }

        // Also check dataset
        if (!isDatasetDownloaded()) {
            return false;
        }

        return true;
    }

    // Manifest exists but verification failed
    return false;
}

bool ReCoNetInstaller::isDatasetDownloaded() {
    // Check for content images (at least 200 COCO images)
    std::string contentDir = "C:/ProgramData/EWOCvj2/datasets/content";
    bool contentOk = false;

    if (fs::exists(contentDir) && fs::is_directory(contentDir)) {
        int imageCount = 0;
        for (const auto& entry : fs::directory_iterator(contentDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
                    imageCount++;
                    if (imageCount >= 200) {
                        contentOk = true;
                        break;
                    }
                }
            }
        }
    }

    // Check for video frame sequences (at least 400 sequences for temporal training)
    std::string videoDir = "C:/ProgramData/EWOCvj2/datasets/video";
    bool videoOk = false;

    if (fs::exists(videoDir) && fs::is_directory(videoDir)) {
        int seqCount = 0;
        for (const auto& entry : fs::directory_iterator(videoDir)) {
            if (entry.is_directory()) {
                std::string name = entry.path().filename().string();
                if (name.rfind("seq_", 0) == 0) {  // starts with "seq_"
                    seqCount++;
                    if (seqCount >= 400) {
                        videoOk = true;
                        break;
                    }
                }
            }
        }
    }

    return contentOk && videoOk;
}

std::string ReCoNetInstaller::getPythonPath() {
    std::string path;
    if (isPythonInstalled(path)) {
        return path;
    }
    return "";
}

std::string ReCoNetInstaller::getDefaultPythonDir() {
    return "C:\\Python312";
}

int64_t ReCoNetInstaller::getRequiredDiskSpace(ReCoNetComponent component) {
    switch (component) {
        case ReCoNetComponent::PYTHON_312:
            return 150LL * 1024 * 1024;  // ~150MB for Python

        case ReCoNetComponent::PYTORCH_CUDA:
            return 3LL * 1024 * 1024 * 1024;  // ~3GB for PyTorch with CUDA

        case ReCoNetComponent::PYTHON_PACKAGES:
            return 500LL * 1024 * 1024;  // ~500MB for additional packages

        case ReCoNetComponent::ALL_COMPONENTS:
            return getRequiredDiskSpace(ReCoNetComponent::PYTHON_312) +
                   getRequiredDiskSpace(ReCoNetComponent::PYTORCH_CUDA) +
                   getRequiredDiskSpace(ReCoNetComponent::PYTHON_PACKAGES);

        default:
            return 0;
    }
}

std::string ReCoNetInstaller::formatSize(int64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unitIndex];
    return oss.str();
}

// ============================================================================
// Environment Variable Methods
// ============================================================================

bool ReCoNetInstaller::setEnvironmentVariable(const std::string& pythonPath, bool systemWide) {
#ifdef _WIN32
    if (systemWide) {
        // Set system environment variable (requires admin)
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
            0, KEY_SET_VALUE, &hKey);

        if (result != ERROR_SUCCESS) {
            // Try user environment instead
            result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey);
            if (result != ERROR_SUCCESS) {
                return false;
            }
        }

        result = RegSetValueExA(hKey, "EWOCVJ2_PYTHON", 0, REG_SZ,
            (const BYTE*)pythonPath.c_str(), (DWORD)(pythonPath.length() + 1));

        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS) {
            // Broadcast environment change
            DWORD_PTR dwResult;
            SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &dwResult);
            return true;
        }
        return false;
    } else {
        // Set user environment variable
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey);
        if (result != ERROR_SUCCESS) {
            return false;
        }

        result = RegSetValueExA(hKey, "EWOCVJ2_PYTHON", 0, REG_SZ,
            (const BYTE*)pythonPath.c_str(), (DWORD)(pythonPath.length() + 1));

        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS) {
            DWORD_PTR dwResult;
            SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &dwResult);
            return true;
        }
        return false;
    }
#else
    // On Unix, we would modify ~/.bashrc or similar
    // For now, just set for current process
    return setenv("EWOCVJ2_PYTHON", pythonPath.c_str(), 1) == 0;
#endif
}

std::string ReCoNetInstaller::getEnvironmentVariable() {
    const char* value = getenv("EWOCVJ2_PYTHON");
    if (value) {
        return std::string(value);
    }

#ifdef _WIN32
    // Also check registry
    HKEY hKey;
    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);

    // Try system first
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueExA(hKey, "EWOCVJ2_PYTHON", NULL, NULL,
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }

    // Try user
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "EWOCVJ2_PYTHON", NULL, NULL,
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
#endif

    return "";
}

// ============================================================================
// Error Handling
// ============================================================================

std::string ReCoNetInstaller::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void ReCoNetInstaller::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ============================================================================
// Installation Threads
// ============================================================================

void ReCoNetInstaller::installPythonThread(ReCoNetInstallConfig config) {
    ReCoNetInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = ReCoNetInstallProgress::State::CHECKING;
    prog.status = "Checking for existing Python installation...";
    prog.stepsTotal = 4;
    prog.stepsCompleted = 0;
    updateProgress(prog);

    std::string existingPython;
    if (isPythonInstalled(existingPython)) {
        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "Python already installed at: " + existingPython;
        prog.percentComplete = 100.0f;
        updateProgress(prog);

        // Set environment variable
        setEnvironmentVariable(existingPython, config.setSystemEnvVar);

        installing.store(false);
        return;
    }

    // Create temp directory
    std::string tempDir = config.tempDir;
    if (tempDir.empty()) {
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        tempDir = std::string(tempPath) + "EWOCvj2_install";
    }
    createDirectories(tempDir);

    std::string installerPath = tempDir + "\\python-3.12.8-amd64.exe";

    // Download Python installer
    prog.state = ReCoNetInstallProgress::State::DOWNLOADING;
    prog.status = "Downloading Python 3.12.8...";
    prog.stepsCompleted = 1;
    prog.percentComplete = 25.0f;
    updateProgress(prog);

    if (!downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to download Python installer: " + getLastError();
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Run Python installer
    prog.state = ReCoNetInstallProgress::State::INSTALLING;
    prog.status = "Installing Python 3.12.8 (this may take a few minutes)...";
    prog.stepsCompleted = 2;
    prog.percentComplete = 50.0f;
    updateProgress(prog);

    if (!runPythonInstaller(installerPath, config.pythonInstallDir)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to install Python: " + getLastError();
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Verify installation
    prog.state = ReCoNetInstallProgress::State::VERIFYING;
    prog.status = "Verifying Python installation...";
    prog.stepsCompleted = 3;
    prog.percentComplete = 75.0f;
    updateProgress(prog);

    std::string pythonExe = config.pythonInstallDir + "\\python.exe";
    if (!fs::exists(pythonExe)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Python installation verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Set environment variable
    setEnvironmentVariable(pythonExe, config.setSystemEnvVar);

    // Cleanup installer
    try {
        fs::remove(installerPath);
    } catch (...) {}

    prog.state = ReCoNetInstallProgress::State::COMPLETE;
    prog.status = "Python 3.12.8 installed successfully";
    prog.stepsCompleted = 4;
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void ReCoNetInstaller::installPyTorchThread(ReCoNetInstallConfig config) {
    ReCoNetInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = ReCoNetInstallProgress::State::CHECKING;
    prog.status = "Checking Python installation...";
    prog.stepsTotal = 3;
    prog.stepsCompleted = 0;
    updateProgress(prog);

    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Python not found";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Check if already installed
    if (isPyTorchInstalled(pythonPath)) {
        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "PyTorch with CUDA already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Get pip path
    fs::path pythonDir = fs::path(pythonPath).parent_path();
    std::string pipPath = (pythonDir / "Scripts" / "pip.exe").string();

    if (!fs::exists(pipPath)) {
        // Try python -m pip instead
        pipPath = "";
    }

    // Install PyTorch with CUDA
    prog.state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
    prog.status = "Installing PyTorch with CUDA " + config.cudaVersion + " (this may take 10-20 minutes)...";
    prog.stepsCompleted = 1;
    prog.percentComplete = 33.0f;
    updateProgress(prog);

    std::string indexUrl = getPyTorchIndexUrl(config.cudaVersion);

    std::vector<std::string> packages = {"torch", "torchvision", "torchaudio"};
    if (!runPipInstallMultiple(pythonPath, packages, indexUrl)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to install PyTorch: " + getLastError();
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Verify CUDA support
    prog.state = ReCoNetInstallProgress::State::VERIFYING;
    prog.status = "Verifying PyTorch CUDA support...";
    prog.stepsCompleted = 2;
    prog.percentComplete = 66.0f;
    updateProgress(prog);

    if (!isPyTorchInstalled(pythonPath)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "PyTorch installed but CUDA not available";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    prog.state = ReCoNetInstallProgress::State::COMPLETE;
    prog.status = "PyTorch with CUDA installed successfully";
    prog.stepsCompleted = 3;
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void ReCoNetInstaller::installPythonPackagesThread(ReCoNetInstallConfig config) {
    ReCoNetInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = ReCoNetInstallProgress::State::CHECKING;
    prog.status = "Checking Python installation...";
    updateProgress(prog);

    std::string pythonPath;
    if (!isPythonInstalled(pythonPath)) {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "Python not found";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    std::vector<std::string> packages = {"numpy", "Pillow", "scikit-image", "opencv-python", "onnx"};
    prog.stepsTotal = static_cast<int>(packages.size());
    prog.stepsCompleted = 0;

    for (size_t i = 0; i < packages.size(); i++) {
        if (shouldCancel.load()) {
            prog.state = ReCoNetInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
        prog.currentItem = packages[i];
        prog.status = "Installing " + packages[i] + "...";
        prog.percentComplete = (static_cast<float>(i) / packages.size()) * 100.0f;
        updateProgress(prog);

        if (!runPipInstall(pythonPath, packages[i])) {
            prog.state = ReCoNetInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to install " + packages[i] + ": " + getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.stepsCompleted = static_cast<int>(i + 1);
    }

    prog.state = ReCoNetInstallProgress::State::COMPLETE;
    prog.status = "All packages installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void ReCoNetInstaller::installAllThread(ReCoNetInstallConfig config) {
    ReCoNetInstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    auto startTime = std::chrono::steady_clock::now();

    // Step 1: Install Python (with lock to prevent multiple installers)
    prog.state = ReCoNetInstallProgress::State::CHECKING;
    prog.status = "Step 1/7: Checking for Python 3.12...";
    prog.stepsTotal = 7;
    prog.stepsCompleted = 0;
    updateProgress(prog);

    std::string pythonPath;

    // Use locking to ensure only one installer installs Python at a time
    {
        ReCoNetInstallConfig configCopy = config;
        ReCoNetInstaller* self = this;
        std::atomic<bool>* cancelFlag = &shouldCancel;
        ReCoNetInstallProgress* progPtr = &prog;

        auto isInstalledFn = [&pythonPath]() {
            return isPythonInstalled(pythonPath);
        };

        auto installFn = [self, configCopy, cancelFlag, progPtr, &pythonPath]() -> bool {
            // Download Python
            std::string tempDir = configCopy.tempDir;
            if (tempDir.empty()) {
                char tempPath[MAX_PATH];
                GetTempPathA(MAX_PATH, tempPath);
                tempDir = std::string(tempPath) + "EWOCvj2_install";
            }
            self->createDirectories(tempDir);

            std::string installerPath = tempDir + "\\python-3.12.8-amd64.exe";

            progPtr->state = ReCoNetInstallProgress::State::DOWNLOADING;
            progPtr->status = "Step 1/7: Downloading Python 3.12.8...";
            progPtr->stepsCompleted = 1;
            progPtr->percentComplete = 10.0f;
            self->updateProgress(*progPtr);

            if (!self->downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
                return false;
            }

            if (cancelFlag->load()) {
                return false;
            }

            // Install Python
            progPtr->state = ReCoNetInstallProgress::State::INSTALLING;
            progPtr->status = "Step 1/7: Installing Python 3.12.8...";
            progPtr->stepsCompleted = 2;
            progPtr->percentComplete = 20.0f;
            self->updateProgress(*progPtr);

            if (!self->runPythonInstaller(installerPath, configCopy.pythonInstallDir)) {
                return false;
            }

            pythonPath = configCopy.pythonInstallDir + "\\python.exe";

            // Cleanup
            try { fs::remove(installerPath); } catch (...) {}

            return true;
        };

        bool pythonReady = installPrerequisiteWithLock(
            PrerequisiteIds::PYTHON312,
            isInstalledFn,
            installFn,
            5000,
            [self, progPtr](const std::string&) {
                progPtr->status = "Step 1/7: Waiting for Python installation by another installer...";
                self->updateProgress(*progPtr);
            }
        );

        if (!pythonReady) {
            prog.state = ReCoNetInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to install Python: " + getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Re-check pythonPath if it wasn't set (another installer might have installed it)
        if (pythonPath.empty()) {
            isPythonInstalled(pythonPath);
        }
    }

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    prog.stepsCompleted = 3;
    prog.percentComplete = 30.0f;

    // Set environment variable
    prog.status = "Step 2/7: Setting EWOCVJ2_PYTHON environment variable...";
    updateProgress(prog);
    setEnvironmentVariable(pythonPath, config.setSystemEnvVar);

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 2: Install PyTorch with CUDA (with lock to prevent multiple installers)
    {
        ReCoNetInstaller* self = this;
        std::string pythonPathCopy = pythonPath;
        ReCoNetInstallConfig configCopy = config;
        std::atomic<bool>* cancelFlag = &shouldCancel;
        ReCoNetInstallProgress* progPtr = &prog;

        auto isInstalledFn = [pythonPathCopy]() {
            return isPyTorchInstalled(pythonPathCopy);
        };

        auto installFn = [self, pythonPathCopy, configCopy, cancelFlag, progPtr]() -> bool {
            progPtr->state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
            progPtr->status = "Step 3/7: Installing PyTorch with CUDA " + configCopy.cudaVersion + " (this may take 10-20 minutes)...";
            progPtr->stepsCompleted = 4;
            progPtr->percentComplete = 40.0f;
            self->updateProgress(*progPtr);

            if (cancelFlag->load()) {
                return false;
            }

            std::string indexUrl = self->getPyTorchIndexUrl(configCopy.cudaVersion);
            std::vector<std::string> pytorchPkgs = {"torch", "torchvision", "torchaudio"};

            if (!self->runPipInstallMultiple(pythonPathCopy, pytorchPkgs, indexUrl)) {
                return false;
            }

            return true;
        };

        bool pytorchReady = installPrerequisiteWithLock(
            PrerequisiteIds::PYTORCH_CUDA,
            isInstalledFn,
            installFn,
            5000,
            [self, progPtr](const std::string&) {
                progPtr->status = "Step 3/7: Waiting for PyTorch installation by another installer...";
                self->updateProgress(*progPtr);
            }
        );

        if (!pytorchReady && !shouldCancel.load()) {
            prog.state = ReCoNetInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to install PyTorch: " + getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }
    }

    prog.stepsCompleted = 5;
    prog.percentComplete = 70.0f;

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 4: Install additional packages
    prog.state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
    prog.status = "Step 4/7: Installing additional packages...";
    prog.stepsCompleted = 6;
    prog.percentComplete = 80.0f;
    updateProgress(prog);

    std::vector<std::string> additionalPkgs = {"numpy", "Pillow", "scikit-image", "opencv-python", "onnx"};

    for (const auto& pkg : additionalPkgs) {
        if (shouldCancel.load()) {
            prog.state = ReCoNetInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.currentItem = pkg;
        prog.status = "Step 4/7: Installing " + pkg + "...";
        updateProgress(prog);

        if (!runPipInstall(pythonPath, pkg)) {
            // Non-fatal for additional packages, continue
            std::cerr << "[ReCoNetInstaller] Warning: Failed to install " << pkg << std::endl;
        }
    }

    // Step 5: Download training datasets if not already present
    if (!isDatasetDownloaded()) {
        std::string scriptsDir = "C:/ProgramData/EWOCvj2/scripts";

        // Download content images (COCO)
        prog.state = ReCoNetInstallProgress::State::DOWNLOADING;
        prog.status = "Step 5/7: Downloading content images (200 COCO images)...";
        prog.stepsCompleted = 7;
        prog.percentComplete = 85.0f;
        updateProgress(prog);

        std::string contentScript = scriptsDir + "/download_content.py";
        if (fs::exists(contentScript)) {
            // Run script with real-time progress updates
            // Script outputs lines like "COCO Content: 10/200 (10 OK, 0 skipped, 0 failed)"
            if (!runScriptWithProgress(pythonPath, contentScript, "COCO Content:", 600000)) {
                std::cerr << "[ReCoNetInstaller] Warning: Content download script failed" << std::endl;
            }
        } else {
            std::cerr << "[ReCoNetInstaller] Warning: download_content.py not found at " << contentScript << std::endl;
        }

        if (shouldCancel.load()) {
            prog.state = ReCoNetInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        // Download video frame pairs (Pexels) using C++ FFmpeg-based downloader
        prog.status = "Step 5/7: Downloading video frame pairs...";
        prog.percentComplete = 88.0f;
        updateProgress(prog);

        {
            VideoDatasetDownloader videoDownloader;
            VideoDatasetConfig videoConfig;
            videoConfig.targetSequences = 500;
            videoConfig.sequencesPerVideo = 20;
            videoConfig.targetResolution = 1024;
            videoConfig.jpegQuality = 90;

            // Forward progress to our progress callback
            videoDownloader.setProgressCallback([this, &prog](const VideoDatasetProgress& vp) {
                prog.status = vp.status;
                // Adjust percent within our 88-95% range
                float videoPercent = vp.percentComplete / 100.0f;
                prog.percentComplete = 88.0f + videoPercent * 7.0f;
                updateProgress(prog);
            });

            // Check for cancellation periodically
            videoDownloader.startDownload(videoConfig);

            // Wait for completion with cancellation check
            while (videoDownloader.isDownloading()) {
                if (shouldCancel.load()) {
                    videoDownloader.cancelDownload();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!shouldCancel.load()) {
                auto finalProgress = videoDownloader.getProgress();
                std::cerr << "[ReCoNetInstaller] Video download complete: " << finalProgress.sequencesCompleted
                          << " sequences from " << finalProgress.videosProcessed << " videos" << std::endl;
            }
        }
    } else {
        prog.status = "Step 5/7: Training datasets already present";
        updateProgress(prog);
    }

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Verification
    prog.state = ReCoNetInstallProgress::State::VERIFYING;
    prog.status = "Step 6/7: Verifying installation...";
    prog.stepsCompleted = 8;
    prog.percentComplete = 95.0f;
    updateProgress(prog);

    bool pytorchOk = isPyTorchInstalled(pythonPath);
    bool packagesOk = arePackagesInstalled(pythonPath);
    bool datasetOk = isDatasetDownloaded();

    auto endTime = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(endTime - startTime).count();

    if (pytorchOk && packagesOk && datasetOk) {
        // Write installation manifest
        InstallManifest manifest;
        manifest.componentId = "reconet_environment";
        manifest.componentName = "ReCoNet Training Environment";
        manifest.version = "3.12";
        manifest.complete = true;
        // For Python environment, track the python.exe location
        fs::path pythonDir = fs::path(pythonPath).parent_path();
        manifest.addFile(fs::path(pythonPath).filename().string(), 0);  // python.exe
        manifest.addDirectory("Lib/site-packages/torch");
        manifest.addDirectory("Lib/site-packages/numpy");
        InstallVerification::writeManifest(pythonDir.string(), manifest);

        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "ReCoNet training environment installed successfully";
        prog.percentComplete = 100.0f;
        prog.elapsedTime = elapsed;
    } else if (pytorchOk && packagesOk) {
        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "Installation complete (dataset download may have failed - 200 images required)";
        prog.percentComplete = 100.0f;
        prog.elapsedTime = elapsed;
    } else if (pytorchOk) {
        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "Installation complete (some optional packages may be missing)";
        prog.percentComplete = 100.0f;
        prog.elapsedTime = elapsed;
    } else {
        prog.state = ReCoNetInstallProgress::State::FAILED;
        prog.errorMessage = "PyTorch CUDA verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
    }

    updateProgress(prog);
    installing.store(false);
}

// ============================================================================
// Download Helpers
// ============================================================================

bool ReCoNetInstaller::downloadFile(const std::string& url, const std::string& localPath,
                                     int64_t expectedSize) {
#ifdef _WIN32
    // Parse URL
    std::wstring wurl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
        setError("Failed to parse URL");
        return false;
    }

    // Create parent directories
    fs::path localFilePath(localPath);
    if (localFilePath.has_parent_path()) {
        createDirectories(localFilePath.parent_path().string());
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-ReCoNet-Installer/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        setError("Failed to open HTTP session");
        return false;
    }

    // Set timeouts
    DWORD timeout = currentConfig.connectionTimeout;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, currentConfig.downloadTimeout);

    // Connect
    INTERNET_PORT port = urlComp.nPort;
    if (port == 0) {
        port = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        setError("Failed to connect to server");
        return false;
    }

    // Open request
    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                             NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to open HTTP request");
        return false;
    }

    // Enable redirects
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to send HTTP request");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to receive HTTP response");
        return false;
    }

    // Check status
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        NULL, &statusCode, &statusCodeSize, NULL);

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("HTTP error " + std::to_string(statusCode));
        return false;
    }

    // Open output file
    std::ofstream outFile(localPath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to create output file");
        return false;
    }

    // Download data
    char buffer[65536];
    DWORD bytesRead = 0;
    int64_t totalDownloaded = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (shouldCancel.load()) {
            outFile.close();
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        outFile.write(buffer, bytesRead);
        totalDownloaded += bytesRead;

        // Update progress
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.bytesDownloaded = totalDownloaded;
        progress.bytesTotal = expectedSize;
        if (progressCallback) {
            progressCallback(progress);
        }
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
#else
    // Linux implementation with libcurl would go here
    setError("Download not implemented on this platform");
    return false;
#endif
}

// ============================================================================
// Python Installation
// ============================================================================

bool ReCoNetInstaller::runPythonInstaller(const std::string& installerPath, const std::string& installDir) {
#ifdef _WIN32
    // Python installer silent options:
    // /quiet - silent install
    // InstallAllUsers=1 - install for all users
    // TargetDir=... - installation directory
    // PrependPath=1 - add to PATH
    // Include_pip=1 - include pip
    // Include_test=0 - don't include test suite

    std::string cmd = "\"" + installerPath + "\" /quiet InstallAllUsers=1 "
                      "TargetDir=\"" + installDir + "\" PrependPath=0 "
                      "Include_pip=1 Include_test=0 Include_launcher=0";

    std::cerr << "[ReCoNetInstaller] Running: " << cmd << std::endl;

    // Run with elevated privileges (required for InstallAllUsers=1 and system directories)
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
        if (lastErr == ERROR_CANCELLED) {
            setError("Python installation cancelled - admin rights required");
        } else {
            setError("Failed to launch Python installer (error " + std::to_string(lastErr) + ")");
        }
        return false;
    }

    // Wait for installer to complete (up to 10 minutes)
    DWORD waitResult = WaitForSingleObject(sei.hProcess, 600000);

    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(sei.hProcess, 1);
        CloseHandle(sei.hProcess);
        setError("Python installer timed out");
        return false;
    }

    DWORD exitCode;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);

    if (exitCode != 0) {
        setError("Python installer failed with exit code " + std::to_string(exitCode));
        return false;
    }

    return true;
#else
    setError("Python installation not implemented on this platform");
    return false;
#endif
}

bool ReCoNetInstaller::verifyPythonInstallation(const std::string& installDir) {
    std::string pythonExe = installDir + "\\python.exe";
    return fs::exists(pythonExe);
}

// ============================================================================
// Pip Operations
// ============================================================================

bool ReCoNetInstaller::runPipInstall(const std::string& pythonPath, const std::string& package,
                                      const std::string& indexUrl) {
    std::string cmd = "\"" + pythonPath + "\" -m pip install --user --no-cache-dir --upgrade " + package;
    if (!indexUrl.empty()) {
        cmd += " --index-url " + indexUrl;
    }

    std::cerr << "[ReCoNetInstaller] Running: " << cmd << std::endl;

    std::string output;
    int exitCode;
    if (!runCommand(cmd, output, exitCode, currentConfig.pipTimeout)) {
        return false;
    }

    return exitCode == 0;
}

bool ReCoNetInstaller::runPipInstallMultiple(const std::string& pythonPath,
                                              const std::vector<std::string>& packages,
                                              const std::string& indexUrl) {
    std::string pkgList;
    for (const auto& pkg : packages) {
        if (!pkgList.empty()) pkgList += " ";
        pkgList += pkg;
    }

    std::string cmd = "\"" + pythonPath + "\" -m pip install --user --no-cache-dir --upgrade " + pkgList;
    if (!indexUrl.empty()) {
        cmd += " --index-url " + indexUrl;
    }

    std::cerr << "[ReCoNetInstaller] Running: " << cmd << std::endl;

    std::string output;
    int exitCode;
    if (!runCommand(cmd, output, exitCode, currentConfig.pipTimeout)) {
        return false;
    }

    if (exitCode != 0) {
        setError("pip install failed: " + output);
        return false;
    }

    return true;
}

bool ReCoNetInstaller::checkPackageInstalled(const std::string& pythonPath, const std::string& package) {
    std::string cmd = "\"" + pythonPath + "\" -c \"import " + package + "\"";
    std::string output;
    int exitCode;
    runCommand(cmd, output, exitCode, 10000);
    return exitCode == 0;
}

// ============================================================================
// Command Execution
// ============================================================================

bool ReCoNetInstaller::runCommand(const std::string& command, std::string& output, int& exitCode,
                                   int timeoutMs) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        setError("Failed to create pipe");
        return false;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = command;
    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        setError("Failed to create process");
        return false;
    }

    CloseHandle(hWritePipe);

    // Read output in a separate thread to avoid deadlock
    output.clear();
    std::thread readThread([&]() {
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
    });

    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);

    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        readThread.join();
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        setError("Command timed out");
        exitCode = -1;
        return false;
    }

    readThread.join();

    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    exitCode = static_cast<int>(dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return true;
#else
    setError("Command execution not implemented on this platform");
    return false;
#endif
}

bool ReCoNetInstaller::runCommandElevated(const std::string& command, int timeoutMs) {
#ifdef _WIN32
    SHELLEXECUTEINFOA sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = "cmd.exe";

    std::string args = "/c " + command;
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        setError("Failed to run elevated command");
        return false;
    }

    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, timeoutMs);
        CloseHandle(sei.hProcess);
    }

    return true;
#else
    return false;
#endif
}

bool ReCoNetInstaller::runScriptWithProgress(const std::string& pythonPath, const std::string& scriptPath,
                                              const std::string& statusPrefix, int timeoutMs) {
#ifdef _WIN32
    std::string cmd = "\"" + pythonPath + "\" \"" + scriptPath + "\"";
    std::cerr << "[ReCoNetInstaller] Running script with progress: " << cmd << std::endl;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        setError("Failed to create pipe for script");
        return false;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = cmd;
    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        setError("Failed to create process for script");
        return false;
    }

    CloseHandle(hWritePipe);  // Close write end so we can detect when process finishes

    // Read output line by line and update progress
    std::string lineBuffer;
    char buffer[256];
    DWORD bytesRead;
    DWORD bytesAvailable;
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        // Check for timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed > timeoutMs) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
            setError("Script timed out");
            return false;
        }

        // Check for cancellation
        if (shouldCancel.load()) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
            return false;
        }

        // Check if process has finished
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 0);
        bool processFinished = (waitResult == WAIT_OBJECT_0);

        // Check for available data
        if (PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
            DWORD toRead = (bytesAvailable < sizeof(buffer) - 1) ? bytesAvailable : sizeof(buffer) - 1;
            if (ReadFile(hReadPipe, buffer, toRead, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';

                // Add to line buffer and process complete lines
                lineBuffer += buffer;
                size_t newlinePos;
                while ((newlinePos = lineBuffer.find('\n')) != std::string::npos) {
                    std::string line = lineBuffer.substr(0, newlinePos);
                    lineBuffer = lineBuffer.substr(newlinePos + 1);

                    // Remove carriage return if present
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    // Check if line starts with our status prefix
                    if (line.find(statusPrefix) == 0) {
                        std::lock_guard<std::mutex> lock(progressMutex);
                        progress.status = line;
                        if (progressCallback) {
                            progressCallback(progress);
                        }
                    }

                    // Log all output to stderr for debugging
                    std::cerr << "[Script] " << line << std::endl;
                }
            }
        } else if (processFinished) {
            // Process finished and no more data to read
            break;
        } else {
            // No data available, sleep briefly
            Sleep(50);
        }
    }

    // Get exit code
    DWORD dwExitCode;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    if (dwExitCode != 0) {
        std::cerr << "[ReCoNetInstaller] Script exited with code " << dwExitCode << std::endl;
        return false;
    }

    return true;
#else
    setError("Script execution not implemented on this platform");
    return false;
#endif
}

// ============================================================================
// Utility Methods
// ============================================================================

bool ReCoNetInstaller::createDirectories(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

bool ReCoNetInstaller::fileExists(const std::string& path) {
    return fs::exists(path);
}

int64_t ReCoNetInstaller::getFileSize(const std::string& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? -1 : static_cast<int64_t>(size);
}

int64_t ReCoNetInstaller::getFreeDiskSpace(const std::string& path) {
    std::error_code ec;

    fs::path checkPath(path);
    while (!fs::exists(checkPath) && checkPath.has_parent_path()) {
        checkPath = checkPath.parent_path();
    }

    if (!fs::exists(checkPath)) {
        // Default to C: drive
        checkPath = "C:\\";
    }

    auto spaceInfo = fs::space(checkPath, ec);
    return ec ? -1 : static_cast<int64_t>(spaceInfo.available);
}

void ReCoNetInstaller::updateProgress(const ReCoNetInstallProgress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
    if (progressCallback) {
        progressCallback(progress);
    }
}

void ReCoNetInstaller::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
    std::cerr << "[ReCoNetInstaller] Error: " << error << std::endl;

    std::lock_guard<std::mutex> plock(progressMutex);
    progress.errorMessage = error;
}

std::string ReCoNetInstaller::getPyTorchIndexUrl(const std::string& cudaVersion) {
    if (cudaVersion == "12.8") {
        return PYTORCH_INDEX_CU128;
    } else if (cudaVersion == "12.6") {
        return PYTORCH_INDEX_CU126;
    } else if (cudaVersion == "11.8") {
        return PYTORCH_INDEX_CU118;
    }
    // Default to CUDA 12.8
    return PYTORCH_INDEX_CU128;
}
