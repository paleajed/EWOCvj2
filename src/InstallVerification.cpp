/**
 * InstallVerification.cpp
 *
 * Implementation of installation verification utilities
 *
 * License: GPL3
 */

#include "InstallVerification.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

// ============================================================================
// PrerequisiteLock Implementation
// ============================================================================

PrerequisiteLock::PrerequisiteLock(const std::string& prerequisiteId, int timeoutMs)
    : prereqId(prerequisiteId), lockAcquired(false) {

#ifdef _WIN32
    // Create a named mutex for cross-process synchronization
    // Mutex names must not contain backslashes, use underscores
    std::string mutexName = "Global\\" + prereqId;

    // Create or open the mutex
    mutexHandle = CreateMutexA(nullptr, FALSE, mutexName.c_str());

    if (mutexHandle == nullptr) {
        // Failed to create mutex - try without Global prefix (for non-admin)
        mutexName = prereqId;
        mutexHandle = CreateMutexA(nullptr, FALSE, mutexName.c_str());
    }

    if (mutexHandle != nullptr) {
        // Wait for the mutex with timeout
        DWORD waitResult = WaitForSingleObject(mutexHandle, static_cast<DWORD>(timeoutMs));

        if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED) {
            lockAcquired = true;
        } else {
            // Timeout or error - close handle
            CloseHandle(mutexHandle);
            mutexHandle = nullptr;
        }
    }
#else
    // Unix: Use file locking
    // Create lock file in /tmp
    lockFilePath = "/tmp/" + prereqId + ".lock";

    lockFd = open(lockFilePath.c_str(), O_CREAT | O_RDWR, 0666);
    if (lockFd >= 0) {
        // Try to acquire exclusive lock with timeout
        // Note: flock doesn't support timeout directly, so we poll
        int elapsed = 0;
        const int pollInterval = 100; // 100ms

        while (elapsed < timeoutMs) {
            if (flock(lockFd, LOCK_EX | LOCK_NB) == 0) {
                lockAcquired = true;
                break;
            }
            usleep(pollInterval * 1000);
            elapsed += pollInterval;
        }

        if (!lockAcquired) {
            close(lockFd);
            lockFd = -1;
        }
    }
#endif
}

PrerequisiteLock::~PrerequisiteLock() {
#ifdef _WIN32
    if (mutexHandle != nullptr) {
        if (lockAcquired) {
            ReleaseMutex(mutexHandle);
        }
        CloseHandle(mutexHandle);
        mutexHandle = nullptr;
    }
#else
    if (lockFd >= 0) {
        if (lockAcquired) {
            flock(lockFd, LOCK_UN);
        }
        close(lockFd);
        lockFd = -1;
    }
#endif
    lockAcquired = false;
}

// ============================================================================
// Prerequisite Installation Helper
// ============================================================================

bool installPrerequisiteWithLock(
    const std::string& prerequisiteId,
    std::function<bool()> isInstalledFn,
    std::function<bool()> installFn,
    int pollIntervalMs,
    std::function<void(const std::string&)> waitingCallback) {

    // First quick check without lock
    if (isInstalledFn()) {
        return true;
    }

    // Polling loop: keep trying to acquire lock OR detect installation completion
    // This handles long-running installations by another process without fixed timeouts
    const int lockAttemptTimeoutMs = 5000;  // Short timeout per lock attempt
    const int maxAttempts = 360;  // 360 * 5 seconds = 30 minutes max wait

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        // Check if another installer completed the installation while we waited
        if (attempt > 0 && isInstalledFn()) {
            return true;
        }

        // Try to acquire lock with short timeout
        PrerequisiteLock lock(prerequisiteId, lockAttemptTimeoutMs);

        if (lock.acquired()) {
            // Lock acquired - re-check if still needed (another installer may have just completed)
            if (isInstalledFn()) {
                return true;  // Already installed
            }

            // Still not installed - we're responsible for installing
            return installFn();
        }

        // Lock not acquired - another installer is working on it
        // Notify caller that we're waiting (so they can update status to "Waiting...")
        if (waitingCallback) {
            waitingCallback(prerequisiteId);
        }

        // Wait before next attempt (using pollIntervalMs or default 5 seconds)
        int waitMs = (pollIntervalMs > 0) ? pollIntervalMs : 5000;
        #ifdef _WIN32
        Sleep(waitMs);
        #else
        usleep(waitMs * 1000);
        #endif
    }

    // Max attempts reached - final check if it got installed
    return isInstalledFn();
}

// ============================================================================
// Manifest Operations
// ============================================================================

bool InstallVerification::writeManifest(const std::string& installDir, const InstallManifest& manifest) {
    try {
        // Ensure manifests directory exists
        std::string manifestsDir = getManifestsDir(installDir);

        // Build JSON object
        json j;
        j["componentId"] = manifest.componentId;
        j["componentName"] = manifest.componentName;
        j["version"] = manifest.version;
        j["installTime"] = manifest.installTime != 0 ? manifest.installTime : std::time(nullptr);
        j["complete"] = manifest.complete;

        // Files array
        json filesArray = json::array();
        for (const auto& file : manifest.files) {
            json fileObj;
            fileObj["path"] = file.relativePath;
            fileObj["size"] = file.expectedSize;
            if (!file.sha256.empty()) {
                fileObj["sha256"] = file.sha256;
            }
            filesArray.push_back(fileObj);
        }
        j["files"] = filesArray;

        // Directories array
        json dirsArray = json::array();
        for (const auto& dir : manifest.directories) {
            dirsArray.push_back(dir);
        }
        j["directories"] = dirsArray;

        // Write to file with pretty formatting
        std::string manifestPath = getManifestPath(installDir, manifest.componentId);
        std::ofstream file(manifestPath);
        if (!file.is_open()) {
            return false;
        }
        file << std::setw(2) << j << std::endl;
        file.close();

        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool InstallVerification::readManifest(const std::string& installDir, const std::string& componentId,
                                        InstallManifest& outManifest) {
    try {
        std::string manifestPath = getManifestPath(installDir, componentId);

        if (!fs::exists(manifestPath)) {
            return false;
        }

        std::ifstream file(manifestPath);
        if (!file.is_open()) {
            return false;
        }

        json j;
        file >> j;
        file.close();

        outManifest.clear();
        outManifest.componentId = j.value("componentId", "");
        outManifest.componentName = j.value("componentName", "");
        outManifest.version = j.value("version", "");
        outManifest.installTime = j.value("installTime", static_cast<time_t>(0));
        outManifest.complete = j.value("complete", false);

        // Parse files
        if (j.contains("files") && j["files"].is_array()) {
            for (const auto& fileObj : j["files"]) {
                InstalledFile installedFile;
                installedFile.relativePath = fileObj.value("path", "");
                installedFile.expectedSize = fileObj.value("size", static_cast<int64_t>(0));
                installedFile.sha256 = fileObj.value("sha256", "");
                if (!installedFile.relativePath.empty()) {
                    outManifest.files.push_back(installedFile);
                }
            }
        }

        // Parse directories
        if (j.contains("directories") && j["directories"].is_array()) {
            for (const auto& dir : j["directories"]) {
                if (dir.is_string()) {
                    outManifest.directories.push_back(dir.get<std::string>());
                }
            }
        }

        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool InstallVerification::deleteManifest(const std::string& installDir, const std::string& componentId) {
    try {
        std::string manifestPath = getManifestPath(installDir, componentId);
        if (fs::exists(manifestPath)) {
            std::error_code ec;
            fs::remove(manifestPath, ec);
            return !ec;
        }
        return true;  // Already doesn't exist
    }
    catch (const std::exception&) {
        return false;
    }
}

// ============================================================================
// Verification
// ============================================================================

VerificationResult InstallVerification::verifyInstallation(const std::string& installDir,
                                                            const std::string& componentId) {
    VerificationResult result;

    // Try to read manifest
    InstallManifest manifest;
    if (!readManifest(installDir, componentId, manifest)) {
        result.success = false;
        result.manifestExists = false;
        result.errorMessage = "Manifest not found for component: " + componentId;
        return result;
    }

    result.manifestExists = true;
    result.manifestComplete = manifest.complete;

    // Check if installation was marked as complete
    if (!manifest.complete) {
        result.success = false;
        result.errorMessage = "Installation was not completed (interrupted?)";
        return result;
    }

    // Verify all files
    for (const auto& file : manifest.files) {
        fs::path fullPath = fs::path(installDir) / file.relativePath;

        if (!fs::exists(fullPath)) {
            result.missingFiles.push_back(file.relativePath);
            continue;
        }

        // Verify size if specified
        if (file.expectedSize > 0) {
            int64_t actualSize = getFileSize(fullPath.string());
            // Allow 5% tolerance for size differences (compression, metadata, etc.)
            float tolerance = 0.05f;
            int64_t minSize = static_cast<int64_t>(file.expectedSize * (1.0f - tolerance));
            int64_t maxSize = static_cast<int64_t>(file.expectedSize * (1.0f + tolerance));

            if (actualSize < minSize || actualSize > maxSize) {
                result.sizeMismatchFiles.push_back(
                    file.relativePath + " (expected: " + formatSize(file.expectedSize) +
                    ", actual: " + formatSize(actualSize) + ")");
            }
        }
    }

    // Verify all directories
    for (const auto& dir : manifest.directories) {
        fs::path fullPath = fs::path(installDir) / dir;
        if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
            result.missingDirectories.push_back(dir);
        }
    }

    // Determine overall success
    result.success = result.missingFiles.empty() &&
                     result.sizeMismatchFiles.empty() &&
                     result.missingDirectories.empty();

    if (!result.success) {
        std::string errorMsg = "Verification failed: ";
        if (!result.missingFiles.empty()) {
            errorMsg += std::to_string(result.missingFiles.size()) + " missing files, ";
        }
        if (!result.sizeMismatchFiles.empty()) {
            errorMsg += std::to_string(result.sizeMismatchFiles.size()) + " size mismatches, ";
        }
        if (!result.missingDirectories.empty()) {
            errorMsg += std::to_string(result.missingDirectories.size()) + " missing directories";
        }
        result.errorMessage = errorMsg;
    }

    return result;
}

bool InstallVerification::isInstallationComplete(const std::string& installDir, const std::string& componentId) {
    InstallManifest manifest;
    if (!readManifest(installDir, componentId, manifest)) {
        return false;
    }
    return manifest.complete;
}

bool InstallVerification::verifyFile(const std::string& filePath, int64_t expectedSize, float sizeTolerance) {
    if (!fs::exists(filePath)) {
        return false;
    }

    if (expectedSize > 0) {
        int64_t actualSize = getFileSize(filePath);
        if (actualSize < 0) {
            return false;
        }

        if (sizeTolerance > 0.0f) {
            int64_t minSize = static_cast<int64_t>(expectedSize * (1.0f - sizeTolerance));
            int64_t maxSize = static_cast<int64_t>(expectedSize * (1.0f + sizeTolerance));
            return actualSize >= minSize && actualSize <= maxSize;
        } else {
            return actualSize == expectedSize;
        }
    }

    return true;  // File exists and no size check required
}

VerificationResult InstallVerification::verifyFiles(const std::string& installDir,
                                                     const std::vector<InstalledFile>& files,
                                                     float sizeTolerance) {
    VerificationResult result;
    result.manifestExists = true;  // Not using manifest for this method
    result.manifestComplete = true;

    for (const auto& file : files) {
        fs::path fullPath = fs::path(installDir) / file.relativePath;

        if (!fs::exists(fullPath)) {
            result.missingFiles.push_back(file.relativePath);
            continue;
        }

        if (file.expectedSize > 0) {
            int64_t actualSize = getFileSize(fullPath.string());
            bool sizeOk = false;

            if (sizeTolerance > 0.0f) {
                int64_t minSize = static_cast<int64_t>(file.expectedSize * (1.0f - sizeTolerance));
                int64_t maxSize = static_cast<int64_t>(file.expectedSize * (1.0f + sizeTolerance));
                sizeOk = actualSize >= minSize && actualSize <= maxSize;
            } else {
                sizeOk = actualSize == file.expectedSize;
            }

            if (!sizeOk) {
                result.sizeMismatchFiles.push_back(
                    file.relativePath + " (expected: " + formatSize(file.expectedSize) +
                    ", actual: " + formatSize(actualSize) + ")");
            }
        }
    }

    result.success = result.missingFiles.empty() && result.sizeMismatchFiles.empty();
    return result;
}

// ============================================================================
// Utility
// ============================================================================

std::string InstallVerification::getManifestPath(const std::string& installDir, const std::string& componentId) {
    return (fs::path(getManifestsDir(installDir)) / (componentId + MANIFEST_EXTENSION)).string();
}

int64_t InstallVerification::getFileSize(const std::string& filePath) {
    try {
        if (!fs::exists(filePath)) {
            return -1;
        }
        return static_cast<int64_t>(fs::file_size(filePath));
    }
    catch (const std::exception&) {
        return -1;
    }
}

bool InstallVerification::fileExists(const std::string& filePath) {
    try {
        return fs::exists(filePath) && fs::is_regular_file(filePath);
    }
    catch (const std::exception&) {
        return false;
    }
}

bool InstallVerification::directoryExists(const std::string& dirPath) {
    try {
        return fs::exists(dirPath) && fs::is_directory(dirPath);
    }
    catch (const std::exception&) {
        return false;
    }
}

std::string InstallVerification::formatSize(int64_t bytes) {
    if (bytes < 0) return "Unknown";

    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << static_cast<int64_t>(size) << " " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    }
    return oss.str();
}

std::string InstallVerification::getManifestsDir(const std::string& installDir) {
    fs::path manifestsPath = fs::path(installDir) / MANIFESTS_DIR;
    try {
        if (!fs::exists(manifestsPath)) {
            fs::create_directories(manifestsPath);
        }
    }
    catch (const std::exception&) {
        // Ignore, will fail on write
    }
    return manifestsPath.string();
}
