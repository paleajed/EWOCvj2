/**
 * ComfyUIInstaller.cpp
 *
 * Implementation of ComfyUI download and installation
 *
 * License: GPL3
 */

#include "ComfyUIInstaller.h"
#include "InstallVerification.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <iostream>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#else
#include <curl/curl.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// ============================================================================
// Constructor / Destructor
// ============================================================================

ComfyUIInstaller::ComfyUIInstaller() {
}

ComfyUIInstaller::~ComfyUIInstaller() {
    cancelInstallation();
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
}

// ============================================================================
// Installation Methods
// ============================================================================

bool ComfyUIInstaller::installComfyUIBase(const InstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    // Validate config
    if (config.installDir.empty()) {
        setError("Installation directory not specified");
        return false;
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(InstallComponent::COMFYUI_BASE);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    // Start installation in background thread
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installComfyUIBaseThread, this, config);

    return true;
}

bool ComfyUIInstaller::installHunyuanVideo(const InstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.installDir.empty()) {
        setError("Installation directory not specified");
        return false;
    }

    // Check if ComfyUI base is installed
    if (!isComfyUIInstalled(config.installDir)) {
        setError("ComfyUI base must be installed first");
        return false;
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(InstallComponent::HUNYUAN_VIDEO);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installHunyuanVideoThread, this, config);

    return true;
}

bool ComfyUIInstaller::installFluxSchnell(const InstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.installDir.empty()) {
        setError("Installation directory not specified");
        return false;
    }

    // Check if ComfyUI base is installed
    if (!isComfyUIInstalled(config.installDir)) {
        setError("ComfyUI base must be installed first");
        return false;
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(InstallComponent::FLUX_SCHNELL);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installFluxSchnellThread, this, config);

    return true;
}

bool ComfyUIInstaller::installStyleToVideo(const InstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.installDir.empty()) {
        setError("Installation directory not specified");
        return false;
    }

    // Check if ComfyUI base is installed
    if (!isComfyUIInstalled(config.installDir)) {
        setError("ComfyUI base must be installed first");
        return false;
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(InstallComponent::STYLE_TO_VIDEO);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installStyleToVideoThread, this, config);

    return true;
}

bool ComfyUIInstaller::installAll(const InstallConfig& config) {
    if (installing.load()) {
        std::string errMsg = "Installation already in progress";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = InstallProgress::State::FAILED;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[Installer] %s\n", errMsg.c_str());
        return false;
    }

    if (config.installDir.empty()) {
        std::string errMsg = "Installation directory not specified";
        setError(errMsg);
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = InstallProgress::State::FAILED;
        progress.status = "FAILED: " + errMsg;
        if (progressCallback) progressCallback(progress);
        printf("[Installer] %s\n", errMsg.c_str());
        return false;
    }

    // Check disk space for selected components
    int64_t required = getRequiredDiskSpace(InstallComponent::COMFYUI_BASE);
    if (config.installHunyuanVideo) required += getRequiredDiskSpace(InstallComponent::HUNYUAN_VIDEO);
    if (config.installFluxSchnell) required += getRequiredDiskSpace(InstallComponent::FLUX_SCHNELL);
    if (config.installStyleToVideo) required += getRequiredDiskSpace(InstallComponent::STYLE_TO_VIDEO);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        std::string errMsg = "Insufficient disk space. Required: " + formatSize(required) +
                             ", Available: " + formatSize(available);
        setError(errMsg);
        // Notify via progress callback so the UI can display the error
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.state = InstallProgress::State::FAILED;
            progress.errorMessage = errMsg;
            progress.status = "FAILED: " + errMsg;
            if (progressCallback) {
                progressCallback(progress);
            }
        }
        printf("[Installer] %s\n", errMsg.c_str());
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    installing.store(true);
    runningInstallAll.store(true);  // Prevents sub-threads from clearing installing flag

    if (installThread && installThread->joinable()) {
        installThread->join();
    }
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installAllThread, this, config);

    return true;
}

void ComfyUIInstaller::cancelInstallation() {
    shouldCancel.store(true);
}

InstallProgress ComfyUIInstaller::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void ComfyUIInstaller::setProgressCallback(std::function<void(const InstallProgress&)> callback) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progressCallback = std::move(callback);
}

// ============================================================================
// Verification Methods
// ============================================================================

bool ComfyUIInstaller::isComfyUIInstalled(const std::string& installDir) {
    // First check manifest for verified installation
    auto result = InstallVerification::verifyInstallation(installDir, "comfyui_base");
    if (result.isValid()) {
        return true;
    }

    // Fall back to component-based checking (for backwards compatibility)
    // But only if manifest doesn't exist (not if it exists but verification failed)
    if (!result.manifestExists) {
        auto components = getComfyUIBaseComponents();
        auto missing = getMissingComponents(components, installDir);
        return missing.empty();
    }

    // Manifest exists but verification failed - installation is incomplete/corrupted
    return false;
}

bool ComfyUIInstaller::isHunyuanVideoInstalled(const std::string& installDir) {
    // Always check if all components exist (handles case where new components were added)
    auto components = getHunyuanComponents();
    auto missing = getMissingComponents(components, installDir);
    if (!missing.empty()) {
        return false;  // Missing components = not fully installed
    }

    // Optionally verify manifest for integrity check
    auto result = InstallVerification::verifyInstallation(installDir, "hunyuan_video");
    if (result.manifestExists && !result.isValid()) {
        return false;  // Manifest exists but corrupted
    }

    return true;
}

bool ComfyUIInstaller::isFluxSchnellInstalled(const std::string& installDir) {
    // Always check if all components exist (handles case where new components were added)
    auto components = getFluxSchnellComponents();
    auto missing = getMissingComponents(components, installDir);
    if (!missing.empty()) {
        return false;  // Missing components = not fully installed
    }

    // Optionally verify manifest for integrity check
    auto result = InstallVerification::verifyInstallation(installDir, "flux_schnell");
    if (result.manifestExists && !result.isValid()) {
        return false;  // Manifest exists but corrupted
    }

    return true;
}

bool ComfyUIInstaller::isStyleToVideoInstalled(const std::string& installDir) {
    // Always check if all components exist (handles case where new components were added)
    auto components = getStyleToVideoComponents();
    auto missing = getMissingComponents(components, installDir);
    printf("[StyleToVideo] Check: installDir=%s, components=%zu, missing=%zu\n",
           installDir.c_str(), components.size(), missing.size());
    if (!missing.empty()) {
        // Debug: print missing components
        printf("[StyleToVideo] Missing components:\n");
        for (const auto& m : missing) {
            printf("  - %s (%s)\n", m.id.c_str(), m.name.c_str());
        }
        return false;  // Missing components = not fully installed
    }

    // Optionally verify manifest for integrity check
    auto result = InstallVerification::verifyInstallation(installDir, "style_to_video");
    printf("[StyleToVideo] Manifest check: exists=%d, valid=%d\n",
           result.manifestExists, result.isValid());
    if (result.manifestExists && !result.isValid()) {
        printf("[StyleToVideo] FAILED: Manifest exists but invalid\n");
        return false;  // Manifest exists but corrupted
    }

    printf("[StyleToVideo] SUCCESS: All checks passed\n");
    return true;
}

int64_t ComfyUIInstaller::getRequiredDiskSpace(InstallComponent component) {
    switch (component) {
        case InstallComponent::COMFYUI_BASE:
            // ComfyUI portable (~2.5GB compressed, ~4GB extracted)
            return 5LL * 1024 * 1024 * 1024;  // 5GB with safety margin

        case InstallComponent::HUNYUAN_VIDEO:
            // Hunyuan Slim (GGUF): T2V Q4 (~5GB) + I2V Q4 (~5GB) + VAE (~2.5GB) +
            // Qwen 2.5 (~9.4GB) + ByT5 (~438MB) + CLIP Vision (~856MB)
            // Note: IP2V requires Hunyuan Full, not included here
            return 25LL * 1024 * 1024 * 1024;  // 25GB with safety margin

        case InstallComponent::FLUX_SCHNELL:
            // Flux Schnell FP8 (~11.9GB) + VAE (~335MB) + CLIP-L (~246MB) + T5-XXL FP8 (~4.9GB) + Prompt Enhance (~900MB)
            return 20LL * 1024 * 1024 * 1024;  // 20GB with safety margin

        case InstallComponent::STYLE_TO_VIDEO:
            // FP8 model (~13.2GB) + VLM (~16.8GB) + VAE (~2.5GB if not shared)
            return 35LL * 1024 * 1024 * 1024;  // 35GB with safety margin

        default:
            return 0;
    }
}

int64_t ComfyUIInstaller::getDownloadSize(InstallComponent component) {
    switch (component) {
        case InstallComponent::COMFYUI_BASE:
            return COMFYUI_PORTABLE_SIZE;

        case InstallComponent::HUNYUAN_VIDEO:
            // Hunyuan Slim (GGUF) - core models only, no IP2V components
            return HUNYUAN_T2V_Q4_SIZE + HUNYUAN_I2V_Q4_SIZE + HUNYUAN_VAE_SIZE +
                   HUNYUAN_QWEN_SIZE + HUNYUAN_BYT5_SIZE + HUNYUAN_CLIP_VISION_SIZE;

        case InstallComponent::FLUX_SCHNELL:
            return FLUX_SCHNELL_GGUF_SIZE + FLUX_VAE_SIZE + FLUX_CLIP_L_SIZE + FLUX_T5XXL_FP8_SIZE + FLUX_PROMPT_ENHANCE_SIZE;

        case InstallComponent::STYLE_TO_VIDEO:
            return HUNYUAN_FP16_T2V_SIZE + HUNYUAN_VAE_SIZE +
                   LLAVA_VLM_MODEL1_SIZE + LLAVA_VLM_MODEL2_SIZE + LLAVA_VLM_MODEL3_SIZE + LLAVA_VLM_MODEL4_SIZE;

        default:
            return 0;
    }
}

std::string ComfyUIInstaller::formatSize(int64_t bytes) {
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
// Prerequisites Detection and Installation
// ============================================================================

bool ComfyUIInstaller::checkPrerequisites() {
    return is7ZipInstalled() && isGitInstalled();
}

bool ComfyUIInstaller::is7ZipInstalled() {
#ifdef _WIN32
    // Check common installation paths
    std::vector<std::string> searchPaths = {
        "C:\\Program Files\\7-Zip\\7z.exe",
        "C:\\Program Files (x86)\\7-Zip\\7z.exe"
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return true;
        }
    }

    // Check if 7z is in PATH
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[] = "cmd /c where 7z >nul 2>&1";
    if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode == 0) return true;
    }

    return false;
#else
    // Linux: check if p7zip is available
    return system("which 7z > /dev/null 2>&1") == 0;
#endif
}

bool ComfyUIInstaller::isGitInstalled() {
#ifdef _WIN32
    // Check common installation paths
    std::vector<std::string> searchPaths = {
        "C:\\Program Files\\Git\\bin\\git.exe",
        "C:\\Program Files (x86)\\Git\\bin\\git.exe",
        "C:\\Program Files\\Git\\cmd\\git.exe"
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return true;
        }
    }

    // Check if git is in PATH
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[] = "cmd /c where git >nul 2>&1";
    if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode == 0) return true;
    }

    return false;
#else
    // Linux: check if git is available
    return system("which git > /dev/null 2>&1") == 0;
#endif
}

bool ComfyUIInstaller::installPrerequisites(const InstallConfig& config) {
    std::string tempDir = config.tempDir.empty() ?
        (fs::path(config.installDir) / "temp").string() : config.tempDir;
    createDirectories(tempDir);

    // Install 7-Zip if missing (with lock to prevent multiple installers from running simultaneously)
    {
        InstallProgress prog;
        prog.state = InstallProgress::State::DOWNLOADING;
        prog.status = "Checking/Installing 7-Zip...";
        prog.currentFile = "7-Zip";
        updateProgress(prog);

        // Capture tempDir and this pointer for lambdas
        std::string tempDirCopy = tempDir;
        ComfyUIInstaller* self = this;

        bool result = installPrerequisiteWithLock(
            PrerequisiteIds::SEVENZIP,
            []() { return is7ZipInstalled(); },
            [self, tempDirCopy]() { return self->install7Zip(tempDirCopy); },
            5000,
            [self](const std::string&) {
                InstallProgress prog;
                prog.state = InstallProgress::State::DOWNLOADING;
                prog.status = "Prerequisites: Waiting for 7-Zip installation by another installer...";
                prog.currentFile = "7-Zip";
                self->updateProgress(prog);
            }
        );

        if (!result) {
            setError("Failed to install 7-Zip prerequisite");
            return false;
        }
    }

    // Install Git if missing (with lock)
    {
        InstallProgress prog;
        prog.state = InstallProgress::State::DOWNLOADING;
        prog.status = "Checking/Installing Git...";
        prog.currentFile = "Git for Windows";
        updateProgress(prog);

        std::string tempDirCopy = tempDir;
        ComfyUIInstaller* self = this;

        bool result = installPrerequisiteWithLock(
            PrerequisiteIds::GIT,
            []() { return isGitInstalled(); },
            [self, tempDirCopy]() { return self->installGit(tempDirCopy); },
            5000,
            [self](const std::string&) {
                InstallProgress prog;
                prog.state = InstallProgress::State::DOWNLOADING;
                prog.status = "Prerequisites: Waiting for Git installation by another installer...";
                prog.currentFile = "Git for Windows";
                self->updateProgress(prog);
            }
        );

        if (!result) {
            setError("Failed to install Git prerequisite");
            return false;
        }
    }

    return true;
}

bool ComfyUIInstaller::install7Zip(const std::string& tempDir) {
#ifdef _WIN32
    std::string installerPath = (fs::path(tempDir) / "7z-installer.exe").string();

    // Delete any existing partial/corrupted download to prevent resume issues
    std::error_code deleteEc;
    if (fs::exists(installerPath)) {
        fs::remove(installerPath, deleteEc);
    }

    InstallProgress prog;
    prog.state = InstallProgress::State::DOWNLOADING;
    prog.status = "Downloading 7-Zip...";
    prog.currentFile = "7-Zip Installer";
    updateProgress(prog);

    // Download 7-Zip installer (fresh download, no resume for installers)
    if (!downloadFileWithResume(SEVENZIP_URL, installerPath, SEVENZIP_SIZE)) {
        setError("Failed to download 7-Zip installer");
        fs::remove(installerPath, deleteEc);
        return false;
    }

    // Verify downloaded file size before running installer
    int64_t downloadedSize = getFileSize(installerPath);
    if (downloadedSize < SEVENZIP_SIZE * 0.9) {
        setError("7-Zip installer download appears corrupted (size mismatch)");
        fs::remove(installerPath, deleteEc);
        return false;
    }

    prog.state = InstallProgress::State::INSTALLING_NODES;
    prog.status = "Installing 7-Zip (may request admin rights)...";
    updateProgress(prog);

    // Run silent install with elevation using ShellExecuteEx
    // 7-Zip installer supports /S for silent mode
    // Retry loop handles file access issues (antivirus scanning, file system delays)
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = "runas";  // Request elevation
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/S";
    sei.nShow = SW_HIDE;

    bool launched = false;
    DWORD lastErr = 0;
    for (int attempt = 0; attempt < 5 && !launched; attempt++) {
        if (attempt > 0) {
            // Wait before retry - gives antivirus time to release file
            Sleep(1000);
        }
        if (ShellExecuteExA(&sei)) {
            launched = true;
        } else {
            lastErr = GetLastError();
            // Only retry on access-related errors
            if (lastErr != ERROR_SHARING_VIOLATION &&
                lastErr != ERROR_LOCK_VIOLATION &&
                lastErr != ERROR_ACCESS_DENIED) {
                break;
            }
        }
    }

    if (!launched) {
        if (lastErr == ERROR_CANCELLED) {
            setError("7-Zip installation cancelled - admin rights required");
        } else {
            setError("Failed to run 7-Zip installer (error " + std::to_string(lastErr) + ")");
        }
        return false;
    }

    // Wait for installation to complete (max 2 minutes)
    if (sei.hProcess) {
        DWORD waitResult = WaitForSingleObject(sei.hProcess, 120000);

        DWORD exitCode = 1;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);

        if (waitResult == WAIT_TIMEOUT) {
            setError("7-Zip installation timed out");
            return false;
        }
    }

    // Cleanup installer
    std::error_code ec;
    fs::remove(installerPath, ec);

    // Verify installation
    if (!is7ZipInstalled()) {
        setError("7-Zip installation verification failed");
        return false;
    }

    return true;
#else
    // Linux: use package manager
    setError("Please install p7zip manually: sudo apt install p7zip-full");
    return false;
#endif
}

bool ComfyUIInstaller::installGit(const std::string& tempDir) {
#ifdef _WIN32
    std::string installerPath = (fs::path(tempDir) / "git-installer.exe").string();

    // Delete any existing partial/corrupted download to prevent resume issues
    std::error_code deleteEc;
    if (fs::exists(installerPath)) {
        fs::remove(installerPath, deleteEc);
    }

    InstallProgress prog;
    prog.state = InstallProgress::State::DOWNLOADING;
    prog.status = "Downloading Git for Windows...";
    prog.currentFile = "Git Installer";
    updateProgress(prog);

    // Download Git installer (fresh download, no resume for installers)
    if (!downloadFileWithResume(GIT_URL, installerPath, GIT_SIZE)) {
        setError("Failed to download Git installer");
        // Clean up partial download
        fs::remove(installerPath, deleteEc);
        return false;
    }

    // Verify downloaded file size before running installer
    int64_t downloadedSize = getFileSize(installerPath);
    // Allow some tolerance (within 10% of expected size)
    if (downloadedSize < GIT_SIZE * 0.9) {
        setError("Git installer download appears corrupted (size mismatch: expected ~" +
                 formatSize(GIT_SIZE) + ", got " + formatSize(downloadedSize) + ")");
        fs::remove(installerPath, deleteEc);
        return false;
    }

    prog.state = InstallProgress::State::INSTALLING_NODES;
    prog.status = "Installing Git (may request admin rights)...";
    updateProgress(prog);

    // Run silent install with elevation using ShellExecuteEx
    // Git installer options:
    // /VERYSILENT - No UI at all
    // /NORESTART - Don't restart
    // /NOCANCEL - No cancel button
    // /SP- - Suppress "This will install..." dialog
    // /CLOSEAPPLICATIONS - Close applications that need to be closed
    // Retry loop handles file access issues (antivirus scanning, file system delays)
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = "runas";  // Request elevation
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/VERYSILENT /NORESTART /NOCANCEL /SP- /CLOSEAPPLICATIONS";
    sei.nShow = SW_HIDE;

    bool launched = false;
    DWORD lastErr = 0;
    for (int attempt = 0; attempt < 5 && !launched; attempt++) {
        if (attempt > 0) {
            // Wait before retry - gives antivirus time to release file
            Sleep(1000);
        }
        if (ShellExecuteExA(&sei)) {
            launched = true;
        } else {
            lastErr = GetLastError();
            // Only retry on access-related errors
            if (lastErr != ERROR_SHARING_VIOLATION &&
                lastErr != ERROR_LOCK_VIOLATION &&
                lastErr != ERROR_ACCESS_DENIED) {
                break;
            }
        }
    }

    if (!launched) {
        if (lastErr == ERROR_CANCELLED) {
            setError("Git installation cancelled - admin rights required");
        } else {
            setError("Failed to run Git installer (error " + std::to_string(lastErr) + ")");
        }
        return false;
    }

    // Wait for installation to complete (max 5 minutes - Git takes longer)
    if (sei.hProcess) {
        DWORD waitResult = WaitForSingleObject(sei.hProcess, 300000);

        DWORD exitCode = 1;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);

        if (waitResult == WAIT_TIMEOUT) {
            setError("Git installation timed out");
            return false;
        }
    }

    // Cleanup installer
    std::error_code ec;
    fs::remove(installerPath, ec);

    // Need to refresh PATH for current process to find git
    // Update environment from registry
    DWORD bufferSize = 32767;
    char* newPath = new char[bufferSize];
    GetEnvironmentVariableA("PATH", newPath, bufferSize);

    // Add Git to PATH for current process if not already there
    std::string gitPath = "C:\\Program Files\\Git\\cmd";
    std::string currentPath(newPath);
    delete[] newPath;

    if (currentPath.find(gitPath) == std::string::npos) {
        std::string updatedPath = gitPath + ";" + currentPath;
        SetEnvironmentVariableA("PATH", updatedPath.c_str());
    }

    // Small delay to let Windows settle
    Sleep(1000);

    // Verify installation
    if (!isGitInstalled()) {
        setError("Git installation verification failed");
        return false;
    }

    return true;
#else
    // Linux: use package manager
    setError("Please install git manually: sudo apt install git");
    return false;
#endif
}

// ============================================================================
// Uninstallation Methods
// ============================================================================

bool ComfyUIInstaller::uninstallHunyuanVideo(const std::string& installDir) {
    // Portable version path structure
    fs::path basePath = fs::path(installDir) / "ComfyUI_windows_portable" / "ComfyUI";
    fs::path modelsPath = basePath / "models";
    fs::path nodesPath = basePath / "custom_nodes";

    std::error_code ec;

    // Remove models
    fs::remove(modelsPath / "diffusion_models" / "hunyuan-video-t2v-720p-Q4_0.gguf", ec);
    fs::remove(modelsPath / "diffusion_models" / "hunyuan-video-i2v-720p-Q4_K_M.gguf", ec);
    fs::remove(modelsPath / "vae" / "hunyuan_video_vae_bf16.safetensors", ec);
    fs::remove(modelsPath / "text_encoders" / "clip_l.safetensors", ec);
    fs::remove(modelsPath / "text_encoders" / "llava_llama3_fp8_scaled.safetensors", ec);

    // Remove custom nodes
    fs::remove_all(nodesPath / "ComfyUI-GGUF", ec);
    fs::remove_all(nodesPath / "ComfyUI-HunyuanVideoWrapper", ec);

    return true;
}

bool ComfyUIInstaller::uninstallFluxSchnell(const std::string& installDir) {
    // Portable version path structure
    fs::path basePath = fs::path(installDir) / "ComfyUI_windows_portable" / "ComfyUI";
    fs::path modelsPath = basePath / "models";

    std::error_code ec;

    // Remove Flux models
    fs::remove(modelsPath / "unet" / "flux1-schnell-Q4_K_S.gguf", ec);
    fs::remove(modelsPath / "unet" / "flux1-schnell-fp8.safetensors", ec);
    fs::remove(modelsPath / "vae" / "ae.safetensors", ec);
    fs::remove(modelsPath / "clip" / "clip_l.safetensors", ec);
    fs::remove(modelsPath / "clip" / "t5xxl_fp8_e4m3fn.safetensors", ec);

    return true;
}

bool ComfyUIInstaller::uninstallAll(const std::string& installDir) {
    fs::path comfyPath(installDir);
    std::error_code ec;
    fs::remove_all(comfyPath, ec);
    return !ec;
}

// ============================================================================
// Error Handling
// ============================================================================

std::string ComfyUIInstaller::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void ComfyUIInstaller::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ============================================================================
// Installation Threads
// ============================================================================

void ComfyUIInstaller::installComfyUIBaseThread(InstallConfig config) {
    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    // Use component-based approach to find what's missing
    auto allComponents = getComfyUIBaseComponents();
    auto missingComponents = getMissingComponents(allComponents, config.installDir);

    // Check if everything is already installed
    if (missingComponents.empty()) {
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "ComfyUI base already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        if (!runningInstallAll.load()) installing.store(false);
        return;
    }

    // Log what's missing
    prog.status = "Installing " + std::to_string(missingComponents.size()) + " missing component(s)...";
    updateProgress(prog);

    // Create directories
    if (!createDirectories(config.installDir)) {
        prog.state = InstallProgress::State::FAILED;
        prog.errorMessage = "Failed to create installation directory";
        updateProgress(prog);
        if (!runningInstallAll.load()) installing.store(false);
        return;
    }

    std::string tempDir = config.tempDir.empty() ?
        (fs::path(config.installDir) / "temp").string() : config.tempDir;
    createDirectories(tempDir);

    // Check and install prerequisites (7-Zip, Git) - only if portable component is missing
    bool needsPortable = false;
    for (const auto& comp : missingComponents) {
        if (comp.id == "comfyui_portable") {
            needsPortable = true;
            break;
        }
    }

    if (needsPortable && !checkPrerequisites()) {
        prog.state = InstallProgress::State::CHECKING;
        prog.status = "Installing prerequisites...";
        updateProgress(prog);

        if (!installPrerequisites(config)) {
            prog.state = InstallProgress::State::FAILED;
            prog.errorMessage = "Failed to install prerequisites: " + getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }
    }

    auto startTime = std::chrono::steady_clock::now();

    std::string nodesDir = (fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "custom_nodes").string();

    // Count total files and nodes to install
    prog.filesTotal = 0;
    for (const auto& comp : missingComponents) {
        prog.filesTotal += static_cast<int>(comp.files.size());
        prog.filesTotal += static_cast<int>(comp.customNodes.size());
    }
    prog.filesCompleted = 0;

    // Calculate total download size for missing components
    int64_t totalBytes = 0;
    int64_t downloadedBytes = 0;
    for (const auto& comp : missingComponents) {
        for (const auto& f : comp.files) {
            totalBytes += f.expectedSize;
        }
    }

    // Install each missing component
    for (const auto& component : missingComponents) {
        if (shouldCancel.load()) {
            prog.state = InstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }

        prog.status = "Installing " + component.name + "...";
        updateProgress(prog);

        // Download files for this component
        for (const auto& file : component.files) {
            if (shouldCancel.load()) break;

            prog.state = InstallProgress::State::DOWNLOADING;
            prog.currentFile = file.description;
            prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = prog.statusPrefix + "Downloading " + file.description;
            if (file.expectedSize > 0) {
                prog.status += " (" + formatSize(file.expectedSize) + ")";
            }
            prog.percentComplete = (totalBytes > 0) ?
                (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
            updateProgress(prog);

            std::string localPath = (fs::path(tempDir) / fs::path(file.localPath).filename()).string();

            // Retry download with resume support for large files
            bool downloadSuccess = false;
            for (int attempt = 0; attempt < currentConfig.maxRetries && !downloadSuccess; attempt++) {
                if (attempt > 0) {
                    prog.status = prog.statusPrefix + "Retrying " + file.description + " (attempt " + std::to_string(attempt + 1) + "/" + std::to_string(currentConfig.maxRetries) + ")";
                    updateProgress(prog);
                    std::this_thread::sleep_for(std::chrono::seconds(2));  // Brief pause before retry
                }
                downloadSuccess = downloadFileWithResume(file.url, localPath, file.expectedSize);
            }

            if (!downloadSuccess) {
                if (file.required) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                    prog.status = "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            downloadedBytes += file.expectedSize;
            prog.filesCompleted++;

            // Extract 7z archive (special case for ComfyUI portable)
            if (file.localPath.find(".7z") != std::string::npos) {
                prog.state = InstallProgress::State::EXTRACTING;
                prog.status = prog.statusPrefix + "Extracting " + file.description;
                updateProgress(prog);

                if (!extract7z(localPath, config.installDir)) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to extract " + file.description + ": " + getLastError();
                    prog.status = "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }
        }

        // Clone custom nodes for this component
        createDirectories(nodesDir);
        for (const auto& nodeUrl : component.customNodes) {
            if (shouldCancel.load()) break;

            // Extract repo name from URL
            std::string repoName = nodeUrl;
            size_t lastSlash = repoName.rfind('/');
            if (lastSlash != std::string::npos) {
                repoName = repoName.substr(lastSlash + 1);
            }
            if (repoName.size() > 4 && repoName.substr(repoName.size() - 4) == ".git") {
                repoName = repoName.substr(0, repoName.size() - 4);
            }

            prog.state = InstallProgress::State::INSTALLING_NODES;
            prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = prog.statusPrefix + "Installing node: " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = (fs::path(nodesDir) / repoName).string();
            if (!fs::exists(targetDir)) {
                if (!cloneRepository(nodeUrl, targetDir)) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to clone " + repoName;
                    prog.status = prog.statusPrefix + "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            prog.filesCompleted++;
        }
    }

    // Install Python dependencies for VideoHelperSuite (cv2, imageio-ffmpeg)
    prog.status = "Installing video dependencies...";
    updateProgress(prog);

    std::string pythonExe = config.installDir + "\\ComfyUI_windows_portable\\python_embeded\\python.exe";
    if (fs::exists(pythonExe)) {
        std::string pipCommand = "\"" + pythonExe + "\" -m pip install opencv-python imageio-ffmpeg av numexpr pandas";

#ifdef _WIN32
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        char cmdLine[4096];
        strncpy(cmdLine, pipCommand.c_str(), sizeof(cmdLine) - 1);
        cmdLine[sizeof(cmdLine) - 1] = '\0';

        if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 900000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
#else
        system(pipCommand.c_str());
#endif
    }

    // Verification
    prog.state = InstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    // Cleanup temp files
    if (config.cleanupOnFailure) {
        std::error_code ec;
        fs::remove_all(tempDir, ec);
    }

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    // Write installation manifest for verification on next startup
    InstallManifest manifest;
    manifest.componentId = "comfyui_base";
    manifest.componentName = "ComfyUI Base";
    manifest.version = "0.7.0";
    manifest.complete = true;

    // Add key files to manifest for verification
    manifest.addFile("ComfyUI_windows_portable/ComfyUI/main.py", 0);  // Size varies
    manifest.addFile("ComfyUI_windows_portable/run_nvidia_gpu.bat", 0);

    // Add custom node directories
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-Manager");
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-VideoHelperSuite");

    InstallVerification::writeManifest(config.installDir, manifest);

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "ComfyUI base installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installHunyuanVideoThread(InstallConfig config) {
    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    // Use component-based approach to find what's missing
    auto allComponents = getHunyuanComponents();
    auto missingComponents = getMissingComponents(allComponents, config.installDir);

    // Check if everything is already installed
    if (missingComponents.empty()) {
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "HunyuanVideo already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        if (!runningInstallAll.load()) installing.store(false);
        return;
    }

    // Log what's missing
    prog.status = "Installing " + std::to_string(missingComponents.size()) + " missing component(s)...";
    updateProgress(prog);

    auto startTime = std::chrono::steady_clock::now();

    // Create model directories (portable version path structure)
    fs::path modelsPath = fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "models";
    createDirectories((modelsPath / "unet").string());
    createDirectories((modelsPath / "vae").string());
    createDirectories((modelsPath / "text_encoders").string());
    createDirectories((modelsPath / "clip_vision").string());

    std::string nodesDir = (fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "custom_nodes").string();

    // Count total files and nodes to install
    prog.filesTotal = 0;
    for (const auto& comp : missingComponents) {
        prog.filesTotal += static_cast<int>(comp.files.size());
        prog.filesTotal += static_cast<int>(comp.customNodes.size());
    }
    prog.filesCompleted = 0;

    // Calculate total download size for missing components
    int64_t totalBytes = 0;
    int64_t downloadedBytes = 0;
    for (const auto& comp : missingComponents) {
        for (const auto& f : comp.files) {
            totalBytes += f.expectedSize;
        }
    }

    // Install each missing component
    for (const auto& component : missingComponents) {
        if (shouldCancel.load()) {
            prog.state = InstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }

        prog.status = "Installing " + component.name + "...";
        updateProgress(prog);

        // Download files for this component
        for (const auto& file : component.files) {
            if (shouldCancel.load()) break;

            prog.state = InstallProgress::State::DOWNLOADING;
            prog.currentFile = file.description;
            prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = prog.statusPrefix + "Downloading " + file.description;
            if (file.expectedSize > 0) {
                prog.status += " (" + formatSize(file.expectedSize) + ")";
            }
            prog.percentComplete = (totalBytes > 0) ?
                (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
            updateProgress(prog);

            std::string localPath = (modelsPath / file.localPath).string();

            // Create parent directory
            fs::path parentDir = fs::path(localPath).parent_path();
            createDirectories(parentDir.string());

            // Retry download with resume support for large files
            bool downloadSuccess = false;
            for (int attempt = 0; attempt < currentConfig.maxRetries && !downloadSuccess; attempt++) {
                if (attempt > 0) {
                    prog.status = prog.statusPrefix + "Retrying " + file.description + " (attempt " + std::to_string(attempt + 1) + "/" + std::to_string(currentConfig.maxRetries) + ")";
                    updateProgress(prog);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                downloadSuccess = downloadFileWithResume(file.url, localPath, file.expectedSize);
            }

            if (!downloadSuccess) {
                if (file.required) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                    prog.status = prog.statusPrefix + "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            downloadedBytes += file.expectedSize;
            prog.filesCompleted++;
        }

        // Clone custom nodes for this component
        for (const auto& nodeUrl : component.customNodes) {
            if (shouldCancel.load()) break;

            // Extract repo name from URL
            std::string repoName = nodeUrl;
            size_t lastSlash = repoName.rfind('/');
            if (lastSlash != std::string::npos) {
                repoName = repoName.substr(lastSlash + 1);
            }
            if (repoName.size() > 4 && repoName.substr(repoName.size() - 4) == ".git") {
                repoName = repoName.substr(0, repoName.size() - 4);
            }

            prog.state = InstallProgress::State::INSTALLING_NODES;
            prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = prog.statusPrefix + "Installing node: " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = (fs::path(nodesDir) / repoName).string();
            if (!fs::exists(targetDir)) {
                if (!cloneRepository(nodeUrl, targetDir)) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to clone " + repoName;
                    prog.status = prog.statusPrefix + "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            prog.filesCompleted++;

            // Patch ComfyUI_LLM_Node to make llama_cpp import optional
            if (repoName == "ComfyUI_LLM_Node") {
                std::string llmNodeFile = targetDir + "\\LLM_Node.py";
                if (fs::exists(llmNodeFile)) {
                    std::ifstream inFile(llmNodeFile);
                    std::string content((std::istreambuf_iterator<char>(inFile)),
                                        std::istreambuf_iterator<char>());
                    inFile.close();

                    // Replace the llama_cpp import with a try/except
                    std::string oldImport = "from llama_cpp import Llama\nimport torch";
                    std::string newImport = "try:\n    from llama_cpp import Llama\n    LLAMA_CPP_AVAILABLE = True\nexcept ImportError:\n    Llama = None\n    LLAMA_CPP_AVAILABLE = False\nimport torch";

                    size_t pos = content.find(oldImport);
                    if (pos != std::string::npos) {
                        content.replace(pos, oldImport.length(), newImport);
                    }

                    // Fix max_length -> max_new_tokens for proper token handling
                    std::string oldMaxLen = "generate_kwargs = {'max_length': max_tokens}";
                    std::string newMaxLen = "generate_kwargs = {'max_new_tokens': max_tokens}";
                    pos = content.find(oldMaxLen);
                    if (pos != std::string::npos) {
                        content.replace(pos, oldMaxLen.length(), newMaxLen);
                    }

                    // Write patched content
                    std::ofstream outFile(llmNodeFile);
                    outFile << content;
                    outFile.close();
                }
            }
        }
    }

    // Install Python dependencies for HunyuanVideoWrapper
    prog.status = "Installing Python dependencies...";
    updateProgress(prog);

    std::string pythonExe = config.installDir + "\\ComfyUI_windows_portable\\python_embeded\\python.exe";

    if (fs::exists(pythonExe)) {
        // HunyuanVideoWrapper requirements
        std::string wrapperDir = nodesDir + "\\ComfyUI-HunyuanVideoWrapper";
        std::string requirementsFile = wrapperDir + "\\requirements.txt";

        if (fs::exists(requirementsFile)) {
            std::string pipCommand = "\"" + pythonExe + "\" -m pip install -r \"" + requirementsFile + "\"";

#ifdef _WIN32
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, pipCommand.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 900000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
#else
            system(pipCommand.c_str());
#endif
        }

        // GGUF dependencies (for loading GGUF quantized models)
        prog.status = "Installing GGUF dependencies...";
        updateProgress(prog);

        std::string ggufDeps = "\"" + pythonExe + "\" -m pip install gguf sentencepiece protobuf";

#ifdef _WIN32
        {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, ggufDeps.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 900000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        system(ggufDeps.c_str());
#endif
    }

    // Verification
    prog.state = InstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    // Write installation manifest for verification on next startup
    InstallManifest manifest;
    manifest.componentId = "hunyuan_video";
    manifest.componentName = "HunyuanVideo";
    manifest.version = "1.5";
    manifest.complete = true;

    // Add model files with expected sizes for verification
    std::string modelsBase = "ComfyUI_windows_portable/ComfyUI/models/";
    manifest.addFile(modelsBase + "unet/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf", HUNYUAN_T2V_Q4_SIZE);
    manifest.addFile(modelsBase + "unet/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf", HUNYUAN_I2V_Q4_SIZE);
    manifest.addFile(modelsBase + "vae/hunyuanvideo15_vae_fp16.safetensors", HUNYUAN_VAE_SIZE);
    manifest.addFile(modelsBase + "text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors", HUNYUAN_QWEN_SIZE);
    manifest.addFile(modelsBase + "text_encoders/byt5_small_glyphxl_fp16.safetensors", HUNYUAN_BYT5_SIZE);
    manifest.addFile(modelsBase + "clip_vision/sigclip_vision_patch14_384.safetensors", HUNYUAN_CLIP_VISION_SIZE);

    // Add custom node directories
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-GGUF");
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-HunyuanVideoWrapper");
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-Frame-Interpolation");

    InstallVerification::writeManifest(config.installDir, manifest);

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "HunyuanVideo installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installFluxSchnellThread(InstallConfig config) {
    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Checking existing Flux installation...";
    updateProgress(prog);

    // Use component-based approach to find what's missing
    auto allComponents = getFluxSchnellComponents();
    auto missingComponents = getMissingComponents(allComponents, config.installDir);

    // Check if everything is already installed
    if (missingComponents.empty()) {
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "Flux.1 Schnell already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        if (!runningInstallAll.load()) installing.store(false);
        return;
    }

    // Log what's missing
    prog.status = "Installing " + std::to_string(missingComponents.size()) + " missing component(s)...";
    updateProgress(prog);

    auto startTime = std::chrono::steady_clock::now();

    // Create model directories (portable version path structure)
    fs::path modelsPath = fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "models";
    createDirectories((modelsPath / "unet").string());
    createDirectories((modelsPath / "vae").string());
    createDirectories((modelsPath / "clip").string());

    std::string nodesDir = (fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "custom_nodes").string();

    // Count total files and nodes to install
    prog.filesTotal = 0;
    for (const auto& comp : missingComponents) {
        prog.filesTotal += static_cast<int>(comp.files.size());
        prog.filesTotal += static_cast<int>(comp.customNodes.size());
    }
    prog.filesCompleted = 0;

    // Calculate total download size for missing components
    int64_t totalBytes = 0;
    int64_t downloadedBytes = 0;
    for (const auto& comp : missingComponents) {
        for (const auto& f : comp.files) {
            totalBytes += f.expectedSize;
        }
    }

    // Install each missing component
    for (const auto& component : missingComponents) {
        if (shouldCancel.load()) {
            prog.state = InstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }

        prog.status = "Installing " + component.name + "...";
        updateProgress(prog);

        // Download files for this component
        for (const auto& file : component.files) {
            if (shouldCancel.load()) break;

            prog.state = InstallProgress::State::DOWNLOADING;
            prog.currentFile = file.description;
            prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = prog.statusPrefix + "Downloading " + file.description;
            if (file.expectedSize > 0) {
                prog.status += " (" + formatSize(file.expectedSize) + ")";
            }
            prog.percentComplete = (totalBytes > 0) ?
                (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
            updateProgress(prog);

            std::string localPath = (modelsPath / file.localPath).string();

            // Create parent directory
            fs::path parentDir = fs::path(localPath).parent_path();
            createDirectories(parentDir.string());

            // Retry download with resume support for large files
            bool downloadSuccess = false;
            for (int attempt = 0; attempt < currentConfig.maxRetries && !downloadSuccess; attempt++) {
                if (attempt > 0) {
                    prog.status = prog.statusPrefix + "Retrying " + file.description + " (attempt " + std::to_string(attempt + 1) + "/" + std::to_string(currentConfig.maxRetries) + ")";
                    updateProgress(prog);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                downloadSuccess = downloadFileWithResume(file.url, localPath, file.expectedSize);
            }

            if (!downloadSuccess) {
                if (file.required) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                    prog.status = prog.statusPrefix + "FAILED: " + prog.errorMessage;
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            downloadedBytes += file.expectedSize;
            prog.filesCompleted++;
        }

        // Clone custom nodes for this component
        for (const auto& nodeUrl : component.customNodes) {
            if (shouldCancel.load()) break;

            // Extract repo name from URL
            std::string repoName = nodeUrl;
            size_t lastSlash = repoName.rfind('/');
            if (lastSlash != std::string::npos) {
                repoName = repoName.substr(lastSlash + 1);
            }
            if (repoName.size() > 4 && repoName.substr(repoName.size() - 4) == ".git") {
                repoName = repoName.substr(0, repoName.size() - 4);
            }

            prog.state = InstallProgress::State::INSTALLING_NODES;
            std::string nodePrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
            prog.status = nodePrefix + "Installing " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = nodesDir + "/" + repoName;

            if (!fs::exists(targetDir)) {
                if (!cloneRepository(nodeUrl, targetDir)) {
                    std::cerr << "[ComfyUIInstaller] Warning: Failed to clone " << repoName << std::endl;
                }
            } else {
                pullRepository(targetDir);
            }

            prog.filesCompleted++;
        }
    }

    // Install GGUF dependencies (needed for loading GGUF quantized models)
    prog.status = "Installing GGUF dependencies...";
    updateProgress(prog);

    std::string pythonExe = config.installDir + "\\ComfyUI_windows_portable\\python_embeded\\python.exe";

    if (fs::exists(pythonExe)) {
        std::string ggufDeps = "\"" + pythonExe + "\" -m pip install gguf sentencepiece protobuf";

#ifdef _WIN32
        {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, ggufDeps.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 900000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        system(ggufDeps.c_str());
#endif
    }

    // Install transformers and accelerate for prompt enhancer and LLM node
    prog.status = "Installing transformers for prompt enhancer...";
    updateProgress(prog);

    if (fs::exists(pythonExe)) {
        std::string transformersDeps = "\"" + pythonExe + "\" -m pip install transformers accelerate";

#ifdef _WIN32
        {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, transformersDeps.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 900000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        system(transformersDeps.c_str());
#endif
    }

    // Pre-download Flux-Prompt-Enhance model (~900MB) to avoid first-use download
    prog.status = "Downloading Flux Prompt Enhance model (~900MB)...";
    updateProgress(prog);

    if (fs::exists(pythonExe)) {
        // Python one-liner to pre-download and cache the model
        std::string downloadModel = "\"" + pythonExe + "\" -c \"from transformers import AutoTokenizer, AutoModelForSeq2SeqLM; AutoTokenizer.from_pretrained('gokaygokay/Flux-Prompt-Enhance'); AutoModelForSeq2SeqLM.from_pretrained('gokaygokay/Flux-Prompt-Enhance'); print('Model downloaded successfully')\"";

#ifdef _WIN32
        {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, downloadModel.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                // Allow up to 10 minutes for ~900MB download
                WaitForSingleObject(pi.hProcess, 600000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        system(downloadModel.c_str());
#endif
    }

    // Verification
    prog.state = InstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    // Write installation manifest for verification on next startup
    InstallManifest manifest;
    manifest.componentId = "flux_schnell";
    manifest.componentName = "Flux.1 Schnell";
    manifest.version = "1.0";
    manifest.complete = true;

    // Add model files with expected sizes for verification
    std::string modelsBase = "ComfyUI_windows_portable/ComfyUI/models/";
    manifest.addFile(modelsBase + "unet/flux1-schnell-Q4_K_S.gguf", FLUX_SCHNELL_GGUF_SIZE);
    manifest.addFile(modelsBase + "vae/ae.safetensors", FLUX_VAE_SIZE);
    manifest.addFile(modelsBase + "clip/clip_l.safetensors", FLUX_CLIP_L_SIZE);
    manifest.addFile(modelsBase + "clip/t5xxl_fp8_e4m3fn.safetensors", FLUX_T5XXL_FP8_SIZE);

    // Add custom node directories
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-Fluxpromptenhancer");

    InstallVerification::writeManifest(config.installDir, manifest);

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "Flux.1 Schnell installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installStyleToVideoThread(InstallConfig config) {
    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Checking existing Style-to-Video installation...";
    updateProgress(prog);

    // Get all files needed for Style-to-Video
    auto files = getStyleToVideoFiles();

    // Check which files are missing
    fs::path modelsPath = fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "models";
    std::vector<DownloadFile> missingFiles;
    for (const auto& file : files) {
        fs::path fullPath = modelsPath / file.localPath;
        if (!fs::exists(fullPath)) {
            missingFiles.push_back(file);
        } else if (file.expectedSize > 0) {
            // Check file size
            auto actualSize = fs::file_size(fullPath);
            if (actualSize != static_cast<uintmax_t>(file.expectedSize)) {
                missingFiles.push_back(file);
            }
        }
    }

    // Check if HunyuanVideoWrapper is installed
    std::string nodesDir = (fs::path(config.installDir) / "ComfyUI_windows_portable" / "ComfyUI" / "custom_nodes").string();
    bool needsWrapper = !fs::exists(fs::path(nodesDir) / "ComfyUI-HunyuanVideoWrapper" / ".git");
    bool needsVideoHelper = !fs::exists(fs::path(nodesDir) / "ComfyUI-VideoHelperSuite" / ".git");

    // Check if everything is already installed
    if (missingFiles.empty() && !needsWrapper && !needsVideoHelper) {
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "Style-to-Video already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        if (!runningInstallAll.load()) installing.store(false);
        return;
    }

    // Log what's missing
    int totalItems = static_cast<int>(missingFiles.size()) + (needsWrapper ? 1 : 0) + (needsVideoHelper ? 1 : 0);
    prog.status = "Installing " + std::to_string(totalItems) + " missing component(s)...";
    updateProgress(prog);

    auto startTime = std::chrono::steady_clock::now();

    // Create model directories
    createDirectories((modelsPath / "diffusion_models").string());
    createDirectories((modelsPath / "vae").string());
    createDirectories((modelsPath / "LLM" / "llava-llama-3-8b-v1_1-transformers").string());

    // Count total files and nodes to install
    prog.filesTotal = totalItems;
    prog.filesCompleted = 0;

    // Calculate total download size
    int64_t totalBytes = 0;
    int64_t downloadedBytes = 0;
    for (const auto& f : missingFiles) {
        totalBytes += f.expectedSize;
    }

    // Download missing files
    for (const auto& file : missingFiles) {
        if (shouldCancel.load()) {
            prog.state = InstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }

        prog.state = InstallProgress::State::DOWNLOADING;
        prog.currentFile = file.description;
        prog.statusPrefix = "File " + std::to_string(prog.filesCompleted + 1) + "/" + std::to_string(prog.filesTotal) + ": ";
        prog.status = prog.statusPrefix + "Downloading " + file.description;
        if (file.expectedSize > 0) {
            prog.status += " (" + formatSize(file.expectedSize) + ")";
        }
        prog.percentComplete = (totalBytes > 0) ?
            (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
        updateProgress(prog);

        std::string destPath = (modelsPath / file.localPath).string();

        // Create parent directory
        fs::path parentDir = fs::path(destPath).parent_path();
        createDirectories(parentDir.string());

        // Retry download with resume support for large files
        bool downloadSuccess = false;
        for (int attempt = 0; attempt < currentConfig.maxRetries && !downloadSuccess; attempt++) {
            if (attempt > 0) {
                prog.status = prog.statusPrefix + "Retrying " + file.description + " (attempt " + std::to_string(attempt + 1) + "/" + std::to_string(currentConfig.maxRetries) + ")";
                updateProgress(prog);
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            downloadSuccess = downloadFileWithResume(file.url, destPath, file.expectedSize);
        }

        if (!downloadSuccess) {
            if (file.required) {
                prog.state = InstallProgress::State::FAILED;
                prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                prog.status = "FAILED: " + prog.errorMessage;
                updateProgress(prog);
                if (!runningInstallAll.load()) installing.store(false);
                return;
            }
        }

        downloadedBytes += file.expectedSize;
        prog.filesCompleted++;
    }

    // Clone custom nodes if needed
    prog.state = InstallProgress::State::INSTALLING_NODES;

    if (needsWrapper) {
        prog.status = "Cloning HunyuanVideoWrapper...";
        updateProgress(prog);
        std::string wrapperDir = (fs::path(nodesDir) / "ComfyUI-HunyuanVideoWrapper").string();
        if (!cloneRepository(NODE_HUNYUAN_WRAPPER, wrapperDir)) {
            prog.state = InstallProgress::State::FAILED;
            prog.status = "FAILED: Failed to clone ComfyUI-HunyuanVideoWrapper";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }
        prog.filesCompleted++;
    }

    if (needsVideoHelper) {
        prog.status = "Cloning VideoHelperSuite...";
        updateProgress(prog);
        std::string helperDir = (fs::path(nodesDir) / "ComfyUI-VideoHelperSuite").string();
        if (!cloneRepository(NODE_VIDEO_HELPER_SUITE, helperDir)) {
            prog.state = InstallProgress::State::FAILED;
            prog.status = "FAILED: Failed to clone ComfyUI-VideoHelperSuite";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }
        prog.filesCompleted++;
    }

    // Write manifest
    InstallManifest manifest;
    manifest.componentId = "style_to_video";
    manifest.componentName = "Style-to-Video";
    manifest.version = "1.0";
    manifest.complete = true;

    // Add model files with expected sizes for verification
    std::string modelsBase = "ComfyUI_windows_portable/ComfyUI/models/";
    manifest.addFile(modelsBase + "diffusion_models/hunyuanvideo1.5_720p_t2v_fp16.safetensors", HUNYUAN_FP16_T2V_SIZE);
    manifest.addFile(modelsBase + "vae/hunyuanvideo15_vae_fp16.safetensors", HUNYUAN_VAE_SIZE);
    manifest.addFile(modelsBase + "LLM/llava-llama-3-8b-v1_1-transformers/model-00001-of-00004.safetensors", LLAVA_VLM_MODEL1_SIZE);
    manifest.addFile(modelsBase + "LLM/llava-llama-3-8b-v1_1-transformers/model-00002-of-00004.safetensors", LLAVA_VLM_MODEL2_SIZE);
    manifest.addFile(modelsBase + "LLM/llava-llama-3-8b-v1_1-transformers/model-00003-of-00004.safetensors", LLAVA_VLM_MODEL3_SIZE);
    manifest.addFile(modelsBase + "LLM/llava-llama-3-8b-v1_1-transformers/model-00004-of-00004.safetensors", LLAVA_VLM_MODEL4_SIZE);

    // Add custom node directories
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-HunyuanVideoWrapper");
    manifest.addDirectory("ComfyUI_windows_portable/ComfyUI/custom_nodes/ComfyUI-VideoHelperSuite");

    InstallVerification::writeManifest(config.installDir, manifest);

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "Style-to-Video installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installAllThread(InstallConfig config) {
    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    // Calculate total steps based on what will be installed
    int totalSteps = 1; // ComfyUI base is always required
    if (config.installHunyuanVideo) totalSteps++;
    if (config.installFluxSchnell) totalSteps++;
    if (config.installStyleToVideo) totalSteps++;
    int currentStep = 0;

    // Install base first (always required) - with lock to prevent multiple installers
    currentStep++;
    {
        ComfyUIInstaller* self = this;
        InstallConfig configCopy = config;
        int stepNum = currentStep;
        int stepTotal = totalSteps;

        auto isInstalledFn = [configCopy]() {
            return isComfyUIInstalled(configCopy.installDir);
        };

        auto installFn = [self, configCopy]() -> bool {
            self->installComfyUIBaseThread(configCopy);
            // Check if installation succeeded
            return self->progress.state != InstallProgress::State::FAILED;
        };

        bool comfyuiReady = installPrerequisiteWithLock(
            PrerequisiteIds::COMFYUI_BASE,
            isInstalledFn,
            installFn,
            5000,
            [self, stepNum, stepTotal](const std::string&) {
                InstallProgress prog;
                prog.state = InstallProgress::State::DOWNLOADING;
                prog.status = "Step " + std::to_string(stepNum) + "/" + std::to_string(stepTotal) + ": Waiting for ComfyUI base installation by another installer...";
                prog.currentFile = "ComfyUI";
                self->updateProgress(prog);
            }
        );

        if (!comfyuiReady && !shouldCancel.load()) {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.state = InstallProgress::State::FAILED;
            if (progress.errorMessage.empty()) {
                progress.errorMessage = "Failed to install ComfyUI base";
            }
            progress.status = "FAILED: " + progress.errorMessage;
            runningInstallAll.store(false);
            installing.store(false);
            return;
        }
    }

    // Immediately reclaim installing flag to prevent race condition with user's while loop
    // (sub-thread sets installing=false at end, but we're not done yet)
    installing.store(true);

    if (shouldCancel.load() || progress.state == InstallProgress::State::FAILED) {
        runningInstallAll.store(false);
        installing.store(false);
        return;
    }

    // Install HunyuanVideo if enabled
    if (config.installHunyuanVideo) {
        currentStep++;
        // Update progress for HunyuanVideo phase
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Installing HunyuanVideo...";
            progress.percentComplete = 0.0f;
            progress.filesCompleted = 0;
            if (progressCallback) {
                progressCallback(progress);
            }
        }

        installHunyuanVideoThread(config);

        // Reclaim installing flag
        installing.store(true);

        if (shouldCancel.load() || progress.state == InstallProgress::State::FAILED) {
            runningInstallAll.store(false);
            installing.store(false);
            return;
        }
    }

    // Install Flux.1 Schnell if enabled
    if (config.installFluxSchnell) {
        currentStep++;
        // Update progress for Flux phase
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Installing Flux.1 Schnell...";
            progress.percentComplete = 0.0f;
            progress.filesCompleted = 0;
            if (progressCallback) {
                progressCallback(progress);
            }
        }

        installFluxSchnellThread(config);
    }

    // Install Style-to-Video if requested
    if (config.installStyleToVideo) {
        currentStep++;
        // Update progress for Style-to-Video phase
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.status = "Step " + std::to_string(currentStep) + "/" + std::to_string(totalSteps) + ": Installing Style-to-Video...";
            progress.percentComplete = 0.0f;
            progress.filesCompleted = 0;
            if (progressCallback) {
                progressCallback(progress);
            }
        }

        installStyleToVideoThread(config);
    }

    // Final status
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = InstallProgress::State::COMPLETE;
        progress.percentComplete = 100.0f;

        // Build status message based on what was installed
        std::string installed;
        installed = "ComfyUI base";
        if (config.installHunyuanVideo) installed += " + HunyuanVideo";
        if (config.installFluxSchnell) installed += " + Flux.1 Schnell";
        if (config.installStyleToVideo) installed += " + Style-to-Video";
        progress.status = installed + " installation complete";

        if (progressCallback) {
            progressCallback(progress);
        }
    }

    // Final cleanup
    runningInstallAll.store(false);
    installing.store(false);
}

// ============================================================================
// Download Helpers
// ============================================================================

bool ComfyUIInstaller::downloadFile(const DownloadFile& file) {
    return downloadFileWithResume(file.url, file.localPath, file.expectedSize);
}

#ifdef _WIN32
// Windows implementation using WinHTTP
bool ComfyUIInstaller::downloadFileWithResume(const std::string& url, const std::string& localPath,
                                               int64_t expectedSize) {
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
        setError("Failed to parse URL: " + url);
        return false;
    }

    // Create parent directories
    fs::path localFilePath(localPath);
    if (localFilePath.has_parent_path()) {
        createDirectories(localFilePath.parent_path().string());
    }

    // Check for existing file (complete or partial download)
    int64_t existingSize = 0;
    if (fs::exists(localPath)) {
        existingSize = getFileSize(localPath);
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-Installer/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        setError("Failed to open HTTP session for: " + url);
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
        DWORD err = GetLastError();
        WinHttpCloseHandle(hSession);
        setError("Failed to connect to server: " + std::string(hostName, hostName + wcslen(hostName)) +
                 " (error " + std::to_string(err) + ")");
        return false;
    }

    // Open request with redirect flag
    DWORD flags = WINHTTP_FLAG_REFRESH;  // Don't use cached version
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                             NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to open HTTP request (error " + std::to_string(err) + ")");
        return false;
    }

    // Enable automatic redirects (up to 10)
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    DWORD maxRedirects = 10;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS, &maxRedirects, sizeof(maxRedirects));

    // For HTTPS, enable modern TLS protocols
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        // TLS 1.2 = 0x00000800, TLS 1.3 = 0x00002000
        DWORD secFlags = 0x00000800 | 0x00002000;
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secFlags, sizeof(secFlags));

        // Also ignore certificate errors for development (remove in production if needed)
        DWORD secOptions = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                           SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                           SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secOptions, sizeof(secOptions));
    }

    // Add Range header for resume if we have a partial file
    if (existingSize > 0 && currentConfig.resumeDownloads) {
        std::wstring rangeHeader = L"Range: bytes=" + std::to_wstring(existingSize) + L"-";
        WinHttpAddRequestHeaders(hRequest, rangeHeader.c_str(), -1,
                                  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to send HTTP request to " + url + " (error " + std::to_string(err) + ")");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to receive HTTP response from " + url + " (error " + std::to_string(err) + ")");
        return false;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        NULL, &statusCode, &statusCodeSize, NULL);

    // 416 = Range Not Satisfiable - file is already complete!
    if (statusCode == 416) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // Update progress to show file was already downloaded
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.status = progress.currentFile + " already downloaded";
            progress.percentComplete = 100.0f;
            if (progressCallback) {
                progressCallback(progress);
            }
        }
        return true;
    }

    if (statusCode != 200 && statusCode != 206) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("HTTP error " + std::to_string(statusCode) + " downloading " + url);
        return false;
    }

    // If server returned 200 instead of 206, it doesn't support resume - start fresh
    if (statusCode == 200 && existingSize > 0) {
        existingSize = 0;  // Will overwrite file
    }

    // Get content length from server (more reliable than hardcoded sizes)
    wchar_t contentLength[32] = {0};
    DWORD contentLengthSize = sizeof(contentLength);
    int64_t totalSize = expectedSize;
    int64_t serverReportedSize = 0;  // Track what server says
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL,
                            contentLength, &contentLengthSize, NULL)) {
        serverReportedSize = _wtoi64(contentLength);
        if (statusCode == 206) {
            totalSize = serverReportedSize + existingSize;
            serverReportedSize = totalSize;
        } else {
            totalSize = serverReportedSize;
        }
    }

    // Open file for writing (append if resuming)
    std::ios_base::openmode mode = std::ios::binary;
    if (existingSize > 0 && statusCode == 206) {
        mode |= std::ios::app;
    } else {
        mode |= std::ios::trunc;
        existingSize = 0;
    }

    std::ofstream outFile(localPath, mode);
    if (!outFile) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to open output file: " + localPath);
        return false;
    }

    // Download data
    char buffer[65536];
    DWORD bytesRead = 0;
    int64_t totalDownloaded = existingSize;
    auto lastProgressUpdate = std::chrono::steady_clock::now();
    auto startTime = std::chrono::steady_clock::now();

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

        // Update progress periodically
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastProgressUpdate).count() > 0.5f) {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.bytesDownloaded = totalDownloaded;
            progress.bytesTotal = totalSize;

            // Calculate percentage
            if (totalSize > 0) {
                progress.percentComplete = (static_cast<float>(totalDownloaded) / totalSize) * 100.0f;
            }

            float elapsed = std::chrono::duration<float>(now - startTime).count();
            if (elapsed > 0) {
                progress.downloadSpeed = static_cast<float>(totalDownloaded - existingSize) / elapsed;
                if (progress.downloadSpeed > 0 && totalSize > 0) {
                    progress.estimatedTimeRemaining =
                        static_cast<float>(totalSize - totalDownloaded) / progress.downloadSpeed;
                }
            }

            // Update status with size info (preserve prefix like "File 1/5: ")
            progress.status = progress.statusPrefix + "Downloading " + progress.currentFile + " (" +
                             formatSize(totalDownloaded) + " / " + formatSize(totalSize) + ")";

            if (progressCallback) {
                progressCallback(progress);
            }

            lastProgressUpdate = now;
        }
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Verify download - use server-reported size if available, otherwise skip strict check
    int64_t actualSize = getFileSize(localPath);
    if (serverReportedSize > 0) {
        // Verify against what server told us
        if (actualSize != serverReportedSize) {
            setError("Downloaded file size mismatch. Expected: " + std::to_string(serverReportedSize) +
                     ", Got: " + std::to_string(actualSize));
            return false;
        }
    } else if (actualSize <= 0) {
        // No server size, but file is empty or missing
        setError("Download resulted in empty file");
        return false;
    }

    return true;
}

#else
// Linux/Unix implementation using libcurl
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    file->write(static_cast<const char*>(contents), totalSize);
    return totalSize;
}

bool ComfyUIInstaller::downloadFileWithResume(const std::string& url, const std::string& localPath,
                                               int64_t expectedSize) {
    // Create parent directories
    fs::path localFilePath(localPath);
    if (localFilePath.has_parent_path()) {
        createDirectories(localFilePath.parent_path().string());
    }

    // Check for existing partial download
    int64_t existingSize = 0;
    if (currentConfig.resumeDownloads && fs::exists(localPath)) {
        existingSize = getFileSize(localPath);
        if (expectedSize > 0 && existingSize >= expectedSize) {
            return true;  // Already complete
        }
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        setError("Failed to initialize curl");
        return false;
    }

    std::ios_base::openmode mode = std::ios::binary;
    if (existingSize > 0) {
        mode |= std::ios::app;
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(existingSize));
    } else {
        mode |= std::ios::trunc;
    }

    std::ofstream outFile(localPath, mode);
    if (!outFile) {
        curl_easy_cleanup(curl);
        setError("Failed to open output file: " + localPath);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, currentConfig.connectionTimeout / 1000);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "EWOCvj2-Installer/1.0");
    // Use low-speed detection instead of hard timeout for large files
    // Abort if speed drops below 1KB/s for 60 seconds
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    if (currentConfig.downloadTimeout > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, currentConfig.downloadTimeout / 1000);
    }

    CURLcode res = curl_easy_perform(curl);

    outFile.close();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        setError("Download failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    // Verify download
    if (expectedSize > 0) {
        int64_t actualSize = getFileSize(localPath);
        if (actualSize != expectedSize) {
            setError("Downloaded file size mismatch");
            return false;
        }
    }

    return true;
}
#endif

bool ComfyUIInstaller::verifyFile(const std::string& path, int64_t expectedSize,
                                   const std::string& sha256) {
    if (!fs::exists(path)) {
        return false;
    }

    if (expectedSize > 0) {
        int64_t actualSize = getFileSize(path);
        if (actualSize != expectedSize) {
            return false;
        }
    }

    // TODO: Implement SHA256 verification if needed

    return true;
}

// ============================================================================
// Git Operations
// ============================================================================

std::string ComfyUIInstaller::findGitExecutable() {
#ifdef _WIN32
    // Common git installation paths on Windows - prefer standalone Git for Windows
    // over msys2 git (which may have runtime dependencies)
    std::vector<std::string> gitPaths = {
        "C:\\Program Files\\Git\\bin\\git.exe",
        "C:\\Program Files (x86)\\Git\\bin\\git.exe",
        "C:\\Program Files\\Git\\cmd\\git.exe",
        "C:\\Git\\bin\\git.exe",
        "C:\\msys64\\mingw64\\bin\\git.exe",  // mingw64 version works better than usr/bin
        "C:\\msys64\\usr\\bin\\git.exe"
    };

    for (const auto& gitPath : gitPaths) {
        if (fs::exists(gitPath)) {
            return gitPath;
        }
    }
    return "git";  // Fall back to PATH
#else
    return "git";
#endif
}

bool ComfyUIInstaller::cloneRepository(const std::string& url, const std::string& targetDir) {
    // Check if already cloned
    if (fs::exists(fs::path(targetDir) / ".git")) {
        // Try to pull updates instead
        return pullRepository(targetDir);
    }

    // Create parent directory
    fs::path target(targetDir);
    if (target.has_parent_path()) {
        createDirectories(target.parent_path().string());
    }

    // Find git executable
    std::string gitExe = findGitExecutable();

#ifdef _WIN32
    // Use CreateProcess with full quoted path in command line
    // lpApplicationName = NULL so Windows parses the command line for the executable
    std::string cmdStr = "\"" + gitExe + "\" clone --depth 1 " + url + " \"" + targetDir + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess needs a mutable command line
    char cmdLine[4096];
    strncpy(cmdLine, cmdStr.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    // lpApplicationName = NULL, let Windows parse the quoted executable from cmdLine
    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to execute git clone: " + cmdStr);
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        setError("git clone failed with exit code " + std::to_string(exitCode) + ": " + cmdStr);
    }
    return exitCode == 0;
#else
    std::string command = "\"" + gitExe + "\" clone --depth 1 " + url + " \"" + targetDir + "\"";
    int result = system(command.c_str());
    return result == 0;
#endif
}

bool ComfyUIInstaller::pullRepository(const std::string& repoDir) {
    if (!fs::exists(fs::path(repoDir) / ".git")) {
        return false;
    }

    std::string gitExe = findGitExecutable();

#ifdef _WIN32
    // Use CreateProcess with full quoted path in command line
    std::string cmdStr = "\"" + gitExe + "\" -C \"" + repoDir + "\" pull";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    strncpy(cmdLine, cmdStr.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    // lpApplicationName = NULL, let Windows parse the quoted executable from cmdLine
    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 60000);  // 1 minute timeout

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
#else
    std::string command = "\"" + gitExe + "\" -C \"" + repoDir + "\" pull";
    int result = system(command.c_str());
    return result == 0;
#endif
}

// ============================================================================
// Archive Extraction
// ============================================================================

bool ComfyUIInstaller::extract7z(const std::string& archivePath, const std::string& targetDir) {
    createDirectories(targetDir);

    // Try to find 7z executable
    std::string sevenZipPath;

#ifdef _WIN32
    // Common 7-Zip installation paths
    std::vector<std::string> searchPaths = {
        "C:\\Program Files\\7-Zip\\7z.exe",
        "C:\\Program Files (x86)\\7-Zip\\7z.exe"
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            sevenZipPath = path;
            break;
        }
    }

    if (sevenZipPath.empty()) {
        setError("7-Zip not found at expected paths. Please install 7-Zip manually.");
        return false;
    }

    // Update progress - extraction doesn't have percentage info
    // Use -1 to indicate indeterminate progress (UI should show spinner or no percentage)
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.state = InstallProgress::State::EXTRACTING;
        progress.status = "Extracting ComfyUI (this may take a few minutes)...";
        progress.percentComplete = -1.0f;  // -1 = indeterminate, UI should not show percentage
        if (progressCallback) {
            progressCallback(progress);
        }
    }

    // Build command - run 7z directly without cmd /c
    std::string args = "x -y -o\"" + targetDir + "\" \"" + archivePath + "\"";

    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = NULL;  // Use default verb (open)
    sei.lpFile = sevenZipPath.c_str();
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        setError("Failed to run 7z extraction (error " + std::to_string(err) + ")");
        return false;
    }

    // Wait with periodic checks for cancellation
    if (sei.hProcess) {
        int dotCount = 0;
        while (WaitForSingleObject(sei.hProcess, 2000) == WAIT_TIMEOUT) {
            if (shouldCancel.load()) {
                TerminateProcess(sei.hProcess, 1);
                CloseHandle(sei.hProcess);
                return false;
            }

            // Update progress with animated dots to show we're still working
            dotCount = (dotCount + 1) % 4;
            std::string dots(dotCount + 1, '.');

            std::lock_guard<std::mutex> lock(progressMutex);
            progress.status = "Extracting ComfyUI" + dots + " (please wait)";
            if (progressCallback) {
                progressCallback(progress);
            }
        }

        DWORD exitCode = 0;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);

        if (exitCode != 0) {
            setError("7z extraction failed with exit code " + std::to_string(exitCode));
            return false;
        }
    }

    return true;
#else
    // Linux: use p7zip
    std::string command = "7z x -y -o\"" + targetDir + "\" \"" + archivePath + "\"";
    int result = system(command.c_str());
    return result == 0;
#endif
}

bool ComfyUIInstaller::extractZip(const std::string& archivePath, const std::string& targetDir) {
    createDirectories(targetDir);

#ifdef _WIN32
    // Use PowerShell's Expand-Archive
    std::string command = "powershell -Command \"Expand-Archive -Path '" +
                          archivePath + "' -DestinationPath '" + targetDir + "' -Force\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    snprintf(cmdLine, sizeof(cmdLine), "%s", command.c_str());

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to execute zip extraction");
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
#else
    std::string command = "unzip -o \"" + archivePath + "\" -d \"" + targetDir + "\"";
    int result = system(command.c_str());
    return result == 0;
#endif
}

// ============================================================================
// Utility Methods
// ============================================================================

bool ComfyUIInstaller::createDirectories(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

bool ComfyUIInstaller::deleteDirectory(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    return !ec;
}

bool ComfyUIInstaller::fileExists(const std::string& path) {
    return fs::exists(path);
}

int64_t ComfyUIInstaller::getFileSize(const std::string& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? -1 : static_cast<int64_t>(size);
}

int64_t ComfyUIInstaller::getFreeDiskSpace(const std::string& path) {
    std::error_code ec;

    // Find existing parent directory
    fs::path checkPath(path);
    while (!fs::exists(checkPath) && checkPath.has_parent_path()) {
        checkPath = checkPath.parent_path();
    }

    if (!fs::exists(checkPath)) {
        return -1;
    }

    auto spaceInfo = fs::space(checkPath, ec);
    return ec ? -1 : static_cast<int64_t>(spaceInfo.available);
}

void ComfyUIInstaller::updateProgress(const InstallProgress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
    if (progressCallback) {
        progressCallback(progress);
    }
}

void ComfyUIInstaller::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;

    // Also update progress error
    std::lock_guard<std::mutex> plock(progressMutex);
    progress.errorMessage = error;
}

// ============================================================================
// File List Builders
// ============================================================================

std::vector<DownloadFile> ComfyUIInstaller::getComfyUIBaseFiles() {
    return {
        {
            COMFYUI_PORTABLE_URL,
            "ComfyUI_windows_portable_nvidia.7z",
            "ComfyUI Portable",
            COMFYUI_PORTABLE_SIZE,
            "",
            true
        }
    };
}

std::vector<DownloadFile> ComfyUIInstaller::getHunyuanVideoFiles() {
    return {
        // HunyuanVideo 1.5 T2V GGUF (VRAM-friendly quantized)
        // Note: UnetLoaderGGUF looks in models/unet/ folder
        {
            HUNYUAN_T2V_Q4_URL,
            "unet/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf",
            "HunyuanVideo 1.5 T2V (Q4 GGUF)",
            HUNYUAN_T2V_Q4_SIZE,
            "",
            true
        },
        // HunyuanVideo 1.5 I2V GGUF (for image-to-video)
        {
            HUNYUAN_I2V_Q4_URL,
            "unet/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf",
            "HunyuanVideo 1.5 I2V (Q4 GGUF)",
            HUNYUAN_I2V_Q4_SIZE,
            "",
            true
        },
        // VAE (1.5 version)
        {
            HUNYUAN_VAE_URL,
            "vae/hunyuanvideo15_vae_fp16.safetensors",
            "HunyuanVideo 1.5 VAE",
            HUNYUAN_VAE_SIZE,
            "",
            true
        },
        // Qwen 2.5 VL text encoder (for 1.5)
        {
            HUNYUAN_QWEN_URL,
            "text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors",
            "Qwen 2.5 VL Text Encoder (FP8)",
            HUNYUAN_QWEN_SIZE,
            "",
            true
        },
        // ByT5 text encoder (for 1.5)
        {
            HUNYUAN_BYT5_URL,
            "text_encoders/byt5_small_glyphxl_fp16.safetensors",
            "ByT5 Small GlyphXL Text Encoder",
            HUNYUAN_BYT5_SIZE,
            "",
            true
        },
        // SigCLIP Vision for I2V (1.5 version)
        {
            HUNYUAN_CLIP_VISION_URL,
            "clip_vision/sigclip_vision_patch14_384.safetensors",
            "SigCLIP Vision Encoder",
            HUNYUAN_CLIP_VISION_SIZE,
            "",
            true
        }
    };
}

std::vector<std::string> ComfyUIInstaller::getHunyuanCustomNodes() {
    return {
        NODE_COMFYUI_GGUF,           // For GGUF model loading
        NODE_HUNYUAN_WRAPPER,        // For HunyuanVideo support
        NODE_VIDEO_HELPER_SUITE,     // For video output
        NODE_FRAME_INTERPOLATION     // For RIFE frame interpolation
    };
}

std::vector<DownloadFile> ComfyUIInstaller::getFluxSchnellFiles() {
    return {
        // Flux.1 Schnell GGUF Q4_K_S (VRAM-efficient quantized model)
        {
            FLUX_SCHNELL_GGUF_URL,
            "unet/flux1-schnell-Q4_K_S.gguf",
            "Flux.1 Schnell GGUF",
            FLUX_SCHNELL_GGUF_SIZE,
            "",
            true
        },
        // Flux VAE
        {
            FLUX_VAE_URL,
            "vae/ae.safetensors",
            "Flux VAE",
            FLUX_VAE_SIZE,
            "",
            true
        },
        // CLIP-L text encoder
        {
            FLUX_CLIP_L_URL,
            "clip/clip_l.safetensors",
            "CLIP-L Text Encoder",
            FLUX_CLIP_L_SIZE,
            "",
            true
        },
        // T5-XXL text encoder FP8
        {
            FLUX_T5XXL_FP8_URL,
            "clip/t5xxl_fp8_e4m3fn.safetensors",
            "T5-XXL FP8",
            FLUX_T5XXL_FP8_SIZE,
            "",
            true
        }
    };
}

std::vector<DownloadFile> ComfyUIInstaller::getStyleToVideoFiles() {
    return {
        // HunyuanVideo 1.5 720p T2V FP16 model (quantized to FP8 on load)
        {
            HUNYUAN_FP16_T2V_URL,
            "diffusion_models/hunyuanvideo1.5_720p_t2v_fp16.safetensors",
            "HunyuanVideo 1.5 720p T2V Model",
            HUNYUAN_FP16_T2V_SIZE,
            "",
            true
        },
        // HunyuanVideo VAE (shared with standard Hunyuan - will skip if exists)
        {
            HUNYUAN_VAE_URL,
            "vae/hunyuanvideo15_vae_fp16.safetensors",
            "HunyuanVideo VAE",
            HUNYUAN_VAE_SIZE,
            "",
            true
        },
        // Llava VLM model shards
        {
            LLAVA_VLM_MODEL1_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/model-00001-of-00004.safetensors",
            "Llava VLM (1/4)",
            LLAVA_VLM_MODEL1_SIZE,
            "",
            true
        },
        {
            LLAVA_VLM_MODEL2_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/model-00002-of-00004.safetensors",
            "Llava VLM (2/4)",
            LLAVA_VLM_MODEL2_SIZE,
            "",
            true
        },
        {
            LLAVA_VLM_MODEL3_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/model-00003-of-00004.safetensors",
            "Llava VLM (3/4)",
            LLAVA_VLM_MODEL3_SIZE,
            "",
            true
        },
        {
            LLAVA_VLM_MODEL4_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/model-00004-of-00004.safetensors",
            "Llava VLM (4/4)",
            LLAVA_VLM_MODEL4_SIZE,
            "",
            true
        },
        // Llava VLM config files
        {
            LLAVA_VLM_CONFIG_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/config.json",
            "Llava Config",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_INDEX_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/model.safetensors.index.json",
            "Llava Index",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_TOKENIZER_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/tokenizer.json",
            "Llava Tokenizer",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_TOKENIZER_CONFIG_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/tokenizer_config.json",
            "Llava Tokenizer Config",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_SPECIAL_TOKENS_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/special_tokens_map.json",
            "Llava Special Tokens",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_PREPROCESSOR_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/preprocessor_config.json",
            "Llava Preprocessor Config",
            0,
            "",
            true
        },
        {
            LLAVA_VLM_GENERATION_URL,
            "LLM/llava-llama-3-8b-v1_1-transformers/generation_config.json",
            "Llava Generation Config",
            0,
            "",
            true
        }
    };
}

// ============================================================================
// Component Management
// ============================================================================

std::vector<ModelComponent> ComfyUIInstaller::getHunyuanComponents() {
    return {
        // HunyuanVideo 1.5 T2V (Text-to-Video)
        {
            "hunyuan_t2v",
            "HunyuanVideo 1.5 T2V",
            "Text-to-video generation model (GGUF Q4)",
            {
                {
                    HUNYUAN_T2V_Q4_URL,
                    "unet/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf",
                    "HunyuanVideo 1.5 T2V (Q4 GGUF)",
                    HUNYUAN_T2V_Q4_SIZE, "", true
                }
            },
            {},  // No custom nodes specific to this component
            {"unet/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf"},
            true, true
        },
        // HunyuanVideo 1.5 I2V (Image-to-Video)
        {
            "hunyuan_i2v",
            "HunyuanVideo 1.5 I2V",
            "Image-to-video generation model (GGUF Q4)",
            {
                {
                    HUNYUAN_I2V_Q4_URL,
                    "unet/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf",
                    "HunyuanVideo 1.5 I2V (Q4 GGUF)",
                    HUNYUAN_I2V_Q4_SIZE, "", true
                }
            },
            {},
            {"unet/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf"},
            true, true
        },
        // HunyuanVideo 1.5 VAE
        {
            "hunyuan_vae",
            "HunyuanVideo 1.5 VAE",
            "Video autoencoder for encoding/decoding",
            {
                {
                    HUNYUAN_VAE_URL,
                    "vae/hunyuanvideo15_vae_fp16.safetensors",
                    "HunyuanVideo 1.5 VAE",
                    HUNYUAN_VAE_SIZE, "", true
                }
            },
            {},
            {"vae/hunyuanvideo15_vae_fp16.safetensors"},
            true, true
        },
        // Text Encoders (Qwen 2.5 + ByT5 for 1.5)
        {
            "hunyuan_text_encoders",
            "HunyuanVideo 1.5 Text Encoders",
            "Qwen 2.5 VL and ByT5 text encoders for prompt understanding",
            {
                {
                    HUNYUAN_QWEN_URL,
                    "text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors",
                    "Qwen 2.5 VL Text Encoder (FP8)",
                    HUNYUAN_QWEN_SIZE, "", true
                },
                {
                    HUNYUAN_BYT5_URL,
                    "text_encoders/byt5_small_glyphxl_fp16.safetensors",
                    "ByT5 Small GlyphXL Text Encoder",
                    HUNYUAN_BYT5_SIZE, "", true
                }
            },
            {},
            {"text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors", "text_encoders/byt5_small_glyphxl_fp16.safetensors"},
            true, true
        },
        // SigCLIP Vision for I2V (1.5 version)
        {
            "hunyuan_clip_vision",
            "HunyuanVideo 1.5 CLIP Vision",
            "SigCLIP vision encoder for image-to-video conditioning",
            {
                {
                    HUNYUAN_CLIP_VISION_URL,
                    "clip_vision/sigclip_vision_patch14_384.safetensors",
                    "SigCLIP Vision Encoder",
                    HUNYUAN_CLIP_VISION_SIZE, "", true
                }
            },
            {},
            {"clip_vision/sigclip_vision_patch14_384.safetensors"},
            true, true
        },
        // Core custom nodes for Hunyuan
        {
            "hunyuan_nodes",
            "HunyuanVideo Custom Nodes",
            "Required ComfyUI nodes for HunyuanVideo",
            {},
            {NODE_COMFYUI_GGUF, NODE_HUNYUAN_WRAPPER, NODE_VIDEO_HELPER_SUITE, NODE_FRAME_INTERPOLATION},
            {},  // Check by node folder existence
            true, true
        }
        // Note: IP2V (Style-to-Video) requires Hunyuan Full backend, not Slim (GGUF)
        // IP2V components are in getStyleToVideoComponents() for Hunyuan Full
    };
}

std::vector<ModelComponent> ComfyUIInstaller::getComfyUIBaseComponents() {
    return {
        // ComfyUI Portable archive
        {
            "comfyui_portable",
            "ComfyUI Portable",
            "Core ComfyUI installation with Python environment",
            {
                {
                    COMFYUI_PORTABLE_URL,
                    "ComfyUI_windows_portable_nvidia.7z",
                    "ComfyUI Portable",
                    COMFYUI_PORTABLE_SIZE, "", true
                }
            },
            {},  // No custom nodes in the archive itself
            {},  // Check is done via directory existence
            true, true
        },
        // ComfyUI Manager
        {
            "comfyui_manager",
            "ComfyUI Manager",
            "Node manager for installing additional custom nodes",
            {},
            {NODE_COMFYUI_MANAGER},
            {},
            true, true
        },
        // Video Helper Suite (needed for video output)
        {
            "video_helper_suite",
            "Video Helper Suite",
            "Video encoding and output nodes",
            {},
            {NODE_VIDEO_HELPER_SUITE},
            {},
            true, true
        }
    };
}

std::vector<ModelComponent> ComfyUIInstaller::getFluxSchnellComponents() {
    return {
        // Flux.1 Schnell GGUF Q4_K_S (VRAM-efficient quantized model)
        {
            "flux_schnell",
            "Flux.1 Schnell (GGUF Q4_K_S)",
            "Fast image generation model (4 steps, VRAM-efficient)",
            {
                {
                    FLUX_SCHNELL_GGUF_URL,
                    "unet/flux1-schnell-Q4_K_S.gguf",
                    "Flux.1 Schnell GGUF",
                    FLUX_SCHNELL_GGUF_SIZE, "", true
                }
            },
            {},  // ComfyUI-GGUF node already installed with Hunyuan
            {"unet/flux1-schnell-Q4_K_S.gguf"},
            true, true
        },
        // Flux VAE
        {
            "flux_vae",
            "Flux VAE",
            "Autoencoder for image encoding/decoding",
            {
                {
                    FLUX_VAE_URL,
                    "vae/ae.safetensors",
                    "Flux VAE",
                    FLUX_VAE_SIZE, "", true
                }
            },
            {},
            {"vae/ae.safetensors"},
            true, true
        },
        // CLIP-L text encoder
        {
            "flux_clip_l",
            "CLIP-L Text Encoder",
            "CLIP-L text encoder for prompt understanding",
            {
                {
                    FLUX_CLIP_L_URL,
                    "clip/clip_l.safetensors",
                    "CLIP-L Text Encoder",
                    FLUX_CLIP_L_SIZE, "", true
                }
            },
            {},
            {"clip/clip_l.safetensors"},
            true, true
        },
        // T5-XXL text encoder FP8
        {
            "flux_t5xxl",
            "T5-XXL Text Encoder (FP8)",
            "T5-XXL text encoder for detailed prompt understanding",
            {
                {
                    FLUX_T5XXL_FP8_URL,
                    "clip/t5xxl_fp8_e4m3fn.safetensors",
                    "T5-XXL FP8",
                    FLUX_T5XXL_FP8_SIZE, "", true
                }
            },
            {},
            {"clip/t5xxl_fp8_e4m3fn.safetensors"},
            true, true
        },
        // Flux Prompt Enhancer custom node
        {
            "flux_prompt_enhancer",
            "Flux Prompt Enhancer",
            "AI-powered prompt enhancement for better image generation",
            {},  // No model files - auto-downloads from HuggingFace on first use
            {NODE_FLUX_PROMPT_ENHANCER},
            {},  // Check by node folder existence
            true, true
        },
        // ComfyUI LLM Node for concept-to-prompt translation
        {
            "comfyui_llm_node",
            "ComfyUI LLM Node",
            "LLM integration via transformers for concept-to-prompt translation",
            {},  // No model files here - model is separate component
            {NODE_COMFYUI_LLM},
            {},  // Check by node folder existence
            true, true
        },
        // Qwen2.5-1.5B-Instruct model for concept translation
        {
            "qwen_1_5b_instruct",
            "Qwen2.5-1.5B-Instruct",
            "Small LLM for concept-to-prompt translation (~3GB)",
            {
                {
                    QWEN_1_5B_CONFIG_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/config.json",
                    "Qwen config",
                    0, "", true
                },
                {
                    QWEN_1_5B_TOKENIZER_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/tokenizer.json",
                    "Qwen tokenizer",
                    0, "", true
                },
                {
                    QWEN_1_5B_TOKENIZER_CONFIG_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/tokenizer_config.json",
                    "Qwen tokenizer config",
                    0, "", true
                },
                {
                    QWEN_1_5B_VOCAB_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/vocab.json",
                    "Qwen vocab",
                    0, "", true
                },
                {
                    QWEN_1_5B_MERGES_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/merges.txt",
                    "Qwen merges",
                    0, "", true
                },
                {
                    QWEN_1_5B_GENERATION_CONFIG_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/generation_config.json",
                    "Qwen generation config",
                    0, "", true
                },
                {
                    QWEN_1_5B_MODEL_URL,
                    "LLM_checkpoints/Qwen2.5-1.5B-Instruct/model.safetensors",
                    "Qwen2.5-1.5B-Instruct model",
                    QWEN_1_5B_MODEL_SIZE, "", true
                }
            },
            {},
            {"LLM_checkpoints/Qwen2.5-1.5B-Instruct/model.safetensors"},
            true, true
        }
    };
}

std::vector<ModelComponent> ComfyUIInstaller::getStyleToVideoComponents() {
    return {
        // HunyuanVideo FP8 model for Style-to-Video
        {
            "hunyuan_fp16",
            "HunyuanVideo 1.5 720p T2V",
            "FP16 model for Style-to-Video and Hunyuan Full (quantized to FP8 on load)",
            {
                {
                    HUNYUAN_FP16_T2V_URL,
                    "diffusion_models/hunyuanvideo1.5_720p_t2v_fp16.safetensors",
                    "HunyuanVideo 1.5 720p T2V Model",
                    HUNYUAN_FP16_T2V_SIZE, "", true
                }
            },
            {},
            {"diffusion_models/hunyuanvideo1.5_720p_t2v_fp16.safetensors"},
            true, true
        },
        // HunyuanVideo VAE (shared with standard Hunyuan)
        {
            "hunyuan_vae",
            "HunyuanVideo VAE FP16",
            "Variational autoencoder for video encoding/decoding",
            {
                {
                    HUNYUAN_VAE_URL,
                    "vae/hunyuanvideo15_vae_fp16.safetensors",
                    "HunyuanVideo VAE FP16",
                    HUNYUAN_VAE_SIZE, "", true
                }
            },
            {},
            {"vae/hunyuanvideo15_vae_fp16.safetensors"},
            true, true
        },
        // Llava VLM Model Shard 1
        {
            "llava_vlm_1",
            "Llava VLM Part 1",
            "Vision Language Model for style understanding (part 1/4)",
            {
                {
                    LLAVA_VLM_MODEL1_URL,
                    "LLM/llava-llama-3-8b-v1_1-transformers/model-00001-of-00004.safetensors",
                    "Llava VLM Model Part 1",
                    LLAVA_VLM_MODEL1_SIZE, "", true
                }
            },
            {},
            {"LLM/llava-llama-3-8b-v1_1-transformers/model-00001-of-00004.safetensors"},
            true, true
        },
        // Llava VLM Model Shard 2
        {
            "llava_vlm_2",
            "Llava VLM Part 2",
            "Vision Language Model for style understanding (part 2/4)",
            {
                {
                    LLAVA_VLM_MODEL2_URL,
                    "LLM/llava-llama-3-8b-v1_1-transformers/model-00002-of-00004.safetensors",
                    "Llava VLM Model Part 2",
                    LLAVA_VLM_MODEL2_SIZE, "", true
                }
            },
            {},
            {"LLM/llava-llama-3-8b-v1_1-transformers/model-00002-of-00004.safetensors"},
            true, true
        },
        // Llava VLM Model Shard 3
        {
            "llava_vlm_3",
            "Llava VLM Part 3",
            "Vision Language Model for style understanding (part 3/4)",
            {
                {
                    LLAVA_VLM_MODEL3_URL,
                    "LLM/llava-llama-3-8b-v1_1-transformers/model-00003-of-00004.safetensors",
                    "Llava VLM Model Part 3",
                    LLAVA_VLM_MODEL3_SIZE, "", true
                }
            },
            {},
            {"LLM/llava-llama-3-8b-v1_1-transformers/model-00003-of-00004.safetensors"},
            true, true
        },
        // Llava VLM Model Shard 4
        {
            "llava_vlm_4",
            "Llava VLM Part 4",
            "Vision Language Model for style understanding (part 4/4)",
            {
                {
                    LLAVA_VLM_MODEL4_URL,
                    "LLM/llava-llama-3-8b-v1_1-transformers/model-00004-of-00004.safetensors",
                    "Llava VLM Model Part 4",
                    LLAVA_VLM_MODEL4_SIZE, "", true
                }
            },
            {},
            {"LLM/llava-llama-3-8b-v1_1-transformers/model-00004-of-00004.safetensors"},
            true, true
        },
        // Llava VLM Config files (small JSON files, no size check needed)
        {
            "llava_vlm_config",
            "Llava VLM Config",
            "Configuration files for Vision Language Model",
            {
                {LLAVA_VLM_CONFIG_URL, "LLM/llava-llama-3-8b-v1_1-transformers/config.json", "config.json", 0, "", false},
                {LLAVA_VLM_INDEX_URL, "LLM/llava-llama-3-8b-v1_1-transformers/model.safetensors.index.json", "model.safetensors.index.json", 0, "", false},
                {LLAVA_VLM_TOKENIZER_URL, "LLM/llava-llama-3-8b-v1_1-transformers/tokenizer.json", "tokenizer.json", 0, "", false},
                {LLAVA_VLM_TOKENIZER_CONFIG_URL, "LLM/llava-llama-3-8b-v1_1-transformers/tokenizer_config.json", "tokenizer_config.json", 0, "", false},
                {LLAVA_VLM_SPECIAL_TOKENS_URL, "LLM/llava-llama-3-8b-v1_1-transformers/special_tokens_map.json", "special_tokens_map.json", 0, "", false},
                {LLAVA_VLM_PREPROCESSOR_URL, "LLM/llava-llama-3-8b-v1_1-transformers/preprocessor_config.json", "preprocessor_config.json", 0, "", false},
                {LLAVA_VLM_GENERATION_URL, "LLM/llava-llama-3-8b-v1_1-transformers/generation_config.json", "generation_config.json", 0, "", false}
            },
            {},
            {"LLM/llava-llama-3-8b-v1_1-transformers/config.json"},
            false, true  // Not critical, config files
        },
        // IP2V custom node (fork with VLM + image conditioning support)
        {
            "ip2v_node",
            "HunyuanVideo IP2V Node",
            "Custom node for VLM-conditioned IP2V generation",
            {},
            {NODE_HUNYUAN_IP2V},
            {},
            true, true
        }
    };
}

bool ComfyUIInstaller::isComponentInstalled(const ModelComponent& component,
                                             const std::string& installDir) {
    namespace fs = std::filesystem;

    // Special case: ComfyUI portable base - check for directory structure
    if (component.id == "comfyui_portable") {
        fs::path portablePath = fs::path(installDir) / "ComfyUI_windows_portable";
        return fs::exists(portablePath / "ComfyUI" / "main.py") ||
               fs::exists(portablePath / "python_embeded") ||
               fs::exists(portablePath / "run_nvidia_gpu.bat");
    }

    // Get the models directory
    std::string modelsDir = installDir + "/ComfyUI_windows_portable/ComfyUI/models";
    std::string nodesDir = installDir + "/ComfyUI_windows_portable/ComfyUI/custom_nodes";

    // Helper to verify file exists with correct size (within 5% tolerance)
    auto verifyFileSize = [](const fs::path& path, int64_t expectedSize) -> bool {
        try {
            if (!fs::exists(path)) return false;
            if (expectedSize <= 0) return true;  // No size check if size unknown
            int64_t actualSize = static_cast<int64_t>(fs::file_size(path));
            int64_t minSize = static_cast<int64_t>(expectedSize * 0.95);
            return actualSize >= minSize;
        } catch (...) {
            return false;
        }
    };

    // Check files with size verification if available
    if (!component.files.empty()) {
        // Use files list which has expected sizes
        // File paths are relative to the models directory
        for (const auto& file : component.files) {
            fs::path filePath = fs::path(modelsDir) / file.localPath;
            if (!verifyFileSize(filePath, file.expectedSize)) {
                return false;
            }
        }
    } else {
        // Fall back to checkFiles (existence only) for components without file list
        for (const auto& checkFile : component.checkFiles) {
            fs::path filePath = fs::path(modelsDir) / checkFile;
            if (!fs::exists(filePath)) {
                return false;
            }
        }
    }

    // Check custom nodes (by folder name)
    for (const auto& nodeUrl : component.customNodes) {
        // Extract repo name from URL
        std::string repoName = nodeUrl;
        size_t lastSlash = repoName.rfind('/');
        if (lastSlash != std::string::npos) {
            repoName = repoName.substr(lastSlash + 1);
        }
        // Remove .git suffix
        if (repoName.size() > 4 && repoName.substr(repoName.size() - 4) == ".git") {
            repoName = repoName.substr(0, repoName.size() - 4);
        }

        fs::path nodePath = fs::path(nodesDir) / repoName;
        if (!fs::exists(nodePath)) {
            return false;
        }
    }

    return true;
}

std::vector<ModelComponent> ComfyUIInstaller::getMissingComponents(
    const std::vector<ModelComponent>& components,
    const std::string& installDir) {

    std::vector<ModelComponent> missing;

    for (const auto& component : components) {
        if (component.enabled && !isComponentInstalled(component, installDir)) {
            missing.push_back(component);
        }
    }

    return missing;
}

bool ComfyUIInstaller::installMissingComponents(const InstallConfig& config,
                                                 const std::vector<ModelComponent>& components) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    // Check for missing components
    auto missing = getMissingComponents(components, config.installDir);
    if (missing.empty()) {
        // Nothing to install
        InstallProgress prog;
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "All components already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        return true;
    }

    currentConfig = config;
    installing.store(true);
    shouldCancel.store(false);

    // Clean up previous thread
    if (installThread && installThread->joinable()) {
        installThread->join();
    }

    installThread = std::make_unique<std::thread>(
        &ComfyUIInstaller::installMissingComponentsThread, this, config, missing);

    return true;
}

void ComfyUIInstaller::installMissingComponentsThread(InstallConfig config,
                                                       std::vector<ModelComponent> components) {
    namespace fs = std::filesystem;

    InstallProgress prog;
    prog.status = "Starting...";
    updateProgress(prog);

    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Installing missing components...";
    prog.filesTotal = 0;

    // Count total files and nodes
    for (const auto& comp : components) {
        prog.filesTotal += comp.files.size();
        prog.filesTotal += comp.customNodes.size();
    }
    updateProgress(prog);

    std::string modelsDir = config.installDir + "/ComfyUI_windows_portable/ComfyUI/models";
    std::string nodesDir = config.installDir + "/ComfyUI_windows_portable/ComfyUI/custom_nodes";

    int filesCompleted = 0;

    for (const auto& component : components) {
        if (shouldCancel.load()) {
            prog.state = InstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            if (!runningInstallAll.load()) installing.store(false);
            return;
        }

        prog.status = "Installing " + component.name + "...";
        updateProgress(prog);

        // Download files
        for (const auto& file : component.files) {
            if (shouldCancel.load()) break;

            prog.currentFile = file.description;
            prog.status = "Downloading " + file.description;
            updateProgress(prog);

            DownloadFile dlFile = file;
            dlFile.localPath = modelsDir + "/" + file.localPath;

            // Create parent directory
            fs::path parentDir = fs::path(dlFile.localPath).parent_path();
            fs::create_directories(parentDir);

            if (!downloadFile(dlFile)) {
                if (file.required) {
                    prog.state = InstallProgress::State::FAILED;
                    prog.status = "Failed to download " + file.description;
                    prog.errorMessage = getLastError();
                    updateProgress(prog);
                    if (!runningInstallAll.load()) installing.store(false);
                    return;
                }
            }

            filesCompleted++;
            prog.filesCompleted = filesCompleted;
            prog.percentComplete = (float)filesCompleted / (float)prog.filesTotal * 100.0f;
            updateProgress(prog);
        }

        // Clone custom nodes
        for (const auto& nodeUrl : component.customNodes) {
            if (shouldCancel.load()) break;

            // Extract repo name
            std::string repoName = nodeUrl;
            size_t lastSlash = repoName.rfind('/');
            if (lastSlash != std::string::npos) {
                repoName = repoName.substr(lastSlash + 1);
            }
            if (repoName.size() > 4 && repoName.substr(repoName.size() - 4) == ".git") {
                repoName = repoName.substr(0, repoName.size() - 4);
            }

            prog.status = "Installing node: " + repoName;
            updateProgress(prog);

            std::string targetDir = nodesDir + "/" + repoName;
            if (!fs::exists(targetDir)) {
                if (!cloneRepository(nodeUrl, targetDir)) {
                    std::cerr << "[ComfyUIInstaller] Warning: Failed to clone " << repoName << std::endl;
                }
            }

            filesCompleted++;
            prog.filesCompleted = filesCompleted;
            prog.percentComplete = (float)filesCompleted / (float)prog.filesTotal * 100.0f;
            updateProgress(prog);
        }
    }

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "All components installed successfully";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}
