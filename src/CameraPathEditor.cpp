/**
 * CameraPathEditor.cpp
 *
 * License: GPL3
 */

#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#define LINUX
#elif defined(__APPLE__)
#define POSIX
#define MACOS
#endif

#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#endif

#include <SDL3/SDL.h>

#include "CameraPathEditor.h"
#include "program.h"
#include "videogenroom.h"
#include "ComfyUIManager.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace fs = std::filesystem;

// ============================================================================
// PointCloudBin - plain buffered file I/O, see the "not memory-mapped" doc
// comment on the struct in CameraPathEditor.h for why.
// ============================================================================

bool writePointCloudBin(const std::string& path, uint32_t numPoints,
                         const std::vector<std::vector<uint8_t>>& perFrameData) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    uint32_t magic = PointCloudBin::kMagic;
    uint32_t numFrames = (uint32_t)perFrameData.size();
    uint32_t reserved = 0;
    out.write((const char*)&magic, 4);
    out.write((const char*)&numFrames, 4);
    out.write((const char*)&numPoints, 4);
    out.write((const char*)&reserved, 4);

    size_t expectedFrameBytes = (size_t)numPoints * PointCloudBin::kBytesPerPoint;
    for (const auto& frame : perFrameData) {
        if (frame.size() != expectedFrameBytes) {
            return false;  // caller built a mismatched frame - fail loudly rather than write garbage
        }
        out.write((const char*)frame.data(), (std::streamsize)frame.size());
    }
    return (bool)out;
}

bool PointCloudBin::readFrame(int i, std::vector<uint8_t>& outBuf) const {
    if (!valid() || i < 0 || (uint32_t)i >= numFrames) return false;
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    size_t offset = kHeaderBytes + (size_t)i * frameSizeBytes();
    in.seekg((std::streamoff)offset, std::ios::beg);
    if (!in) return false;
    outBuf.resize(frameSizeBytes());
    in.read((char*)outBuf.data(), (std::streamsize)outBuf.size());
    return (bool)in || in.eof();
}

static bool openPointCloudBinHeader(const std::string& path, PointCloudBin& bin) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    uint32_t magic = 0, numFrames = 0, numPoints = 0, reserved = 0;
    in.read((char*)&magic, 4);
    in.read((char*)&numFrames, 4);
    in.read((char*)&numPoints, 4);
    in.read((char*)&reserved, 4);
    if (!in || magic != PointCloudBin::kMagic) return false;
    bin.path = path;
    bin.numFrames = numFrames;
    bin.numPoints = numPoints;
    return true;
}

// ============================================================================
// CameraPathEditor
// ============================================================================

CameraPathEditor::CameraPathEditor() {}

CameraPathEditor::~CameraPathEditor() {
    if (dragging) SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, false);
    if (readyThread && readyThread->joinable()) readyThread->join();
    if (buildThread && buildThread->joinable()) buildThread->join();
    delete viewportBox;
    delete timelineBox;
    delete applyButtonBox;
    delete cancelButtonBox;
    delete addKeyframeButtonBox;
    delete deleteKeyframeButtonBox;
    delete clearKeyframesButtonBox;
    delete easingToggleBox;
#ifndef USE_GLES
    if (pointVBO) glDeleteBuffers(1, &pointVBO);
    if (pointVAO) glDeleteVertexArrays(1, &pointVAO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
#endif
}

void CameraPathEditor::open(ComfyUIManager* comfyIn, int slotIndexIn, const std::string& controlVideoPathIn,
                             int frameCount, float initAz, float initEl, float initDist, float initHfov,
                             float initPivotX, float initPivotY, float initPivotZ,
                             const std::string& initKeyframesJson) {
    if (!comfyIn || controlVideoPathIn.empty() || frameCount <= 0) return;

    comfy = comfyIn;
    slotIndex = slotIndexIn;
    controlVideoPath = controlVideoPathIn;
    totalFrames = frameCount;
    scrubFrame = 0;
    selectedKeyframe = -1;
    applyRequested = false;

    camera = OrbitCamera();
    camera.azimuthDeg = initAz;
    camera.elevationDeg = initEl;
    desiredAzimuth = camera.azimuthDeg;
    desiredElevation = camera.elevationDeg;
    camera.distance = (initDist > 0.0f) ? initDist : 1.0f;
    desiredDistance = camera.distance;
    camera.hfovDeg = (initHfov > 0.0f) ? initHfov : 50.0f;
    camera.pivot[0] = initPivotX;
    camera.pivot[1] = initPivotY;
    camera.pivot[2] = (initPivotZ > 0.0f) ? initPivotZ : 1.05f;

    keyframes.clear();
    if (!initKeyframesJson.empty()) {
        try {
            nlohmann::json arr = nlohmann::json::parse(initKeyframesJson);
            for (auto& kf : arr) {
                CameraKeyframe ck;
                // Stored/exported "f" is 1-based (see getResultKeyframesJson()) - convert back to
                // the internal 0-based frame index everything else here uses.
                ck.frame = kf.value("f", 1) - 1;
                ck.azimuth = kf.value("az", 0.0f);
                ck.elevation = kf.value("el", 0.0f);
                ck.distance = kf.value("dist", camera.distance);
                keyframes.push_back(ck);
            }
            std::sort(keyframes.begin(), keyframes.end(),
                      [](const CameraKeyframe& a, const CameraKeyframe& b) { return a.frame < b.frame; });
        } catch (...) {
            keyframes.clear();
        }
    }

    depthJobStarted.store(false);
    buildStarted.store(false);
    buildDone.store(false);
    buildFailed.store(false);
    buildError.clear();
    cloudBin = PointCloudBin();
    currentVBOFrame = -1;

    // A shader load failure earlier this session (e.g. before the .vs/.fs files were placed next
    // to the exe) must not permanently block retrying - ensureGLResources() only tries once per
    // "session" of this flag being false, so reset it every time the editor is (re)opened.
    shaderProgram = 0;
    shaderLoadAttempted = false;

    active = true;
    setBuildStatus("Connecting to ComfyUI...");

    // The depth job's own upload/submit calls assume ComfyUI is already up and connected - true
    // whenever the user has already generated at least once this session, but NOT guaranteed
    // otherwise (unlike the normal Generate flow, which always runs this check first). Doing it
    // here too means "Edit CAM" works standalone. Blocking (server start + connect retries can
    // take up to 300s), so it runs on its own thread rather than the button-click/GL thread.
    if (readyThread && readyThread->joinable()) readyThread->join();
    readyThread = std::make_unique<std::thread>(
        &CameraPathEditor::ensureReadyAndStartDepthJobThreadFunc, this, controlVideoPath, totalFrames);
}

void CameraPathEditor::ensureReadyAndStartDepthJobThreadFunc(std::string videoPath, int frameCount) {
    bool ready = mainvideogenroom->ensureComfyUIReady([this](const std::string& status) {
        setBuildStatus(status);
    });
    if (!ready) {
        failBuild("Could not connect to ComfyUI server");
        return;
    }

    if (comfy->extractCameraWarpDepthPreview(videoPath, frameCount)) {
        depthJobStarted.store(true);
    } else {
        failBuild("Could not start depth extraction (a depth job may already be running)");
    }
}

void CameraPathEditor::close() {
    // Deliberately NOT joining readyThread/buildThread here - ensureComfyUIReady() can block for
    // up to 300s waiting on the server, and Cancel must be instant regardless of what phase
    // things are in. Any in-flight thread just keeps running in the background and its result is
    // silently ignored (active is already false by the time it would report anything); open()
    // joins whatever's left over before starting a fresh attempt, and the destructor joins on
    // actual app shutdown.
    active = false;
    // Both Apply and Cancel route through here - if the user managed to click either while
    // mid-drag, relative mouse mode (which hides the cursor) must be released or the cursor stays
    // invisible/captured for the rest of the app after this modal closes.
    if (dragging) {
        dragging = false;
        SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, false);
    }
}

bool CameraPathEditor::consumeApplyRequested() {
    bool r = applyRequested;
    applyRequested = false;
    return r;
}

std::string CameraPathEditor::getResultKeyframesJson() const {
    if (keyframes.empty()) return "";
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& kf : keyframes) {
        nlohmann::json obj;
        // kf.frame is the internal 0-based scrubFrame it was captured at (frame 0 = the first
        // video frame) - matches every other internal use (timeline tick positioning, scrubTo()'s
        // clamping) but the timeline's own "Frame %d / %d" display already converts to 1-based for
        // the user, and the exported frame number needs to match that same 1-based convention.
        obj["f"] = kf.frame + 1;
        obj["az"] = kf.azimuth;
        obj["el"] = kf.elevation;
        obj["dist"] = kf.distance;
        // Deliberately omit vs/px/py/pz rather than sending them as 0: crossview_warp_node.py's
        // _parse_keyframes() does `kf.get("pz", default_pivot[2])` per keyframe, where
        // default_pivot is the RESOLVED static pivot (pivot_override's pivot_x/y/z widgets, e.g.
        // (0,0,1.05)) - omitting the key lets each keyframe inherit that correctly. Explicitly
        // sending "pz":0 (the old behavior) overrode it to 0 on every single keyframe, forcing
        // pivot=(0,0,0) - which makes eye == pivot == target for every az/el/dist (see
        // _orbit_C_tgt: eye = pivot + dist*(R_orbit @ (-pivot)), and with pivot=0 that's always
        // just pivot itself), a degenerate lookAt() on every keyframed frame. This, not stale
        // keyframe data or dolly distance, was the actual cause of ComfyUI's recurring
        // "Singular matrix" crash in keyframe mode.
        arr.push_back(obj);
    }
    return arr.dump();
}

void CameraPathEditor::setBuildStatus(const std::string& text) {
    std::lock_guard<std::mutex> lock(buildStatusMutex);
    buildStatusText = text;
}

void CameraPathEditor::failBuild(const std::string& err) {
    std::lock_guard<std::mutex> lock(buildStatusMutex);
    buildFailed.store(true);
    buildError = err;
}

// ============================================================================
// Depth job polling + point-cloud build orchestration
// ============================================================================

void CameraPathEditor::pollDepthJobAndMaybeStartBuild() {
    if (!active || !comfy || buildStarted.load() || buildDone.load() || buildFailed.load()) return;
    if (!depthJobStarted.load()) return;

    ComfyUIManager::DepthPreviewStatus st = comfy->getDepthPreviewStatus();
    if (st.failed) {
        failBuild("Depth extraction failed: " + st.error);
        return;
    }
    if (!st.done) {
        setBuildStatus(st.statusText.empty() ? "Extracting depth..." : st.statusText);
        return;
    }

    // Depth job just finished - kick off the local point-cloud build on its own thread.
    buildStarted.store(true);
    std::string depthMetricPath = st.depthMetricPath;
    int frameCount = totalFrames;
    if (buildThread && buildThread->joinable()) buildThread->join();
    buildThread = std::make_unique<std::thread>(
        &CameraPathEditor::buildPointCloudsThreadFunc, this, depthMetricPath, frameCount);
}

void CameraPathEditor::buildPointCloudsThreadFunc(std::string depthMetricPath, int frameCount) {
    setBuildStatus("Decoding video frames...");

    std::vector<std::vector<uint8_t>> rgbFrames;
    int rgbW = 0, rgbH = 0;
    if (!mainvideogenroom->decodeVideoFramesRGBA(controlVideoPath, frameCount, rgbFrames, rgbW, rgbH)) {
        failBuild("Failed to decode input video frames locally");
        return;
    }

    setBuildStatus("Loading metric depth...");
    // Written by EwocMogeMetricExport (src/custom_nodes/EWOCvj2-MogeMetricExport): magic "MOGZ",
    // then int32 numFrames/H/W + float32 fx(px), then numFrames*H*W float32 metric depth values
    // (metres, -1 = invalid/masked) - MoGe's own raw depth, straight from the model, no
    // percentile normalization. Replaces the old approach of decoding a rendered, percentile-
    // normalized disparity PNG and inverting it: that discarded the real metric scale entirely,
    // which is exactly why this preview's orbit never matched what CrossViewWarp actually did at
    // generation time (see Camera3D.h's OrbitCamera::pivot comment for the other half of that fix).
    std::ifstream metricFile(depthMetricPath, std::ios::binary);
    if (!metricFile) {
        failBuild("Failed to open metric depth file: " + depthMetricPath);
        return;
    }
    char magic[4] = {0};
    int32_t metricB = 0, metricH = 0, metricW = 0;
    float metricFx = 0.0f;
    metricFile.read(magic, 4);
    metricFile.read((char*)&metricB, 4);
    metricFile.read((char*)&metricH, 4);
    metricFile.read((char*)&metricW, 4);
    metricFile.read((char*)&metricFx, 4);
    if (!metricFile || std::memcmp(magic, "MOGZ", 4) != 0 || metricB <= 0 || metricH <= 0 || metricW <= 0) {
        failBuild("Invalid metric depth file: " + depthMetricPath);
        return;
    }
    size_t frameFloats = (size_t)metricH * (size_t)metricW;
    std::vector<float> metricZ(frameFloats * (size_t)metricB);
    metricFile.read((char*)metricZ.data(), (std::streamsize)(metricZ.size() * sizeof(float)));
    if (!metricFile && !metricFile.eof()) {
        failBuild("Failed to read metric depth data from " + depthMetricPath);
        return;
    }

    int numFrames = std::min((int)rgbFrames.size(), (int)metricB);
    if (numFrames <= 0) {
        failBuild("No usable frames (video/metric-depth frame count mismatch)");
        return;
    }

    // totalFrames was set in open() from the REQUESTED frame cap (videogenroom.cpp's 8n+1 snap
    // of the frames slider), not the control video's own actual length - if the control clip is
    // shorter than that cap, decodeVideoFramesRGBA() above returns fewer frames than requested,
    // and numFrames (capped the same way CrossViewWarp's own VHS_LoadVideo caps its B) ends up
    // smaller than totalFrames. Left uncorrected, a keyframe placed near the assumed end (using
    // the stale, too-large totalFrames) sits past frame B and is never reached during generation -
    // the real last generated frame lands partway through the interpolation toward it, which is
    // exactly what "orbit/zoom doesn't reach as far as I set it, by some percentage" looks like.
    // Sync totalFrames (and any already-loaded keyframes' frame indices, in case a persisted path
    // was captured against a longer video) to the real, authoritative frame count now.
    if (numFrames < totalFrames) {
        totalFrames = numFrames;
        for (auto& kf : keyframes) {
            if (kf.frame > totalFrames - 1) kf.frame = totalFrames - 1;
        }
    }

    // Downsample stride so the point count per frame lands around ~80k regardless of source
    // resolution - a preview aid, not final-render geometry.
    int stride = 1;
    {
        long long totalPixels = (long long)rgbW * rgbH;
        const long long target = 80000;
        while (stride < 64 && (totalPixels / ((long long)stride * stride)) > target) stride++;
    }
    int outW = std::max(1, rgbW / stride);
    int outH = std::max(1, rgbH / stride);
    uint32_t numPoints = (uint32_t)outW * (uint32_t)outH;

    videoAspect = (rgbH > 0) ? (float)rgbW / (float)rgbH : (16.0f / 9.0f);

    // hfov drives BOTH this unprojection and the literal loraCameraHfovN value the LoRA will use -
    // CrossViewWarp's own build() computes fx = W/(2*tan(hfov/2)) against this same clip's frame
    // width (moge_geometry's own intrinsics are only a hfov=0 fallback, never hit here since this
    // editor always sends a non-zero hfov) - so the preview camera and the real one agree exactly.
    float hfovRad = camera.hfovDeg * (float)M_PI / 180.0f;
    float focalPx = (rgbW * 0.5f) / std::tan(hfovRad * 0.5f);

    std::vector<std::vector<uint8_t>> perFrame;
    perFrame.reserve(numFrames);

    for (int fi = 0; fi < numFrames; fi++) {
        setBuildStatus("Building point cloud... frame " + std::to_string(fi + 1) +
                        "/" + std::to_string(numFrames));

        const float* zFrame = metricZ.data() + (size_t)fi * frameFloats;
        const std::vector<uint8_t>& rgb = rgbFrames[fi];

        std::vector<uint8_t> buf(numPoints * PointCloudBin::kBytesPerPoint);
        uint8_t* dst = buf.data();

        // World frame matches crossview_warp_node.py's own unprojection exactly, with NO flip:
        // source camera at the origin, scene in front at positive z, x=(u-cx)/fx*z, y=(v-cy)/fx*z
        // (+Y is DOWN, same as the source image's own row direction). OrbitCamera's eye/target
        // math (Camera3D.h) stays in this exact frame too, un-flipped - getViewMatrix() achieves
        // "+Y renders as up on screen" via the view matrix's up-vector choice instead of touching
        // any coordinate, specifically because flipping points/eye/target here mirrored the render
        // on X (flipping exactly one axis inverts handedness - see that comment for the full story).
        for (int y = 0; y < outH; y++) {
            for (int x = 0; x < outW; x++) {
                int sx = std::min(rgbW - 1, x * stride);
                int sy = std::min(rgbH - 1, y * stride);

                int dsx = std::min(metricW - 1, sx * metricW / rgbW);
                int dsy = std::min(metricH - 1, sy * metricH / rgbH);
                float z = zFrame[(size_t)dsy * metricW + dsx];

                float px, py, pz;
                if (z > 0.0f) {
                    px = (sx - rgbW * 0.5f) * z / focalPx;
                    py = (sy - rgbH * 0.5f) * z / focalPx;
                    pz = z;
                } else {
                    // Invalid/masked (EwocMogeMetricExport's -1 sentinel) - push far outside the
                    // view frustum (getProjectionMatrix()'s farZ) rather than skipping the point,
                    // since the point buffer's size is fixed per frame; the perspective projection
                    // naturally clips it, no shader-side discard needed.
                    px = 0.0f; py = 0.0f; pz = 1.0e6f;
                }

                size_t rIdx = ((size_t)sy * rgbW + sx) * 4;
                uint8_t r = (rIdx + 0 < rgb.size()) ? rgb[rIdx + 0] : 200;
                uint8_t g = (rIdx + 1 < rgb.size()) ? rgb[rIdx + 1] : 200;
                uint8_t b = (rIdx + 2 < rgb.size()) ? rgb[rIdx + 2] : 200;

                memcpy(dst, &px, 4); dst += 4;
                memcpy(dst, &py, 4); dst += 4;
                memcpy(dst, &pz, 4); dst += 4;
                *dst++ = r; *dst++ = g; *dst++ = b; *dst++ = 255;
            }
        }
        perFrame.push_back(std::move(buf));
    }

    // Auto-calibrate the STARTING pivot depth from this clip's own real metric depth (median of
    // a trimmed central region on frame 0, mirroring crossview_warp_node.py's own auto-estimate).
    // This was briefly removed after discovering that a FAR pivot makes even a pure orbit swing
    // the eye by several real metres (orbit radius = dist*|pivot|) - genuinely unphotographed
    // vantage points, i.e. parallax holes. But the LoRA's own tiny close-up default (1.05m) turned
    // out worse for typical (non-closeup) footage, for a DIFFERENT reason: with pivot this close
    // to the eye, the camera - which always looks AT the pivot - has to ROTATE almost 1:1 with the
    // azimuth angle to keep facing it (confirmed directly: 37 degrees of azimuth at pivot=1.05m
    // rotated the viewing direction ~35.6 degrees), which at hfov=50 (~25 degree half-angle) spins
    // the ENTIRE original frame out of the lens's field of view well within the LoRA's own
    // documented "reliable" zone - a worse failure (67% holes) than the far-pivot case it replaced.
    // A pivot near the actual SUBJECT's own depth keeps both effects bounded: the camera stays
    // aimed near the subject without a large rotation, and doesn't need to travel unrealistically
    // far either - which is exactly what this estimate is trying to approximate.
    if (camera.pivot[0] == 0.0f && camera.pivot[1] == 0.0f && std::abs(camera.pivot[2] - 1.05f) < 0.001f) {
        std::vector<float> centralVals;
        int cx0 = metricW / 5, cx1 = 4 * metricW / 5;
        int cy0 = metricH / 8, cy1 = 4 * metricH / 5;
        for (int yy = cy0; yy < cy1; yy++) {
            for (int xx = cx0; xx < cx1; xx++) {
                float v = metricZ[(size_t)yy * metricW + xx];  // frame 0
                if (v > 0.0f) centralVals.push_back(v);
            }
        }
        if (!centralVals.empty()) {
            std::sort(centralVals.begin(), centralVals.end());
            camera.pivot[2] = centralVals[centralVals.size() / 2];
        }
    }

    setBuildStatus("Saving point cloud...");
    std::string binPath = mainprogram->temppath + "/camera_path_pointcloud_" +
                           std::to_string((long long)(size_t)this) + ".bin";
    if (!writePointCloudBin(binPath, numPoints, perFrame)) {
        failBuild("Failed to write point-cloud cache file");
        return;
    }

    PointCloudBin bin;
    if (!openPointCloudBinHeader(binPath, bin)) {
        failBuild("Failed to reopen point-cloud cache file");
        return;
    }
    cloudBin = bin;
    buildDone.store(true);
    setBuildStatus("Ready");
}

// ============================================================================
// GL resources
// ============================================================================

void CameraPathEditor::ensureGLResources() {
#ifndef USE_GLES
    if (shaderProgram || shaderLoadAttempted) return;
    shaderLoadAttempted = true;
    shaderProgram = mainprogram->set_shader_from_files("pointcloud.vs", "pointcloud.fs");
    if (!shaderProgram) return;  // logged by set_shader_from_files() itself - don't retry every frame

    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    // Sized once for the largest possible frame (numPoints varies per build, so (re)allocate
    // lazily in uploadFrameToVBO() the first time cloudBin.numPoints is known).
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)PointCloudBin::kBytesPerPoint, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLsizei)PointCloudBin::kBytesPerPoint, (void*)12);
    glBindVertexArray(0);

#ifndef USE_GLES
    glEnable(GL_PROGRAM_POINT_SIZE);  // desktop core profile needs this for gl_PointSize to take effect
#endif
#endif
}

void CameraPathEditor::uploadFrameToVBO(int frame) {
#ifndef USE_GLES
    if (!cloudBin.valid() || frame == currentVBOFrame) return;
    std::vector<uint8_t> buf;
    if (!cloudBin.readFrame(frame, buf)) return;

    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)buf.size(), buf.data(), GL_DYNAMIC_DRAW);
    currentVBOFrame = frame;
    currentFrameCpu = std::move(buf);  // see computeHoleFraction()'s comment for why this is kept
#endif
}

float CameraPathEditor::computeHoleFraction(const OrbitCamera& cam) const {
    if (currentFrameCpu.empty()) return 0.0f;
    Mat4 view = cam.getViewMatrix();
    float aspect = (videoAspect > 0.01f) ? videoAspect : (16.0f / 9.0f);
    Mat4 proj = cam.getProjectionMatrix(aspect, 0.01f, 100.0f);
    Mat4 mvp = mat4Multiply(proj, view);
    return computeHoleFractionForBuffer(currentFrameCpu, mvp);
}

float CameraPathEditor::computeHoleFractionForBuffer(const std::vector<uint8_t>& buf, const Mat4& mvp) const {
    // Coarse - this only needs to catch "the frame collapsed into mostly holes", not reproduce
    // CrossViewWarp's own per-pixel splat exactly, and it runs every frame this editor is open
    // (via computeHoleFraction() above).
    const int gridW = 32, gridH = 18;
    std::vector<bool> covered(gridW * gridH, false);

    size_t numPoints = buf.size() / PointCloudBin::kBytesPerPoint;
    if (numPoints == 0) return 0.0f;
    size_t stride = std::max((size_t)1, numPoints / 8000);

    const uint8_t* base = buf.data();
    for (size_t i = 0; i < numPoints; i += stride) {
        const uint8_t* p = base + i * PointCloudBin::kBytesPerPoint;
        float pos[3];
        memcpy(pos, p, 12);
        // Invalid/masked source pixels are pushed to pz=1e6 (buildPointCloudsThreadFunc()'s
        // sentinel) - they never represented real content, so they can't cover anything here.
        if (pos[2] > 1.0e5f) continue;

        float clip[4];
        mat4TransformPoint(mvp, pos, clip);
        if (clip[3] <= 1e-6f) continue;  // behind the camera
        float ndcX = clip[0] / clip[3];
        float ndcY = clip[1] / clip[3];
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) continue;

        int gx = std::min(gridW - 1, std::max(0, (int)((ndcX * 0.5f + 0.5f) * gridW)));
        int gy = std::min(gridH - 1, std::max(0, (int)((ndcY * 0.5f + 0.5f) * gridH)));
        covered[(size_t)gy * gridW + gx] = true;
    }

    int coveredCount = 0;
    for (bool c : covered) if (c) coveredCount++;
    return 1.0f - (float)coveredCount / (float)(gridW * gridH);
}

void CameraPathEditor::autoEaseDistanceForHoles() {
    if (!buildDone.load() || currentFrameCpu.empty()) return;
    // The identity pose (az=el=0, dist=1) exactly reproduces the source frame - zero holes by
    // construction, not worth the per-frame cost of checking. Anything else is a real candidate.
    if (std::fabs(camera.azimuthDeg) < 0.01f && std::fabs(camera.elevationDeg) < 0.01f &&
        std::fabs(camera.distance - 1.0f) < 0.001f) {
        lastHoleFraction = 0.0f;
        return;
    }

    // kHoleThreshold (declared in the header, shared with drawSafeZoneHud()'s HUD note) picked
    // from the actual failure this responds to: dist=0.293 with a mere 14 degree orbit produced a
    // real warp render that was ~80% magenta (checked directly against the server's own
    // live-preview endpoint - see ComfyUIManager.cpp's overshoot-compensation comment). Eases
    // toward 1.0 from EITHER side - dist>1 (pulling back) can blow holes open too ("reveals area
    // the source never framed", per the node's own distance tooltip), not just dist<1 (though
    // dist<1 combined with orbit, as tested, is the case this was reported against - "dollies out").
    const float kEaseStep = 0.01f;

    // Check against the BOOSTED pose - what workflows/ltx_*/camera_warp.json's CrossViewWarp
    // node actually receives, via the shared boostAzimuth/boostElevation/boostDistance
    // (Camera3D.h) - not
    // the raw editor values. Confirmed directly as a real bug, not a theoretical one: a pose that
    // estimated only 7% holes here, unboosted, corresponds to a boosted send that rendered at
    // 80%+ magenta on the actual server - checking the unboosted pose alone made this feature
    // essentially blind to the exact failure it exists to catch.
    auto boostedOf = [](const OrbitCamera& c) {
        OrbitCamera b = c;
        b.azimuthDeg = boostAzimuth(c.azimuthDeg);
        b.elevationDeg = boostElevation(c.elevationDeg);
        b.distance = boostDistance(c.distance);
        return b;
    };

    // Second tier: once distance is already back at 1.0, pure orbit alone can still blow the
    // safe-zone's own "safe" band open into holes (confirmed directly: 37 degrees of azimuth,
    // well within the documented reliable/usable bands, produced 67% holes at dist=1.0 with the
    // near-pivot case, and up to 40% with the restored far-pivot case). Distance has nowhere
    // left to give at that point, so ease az/el back toward 0 the same way distance eases toward 1.
    const float kAngleEaseStep = 1.0f;  // degrees/frame - matches kEaseStep's "small, unhurried" feel
    const bool distanceAtOne = std::fabs(camera.distance - 1.0f) < 0.001f;

    lastHoleFraction = computeHoleFraction(boostedOf(camera));
    // Testing escape hatch: still measure and report lastHoleFraction (so the HUD keeps telling
    // the truth about the current pose) but skip every correction below - see easingEnabled's
    // doc comment in the header.
    if (!easingEnabled) return;
    if (lastHoleFraction > kHoleThreshold) {
        if (!distanceAtOne) {
            // Too many holes right now - back off toward 1.0 (the one distance guaranteed to be
            // hole-free at any az/el, since it's what the source camera itself already saw).
            if (camera.distance < 1.0f) camera.distance = std::min(1.0f, camera.distance + kEaseStep);
            else if (camera.distance > 1.0f) camera.distance = std::max(1.0f, camera.distance - kEaseStep);
        } else {
            // Distance has already given all it can (sitting at 1.0) and holes are still over
            // threshold - the orbit angle itself is now the remaining problem. Ease az/el toward
            // 0 (the identity view direction, also guaranteed hole-free) the same way distance
            // eases toward 1.0, independently on each axis so a pure-azimuth orbit doesn't also
            // drag elevation back for no reason.
            if (camera.azimuthDeg != 0.0f) {
                float step = (camera.azimuthDeg > 0.0f) ? -kAngleEaseStep : kAngleEaseStep;
                camera.azimuthDeg = (std::fabs(camera.azimuthDeg) <= kAngleEaseStep) ? 0.0f : camera.azimuthDeg + step;
            }
            if (camera.elevationDeg != 0.0f) {
                float step = (camera.elevationDeg > 0.0f) ? -kAngleEaseStep : kAngleEaseStep;
                camera.elevationDeg = (std::fabs(camera.elevationDeg) <= kAngleEaseStep) ? 0.0f : camera.elevationDeg + step;
            }
        }
    } else {
        // Holes are fine right now, but distance and/or az/el aren't at what the user actually
        // asked for (desired*) - either they never needed easing to begin with, or were backed
        // off earlier and the user has since moved somewhere more forgiving. Try creeping one
        // step toward each desired value, but verify THAT step first rather than committing
        // blind: if the candidate still opens up holes (a step can cross back over the threshold
        // even though the current position is fine), stay put this frame and try again next frame.
        // Distance creeps first (mirrors the primary easing order above); az/el only creep once
        // distance has nothing left to gain from creeping, so the two tiers don't fight each other.
        if (std::fabs(camera.distance - desiredDistance) > 0.0005f) {
            float step = (desiredDistance > camera.distance) ? kEaseStep : -kEaseStep;
            float candidate = camera.distance + step;
            bool overshoot = (step > 0.0f) ? (candidate > desiredDistance) : (candidate < desiredDistance);
            if (overshoot) candidate = desiredDistance;

            OrbitCamera trial = camera;
            trial.distance = candidate;
            if (computeHoleFraction(boostedOf(trial)) <= kHoleThreshold) {
                camera.distance = candidate;
            }
        } else {
            bool azDone = std::fabs(camera.azimuthDeg - desiredAzimuth) <= 0.05f;
            bool elDone = std::fabs(camera.elevationDeg - desiredElevation) <= 0.05f;
            if (!azDone) {
                float step = (desiredAzimuth > camera.azimuthDeg) ? kAngleEaseStep : -kAngleEaseStep;
                float candidate = camera.azimuthDeg + step;
                bool overshoot = (step > 0.0f) ? (candidate > desiredAzimuth) : (candidate < desiredAzimuth);
                if (overshoot) candidate = desiredAzimuth;

                OrbitCamera trial = camera;
                trial.azimuthDeg = candidate;
                if (computeHoleFraction(boostedOf(trial)) <= kHoleThreshold) {
                    camera.azimuthDeg = candidate;
                }
            } else if (!elDone) {
                float step = (desiredElevation > camera.elevationDeg) ? kAngleEaseStep : -kAngleEaseStep;
                float candidate = camera.elevationDeg + step;
                bool overshoot = (step > 0.0f) ? (candidate > desiredElevation) : (candidate < desiredElevation);
                if (overshoot) candidate = desiredElevation;

                OrbitCamera trial = camera;
                trial.elevationDeg = candidate;
                if (computeHoleFraction(boostedOf(trial)) <= kHoleThreshold) {
                    camera.elevationDeg = candidate;
                }
            }
        }
    }
}

// ============================================================================
// Timeline / keyframes
// ============================================================================

void CameraPathEditor::scrubTo(int frame) {
    if (totalFrames <= 0) return;
    if (frame < 0) frame = 0;
    if (frame >= totalFrames) frame = totalFrames - 1;
    scrubFrame = frame;
    interpolateCameraAtScrub();
    if (buildDone.load()) uploadFrameToVBO(scrubFrame);
}

void CameraPathEditor::interpolatePoseAtFrame(int frame, float& outAz, float& outEl, float& outDist) const {
    if (keyframes.size() < 2) return;

    // CrossViewWarp's own keyframe interpolation is fixed to linear (see the LORA_WIRING_CROSSVIEW
    // branch in ComfyUIManager.cpp) - match that here so the preview reflects what generation
    // will actually do.
    const CameraKeyframe* before = nullptr;
    const CameraKeyframe* after = nullptr;
    for (const auto& kf : keyframes) {
        if (kf.frame <= frame) before = &kf;
        if (kf.frame >= frame && !after) after = &kf;
    }
    if (!before) before = &keyframes.front();
    if (!after) after = &keyframes.back();

    if (before == after) {
        outAz = before->azimuth;
        outEl = before->elevation;
        outDist = before->distance;
        return;
    }

    float t = (float)(frame - before->frame) / (float)std::max(1, after->frame - before->frame);
    t = std::max(0.0f, std::min(1.0f, t));
    outAz = before->azimuth + (after->azimuth - before->azimuth) * t;
    outEl = before->elevation + (after->elevation - before->elevation) * t;
    outDist = before->distance + (after->distance - before->distance) * t;
}

void CameraPathEditor::interpolateCameraAtScrub() {
    if (keyframes.size() < 2) return;
    interpolatePoseAtFrame(scrubFrame, camera.azimuthDeg, camera.elevationDeg, camera.distance);
    // this frame's (interpolated or held) value, not a live-drag target
    desiredAzimuth = camera.azimuthDeg;
    desiredElevation = camera.elevationDeg;
    desiredDistance = camera.distance;
}

void CameraPathEditor::addKeyframeAtScrub() {
    for (auto it = keyframes.begin(); it != keyframes.end(); ++it) {
        if (it->frame == scrubFrame) {
            it->azimuth = camera.azimuthDeg;
            it->elevation = camera.elevationDeg;
            it->distance = camera.distance;
            return;
        }
    }
    CameraKeyframe kf;
    kf.frame = scrubFrame;
    kf.azimuth = camera.azimuthDeg;
    kf.elevation = camera.elevationDeg;
    kf.distance = camera.distance;
    keyframes.push_back(kf);
    std::sort(keyframes.begin(), keyframes.end(),
              [](const CameraKeyframe& a, const CameraKeyframe& b) { return a.frame < b.frame; });
}

void CameraPathEditor::deleteSelectedKeyframe() {
    if (selectedKeyframe < 0 || selectedKeyframe >= (int)keyframes.size()) return;
    keyframes.erase(keyframes.begin() + selectedKeyframe);
    selectedKeyframe = -1;
}

void CameraPathEditor::clearAllKeyframes() {
    keyframes.clear();
    selectedKeyframe = -1;
}

// ============================================================================
// Input
// ============================================================================

static const float kOrbitSensitivity = 0.4f;
static const float kPanSensitivity = 0.0025f;
static const float kDollySensitivity = 0.03f;
static const float kPivotDepthSensitivity = 0.05f;

void CameraPathEditor::handleOrbitInput() {
    bool altHeld = (SDL_GetModState() & SDL_KMOD_ALT) != 0;
    bool ctrlHeld = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
    // mainprogram->leftmouse/middlemouse are ONE-SHOT "a click just completed" flags - only true
    // for the single frame a button-up event fires - never true while a button is actually held
    // down, so they can't drive a drag gesture at all. leftmousedown is the real "currently held"
    // state and works correctly for that. middlemousedown does NOT, though: start.cpp's per-frame
    // cleanup (the same block that clears the genuinely one-shot flags) force-resets it to false
    // every single frame regardless of whether the button is still physically down - unlike
    // leftmousedown, which that same block leaves alone. So middlemousedown only ever reads true
    // for the one frame right after the button-down event, then gets wiped, which is exactly the
    // "hiccups instead of a continuous drag" symptom pan had. Querying SDL's own live button state
    // directly sidesteps this app-state quirk entirely for both buttons, rather than depending on
    // this codebase's own (inconsistent between the two buttons) bookkeeping of it.
    SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(nullptr, nullptr);
    bool wantOrbit = altHeld && (mouseButtons & SDL_BUTTON_LMASK);
    bool wantPan = altHeld && (mouseButtons & SDL_BUTTON_MMASK);
    bool wantDrag = wantOrbit || wantPan;

    if (!dragging && wantDrag && viewportBox && viewportBox->in()) {
        // Only the *start* of a drag needs the cursor inside the viewport - once relative mode
        // takes over below, absolute cursor position (and therefore viewportBox->in()) stops
        // being meaningful, which is exactly what fixes "orbiting does nothing once the cursor
        // drifts past the viewport edge, or if you release outside it": the drag now lives on
        // its own dragging/draggingIsOrbit state instead of re-testing box membership every frame.
        dragging = true;
        draggingIsOrbit = wantOrbit;
        SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, true);  // hides the cursor and
                                                                         // switches to relative deltas
        float discardX, discardY;
        SDL_GetRelativeMouseState(&discardX, &discardY);  // clear whatever accumulated before this
    }

    if (dragging) {
        float relX = 0.0f, relY = 0.0f;
        SDL_GetRelativeMouseState(&relX, &relY);
        if (draggingIsOrbit) {
            camera.azimuthDeg -= relX * kOrbitSensitivity;
            camera.wrapAzimuth();
            camera.elevationDeg -= relY * kOrbitSensitivity;
            camera.clampElevation();
            // This IS the user's actual intent from here on - autoEaseDistanceForHoles() may
            // still temporarily hold az/el away from this to dodge holes, but will keep trying to
            // creep back toward whatever was explicitly dragged here (mirrors desiredDistance).
            desiredAzimuth = camera.azimuthDeg;
            desiredElevation = camera.elevationDeg;
        } else {
            // Direct pivot[0]/[1] shift in the same Y-down, pixel-aligned frame the point cloud
            // and pivot both already live in, THEN auto-compensate azimuth/elevation so the
            // camera keeps looking the same direction afterward. Moving the pivot ALONE isn't a
            // real pan here: target=pivot always (keep_source_aim=false, matching the real
            // generation), and forward = target-eye = dist*(Ry(-az)@Rx(-el)@pivot) is direction-
            // only (dist cancels on normalize) - so shifting pivot changes what the camera looks
            // AT, i.e. re-aims it, rather than translating the view. Reported directly: "alt+
            // middlemouse works totally wrong changes view direction". solveAzElForForward()
            // (Camera3D.h) solves for the az/el that restore the pre-pan looking direction
            // against the new pivot, so the drag reads as a real pan (framing shifts, camera
            // keeps facing the same way) instead of a disorienting re-aim.
            float oldEye[3], oldTarget[3];
            camera.getEyeAndTargetServerFrame(oldEye, oldTarget);
            float oldForward[3] = {oldTarget[0] - oldEye[0], oldTarget[1] - oldEye[1], oldTarget[2] - oldEye[2]};

            float scale = std::max(camera.pivot[2], 0.1f) * kPanSensitivity;
            camera.pivot[0] += relX * scale;
            camera.pivot[1] += relY * scale;

            float newAz, newEl;
            if (camera.solveAzElForForward(camera.pivot, oldForward, camera.azimuthDeg,
                                            camera.elevationDeg, newAz, newEl)) {
                camera.azimuthDeg = newAz;
                camera.wrapAzimuth();
                camera.elevationDeg = newEl;
                camera.clampElevation();
                // Panning re-aims az/el as a side effect (see comment above), but the result is
                // still the user's actual current intent, not an easing artifact - keep it in sync
                // the same way the direct orbit-drag branch does.
                desiredAzimuth = camera.azimuthDeg;
                desiredElevation = camera.elevationDeg;
            }
        }
        if (!wantDrag) {
            dragging = false;
            SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, false);
        }
    }

    if (mainprogram->mousewheel != 0.0f && viewportBox && viewportBox->in()) {
        if (ctrlHeld) {
            // pivot[2] is a real metric depth (metres), now with a direct visual analog: it's in
            // the exact same coordinate system this preview's own point cloud is built in (see
            // Camera3D.h's `pivot` comment and buildPointCloudsThreadFunc()), so pushing/pulling
            // it visibly walks the orbit centre through the actual reconstructed scene.
            camera.pivot[2] *= (1.0f - mainprogram->mousewheel * kPivotDepthSensitivity);
            if (camera.pivot[2] < 0.01f) camera.pivot[2] = 0.01f;
            if (camera.pivot[2] > 1000.0f) camera.pivot[2] = 1000.0f;
        } else {
            camera.distance *= (1.0f - mainprogram->mousewheel * kDollySensitivity);
            // Clamped to CrossViewWarp's own accepted range for this literal parameter (see
            // OrbitCamera::distance's comment) - dragging outside [0.1, 3.0] would otherwise fail
            // ComfyUI's prompt validation exactly like the unbounded azimuth did before wrapAzimuth().
            if (camera.distance < 0.1f) camera.distance = 0.1f;
            if (camera.distance > 3.0f) camera.distance = 3.0f;
            // This IS the user's actual intent from here on - autoEaseDistanceForHoles() may
            // still temporarily hold distance away from it to dodge holes, but will keep trying
            // to creep back toward whatever was explicitly dialed in here.
            desiredDistance = camera.distance;
        }
    }
}

void CameraPathEditor::handleTimelineInput() {
    if (!timelineBox || totalFrames <= 0) return;
    // mainprogram->leftmouse is a one-shot "a click just completed" flag (only true for the
    // single frame the button-up event fires - see handleOrbitInput()'s comment for the full
    // story) so a drag across the timeline never scrubbed continuously, only jumping once right
    // when the mouse was released. Querying SDL's own live button state instead tracks the drag
    // the whole time the button is held, matching the same fix already applied to orbit/pan.
    bool leftDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
    if (timelineBox->in() && leftDown) {
        // xscrtovtx() alone maps screen-X to [0, 2.0], not [-1, 1] - every other caller in this
        // codebase (bins.cpp, node.cpp) prepends "-1.0f +" to get an absolute vertex-space X;
        // omitting that here was why the scrub position was consistently offset too far right.
        float mouseVtxX = -1.0f + mainprogram->xscrtovtx((float)mainprogram->mx);
        float t = (mouseVtxX - timelineBox->vtxcoords->x1) / timelineBox->vtxcoords->w;
        t = std::max(0.0f, std::min(1.0f, t));
        scrubTo((int)std::round(t * (totalFrames - 1)));
    }
}

void CameraPathEditor::handleButtons() {
    // Apply is only shown/clickable once at least one keyframe exists - this editor is a camera
    // PATH builder, not a single-static-pose picker, so a bare orbit with nothing keyed doesn't
    // have anything meaningful to apply.
    if (!keyframes.empty() && applyButtonBox && applyButtonBox->in() && mainprogram->leftmouse) {
        applyRequested = true;
        mainprogram->leftmouse = false;
        return;
    }
    // Cancel is only shown/clickable once the point-cloud finished loading (or failed) - close()
    // itself is already safe to call mid-flight (see its own comment), this is purely a UX
    // choice: nothing to cancel out of yet while the only thing on screen is a status line.
    bool loadingComplete = buildDone.load() || buildFailed.load();
    if (loadingComplete && cancelButtonBox && cancelButtonBox->in() && mainprogram->leftmouse) {
        mainprogram->leftmouse = false;
        close();
        return;
    }
    if (addKeyframeButtonBox && addKeyframeButtonBox->in() && mainprogram->leftmouse) {
        addKeyframeAtScrub();
        mainprogram->leftmouse = false;
        return;
    }
    if (deleteKeyframeButtonBox && selectedKeyframe >= 0 &&
        deleteKeyframeButtonBox->in() && mainprogram->leftmouse) {
        deleteSelectedKeyframe();
        mainprogram->leftmouse = false;
        return;
    }
    if (clearKeyframesButtonBox && !keyframes.empty() &&
        clearKeyframesButtonBox->in() && mainprogram->leftmouse) {
        clearAllKeyframes();
        mainprogram->leftmouse = false;
        return;
    }
    if (easingToggleBox && easingToggleBox->in() && mainprogram->leftmouse) {
        easingEnabled = !easingEnabled;
        mainprogram->leftmouse = false;
        return;
    }
}

void CameraPathEditor::handle() {
    if (!active) return;

    pollDepthJobAndMaybeStartBuild();

    ensureBoxes();

    if (buildDone.load() && !buildFailed.load()) {
        handleOrbitInput();
        handleTimelineInput();
        // Runs every frame (not just while dragging) so it also reacts to poses reached via
        // keyframe scrubbing (interpolateCameraAtScrub()), not only live orbit/dolly input.
        autoEaseDistanceForHoles();
    }
    handleButtons();
}

// ============================================================================
// UI box layout (lazy, matches the rest of this app's Boxx idiom)
// ============================================================================

void CameraPathEditor::ensureBoxes() {
    if (boxesInitialized) return;
    boxesInitialized = true;

    viewportBox = new Boxx;
    viewportBox->vtxcoords->x1 = -0.65f;
    viewportBox->vtxcoords->y1 = -0.55f;
    viewportBox->vtxcoords->w = 1.3f;
    viewportBox->vtxcoords->h = 1.0f;
    viewportBox->upvtxtoscr();

    timelineBox = new Boxx;
    timelineBox->vtxcoords->x1 = -0.65f;
    timelineBox->vtxcoords->y1 = -0.68f;
    timelineBox->vtxcoords->w = 1.3f;
    timelineBox->vtxcoords->h = 0.05f;
    timelineBox->upvtxtoscr();

    // All buttons below are 2x their original size (both requested explicitly) - laid out as two
    // rows instead of one, since doubling the widths in-place would have overlapped neighbors.
    // Row 1 (keyframe management, right under the timeline): Add / Delete / Clear All, each
    // 0.4 wide, spanning the viewport's own [-0.65, 0.65] width. Row 2 (Apply / Cancel, further
    // down so they're not adjacent to the keyframe buttons): 0.3 wide each, centered.
    addKeyframeButtonBox = new Boxx;
    addKeyframeButtonBox->vtxcoords->x1 = -0.65f;
    addKeyframeButtonBox->vtxcoords->y1 = -0.80f;
    addKeyframeButtonBox->vtxcoords->w = 0.4f;
    addKeyframeButtonBox->vtxcoords->h = 0.12f;
    addKeyframeButtonBox->upvtxtoscr();

    deleteKeyframeButtonBox = new Boxx;
    deleteKeyframeButtonBox->vtxcoords->x1 = -0.23f;
    deleteKeyframeButtonBox->vtxcoords->y1 = -0.80f;
    deleteKeyframeButtonBox->vtxcoords->w = 0.4f;
    deleteKeyframeButtonBox->vtxcoords->h = 0.12f;
    deleteKeyframeButtonBox->upvtxtoscr();

    clearKeyframesButtonBox = new Boxx;
    clearKeyframesButtonBox->vtxcoords->x1 = 0.19f;
    clearKeyframesButtonBox->vtxcoords->y1 = -0.80f;
    clearKeyframesButtonBox->vtxcoords->w = 0.4f;
    clearKeyframesButtonBox->vtxcoords->h = 0.12f;
    clearKeyframesButtonBox->upvtxtoscr();

    applyButtonBox = new Boxx;
    applyButtonBox->vtxcoords->x1 = -0.31f;
    applyButtonBox->vtxcoords->y1 = -0.95f;
    applyButtonBox->vtxcoords->w = 0.3f;
    applyButtonBox->vtxcoords->h = 0.12f;
    applyButtonBox->upvtxtoscr();

    cancelButtonBox = new Boxx;
    cancelButtonBox->vtxcoords->x1 = 0.01f;
    cancelButtonBox->vtxcoords->y1 = -0.95f;
    cancelButtonBox->vtxcoords->w = 0.3f;
    cancelButtonBox->vtxcoords->h = 0.12f;
    cancelButtonBox->upvtxtoscr();

    // Top-right of the viewport, clear of the text annotations (which live at barX, the LEFT
    // edge - see drawSafeZoneHud()) and roughly level with them vertically.
    easingToggleBox = new Boxx;
    easingToggleBox->vtxcoords->x1 = 0.35f;
    easingToggleBox->vtxcoords->y1 = 0.35f;
    easingToggleBox->vtxcoords->w = 0.28f;
    easingToggleBox->vtxcoords->h = 0.07f;
    easingToggleBox->upvtxtoscr();
}

// ============================================================================
// Rendering
// ============================================================================

void CameraPathEditor::draw() {
    if (!active) return;
    ensureBoxes();

    // draw_box() normally queues into per-frame batch arrays that only actually render later in
    // the_loop() (see the flush around start.cpp:10108), well after this function returns - but
    // drawViewport() below issues its own raw, synchronous GL_POINTS draw (straight glDrawArrays
    // calls), which paints immediately. So no matter what order these are submitted in, the
    // batched dim overlay always ends up painted-over-and-after the point cloud once that later
    // flush runs - drawing it as one full-screen quad would always bury the cloud regardless of
    // code order (confirmed: forcing everything through directmode=true's synchronous path
    // instead broke the rest of this modal's rendering, which apparently depends on state only
    // set up around the batch flush's own specific directmode use). Simplest fix that doesn't
    // fight the app's rendering architecture: never paint the dim overlay over the viewport
    // rectangle in the first place - dim everything AROUND it instead of the whole screen.
    float dim[4] = {0.0f, 0.0f, 0.0f, 0.75f};
    float vpX0 = viewportBox->vtxcoords->x1, vpX1 = vpX0 + viewportBox->vtxcoords->w;
    float vpY0 = viewportBox->vtxcoords->y1, vpY1 = vpY0 + viewportBox->vtxcoords->h;
    draw_box(nullptr, dim, -1.0f, vpY1, 2.0f, 1.0f - vpY1, (GLuint)-1);              // above
    draw_box(nullptr, dim, -1.0f, -1.0f, 2.0f, vpY0 + 1.0f, (GLuint)-1);             // below
    draw_box(nullptr, dim, -1.0f, vpY0, vpX0 + 1.0f, vpY1 - vpY0, (GLuint)-1);       // left
    draw_box(nullptr, dim, vpX1, vpY0, 1.0f - vpX1, vpY1 - vpY0, (GLuint)-1);        // right

    // While loading (or on failure), drawViewport()'s raw synchronous GL_POINTS draw below isn't
    // called - and since the dim overlay above is deliberately kept off this exact rectangle (so
    // it never buries the live point cloud), nothing repaints it on those frames. That rectangle
    // would otherwise keep showing whatever drawViewport() last painted there - on a first open
    // that's blank, but on reopening the editor for a second time it's the PREVIOUS session's
    // final point-cloud frame, frozen in place until this session's build finishes and overwrites
    // it. Paint over it explicitly here (batched draw_box(), which flushes after drawViewport()'s
    // synchronous draw would have run anyway, so this is only ever reached on frames where
    // drawViewport() is skipped) so no stale pixels from an earlier session ever show through.
    if (buildFailed.load() || !buildDone.load()) {
        float black4[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        draw_box(nullptr, black4, viewportBox->vtxcoords->x1, viewportBox->vtxcoords->y1,
                 viewportBox->vtxcoords->w, viewportBox->vtxcoords->h, (GLuint)-1);
    }

    // Only while depth extraction / point-cloud build is actually in flight - once buildDone,
    // buildStatusText just sits at "Ready" (set at the end of the build thread) and would
    // otherwise overlap drawSafeZoneHud()'s own top-of-viewport annotations forever while idle.
    if (!buildDone.load()) drawStatus();

    if (buildFailed.load()) {
        // render_text() doesn't wrap/clip on its own (see the same issue fixed for the main
        // progress box in videogenroom.cpp) - truncate a display copy so a long reason (e.g. a
        // full local file path) can't run off the edge of this modal.
        std::string text;
        {
            std::lock_guard<std::mutex> lock(buildStatusMutex);
            text = "Camera path preview failed: " + buildError;
        }
        const size_t kMaxChars = 100;
        if (text.size() > kMaxChars) text = text.substr(0, kMaxChars - 3) + "...";
        render_text(text, red, viewportBox->vtxcoords->x1 + 0.02f, 0.0f, 0.0005f, 0.0008f);
    } else if (!buildDone.load()) {
        // Status text already drawn by drawStatus() - nothing else interactive yet.
    } else {
        drawViewport();
        drawSafeZoneHud();
        drawTimeline();
    }

    // Apply only shown once at least one keyframe exists - see handleButtons()'s comment.
    if (!keyframes.empty()) {
        draw_box(white, black, applyButtonBox, (GLuint)-1);
        render_text("Apply", white, applyButtonBox->vtxcoords->x1 + 0.04f,
                    applyButtonBox->vtxcoords->y1 + 0.04f, 0.0009f, 0.0015f);
    }

    // Cancel only shown once loading finished (or failed) - see handleButtons()'s comment.
    bool loadingComplete = buildDone.load() || buildFailed.load();
    if (loadingComplete) {
        draw_box(white, black, cancelButtonBox, (GLuint)-1);
        render_text("Cancel", white, cancelButtonBox->vtxcoords->x1 + 0.04f,
                    cancelButtonBox->vtxcoords->y1 + 0.04f, 0.0009f, 0.0015f);
    }

    if (buildDone.load() && !buildFailed.load()) {
        draw_box(white, black, addKeyframeButtonBox, (GLuint)-1);
        render_text("Add Keyframe", white, addKeyframeButtonBox->vtxcoords->x1 + 0.03f,
                    addKeyframeButtonBox->vtxcoords->y1 + 0.04f, 0.0007f, 0.0012f);

        float* delColor = (selectedKeyframe >= 0) ? white : black;
        draw_box(delColor, black, deleteKeyframeButtonBox, (GLuint)-1);
        render_text("Delete Keyframe", white, deleteKeyframeButtonBox->vtxcoords->x1 + 0.03f,
                    deleteKeyframeButtonBox->vtxcoords->y1 + 0.04f, 0.00064f, 0.0011f);

        float* clearColor = keyframes.empty() ? black : white;
        draw_box(clearColor, black, clearKeyframesButtonBox, (GLuint)-1);
        render_text("Clear All Keyframes", white, clearKeyframesButtonBox->vtxcoords->x1 + 0.03f,
                    clearKeyframesButtonBox->vtxcoords->y1 + 0.04f, 0.00056f, 0.00096f);
    }
}

void CameraPathEditor::drawStatus() {
    std::string text;
    {
        std::lock_guard<std::mutex> lock(buildStatusMutex);
        text = buildStatusText;
    }
    if (buildFailed.load()) return;  // error text drawn separately in draw()
    if (text.empty()) text = "Starting depth extraction...";
    render_text(text, white, viewportBox->vtxcoords->x1 + 0.02f,
                viewportBox->vtxcoords->y1 + viewportBox->vtxcoords->h - 0.03f, 0.00045f, 0.00075f);
}

void CameraPathEditor::drawViewport() {
#ifndef USE_GLES
    ensureGLResources();
    if (!shaderProgram || !cloudBin.valid()) return;
    uploadFrameToVBO(scrubFrame);
    if (currentVBOFrame < 0) return;

    glDrawBuffer_Back();

    // Convert the viewport Boxx's normalized vertex coords to a pixel-space GL viewport/scissor
    // so the 3D render doesn't draw over the rest of the (already dimmed) screen.
    int panelX = (int)((viewportBox->vtxcoords->x1 + 1.0f) * 0.5f * glob->w);
    int panelY = (int)((viewportBox->vtxcoords->y1 + 1.0f) * 0.5f * glob->h);
    int panelW = (int)(viewportBox->vtxcoords->w * 0.5f * glob->w);
    int panelH = (int)(viewportBox->vtxcoords->h * 0.5f * glob->h);
    if (panelW <= 0 || panelH <= 0) return;

    // Letterbox/pillarbox the actual render rectangle to the source video's own aspect ratio
    // within the (differently-shaped) UI panel, instead of stretching to fill it - using the
    // panel's own aspect ratio for the projection matrix's horizontal-to-vertical FOV conversion
    // reconstructed a vertical FOV that didn't match the video (cut off the top while the sides,
    // driven directly by hfovDeg, stayed correct). Deriving both axes from the one aspect ratio
    // that's actually correct - the video's - needs the rendered rectangle to actually have that
    // shape, not just the projection math to assume it.
    float panelAspect = (float)panelW / (float)panelH;
    int vx, vy, vw, vh;
    if (panelAspect > videoAspect) {
        // panel wider than the video -> pillarbox (bars on left/right)
        vh = panelH;
        vw = (int)(panelH * videoAspect);
        vx = panelX + (panelW - vw) / 2;
        vy = panelY;
    } else {
        // panel taller than the video -> letterbox (bars above/below)
        vw = panelW;
        vh = (int)(panelW / videoAspect);
        vx = panelX;
        vy = panelY + (panelH - vh) / 2;
    }
    if (vw <= 0 || vh <= 0) return;

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    // This is a core-profile context (#version 430 core) - there's no fixed-function fallback, so
    // leaving 0 bound for either of these (as this code used to do) means every draw_box/render_text
    // call for the rest of the frame (and beyond) has no program/VAO to draw with and silently
    // renders nothing. Save and restore both, the same way the viewport already is.
    GLint prevProgram = 0, prevVAO = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    // Confirmed via diagnostic: GL_FRAMEBUFFER_BINDING/GL_DRAW_FRAMEBUFFER_BINDING were a non-zero
    // FBO (object 4) at this point in the frame - some other room's off-screen render target,
    // still bound from whatever ran just before this. glDrawBuffer_Back() only picks which color
    // attachment to draw into *within* whatever framebuffer is currently bound - it does nothing
    // to rebind the default (window) framebuffer, so every point was being rasterized into that
    // invisible FBO instead of the screen. This is why the draw call reported zero GL errors and
    // huge, solid-red, unmissable points still never appeared - they were being drawn, just not
    // anywhere the user could see. Bind the default framebuffer explicitly and restore whatever
    // was bound before, the same save/restore pattern as viewport/program/VAO above.
    GLint prevFB = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFB);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_SCISSOR_TEST);
    glScissor(vx, vy, vw, vh);
    glViewport(vx, vy, vw, vh);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    Mat4 view = camera.getViewMatrix();
    Mat4 proj = camera.getProjectionMatrix((float)vw / (float)vh, 0.01f, 100.0f);
    Mat4 mvp = mat4Multiply(proj, view);

    glUseProgram(shaderProgram);
    GLint mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
    GLint sizeLoc = glGetUniformLocation(shaderProgram, "PointSize");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.m);
    glUniform1f(sizeLoc, 4.0f);

    glBindVertexArray(pointVAO);
    glDrawArrays(GL_POINTS, 0, (GLsizei)cloudBin.numPoints);
    glBindVertexArray((GLuint)prevVAO);
    glUseProgram((GLuint)prevProgram);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFB);
#endif
}

void CameraPathEditor::drawSafeZoneHud() {
    // The LoRA doc's own safe-zone az/el bands (formerly drawn here as two green/yellow/red
    // gauge bars) assumed its own automatic subject-pivot system - our hole-fraction calibration
    // (computeHoleFraction()/autoEaseDistanceForHoles()) measures the ACTUAL warp risk against
    // this clip's real reconstructed geometry instead, which is strictly better ground truth (and
    // self-corrects for scene layout: a closer background genuinely tolerates more az/el/dist
    // before holes appear, which the doc's fixed degree bands could never account for). With hole%
    // now the thing that actually gates easing, the gauges were redundant guidance at best and
    // misleading at worst, so removed - the numeric annotations below stay.
    float barX = viewportBox->vtxcoords->x1 + 0.02f;

    // Precise numeric readout. These are the literal values getResultAzimuth()/
    // getResultElevation()/getResultDistance() will hand back on Apply.
    //
    // Placed at the TOP of the viewport: the timeline box's own screen area draws AFTER this
    // (drawTimeline() runs after drawSafeZoneHud() in draw()) and paints its white background
    // over anything placed too low, hiding it completely (confirmed: a screenshot showed only a
    // sliver of "0.576" peeking out from under the timeline). The top of the viewport has plenty
    // of open room instead.
    float topY = viewportBox->vtxcoords->y1 + viewportBox->vtxcoords->h - 0.06f;
    char camBuf[96];
    snprintf(camBuf, sizeof(camBuf), "Camera: az=%.2f  el=%.2f  dist=%.3f",
             camera.azimuthDeg, camera.elevationDeg, camera.distance);
    render_text(camBuf, white, barX, topY, 0.0006f, 0.001f);

    // What actually reaches the server after overshoot compensation (Camera3D.h's
    // boostAzimuth/boostElevation/boostDistance) - shown right under the raw values above so the
    // gap is never hidden.
    char boostedBuf[96];
    snprintf(boostedBuf, sizeof(boostedBuf), "Sent (x%.2f): az=%.2f  el=%.2f  dist=%.3f",
             kCameraMotionMultiplier, boostAzimuth(camera.azimuthDeg),
             boostElevation(camera.elevationDeg), boostDistance(camera.distance));
    render_text(boostedBuf, yellow, barX, topY - 0.045f, 0.0006f, 0.001f);

    // Surfaces autoEaseDistanceForHoles()'s own signal - without this, distance quietly
    // resisting the scroll wheel past a certain point (while orbited) would look like a bug
    // rather than the deliberate hole-avoidance easing it actually is. kHoleThreshold is the
    // same shared constant that function itself checks against (declared in the header) - this
    // used to be a separate hardcoded 0.20f here that had already drifted out of sync with the
    // real threshold once.
    if (lastHoleFraction > 0.01f) {
        // Mirrors autoEaseDistanceForHoles()'s own two-tier order: distance eases first, and only
        // once it's already pinned at 1.0 (nothing left to give) does the orbit angle itself ease -
        // so the note should say whichever one is actually active, not always "distance". When
        // easingEnabled is off (see the toggle button below), correction is skipped entirely even
        // though lastHoleFraction is still measured honestly - say so rather than claiming an
        // easing that isn't actually happening right now.
        bool wouldEase = lastHoleFraction > kHoleThreshold;
        bool distanceAtOne = std::fabs(camera.distance - 1.0f) < 0.001f;
        const char* what = "";
        if (wouldEase) {
            what = !easingEnabled ? "  (would ease - OFF)"
                                   : (distanceAtOne ? "  (easing orbit back)" : "  (easing distance back)");
        }
        char holeBuf[64];
        snprintf(holeBuf, sizeof(holeBuf), "Est. warp holes: %.0f%%%s", lastHoleFraction * 100.0f, what);
        render_text(holeBuf, wouldEase ? yellow : white, barX, topY - 0.09f, 0.0006f, 0.001f);
    }

    // Toggle button for easingEnabled - top-right of the viewport (see ensureBoxes()), clear of
    // the readout column above. Testing tool only, not persisted across sessions.
    if (easingToggleBox) {
        draw_box(white, easingEnabled ? black : darkgreen1, easingToggleBox, (GLuint)-1);
        render_text(easingEnabled ? "Easing: ON" : "Easing: OFF", white,
                    easingToggleBox->vtxcoords->x1 + 0.015f, easingToggleBox->vtxcoords->y1 + 0.02f,
                    0.0006f, 0.001f);
    }

    // pivot has no meaningful "reliable/usable" band (it's a literal scene-metric position, not
    // an angle the LoRA was scored against) - a numeric readout instead, now that the point cloud
    // itself is built from real metric MoGe depth (see buildPointCloudsThreadFunc()): pivot[2] is
    // literally a depth in this same reconstructed scene, so Ctrl+scroll walking it through the
    // readout should visibly correspond to walking the orbit centre through the actual geometry.
    char pivotBuf[96];
    snprintf(pivotBuf, sizeof(pivotBuf), "Pivot: (%.2f, %.2f, %.2f)m", camera.pivot[0],
             camera.pivot[1], camera.pivot[2]);
    render_text(pivotBuf, white, barX, topY - 0.135f, 0.0006f, 0.001f);
    render_text("Ctrl+Scroll = pivot depth, Alt+MMB = pan", white, barX, topY - 0.18f, 0.0006f, 0.001f);
}

void CameraPathEditor::drawTimeline() {
    draw_box(white, black, timelineBox, (GLuint)-1);

    if (totalFrames > 1) {
        float t = (float)scrubFrame / (float)(totalFrames - 1);
        float markerX = timelineBox->vtxcoords->x1 + t * timelineBox->vtxcoords->w;
        draw_box(nullptr, yellow, markerX - 0.003f, timelineBox->vtxcoords->y1,
                 0.006f, timelineBox->vtxcoords->h, (GLuint)-1);

        // Hover-based selection (not click-based): Delete Keyframe should be available whenever
        // the cursor sits on a tick, with no click needed. Only reassign/clear selectedKeyframe
        // while the cursor is actually within the tick row's vertical band - moving the mouse
        // down to the Delete Keyframe button itself (outside this band) must leave the last
        // hovered selection intact so the button has something to act on.
        float mvx = -1.0f + mainprogram->xscrtovtx((float)mainprogram->mx);
        float mvy = 1.0f - mainprogram->yscrtovtx((float)mainprogram->my);
        bool inTickRow = mvy > timelineBox->vtxcoords->y1 - 0.02f && mvy < timelineBox->vtxcoords->y1;
        bool hitAny = false;

        for (int i = 0; i < (int)keyframes.size(); i++) {
            float kt = (float)keyframes[i].frame / (float)(totalFrames - 1);
            float kx = timelineBox->vtxcoords->x1 + kt * timelineBox->vtxcoords->w;
            float* col = (i == selectedKeyframe) ? purple : green;
            draw_box(nullptr, col, kx - 0.004f,
                     timelineBox->vtxcoords->y1 - 0.015f, 0.008f, 0.015f, (GLuint)-1);

            if (inTickRow && mvx > kx - 0.008f && mvx < kx + 0.008f) {
                selectedKeyframe = i;
                hitAny = true;
            }
        }
        if (inTickRow && !hitAny) selectedKeyframe = -1;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "Frame %d / %d", scrubFrame + 1, totalFrames);
    render_text(buf, white, timelineBox->vtxcoords->x1, timelineBox->vtxcoords->y1 + timelineBox->vtxcoords->h + 0.005f,
                0.00032f, 0.00055f);
}
