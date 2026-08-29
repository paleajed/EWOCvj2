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
    FLUX_KLEIN = 2,             // FLUX.2 Klein 4B Distilled (fast image + style ref generation)
    STYLE_TO_VIDEO = 3,         // Style-to-Video (IP2V) - VLM + FP8 model (~30GB)
    LTX_BF16 = 4,               // LTX 2 High Quality - LTX-2.5 22B dev, BF16 (gated HF download)
    LTX_NVFP4 = 5,              // LTX 2 Fast Blackwell - LTX-2.5 22B distilled, NVFP4
    LTX_GGUF = 6,               // LTX 2 Consumer - LTX-2.5 22B distilled, GGUF Q4_K_M
    COMPONENT_COUNT = 7
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
    std::string statusPrefix = "";    // e.g. "File 1/5: " - preserved during download updates
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
    std::string id;                           // Unique identifier (e.g., "hunyuan_t2v", "flux_klein")
    std::string name;                         // Human-readable name
    std::string description;                  // What this component provides
    std::vector<DownloadFile> files;          // Files to download
    std::vector<std::string> customNodes;     // Git URLs for custom nodes
    std::vector<std::string> checkFiles;      // Files to check for existence (relative paths)
    bool required = true;                     // If false, component is optional
    bool enabled = true;                      // Can be disabled by user preference
};

/**
 * One entry from the online LTX-2.5 LoRA catalog (see fetchLtxLoraCatalog()) - a HuggingFace
 * repo carrying a ComfyUI-compatible LoRA for LTX-2.5 (or LTX-2.3, which mostly loads fine on
 * 2.5 too), with the specific .safetensors file already resolved out of the repo's file list.
 */
struct LoraCatalogEntry {
    std::string repoId;    // e.g. "AhsanHareem/reelbids-ltx25-camera-lora"
    std::string filename;  // resolved .safetensors file within the repo
    std::string license;   // license tag as reported by HuggingFace ("unknown" if untagged)
    std::string baseModel; // "Lightricks/LTX-2.5" or "Lightricks/LTX-2.3"
};

// Conditioning mechanism a LoraModeOverride entry can force for its LoRA. Originally mirrored the
// class_type pairing a runtime-splicing function (applyLtxLora(), now removed) built - the four
// LoRAs that needed non-default wiring are now their own baked presets/workflow JSONs instead
// (LTX_FIRST_FRAME_EDIT/LTX_CHARACTER_RETENTION/LTX_CUTOUT_GUIDES and the still-unregistered
// workflows/ltx_*/camera_warp.json - see PresetType's own comment). This enum and the
// LoraModeOverride table below are kept only because the old per-slot LoRA UI (hidden behind
// kEnableLegacyLoraSlotsUI in videogenroom.cpp, not deleted, in case a future LoRA needs the
// general per-slot mechanism again) still reads them for its "(CAM)"/"(ID)" mode-indicator labels.
enum LoraWiringMode {
    LORA_WIRING_GUIDE = 0,     // Lightricks "Ingredients"-style IC-LoRA guide (the default for
                                // anything not specifically registered below) - now baked as
                                // workflows/ltx_*/cutout_guides.json
    LORA_WIRING_IDENTITY = 1,  // BFS identity-overlap conditioning (LTXIdentityOverlapConditioning)
                                // - now baked as workflows/ltx_*/character_retention.json
    LORA_WIRING_CROSSVIEW = 2, // CrossView-Warp: MoGe depth + CrossViewWarp node feeding two
                                // chained IC-LoRA guides (warp output, then the raw clip) - now
                                // baked as workflows/ltx_*/camera_warp.json (not registered as a
                                // preset yet)
    LORA_WIRING_FIRSTFRAME = 3,  // First Frame All Frames: the CONTENT image (an edited copy of
                                  // the control video's own first frame) is locally prepended as
                                  // frame 0 of a fresh video built from the control video's
                                  // remaining frames (see buildFirstFramePrependedVideo() in
                                  // ComfyUIManager.cpp), then that ONE combined clip feeds a
                                  // single IC-LoRA guide - matching the LoRA's own reference
                                  // workflow (LoadImage+VHS_LoadVideo -> resize -> batch -> slice
                                  // -> one LTXAddVideoICLoRAGuideAdvanced), done locally instead
                                  // of depending on the ComfyUI-side compositing custom nodes
                                  // (ImageResizeKJv2/BatchImagesNode/Frames Slice) that reference
                                  // graph uses. Now baked as workflows/ltx_*/first_frame_all_frames.json.
};

/**
 * Many LTX LoRAs need conditioning wiring specific to how they were trained - not a strength
 * value you can tune your way out of a mismatch on (see the long debugging trail that led here:
 * a keyframe-anchor guide forced onto a LoRA trained on RoPE-tagged reference tokens just
 * produces zero prompt influence or garbled output, regardless of any parameter). This table used
 * to be looked up automatically by a runtime-splicing function (applyLtxLora(), now removed) -
 * the four LoRAs it covered are now their own baked presets/workflow JSONs instead (see
 * LoraWiringMode's own comment). Kept only because the hidden legacy per-slot UI
 * (kEnableLegacyLoraSlotsUI in videogenroom.cpp) still reads it via findLoraModeOverride().
 */
struct LoraModeOverride {
    std::string filename;        // exact .safetensors filename to match (as stored in models/loras/)
    std::string displayName;     // friendly name shown in the LoRA dropdown instead of the raw
                                  // filename (and instead of the "[online] repoId" form when it's
                                  // only in the online catalog so far, not yet downloaded)
    LoraWiringMode mode;         // forced conditioning mechanism for this LoRA
    bool needsContentImage = false;  // true = also show the (currently unused otherwise) CONTENT
                                      // box for this slot, for whichever future LoRA needs a
                                      // second reference image alongside the control image
    bool bypassImgToVideo = false;   // true = neutralize LTXVImgToVideo's hard image-anchor for
                                      // the I2V preset (rewires its consumers to its own pass-
                                      // through positive/negative conditioning and a plain empty
                                      // latent, matching what T2V would have produced) - for
                                      // LoRAs that rely purely on the IC-LoRA guide below for
                                      // image context and whose training the anchor otherwise
                                      // fights. No-op for T2V (no such node) and FLF2V (a
                                      // different keyframe-guide mechanism, LTXVAddGuide).
    float forcedGuideStrength = -1.0f;  // >=0 = override this slot's own guide-strength UI
                                         // slider with a fixed value this LoRA specifically
                                         // needs; -1 (default) = use the slider as normal
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
    bool installFluxKlein = true;             // Install FLUX.2 Klein 4B Distilled models
    bool installStyleToVideo = false;         // Install Style-to-Video (IP2V) - ~30GB extra, requires HunyuanVideo

    // HuggingFace access token, required to download the gated Lightricks/LTX-2.5 files
    // (the official BF16 "dev" transformer and its matching bf16 text encoder). Only used
    // for LTX 2 High Quality - the other two LTX backends use ungated community mirrors.
    std::string hfToken = "";
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
     * Install FLUX.2 Klein 4B Distilled (fast image + style reference generation)
     * Includes: Klein GGUF Q4_K_S, Qwen3 4B text encoder, flux2-vae
     * VRAM requirement: ~6GB minimum, ~10GB recommended
     * Download size: ~11GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installFluxKlein(const InstallConfig& config);

    /**
     * Install Style-to-Video (IP2V) addon
     * Requires HunyuanVideo to be installed first
     * Downloads VLM (~17GB) + FP8 model (~13GB) = ~30GB total
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installStyleToVideo(const InstallConfig& config);

    /**
     * Install LTX 2 High Quality (LTX-2.5 22B "dev" transformer, BF16)
     * Requires a HuggingFace token with access to the gated Lightricks/LTX-2.5 repo
     * (config.hfToken) - the transformer and its matching bf16 text encoder are gated.
     * VRAM requirement: ~32GB+ recommended
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installLtxBF16(const InstallConfig& config);

    /**
     * Install LTX 2 Fast Blackwell (LTX-2.5 22B distilled transformer, NVFP4)
     * Requires an RTX 50-series/B100/B200 GPU (SM >= 10.0) - see detectBlackwellGPU()
     * VRAM requirement: ~16-24GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installLtxNVFP4(const InstallConfig& config);

    /**
     * Install LTX 2 Consumer (LTX-2.5 22B distilled transformer, GGUF Q4_K_M)
     * VRAM requirement: ~13GB
     * @param config Installation configuration
     * @return true if installation started
     */
    bool installLtxGGUF(const InstallConfig& config);

    /**
     * Install a LoRA file (e.g. for use with LTX-2.5 FLF2V) by copying it into ComfyUI's
     * shared models/loras/ folder, where it becomes selectable by name in any LoraLoader/
     * LoraLoaderModelOnly node regardless of which backend is active. Synchronous - local
     * file copies are fast enough not to need the async download-thread machinery used by
     * the other install* methods.
     * @param localFilePath Path to the LoRA file on disk (.safetensors)
     * @param installDir ComfyUI installation directory (same as InstallConfig::installDir)
     * @return true if the file was copied successfully
     */
    bool installLocalLora(const std::string& localFilePath, const std::string& installDir);

    /**
     * Download and install a LoRA file by URL (e.g. a HuggingFace "resolve/main/..." link,
     * such as one returned by fetchLtxLoraCatalog()) into ComfyUI's models/loras/ folder.
     * Async, like the other install* methods - spawns installThread and returns immediately;
     * poll isInstalling()/getProgress() for status.
     * @param url Direct download URL for the LoRA file (.safetensors)
     * @param filename Local filename to save as
     * @param config Installation configuration (uses installDir + retry/timeout settings)
     * @return true if the download started
     */
    bool installLoraFromUrl(const std::string& url, const std::string& filename, const InstallConfig& config);

    /**
     * Check whether a LoRA file with the given filename has already been installed
     */
    static bool isLoraInstalled(const std::string& installDir, const std::string& filename);

    /**
     * List LoRA files already present in ComfyUI's models/loras/ folder, straight off disk.
     * Unlike ComfyUIManager::getAvailableLoRAs() (which asks the running ComfyUI server, and
     * only reflects what the server cached at startup), this reads the filesystem directly -
     * so it works even before ComfyUI is running/connected, and immediately reflects a file
     * that was just installed or downloaded this session.
     * @param installDir ComfyUI installation directory (same as InstallConfig::installDir)
     * @return Sorted list of filenames (not full paths) with a model file extension
     */
    static std::vector<std::string> listInstalledLoras(const std::string& installDir);

    /**
     * Fetch the online catalog of LTX-2.5 (and LTX-2.3, which mostly loads on 2.5 too) LoRAs
     * from the HuggingFace Hub API. Synchronous and does one HTTP request per candidate repo
     * to resolve its actual .safetensors filename, so this can take several seconds - call it
     * from a background thread, not the UI thread.
     * Not filtered by whether ComfyUI's native LoraLoader can actually read a given repo's key
     * naming convention (diffusers/PEFT vs. native) - some untagged repos load fine and some
     * "comfyui"-tagged ones don't, so that self-reported tag isn't trusted as a gate. A bad
     * pick fails cleanly instead: startGeneration()'s disk-existence check plus ComfyUI's own
     * combo validation turn it into a specific error rather than a crash.
     * @param outEntries Populated with the catalog on success
     * @param outError Set to an error message on failure
     * @return true if the catalog was fetched (an empty result is not itself an error)
     */
    static bool fetchLtxLoraCatalog(std::vector<LoraCatalogEntry>& outEntries, std::string& outError);

    // Per-LoRA conditioning/display overrides - see LoraModeOverride above. Returns nullptr if
    // this exact filename has no registered override (the common case - most LoRAs just want
    // the default guide and their own filename as the display name).
    static const LoraModeOverride* findLoraModeOverride(const std::string& filename);

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
     * Check if FLUX.2 Klein 4B Distilled is installed
     * @param installDir Installation directory
     */
    static bool isFluxKleinInstalled(const std::string& installDir);

    /**
     * Check if Style-to-Video (IP2V) is installed
     * @param installDir Installation directory
     */
    static bool isStyleToVideoInstalled(const std::string& installDir);

    /**
     * Check if LTX 2 High Quality (BF16) is installed
     * @param installDir Installation directory
     */
    static bool isLtxBF16Installed(const std::string& installDir);

    /**
     * Check if LTX 2 Fast Blackwell (NVFP4) is installed
     * @param installDir Installation directory
     */
    static bool isLtxNVFP4Installed(const std::string& installDir);

    /**
     * Check if LTX 2 Consumer (GGUF) is installed
     * @param installDir Installation directory
     */
    static bool isLtxGGUFInstalled(const std::string& installDir);

    /**
     * Detect whether a Blackwell-class NVIDIA GPU (RTX 50xx / B100 / B200, SM >= 10.0) is
     * present, required for LTX 2 Fast Blackwell (NVFP4). Runs `nvidia-smi` and parses its
     * reported compute capability; always false on macOS (no NVIDIA GPUs) without spawning
     * a process. Result should be cached by the caller - this shells out each call.
     * @param gpuNameOut Set to the detected GPU's name if found (empty otherwise)
     * @return true if a Blackwell-class GPU was detected
     */
    static bool detectBlackwellGPU(std::string& gpuNameOut);

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
     * Get all components for FLUX.2 Klein 4B Distilled backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getFluxKleinComponents();

    /**
     * Get all components for Style-to-Video (IP2V) backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getStyleToVideoComponents();

    /**
     * Get all components for LTX 2 High Quality (BF16) backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getLtxBF16Components();

    /**
     * Get all components for LTX 2 Fast Blackwell (NVFP4) backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getLtxNVFP4Components();

    /**
     * Get all components for LTX 2 Consumer (GGUF) backend
     * @return Vector of ModelComponent definitions
     */
    static std::vector<ModelComponent> getLtxGGUFComponents();

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
     * Remove FLUX.2 Klein models (keeps ComfyUI base)
     * Also removes old Schnell files if present.
     */
    bool uninstallFluxKlein(const std::string& installDir);

    /**
     * Remove LTX 2 High Quality (BF16) models (keeps ComfyUI base and shared LTX assets)
     */
    bool uninstallLtxBF16(const std::string& installDir);

    /**
     * Remove LTX 2 Fast Blackwell (NVFP4) models (keeps ComfyUI base and shared LTX assets)
     */
    bool uninstallLtxNVFP4(const std::string& installDir);

    /**
     * Remove LTX 2 Consumer (GGUF) models (keeps ComfyUI base and shared LTX assets)
     */
    bool uninstallLtxGGUF(const std::string& installDir);

    /**
     * Remove everything
     */
    bool uninstallAll(const std::string& installDir);

    // === Prerequisites ===

    /**
     * Check if all prerequisites are installed (Git + Python 3.12 on Windows)
     */
    static bool checkPrerequisites();

    /**
     * Check if Git is installed
     */
    static bool isGitInstalled();

    /**
     * Check if Python 3.12 is installed
     * @param pythonPath Output: path to python executable if found
     * @return true if Python 3.12.x is installed
     */
    static bool isPython312Installed(std::string& pythonPath);

    /**
     * Install missing prerequisites automatically
     * Downloads and installs Git and Python 3.12 (Windows) if not present
     * @param config Installation configuration (uses tempDir for downloads)
     * @return true if all prerequisites are now available
     */
    bool installPrerequisites(const InstallConfig& config);

    /**
     * Install Git silently
     */
    bool installGit(const std::string& tempDir);

    /**
     * Install Python 3.12 silently
     * Windows: downloads python-3.12.8-amd64.exe; Linux: downloads python-build-standalone tarball
     */
    bool installPython312(const std::string& tempDir);

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

    // Process-wide cap on concurrent parallel-chunk HTTP connections, shared across every
    // ComfyUIInstaller instance - separate LoRA/model slots each run their own installer and can
    // download at the same time (see ComfyUIManager's per-slot installer usage), so this can't be
    // a per-instance count. downloadFileParallel()'s worker pool claims one slot per connection it
    // opens and releases it when that connection's worker exits; a claim attempt that finds the
    // budget already exhausted just doesn't grow further (or, for the very first connection,
    // declines parallel downloading entirely and lets the caller fall back to single-stream, which
    // isn't budget-tracked but is just one connection). This is a safety backstop more than a
    // tuning target - downloadFileParallel()'s own throughput-plateau check normally stops
    // growing a single download's pool well before this many connections are open.
    static std::atomic<int> sGlobalActiveConnections;
    static constexpr int kGlobalMaxConnections = 24;
    static bool tryClaimGlobalConnectionSlot();
    static void releaseGlobalConnectionSlot();

    // === Download URLs ===

    // Prerequisites (Git for Windows)
    static constexpr const char* GIT_URL =
        "https://github.com/git-for-windows/git/releases/download/v2.47.1.windows.1/Git-2.47.1-64-bit.exe";
    static constexpr int64_t GIT_SIZE = 65000000LL;  // ~65MB

    // Python 3.12 installer (Windows)
    static constexpr const char* PYTHON_312_URL =
        "https://www.python.org/ftp/python/3.12.8/python-3.12.8-amd64.exe";
    static constexpr int64_t PYTHON_312_SIZE = 25500000LL;  // ~25MB

    // Python 3.12 standalone for Linux (python-build-standalone, self-contained tarball)
    // Extracts to <installDir>/bin/python3.12 with --strip-components=1
    static constexpr const char* PYTHON_LINUX_URL =
        "https://github.com/astral-sh/python-build-standalone/releases/download/20250106/cpython-3.12.8+20250106-x86_64_v2-unknown-linux-gnu-install_only.tar.gz";
    static constexpr int64_t PYTHON_LINUX_SIZE = 61119422LL;  // exact size of the release-20250106 asset

    // Python 3.12 standalone for macOS (python-build-standalone, self-contained tarball)
    static constexpr const char* PYTHON_MACOS_ARM64_URL =
        "https://github.com/astral-sh/python-build-standalone/releases/download/20250106/cpython-3.12.8+20250106-aarch64-apple-darwin-install_only.tar.gz";
    static constexpr const char* PYTHON_MACOS_X86_64_URL =
        "https://github.com/astral-sh/python-build-standalone/releases/download/20250106/cpython-3.12.8+20250106-x86_64-apple-darwin-install_only.tar.gz";
    // Sizes differ per architecture — downloadFileWithResume/verifyFile do an
    // exact byte-count match, so a single shared constant can't be right for
    // both (this is what silently broke the macOS download before).
    static constexpr int64_t PYTHON_MACOS_ARM64_SIZE = 15676873LL;
    static constexpr int64_t PYTHON_MACOS_X86_64_SIZE = 15859636LL;

    // Static git binary for Linux (fallback when git not in system PATH)
    static constexpr const char* GIT_LINUX_URL =
        "https://raw.githubusercontent.com/andrew-d/static-binaries/master/binaries/linux/x86-64/git";
    static constexpr int64_t GIT_LINUX_SIZE = 11000000LL;  // ~11MB

    // HunyuanVideo 1.5 GGUF (VRAM-friendly quantized models)
    static constexpr const char* HUNYUAN_T2V_Q4_URL =
        "https://huggingface.co/jayn7/HunyuanVideo-1.5_T2V_720p-GGUF/resolve/main/720p/hunyuanvideo1.5_720p_t2v-Q4_K_M.gguf";
    static constexpr int64_t HUNYUAN_T2V_Q4_SIZE = 5090407648LL;  // ~5.09GB

    static constexpr const char* HUNYUAN_I2V_Q4_URL =
        "https://huggingface.co/jayn7/HunyuanVideo-1.5_I2V_720p-GGUF/resolve/main/720p/hunyuanvideo1.5_720p_i2v-Q4_K_M.gguf";
    static constexpr int64_t HUNYUAN_I2V_Q4_SIZE = 5090407648LL;  // ~5.09GB

    // HunyuanVideo 1.5 VAE
    static constexpr const char* HUNYUAN_VAE_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/vae/hunyuanvideo15_vae_fp16.safetensors";
    static constexpr int64_t HUNYUAN_VAE_SIZE = 2521292758LL;  // ~2.5GB

    // HunyuanVideo 1.5 CLIP text encoders (qwen 2.5 + byt5)
    static constexpr const char* HUNYUAN_QWEN_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/text_encoders/qwen_2.5_vl_7b_fp8_scaled.safetensors";
    static constexpr int64_t HUNYUAN_QWEN_SIZE = 9384670680LL;  // ~9.4GB

    static constexpr const char* HUNYUAN_BYT5_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/text_encoders/byt5_small_glyphxl_fp16.safetensors";
    static constexpr int64_t HUNYUAN_BYT5_SIZE = 438643184LL;  // ~438MB

    // CLIP Vision for I2V (sigclip for 1.5)
    static constexpr const char* HUNYUAN_CLIP_VISION_URL =
        "https://huggingface.co/Comfy-Org/sigclip_vision_384/resolve/main/sigclip_vision_patch14_384.safetensors";
    static constexpr int64_t HUNYUAN_CLIP_VISION_SIZE = 856505640LL;  // ~856MB

    // Llava VLM for IP2V (Style to Video / Hunyuan Full) - uses image as style reference
    static constexpr const char* LLAVA_VLM_MODEL1_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/model-00001-of-00004.safetensors";
    static constexpr int64_t LLAVA_VLM_MODEL1_SIZE = 4997088760LL;  // ~4.65GB

    static constexpr const char* LLAVA_VLM_MODEL2_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/model-00002-of-00004.safetensors";
    static constexpr int64_t LLAVA_VLM_MODEL2_SIZE = 4915917552LL;  // ~4.58GB

    static constexpr const char* LLAVA_VLM_MODEL3_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/model-00003-of-00004.safetensors";
    static constexpr int64_t LLAVA_VLM_MODEL3_SIZE = 4999820824LL;  // ~4.66GB

    static constexpr const char* LLAVA_VLM_MODEL4_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/model-00004-of-00004.safetensors";
    static constexpr int64_t LLAVA_VLM_MODEL4_SIZE = 1839769624LL;  // ~1.71GB

    static constexpr const char* LLAVA_VLM_CONFIG_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/config.json";
    static constexpr const char* LLAVA_VLM_INDEX_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/model.safetensors.index.json";
    static constexpr const char* LLAVA_VLM_TOKENIZER_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/tokenizer.json";
    static constexpr const char* LLAVA_VLM_TOKENIZER_CONFIG_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/tokenizer_config.json";
    static constexpr const char* LLAVA_VLM_SPECIAL_TOKENS_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/special_tokens_map.json";
    static constexpr const char* LLAVA_VLM_PREPROCESSOR_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/preprocessor_config.json";
    static constexpr const char* LLAVA_VLM_GENERATION_URL =
        "https://huggingface.co/xtuner/llava-llama-3-8b-v1_1-transformers/resolve/main/generation_config.json";

    // HunyuanVideo 1.5 720p T2V FP16 model for Style-to-Video (quantized to FP8 on load)
    static constexpr const char* HUNYUAN_FP16_T2V_URL =
        "https://huggingface.co/Comfy-Org/HunyuanVideo_1.5_repackaged/resolve/main/split_files/diffusion_models/hunyuanvideo1.5_720p_t2v_fp16.safetensors";
    static constexpr int64_t HUNYUAN_FP16_T2V_SIZE = 16653368128LL;  // ~15.5GB

    // Custom Node Git URLs (for HunyuanVideo backend)
    static constexpr const char* NODE_VIDEO_HELPER_SUITE =
        "https://github.com/Kosinkadink/ComfyUI-VideoHelperSuite.git";
    static constexpr const char* NODE_COMFYUI_GGUF =
        "https://github.com/city96/ComfyUI-GGUF.git";
    static constexpr const char* NODE_HUNYUAN_WRAPPER =
        "https://github.com/kijai/ComfyUI-HunyuanVideoWrapper.git";
    static constexpr const char* NODE_HUNYUAN_IP2V =
        "https://github.com/Dango233/ComfyUI-HunyuanVideoWrapper-IP2V.git";
    static constexpr const char* NODE_COMFYUI_MANAGER =
        "https://github.com/ltdrdata/ComfyUI-Manager.git";
    static constexpr const char* NODE_FRAME_INTERPOLATION =
        "https://github.com/Fannovel16/ComfyUI-Frame-Interpolation.git";
    static constexpr const char* NODE_COMFYUI_LLM =
        "https://github.com/Big-Idea-Technology/ComfyUI_LLM_Node.git";
    static constexpr const char* NODE_REFERENCE_LATENT_PLUS =
        "https://github.com/shootthesound/comfyui-ReferenceLatentPlus.git";

    // Qwen2.5-1.5B-Instruct for concept-to-prompt translation (~3GB quantized)
    static constexpr const char* QWEN_1_5B_CONFIG_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/config.json";
    static constexpr const char* QWEN_1_5B_TOKENIZER_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/tokenizer.json";
    static constexpr const char* QWEN_1_5B_TOKENIZER_CONFIG_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/tokenizer_config.json";
    static constexpr const char* QWEN_1_5B_VOCAB_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/vocab.json";
    static constexpr const char* QWEN_1_5B_MERGES_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/merges.txt";
    static constexpr const char* QWEN_1_5B_GENERATION_CONFIG_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/generation_config.json";
    static constexpr const char* QWEN_1_5B_MODEL_URL =
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct/resolve/main/model.safetensors";
    static constexpr int64_t QWEN_1_5B_MODEL_SIZE = 3086839064LL;  // ~3.1GB

    // =========================================================================
    // FLUX.2 Klein 4B Distilled (fast image + style reference generation)
    // =========================================================================

    // FLUX.2 Klein transformer GGUF Q4_K_S (VRAM-efficient quantized model, ~2.58GB)
    static constexpr const char* FLUX_KLEIN_GGUF_URL =
        "https://huggingface.co/unsloth/FLUX.2-klein-4B-GGUF/resolve/main/flux-2-klein-4b-Q4_K_S.gguf";
    static constexpr int64_t FLUX_KLEIN_GGUF_SIZE = 2583077440LL;  // 2,583,077,440 bytes

    // Qwen3 4B text encoder for FLUX.2 Klein
    static constexpr const char* FLUX_KLEIN_QWEN_URL =
        "https://huggingface.co/Comfy-Org/vae-text-encorder-for-flux-klein-4b/resolve/main/split_files/text_encoders/qwen_3_4b.safetensors";
    static constexpr int64_t FLUX_KLEIN_QWEN_SIZE = 8044982048LL;  // 8,044,982,048 bytes

    // FLUX.2 VAE
    static constexpr const char* FLUX_KLEIN_VAE_URL =
        "https://huggingface.co/Comfy-Org/vae-text-encorder-for-flux-klein-4b/resolve/main/split_files/vae/flux2-vae.safetensors";
    static constexpr int64_t FLUX_KLEIN_VAE_SIZE = 336211292LL;  // 336,211,292 bytes

    // =========================================================================
    // LTX-2.5 (three quantizations of the same 22B joint audio-video DiT model)
    // =========================================================================

    // Shared across all three LTX backends (community-mirrored, ungated): the VAE only -
    // the transformer AND text encoder both differ by tier now (see below).
    static constexpr const char* LTX_VAE_URL =
        "https://huggingface.co/vonkaiser/LTX-2.5-FP8-NVFP4/resolve/main/vae/ltx-2.5-video-vae-bf16.safetensors";
    static constexpr int64_t LTX_VAE_SIZE = 1472223346LL;

    // Text encoder for LTX 2 High Quality: plain bf16, ~24GB. Community "torchao"-quantized
    // re-packagings (e.g. gemma4-12b-with-proj-nvfp4-torchao.safetensors) store weights in a
    // bit-packed format plain CLIPLoader can't deserialize (confirmed via a real state_dict
    // size-mismatch error - checkpoint tensors come out exactly half-width), so this plain
    // bf16 file is what Lightricks' own reference workflow uses. Reserved for the "High
    // Quality" tier, which already documents needing 32GB+ VRAM - the other two tiers use
    // the int8-convrot encoder below instead, since a 24GB bf16 text encoder alone would
    // blow past what "Fast Blackwell"/"Consumer" are supposed to fit in.
    static constexpr const char* LTX_CLIP_BF16_URL =
        "https://huggingface.co/Lightricks/LTX-2.5/resolve/main/text_encoders/gemma4-12b-with-proj-ltx-2.5-bf16.safetensors";
    // Exact size unknown (gated repo, can't HEAD without a token) - verifyFile falls back to
    // "exists and non-empty" when expectedSize is 0.
    static constexpr int64_t LTX_CLIP_BF16_SIZE = 0LL;

    // Text encoder for LTX 2 Fast Blackwell + LTX 2 Consumer: official Lightricks int8-convrot
    // quantized file (~12GB, half the bf16 size) - this is ComfyUI's own native "convrot"
    // quantization scheme (confirmed present as comfy_kitchen backend capabilities
    // dequantize_int8_convrot_weight/int8_linear in a real ComfyUI startup log), not a
    // third-party format, so it should load through plain CLIPLoader without needing a
    // patched/custom node the way the broken community "torchao" file did.
    static constexpr const char* LTX_CLIP_INT8CONVROT_URL =
        "https://huggingface.co/Lightricks/LTX-2.5/resolve/main/text_encoders/gemma4-12b-with-proj-ltx-2.5-comfy-int8-convrot.safetensors";
    static constexpr int64_t LTX_CLIP_INT8CONVROT_SIZE = 0LL;

    // LTX 2 High Quality (LTX-2.5 22B "dev" transformer, BF16) - official Lightricks repo, gated
    static constexpr const char* LTX_BF16_UNET_URL =
        "https://huggingface.co/Lightricks/LTX-2.5/resolve/main/diffusion_models/ltx-2.5-22b-dev-transformer-bf16.safetensors";
    static constexpr int64_t LTX_BF16_UNET_SIZE = 0LL;

    // LTX 2 Fast Blackwell (LTX-2.5 22B distilled transformer, NVFP4) - community mirror, ungated.
    // The vonkaiser mirror's NVFP4 tensors are missing a per-tensor .comfy_quant marker, so
    // ComfyUI silently loads them as raw packed weights instead of dequantizing them - this
    // doesn't fail at load time, only later during sampling, as a "mat1 and mat2 shapes cannot
    // be multiplied" error the first time a non-square attention projection (the audio<->video
    // cross-attention blocks) hits an affected layer. rockerBOO's re-tagged file fixes this by
    // adding the missing marker to all 160 affected tensors - confirmed against a real error
    // log (RTX 5090, block 42 audio_to_video_attn.to_q). Local filename intentionally differs
    // from the old vonkaiser one (see getLtxNVFP4Components()) so a stale already-downloaded
    // vonkaiser file is never "resumed" from - that would silently append fresh bytes from this
    // unrelated file onto old content instead of triggering a clean re-download.
    static constexpr const char* LTX_NVFP4_UNET_URL =
        "https://huggingface.co/rockerBOO/ltx-2.5-nvfp4-convrot/resolve/main/ltx-2.5-22b-distilled-transformer_nvfp4_convrot_int8.safetensors";
    static constexpr int64_t LTX_NVFP4_UNET_SIZE = 22116870688LL;

    // LTX 2 Consumer (LTX-2.5 22B distilled transformer, GGUF Q4_K_M) - community mirror, ungated
    static constexpr const char* LTX_GGUF_UNET_URL =
        "https://huggingface.co/realrebelai/LTX-2.5_GGUFs/resolve/main/LTX-2.5-Distilled-Q4_K_M.gguf";
    static constexpr int64_t LTX_GGUF_UNET_SIZE = 15086587904LL;

    // === Private Methods ===

    // Installation threads
    void installComfyUIBaseThread(InstallConfig config);
    void installHunyuanVideoThread(InstallConfig config);
    void installFluxKleinThread(InstallConfig config);
    void installStyleToVideoThread(InstallConfig config);
    void installLtxBF16Thread(InstallConfig config);
    void installLtxNVFP4Thread(InstallConfig config);
    void installLtxGGUFThread(InstallConfig config);
    void installLoraFromUrlThread(std::string url, std::string filename, InstallConfig config);
    // Shared download loop for the three LTX install threads (no custom nodes/pip needed)
    bool downloadLtxComponents(const InstallConfig& config, const std::vector<ModelComponent>& components,
                                const std::string& backendLabel, InstallProgress& prog);
    // Called automatically from downloadLtxComponents() before any LTX model download:
    // git-pulls the ComfyUI core checkout and re-syncs its pip requirements if the
    // installed version is older than LTX25_MIN_COMFYUI_VERSION. No-op if already current.
    bool updateComfyUICoreForLtx25(const InstallConfig& config, InstallProgress& prog);
    void installAllThread(InstallConfig config);
    void installMissingComponentsThread(InstallConfig config,
                                         std::vector<ModelComponent> components);

    // Download helpers
    bool downloadFile(const DownloadFile& file);
    bool downloadFileWithResume(const std::string& url, const std::string& localPath,
                                 int64_t expectedSize = 0);
    // Parallel-chunk downloader tried first by downloadFileWithResume() for a fresh download
    // (nothing already on disk to resume) - splits the file into small fixed-size pieces pulled
    // off a shared queue by a pool of worker connections, since a single CDN connection is
    // commonly capped well below the link's actual capacity (same idea as aria2/hf_xet). The pool
    // starts small and grows while aggregate throughput keeps meaningfully improving, backing off
    // once it plateaus - auto-tuned to whatever this download is actually getting, rather than a
    // fixed connection count guessed up front. Every connection it opens (across every
    // ComfyUIInstaller instance - see sGlobalActiveConnections) shares one process-wide budget, so
    // several model/LoRA downloads auto-tuning at the same time can't collectively open an
    // unbounded number of connections. Returns false with no partial file left behind - and
    // downloadFileWithResume() falls back to its normal single-stream path - when the server
    // doesn't advertise Range support, the file's too small to be worth splitting, no connection
    // budget is currently available, or any piece fails/is cancelled.
    bool downloadFileParallel(const std::string& url, const std::string& localPath, int64_t expectedSize);
    // One piece of downloadFileParallel(): GETs bytes [rangeStart, rangeEnd] of url and writes
    // them into localPath at that byte offset. localPath must already exist and be at least
    // rangeEnd+1 bytes long. Safe to call concurrently for disjoint ranges of the same file -
    // each call opens its own connection and file handle. totalDownloaded is bumped as bytes
    // arrive so the caller can report aggregate progress across all chunks.
    bool downloadFileRangeChunk(const std::string& url, const std::string& localPath,
                                 int64_t rangeStart, int64_t rangeEnd,
                                 std::atomic<int64_t>& totalDownloaded);
    bool verifyFile(const std::string& path, int64_t expectedSize,
                    const std::string& sha256 = "");

    // Small-payload HTTP GET (arbitrary host, not just ComfyUI's local server) - for JSON API
    // calls like the HuggingFace Hub catalog lookups, as opposed to downloadFileWithResume's
    // big-file-to-disk streaming. No auth/resume/progress - a plain synchronous fetch.
    static bool fetchUrlText(const std::string& url, std::string& outBody, std::string& outError);

    // Git operations
    std::string findGitExecutable();
    bool cloneRepository(const std::string& url, const std::string& targetDir);
    bool pullRepository(const std::string& repoDir);

    // Progress-reporting git clone (parses git stderr output)
    bool cloneRepositoryWithProgress(const std::string& url, const std::string& targetDir,
                                      InstallProgress& prog, const std::string& label);

    // Progress-reporting pip install (parses pip stdout output)
    bool runPipWithProgress(const std::string& pythonExe, const std::string& args,
                             InstallProgress& prog, const std::string& label,
                             const std::string& envPrefix = "");

    // Archive extraction
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
    std::vector<DownloadFile> getFluxKleinFiles();
    std::vector<DownloadFile> getStyleToVideoFiles();

    // Shared by all three LTX-2.5 backends (identical VAE regardless of transformer quantization)
    static ModelComponent getLtxSharedVaeComponent();
    // Text encoder for LTX 2 High Quality only (bf16, gated on HuggingFace)
    static ModelComponent getLtxClipBF16Component();
    // Text encoder shared by LTX 2 Fast Blackwell + LTX 2 Consumer (int8-convrot, gated on HuggingFace)
    static ModelComponent getLtxClipInt8ConvrotComponent();
    // Shared by all three LTX-2.5 backends - Lightricks' ComfyUI-LTXVideo custom nodes, needed
    // for proper attention-based IC-LoRA conditioning (not bundled with core ComfyUI)
    static ModelComponent getLtxIcLoraCustomNodesComponent();
    // Shared by all three LTX-2.5 backends - alisson-anjos/ComfyUI-BFSNodes, needed for
    // identity/Face-ID-style LoRAs via LTXIdentityOverlapConditioning
    static ModelComponent getLtxBfsIdentityCustomNodesComponent();
    // Shared by all three LTX-2.5 backends - cseti007/ComfyUI-CrossViewWarp, needed for the
    // "Camera Warp" LoRA's CrossViewWarp node (depth-warped reprojection driving its IC-LoRA
    // guide). Depends on ComfyUI-LTXVideo (getLtxIcLoraCustomNodesComponent(), already installed
    // alongside it) for LTXAddVideoICLoRAGuide, and on VideoHelperSuite (already part of the base
    // ComfyUI install) for VHS_LoadVideo. MoGe itself is a core ComfyUI node, no install needed.
    static ModelComponent getLtxCrossViewWarpCustomNodesComponent();
};

#endif // COMFYUI_INSTALLER_H
