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
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdint>
#include "ComfyUIManager.h"
#include "ComfyUIInstaller.h"  // for LoraCatalogEntry, InstallProgress::State
#include "CameraPathEditor.h"

class Boxx;
class Param;
class Menu;
class Layer;

// Menu options for video generation room
typedef enum {
    VGEN_DELETE = 0,
    VGEN_BROWSEIMAGE = 1,
    VGEN_EXPORT = 2,
    VGEN_CLEARIMAGE = 3,
    VGEN_QUIT = 4
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
    std::string negprompt = "";
    std::vector<std::string> negpromptlines;
    bool isImg = false;
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

    bool hunyuanfullinstalled = false;
    bool hunyuaninstalled = false;
    bool fluxinstalled = false;
    bool ltxBF16Installed = false;
    bool ltxNVFP4Installed = false;
    bool ltxGGUFInstalled = false;

    // Maps option index to GenerationBackend enum value
    std::vector<int> backendOptionMapping;
    void rebuildBackendOptions();
    GenerationBackend getSelectedBackend();

    // UI Layout Boxes
    Boxx* promptBox = nullptr;
    Boxx* negpromptBox = nullptr;
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
    Boxx* styleImageBox = nullptr;                      // Style reference preview (legacy)
    Boxx* lastFrameImageBox = nullptr;                  // LTX-2.5 FLF2V: last frame preview
    Boxx* loraInstallButton = nullptr;                  // LTX-2.5 FLF2V: "Install LoRA..." button
    Boxx* loraBrowseOnlineButton = nullptr;             // LTX-2.5 FLF2V: "Browse Online LoRAs..." button

    // Single shared Content box, replacing the old per-slot loraContentN boxes for the new
    // baked-preset LoRAs (LTX_FIRST_FRAME_EDIT's edited first frame) - see
    // PresetInfo::requiresContentImage. Deliberately separate members from the legacy
    // loraContent1-4* ones so the hidden/kept-for-reuse old system stays fully independent.
    Boxx* contentBox = nullptr;
    GLuint contentImageTex = -1;
    std::string contentImagePath = "";
    Param* contentStrengthParam = nullptr;
    // Main input box's own Strength slider - see PresetInfo::requiresInputStrengthSlider
    // (LTX_CHARACTER_RETENTION only).
    Param* inputStrengthParam = nullptr;

    // IC-LoRA control image boxes (LTX-2.5 I2V/FLF2V only) - one per LoRA slot, arranged
    // around the main input image like the FLUX.2 Klein style reference boxes.
    //
    // The conditioning mechanism used for a given slot's control image (Lightricks "Ingredients"
    // guide vs BFS identity-overlap etc.) is no longer a manual per-slot toggle - it's looked up
    // automatically from the selected LoRA's filename via
    // ComfyUIInstaller::findLoraModeOverride() (a small, growing registry - each LoRA that turns
    // out to need non-default wiring gets its own entry there as it's tested), defaulting to the
    // plain Ingredients guide for anything not specifically registered.
    Boxx* loraControl1Box = nullptr;
    Boxx* loraControl2Box = nullptr;
    Boxx* loraControl3Box = nullptr;
    Boxx* loraControl4Box = nullptr;
    std::string loraControl1ImagePath = "", loraControl2ImagePath = "", loraControl3ImagePath = "", loraControl4ImagePath = "";
    GLuint loraControl1ImageTex = -1, loraControl2ImageTex = -1, loraControl3ImageTex = -1, loraControl4ImageTex = -1;

    // Second per-slot input, stacked below loraControlNBox - the old "Union Control" mechanism
    // that used to consume this (LTXVAddGuide+LTXVImgToVideoInplace, gated on a manual mode
    // toggle) has been removed since no tested LoRA needed it, but the boxes themselves stay
    // wired up (drag-drop, browse, clear, strength slider) and kept in the UI for whichever
    // future per-LoRA JSON tweak turns out to need a second reference input.
    Boxx* loraContent1Box = nullptr;
    Boxx* loraContent2Box = nullptr;
    Boxx* loraContent3Box = nullptr;
    Boxx* loraContent4Box = nullptr;
    std::string loraContent1ImagePath = "", loraContent2ImagePath = "", loraContent3ImagePath = "", loraContent4ImagePath = "";
    GLuint loraContent1ImageTex = -1, loraContent2ImageTex = -1, loraContent3ImageTex = -1, loraContent4ImageTex = -1;

    // Camera Warp (LORA_WIRING_CROSSVIEW) per-slot camera state, set via the CameraPathEditor
    // ("Edit CAM" button, shown in the dormant CONTENT-box row for these slots) - direct mirrors
    // of GenerationParams::loraCameraAzimuthN etc. (ComfyUIManager.h), copied across in
    // buildGenerationParams(). loraCameraKeyframesN empty = static camera (azimuth/elevation/
    // distance/hfov alone); non-empty = moving camera, those four static values ignored.
    float loraCameraAzimuth1 = 0.0f, loraCameraAzimuth2 = 0.0f, loraCameraAzimuth3 = 0.0f, loraCameraAzimuth4 = 0.0f;
    float loraCameraElevation1 = 0.0f, loraCameraElevation2 = 0.0f, loraCameraElevation3 = 0.0f, loraCameraElevation4 = 0.0f;
    // 1.0 = "unchanged from the source camera" per CrossViewWarp's own semantics (multiplicative,
    // not an absolute distance) - see Camera3D.h's OrbitCamera::distance comment.
    float loraCameraDistance1 = 1.0f, loraCameraDistance2 = 1.0f, loraCameraDistance3 = 1.0f, loraCameraDistance4 = 1.0f;
    float loraCameraHfov1 = 50.0f, loraCameraHfov2 = 50.0f, loraCameraHfov3 = 50.0f, loraCameraHfov4 = 50.0f;
    // pivot_x/y/z with pivot_override=true - a literal metric position (metres), not a
    // preview-scale value. See GenerationParams::loraCameraPivotXN's comment (ComfyUIManager.h).
    float loraCameraPivotX1 = 0.0f, loraCameraPivotX2 = 0.0f, loraCameraPivotX3 = 0.0f, loraCameraPivotX4 = 0.0f;
    float loraCameraPivotY1 = 0.0f, loraCameraPivotY2 = 0.0f, loraCameraPivotY3 = 0.0f, loraCameraPivotY4 = 0.0f;
    float loraCameraPivotZ1 = 1.05f, loraCameraPivotZ2 = 1.05f, loraCameraPivotZ3 = 1.05f, loraCameraPivotZ4 = 1.05f;
    std::string loraCameraKeyframes1 = "", loraCameraKeyframes2 = "", loraCameraKeyframes3 = "", loraCameraKeyframes4 = "";

    CameraPathEditor cameraPathEditor;

    // FLUX.2 Klein style reference boxes (shown only for Klein backend)
    Boxx* style1ImageBox = nullptr;
    Boxx* style2ImageBox = nullptr;
    Boxx* style3ImageBox = nullptr;
    Boxx* style4ImageBox = nullptr;
    std::string style1ImagePath = "", style2ImagePath = "", style3ImagePath = "", style4ImagePath = "";
    GLuint style1ImageTex = -1, style2ImageTex = -1, style3ImageTex = -1, style4ImageTex = -1;

    // FLUX.2 Klein per-reference strength params
    Param* style1Strength = nullptr;
    Param* style2Strength = nullptr;
    Param* style3Strength = nullptr;
    Param* style4Strength = nullptr;

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
    std::string lastFrameImagePath = "";        // LTX-2.5 FLF2V: end anchor image
    GLuint lastFrameImageTex = -1;
    std::string promptstr = "";
    std::string oldpromptstr = "";
    std::vector<std::string> promptlines;
    std::string negpromptstr = "";
    std::string negoldpromptstr = "";
    std::vector<std::string> negpromptlines;

    // Progress state (cached from callback)
    float progressPercent = 0.0f;
    std::string progressStatus = "Ready";
    GenerationProgress::State progressState = GenerationProgress::State::IDLE;
    bool wasGenerating = false;  // Detects the isGenerating() true->false edge in updateProgress()

    // Server startup thread
    std::unique_ptr<std::thread> startupThread;
    std::atomic<bool> startupInProgress{false};

    // Parameters (Param objects for UI)
    Param* backendParam = nullptr;              // Backend selection (HunyuanVideo, Flux Klein)
    GenerationBackend lastBackend = GenerationBackend::HUNYUAN_SLIM;  // Track backend changes for preset reset
    bool lastBackendInitialized = false;

    // Generation params
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
    Param* denoiseStrength = nullptr;           // Numeric (0.0-1.0) - how much to regenerate vs preserve (Hunyuan, inverted)
    Param* flf2vFirstFrameStrength = nullptr;   // Numeric (0.0-1.0) - LTX-2.5 FLF2V: first-frame guide anchor strength
    Param* flf2vLastFrameStrength = nullptr;    // Numeric (0.0-1.0) - LTX-2.5 FLF2V: last-frame guide anchor strength
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

    // LoRA params - any LTX-2.5 preset (T2V, I2V, FLF2V), up to 4 simultaneous slots.
    // Each dropdown shares the same option list (all get the same options/mapping copied in
    // by rebuildLoraOptions()) - "None", <installed lora files...>, <online catalog...>.
    // "Browse Online LoRAs..." doesn't trigger the catalog fetch (that runs automatically in
    // the background from program start) - it just opens HuggingFace in a browser so the user
    // can read about what's there.
    Param* loraParam1 = nullptr;
    Param* loraParam2 = nullptr;
    Param* loraParam3 = nullptr;
    Param* loraParam4 = nullptr;
    Param* loraStrength1 = nullptr;             // Numeric (0.0-2.0), each
    Param* loraStrength2 = nullptr;
    Param* loraStrength3 = nullptr;
    Param* loraStrength4 = nullptr;
    // Per-slot IC-LoRA control image strength (0.0-1.0), one independent slider per LoRA slot,
    // drawn/positioned below its own loraControlNBox like the FLUX.2 Klein reference strength
    // sliders sit below their style boxes.
    Param* loraControlStrength1 = nullptr;
    Param* loraControlStrength2 = nullptr;
    Param* loraControlStrength3 = nullptr;
    Param* loraControlStrength4 = nullptr;
    // Strength for the (currently dormant, kept for reuse) CONTENT box above.
    Param* loraContentStrength1 = nullptr;
    Param* loraContentStrength2 = nullptr;
    Param* loraContentStrength3 = nullptr;
    Param* loraContentStrength4 = nullptr;
    std::vector<std::string> loraOptionMapping; // Parallel to each loraParamN->options; "" for "None"
    std::vector<int> loraOptionCatalogIndex;    // Parallel to each loraParamN->options; index into loraCatalog, -1 if local/None
    bool loraOptionsLoaded = false;             // Set once rebuildLoraOptions() has run (avoids polling ComfyUI every frame)
    float lastHandledLoraValue1 = -1.0f;        // Last loraParamN->value we reacted to (detects a fresh dropdown pick), per slot
    float lastHandledLoraValue2 = -1.0f;
    float lastHandledLoraValue3 = -1.0f;
    float lastHandledLoraValue4 = -1.0f;

    // Online LoRA catalog (HuggingFace, LTX-2.5/2.3) - fetched automatically once in the
    // background at program start (see startLoraCatalogFetch(), called from the constructor)
    std::vector<LoraCatalogEntry> loraCatalog;
    std::vector<LoraCatalogEntry> loraCatalogPending;   // written by loraCatalogThread, merged in once done
    std::unique_ptr<std::thread> loraCatalogThread;
    std::atomic<bool> loraCatalogThreadDone{false};
    bool loraCatalogFetching = false;
    bool loraCatalogFetched = false;
    std::string loraCatalogError;

    // Download of a selected-but-not-yet-local catalog LoRA - one installer per slot so all
    // 4 can download in parallel instead of queuing behind a single shared install thread
    ComfyUIInstaller* loraInstaller1 = nullptr;
    ComfyUIInstaller* loraInstaller2 = nullptr;
    ComfyUIInstaller* loraInstaller3 = nullptr;
    ComfyUIInstaller* loraInstaller4 = nullptr;
    InstallProgress::State lastLoraInstallState1 = InstallProgress::State::IDLE;
    InstallProgress::State lastLoraInstallState2 = InstallProgress::State::IDLE;
    InstallProgress::State lastLoraInstallState3 = InstallProgress::State::IDLE;
    InstallProgress::State lastLoraInstallState4 = InstallProgress::State::IDLE;

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
    int savedFlux2KleinWidth = 1024;
    int savedFlux2KleinHeight = 1024;
    int savedLtxWidth = 1920;
    int savedLtxHeight = 1056;  // 1080 isn't a multiple of 32 (LTX's latent grid step); this is the nearest valid value

    // Saved steps per backend (remembered when switching)
    int savedHunyuanSteps = 20;
    int savedFlux2KleinSteps = 4;
    int savedLtxSteps = 8;
    float savedLtxFps = 24.0f;  // this->fps defaults to 8.0f generically; LTX needs its own sensible default

    bool dragging = false;

    // Methods
    void startGeneration();
    void cancelGeneration();
    void updateProgress();
    void loadOutputToHistory(const std::string& path);
    void loadFirstFramePreview(const std::string& path, GLuint& outTex);
    // Multi-frame sibling of loadFirstFramePreview() for the Camera Path Editor: decodes up to
    // frameCount frames (fewer if the clip is shorter) at native resolution into raw RGBA byte
    // buffers, one per frame, in decode order. No GL calls - safe to call off the GL thread
    // (the point-cloud build thread). Returns false if the file can't be opened/decoded at all.
    bool decodeVideoFramesRGBA(const std::string& path, int frameCount,
                                std::vector<std::vector<uint8_t>>& outFrames,
                                int& outW, int& outH);
    // EDIT_IMAGE (Flux) sizes its output canvas from the input media's own resolution rather
    // than a fixed default, since it's an edit of that exact image/video, not a fresh generation
    // at some other size. No-op unless EDIT_IMAGE is the active preset and inputImagePath is set
    // (image or video). Called on preset selection AND whenever the main input box's content
    // changes while already on this preset (drag-drop, internal drag, Browse...).
    void syncEditImageDimensionsFromInput();
    void clearInputImage();
    void clearControlNetImage();
    void clearStyleImage();
    float getDetectedBpm();
    std::vector<PresetInfo> getFilteredPresets();
    void selectPreset(int index);
    void applyPresetDefaults();
    GenerationParams buildGenerationParams();
    void rebuildLoraOptions();
    // Handles one LoRA dropdown+strength slot: .handle() both, plus the strength slider
    // whenever the slot isn't "None"
    // Handles one LoRA dropdown+strength slot: .handle() both, detect a fresh dropdown pick,
    // and kick off the download (via this slot's own ComfyUIInstaller, so slots download in
    // parallel rather than sharing one install thread) if it points at an online catalog
    // entry not yet on disk
    void handleLoraSlot(Param* loraParam, Param* loraStrengthParam, Param* loraControlStrengthParam,
                         float& lastHandledValue, ComfyUIInstaller*& installer,
                         InstallProgress::State& lastInstallState);
    // Kicks off (or restarts) the background HuggingFace LoRA catalog fetch. Called once
    // automatically at construction (program start). Non-blocking - results are picked up and
    // merged into the dropdowns from handle() once loraCatalogThreadDone flips true.
    void startLoraCatalogFetch();

    // Ensures comfyManager is initialized, the ComfyUI server process is running, and
    // comfyManager is connected to it - the same "server up + connected" sequence
    // startupThreadFunc() runs before every generation (initialize -> start server if needed ->
    // connect with retries, up to 300s total). Factored out so CameraPathEditor's depth-preview
    // job can go through the same readiness check instead of assuming the server is already up
    // (previously it wasn't, which meant clicking "Edit CAM" without having generated at least
    // once yet failed the upload with a plain connection-refused error). BLOCKING - call this
    // from a background thread, never the main/GL thread. onStatus is invoked repeatedly with
    // human-readable progress text (mirrors what progressStatus shows during normal startup).
    bool ensureComfyUIReady(std::function<void(const std::string&)> onStatus);

private:
    void startupThreadFunc();
};

extern VideoGenRoom* mainvideogenroom;

// ComfyUI server management (defined in videogenroom.cpp)
// extraVramHeadroom: pass true only for the backend(s) that need the --vram-headroom
// mitigation (see call site) - it costs usable VRAM, so it must not be applied blindly
// to backends/cards that don't need it (e.g. Hunyuan on a 16GB card).
bool startComfyUIServer(std::function<void(const std::string&)> statusCallback = nullptr,
                         bool extraVramHeadroom = false);
void stopComfyUIServer();

// Delete the temp ComfyUI output files for a history item (video file + frames dir)
void deleteHistoryItemOutputFiles(const std::string& path);

// ComfyUI process throttling — suspend/resume to protect main mix fps.
// When enabled (default), the ComfyUI process is suspended for the duration of
// each render frame and only allowed to run during the main loop's idle sleep,
// so the VJ rendering gets GPU priority while generation still makes progress.
void throttleComfyUIProcess(bool suspend);
extern bool comfyUIThrottleEnabled;

#endif // VIDEOGENROOM_H
