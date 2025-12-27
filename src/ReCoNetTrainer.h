/**
 * ReCoNetTrainer.h
 *
 * ReCoNet (Real-time Collection Network) Style Transfer Trainer
 * Trains custom AI style effects using PyTorch, exports to ONNX
 *
 * License: GPL3
 */

#ifndef RECONET_TRAINER_H
#define RECONET_TRAINER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include "SDL2/SDL.h"
#else
#include <SDL2/SDL.h>
#endif

// Forward declarations
class StylePreparationBin;

/**
 * ReCoNet Style Transfer Trainer
 *
 * Orchestrates training of neural style transfer models:
 * - Preprocesses training images using OpenGL lanczos upscaling
 * - Launches Python subprocess for PyTorch training
 * - Exports trained models to ONNX format
 * - Provides progress monitoring and error handling
 *
 * User implements UI, this class provides backend API
 */
class ReCoNetTrainer {
public:
    /**
     * Quality presets for training (resolution and iterations)
     */
    enum class Quality {
        FAST,
        BALANCED_256,
        BALANCED_512,
        HIGH_256,
        HIGH_512
    };

    /**
     * Style influence presets (content/style/temporal balance)
     * Calibrated for [-1, 1] image range with VGG16 perceptual loss
     */
    enum class StyleInfluence {
        MINIMAL,   // Subtle stylization, preserves content structure
        BALANCED,  // Good balance for most artistic styles
        STRONG     // Bold artistic style with pronounced brush strokes
    };

    /**
     * Abstraction level presets (VGG layer emphasis)
     * Controls which visual features are captured from the style image
     */
    enum class AbstractionLevel {
        LOW,       // Fine details: edges, colors, small textures
        MEDIUM,    // Balanced: textures and medium-scale patterns
        HIGH       // Abstract: broad strokes, mood, semantic structure
    };

    /**
     * Training configuration
     */
    struct Config {
        Quality quality = Quality::BALANCED_256;
        StyleInfluence styleInfluence = StyleInfluence::BALANCED;
        AbstractionLevel abstractionLevel = AbstractionLevel::MEDIUM;
        float learningRate = 1e-3f;

        // Loss weights - set automatically by styleInfluence preset, or manually
        // Calibrated for [-1, 1] image range
        float contentWeight = 1.0f;
        float styleWeight = 1e6f;     // BALANCED preset default
        float tvWeight = 2.0f;        // Total variation for smoothness
        bool useGPU = true;
        std::string contentDataset;   // Path to content images folder (COCO, ImageNet, etc.)

        // Custom training parameters (used when quality == CUSTOM)
        int resolution = 256;         // Training resolution (256, 512, etc.)
        int iterations = 20000;       // Number of training iterations
        int batchSize = 4;            // Images per training batch (lower = less VRAM)

        // Inspiration image preprocessing: smallest side will be scaled to this value
        // This ensures uniform style element size regardless of source image resolution
        int inspirationResolution = 256;

        // Temporal coherence settings (for flicker-free video style transfer)
        // NOTE: Start with 0 to verify style works, then add temporal loss
        float temporalWeight = 0.0f;  // Weight for temporal consistency (0 = disabled)
        std::string videoDataset;     // Path to video files or frame folders (comma-separated)
        int sequenceLength = 2;       // Number of consecutive frames per training sample

        // Advanced: Per-layer style weights (VGG19 layers)
        // Higher layers = broader patterns, lower layers = finer details
        // Default values optimized for brush strokes (relu3_1 and relu4_1 are key)
        float styleWeightRelu1 = 0.1f;   // relu1_1: Fine edges
        float styleWeightRelu2 = 0.5f;   // relu2_1: Small textures
        float styleWeightRelu3 = 2.0f;   // relu3_1: Medium strokes (KEY)
        float styleWeightRelu4 = 3.0f;   // relu4_1: Brush strokes (KEY)
        float styleWeightRelu5 = 3.0f;   // relu5_1: Abstract mood

        // Apply style influence preset to weights
        void applyStyleInfluence() {
            switch (styleInfluence) {
                case StyleInfluence::MINIMAL:
                    // Subtle stylization - preserves more content
                    contentWeight = 4.0f;
                    styleWeight = 2e6f;
                    temporalWeight = (temporalWeight > 0) ? 1e4f : 0.0f;
                    tvWeight = 2.0f;
                    break;
                case StyleInfluence::BALANCED:
                    // Good for most use cases
                    contentWeight = 3.0f;
                    styleWeight = 4e6f;
                    temporalWeight = (temporalWeight > 0) ? 1e4f : 0.0f;
                    tvWeight = 2.0f;
                    break;
                case StyleInfluence::STRONG:
                    // Bold artistic style with pronounced strokes
                    contentWeight = 2.0f;
                    styleWeight = 6e6f;
                    temporalWeight = (temporalWeight > 0) ? 1e4f : 0.0f;
                    tvWeight = 2.0f;
                    break;
            }
        }

        // Apply abstraction level preset to VGG layer weights
        void applyAbstractionLevel() {
            switch (abstractionLevel) {
                case AbstractionLevel::LOW:
                    // Fine details: edges, colors, small textures
                    styleWeightRelu1 = 2.0f;   // Fine edges - HIGH
                    styleWeightRelu2 = 2.0f;   // Small textures - HIGH
                    styleWeightRelu3 = 1.0f;   // Medium strokes - medium
                    styleWeightRelu4 = 0.4f;   // Brush strokes - low
                    styleWeightRelu5 = 0.2f;   // Abstract mood - minimal
                    break;
                case AbstractionLevel::MEDIUM:
                    // Balanced: textures and medium-scale patterns
                    styleWeightRelu1 = 0.4f;   // Fine edges - low
                    styleWeightRelu2 = 1.0f;   // Small textures - medium
                    styleWeightRelu3 = 2.0f;   // Medium strokes - HIGH
                    styleWeightRelu4 = 2.0f;   // Brush strokes - HIGH
                    styleWeightRelu5 = 1.0f;   // Abstract mood - medium
                    break;
                case AbstractionLevel::HIGH:
                    // Abstract: broad strokes, mood, semantic structure
                    styleWeightRelu1 = 0.2f;   // Fine edges - minimal
                    styleWeightRelu2 = 0.4f;   // Small textures - low
                    styleWeightRelu3 = 1.0f;   // Medium strokes - medium
                    styleWeightRelu4 = 2.0f;   // Brush strokes - HIGH
                    styleWeightRelu5 = 2.0f;   // Abstract mood - HIGH
                    break;
            }
        }

        // Helper to get resolution from quality preset
        int getResolution() const {
            switch (quality) {
                case Quality::FAST: return 256;
                case Quality::BALANCED_256: return 256;
                case Quality::BALANCED_512: return 512;
                case Quality::HIGH_256: return 256;
                case Quality::HIGH_512: return 512;
                default: return 256;
            }
        }

        // Helper to get batch size from quality preset
        int getBatchSize() const {
            switch (quality) {
                case Quality::FAST: return 4;
                case Quality::BALANCED_256: return 4;
                case Quality::BALANCED_512: return 4;
                case Quality::HIGH_256: return 4;
                case Quality::HIGH_512: return 2;
                default: return 4;
            }
        }

        // Helper to get iteration count from quality preset
        int getIterations() const {
            switch (quality) {
                case Quality::FAST: return 10000;
                case Quality::BALANCED_256: return 20000;
                case Quality::BALANCED_512: return 20000;
                case Quality::HIGH_256: return 40000;
                case Quality::HIGH_512: return 40000;
                default: return 20000;
            }
        }
    };

    /**
     * Training progress information
     */
    struct Progress {
        int currentIteration = 0;
        int totalIterations = 0;
        float contentLoss = 0.0f;
        float styleLoss = 0.0f;
        float totalLoss = 0.0f;
        std::string status = "Idle";
        float estimatedTimeRemaining = 0.0f; // seconds
    };

    /**
     * Constructor
     */
    ReCoNetTrainer();

    /**
     * Destructor
     */
    ~ReCoNetTrainer();

    /**
     * Initialize trainer
     * Checks for Python, PyTorch, creates directories
     * @return true if initialized successfully
     */
    bool initialize();

    /**
     * Start training on images from StylePreparationBin
     * Spawns background thread for preprocessing and training
     *
     * @param bin Source of training images (up to 16 elements)
     * @param modelName Output model name (no extension, alphanumeric + underscore)
     * @param config Training configuration
     * @return true if training started successfully
     */
    bool startTraining(StylePreparationBin* bin,
                       const std::string& modelName,
                       const Config& config);

    /**
     * Stop ongoing training
     * Signals training thread to terminate gracefully
     */
    void stopTraining();

    /**
     * Check if training is currently in progress
     * @return true if training thread is active
     */
    bool isTraining() const {
        return training.load();
    }

    /**
     * Get current training progress
     * Thread-safe, can be called from UI thread
     * @return Progress structure with current status
     */
    Progress getProgress() const;

    /**
     * Estimate VRAM required for training
     *
     * @param numImages Number of training images (1-16)
     * @param width Training resolution width
     * @param height Training resolution height
     * @return VRAM requirement in bytes
     */
    static size_t estimateVRAM(int numImages, int width, int height);

    /**
     * Format VRAM size for display
     * @param bytes VRAM size in bytes
     * @return Formatted string (e.g., "950 MB" or "3.2 GB")
     */
    static std::string formatVRAM(size_t bytes);

    /**
     * Get last error message
     * @return Error string, empty if no error
     */
    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(errorMutex);
        return lastError;
    }

private:
    // === Threading ===
    std::unique_ptr<std::thread> trainingThread;
    SDL_Window* trainingWindow = nullptr;
    SDL_GLContext trainingContext = nullptr;
    std::atomic<bool> training{false};
    std::atomic<bool> shouldStop{false};

    // === OpenGL resources ===
    GLuint fullscreenQuadVAO = 0;
    GLuint fullscreenQuadVBO = 0;
    GLuint resizeShaderProgram = 0;

    // === Progress tracking ===
    mutable std::mutex progressMutex;
    Progress progress;

    // === Configuration ===
    bool initialized = false;
    std::string pythonExecutable;
    std::string trainingScriptPath;
    std::string tempDir;
    std::string modelsDir;
    std::string scriptsDir;

    // === Error handling ===
    mutable std::mutex errorMutex;
    std::string lastError;

    // === Windows subprocess handles ===
#ifdef _WIN32
    void* processHandle = nullptr;
    void* stdoutReadHandle = nullptr;
    void* stderrReadHandle = nullptr;
#endif

    // === Private methods ===

    /**
     * Training thread function
     * Runs preprocessing, Python training, and ONNX export
     */
    void trainingThreadFunc(StylePreparationBin* bin,
                           std::string modelName,
                           Config config);

    /**
     * Setup training environment
     * Creates temp directories, checks Python
     */
    bool setupTrainingEnvironment();

    /**
     * Preprocess all training images
     * Loads from StylePreparationBin, scales with lanczos, saves to temp dir
     */
    bool preprocessImages(StylePreparationBin* bin, const Config& config,
                         std::vector<std::string>& outImagePaths);

    /**
     * Preprocess single image
     * Loads with DevIL, uploads to GL, scales with lanczos shader, downloads
     * Smallest side is scaled to targetMinDimension, aspect ratio preserved
     * @param outWidth Output: actual width of processed image
     * @param outHeight Output: actual height of processed image
     * @return OpenGL texture ID, or -1 on error
     */
    GLuint preprocessSingleImage(const std::string& imagePath,
                                 int targetMinDimension, int& outWidth, int& outHeight);

    /**
     * Save preprocessed texture to PNG file
     * Downloads from GL texture using glReadPixels, saves with DevIL
     */
    bool savePreprocessedImage(GLuint texture, int width, int height,
                               const std::string& outputPath);

    /**
     * Generate training configuration JSON file
     * Contains hyperparameters, image paths, output path
     */
    bool generateConfigJSON(const Config& config,
                            const std::vector<std::string>& imagePaths,
                            const std::vector<std::string>& originalImagePaths,
                            const std::string& modelName,
                            const std::string& outputPath);

    /**
     * Launch Python training subprocess
     * Executes train_reconet.py with config JSON
     */
    bool launchPythonTraining(const std::string& configPath,
                              const std::string& outputPath);

    /**
     * Monitor training progress
     * Reads stdout from Python process, parses progress
     */
    void monitorTrainingProgress();

    /**
     * Parse training output line
     * Extracts iteration count, loss values from stdout
     */
    bool parseProgressLine(const std::string& line);

    /**
     * Deploy Python training scripts
     * Copies train_reconet.py, reconet_model.py, loss_functions.py to scripts dir
     */
    bool deployTrainingScripts();

    /**
     * Validate model name
     * Checks for valid characters (alphanumeric + underscore)
     */
    bool validateModelName(const std::string& name);

    /**
     * Count valid images in bin
     * Counts elements with non-empty abspath
     */
    int countValidImages(StylePreparationBin* bin);

    /**
     * Set error message
     * Thread-safe error setter
     */
    void setError(const std::string& error);

    /**
     * Update progress
     * Thread-safe progress updater
     */
    void updateProgress(const Progress& newProgress);

    /**
     * Clear error
     */
    void clearError();

    /**
     * Create fullscreen quad VAO for rendering
     * Called once in training thread context
     */
    void createFullscreenQuad();

    /**
     * Render fullscreen quad using VAO
     * Must be called after createFullscreenQuad()
     */
    void renderFullscreenQuad();

    /**
     * Create simple resize shader for training thread
     * Basic texture sampling shader
     */
    bool createResizeShader();
};

#endif // RECONET_TRAINER_H
