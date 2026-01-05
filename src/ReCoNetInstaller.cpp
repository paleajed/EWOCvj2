/**
 * ReCoNetInstaller.cpp
 *
 * Implementation of Python and ReCoNet dependencies installation
 *
 * License: GPL3
 */

#include "ReCoNetInstaller.h"

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
        setError("Installation already in progress");
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
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
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
    // First check EWOCVJ2_PYTHON environment variable
    std::string envPath = getEnvironmentVariable();
    if (!envPath.empty() && fs::exists(envPath)) {
        pythonPath = envPath;
        return true;
    }

    // Check common Python 3.12 locations
    std::vector<std::string> pythonPaths = {
        "C:\\Python312\\python.exe",
        "C:\\Python313\\python.exe",
        "C:\\Python311\\python.exe"
    };

#ifdef _WIN32
    // Add user-specific paths
    const char* username = getenv("USERNAME");
    if (username) {
        pythonPaths.push_back(std::string("C:\\Users\\") + username +
            "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe");
        pythonPaths.push_back(std::string("C:\\Users\\") + username +
            "\\AppData\\Local\\Programs\\Python\\Python313\\python.exe");
    }
#endif

    for (const auto& path : pythonPaths) {
        if (fs::exists(path)) {
            // Verify it's actually Python 3.12+
            std::string output;
            int exitCode;
            std::string cmd = "\"" + path + "\" --version";

            // Simple check - run python --version
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
                WaitForSingleObject(pi.hProcess, 5000);
                DWORD dwExitCode;
                GetExitCodeProcess(pi.hProcess, &dwExitCode);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                if (dwExitCode == 0) {
                    pythonPath = path;
                    return true;
                }
            }
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

    return true;
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

    std::string installerPath = tempDir + "\\python-3.12.7-amd64.exe";

    // Download Python installer
    prog.state = ReCoNetInstallProgress::State::DOWNLOADING;
    prog.status = "Downloading Python 3.12.7...";
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
    prog.status = "Installing Python 3.12.7 (this may take a few minutes)...";
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
    prog.status = "Python 3.12.7 installed successfully";
    prog.stepsCompleted = 4;
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void ReCoNetInstaller::installPyTorchThread(ReCoNetInstallConfig config) {
    ReCoNetInstallProgress prog;
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
    auto startTime = std::chrono::steady_clock::now();

    // Step 1: Install Python
    prog.state = ReCoNetInstallProgress::State::CHECKING;
    prog.status = "Checking for Python 3.12...";
    prog.stepsTotal = 7;
    prog.stepsCompleted = 0;
    updateProgress(prog);

    std::string pythonPath;
    bool needsPython = !isPythonInstalled(pythonPath);

    if (needsPython) {
        // Download Python
        std::string tempDir = config.tempDir;
        if (tempDir.empty()) {
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            tempDir = std::string(tempPath) + "EWOCvj2_install";
        }
        createDirectories(tempDir);

        std::string installerPath = tempDir + "\\python-3.12.7-amd64.exe";

        prog.state = ReCoNetInstallProgress::State::DOWNLOADING;
        prog.status = "Downloading Python 3.12.7...";
        prog.stepsCompleted = 1;
        prog.percentComplete = 10.0f;
        updateProgress(prog);

        if (!downloadFile(PYTHON_312_URL, installerPath, PYTHON_312_SIZE)) {
            prog.state = ReCoNetInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to download Python: " + getLastError();
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

        // Install Python
        prog.state = ReCoNetInstallProgress::State::INSTALLING;
        prog.status = "Installing Python 3.12.7...";
        prog.stepsCompleted = 2;
        prog.percentComplete = 20.0f;
        updateProgress(prog);

        if (!runPythonInstaller(installerPath, config.pythonInstallDir)) {
            prog.state = ReCoNetInstallProgress::State::FAILED;
            prog.errorMessage = "Failed to install Python: " + getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
            updateProgress(prog);
            installing.store(false);
            return;
        }

        pythonPath = config.pythonInstallDir + "\\python.exe";

        // Cleanup
        try { fs::remove(installerPath); } catch (...) {}
    }

    prog.stepsCompleted = 3;
    prog.percentComplete = 30.0f;

    // Set environment variable
    prog.status = "Setting EWOCVJ2_PYTHON environment variable...";
    updateProgress(prog);
    setEnvironmentVariable(pythonPath, config.setSystemEnvVar);

    if (shouldCancel.load()) {
        prog.state = ReCoNetInstallProgress::State::CANCELLED;
        prog.status = "Installation cancelled";
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Step 2: Install PyTorch with CUDA
    bool needsPyTorch = !isPyTorchInstalled(pythonPath);

    if (needsPyTorch) {
        prog.state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
        prog.status = "Installing PyTorch with CUDA " + config.cudaVersion + " (this may take 10-20 minutes)...";
        prog.stepsCompleted = 4;
        prog.percentComplete = 40.0f;
        updateProgress(prog);

        std::string indexUrl = getPyTorchIndexUrl(config.cudaVersion);
        std::vector<std::string> pytorchPkgs = {"torch", "torchvision", "torchaudio"};

        if (!runPipInstallMultiple(pythonPath, pytorchPkgs, indexUrl)) {
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

    // Step 3: Install additional packages
    prog.state = ReCoNetInstallProgress::State::INSTALLING_PACKAGES;
    prog.status = "Installing additional packages...";
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
        prog.status = "Installing " + pkg + "...";
        updateProgress(prog);

        if (!runPipInstall(pythonPath, pkg)) {
            // Non-fatal for additional packages, continue
            std::cerr << "[ReCoNetInstaller] Warning: Failed to install " << pkg << std::endl;
        }
    }

    // Verification
    prog.state = ReCoNetInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    prog.stepsCompleted = 7;
    prog.percentComplete = 95.0f;
    updateProgress(prog);

    bool pytorchOk = isPyTorchInstalled(pythonPath);
    bool packagesOk = arePackagesInstalled(pythonPath);

    auto endTime = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(endTime - startTime).count();

    if (pytorchOk && packagesOk) {
        prog.state = ReCoNetInstallProgress::State::COMPLETE;
        prog.status = "ReCoNet training environment installed successfully";
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

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = cmd;
    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to launch Python installer (error " + std::to_string(GetLastError()) + ")");
        return false;
    }

    // Wait for installer to complete (up to 10 minutes)
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 600000);

    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        setError("Python installer timed out");
        return false;
    }

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

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
    std::string cmd = "\"" + pythonPath + "\" -m pip install --upgrade " + package;
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

    std::string cmd = "\"" + pythonPath + "\" -m pip install --upgrade " + pkgList;
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
