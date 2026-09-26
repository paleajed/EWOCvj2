/**
 * CameraPathEditor.h
 *
 * Modal editor for the "Camera Warp" LoRA (LORA_WIRING_CROSSVIEW): lets the user orbit a
 * Maya-style 3D camera around a per-frame point-cloud reconstruction of the input clip's
 * MoGe depth, instead of guessing loraCameraAzimuthN/loraCameraElevationN/etc. blind, and
 * place per-frame keyframes for a moving camera path.
 *
 * Owned by VideoGenRoom (one instance, opened per-slot) - not a top-level room. Drives its own
 * background jobs (ComfyUIManager's depth-extraction job, then a local point-cloud build) via
 * VideoGenRoom's existing ComfyUIManager* instance.
 *
 * License: GPL3
 */

#ifndef CAMERAPATHEDITOR_H
#define CAMERAPATHEDITOR_H

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <cstdint>
#include "Camera3D.h"

class ComfyUIManager;
class Boxx;

// One camera pose at a specific frame (moving-camera mode). vs/px/py/pz are not exposed in the
// UI yet - getResultKeyframesJson() omits them entirely so each keyframe inherits the resolved
// static pivot (pivot_x/y/z) from CrossViewWarp's own _parse_keyframes() default, rather than
// serializing them as 0 (which used to force pivot=(0,0,0) on every keyframe - see that
// function's comment for why that caused a recurring "Singular matrix" crash).
struct CameraKeyframe {
    int frame = 0;
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float distance = 1.0f;  // CrossViewWarp's own "distance" is multiplicative vs. the source
                             // camera (1.0 = unchanged) - see OrbitCamera::distance's comment
};

// Per-frame point cloud storage. Deliberately NOT memory-mapped (unlike SAMSegmentation's
// PropagationBin, which only ever reads a file some other process already wrote) - this editor
// both writes (once, after the depth job + build finish) and reads (every scrub) the same file
// from the same process, so plain buffered file I/O is simpler and, for the ~1-2MB/frame sizes
// involved, just as fast in practice (the OS page cache already has the data hot right after
// writing it).
struct PointCloudBin {
    std::string path;
    uint32_t numFrames = 0;
    uint32_t numPoints = 0;    // fixed per frame, after downsampling
    static constexpr size_t kBytesPerPoint = 16;  // 3x float32 xyz + 4x uint8 rgba
    static constexpr size_t kHeaderBytes = 16;    // magic(4) + numFrames(4) + numPoints(4) + reserved(4)
    static constexpr uint32_t kMagic = 0x50434C44u;  // 'PCLD'

    bool valid() const { return numFrames > 0 && numPoints > 0 && !path.empty(); }
    size_t frameSizeBytes() const { return (size_t)numPoints * kBytesPerPoint; }

    // Reads frame i's raw interleaved point data (numPoints * 16 bytes) into outBuf. Returns
    // false on any I/O error or out-of-range index.
    bool readFrame(int i, std::vector<uint8_t>& outBuf) const;
};

// Writes a fresh .bin file: header + numFrames * numPoints * 16 bytes, from perFrameData (each
// entry already exactly numPoints*16 bytes, interleaved xyz+rgba per point). Returns false on
// any I/O error.
bool writePointCloudBin(const std::string& path, uint32_t numPoints,
                         const std::vector<std::vector<uint8_t>>& perFrameData);

class CameraPathEditor {
public:
    CameraPathEditor();
    ~CameraPathEditor();

    // Opens the editor for a given LoRA slot and immediately kicks off the depth-extraction job
    // (ComfyUIManager::extractCameraWarpDepthPreview) followed by the local point-cloud build -
    // no separate "start" step. comfy must already be connected (VideoGenRoom's existing
    // instance). initKeyframesJson may be empty (static-camera mode).
    void open(ComfyUIManager* comfy, int slotIndex, const std::string& controlVideoPath,
              int frameCount, float initAz, float initEl, float initDist, float initHfov,
              float initPivotX, float initPivotY, float initPivotZ,
              const std::string& initKeyframesJson);

    // Cancel: discards in-progress/edited state, does not touch the slot's stored values.
    void close();
    bool isOpen() const { return active; }
    int  getSlotIndex() const { return slotIndex; }

    // Set by handle() when the user clicks Apply - VideoGenRoom checks this after calling
    // handle() each frame and, if true, copies azimuth/elevation/distance/hfov/pivot/keyframesJson
    // back into the slot's stored fields, then calls close().
    bool consumeApplyRequested();
    float getResultAzimuth() const { return camera.azimuthDeg; }
    float getResultPivotX() const { return camera.pivot[0]; }
    float getResultPivotY() const { return camera.pivot[1]; }
    float getResultPivotZ() const { return camera.pivot[2]; }
    float getResultElevation() const { return camera.elevationDeg; }
    float getResultDistance() const { return camera.distance; }
    float getResultHfov() const { return camera.hfovDeg; }
    std::string getResultKeyframesJson() const;

    void handle();  // consumes all input for the frame while open
    void draw();    // renders the dimmed overlay + viewport + HUD + timeline

private:
    bool active = false;
    int slotIndex = -1;
    ComfyUIManager* comfy = nullptr;
    std::string controlVideoPath;
    bool applyRequested = false;

    OrbitCamera camera;
    // Source control video's own aspect ratio (rgbW/rgbH, set once buildPointCloudsThreadFunc()
    // knows the decoded frame dimensions). The 3D viewport panel's own pixel rectangle is a
    // different, fixed UI shape - using ITS aspect ratio for the projection matrix's vertical FOV
    // conversion reconstructed a vertical FOV that didn't match what the video actually captured
    // (cutting off the top while the sides, driven directly by hfovDeg, stayed correct). Instead
    // drawViewport() letterboxes the actual rendered rectangle to this aspect ratio within the
    // panel, so horizontal and vertical FOV are both derived from the one aspect ratio that's
    // actually correct: the video's.
    float videoAspect = 16.0f / 9.0f;
    // Orbit/pan drag: uses SDL relative-mouse-mode (hides the cursor, reports raw motion deltas
    // unaffected by hitting the window/screen edge) instead of diffing absolute mx/my - see
    // handleOrbitInput()'s comment for why. dragging is only ever true while relative mode is
    // active; anything that can end the modal early (close(), the destructor) must restore normal
    // mouse mode if dragging was left true, or the cursor stays hidden/captured afterward.
    bool dragging = false;
    bool draggingIsOrbit = false;

    // === Depth job + point-cloud build (background threads) ===
    std::unique_ptr<std::thread> readyThread;
    void ensureReadyAndStartDepthJobThreadFunc(std::string videoPath, int frameCount);
    std::atomic<bool> depthJobStarted{false};
    std::atomic<bool> buildStarted{false};
    std::atomic<bool> buildDone{false};
    std::atomic<bool> buildFailed{false};
    std::string buildError;
    std::string buildStatusText;
    std::mutex buildStatusMutex;
    std::unique_ptr<std::thread> buildThread;
    PointCloudBin cloudBin;

    void pollDepthJobAndMaybeStartBuild();  // called from handle()/draw() each frame
    void buildPointCloudsThreadFunc(std::string depthMetricPath, int frameCount);
    void setBuildStatus(const std::string& text);
    void failBuild(const std::string& err);

    // === Timeline / keyframes ===
    std::vector<CameraKeyframe> keyframes;  // kept sorted by frame
    int scrubFrame = 0;
    int totalFrames = 0;
    int selectedKeyframe = -1;

    void scrubTo(int frame);
    void addKeyframeAtScrub();
    void deleteSelectedKeyframe();
    void clearAllKeyframes();
    void interpolateCameraAtScrub();  // no-op if fewer than 2 keyframes
    // Selection now tracks the scrub position (driven by the timeline drag, which already works
    // reliably) instead of raw per-pixel mouse hover math against the tick marks - sets
    // selectedKeyframe to whichever keyframe (if any) sits exactly at scrubFrame, else -1. Called
    // any time scrubFrame or keyframes[] changes.
    void updateSelectedKeyframeFromScrub();
    // Called from open() right after loading keyframes, in case Frames was lowered since they
    // were captured (totalFrames is already the NEW value by then). Reconciles the path with the
    // shorter clip: first captures where the ORIGINAL path (all keyframes, including the ones
    // about to be dropped) was actually headed at the new last frame, inserts that as a real
    // keyframe there, and only then removes everything past it - so the retained portion of the
    // camera flow keeps moving toward where it was going instead of abruptly freezing at whichever
    // keyframe happened to survive.
    void truncateKeyframesToFrameRange();

    // === GL resources (created lazily on first draw(), since GL calls must happen on the
    // render thread while the build thread only ever touches cloudBin/CPU buffers) ===
    unsigned int shaderProgram = 0;  // GLuint, avoid pulling GL headers into this header
    bool shaderLoadAttempted = false;  // set_shader_from_files() failed - don't retry every frame
    unsigned int pointVAO = 0, pointVBO = 0;
    int currentVBOFrame = -1;        // which frame's data is currently uploaded, -1 = none
    void ensureGLResources();
    void uploadFrameToVBO(int frame);

    // Pure - the interpolated (unboosted) az/el/dist at an arbitrary frame along the keyframe
    // path, with no side effects on camera state. Used by interpolateCameraAtScrub() (applies the
    // result to the live camera). No-op (leaves outputs untouched) if fewer than 2 keyframes -
    // callers must check first.
    void interpolatePoseAtFrame(int frame, float& outAz, float& outEl, float& outDist) const;

    // === UI boxes (lazily positioned on first draw(), same idiom as the rest of videogenroom) ===
    Boxx* viewportBox = nullptr;
    Boxx* timelineBox = nullptr;
    Boxx* applyButtonBox = nullptr;
    Boxx* cancelButtonBox = nullptr;
    Boxx* addKeyframeButtonBox = nullptr;
    Boxx* deleteKeyframeButtonBox = nullptr;
    Boxx* clearKeyframesButtonBox = nullptr;
    bool boxesInitialized = false;
    void ensureBoxes();

    void handleOrbitInput();
    void handleTimelineInput();
    void handleButtons();

    void drawStatus();
    void drawViewport();
    void drawCameraHud();
    void drawTimeline();
};

#endif // CAMERAPATHEDITOR_H
