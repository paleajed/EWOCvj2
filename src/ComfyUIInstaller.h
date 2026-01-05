/**
 * ComfyUIInstaller.h
 *
 * Downloads and installs ComfyUI with video generation backends:
 * - StableDiffusion + AnimateDiff (lower VRAM, ~8GB)
 * - HunyuanVideo GGUF (higher quality, ~12GB recommended)
 *
 * All models are optimized for consumer hardware (FP16/GGUF quantized)
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
    SD_ANIMATEDIFF = 1,         // SD1.5 + AnimateDiff stack
    HUNYUAN_VIDEO = 2,          // HunyuanVideo GGUF stack
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
    std::string id;                           // Unique identifier (e.g., "florence2", "hunyuan_t2v")
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
    int downloadTimeout = 600000;             // Download timeout per file (ms)
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
 *   // Install SD+AnimateDiff (smaller, works on 8GB VRAM)
 *   if (installer.installSDAnimateDiff(config)) {
 *       while (installer.isInstalling()) {
 *           // Wait or update UI
 *       }
 *   }
 *
 *   // Later, optionally install HunyuanVideo (needs 12GB+ VRAM)
 *   if (installer.installHunyuanVideo(config)) {
 *       // ...
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
     * Install StableDiffusion + AnimateDiff stack
     * Includes: SD1.5 FP16, AnimateDiff motion modules, IPAdapter, ControlNet
     * VRAM requirement: ~8GB minimum
     * Download size: ~4GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installSDAnimateDiff(const InstallConfig& config);

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
     * Install everything (ComfyUI + SD + Hunyuan)
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
     * Check if SD+AnimateDiff is installed
     * @param installDir Installation directory
     */
    static bool isSDAnimateDiffInstalled(const std::string& installDir);

    /**
     * Check if HunyuanVideo is installed
     * @param installDir Installation directory
     */
    static bool isHunyuanVideoInstalled(const std::string& installDir);

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
     * Get all components for SD+AnimateDiff backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getSDComponents();

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
     * Remove SD+AnimateDiff models (keeps ComfyUI base)
     */
    bool uninstallSDAnimateDiff(const std::string& installDir);

    /**
     * Remove HunyuanVideo models (keeps ComfyUI base)
     */
    bool uninstallHunyuanVideo(const std::string& installDir);

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

    // SD1.5 + AnimateDiff
    static constexpr const char* SD15_FP16_URL =
        "https://huggingface.co/Comfy-Org/stable-diffusion-v1-5-archive/resolve/main/v1-5-pruned-emaonly-fp16.safetensors";
    static constexpr int64_t SD15_FP16_SIZE = 2132649990LL;  // ~2GB

    static constexpr const char* ANIMATEDIFF_MM_V2_FP16_URL =
        "https://huggingface.co/neggles/animatediff-modules/resolve/main/mm_sd_v15_v2.fp16.safetensors";
    static constexpr int64_t ANIMATEDIFF_MM_V2_SIZE = 909000000LL;  // ~909MB

    static constexpr const char* SD_VAE_FP16_URL =
        "https://huggingface.co/stabilityai/sd-vae-ft-mse-original/resolve/main/vae-ft-mse-840000-ema-pruned.safetensors";
    static constexpr int64_t SD_VAE_SIZE = 334700000LL;  // ~335MB

    // ControlNet models
    static constexpr const char* CONTROLNET_DEPTH_URL =
        "https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11f1p_sd15_depth.pth";
    static constexpr int64_t CONTROLNET_DEPTH_SIZE = 1450000000LL;  // ~1.4GB

    static constexpr const char* CONTROLNET_CANNY_URL =
        "https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11p_sd15_canny.pth";
    static constexpr int64_t CONTROLNET_CANNY_SIZE = 1450000000LL;

    // IPAdapter
    static constexpr const char* IPADAPTER_PLUS_URL =
        "https://huggingface.co/h94/IP-Adapter/resolve/main/models/ip-adapter-plus_sd15.safetensors";
    static constexpr int64_t IPADAPTER_PLUS_SIZE = 98000000LL;  // ~98MB

    static constexpr const char* CLIP_VISION_URL =
        "https://huggingface.co/h94/IP-Adapter/resolve/main/models/image_encoder/model.safetensors";
    static constexpr int64_t CLIP_VISION_SIZE = 2530000000LL;  // ~2.5GB

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

    // Custom Node Git URLs
    static constexpr const char* NODE_ANIMATEDIFF_EVOLVED =
        "https://github.com/Kosinkadink/ComfyUI-AnimateDiff-Evolved.git";
    static constexpr const char* NODE_VIDEO_HELPER_SUITE =
        "https://github.com/Kosinkadink/ComfyUI-VideoHelperSuite.git";
    static constexpr const char* NODE_ADVANCED_CONTROLNET =
        "https://github.com/Kosinkadink/ComfyUI-Advanced-ControlNet.git";
    static constexpr const char* NODE_IPADAPTER_PLUS =
        "https://github.com/cubiq/ComfyUI_IPAdapter_plus.git";
    static constexpr const char* NODE_FIZZNODES =
        "https://github.com/FizzleDorf/ComfyUI_FizzNodes.git";
    static constexpr const char* NODE_COMFYUI_GGUF =
        "https://github.com/city96/ComfyUI-GGUF.git";
    static constexpr const char* NODE_HUNYUAN_WRAPPER =
        "https://github.com/kijai/ComfyUI-HunyuanVideoWrapper.git";
    static constexpr const char* NODE_COMFYUI_MANAGER =
        "https://github.com/ltdrdata/ComfyUI-Manager.git";
    static constexpr const char* NODE_FRAME_INTERPOLATION =
        "https://github.com/Fannovel16/ComfyUI-Frame-Interpolation.git";
    static constexpr const char* NODE_FLORENCE2 =
        "https://github.com/kijai/ComfyUI-Florence2.git";

    // Florence-2 model for image captioning (used by HunyuanVideo I2V)
    static constexpr const char* FLORENCE2_LARGE_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/model.safetensors";
    static constexpr int64_t FLORENCE2_LARGE_SIZE = 1553563458LL;  // ~1.55GB
    static constexpr const char* FLORENCE2_CONFIG_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/config.json";
    static constexpr const char* FLORENCE2_TOKENIZER_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/tokenizer.json";
    static constexpr const char* FLORENCE2_TOKENIZER_CONFIG_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/tokenizer_config.json";
    static constexpr const char* FLORENCE2_VOCAB_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/vocab.json";
    static constexpr const char* FLORENCE2_PROCESSOR_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/preprocessor_config.json";
    static constexpr const char* FLORENCE2_GENERATION_CONFIG_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/generation_config.json";
    // Python code files (required for custom model architecture)
    static constexpr const char* FLORENCE2_MODELING_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/modeling_florence2.py";
    static constexpr const char* FLORENCE2_CONFIGURATION_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/configuration_florence2.py";
    static constexpr const char* FLORENCE2_PROCESSING_URL =
        "https://huggingface.co/microsoft/Florence-2-large/resolve/main/processing_florence2.py";

    // === Private Methods ===

    // Installation threads
    void installComfyUIBaseThread(InstallConfig config);
    void installSDAnimateDiffThread(InstallConfig config);
    void installHunyuanVideoThread(InstallConfig config);
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
    std::vector<DownloadFile> getSDAnimateDiffFiles();
    std::vector<DownloadFile> getHunyuanVideoFiles();
    std::vector<std::string> getSDCustomNodes();
    std::vector<std::string> getHunyuanCustomNodes();
};

#endif // COMFYUI_INSTALLER_H
