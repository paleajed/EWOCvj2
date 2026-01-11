/**
 * ComfyUIInstaller.h
 *
 * Downloads and installs ComfyUI with HunyuanVideo GGUF backend
 * for VRAM-friendly video generation (~12GB recommended)
 *
 * All models are optimized for consumer hardware (GGUF quantized)
 *
 * License: GPL3
 */

#ifndef COMFYUI_INSTALLER_H
#define COMFYUI_INSTALLER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

/**
 * Installation component identifiers
 */
enum class InstallComponent {
    COMFYUI_BASE = 0,           // ComfyUI portable base
    HUNYUAN_VIDEO = 1,          // HunyuanVideo GGUF stack
    FLUX_SCHNELL = 2,           // Flux.1 Schnell NF4 (fast image generation)
    COMPONENT_COUNT = 3
};

/**
 * Download/installation progress
 */
struct InstallProgress {
    enum class State {
        IDLE = 0,
        CHECKING = 1,
        DOWNLOADING = 2,
        EXTRACTING = 3,
        INSTALLING_NODES = 4,
        VERIFYING = 5,
        COMPLETE = 6,
        FAILED = 7,
        CANCELLED = 8
    };

    State state = State::IDLE;
    std::string status = "Idle";
    std::string currentFile = "";

    // Download progress
    int64_t bytesDownloaded = 0;
    int64_t bytesTotal = 0;
    float downloadSpeed = 0.0f;       // bytes/second

    // Overall progress
    int filesCompleted = 0;
    int filesTotal = 0;
    float percentComplete = 0.0f;     // -1.0 = indeterminate (don't show percentage)

    // Time estimates
    float elapsedTime = 0.0f;
    float estimatedTimeRemaining = 0.0f;

    // Error info
    std::string errorMessage = "";
    int errorCode = 0;
};

/**
 * File download descriptor
 */
struct DownloadFile {
    std::string url;              // Source URL
    std::string localPath;        // Destination path (relative to install dir)
    std::string description;      // Human-readable description
    int64_t expectedSize = 0;     // Expected file size in bytes (0 = unknown)
    std::string sha256 = "";      // Optional SHA256 hash for verification
    bool required = true;         // If false, skip on failure
};

/**
 * Model component descriptor - extensible unit of installation
 * Each component represents a logical group of files/nodes that can be
 * checked and installed independently
 */
struct ModelComponent {
    std::string id;                           // Unique identifier (e.g., "hunyuan_t2v", "flux_schnell")
    std::string name;                         // Human-readable name
    std::string description;                  // What this component provides
    std::vector<DownloadFile> files;          // Files to download
    std::vector<std::string> customNodes;     // Git URLs for custom nodes
    std::vector<std::string> checkFiles;      // Files to check for existence (relative paths)
    bool required = true;                     // If false, component is optional
    bool enabled = true;                      // Can be disabled by user preference
};

/**
 * Installation configuration
 */
struct InstallConfig {
    std::string installDir = "";              // Base installation directory
    std::string tempDir = "";                 // Temporary download directory
    bool verifyDownloads = true;              // Verify file sizes/hashes
    bool cleanupOnFailure = true;             // Remove partial downloads on failure
    bool resumeDownloads = true;              // Resume interrupted downloads
    int maxRetries = 3;                       // Retry failed downloads
    int connectionTimeout = 30000;            // Connection timeout (ms)
    int downloadTimeout = 0;                  // Download timeout per file (ms), 0 = no timeout

    // Component selection for installAll()
    // ComfyUI base is always installed if any component is selected
    bool installHunyuanVideo = true;          // Install HunyuanVideo models
    bool installFluxSchnell = true;           // Install Flux.1 Schnell models
};

/**
 * ComfyUIInstaller - Downloads and installs ComfyUI video generation stack
 *
 * Usage:
 *   ComfyUIInstaller installer;
 *   InstallConfig config;
 *   config.installDir = "C:/ProgramData/EWOCvj2/ComfyUI";
 *
 *   installer.setProgressCallback([](const InstallProgress& p) {
 *       std::cout << p.status << " " << p.percentComplete << "%" << std::endl;
 *   });
 *
 *   // Install HunyuanVideo (needs 12GB+ VRAM)
 *   if (installer.installHunyuanVideo(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 */
class ComfyUIInstaller {
public:
    ComfyUIInstaller();
    ~ComfyUIInstaller();

    // Prevent copying
    ComfyUIInstaller(const ComfyUIInstaller&) = delete;
    ComfyUIInstaller& operator=(const ComfyUIInstaller&) = delete;

    // === Installation Methods ===

    /**
     * Install ComfyUI base (required for both backends)
     * Downloads portable ComfyUI and essential custom nodes
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installComfyUIBase(const InstallConfig& config);

    /**
     * Install HunyuanVideo stack (GGUF quantized for low VRAM)
     * Includes: HunyuanVideo T2V Q4, I2V Q4, VAE, text encoders
     * VRAM requirement: ~8GB minimum (Q4), ~12GB recommended
     * Download size: ~20GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installHunyuanVideo(const InstallConfig& config);

    /**
     * Install Flux.1 Schnell NF4 (fast image generation)
     * Includes: Flux Schnell transformer NF4, CLIP, T5XXL, VAE
     * VRAM requirement: ~8GB minimum, ~12GB recommended
     * Download size: ~13GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installFluxSchnell(const InstallConfig& config);

    /**
     * Install everything (ComfyUI + HunyuanVideo + Flux)
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installAll(const InstallConfig& config);

    // === Status ===

    /**
     * Check if installation is in progress
     */
    bool isInstalling() const { return installing.load(); }

    /**
     * Cancel ongoing installation
     */
    void cancelInstallation();

    /**
     * Get current installation progress
     */
    InstallProgress getProgress() const;

    /**
     * Set progress callback (called from download thread)
     */
    void setProgressCallback(std::function<void(const InstallProgress&)> callback);

    // === Verification ===

    /**
     * Check if ComfyUI base is installed
     * @param installDir Installation directory
     */
    static bool isComfyUIInstalled(const std::string& installDir);

    /**
     * Check if HunyuanVideo is installed
     * @param installDir Installation directory
     */
    static bool isHunyuanVideoInstalled(const std::string& installDir);

    /**
     * Check if Flux.1 Schnell is installed
     * @param installDir Installation directory
     */
    static bool isFluxSchnellInstalled(const std::string& installDir);

    /**
     * Get required disk space for a component (bytes)
     */
    static int64_t getRequiredDiskSpace(InstallComponent component);

    /**
     * Get download size for a component (bytes)
     */
    static int64_t getDownloadSize(InstallComponent component);

    /**
     * Get human-readable size string
     */
    static std::string formatSize(int64_t bytes);

    // === Component Management ===

    /**
     * Get all components for ComfyUI base installation
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getComfyUIBaseComponents();

    /**
     * Get all components for HunyuanVideo backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getHunyuanComponents();

    /**
     * Get all components for Flux.1 Schnell backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getFluxSchnellComponents();

    /**
     * Check if a specific component is installed
     * @param component The component to check
     * @param installDir Installation directory
     * @return true if all check files exist
     */
    static bool isComponentInstalled(const ModelComponent& component,
                                      const std::string& installDir);

    /**
     * Get list of missing components for a backend
     * @param components All components to check
     * @param installDir Installation directory
     * @return Vector of components that are not fully installed
     */
    static std::vector<ModelComponent> getMissingComponents(
        const std::vector<ModelComponent>& components,
        const std::string& installDir);

    /**
     * Install only missing components (incremental install)
     * @param config Installation configuration
     * @param components Components to check and install if missing
     * @return true if installation started
     */
    bool installMissingComponents(const InstallConfig& config,
                                   const std::vector<ModelComponent>& components);

    // === Uninstallation ===

    /**
     * Remove HunyuanVideo models (keeps ComfyUI base)
     */
    bool uninstallHunyuanVideo(const std::string& installDir);

    /**
     * Remove Flux.1 Schnell models (keeps ComfyUI base)
     */
    bool uninstallFluxSchnell(const std::string& installDir);

    /**
     * Remove everything
     */
    bool uninstallAll(const std::string& installDir);

    // === Prerequisites ===

    /**
     * Check if all prerequisites are installed (7-Zip, Git)
     */
    static bool checkPrerequisites();

    /**
     * Check if 7-Zip is installed
     */
    static bool is7ZipInstalled();

    /**
     * Check if Git is installed
     */
    static bool isGitInstalled();

    /**
     * Install missing prerequisites automatically
     * Downloads and installs 7-Zip and/or Git if not present
     * @param config Installation configuration (uses tempDir for downloads)
     * @return true if all prerequisites are now available
     */
    bool installPrerequisites(const InstallConfig& config);

    /**
     * Install 7-Zip silently
     */
    bool install7Zip(const std::string& tempDir);

    /**
     * Install Git silently
     */
    bool installGit(const std::string& tempDir);

    // === Error Handling ===

    std::string getLastError() const;
    void clearError();

private:
    // State
    std::atomic<bool> installing{false};
    std::atomic<bool> shouldCancel{false};
    std::atomic<bool> runningInstallAll{false};  // Prevents sub-threads from clearing installing flag
    InstallConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> installThread;

    // Progress
    mutable std::mutex progressMutex;
    InstallProgress progress;
    std::function<void(const InstallProgress&)> progressCallback;

    // Error
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Download URLs ===

    // Prerequisites (7-Zip and Git for Windows)
    static constexpr const char* SEVENZIP_URL =
        "https://www.7-zip.org/a/7z2408-x64.exe";  // 7-Zip 24.08 x64 installer
    static constexpr int64_t SEVENZIP_SIZE = 1600000LL;  // ~1.6MB

    static constexpr const char* GIT_URL =
        "https://github.com/git-for-windows/git/releases/download/v2.47.1.windows.1/Git-2.47.1-64-bit.exe";
    static constexpr int64_t GIT_SIZE = 65000000LL;  // ~65MB

    // ComfyUI Base
    static constexpr const char* COMFYUI_PORTABLE_URL =
        "https://github.com/comfyanonymous/ComfyUI/releases/download/v0.7.0/ComfyUI_windows_portable_nvidia.7z";
    static constexpr int64_t COMFYUI_PORTABLE_SIZE = 1700000000LL;  // ~1.7GB (estimated)


    // HunyuanVideo 1.5 GGUF (VRAM-friendly quantized models)
    static constexpr const char* HUNYUAN_T2V_Q4_URL =
        "https://huggingface.co/jayn7/HunyuanVideo-1.5_T2V_720p-GGUF/resolve/main/720p/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf";
    static constexpr int64_t HUNYUAN_T2V_Q4_SIZE = 5090000000LL;  // ~5.09GB (was 7.74GB in old version)

    static constexpr const char* HUNYUAN_I2V_Q4_URL =
        "https://huggingface.co/jayn7/HunyuanVideo-1.5_I2V_720p-GGUF/resolve/main/720p/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf";
    static constexpr int64_t HUNYUAN_I2V_Q4_SIZE = 5090000000LL;  // ~5.09GB (was 8GB in old version)

    // HunyuanVideo 1.5 VAE
    static constexpr const char* HUNYUAN_VAE_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/vae/hunyuanvideo15_vae_fp16.safetensors";
    static constexpr int64_t HUNYUAN_VAE_SIZE = 493000000LL;  // ~493MB

    // HunyuanVideo 1.5 CLIP text encoders (qwen 2.5 + byt5)
    static constexpr const char* HUNYUAN_QWEN_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors";
    static constexpr int64_t HUNYUAN_QWEN_SIZE = 8100000000LL;  // ~8.1GB

    static constexpr const char* HUNYUAN_BYT5_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/text_encoders/byt5_small_glyphxl_fp16.safetensors";
    static constexpr int64_t HUNYUAN_BYT5_SIZE = 593000000LL;  // ~593MB

    // CLIP Vision for I2V (sigclip for 1.5)
    static constexpr const char* HUNYUAN_CLIP_VISION_URL =
        "https://huggingface.co/Comfy-Org/sigclip_vision_384/resolve/main/sigclip_vision_patch14_384.safetensors";
    static constexpr int64_t HUNYUAN_CLIP_VISION_SIZE = 856000000LL;  // ~856MB

    // Custom Node Git URLs (for HunyuanVideo backend)
    static constexpr const char* NODE_VIDEO_HELPER_SUITE =
        "https://github.com/Kosinkadink/ComfyUI-VideoHelperSuite.git";
    static constexpr const char* NODE_COMFYUI_GGUF =
        "https://github.com/city96/ComfyUI-GGUF.git";
    static constexpr const char* NODE_HUNYUAN_WRAPPER =
        "https://github.com/kijai/ComfyUI-HunyuanVideoWrapper.git";
    static constexpr const char* NODE_COMFYUI_MANAGER =
        "https://github.com/ltdrdata/ComfyUI-Manager.git";
    static constexpr const char* NODE_FRAME_INTERPOLATION =
        "https://github.com/Fannovel16/ComfyUI-Frame-Interpolation.git";
    static constexpr const char* NODE_FLUX_PROMPT_ENHANCER =
        "https://github.com/marduk191/ComfyUI-Fluxpromptenhancer.git";

    // =========================================================================
    // Flux.1 Schnell NF4 (fast image generation, VRAM-efficient)
    // =========================================================================

    // Flux Schnell transformer fp8 (better quality than GGUF)
    static constexpr const char* FLUX_SCHNELL_FP8_URL =
        "https://huggingface.co/Comfy-Org/flux1-schnell/resolve/main/flux1-schnell-fp8.safetensors";
    static constexpr int64_t FLUX_SCHNELL_FP8_SIZE = 11900000000LL;  // ~11.9GB fp8

    // Flux VAE (shared between Schnell and Dev) - using ModelScope mirror (public access)
    static constexpr const char* FLUX_VAE_URL =
        "https://modelscope.cn/models/AI-ModelScope/FLUX.1-schnell/resolve/master/ae.safetensors";
    static constexpr int64_t FLUX_VAE_SIZE = 335000000LL;  // ~335MB

    // CLIP-L text encoder for Flux
    static constexpr const char* FLUX_CLIP_L_URL =
        "https://huggingface.co/comfyanonymous/flux_text_encoders/resolve/main/clip_l.safetensors";
    static constexpr int64_t FLUX_CLIP_L_SIZE = 246000000LL;  // ~246MB

    // T5-XXL text encoder fp8 (good balance of quality and VRAM)
    static constexpr const char* FLUX_T5XXL_FP8_URL =
        "https://huggingface.co/comfyanonymous/flux_text_encoders/resolve/main/t5xxl_fp8_e4m3fn.safetensors";
    static constexpr int64_t FLUX_T5XXL_FP8_SIZE = 4890000000LL;  // ~4.9GB fp8

    // Flux Prompt Enhance model (gokaygokay/Flux-Prompt-Enhance from HuggingFace)
    static constexpr int64_t FLUX_PROMPT_ENHANCE_SIZE = 900000000LL;  // ~900MB

    // === Private Methods ===

    // Installation threads
    void installComfyUIBaseThread(InstallConfig config);
    void installHunyuanVideoThread(InstallConfig config);
    void installFluxSchnellThread(InstallConfig config);
    void installAllThread(InstallConfig config);
    void installMissingComponentsThread(InstallConfig config,
                                         std::vector<ModelComponent> components);

    // Download helpers
    bool downloadFile(const DownloadFile& file);
    bool downloadFileWithResume(const std::string& url, const std::string& localPath,
                                 int64_t expectedSize = 0);
    bool verifyFile(const std::string& path, int64_t expectedSize,
                    const std::string& sha256 = "");

    // Git operations
    bool cloneRepository(const std::string& url, const std::string& targetDir);
    bool pullRepository(const std::string& repoDir);

    // Archive extraction
    bool extract7z(const std::string& archivePath, const std::string& targetDir);
    bool extractZip(const std::string& archivePath, const std::string& targetDir);

    // Utility
    bool createDirectories(const std::string& path);
    bool deleteDirectory(const std::string& path);
    bool fileExists(const std::string& path);
    int64_t getFileSize(const std::string& path);
    int64_t getFreeDiskSpace(const std::string& path);

    // Progress
    void updateProgress(const InstallProgress& newProgress);
    void setError(const std::string& error);

    // Build file lists
    std::vector<DownloadFile> getComfyUIBaseFiles();
    std::vector<DownloadFile> getHunyuanVideoFiles();
    std::vector<std::string> getHunyuanCustomNodes();
    std::vector<DownloadFile> getFluxSchnellFiles();
};

#endif // COMFYUI_INSTALLER_H
