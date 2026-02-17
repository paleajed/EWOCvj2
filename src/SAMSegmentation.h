/**
 * SAMSegmentation.h
 *
 * Backend for SAM 3 (Segment Anything Model 3) segmentation via ComfyUI
 * SAM 3 uses open-vocabulary text prompts for unified detection,
 * segmentation, and tracking - no separate GroundingDINO needed.
 *
 * License: GPL3
 */

#ifndef SAM_SEGMENTATION_H
#define SAM_SEGMENTATION_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <cstdint>
#include "GL/gl.h"
#include "nlohmann/json.hpp"

// ============================================================================
// Structs
// ============================================================================

struct SegmentationMask {
    int id = 0;                        // Unique segment ID
    std::string label = "";            // Text label from prompt
    float confidence = 0.0f;           // Detection confidence
    std::vector<uint8_t> mask;         // Binary mask (W*H, 0 or 255)
    float bbox[4] = {0, 0, 0, 0};     // Bounding box [x1,y1,x2,y2] normalized 0-1
    bool selected = false;             // User selection state
    float outlineColor[3] = {1, 0, 0}; // RGB color for outline visualization
};

struct SegmentationResult {
    int width = 0;
    int height = 0;
    std::vector<SegmentationMask> masks;
    GLuint outlineTex = -1;            // Texture with colored outlines overlaid on input
    GLuint maskedTex = -1;             // Texture with selected masks applied (transparent BG)
};

// ============================================================================
// SAMSegmentation Class
// ============================================================================

class SAMSegmentation {
public:
    SAMSegmentation();
    ~SAMSegmentation();

    SAMSegmentation(const SAMSegmentation&) = delete;
    SAMSegmentation& operator=(const SAMSegmentation&) = delete;

    /**
     * Initialize connection to ComfyUI
     */
    bool initialize(const std::string& comfyHost = "127.0.0.1", int comfyPort = 8188);

    /**
     * Run segmentation on a single frame (threaded internally)
     */
    bool segmentFrame(const std::string& inputImagePath, const std::string& prompt);

    /**
     * Run segmentation on video (extracts first frame, segments it)
     */
    bool segmentVideo(const std::string& videoPath, const std::string& prompt);

    /**
     * Get current segmentation result
     */
    SegmentationResult getResult() const;

    /**
     * Toggle selection of a specific mask
     */
    void toggleMaskSelection(int maskId);

    /**
     * Get mask ID at a normalized position (0-1 coords)
     * @return mask ID or -1 if no mask at that position
     */
    int getMaskAtPosition(float normalizedX, float normalizedY);

    /**
     * Set mask inversion mode
     */
    void setInverted(bool inverted);
    bool isInverted() const;

    /**
     * Compose masked output from selected masks (RGBA with alpha)
     */
    void composeMaskedOutput();

    /**
     * Get masked pixel data (RGBA) and dimensions for direct PNG export
     */
    const std::vector<uint8_t>& getMaskedPixelData() const { return maskedPixelData; }
    int getMaskedPixelWidth() const { return maskedPixelWidth; }
    int getMaskedPixelHeight() const { return maskedPixelHeight; }

    /**
     * Export all video frames with mask applied as RGBA PNGs.
     * Uses per-frame propagation masks when available, otherwise static mask.
     */
    bool exportMaskedFrames(const std::string& videoPath, const std::string& outputDir,
                            std::function<void(float)> progressCallback = nullptr);

    /**
     * Whether video propagation masks are available (per-frame tracking)
     */
    bool hasPropagation() const { return !propagationMaskDir.empty() && numPropagationMasks > 0; }

    float scoreThreshold = 0.3f;          // Detection confidence threshold (0-1)

    bool isProcessing() const { return processing.load(); }
    float getProgress() const;
    std::string getStatus() const;

    void cancelSegmentation();

    /**
     * Clean up sam3 output files and temp directories
     */
    void cleanupSam3Outputs();

    /**
     * Test if ComfyUI server is reachable
     * @return non-empty string on success, empty on failure
     */
    std::string testConnection();

    /**
     * Check if new results are ready (pixel data prepared on worker thread).
     * Call uploadResultTextures() from the GL thread to create textures.
     */
    bool hasNewResults() const { return newResultsReady.load(); }

    /**
     * Upload pixel data to GL textures. MUST be called from the main GL thread.
     */
    void uploadResultTextures();

private:
    // ComfyUI HTTP helpers
    std::string httpPost(const std::string& endpoint, const std::string& data);
    std::string httpGet(const std::string& endpoint);
    bool uploadImage(const std::string& localPath, std::string& uploadedName);

    // Workflow (single-frame segmentation)
    nlohmann::json prepareSegmentationWorkflow(const std::string& imageName, const std::string& prompt);
    bool parseSegmentationOutput(const nlohmann::json& historyData);

    // Workflow (video propagation)
    nlohmann::json preparePropagationWorkflow(const std::string& videoName, const std::string& prompt);
    bool parsePropagationOutput(const nlohmann::json& historyData);
    void propagateThreadFunc(std::string videoPath, std::string prompt);

    // Outline rendering
    void renderOutlines();
    void generateOutlineColors();

    // Mask composition
    void composeMaskedOutputInternal();

    std::string comfyHost = "127.0.0.1";
    int comfyPort = 8188;
    bool initialized = false;
    bool inverted = false;
    std::atomic<bool> processing{false};
    std::atomic<bool> shouldCancel{false};
    std::string statusText = "Ready";
    float progressValue = 0.0f;
    mutable std::mutex statusMutex;

    SegmentationResult currentResult;
    mutable std::mutex resultMutex;

    // Segmentation thread
    std::unique_ptr<std::thread> segmentThread;
    void segmentThreadFunc(std::string imagePath, std::string prompt);

    // Video frame extraction
    std::string extractFirstFrame(const std::string& videoPath, const std::string& tempDir);
    float videoFps = 0.0f;
    int videoWidth = 0;
    int videoHeight = 0;

    // Input image data for composition
    std::vector<uint8_t> inputImageData;
    int inputImageWidth = 0;
    int inputImageHeight = 0;

    // Pixel data prepared on worker thread, uploaded to GL on main thread
    std::atomic<bool> newResultsReady{false};
    std::vector<uint8_t> outlinePixelData;
    int outlinePixelWidth = 0;
    int outlinePixelHeight = 0;
    std::vector<uint8_t> maskedPixelData;
    int maskedPixelWidth = 0;
    int maskedPixelHeight = 0;

    // Per-frame propagation masks, vis frames, and orig frames (stored as files on disk)
    std::string propagationMaskDir = "";
    std::string propagationVisDir = "";
    std::string propagationOrigDir = "";
    int numPropagationMasks = 0;
    int numPropagationVis = 0;
    int numPropagationOrig = 0;
    std::string firstVisPath = "";   // First visualization frame for instance separation
    std::string firstOrigPath = "";  // First original frame (VHS-decoded, for accurate vis-comparison)

    // Instance-to-palette mapping (from vis demixing) — instancePaletteColors[i] = palette index for mask i
    std::vector<int> instancePaletteColors;

    // Build set of selected palette indices for export filtering
    std::vector<int> getSelectedPaletteColors() const;

    // Load a single propagation mask/vis/orig from disk
    std::vector<uint8_t> loadPropagationMask(int frameIndex, int* outWidth = nullptr, int* outHeight = nullptr) const;
    std::vector<uint8_t> loadPropagationVis(int frameIndex, int* outWidth = nullptr, int* outHeight = nullptr) const;
    std::vector<uint8_t> loadPropagationOrig(int frameIndex, int* outWidth = nullptr, int* outHeight = nullptr) const;

    // Split combined mask into per-instance masks using visualization colors
    void splitMasksByVisualization(const std::vector<uint8_t>& combinedMask, int maskW, int maskH);

    // Output organization: sam3/<videoname>/ subdirectory under ComfyUI outputs
    std::string sam3OutputSubdir;  // e.g. "sam3/myvideo"
};

#endif // SAM_SEGMENTATION_H
