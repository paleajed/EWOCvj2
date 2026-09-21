/**
 * segmentationroom.h
 *
 * UI room for SAM 3 video segmentation and masking
 * Supports text-prompt segmentation, mask selection,
 * inversion, and HAP Alpha export with transparency.
 *
 * License: GPL3
 */

#ifndef SEGMENTATIONROOM_H
#define SEGMENTATIONROOM_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "SAMSegmentation.h"

class Boxx;
class Param;

// Menu options for segmentqtion room
typedef enum {
    SEG_BROWSEINPUT = 0,
    SEG_QUIT = 1,
} SEGMENU_OPTION;

class SegmentationRoom {
public:
    SegmentationRoom();
    ~SegmentationRoom();

    // Main render/interaction loop
    void handle();

    SAMSegmentation* samBackend = nullptr;

    Layer *prelay = nullptr;
    Layer *prelayout = nullptr;
    float preframe = 0.0f;

    // Dummy layer driving the OUTLINE/MASKED preview's own loopbox (scrub position +
    // export loop range), fully decoupled from prelay's INPUT-box loopbox (own screen
    // position, always live independent of prelay's state). Decodes the exact video
    // samBackend tracked (clippedVideoPath if segmentation ran on a sub-range,
    // otherwise inputVideoPath) so scrubbing can show the true per-frame colors -
    // vis.bin alone isn't enough for display since SAM3 tints masked pixels with the
    // object's palette color. numf is pinned to the tracked-frame count.
    Layer *trackScrubLay = nullptr;
    std::string trackScrubVideoPath = "";  // video currently loaded into trackScrubLay

    // UI Boxes
    Boxx* outlinePreviewBox = nullptr;     // Left: input + outlines
    Boxx* maskedPreviewBox = nullptr;      // Right: masked result
    Boxx* promptBox = nullptr;             // Text prompt input
    Boxx* segmentButton = nullptr;         // "SEGMENT" button
    Boxx* invertButton = nullptr;          // "INVERT" button
    Boxx* exportButton = nullptr;          // "EXPORT" button
    Boxx* progressBox = nullptr;           // Status display
    Boxx* inputBox = nullptr;              // Input video drag target
    Boxx* outputBox = nullptr;              // Output video drag from
    Param* thresholdParam = nullptr;       // Detection score threshold (0-1)

    // Textures
    GLuint outlineTex = -1;
    GLuint maskedTex = -1;
    GLuint inputTex = -1;
    int inputTexWidth = 0;
    int inputTexHeight = 0;
    GLuint outputTex = -1;
    int outputTexWidth = 0;
    int outputTexHeight = 0;
    GLuint checkerboardTex = -1;           // Transparency visualization
    GLuint trackScrubDecodeTex = -1;       // scratch DXT texture reused by decodeHapFrameToRGBA

    // Menus
    Menu* segmenu = nullptr;
    std::vector<SEGMENU_OPTION> menuoptions;

    // State
    std::string inputVideoPath = "";
    std::string clippedVideoPath = "";     // temp clip extracted for segmentation (startframe..endframe)
    int segmentedStartFrame = 0;           // startframe at time of last segmentation
    int segmentedEndFrame = 0;             // endframe at time of last segmentation
    std::string exportedpath = "";
    std::string promptstr = "";
    std::string oldpromptstr = "";
    std::vector<std::string> promptlines;
    bool inverted = false;
    bool samInstalled = false;
    bool dragging = false;

    float progressPercent = 0.0f;
    std::string progressStatus = "Ready";
    bool prevProcessing = false;       // tracks isProcessing() transition for status update

    std::atomic<bool> exporting{false};
    std::atomic<bool> exportCancelled{false};
    std::atomic<bool> exportFinishedSuccess{false};
    std::unique_ptr<std::thread> exportThread;

    void startSegmentation();
    void toggleInvert();
    void startExport(const std::string& outputPath);

    void loadFirstFramePreview(const std::string& path, bool inout);

private:
    void exportThreadFunc(std::string videoPath, std::string outputPath, int exportStartFrame, int exportEndFrame);
    void generateCheckerboard();
    void setupTrackScrubLayer(const std::string& videoPath, int numTrackedFrames);
};

extern SegmentationRoom* mainsegmentationroom;

void draw_box_letterbox_seg(Boxx* box, GLuint tex, int texWidth, int texHeight);
void draw_box_letterbox_seg_flip(Boxx* box, GLuint tex, int texWidth, int texHeight);

#endif // SEGMENTATIONROOM_H
