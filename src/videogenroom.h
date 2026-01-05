/**
 * videogenroom.h
 *
 * UI room for ComfyUI-based video generation
 * Supports StableDiffusion+AnimateDiff and HunyuanVideo backends
 * with 13 creative presets across 5 tiers
 *
 * License: GPL3
 */

#ifndef VIDEOGENROOM_H
#define VIDEOGENROOM_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "ComfyUIManager.h"

class Boxx;
class Param;
class Menu;
class Layer;

// Menu options for video generation room
typedef enum {
    VGEN_DELETE = 0,
    VGEN_BROWSEIMAGE = 1,
    VGEN_SENDTODECK = 2,
    VGEN_CLEARIMAGE = 3
} VGENMENU_OPTION;

// History item for generated outputs
class VideoGenHistoryItem {
public:
    std::string path = "";
    std::string name = "";
    GLuint tex = -1;
    PresetType preset = PresetType::TEXT_TO_VIDEO;
    std::string prompt = "";
    Boxx* box = nullptr;
    Layer* layer = nullptr;

    VideoGenHistoryItem();
    ~VideoGenHistoryItem();
};

class VideoGenRoom {
public:
    VideoGenRoom();
    ~VideoGenRoom();

    // Main render/interaction loop
    void handle();

    // ComfyUI integration
    ComfyUIManager* comfyManager = nullptr;

    bool sdinstalled = false;
    bool hunyuaninstalled = false;

    // UI Layout Boxes
    Boxx* previewBox = nullptr;                         // Large preview area (left side)
    Boxx* historyBox = nullptr;                         // History container
    std::vector<VideoGenHistoryItem*> historyItems;     // Generated outputs history
    int historyScroll = 0;
    Boxx* historyScrollLeft = nullptr;
    Boxx* historyScrollRight = nullptr;

    Boxx* presetsBox = nullptr;                         // Preset list container
    std::vector<Boxx*> presetBoxes;                     // Individual preset boxes
    int presetsScroll = 0;
    Boxx* presetsScrollUp = nullptr;
    Boxx* presetsScrollDown = nullptr;

    Boxx* inputImageBox = nullptr;                      // Input image preview
    Boxx* controlNetBox = nullptr;                      // ControlNet image preview
    Boxx* styleImageBox = nullptr;                      // Style reference preview

    Boxx* generateButton = nullptr;
    Boxx* cancelButton = nullptr;
    Boxx* progressBox = nullptr;

    // Menus
    Menu* videogenmenu = nullptr;
    std::vector<VGENMENU_OPTION> menuoptions;
    VideoGenHistoryItem* menuitem = nullptr;
    int menuboxnr = -1;

    // State
    PresetType selectedPreset = PresetType::TEXT_TO_VIDEO;
    int selectedPresetIndex = 0;
    VideoGenHistoryItem* currentPreviewItem = nullptr;
    GLuint previewTex = -1;
    bool previewIsVideo = false;

    // Input paths
    std::string inputImagePath = "";
    GLuint inputImageTex = -1;
    std::string controlNetImagePath = "";
    GLuint controlNetImageTex = -1;
    std::string styleImagePath = "";
    GLuint styleImageTex = -1;

    // Progress state (cached from callback)
    float progressPercent = 0.0f;
    std::string progressStatus = "Ready";
    GenerationProgress::State progressState = GenerationProgress::State::IDLE;

    // Server startup thread
    std::unique_ptr<std::thread> startupThread;
    std::atomic<bool> startupInProgress{false};

    // Parameters (Param objects for UI)
    Param* tierFilter = nullptr;                // Options: All, Beginner, Intermediate, Advanced, Power User, Interactive
    Param* backendParam = nullptr;              // Options: SD AnimateDiff, HunyuanVideo

    // Generation params
    Param* prompt = nullptr;                    // Text input
    Param* negativePrompt = nullptr;            // Text input
    Param* seed = nullptr;                      // Numeric (-1 = random)
    Param* steps = nullptr;                     // Numeric (default 20)
    Param* cfgScale = nullptr;                  // Numeric (default 7.0)

    // Video params
    Param* frames = nullptr;                    // Numeric (default 16)
    Param* width = nullptr;                     // Numeric (default 512)
    Param* height = nullptr;                    // Numeric (default 512)
    Param* fps = nullptr;                       // Numeric (default 8.0)
    Param* seamlessLoop = nullptr;              // Boolean

    // ControlNet params
    Param* controlNetType = nullptr;            // Options: None, Depth, Canny, Pose, Sketch, Normal
    Param* controlNetStrength = nullptr;        // Numeric (0.0-1.0)

    // Style params
    Param* styleStrength = nullptr;             // Numeric (0.0-1.0)
    Param* preserveColors = nullptr;            // Boolean

    // Motion params
    Param* motionType = nullptr;                // Options: Zoom In, Zoom Out, Pan Left, Pan Right, etc.
    Param* motionStrength = nullptr;            // Numeric (0.0-1.0)
    Param* denoiseStrength = nullptr;           // Numeric (0.0-1.0) - how much to regenerate vs preserve

    // AnimateDiff context params (SD only)
    Param* contextLength = nullptr;             // Numeric (default 16, range 8-32)
    Param* contextOverlap = nullptr;            // Numeric (default 6, range 1-16)

    // Beat sync params
    Param* bpmMode = nullptr;                   // Options: Auto-detect, Manual
    Param* manualBpm = nullptr;                 // Numeric (default 120)
    Param* barLength = nullptr;                 // Numeric (default 4)

    // Kaleidoscope params
    Param* symmetryFold = nullptr;              // Options: 2, 4, 6, 8

    // Morphing params
    Param* morphStartPrompt = nullptr;          // Text input - starting concept
    Param* morphEndPrompt = nullptr;            // Text input - ending concept
    Param* morphMidpoint = nullptr;             // Percentage where transition completes (0-100)

    // Texture evolution params
    Param* startTexture = nullptr;              // Text input - starting texture (e.g., "liquid", "crystal")
    Param* endTexture = nullptr;                // Text input - ending texture

    // Remix params
    Param* remixStrength = nullptr;             // Denoise amount (0.0-1.0)

    // Batch variation params
    Param* batchSize = nullptr;                 // Number of variations to generate (1-8)

    // Frame interpolation params
    Param* frameMultiplier = nullptr;           // Options: 2, 4, 8
    float inputVideoFps = 0.0f;                 // Detected FPS of input video

    // Video continuation params
    Param* appendToSource = nullptr;            // ON/OFF - append to source video or create new clip

    // Output encoding params
    Param* hapOutput = nullptr;                 // Options: OFF, ON - encode to HAP for VJ playback

    bool dragging = false;

    // Methods
    void startGeneration();
    void cancelGeneration();
    void updateProgress();
    void loadOutputToHistory(const std::string& path);
    void clearInputImage();
    void clearControlNetImage();
    void clearStyleImage();
    float getDetectedBpm();
    std::vector<PresetInfo> getFilteredPresets();
    void selectPreset(int index);
    void applyPresetDefaults();
    GenerationParams buildGenerationParams();

private:
    void startupThreadFunc();
};

extern VideoGenRoom* mainvideogenroom;

// ComfyUI server management (defined in videogenroom.cpp)
void stopComfyUIServer();

#endif // VIDEOGENROOM_H
