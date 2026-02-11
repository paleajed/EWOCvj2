/**
 * InstallVerification.h
 *
 * Shared utility for verifying installation completeness across all installers.
 * Uses a JSON manifest system to track installed files and verify integrity.
 *
 * Features:
 * - Tracks all installed files with expected sizes
 * - Writes manifest only after successful installation
 * - Verifies file existence AND size on startup
 * - Detects partial/interrupted installations
 *
 * Usage:
 *   // After successful installation:
 *   InstallManifest manifest;
 *   manifest.componentId = "hunyuan_video";
 *   manifest.addFile("models/model.safetensors", 5090000000LL);
 *   InstallVerification::writeManifest(installDir, manifest);
 *
 *   // On startup verification:
 *   if (!InstallVerification::verifyInstallation(installDir, "hunyuan_video")) {
 *       // Installation incomplete or corrupted
 *   }
 *
 * License: GPL3
 */

#ifndef INSTALL_VERIFICATION_H
#define INSTALL_VERIFICATION_H

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Describes a single installed file
 */
struct InstalledFile {
    std::string relativePath;     // Path relative to install directory
    int64_t expectedSize = 0;     // Expected file size in bytes (0 = don't verify size)
    std::string sha256 = "";      // Optional SHA256 hash (empty = don't verify hash)

    InstalledFile() = default;
    InstalledFile(const std::string& path, int64_t size, const std::string& hash = "")
        : relativePath(path), expectedSize(size), sha256(hash) {}
};

/**
 * Installation manifest - tracks what was installed and verification info
 */
struct InstallManifest {
    std::string componentId;              // Unique component identifier (e.g., "hunyuan_video", "flux_schnell")
    std::string componentName;            // Human-readable name
    std::string version;                  // Version string (e.g., "1.0.0")
    std::vector<InstalledFile> files;     // List of installed files
    std::vector<std::string> directories; // Directories that should exist (for git repos, etc.)
    time_t installTime = 0;               // Unix timestamp of installation
    bool complete = false;                // True only if installation finished successfully

    // Helper to add a file
    void addFile(const std::string& relativePath, int64_t expectedSize, const std::string& sha256 = "") {
        files.emplace_back(relativePath, expectedSize, sha256);
    }

    // Helper to add a directory (for git clones, etc.)
    void addDirectory(const std::string& relativePath) {
        directories.push_back(relativePath);
    }

    // Clear all entries
    void clear() {
        componentId.clear();
        componentName.clear();
        version.clear();
        files.clear();
        directories.clear();
        installTime = 0;
        complete = false;
    }
};

/**
 * Verification result with details about what failed
 */
struct VerificationResult {
    bool success = false;
    bool manifestExists = false;
    bool manifestComplete = false;
    std::vector<std::string> missingFiles;
    std::vector<std::string> sizeMismatchFiles;
    std::vector<std::string> missingDirectories;
    std::string errorMessage;

    bool isValid() const {
        return success && manifestExists && manifestComplete &&
               missingFiles.empty() && sizeMismatchFiles.empty() && missingDirectories.empty();
    }
};

/**
 * PrerequisiteLock - RAII lock for shared prerequisites (7-Zip, Git, Python, ComfyUI base)
 *
 * Prevents multiple installers from trying to install the same prerequisite simultaneously.
 * Uses named mutexes on Windows for cross-process synchronization.
 *
 * Usage:
 *   {
 *       PrerequisiteLock lock("7zip");
 *       if (lock.acquired()) {
 *           // Re-check if still needed (another process might have installed it)
 *           if (!is7ZipInstalled()) {
 *               install7Zip();
 *           }
 *       }
 *   } // Lock automatically released
 */
class PrerequisiteLock {
public:
    /**
     * Acquire lock for a prerequisite
     * @param prerequisiteId Unique ID for the prerequisite (e.g., "7zip", "git", "python312", "comfyui_base")
     * @param timeoutMs Maximum time to wait for lock (default 5 minutes)
     */
    explicit PrerequisiteLock(const std::string& prerequisiteId, int timeoutMs = 300000);

    /**
     * Release lock on destruction
     */
    ~PrerequisiteLock();

    // Prevent copying
    PrerequisiteLock(const PrerequisiteLock&) = delete;
    PrerequisiteLock& operator=(const PrerequisiteLock&) = delete;

    /**
     * Check if lock was successfully acquired
     * @return true if this instance holds the lock
     */
    bool acquired() const { return lockAcquired; }

    /**
     * Get the prerequisite ID
     */
    const std::string& getId() const { return prereqId; }

private:
    std::string prereqId;
    bool lockAcquired = false;

#ifdef _WIN32
    HANDLE mutexHandle = nullptr;
#else
    int lockFd = -1;
    std::string lockFilePath;
#endif
};

/**
 * Shared prerequisite IDs - use these constants for consistency
 */
namespace PrerequisiteIds {
    constexpr const char* SEVENZIP = "ewocvj2_prereq_7zip";
    constexpr const char* GIT = "ewocvj2_prereq_git";
    constexpr const char* PYTHON312 = "ewocvj2_prereq_python312";
    constexpr const char* COMFYUI_BASE = "ewocvj2_prereq_comfyui_base";
    constexpr const char* PYTORCH_CUDA = "ewocvj2_prereq_pytorch_cuda";
}

/**
 * Helper to safely install a prerequisite with locking
 *
 * Uses a polling loop to handle long-running installations by other processes:
 * - Repeatedly tries to acquire lock with short timeout (5 sec)
 * - Between attempts, checks if prerequisite was installed by another process
 * - Exits when either lock is acquired OR installation is detected
 * - Maximum wait time: 30 minutes (360 attempts * 5 seconds)
 *
 * @param prerequisiteId Prerequisite identifier (use PrerequisiteIds constants)
 * @param isInstalledFn Function that returns true if already installed
 * @param installFn Function that performs the installation
 * @param pollIntervalMs Interval between lock attempts in milliseconds (default 5000)
 * @param waitingCallback Optional callback invoked when waiting for another installer (receives prerequisiteId)
 * @return true if prerequisite is now installed (either was already or installed successfully)
 */
bool installPrerequisiteWithLock(
    const std::string& prerequisiteId,
    std::function<bool()> isInstalledFn,
    std::function<bool()> installFn,
    int pollIntervalMs = 5000,
    std::function<void(const std::string&)> waitingCallback = nullptr);

/**
 * InstallVerification - Static utility class for installation verification
 */
class InstallVerification {
public:
    // === Manifest Operations ===

    /**
     * Write installation manifest to disk
     * Call this ONLY after all files are successfully downloaded and verified
     * @param installDir Base installation directory
     * @param manifest The manifest to write
     * @return true if manifest was written successfully
     */
    static bool writeManifest(const std::string& installDir, const InstallManifest& manifest);

    /**
     * Read installation manifest from disk
     * @param installDir Base installation directory
     * @param componentId Component identifier to look for
     * @param outManifest Output manifest (filled if found)
     * @return true if manifest was found and parsed successfully
     */
    static bool readManifest(const std::string& installDir, const std::string& componentId,
                             InstallManifest& outManifest);

    /**
     * Delete installation manifest (for uninstallation or reset)
     * @param installDir Base installation directory
     * @param componentId Component identifier
     * @return true if deleted or didn't exist
     */
    static bool deleteManifest(const std::string& installDir, const std::string& componentId);

    // === Verification ===

    /**
     * Verify installation completeness
     * Checks manifest exists, is marked complete, and all files exist with correct sizes
     * @param installDir Base installation directory
     * @param componentId Component identifier to verify
     * @return VerificationResult with detailed status
     */
    static VerificationResult verifyInstallation(const std::string& installDir,
                                                  const std::string& componentId);

    /**
     * Quick check if installation is complete (doesn't verify file sizes)
     * Use verifyInstallation() for full verification
     * @param installDir Base installation directory
     * @param componentId Component identifier
     * @return true if manifest exists and is marked complete
     */
    static bool isInstallationComplete(const std::string& installDir, const std::string& componentId);

    /**
     * Verify a single file exists and optionally matches expected size
     * @param filePath Full path to file
     * @param expectedSize Expected size (0 = don't check size)
     * @param sizeTolerance Tolerance for size mismatch (0.0 = exact, 0.1 = within 10%)
     * @return true if file exists and size matches (within tolerance)
     */
    static bool verifyFile(const std::string& filePath, int64_t expectedSize = 0,
                           float sizeTolerance = 0.0f);

    /**
     * Verify multiple files exist with correct sizes
     * @param installDir Base installation directory
     * @param files List of files to verify
     * @param sizeTolerance Tolerance for size mismatch
     * @return VerificationResult with details
     */
    static VerificationResult verifyFiles(const std::string& installDir,
                                          const std::vector<InstalledFile>& files,
                                          float sizeTolerance = 0.0f);

    // === Utility ===

    /**
     * Get manifest file path for a component
     * @param installDir Base installation directory
     * @param componentId Component identifier
     * @return Full path to manifest file
     */
    static std::string getManifestPath(const std::string& installDir, const std::string& componentId);

    /**
     * Get file size
     * @param filePath Full path to file
     * @return File size in bytes, or -1 if file doesn't exist
     */
    static int64_t getFileSize(const std::string& filePath);

    /**
     * Check if file exists
     * @param filePath Full path to file
     * @return true if file exists
     */
    static bool fileExists(const std::string& filePath);

    /**
     * Check if directory exists
     * @param dirPath Full path to directory
     * @return true if directory exists
     */
    static bool directoryExists(const std::string& dirPath);

    /**
     * Format size as human-readable string
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 GB")
     */
    static std::string formatSize(int64_t bytes);

    // === Manifest Directory ===

    /**
     * Get the manifests directory path
     * Creates it if it doesn't exist
     * @param installDir Base installation directory
     * @return Path to manifests directory
     */
    static std::string getManifestsDir(const std::string& installDir);

private:
    // Manifest filename suffix
    static constexpr const char* MANIFEST_EXTENSION = ".manifest.json";
    static constexpr const char* MANIFESTS_DIR = ".manifests";
};

#endif // INSTALL_VERIFICATION_H
