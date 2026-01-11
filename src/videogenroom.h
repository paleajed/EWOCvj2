/**
 * videogenroom.h
 *
 * UI room for ComfyUI-based video generation
 * Supports HunyuanVideo GGUF backend
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
    std::vector<std::string> promptlines;
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

    bool hunyuaninstalled = false;
    bool fluxinstalled = false;

    // UI Layout Boxes
    Boxx* promptBox = nullptr;                         // Large preview area (left side)
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
    bool controlNetIsVideo = false;
    std::string styleImagePath = "";
    GLuint styleImageTex = -1;
    std::string promptstr = "";
    std::string oldpromptstr = "";
    std::vector<std::string> promptlines;

    // Progress state (cached from callback)
    float progressPercent = 0.0f;
    std::string progressStatus = "Ready";
    GenerationProgress::State progressState = GenerationProgress::State::IDLE;

    // Server startup thread
    std::unique_ptr<std::thread> startupThread;
    std::atomic<bool> startupInProgress{false};

    // Parameters (Param objects for UI)
    Param* backendParam = nullptr;              // Backend selection (HunyuanVideo, Flux Schnell)
    int lastBackendValue = -1;                  // Track backend changes for preset reset

    // Generation params
    Param* negativePrompt = nullptr;            // Text input
    Param* seed = nullptr;                      // Numeric (-1 = random)
    Param* steps = nullptr;                     // Numeric (default 20)
    Param* cfgScale = nullptr;                  // Numeric (default 7.0)
    Param* promptImprove = nullptr;             // Boolean - AI prompt enhancement (Flux only)

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
    Param* denoiseStrength = nullptr;           // Numeric (0.0-1.0) - how much to regenerate vs preserve (Hunyuan, inverted)
    Param* fluxDenoiseStrength = nullptr;       // Numeric (0.0-1.0) - Flux denoise (direct, not inverted)

    // Beat sync params
    Param* bpmMode = nullptr;                   // Options: Auto-detect, Manual
    Param* manualBpm = nullptr;                 // Numeric (default 120)
    Param* barLength = nullptr;                 // Numeric (default 4)

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

    // Saved dimensions per backend (remembered when switching)
    int savedHunyuanWidth = 640;
    int savedHunyuanHeight = 368;
    int savedFluxWidth = 1920;
    int savedFluxHeight = 1080;

    // Saved steps per backend (remembered when switching)
    int savedHunyuanSteps = 20;
    int savedFluxSteps = 4;

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
