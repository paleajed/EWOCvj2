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
     * Quality presets for training
     */
    enum class Quality {
        FAST,      // 256x256, batch=4, iter=2000, ~5-10 min GPU
        BALANCED,  // 512x512, batch=2, iter=5000, ~30-60 min GPU
        HIGH       // 1024x1024, batch=1, iter=10000, ~2-4 hours GPU
    };

    /**
     * Training configuration
     */
    struct Config {
        Quality quality = Quality::BALANCED;
        float learningRate = 1e-3f;
        float contentWeight = 1.0f;
        float styleWeight = 10.0f;
        float tvWeight = 1e-4f;
        bool useGPU = true;

        // Helper to get resolution from quality preset
        int getResolution() const {
            switch (quality) {
                case Quality::FAST: return 256;
                case Quality::BALANCED: return 512;
                case Quality::HIGH: return 1024;
                default: return 512;
            }
        }

        // Helper to get batch size from quality preset
        int getBatchSize() const {
            switch (quality) {
                case Quality::FAST: return 4;
                case Quality::BALANCED: return 2;
                case Quality::HIGH: return 1;
                default: return 2;
            }
        }

        // Helper to get iteration count from quality preset
        int getIterations() const {
            switch (quality) {
                case Quality::FAST: return 2000;
                case Quality::BALANCED: return 5000;
                case Quality::HIGH: return 10000;
                default: return 5000;
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
     * @return OpenGL texture ID, or -1 on error
     */
    GLuint preprocessSingleImage(const std::string& imagePath,
                                 int targetWidth, int targetHeight);

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
