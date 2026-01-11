/**
 * ComfyUIManager.h
 *
 * Backend integration for ComfyUI video generation
 * Supports HunyuanVideo GGUF backend
 *
 * Communication:
 * - WebSocket for real-time progress updates
 * - HTTP REST for workflow submission and output retrieval
 * - External JSON workflow files for user customization
 *
 * License: GPL3
 */

#ifndef COMFYUI_MANAGER_H
#define COMFYUI_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <unordered_map>
#include "nlohmann/json.hpp"

// ============================================================================
// Enums
// ============================================================================

/**
 * Generation backend selection
 */
enum class GenerationBackend {
    HUNYUAN_VIDEO = 0,     // HunyuanVideo GGUF (video generation)
    FLUX_SCHNELL = 1,      // Flux.1 Schnell NF4 (fast image generation)
    BACKEND_COUNT = 2
};

/**
 * All video generation presets
 */
enum class PresetType {
    // Video presets (Hunyuan)
    TEXT_TO_VIDEO = 0,          // Prompt -> video
    IMAGE_TO_MOTION = 1,        // Still image -> animated video
    STYLE_TRANSFER_LOOP = 2,    // Apply artistic style via IPAdapter
    MORPHING_SEQUENCES = 3,     // Smooth transitions between concepts
    VIDEO_CONTINUATION = 4,     // Continue video from last frame (Hunyuan only)
    CONTROLLABLE_CHARACTER = 5, // Consistent character across clips
    TEXTURE_EVOLUTION = 6,      // Organic material transformations
    BATCH_VARIATION_GENERATOR_T2V = 7,  // Generate multiple T2V variations
    BATCH_VARIATION_GENERATOR_I2V = 8,  // Generate multiple I2V variations
    CONTROLNET_DIRECTOR = 9,            // Guide with sketch/depth/pose
    FRAME_INTERPOLATION = 10,           // Increase FPS using RIFE
    REMIX_EXISTING_CLIP = 11,           // Variation on previous generation

    // Image presets (Flux)
    TEXT_TO_IMAGE = 12,         // Prompt -> image (Flux Schnell)
    IMAGE_TO_IMAGE = 13,        // Image variation/edit (Flux)

    PRESET_COUNT = 14
};

/**
 * ControlNet conditioning types
 */
enum class ControlNetType {
    NONE = 0,
    DEPTH = 1,      // Depth maps
    CANNY = 2,      // Edge detection
    POSE = 3,       // OpenPose
    SKETCH = 4,     // Sketch/lineart
    NORMAL = 5      // Normal maps
};

/**
 * Motion types for Image-to-Motion preset
 */
enum class MotionType {
    ZOOM_IN = 0,
    ZOOM_OUT = 1,
    PAN_LEFT = 2,
    PAN_RIGHT = 3,
    ROTATE_CW = 4,
    ROTATE_CCW = 5,
    DRIFT = 6,
    PULSE = 7
};

/**
 * Style presets for Text-to-Video
 */
enum class StylePreset {
    ABSTRACT = 0,
    GEOMETRIC = 1,
    ORGANIC = 2,
    CINEMATIC = 3,
    RETRO = 4,
    CYBERPUNK = 5
};

// ============================================================================
// Structs
// ============================================================================

/**
 * Preset metadata and compatibility information
 */
struct PresetInfo {
    PresetType type;
    std::string name;
    std::string description;

    // Backend compatibility
    bool supportedBySD = true;
    bool supportedByHunyuan = true;
    bool supportedByFlux = false;       // Flux.1 Schnell support
    bool hunyuanPartialSupport = false;
    std::string hunyuanLimitations;

    // Required inputs
    bool requiresPrompt = true;
    bool requiresImage = false;
    bool requiresVideo = false;
    bool requiresStyle = false;
    bool requiresControlNet = false;
    std::vector<ControlNetType> supportedControlNets;

    // Default parameters (SDXL native resolution)
    int defaultFrames = 16;
    int defaultWidth = 1024;
    int defaultHeight = 1024;
    float defaultFPS = 8.0f;

    // Workflow file name (without extension, relative to backend folder)
    std::string workflowFile;
};

/**
 * Configuration for ComfyUI connection and processing
 */
struct ComfyUIConfig {
    // Connection settings
    std::string host = "127.0.0.1";
    int port = 8188;
    bool useTLS = false;
    std::string apiKey = "";

    // Paths
    std::string workflowsDir = "";   // Directory with workflow JSON files
    std::string outputDir = "";       // Where to save generated videos
    std::string inputDir = "";        // Temp directory for input images

    // Processing settings
    GenerationBackend preferredBackend = GenerationBackend::HUNYUAN_VIDEO;
    bool autoFallback = true;         // Reserved for future backend fallback
    int maxQueueSize = 5;
    int connectionTimeout = 30000;    // ms
    int generationTimeout = 0;        // ms (0 = no timeout, wait indefinitely)

    // GPU settings
    bool useGPU = true;
    int gpuId = 0;

    // Helper methods
    std::string getWebSocketURL() const {
        return (useTLS ? "wss://" : "ws://") + host + ":" + std::to_string(port) + "/ws";
    }

    std::string getHTTPURL() const {
        return (useTLS ? "https://" : "http://") + host + ":" + std::to_string(port);
    }
};

/**
 * Generation request parameters
 * Contains all possible parameters across all presets
 */
struct GenerationParams {
    PresetType preset = PresetType::TEXT_TO_VIDEO;
    GenerationBackend backend = GenerationBackend::HUNYUAN_VIDEO;

    // Core generation parameters
    std::string prompt = "";
    std::string negativePrompt = "blurry, low quality, distorted";
    int seed = -1;                    // -1 = random
    int steps = 20;
    float cfgScale = 7.0f;
    bool promptImprove = false;       // AI prompt enhancement (Flux only)

    // Video parameters (SDXL native resolution)
    int frames = 16;
    int width = 1024;
    int height = 1024;
    float fps = 8.0f;
    bool seamlessLoop = false;        // Seamless looping (TEXT_TO_VIDEO only)

    // Context parameters for long video generation
    int contextLength = 16;           // Frames processed together (8-32)
    int contextOverlap = 6;           // Overlap between windows (1-16)

    // Input media paths
    std::string inputImagePath = "";
    std::string inputVideoPath = "";
    std::string styleImagePath = "";

    // ControlNet settings
    std::string controlNetImagePath = "";
    ControlNetType controlNetType = ControlNetType::NONE;
    float controlNetStrength = 1.0f;

    // Style transfer
    float styleStrength = 0.8f;
    bool preserveColors = false;

    // Remix clip
    float remixStrength = 0.5f;       // Denoise amount (0.0-1.0, lower = more original)

    // Image-to-Motion specific
    MotionType motionType = MotionType::ZOOM_IN;
    float motionStrength = 0.5f;
    float denoiseStrength = 1.0f;     // Hunyuan denoise (inverted in workflow)
    float fluxDenoiseStrength = 0.75f; // Flux denoise (direct, not inverted)

    // Text-to-Video specific
    StylePreset stylePreset = StylePreset::ABSTRACT;
    float motionIntensity = 0.5f;

    // Kaleidoscope specific
    int symmetryFold = 6;             // 2, 4, 6, or 8-fold symmetry
    float rotationSpeed = 0.5f;

    // Morphing sequences
    std::string morphStartPrompt = "";
    std::string morphEndPrompt = "";
    int morphMidpoint = 50;           // Percentage of frames where transition completes

    // Glitch/Databend specific
    float glitchIntensity = 0.5f;
    bool temporalCoherence = true;

    // Multi-layer composite
    std::vector<std::string> layerPrompts;
    std::vector<float> layerOpacities;

    // Character consistency (future implementation)
    std::vector<std::string> referenceImagePaths;
    float identityStrength = 0.8f;

    // Texture evolution
    std::string startTexture = "";    // e.g., "liquid", "crystal"
    std::string endTexture = "";

    // Beat sync
    float bpm = 120.0f;
    bool syncToAudio = false;
    std::string audioPath = "";
    int barLength = 4;                // Pattern changes every N bars

    // Batch generation
    int batchSize = 1;
    int variationSeed = -1;

    // LoRA settings
    std::string loraPath = "";
    float loraStrength = 0.8f;

    // Upscaling
    int upscaleFactor = 2;            // 2x or 4x
    bool frameInterpolation = false;
    int interpolationFactor = 2;      // 2x or 4x frames

    // Video continuation
    bool appendToSource = true;       // Append generated frames to source video
    std::string sourceVideoPath = ""; // Path to source video for continuation

    // Output settings
    std::string outputPath = "";      // If empty, auto-generate
    std::string outputFormat = "mp4";
    int outputQuality = 85;           // 0-100
};

/**
 * Real-time generation progress information
 */
struct GenerationProgress {
    enum class State {
        IDLE = 0,
        CONNECTING = 1,
        QUEUED = 2,
        LOADING_MODEL = 3,
        GENERATING = 4,
        UPSCALING = 5,
        ENCODING = 6,
        DOWNLOADING = 7,
        COMPLETE = 8,
        FAILED = 9,
        CANCELLED = 10
    };

    State state = State::IDLE;
    std::string status = "Idle";

    // Progress tracking
    int currentStep = 0;
    int totalSteps = 0;
    float percentComplete = 0.0f;

    // Frame-based progress
    int currentFrame = 0;
    int totalFrames = 0;

    // Node execution tracking
    std::string currentNode = "";
    int nodesCompleted = 0;
    int totalNodes = 0;

    // Timing
    float elapsedTime = 0.0f;           // seconds
    float estimatedTimeRemaining = 0.0f;

    // Queue info
    int queuePosition = 0;
    int queueSize = 0;

    // Output info
    std::string outputPath = "";
    std::string previewImagePath = "";  // Live preview during generation

    // Error info
    std::string errorMessage = "";
    int errorCode = 0;
};

// ============================================================================
// ComfyUIManager Class
// ============================================================================

/**
 * ComfyUIManager - Backend integration for ComfyUI video generation
 *
 * Usage:
 *   ComfyUIManager manager;
 *   ComfyUIConfig config;
 *   config.host = "127.0.0.1";
 *   config.workflowsDir = "C:/ProgramData/EWOCvj2/comfyui/workflows";
 *
 *   if (manager.initialize(config) && manager.connect()) {
 *       GenerationParams params;
 *       params.preset = PresetType::TEXT_TO_VIDEO;
 *       params.prompt = "A spinning galaxy, cosmic colors";
 *       params.frames = 24;
 *
 *       if (manager.startGeneration(params)) {
 *           while (manager.isGenerating()) {
 *               auto progress = manager.getProgress();
 *               // Update UI with progress
 *           }
 *           std::string output = manager.getLastOutputPath();
 *       }
 *   }
 */
class ComfyUIManager {
public:
    // === Lifecycle ===
    ComfyUIManager();
    ~ComfyUIManager();

    // Prevent copying
    ComfyUIManager(const ComfyUIManager&) = delete;
    ComfyUIManager& operator=(const ComfyUIManager&) = delete;

    // === Initialization ===

    /**
     * Initialize the manager with configuration
     * Loads workflow files and prepares for connection
     * @param config Configuration settings
     * @return true if initialization successful
     */
    bool initialize(const ComfyUIConfig& config = ComfyUIConfig());

    /**
     * Check if manager is initialized
     */
    bool isInitialized() const { return initialized; }

    // === Connection ===

    /**
     * Connect to ComfyUI server
     * Establishes HTTP connection and starts WebSocket thread
     * @return true if connection successful
     */
    bool connect();

    /**
     * Disconnect from ComfyUI server
     * Stops WebSocket thread and cleans up
     */
    void disconnect();

    /**
     * Check if currently connected
     */
    bool isConnected() const { return connected.load(); }

    /**
     * Test connection to ComfyUI server
     * Sends a simple HTTP request to verify server is responding
     * @return true if server responds
     */
    bool testConnection();

    // === Preset Information ===

    /**
     * Get metadata for a specific preset
     * @param preset The preset type
     * @return PresetInfo with name, description, compatibility, etc.
     */
    static const PresetInfo& getPresetInfo(PresetType preset);

    /**
     * Get all presets supported by a specific backend
     * @param backend The generation backend
     * @param includePartial Include presets with partial support
     * @return Vector of preset types
     */
    static std::vector<PresetType> getPresetsForBackend(GenerationBackend backend,
                                                         bool includePartial = true);

    /**
     * Check if a preset is supported by a specific backend
     * @param preset The preset type
     * @param backend The generation backend
     * @return true if fully or partially supported
     */
    static bool isPresetSupported(PresetType preset, GenerationBackend backend);

    /**
     * Get human-readable name for a preset
     */
    static std::string presetToString(PresetType preset);

    /**
     * Get human-readable name for a backend
     */
    static std::string backendToString(GenerationBackend backend);

    // === Generation ===

    /**
     * Start video generation (async)
     * Validates parameters, loads workflow, and submits to ComfyUI
     * @param params Generation parameters
     * @return true if generation started successfully
     */
    bool startGeneration(const GenerationParams& params);

    /**
     * Cancel ongoing generation
     * Sends interrupt signal to ComfyUI
     */
    void cancelGeneration();

    /**
     * Check if generation is in progress
     */
    bool isGenerating() const { return generating.load(); }

    // === Progress ===

    /**
     * Get current generation progress
     * Thread-safe, can be called from UI thread
     * @return Current progress state
     */
    GenerationProgress getProgress() const;

    /**
     * Set progress callback function
     * Called on WebSocket thread when progress updates
     * @param callback Function to call with progress updates
     */
    void setProgressCallback(std::function<void(const GenerationProgress&)> callback);

    // === Queue Management ===

    /**
     * Get current ComfyUI queue length
     * @return Number of items in queue
     */
    int getQueueLength();

    /**
     * Clear the ComfyUI queue
     * @return true if queue cleared successfully
     */
    bool clearQueue();

    // === Workflow Management ===

    /**
     * Load all workflow files from directory
     * @param workflowsDir Directory containing workflow JSON files
     * @return true if workflows loaded successfully
     */
    bool loadWorkflows(const std::string& workflowsDir);

    /**
     * Reload a specific workflow file
     * @param preset The preset to reload
     * @return true if workflow reloaded successfully
     */
    bool reloadWorkflow(PresetType preset);

    /**
     * Get list of available workflow files
     * @return Vector of workflow file paths
     */
    std::vector<std::string> getAvailableWorkflows();

    // === Backend Detection ===

    /**
     * Check if a backend is available in ComfyUI
     * Checks for required models and nodes
     * @param backend The backend to check
     * @return true if backend is available
     */
    bool checkBackendAvailability(GenerationBackend backend);

    /**
     * Get available models for a backend
     * @param backend The generation backend
     * @return Vector of model names
     */
    std::vector<std::string> getAvailableModels(GenerationBackend backend);

    /**
     * Get available LoRAs
     * @return Vector of LoRA file paths
     */
    std::vector<std::string> getAvailableLoRAs();

    /**
     * Get available ControlNet models
     * @return Vector of ControlNet model names
     */
    std::vector<std::string> getAvailableControlNets();

    // === Output ===

    /**
     * Get path to last generated output
     * @return Output file path, empty if no output
     */
    std::string getLastOutputPath() const;

    /**
     * Get recent output history
     * @param count Number of items to retrieve
     * @return Vector of output file paths
     */
    std::vector<std::string> getOutputHistory(int count = 10);

    // === Error Handling ===

    /**
     * Get last error message
     * @return Error string, empty if no error
     */
    std::string getLastError() const;

    /**
     * Clear last error
     */
    void clearError();

    /**
     * Free VRAM by unloading models from GPU
     * Calls ComfyUI /free endpoint to release GPU memory
     */
    void freeVRAM();

    // === Configuration ===

    /**
     * Update configuration
     * May require reconnection if connection settings changed
     * @param config New configuration
     */
    void setConfig(const ComfyUIConfig& config);

    /**
     * Get current configuration
     */
    ComfyUIConfig getConfig() const { return config; }

    /**
     * Get current batch ID (for frame output directory)
     */
    std::string getCurrentBatchId() const { return currentBatchId; }

    /**
     * Get ComfyUI output directory path
     */
    std::string getComfyOutputDir() const;

    /**
     * Get frames directory path for current batch
     * Returns the path where SaveImage stored the frames
     */
    std::string getFramesDirectory() const;

private:
    // === Configuration ===
    ComfyUIConfig config;
    bool initialized = false;
    std::atomic<bool> connected{false};
    std::atomic<bool> generating{false};
    std::atomic<bool> shouldStop{false};

    // === Threading ===
    std::unique_ptr<std::thread> webSocketThread;
    std::unique_ptr<std::thread> generationThread;
    std::atomic<bool> wsThreadRunning{false};

    // === Progress ===
    mutable std::mutex progressMutex;
    GenerationProgress progress;
    std::function<void(const GenerationProgress&)> progressCallback;

    // === Workflow Storage ===
    std::unordered_map<std::string, nlohmann::json> workflowsHunyuan; // HunyuanVideo workflows
    std::string workflowsDir;

    // === Error Handling ===
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Output History ===
    std::vector<std::string> outputHistory;
    mutable std::mutex historyMutex;

    // === Current Job ===
    std::string currentPromptId;
    std::string currentClientId;
    GenerationParams currentParams;
    std::string currentBatchId;  // Unique ID for frame output directory

    // === Private Methods ===

    // WebSocket handling
    void webSocketThreadFunc();
    void handleWebSocketMessage(const std::string& message);
    void parseStatusMessage(const nlohmann::json& msg);
    void parseProgressMessage(const nlohmann::json& msg);
    void parseExecutionMessage(const nlohmann::json& msg);
    void parseExecutedMessage(const nlohmann::json& msg);
    void parseErrorMessage(const nlohmann::json& msg);

    // HTTP API
    std::string httpPost(const std::string& endpoint, const nlohmann::json& data);
    std::string httpPost(const std::string& endpoint, const std::string& data,
                         const std::string& contentType = "application/json");
    std::string httpGet(const std::string& endpoint);
    nlohmann::json submitWorkflow(const nlohmann::json& workflow);
    bool interruptGeneration();
    bool downloadOutput(const std::string& filename, const std::string& subfolder,
                        const std::string& localPath);
    nlohmann::json getHistory(const std::string& promptId);
    nlohmann::json getQueue();
    nlohmann::json getSystemStats();

    // Workflow handling
    bool loadWorkflowFile(const std::string& path, GenerationBackend backend);
    std::string getWorkflowPath(PresetType preset, GenerationBackend backend, bool promptImprove = false);
    nlohmann::json prepareWorkflow(PresetType preset, const GenerationParams& params);
    void substituteParameters(nlohmann::json& workflow, const GenerationParams& params);
    void substituteNode(nlohmann::json& node, const std::string& key,
                        const GenerationParams& params);

    // Generation
    void generationThreadFunc(GenerationParams params);
    bool waitForCompletion(int timeoutMs);
    bool processOutput(const nlohmann::json& historyData);

    // Image upload for input images
    bool uploadImage(const std::string& localPath, std::string& uploadedName);

    // Utility
    void updateProgress(const GenerationProgress& newProgress);
    void setError(const std::string& error);
    std::string generateOutputPath(const GenerationParams& params);
    std::string generateClientId();
    void addToHistory(const std::string& path);

    // Backend-specific workflow modifications
    void applyPresetDefaults(GenerationParams& params);
    void adjustForHunyuan(nlohmann::json& workflow, const GenerationParams& params);
    void addControlNet(nlohmann::json& workflow, const GenerationParams& params);
    void addIPAdapter(nlohmann::json& workflow, const GenerationParams& params);
    void setupSeamlessLoop(nlohmann::json& workflow, bool enable);
    void setupBeatSync(nlohmann::json& workflow, const GenerationParams& params);

    // Static preset registry
    static void initPresetRegistry();
    static std::vector<PresetInfo> presetRegistry;
    static bool registryInitialized;
};

#endif // COMFYUI_MANAGER_H
