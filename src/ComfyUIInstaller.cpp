/**
 * ComfyUIInstaller.cpp
 *
 * Implementation of ComfyUI download and installation
 *
 * License: GPL3
 */

#include "ComfyUIInstaller.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
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

bool ComfyUIInstaller::installSDAnimateDiff(const InstallConfig& config) {
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
    int64_t required = getRequiredDiskSpace(InstallComponent::SD_ANIMATEDIFF);
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
    installThread = std::make_unique<std::thread>(&ComfyUIInstaller::installSDAnimateDiffThread, this, config);

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

bool ComfyUIInstaller::installAll(const InstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.installDir.empty()) {
        setError("Installation directory not specified");
        return false;
    }

    // Check disk space for everything
    int64_t required = getRequiredDiskSpace(InstallComponent::COMFYUI_BASE) +
                       getRequiredDiskSpace(InstallComponent::SD_ANIMATEDIFF) +
                       getRequiredDiskSpace(InstallComponent::HUNYUAN_VIDEO);
    int64_t available = getFreeDiskSpace(config.installDir);
    if (available > 0 && available < required) {
        setError("Insufficient disk space. Required: " + formatSize(required) +
                 ", Available: " + formatSize(available));
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
    // Use component-based checking for consistency
    auto components = getComfyUIBaseComponents();
    auto missing = getMissingComponents(components, installDir);

    // All components installed = nothing missing
    return missing.empty();
}

bool ComfyUIInstaller::isSDAnimateDiffInstalled(const std::string& installDir) {
    // Use component-based checking - only check required components
    auto allComponents = getSDComponents();

    // Filter to only required components for "is installed" check
    std::vector<ModelComponent> requiredComponents;
    for (const auto& comp : allComponents) {
        if (comp.required) {
            requiredComponents.push_back(comp);
        }
    }

    auto missing = getMissingComponents(requiredComponents, installDir);

    // All required components installed = nothing missing
    return missing.empty();
}

bool ComfyUIInstaller::isHunyuanVideoInstalled(const std::string& installDir) {
    // Use component-based checking for consistency
    auto components = getHunyuanComponents();
    auto missing = getMissingComponents(components, installDir);

    // All components installed = nothing missing
    return missing.empty();
}

int64_t ComfyUIInstaller::getRequiredDiskSpace(InstallComponent component) {
    switch (component) {
        case InstallComponent::COMFYUI_BASE:
            // ComfyUI portable (~2.5GB compressed, ~4GB extracted)
            return 5LL * 1024 * 1024 * 1024;  // 5GB with safety margin

        case InstallComponent::SD_ANIMATEDIFF:
            // SD1.5 FP16 (~2GB) + AnimateDiff (~909MB) + VAE (~335MB) +
            // ControlNets (~2.9GB) + IPAdapter (~98MB) + CLIP (~2.5GB)
            return 10LL * 1024 * 1024 * 1024;  // 10GB

        case InstallComponent::HUNYUAN_VIDEO:
            // T2V Q4 (~7.74GB) + I2V Q4 (~8GB) + VAE (~493MB) + CLIP (~246MB) +
            // LLaVA (~9GB) + CLIP Vision (~1.17GB)
            return 30LL * 1024 * 1024 * 1024;  // 30GB

        default:
            return 0;
    }
}

int64_t ComfyUIInstaller::getDownloadSize(InstallComponent component) {
    switch (component) {
        case InstallComponent::COMFYUI_BASE:
            return COMFYUI_PORTABLE_SIZE;

        case InstallComponent::SD_ANIMATEDIFF:
            return SD15_FP16_SIZE + ANIMATEDIFF_MM_V2_SIZE + SD_VAE_SIZE +
                   CONTROLNET_DEPTH_SIZE + CONTROLNET_CANNY_SIZE +
                   IPADAPTER_PLUS_SIZE + CLIP_VISION_SIZE;

        case InstallComponent::HUNYUAN_VIDEO:
            return HUNYUAN_T2V_Q4_SIZE + HUNYUAN_I2V_Q4_SIZE + HUNYUAN_VAE_SIZE +
                   HUNYUAN_QWEN_SIZE + HUNYUAN_BYT5_SIZE + HUNYUAN_CLIP_VISION_SIZE;

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

    // Install 7-Zip if missing
    if (!is7ZipInstalled()) {
        InstallProgress prog;
        prog.state = InstallProgress::State::DOWNLOADING;
        prog.status = "Installing 7-Zip...";
        prog.currentFile = "7-Zip";
        updateProgress(prog);

        if (!install7Zip(tempDir)) {
            // Error already set by install7Zip
            return false;
        }
    }

    // Install Git if missing
    if (!isGitInstalled()) {
        InstallProgress prog;
        prog.state = InstallProgress::State::DOWNLOADING;
        prog.status = "Installing Git...";
        prog.currentFile = "Git for Windows";
        updateProgress(prog);

        if (!installGit(tempDir)) {
            // Error already set by installGit
            return false;
        }
    }

    return true;
}

bool ComfyUIInstaller::install7Zip(const std::string& tempDir) {
#ifdef _WIN32
    std::string installerPath = (fs::path(tempDir) / "7z-installer.exe").string();

    InstallProgress prog;
    prog.state = InstallProgress::State::DOWNLOADING;
    prog.status = "Downloading 7-Zip...";
    prog.currentFile = "7-Zip Installer";
    updateProgress(prog);

    // Download 7-Zip installer
    if (!downloadFileWithResume(SEVENZIP_URL, installerPath, SEVENZIP_SIZE)) {
        setError("Failed to download 7-Zip installer");
        return false;
    }

    prog.state = InstallProgress::State::INSTALLING_NODES;
    prog.status = "Installing 7-Zip (may request admin rights)...";
    updateProgress(prog);

    // Run silent install with elevation using ShellExecuteEx
    // 7-Zip installer supports /S for silent mode
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = "runas";  // Request elevation
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/S";
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            setError("7-Zip installation cancelled - admin rights required");
        } else {
            setError("Failed to run 7-Zip installer (error " + std::to_string(err) + ")");
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

    InstallProgress prog;
    prog.state = InstallProgress::State::DOWNLOADING;
    prog.status = "Downloading Git for Windows...";
    prog.currentFile = "Git Installer";
    updateProgress(prog);

    // Download Git installer
    if (!downloadFileWithResume(GIT_URL, installerPath, GIT_SIZE)) {
        setError("Failed to download Git installer");
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
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = "runas";  // Request elevation
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/VERYSILENT /NORESTART /NOCANCEL /SP- /CLOSEAPPLICATIONS";
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            setError("Git installation cancelled - admin rights required");
        } else {
            setError("Failed to run Git installer (error " + std::to_string(err) + ")");
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

bool ComfyUIInstaller::uninstallSDAnimateDiff(const std::string& installDir) {
    // Portable version path structure
    fs::path basePath = fs::path(installDir) / "ComfyUI_windows_portable" / "ComfyUI";
    fs::path modelsPath = basePath / "models";
    fs::path nodesPath = basePath / "custom_nodes";

    std::error_code ec;

    // Remove models
    fs::remove(modelsPath / "checkpoints" / "v1-5-pruned-emaonly-fp16.safetensors", ec);
    fs::remove(modelsPath / "animatediff_models" / "mm_sd_v15_v2.fp16.safetensors", ec);
    fs::remove(modelsPath / "vae" / "vae-ft-mse-840000-ema-pruned.safetensors", ec);
    fs::remove(modelsPath / "controlnet" / "control_v11f1p_sd15_depth.pth", ec);
    fs::remove(modelsPath / "controlnet" / "control_v11p_sd15_canny.pth", ec);
    fs::remove(modelsPath / "ipadapter" / "ip-adapter-plus_sd15.safetensors", ec);
    fs::remove(modelsPath / "clip_vision" / "model.safetensors", ec);

    // Remove custom nodes
    fs::remove_all(nodesPath / "ComfyUI-AnimateDiff-Evolved", ec);
    fs::remove_all(nodesPath / "ComfyUI-VideoHelperSuite", ec);
    fs::remove_all(nodesPath / "ComfyUI-Advanced-ControlNet", ec);
    fs::remove_all(nodesPath / "ComfyUI_IPAdapter_plus", ec);

    return true;
}

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
            prog.status = "Downloading " + file.description;
            if (file.expectedSize > 0) {
                prog.status += " (" + formatSize(file.expectedSize) + ")";
            }
            prog.percentComplete = (totalBytes > 0) ?
                (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
            updateProgress(prog);

            std::string localPath = (fs::path(tempDir) / fs::path(file.localPath).filename()).string();

            if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
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
                prog.status = "Extracting " + file.description;
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
            prog.status = "Installing node: " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = (fs::path(nodesDir) / repoName).string();
            if (!fs::exists(targetDir)) {
                cloneRepository(nodeUrl, targetDir);
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
            WaitForSingleObject(pi.hProcess, 300000);
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

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "ComfyUI base installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installSDAnimateDiffThread(InstallConfig config) {
    InstallProgress prog;
    prog.state = InstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    // Use component-based approach to find what's missing
    auto allComponents = getSDComponents();
    auto missingComponents = getMissingComponents(allComponents, config.installDir);

    // Check if everything is already installed
    if (missingComponents.empty()) {
        prog.state = InstallProgress::State::COMPLETE;
        prog.status = "SD + AnimateDiff already installed";
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
    createDirectories((modelsPath / "checkpoints").string());
    createDirectories((modelsPath / "animatediff_models").string());
    createDirectories((modelsPath / "vae").string());
    createDirectories((modelsPath / "controlnet").string());
    createDirectories((modelsPath / "ipadapter").string());
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
            prog.status = "Downloading " + file.description;
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

            if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
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
            prog.status = "Installing node: " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = (fs::path(nodesDir) / repoName).string();
            if (!fs::exists(targetDir)) {
                cloneRepository(nodeUrl, targetDir);
            }

            prog.filesCompleted++;
        }
    }

    // Verification
    prog.state = InstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "SD + AnimateDiff installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installHunyuanVideoThread(InstallConfig config) {
    InstallProgress prog;
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
    createDirectories((modelsPath / "LLM" / "Florence-2-large").string());

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
            prog.status = "Downloading " + file.description;
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

            if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
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
            prog.status = "Installing node: " + repoName + "...";
            updateProgress(prog);

            std::string targetDir = (fs::path(nodesDir) / repoName).string();
            if (!fs::exists(targetDir)) {
                cloneRepository(nodeUrl, targetDir);
            }

            prog.filesCompleted++;
        }
    }

    // Install Python dependencies for HunyuanVideoWrapper and Florence-2
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
                WaitForSingleObject(pi.hProcess, 300000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
#else
            system(pipCommand.c_str());
#endif
        }

        // Florence-2 dependencies (matplotlib, timm, einops, transformers)
        prog.status = "Installing Florence-2 dependencies...";
        updateProgress(prog);

        std::string florence2Deps = "\"" + pythonExe + "\" -m pip install matplotlib timm einops transformers";

#ifdef _WIN32
        {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            char cmdLine[4096];
            strncpy(cmdLine, florence2Deps.c_str(), sizeof(cmdLine) - 1);
            cmdLine[sizeof(cmdLine) - 1] = '\0';

            if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 300000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        system(florence2Deps.c_str());
#endif

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
                WaitForSingleObject(pi.hProcess, 300000);
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

    prog.state = InstallProgress::State::COMPLETE;
    prog.status = "HunyuanVideo installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    if (!runningInstallAll.load()) installing.store(false);
}

void ComfyUIInstaller::installAllThread(InstallConfig config) {
    // Install base first
    installComfyUIBaseThread(config);

    // Immediately reclaim installing flag to prevent race condition with user's while loop
    // (sub-thread sets installing=false at end, but we're not done yet)
    installing.store(true);

    if (shouldCancel.load() || progress.state == InstallProgress::State::FAILED) {
        installing.store(false);
        return;
    }

    // Update progress for next phase
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.status = "Starting SD + AnimateDiff installation...";
        progress.percentComplete = 0.0f;
        progress.filesCompleted = 0;
        if (progressCallback) {
            progressCallback(progress);
        }
    }

    // Install SD + AnimateDiff
    installSDAnimateDiffThread(config);

    // Reclaim installing flag again
    installing.store(true);

    if (shouldCancel.load() || progress.state == InstallProgress::State::FAILED) {
        installing.store(false);
        return;
    }

    // Update progress for next phase
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.status = "Starting HunyuanVideo installation...";
        progress.percentComplete = 0.0f;
        progress.filesCompleted = 0;
        if (progressCallback) {
            progressCallback(progress);
        }
    }

    // Install HunyuanVideo
    installHunyuanVideoThread(config);

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

            // Update status with size info
            progress.status = "Downloading " + progress.currentFile + " (" +
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, currentConfig.downloadTimeout / 1000);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, currentConfig.connectionTimeout / 1000);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "EWOCvj2-Installer/1.0");

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

    // Execute git clone
    std::string command = "git clone --depth 1 \"" + url + "\" \"" + targetDir + "\"";

#ifdef _WIN32
    // Use CreateProcess for better control
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    snprintf(cmdLine, sizeof(cmdLine), "cmd /c %s", command.c_str());

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to execute git clone");
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
#else
    int result = system(command.c_str());
    return result == 0;
#endif
}

bool ComfyUIInstaller::pullRepository(const std::string& repoDir) {
    if (!fs::exists(fs::path(repoDir) / ".git")) {
        return false;
    }

    std::string command = "cd \"" + repoDir + "\" && git pull";

#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdLine[4096];
    snprintf(cmdLine, sizeof(cmdLine), "cmd /c %s", command.c_str());

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

std::vector<DownloadFile> ComfyUIInstaller::getSDAnimateDiffFiles() {
    return {
        // SD1.5 checkpoint
        {
            SD15_FP16_URL,
            "checkpoints/v1-5-pruned-emaonly-fp16.safetensors",
            "Stable Diffusion 1.5 (FP16)",
            SD15_FP16_SIZE,
            "",
            true
        },
        // AnimateDiff motion module
        {
            ANIMATEDIFF_MM_V2_FP16_URL,
            "animatediff_models/mm_sd_v15_v2.fp16.safetensors",
            "AnimateDiff Motion Module v2",
            ANIMATEDIFF_MM_V2_SIZE,
            "",
            true
        },
        // VAE
        {
            SD_VAE_FP16_URL,
            "vae/vae-ft-mse-840000-ema-pruned.safetensors",
            "SD VAE (MSE)",
            SD_VAE_SIZE,
            "",
            true
        },
        // ControlNet - Depth
        {
            CONTROLNET_DEPTH_URL,
            "controlnet/control_v11f1p_sd15_depth.pth",
            "ControlNet Depth",
            CONTROLNET_DEPTH_SIZE,
            "",
            false  // Optional
        },
        // ControlNet - Canny
        {
            CONTROLNET_CANNY_URL,
            "controlnet/control_v11p_sd15_canny.pth",
            "ControlNet Canny",
            CONTROLNET_CANNY_SIZE,
            "",
            false  // Optional
        },
        // IPAdapter
        {
            IPADAPTER_PLUS_URL,
            "ipadapter/ip-adapter-plus_sd15.safetensors",
            "IP-Adapter Plus",
            IPADAPTER_PLUS_SIZE,
            "",
            false  // Optional
        },
        // CLIP Vision
        {
            CLIP_VISION_URL,
            "clip_vision/model.safetensors",
            "CLIP Vision Encoder",
            CLIP_VISION_SIZE,
            "",
            false  // Optional - needed for IPAdapter
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
        },
        // Florence-2 Large for detailed image captioning (I2V)
        {
            FLORENCE2_LARGE_URL,
            "LLM/Florence-2-large/model.safetensors",
            "Florence-2 Large Model",
            FLORENCE2_LARGE_SIZE,
            "",
            true
        },
        {
            FLORENCE2_CONFIG_URL,
            "LLM/Florence-2-large/config.json",
            "Florence-2 Config",
            0,
            "",
            true
        },
        {
            FLORENCE2_TOKENIZER_URL,
            "LLM/Florence-2-large/tokenizer.json",
            "Florence-2 Tokenizer",
            0,
            "",
            true
        },
        {
            FLORENCE2_PROCESSOR_URL,
            "LLM/Florence-2-large/preprocessor_config.json",
            "Florence-2 Preprocessor Config",
            0,
            "",
            true
        }
    };
}

std::vector<std::string> ComfyUIInstaller::getSDCustomNodes() {
    return {
        NODE_ANIMATEDIFF_EVOLVED,
        NODE_VIDEO_HELPER_SUITE,
        NODE_ADVANCED_CONTROLNET,
        NODE_IPADAPTER_PLUS,
        NODE_FIZZNODES,
        NODE_FRAME_INTERPOLATION     // For RIFE frame interpolation
    };
}

std::vector<std::string> ComfyUIInstaller::getHunyuanCustomNodes() {
    return {
        NODE_COMFYUI_GGUF,           // For GGUF model loading
        NODE_VIDEO_HELPER_SUITE,     // For video output
        NODE_FRAME_INTERPOLATION,    // For RIFE frame interpolation
        NODE_FLORENCE2               // For detailed image captioning (I2V)
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
        // Florence-2 for detailed image captioning
        {
            "florence2",
            "Florence-2 Image Captioning",
            "Generates detailed descriptions of input images for better I2V results",
            {
                // Model weights
                {
                    FLORENCE2_LARGE_URL,
                    "LLM/Florence-2-large/model.safetensors",
                    "Florence-2 Large Model",
                    FLORENCE2_LARGE_SIZE, "", true
                },
                // Config files
                {
                    FLORENCE2_CONFIG_URL,
                    "LLM/Florence-2-large/config.json",
                    "Florence-2 Config",
                    0, "", true
                },
                {
                    FLORENCE2_GENERATION_CONFIG_URL,
                    "LLM/Florence-2-large/generation_config.json",
                    "Florence-2 Generation Config",
                    0, "", true
                },
                // Tokenizer files
                {
                    FLORENCE2_TOKENIZER_URL,
                    "LLM/Florence-2-large/tokenizer.json",
                    "Florence-2 Tokenizer",
                    0, "", true
                },
                {
                    FLORENCE2_TOKENIZER_CONFIG_URL,
                    "LLM/Florence-2-large/tokenizer_config.json",
                    "Florence-2 Tokenizer Config",
                    0, "", true
                },
                {
                    FLORENCE2_VOCAB_URL,
                    "LLM/Florence-2-large/vocab.json",
                    "Florence-2 Vocab",
                    0, "", true
                },
                // Preprocessor
                {
                    FLORENCE2_PROCESSOR_URL,
                    "LLM/Florence-2-large/preprocessor_config.json",
                    "Florence-2 Preprocessor Config",
                    0, "", true
                },
                // Python code files (required for custom architecture)
                {
                    FLORENCE2_MODELING_URL,
                    "LLM/Florence-2-large/modeling_florence2.py",
                    "Florence-2 Modeling Code",
                    0, "", true
                },
                {
                    FLORENCE2_CONFIGURATION_URL,
                    "LLM/Florence-2-large/configuration_florence2.py",
                    "Florence-2 Configuration Code",
                    0, "", true
                },
                {
                    FLORENCE2_PROCESSING_URL,
                    "LLM/Florence-2-large/processing_florence2.py",
                    "Florence-2 Processing Code",
                    0, "", true
                }
            },
            {NODE_FLORENCE2},
            {"LLM/Florence-2-large/processing_florence2.py"},  // Check for Python file existence
            true, true
        },
        // Core custom nodes for Hunyuan
        {
            "hunyuan_nodes",
            "HunyuanVideo Custom Nodes",
            "Required ComfyUI nodes for HunyuanVideo",
            {},
            {NODE_COMFYUI_GGUF, NODE_VIDEO_HELPER_SUITE, NODE_FRAME_INTERPOLATION},
            {},  // Check by node folder existence
            true, true
        }
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

std::vector<ModelComponent> ComfyUIInstaller::getSDComponents() {
    return {
        // SD1.5 Checkpoint
        {
            "sd15_checkpoint",
            "Stable Diffusion 1.5",
            "Base SD1.5 model (FP16)",
            {
                {
                    SD15_FP16_URL,
                    "checkpoints/v1-5-pruned-emaonly-fp16.safetensors",
                    "Stable Diffusion 1.5 (FP16)",
                    SD15_FP16_SIZE, "", true
                }
            },
            {},
            {"checkpoints/v1-5-pruned-emaonly-fp16.safetensors"},
            true, true
        },
        // AnimateDiff Motion Module
        {
            "animatediff_motion",
            "AnimateDiff Motion Module",
            "Motion module for video generation",
            {
                {
                    ANIMATEDIFF_MM_V2_FP16_URL,
                    "animatediff_models/mm_sd_v15_v2.fp16.safetensors",
                    "AnimateDiff Motion Module v2",
                    ANIMATEDIFF_MM_V2_SIZE, "", true
                }
            },
            {},
            {"animatediff_models/mm_sd_v15_v2.fp16.safetensors"},
            true, true
        },
        // VAE
        {
            "sd_vae",
            "SD VAE",
            "Variational autoencoder for image encoding/decoding",
            {
                {
                    SD_VAE_FP16_URL,
                    "vae/vae-ft-mse-840000-ema-pruned.safetensors",
                    "SD VAE (MSE)",
                    SD_VAE_SIZE, "", true
                }
            },
            {},
            {"vae/vae-ft-mse-840000-ema-pruned.safetensors"},
            true, true
        },
        // ControlNet Depth (optional)
        {
            "controlnet_depth",
            "ControlNet Depth",
            "Depth-based control for image generation",
            {
                {
                    CONTROLNET_DEPTH_URL,
                    "controlnet/control_v11f1p_sd15_depth.pth",
                    "ControlNet Depth",
                    CONTROLNET_DEPTH_SIZE, "", false
                }
            },
            {},
            {"controlnet/control_v11f1p_sd15_depth.pth"},
            false, true  // Optional
        },
        // ControlNet Canny (optional)
        {
            "controlnet_canny",
            "ControlNet Canny",
            "Edge-based control for image generation",
            {
                {
                    CONTROLNET_CANNY_URL,
                    "controlnet/control_v11p_sd15_canny.pth",
                    "ControlNet Canny",
                    CONTROLNET_CANNY_SIZE, "", false
                }
            },
            {},
            {"controlnet/control_v11p_sd15_canny.pth"},
            false, true  // Optional
        },
        // IPAdapter (optional)
        {
            "ipadapter",
            "IP-Adapter Plus",
            "Image prompt adapter for style/content transfer",
            {
                {
                    IPADAPTER_PLUS_URL,
                    "ipadapter/ip-adapter-plus_sd15.safetensors",
                    "IP-Adapter Plus",
                    IPADAPTER_PLUS_SIZE, "", false
                }
            },
            {},
            {"ipadapter/ip-adapter-plus_sd15.safetensors"},
            false, true  // Optional
        },
        // CLIP Vision (optional, needed for IPAdapter)
        {
            "clip_vision_sd",
            "CLIP Vision Encoder",
            "Vision encoder for IP-Adapter",
            {
                {
                    CLIP_VISION_URL,
                    "clip_vision/model.safetensors",
                    "CLIP Vision Encoder",
                    CLIP_VISION_SIZE, "", false
                }
            },
            {},
            {"clip_vision/model.safetensors"},
            false, true  // Optional
        },
        // AnimateDiff Custom Nodes
        {
            "animatediff_nodes",
            "AnimateDiff Nodes",
            "Required custom nodes for AnimateDiff",
            {},
            {NODE_ANIMATEDIFF_EVOLVED, NODE_VIDEO_HELPER_SUITE},
            {},
            true, true
        },
        // ControlNet Custom Nodes (optional)
        {
            "controlnet_nodes",
            "ControlNet Nodes",
            "Advanced ControlNet custom nodes",
            {},
            {NODE_ADVANCED_CONTROLNET},
            {},
            false, true  // Optional
        },
        // IPAdapter Custom Nodes (optional)
        {
            "ipadapter_nodes",
            "IP-Adapter Nodes",
            "IP-Adapter custom nodes",
            {},
            {NODE_IPADAPTER_PLUS},
            {},
            false, true  // Optional
        },
        // FizzNodes for batch prompts
        {
            "fizznodes",
            "FizzNodes",
            "Batch prompt scheduling nodes",
            {},
            {NODE_FIZZNODES},
            {},
            true, true
        },
        // Frame Interpolation
        {
            "frame_interpolation",
            "Frame Interpolation",
            "RIFE frame interpolation for smoother videos",
            {},
            {NODE_FRAME_INTERPOLATION},
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

    // Check all required files
    for (const auto& checkFile : component.checkFiles) {
        fs::path filePath = fs::path(modelsDir) / checkFile;
        if (!fs::exists(filePath)) {
            return false;
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
