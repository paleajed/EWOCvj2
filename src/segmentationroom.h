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

class SegmentationRoom {
public:
    SegmentationRoom();
    ~SegmentationRoom();

    // Main render/interaction loop
    void handle();

    SAMSegmentation* samBackend = nullptr;

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

    // State
    std::string inputVideoPath = "";
    std::string exportedpath = "";
    std::string promptstr = "";
    std::string oldpromptstr = "";
    std::vector<std::string> promptlines;
    bool inverted = false;
    bool samInstalled = false;
    bool dragging = false;

    float progressPercent = 0.0f;
    std::string progressStatus = "Ready";

    std::atomic<bool> exporting{false};
    std::atomic<bool> exportCancelled{false};
    std::atomic<bool> exportFinishedSuccess{false};
    std::unique_ptr<std::thread> exportThread;

    void startSegmentation();
    void toggleInvert();
    void startExport(const std::string& outputPath);

    void loadFirstFramePreview(const std::string& path, bool inout);

private:
    void exportThreadFunc(std::string videoPath, std::string outputPath);
    void generateCheckerboard();
};

extern SegmentationRoom* mainsegmentationroom;

#endif // SEGMENTATIONROOM_H
