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

    // === GL resources (created lazily on first draw(), since GL calls must happen on the
    // render thread while the build thread only ever touches cloudBin/CPU buffers) ===
    unsigned int shaderProgram = 0;  // GLuint, avoid pulling GL headers into this header
    bool shaderLoadAttempted = false;  // set_shader_from_files() failed - don't retry every frame
    unsigned int pointVAO = 0, pointVBO = 0;
    int currentVBOFrame = -1;        // which frame's data is currently uploaded, -1 = none
    void ensureGLResources();
    void uploadFrameToVBO(int frame);

    // CPU-side copy of the currently-uploaded frame's raw point data (same bytes as the VBO),
    // kept around for computeHoleFraction() below - re-reading from cloudBin's file every frame
    // just to check for holes would mean disk I/O at interactive drag rates, so this is cached
    // alongside the GPU upload in uploadFrameToVBO() instead (same frame, same buffer, no copy
    // beyond the one uploadFrameToVBO() already did anyway).
    std::vector<uint8_t> currentFrameCpu;

    // Estimates what fraction of the frame CrossViewWarp's actual warp would leave as
    // disocclusion holes (its own magenta fill) if generation used exactly this camera pose -
    // by reprojecting this preview's own point cloud through the SAME view/projection this
    // camera would render with, onto a coarse grid, and counting cells nothing lands in. See
    // autoEaseDistanceForHoles()'s comment for why this exists.
    float computeHoleFraction(const OrbitCamera& cam) const;
    // Shared grid-coverage core of computeHoleFraction() above, factored out so it can run the
    // same estimate against an arbitrary OTHER frame's point-cloud buffer if needed later.
    float computeHoleFractionForBuffer(const std::vector<uint8_t>& buf, const Mat4& mvp) const;

    // Pure - the interpolated (unboosted) az/el/dist at an arbitrary frame along the keyframe
    // path, with no side effects on camera/desired* state. Used by interpolateCameraAtScrub()
    // (applies the result to the live camera). No-op (leaves outputs untouched) if fewer than 2
    // keyframes - callers must check first.
    void interpolatePoseAtFrame(int frame, float& outAz, float& outEl, float& outDist) const;

    // Escape hatch for testing runs where hole-driven easing shouldn't second-guess the pose -
    // e.g. once the CrossView-Warp LoRA gets an LTX-2.5-compatible release (confirmed as of now
    // that the current v2 LoRA simply doesn't work on 2.5 at all - an orbit test elsewhere showed
    // the same near-static failure independent of hole content or wiring, so this session's
    // pipeline is sound and just waiting on the LoRA side). With this off,
    // autoEaseDistanceForHoles() still measures and reports lastHoleFraction (so the HUD stays
    // honest) but stops correcting the camera for it. Toggled via a HUD button, not persisted.
    bool easingEnabled = true;

    // Shared between autoEaseDistanceForHoles() (the actual easing decision) and
    // drawSafeZoneHud() (the "(easing distance back)" HUD note) so they can't silently drift
    // apart the way a HUD-local copy of this number already did once - edit this one value to
    // experiment with different thresholds, nothing else needs to change.
    static constexpr float kHoleThreshold = 0.15f;

    // Reactively backs the camera off toward the hole-free identity pose (az=el=0, dist=1)
    // whenever the current az/el/dist/pivot combination would blow the warp's guide apart into
    // mostly holes - confirmed directly against the actual server (crossview_preview's own
    // live-render endpoint) via two different failure modes: dist=0.293 combined with just a 14
    // degree orbit rendered ~80%+ magenta (far-eye-translation/parallax holes), and separately a
    // PURE 37 degree orbit at dist=1.0 (no dolly at all, well inside the LoRA's own documented
    // "reliable" zone) rendered ~67% magenta (reframing holes - the camera has to rotate to keep
    // facing the pivot, and that rotation alone can spin the original frame out of the lens's
    // FOV). Distance is tried FIRST since it's the more common culprit and a smaller-feeling
    // adjustment; azimuth/elevation only ease once distance is already back at 1.0 and holes are
    // STILL over threshold, meaning the orbit angle itself is the remaining problem. Called every
    // frame from handle() while the editor is interactive, so it responds live as the user
    // orbits, not just at Apply time.
    void autoEaseDistanceForHoles();
    // Last value computeHoleFraction() returned - drawSafeZoneHud() shows it so easing doesn't
    // look like distance/orbit mysteriously resisting input for no visible reason.
    float lastHoleFraction = 0.0f;
    // What the user actually asked for - the last distance/azimuth/elevation explicitly set via
    // scroll-wheel dolly or orbit-drag (handleOrbitInput()), or the last keyframe-interpolated
    // values (interpolateCameraAtScrub()). autoEaseDistanceForHoles() may temporarily hold the
    // live camera values away from these to avoid holes, then creeps them back once the rest of
    // the pose no longer needs the detour - e.g. dolly in, orbit out to where holes appear
    // (distance eases back first, then orbit if that alone isn't enough), orbit back toward the
    // original angle, and both should return on their own rather than staying backed off forever.
    float desiredDistance = 1.0f;
    float desiredAzimuth = 0.0f;
    float desiredElevation = 0.0f;

    // === UI boxes (lazily positioned on first draw(), same idiom as the rest of videogenroom) ===
    Boxx* viewportBox = nullptr;
    Boxx* timelineBox = nullptr;
    Boxx* applyButtonBox = nullptr;
    Boxx* cancelButtonBox = nullptr;
    Boxx* addKeyframeButtonBox = nullptr;
    Boxx* deleteKeyframeButtonBox = nullptr;
    Boxx* clearKeyframesButtonBox = nullptr;
    Boxx* easingToggleBox = nullptr;
    bool boxesInitialized = false;
    void ensureBoxes();

    void handleOrbitInput();
    void handleTimelineInput();
    void handleButtons();

    void drawStatus();
    void drawViewport();
    void drawSafeZoneHud();
    void drawTimeline();
};

#endif // CAMERAPATHEDITOR_H
