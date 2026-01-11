/**
 * RealESRGANInstaller.cpp
 *
 * Implementation of Real-ESRGAN model download and installation
 *
 * License: GPL3
 */

#include "RealESRGANInstaller.h"

// Helper to get programData without including program.h (avoids OpenGL header conflicts)
extern std::string getProgramDataPath();

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
    return getProgramDataPath() + "/EWOCvj2/models/upscale";
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

    auto modelFiles = getX4PlusModelFiles();
    prog.filesTotal = static_cast<int>(modelFiles.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    // Download and extract from release zip
    prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
    updateProgress(prog);

    if (!downloadAndExtractModels(config.modelsDir, config.tempDir, modelFiles)) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
        } else {
            prog.state = RealESRGANInstallProgress::State::FAILED;
            prog.errorMessage = getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
        }
        updateProgress(prog);
        installing.store(false);
        return;
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

    auto modelFiles = getX4PlusAnimeModelFiles();
    prog.filesTotal = static_cast<int>(modelFiles.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    // Download and extract from release zip
    prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
    updateProgress(prog);

    if (!downloadAndExtractModels(config.modelsDir, config.tempDir, modelFiles)) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
        } else {
            prog.state = RealESRGANInstallProgress::State::FAILED;
            prog.errorMessage = getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
        }
        updateProgress(prog);
        installing.store(false);
        return;
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
        prog.status = "AnimeVideoV3 models already installed";
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

    auto modelFiles = getAnimeVideoV3ModelFiles();
    prog.filesTotal = static_cast<int>(modelFiles.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    // Download and extract from release zip
    prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
    updateProgress(prog);

    if (!downloadAndExtractModels(config.modelsDir, config.tempDir, modelFiles)) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
        } else {
            prog.state = RealESRGANInstallProgress::State::FAILED;
            prog.errorMessage = getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
        }
        updateProgress(prog);
        installing.store(false);
        return;
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

    auto modelFiles = getAllModelFilesList();
    prog.filesTotal = static_cast<int>(modelFiles.size());
    prog.filesCompleted = 0;

    auto startTime = std::chrono::steady_clock::now();

    // Download and extract from release zip
    prog.state = RealESRGANInstallProgress::State::DOWNLOADING;
    updateProgress(prog);

    if (!downloadAndExtractModels(config.modelsDir, config.tempDir, modelFiles)) {
        if (shouldCancel.load()) {
            prog.state = RealESRGANInstallProgress::State::CANCELLED;
            prog.status = "Installation cancelled";
        } else {
            prog.state = RealESRGANInstallProgress::State::FAILED;
            prog.errorMessage = getLastError();
            prog.status = "FAILED: " + prog.errorMessage;
        }
        updateProgress(prog);
        installing.store(false);
        return;
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
// Model Filename Lists
// ============================================================================

std::vector<std::string> RealESRGANInstaller::getX4PlusModelFiles() {
    return {
        X4PLUS_BIN_FILE,
        X4PLUS_PARAM_FILE
    };
}

std::vector<std::string> RealESRGANInstaller::getX4PlusAnimeModelFiles() {
    return {
        X4PLUS_ANIME_BIN_FILE,
        X4PLUS_ANIME_PARAM_FILE
    };
}

std::vector<std::string> RealESRGANInstaller::getAnimeVideoV3ModelFiles() {
    return {
        ANIMEVIDEO_X2_BIN_FILE,
        ANIMEVIDEO_X2_PARAM_FILE,
        ANIMEVIDEO_X3_BIN_FILE,
        ANIMEVIDEO_X3_PARAM_FILE,
        ANIMEVIDEO_X4_BIN_FILE,
        ANIMEVIDEO_X4_PARAM_FILE
    };
}

std::vector<std::string> RealESRGANInstaller::getAllModelFilesList() {
    std::vector<std::string> all;

    auto x4plus = getX4PlusModelFiles();
    auto x4plusAnime = getX4PlusAnimeModelFiles();
    auto animeVideo = getAnimeVideoV3ModelFiles();

    all.insert(all.end(), x4plus.begin(), x4plus.end());
    all.insert(all.end(), x4plusAnime.begin(), x4plusAnime.end());
    all.insert(all.end(), animeVideo.begin(), animeVideo.end());

    return all;
}

// ============================================================================
// Zip-based Installation
// ============================================================================

bool RealESRGANInstaller::downloadAndExtractModels(const std::string& modelsDir,
                                                    const std::string& tempDir,
                                                    const std::vector<std::string>& modelFiles) {
    // Determine temp directory
    std::string actualTempDir = tempDir;
    if (actualTempDir.empty()) {
#ifdef _WIN32
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath)) {
            actualTempDir = std::string(tempPath) + "RealESRGAN_install";
        } else {
            actualTempDir = modelsDir + "/../temp";
        }
#else
        actualTempDir = "/tmp/RealESRGAN_install";
#endif
    }

    // Create temp directory
    if (!createDirectories(actualTempDir)) {
        setError("Failed to create temp directory: " + actualTempDir);
        return false;
    }

    std::string zipPath = actualTempDir + "/realesrgan-ncnn-vulkan.zip";
    std::string extractDir = actualTempDir + "/extracted";

    // Download the zip file
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.currentFile = "RealESRGAN ncnn models";
        progress.status = "Downloading RealESRGAN models (~45MB)...";
        if (progressCallback) progressCallback(progress);
    }

    if (!downloadFileWithResume(RELEASE_ZIP_URL, zipPath, RELEASE_ZIP_SIZE)) {
        return false;
    }

    // Extract the zip
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.status = "Extracting models...";
        if (progressCallback) progressCallback(progress);
    }

    if (!extractZip(zipPath, extractDir)) {
        // Cleanup zip on failure
        deleteFile(zipPath);
        return false;
    }

    // Copy model files to destination
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress.status = "Installing models...";
        if (progressCallback) progressCallback(progress);
    }

    if (!copyModelFiles(extractDir, modelsDir, modelFiles)) {
        // Cleanup on failure
        std::error_code ec;
        fs::remove_all(extractDir, ec);
        deleteFile(zipPath);
        return false;
    }

    // Cleanup temp files
    std::error_code ec;
    fs::remove_all(extractDir, ec);
    deleteFile(zipPath);
    fs::remove(actualTempDir, ec);

    return true;
}

bool RealESRGANInstaller::extractZip(const std::string& zipPath, const std::string& extractDir) {
#ifdef _WIN32
    // Use PowerShell to extract the zip
    if (!createDirectories(extractDir)) {
        setError("Failed to create extraction directory");
        return false;
    }

    // Convert paths to Windows format with backslashes
    std::string winZipPath = zipPath;
    std::string winExtractDir = extractDir;
    for (char& c : winZipPath) if (c == '/') c = '\\';
    for (char& c : winExtractDir) if (c == '/') c = '\\';

    // Build PowerShell command
    std::string psCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -Path '" +
                            winZipPath + "' -DestinationPath '" + winExtractDir + "' -Force\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char cmdBuf[4096];
    strncpy(cmdBuf, psCommand.c_str(), sizeof(cmdBuf) - 1);
    cmdBuf[sizeof(cmdBuf) - 1] = '\0';

    if (!CreateProcessA(NULL, cmdBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        setError("Failed to start PowerShell for extraction");
        return false;
    }

    // Wait for extraction to complete (max 5 minutes)
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 300000);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        setError("Zip extraction timed out");
        return false;
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        setError("PowerShell extraction failed with exit code " + std::to_string(exitCode));
        return false;
    }

    return true;
#else
    // Use system unzip on Linux/Mac
    std::string cmd = "unzip -o \"" + zipPath + "\" -d \"" + extractDir + "\"";
    int result = system(cmd.c_str());
    if (result != 0) {
        setError("Failed to extract zip file");
        return false;
    }
    return true;
#endif
}

bool RealESRGANInstaller::copyModelFiles(const std::string& extractDir,
                                          const std::string& modelsDir,
                                          const std::vector<std::string>& modelFiles) {
    // Create models directory if it doesn't exist
    if (!createDirectories(modelsDir)) {
        setError("Failed to create models directory: " + modelsDir);
        return false;
    }

    // Find the models subdirectory in the extracted content
    // The zip structure is: realesrgan-ncnn-vulkan-*/models/
    std::string modelsSubdir;
    for (const auto& entry : fs::directory_iterator(extractDir)) {
        if (entry.is_directory()) {
            fs::path candidateModels = entry.path() / "models";
            if (fs::exists(candidateModels) && fs::is_directory(candidateModels)) {
                modelsSubdir = candidateModels.string();
                break;
            }
        }
    }

    if (modelsSubdir.empty()) {
        // Try direct models folder
        fs::path directModels = fs::path(extractDir) / "models";
        if (fs::exists(directModels)) {
            modelsSubdir = directModels.string();
        } else {
            setError("Could not find models folder in extracted zip");
            return false;
        }
    }

    // Copy each required model file
    int filesCompleted = 0;
    int filesTotal = static_cast<int>(modelFiles.size());

    for (const auto& filename : modelFiles) {
        if (shouldCancel.load()) {
            return false;
        }

        fs::path srcPath = fs::path(modelsSubdir) / filename;
        fs::path dstPath = fs::path(modelsDir) / filename;

        if (!fs::exists(srcPath)) {
            setError("Model file not found in zip: " + filename);
            return false;
        }

        std::error_code ec;
        fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            setError("Failed to copy " + filename + ": " + ec.message());
            return false;
        }

        filesCompleted++;
        {
            std::lock_guard<std::mutex> lock(progressMutex);
            progress.filesCompleted = filesCompleted;
            progress.filesTotal = filesTotal;
            progress.percentComplete = (static_cast<float>(filesCompleted) / filesTotal) * 100.0f;
            progress.status = "Installed " + filename;
            if (progressCallback) progressCallback(progress);
        }
    }

    return true;
}
