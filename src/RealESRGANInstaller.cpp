/**
 * RealESRGANInstaller.cpp
 *
 * Implementation of Real-ESRGAN model download and installation
 *
 * License: GPL3
 */

#include "RealESRGANInstaller.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#pragma comment(lib, "winhttp.lib")
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

RealESRGANInstaller::RealESRGANInstaller() {
}

RealESRGANInstaller::~RealESRGANInstaller() {
    cancelInstallation();
    if (installThread && installThread->joinable()) {
        installThread->join();
    }
}

// ============================================================================
// Installation Methods
// ============================================================================

bool RealESRGANInstaller::installX4Plus(const RealESRGANInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.modelsDir.empty()) {
        setError("Models directory not specified");
        return false;
    }

    // Check disk space
    int64_t required = getRequiredDiskSpace(RealESRGANComponent::REALESRGAN_X4PLUS);
    int64_t available = getFreeDiskSpace(config.modelsDir);
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
    installThread = std::make_unique<std::thread>(&RealESRGANInstaller::installX4PlusThread, this, config);

    return true;
}

bool RealESRGANInstaller::installX4PlusAnime(const RealESRGANInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.modelsDir.empty()) {
        setError("Models directory not specified");
        return false;
    }

    int64_t required = getRequiredDiskSpace(RealESRGANComponent::REALESRGAN_X4PLUS_ANIME);
    int64_t available = getFreeDiskSpace(config.modelsDir);
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
    installThread = std::make_unique<std::thread>(&RealESRGANInstaller::installX4PlusAnimeThread, this, config);

    return true;
}

bool RealESRGANInstaller::installAnimeVideoV3(const RealESRGANInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.modelsDir.empty()) {
        setError("Models directory not specified");
        return false;
    }

    int64_t required = getRequiredDiskSpace(RealESRGANComponent::ANIMEVIDEOV3_ALL);
    int64_t available = getFreeDiskSpace(config.modelsDir);
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
    installThread = std::make_unique<std::thread>(&RealESRGANInstaller::installAnimeVideoV3Thread, this, config);

    return true;
}

bool RealESRGANInstaller::installAllModels(const RealESRGANInstallConfig& config) {
    if (installing.load()) {
        setError("Installation already in progress");
        return false;
    }

    if (config.modelsDir.empty()) {
        setError("Models directory not specified");
        return false;
    }

    int64_t required = getRequiredDiskSpace(RealESRGANComponent::ALL_MODELS);
    int64_t available = getFreeDiskSpace(config.modelsDir);
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
    installThread = std::make_unique<std::thread>(&RealESRGANInstaller::installAllModelsThread, this, config);

    return true;
}

void RealESRGANInstaller::cancelInstallation() {
    shouldCancel.store(true);
}

RealESRGANInstallProgress RealESRGANInstaller::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void RealESRGANInstaller::setProgressCallback(std::function<void(const RealESRGANInstallProgress&)> callback) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progressCallback = std::move(callback);
}

// ============================================================================
// Verification Methods
// ============================================================================

bool RealESRGANInstaller::isX4PlusInstalled(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    return fs::exists(dir / "realesrgan-x4plus.bin") &&
           fs::exists(dir / "realesrgan-x4plus.param");
}

bool RealESRGANInstaller::isX4PlusAnimeInstalled(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    return fs::exists(dir / "realesrgan-x4plus-anime.bin") &&
           fs::exists(dir / "realesrgan-x4plus-anime.param");
}

bool RealESRGANInstaller::isAnimeVideoV3Installed(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    return fs::exists(dir / "realesr-animevideov3-x2.bin") &&
           fs::exists(dir / "realesr-animevideov3-x2.param") &&
           fs::exists(dir / "realesr-animevideov3-x3.bin") &&
           fs::exists(dir / "realesr-animevideov3-x3.param") &&
           fs::exists(dir / "realesr-animevideov3-x4.bin") &&
           fs::exists(dir / "realesr-animevideov3-x4.param");
}

bool RealESRGANInstaller::isAllModelsInstalled(const std::string& modelsDir) {
    return isX4PlusInstalled(modelsDir) &&
           isX4PlusAnimeInstalled(modelsDir) &&
           isAnimeVideoV3Installed(modelsDir);
}

int64_t RealESRGANInstaller::getRequiredDiskSpace(RealESRGANComponent component) {
    // Add 10% buffer for safety
    switch (component) {
        case RealESRGANComponent::REALESRGAN_X4PLUS:
            return static_cast<int64_t>((X4PLUS_BIN_SIZE + X4PLUS_PARAM_SIZE) * 1.1);

        case RealESRGANComponent::REALESRGAN_X4PLUS_ANIME:
            return static_cast<int64_t>((X4PLUS_ANIME_BIN_SIZE + X4PLUS_ANIME_PARAM_SIZE) * 1.1);

        case RealESRGANComponent::ANIMEVIDEOV3_ALL:
            return static_cast<int64_t>((ANIMEVIDEO_X2_BIN_SIZE + ANIMEVIDEO_X2_PARAM_SIZE +
                    ANIMEVIDEO_X3_BIN_SIZE + ANIMEVIDEO_X3_PARAM_SIZE +
                    ANIMEVIDEO_X4_BIN_SIZE + ANIMEVIDEO_X4_PARAM_SIZE) * 1.1);

        case RealESRGANComponent::ALL_MODELS:
            return getRequiredDiskSpace(RealESRGANComponent::REALESRGAN_X4PLUS) +
                   getRequiredDiskSpace(RealESRGANComponent::REALESRGAN_X4PLUS_ANIME) +
                   getRequiredDiskSpace(RealESRGANComponent::ANIMEVIDEOV3_ALL);

        default:
            return 0;
    }
}

int64_t RealESRGANInstaller::getDownloadSize(RealESRGANComponent component) {
    switch (component) {
        case RealESRGANComponent::REALESRGAN_X4PLUS:
            return X4PLUS_BIN_SIZE + X4PLUS_PARAM_SIZE;

        case RealESRGANComponent::REALESRGAN_X4PLUS_ANIME:
            return X4PLUS_ANIME_BIN_SIZE + X4PLUS_ANIME_PARAM_SIZE;

        case RealESRGANComponent::ANIMEVIDEOV3_ALL:
            return ANIMEVIDEO_X2_BIN_SIZE + ANIMEVIDEO_X2_PARAM_SIZE +
                   ANIMEVIDEO_X3_BIN_SIZE + ANIMEVIDEO_X3_PARAM_SIZE +
                   ANIMEVIDEO_X4_BIN_SIZE + ANIMEVIDEO_X4_PARAM_SIZE;

        case RealESRGANComponent::ALL_MODELS:
            return getDownloadSize(RealESRGANComponent::REALESRGAN_X4PLUS) +
                   getDownloadSize(RealESRGANComponent::REALESRGAN_X4PLUS_ANIME) +
                   getDownloadSize(RealESRGANComponent::ANIMEVIDEOV3_ALL);

        default:
            return 0;
    }
}

std::string RealESRGANInstaller::formatSize(int64_t bytes) {
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

std::string RealESRGANInstaller::getDefaultModelsDir() {
#ifdef _WIN32
    // Get ProgramData path
    char programData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programData))) {
        return std::string(programData) + "\\EWOCvj2\\models\\upscale";
    }
    return "C:\\ProgramData\\EWOCvj2\\models\\upscale";
#else
    // Linux/Mac: use ~/.local/share
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/EWOCvj2/models/upscale";
    }
    return "/usr/local/share/EWOCvj2/models/upscale";
#endif
}

// ============================================================================
// Uninstallation Methods
// ============================================================================

bool RealESRGANInstaller::uninstallX4Plus(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    std::error_code ec;
    fs::remove(dir / "realesrgan-x4plus.bin", ec);
    fs::remove(dir / "realesrgan-x4plus.param", ec);
    return true;
}

bool RealESRGANInstaller::uninstallX4PlusAnime(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    std::error_code ec;
    fs::remove(dir / "realesrgan-x4plus-anime.bin", ec);
    fs::remove(dir / "realesrgan-x4plus-anime.param", ec);
    return true;
}

bool RealESRGANInstaller::uninstallAnimeVideoV3(const std::string& modelsDir) {
    fs::path dir(modelsDir);
    std::error_code ec;
    fs::remove(dir / "realesr-animevideov3-x2.bin", ec);
    fs::remove(dir / "realesr-animevideov3-x2.param", ec);
    fs::remove(dir / "realesr-animevideov3-x3.bin", ec);
    fs::remove(dir / "realesr-animevideov3-x3.param", ec);
    fs::remove(dir / "realesr-animevideov3-x4.bin", ec);
    fs::remove(dir / "realesr-animevideov3-x4.param", ec);
    return true;
}

bool RealESRGANInstaller::uninstallAll(const std::string& modelsDir) {
    uninstallX4Plus(modelsDir);
    uninstallX4PlusAnime(modelsDir);
    uninstallAnimeVideoV3(modelsDir);
    return true;
}

// ============================================================================
// Error Handling
// ============================================================================

std::string RealESRGANInstaller::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void RealESRGANInstaller::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

// ============================================================================
// Installation Threads
// ============================================================================

void RealESRGANInstaller::installX4PlusThread(RealESRGANInstallConfig config) {
    RealESRGANInstallProgress prog;
    prog.state = RealESRGANInstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    // Check if already installed
    if (isX4PlusInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::COMPLETE;
        prog.status = "RealESRGAN-x4plus already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    // Create directories
    if (!createDirectories(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to create models directory";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto files = getX4PlusFiles();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    // Download files
    for (const auto& file : files) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
        prog.currentFile = file.description;
        prog.status = "Downloading " + file.description;
        updateProgress(prog);

        std::string localPath = (fs::path(config.modelsDir) / file.localPath).string();

        if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
            if (file.required) {
                prog.state = RealESRGANInstallProgress::State::FAILED;
                prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                prog.status = "FAILED: " + prog.errorMessage;
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.filesCompleted++;
        prog.percentComplete = (static_cast<float>(prog.filesCompleted) / prog.filesTotal) * 100.0f;
    }

    // Verification
    prog.state = RealESRGANInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    if (!isX4PlusInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Installation verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = RealESRGANInstallProgress::State::COMPLETE;
    prog.status = "RealESRGAN-x4plus installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void RealESRGANInstaller::installX4PlusAnimeThread(RealESRGANInstallConfig config) {
    RealESRGANInstallProgress prog;
    prog.state = RealESRGANInstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    if (isX4PlusAnimeInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::COMPLETE;
        prog.status = "RealESRGAN-x4plus-anime already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (!createDirectories(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to create models directory";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto files = getX4PlusAnimeFiles();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    for (const auto& file : files) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
        prog.currentFile = file.description;
        prog.status = "Downloading " + file.description;
        updateProgress(prog);

        std::string localPath = (fs::path(config.modelsDir) / file.localPath).string();

        if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
            if (file.required) {
                prog.state = RealESRGANInstallProgress::State::FAILED;
                prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                prog.status = "FAILED: " + prog.errorMessage;
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.filesCompleted++;
        prog.percentComplete = (static_cast<float>(prog.filesCompleted) / prog.filesTotal) * 100.0f;
    }

    prog.state = RealESRGANInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    if (!isX4PlusAnimeInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Installation verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = RealESRGANInstallProgress::State::COMPLETE;
    prog.status = "RealESRGAN-x4plus-anime installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void RealESRGANInstaller::installAnimeVideoV3Thread(RealESRGANInstallConfig config) {
    RealESRGANInstallProgress prog;
    prog.state = RealESRGANInstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    if (isAnimeVideoV3Installed(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::COMPLETE;
        prog.status = "AnimevideV3 models already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (!createDirectories(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to create models directory";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto files = getAnimeVideoV3Files();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    for (const auto& file : files) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
        prog.currentFile = file.description;
        prog.status = "Downloading " + file.description;
        updateProgress(prog);

        std::string localPath = (fs::path(config.modelsDir) / file.localPath).string();

        if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
            if (file.required) {
                prog.state = RealESRGANInstallProgress::State::FAILED;
                prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                prog.status = "FAILED: " + prog.errorMessage;
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        prog.filesCompleted++;
        prog.percentComplete = (static_cast<float>(prog.filesCompleted) / prog.filesTotal) * 100.0f;
    }

    prog.state = RealESRGANInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    if (!isAnimeVideoV3Installed(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Installation verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = RealESRGANInstallProgress::State::COMPLETE;
    prog.status = "AnimeVideoV3 models installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

void RealESRGANInstaller::installAllModelsThread(RealESRGANInstallConfig config) {
    RealESRGANInstallProgress prog;
    prog.state = RealESRGANInstallProgress::State::CHECKING;
    prog.status = "Checking existing installation...";
    updateProgress(prog);

    if (isAllModelsInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::COMPLETE;
        prog.status = "All RealESRGAN models already installed";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    if (!createDirectories(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Failed to create models directory";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto files = getAllModelFiles();
    prog.filesTotal = static_cast<int>(files.size());
    prog.filesCompleted = 0;

    int64_t totalBytes = 0;
    int64_t downloadedBytes = 0;
    for (const auto& f : files) totalBytes += f.expectedSize;

    auto startTime = std::chrono::steady_clock::now();

    for (const auto& file : files) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
            updateProgress(prog);
            installing.store(false);
            return;
        }

        prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
        prog.currentFile = file.description;
        prog.status = "Downloading " + file.description + " (" + formatSize(file.expectedSize) + ")";
        prog.percentComplete = (totalBytes > 0) ?
            (static_cast<float>(downloadedBytes) / totalBytes * 100.0f) : 0.0f;
        updateProgress(prog);

        std::string localPath = (fs::path(config.modelsDir) / file.localPath).string();

        if (!downloadFileWithResume(file.url, localPath, file.expectedSize)) {
            if (file.required) {
                prog.state = RealESRGANInstallProgress::State::FAILED;
                prog.errorMessage = "Failed to download " + file.description + ": " + getLastError();
                prog.status = "FAILED: " + prog.errorMessage;
                updateProgress(prog);
                installing.store(false);
                return;
            }
        }

        downloadedBytes += file.expectedSize;
        prog.filesCompleted++;
    }

    prog.state = RealESRGANInstallProgress::State::VERIFYING;
    prog.status = "Verifying installation...";
    updateProgress(prog);

    if (!isAllModelsInstalled(config.modelsDir)) {
        prog.state = RealESRGANInstallProgress::State::FAILED;
        prog.errorMessage = "Installation verification failed";
        prog.status = "FAILED: " + prog.errorMessage;
        updateProgress(prog);
        installing.store(false);
        return;
    }

    auto endTime = std::chrono::steady_clock::now();
    prog.elapsedTime = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = RealESRGANInstallProgress::State::COMPLETE;
    prog.status = "All RealESRGAN models installation complete";
    prog.percentComplete = 100.0f;
    updateProgress(prog);

    installing.store(false);
}

// ============================================================================
// Download Helpers
// ============================================================================

bool RealESRGANInstaller::downloadFile(const RealESRGANModelFile& file) {
    return downloadFileWithResume(file.url, file.localPath, file.expectedSize);
}

#ifdef _WIN32
// Windows implementation using WinHTTP
bool RealESRGANInstaller::downloadFileWithResume(const std::string& url, const std::string& localPath,
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
        // If file is already complete, skip download
        if (expectedSize > 0 && existingSize >= expectedSize) {
            return true;
        }
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-RealESRGAN-Installer/1.0",
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
        setError("Failed to connect to server (error " + std::to_string(err) + ")");
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
        DWORD err = GetLastError();
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to open HTTP request (error " + std::to_string(err) + ")");
        return false;
    }

    // Enable automatic redirects
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    DWORD maxRedirects = 10;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS, &maxRedirects, sizeof(maxRedirects));

    // For HTTPS, enable TLS
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        DWORD secFlags = 0x00000800 | 0x00002000;  // TLS 1.2 + 1.3
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secFlags, sizeof(secFlags));

        DWORD secOptions = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                           SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                           SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secOptions, sizeof(secOptions));
    }

    // Add Range header for resume
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
        setError("Failed to send HTTP request (error " + std::to_string(err) + ")");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        setError("Failed to receive HTTP response (error " + std::to_string(err) + ")");
        return false;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        NULL, &statusCode, &statusCodeSize, NULL);

    // 416 = Range Not Satisfiable - file is already complete
    if (statusCode == 416) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        std::lock_guard<std::mutex> lock(progressMutex);
        progress.status = progress.currentFile + " already downloaded";
        progress.percentComplete = 100.0f;
        if (progressCallback) {
            progressCallback(progress);
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

    // If server returned 200 instead of 206, start fresh
    if (statusCode == 200 && existingSize > 0) {
        existingSize = 0;
    }

    // Get content length
    wchar_t contentLength[32] = {0};
    DWORD contentLengthSize = sizeof(contentLength);
    int64_t totalSize = expectedSize;
    int64_t serverReportedSize = 0;
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

    // Open file for writing
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

    // Verify download
    int64_t actualSize = getFileSize(localPath);
    if (serverReportedSize > 0) {
        if (actualSize != serverReportedSize) {
            setError("Downloaded file size mismatch. Expected: " + std::to_string(serverReportedSize) +
                     ", Got: " + std::to_string(actualSize));
            return false;
        }
    } else if (actualSize <= 0) {
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

bool RealESRGANInstaller::downloadFileWithResume(const std::string& url, const std::string& localPath,
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "EWOCvj2-RealESRGAN-Installer/1.0");

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

bool RealESRGANInstaller::verifyFile(const std::string& path, int64_t expectedSize) {
    if (!fs::exists(path)) {
        return false;
    }

    if (expectedSize > 0) {
        int64_t actualSize = getFileSize(path);
        if (actualSize != expectedSize) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Utility Methods
// ============================================================================

bool RealESRGANInstaller::createDirectories(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

bool RealESRGANInstaller::deleteFile(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
    return !ec;
}

bool RealESRGANInstaller::fileExists(const std::string& path) {
    return fs::exists(path);
}

int64_t RealESRGANInstaller::getFileSize(const std::string& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? -1 : static_cast<int64_t>(size);
}

int64_t RealESRGANInstaller::getFreeDiskSpace(const std::string& path) {
    std::error_code ec;

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

void RealESRGANInstaller::updateProgress(const RealESRGANInstallProgress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
    if (progressCallback) {
        progressCallback(progress);
    }
}

void RealESRGANInstaller::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;

    std::lock_guard<std::mutex> plock(progressMutex);
    progress.errorMessage = error;
}

// ============================================================================
// File List Builders
// ============================================================================

std::vector<RealESRGANModelFile> RealESRGANInstaller::getX4PlusFiles() {
    return {
        {
            X4PLUS_BIN_URL,
            "realesrgan-x4plus.bin",
            "RealESRGAN-x4plus (weights)",
            X4PLUS_BIN_SIZE,
            true
        },
        {
            X4PLUS_PARAM_URL,
            "realesrgan-x4plus.param",
            "RealESRGAN-x4plus (params)",
            X4PLUS_PARAM_SIZE,
            true
        }
    };
}

std::vector<RealESRGANModelFile> RealESRGANInstaller::getX4PlusAnimeFiles() {
    return {
        {
            X4PLUS_ANIME_BIN_URL,
            "realesrgan-x4plus-anime.bin",
            "RealESRGAN-x4plus-anime (weights)",
            X4PLUS_ANIME_BIN_SIZE,
            true
        },
        {
            X4PLUS_ANIME_PARAM_URL,
            "realesrgan-x4plus-anime.param",
            "RealESRGAN-x4plus-anime (params)",
            X4PLUS_ANIME_PARAM_SIZE,
            true
        }
    };
}

std::vector<RealESRGANModelFile> RealESRGANInstaller::getAnimeVideoV3Files() {
    return {
        // x2 model
        {
            ANIMEVIDEO_X2_BIN_URL,
            "realesr-animevideov3-x2.bin",
            "AnimeVideoV3-x2 (weights)",
            ANIMEVIDEO_X2_BIN_SIZE,
            true
        },
        {
            ANIMEVIDEO_X2_PARAM_URL,
            "realesr-animevideov3-x2.param",
            "AnimeVideoV3-x2 (params)",
            ANIMEVIDEO_X2_PARAM_SIZE,
            true
        },
        // x3 model
        {
            ANIMEVIDEO_X3_BIN_URL,
            "realesr-animevideov3-x3.bin",
            "AnimeVideoV3-x3 (weights)",
            ANIMEVIDEO_X3_BIN_SIZE,
            true
        },
        {
            ANIMEVIDEO_X3_PARAM_URL,
            "realesr-animevideov3-x3.param",
            "AnimeVideoV3-x3 (params)",
            ANIMEVIDEO_X3_PARAM_SIZE,
            true
        },
        // x4 model
        {
            ANIMEVIDEO_X4_BIN_URL,
            "realesr-animevideov3-x4.bin",
            "AnimeVideoV3-x4 (weights)",
            ANIMEVIDEO_X4_BIN_SIZE,
            true
        },
        {
            ANIMEVIDEO_X4_PARAM_URL,
            "realesr-animevideov3-x4.param",
            "AnimeVideoV3-x4 (params)",
            ANIMEVIDEO_X4_PARAM_SIZE,
            true
        }
    };
}

std::vector<RealESRGANModelFile> RealESRGANInstaller::getAllModelFiles() {
    std::vector<RealESRGANModelFile> all;

    auto x4plus = getX4PlusFiles();
    auto x4plusAnime = getX4PlusAnimeFiles();
    auto animeVideo = getAnimeVideoV3Files();

    all.insert(all.end(), x4plus.begin(), x4plus.end());
    all.insert(all.end(), x4plusAnime.begin(), x4plusAnime.end());
    all.insert(all.end(), animeVideo.begin(), animeVideo.end());

    return all;
}
