/**
 * SAMSegmentation.cpp
 *
 * Backend for SAM 3 segmentation via ComfyUI
 *
 * License: GPL3
 */

#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#include "SAMSegmentation.h"
#include "program.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <climits>
#include "ImageLoader.h"

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#endif

#ifdef POSIX
#include <curl/curl.h>
#endif

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
}

namespace fs = std::filesystem;

// Flip RGBA pixel data vertically (top-left origin → OpenGL bottom-left origin)
static void flipVertically(std::vector<uint8_t>& data, int width, int height) {
    int rowSize = width * 4;
    std::vector<uint8_t> temp(rowSize);
    for (int y = 0; y < height / 2; y++) {
        int topOffset = y * rowSize;
        int botOffset = (height - 1 - y) * rowSize;
        memcpy(temp.data(), &data[topOffset], rowSize);
        memcpy(&data[topOffset], &data[botOffset], rowSize);
        memcpy(&data[botOffset], temp.data(), rowSize);
    }
}

// ============================================================================
// Thread-safe Work Queue + FFmpeg PNG Helpers (for parallel export/download)
// ============================================================================

template<typename T>
struct WorkQueue {
    std::queue<T> items;
    std::mutex mtx;
    std::condition_variable canPush, canPop;
    size_t maxSize;
    bool finished = false;

    WorkQueue(size_t max) : maxSize(max) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mtx);
        canPush.wait(lock, [this]{ return items.size() < maxSize || finished; });
        if (finished) return;
        items.push(std::move(item));
        canPop.notify_one();
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx);
        canPop.wait(lock, [this]{ return !items.empty() || finished; });
        if (items.empty()) return false;
        item = std::move(items.front());
        items.pop();
        canPush.notify_one();
        return true;
    }

    void finish() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        canPush.notify_all();
        canPop.notify_all();
    }
};

// ffmpegDecodePng/ffmpegEncodePng moved to ImageLoader module

// ============================================================================
// WebSocket Progress Helper
// ============================================================================

#ifdef WINDOWS
/**
 * Open a WebSocket connection to ComfyUI for real-time progress.
 * Returns the socket handle, or INVALID_SOCKET on failure.
 */
static SOCKET wsConnect(const std::string& host, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (::connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // WebSocket upgrade handshake
    std::string upgradeRequest =
        "GET /ws?clientId=sam_progress HTTP/1.1\r\n"
        "Host: " + host + ":" + std::to_string(port) + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sock, upgradeRequest.c_str(), static_cast<int>(upgradeRequest.length()), 0);

    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(sock);
        return INVALID_SOCKET;
    }
    buffer[bytesReceived] = '\0';

    std::string response(buffer);
    if (response.find("101") == std::string::npos) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Set non-blocking mode
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    return sock;
}

/**
 * Poll the WebSocket for progress messages. Non-blocking.
 * Tracks which node is executing and resets progress on node change.
 * Only reports progress from the propagation node (node "4").
 */
static bool wsPollProgress(SOCKET sock, int& progressValue, int& progressMax,
                           std::string& currentNode,
                           std::vector<unsigned char>& frameBuffer) {
    if (sock == INVALID_SOCKET) return false;

    bool gotProgress = false;
    char buffer[4096];

    // Read available data (non-blocking)
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        frameBuffer.insert(frameBuffer.end(), buffer, buffer + bytesReceived);
    }

    // Process complete WebSocket frames
    while (frameBuffer.size() >= 2) {
        unsigned char opcode = frameBuffer[0] & 0x0F;
        bool fin = (frameBuffer[0] & 0x80) != 0;
        bool masked = (frameBuffer[1] & 0x80) != 0;
        uint64_t payloadLen = frameBuffer[1] & 0x7F;
        size_t headerLen = 2;

        if (payloadLen == 126) {
            if (frameBuffer.size() < 4) break;
            payloadLen = (static_cast<uint64_t>(frameBuffer[2]) << 8) |
                          static_cast<uint64_t>(frameBuffer[3]);
            headerLen = 4;
        } else if (payloadLen == 127) {
            if (frameBuffer.size() < 10) break;
            payloadLen = 0;
            for (int i = 0; i < 8; i++) {
                payloadLen = (payloadLen << 8) | static_cast<uint64_t>(frameBuffer[2 + i]);
            }
            headerLen = 10;
        }

        size_t maskOffset = headerLen;
        if (masked) headerLen += 4;

        size_t totalFrameLen = headerLen + static_cast<size_t>(payloadLen);
        if (frameBuffer.size() < totalFrameLen) break;

        // Extract payload
        std::string payload;
        if (payloadLen > 0) {
            payload.resize(static_cast<size_t>(payloadLen));
            for (size_t i = 0; i < payloadLen; i++) {
                unsigned char byte = frameBuffer[headerLen + i];
                if (masked) byte ^= frameBuffer[maskOffset + (i % 4)];
                payload[i] = static_cast<char>(byte);
            }
        }

        frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + totalFrameLen);

        if (opcode == 0x01 && fin && !payload.empty()) {
            try {
                auto msg = nlohmann::json::parse(payload);
                std::string type = msg.value("type", "");

                if (type == "executing" && msg.contains("data")) {
                    // New node started — reset progress and track node ID
                    auto& data = msg["data"];
                    if (data.contains("node") && !data["node"].is_null()) {
                        currentNode = data["node"].get<std::string>();
                        progressValue = 0;
                        progressMax = 0;
                    }
                } else if (type == "progress" && msg.contains("data")) {
                    auto& data = msg["data"];
                    if (data.contains("value")) progressValue = data["value"].get<int>();
                    if (data.contains("max")) progressMax = data["max"].get<int>();
                    // Only report real progress for the propagation node (node "4")
                    if (currentNode == "4") {
                        gotProgress = true;
                    }
                }
            } catch (...) {}
        } else if (opcode == 0x09) {
            // Respond to ping with pong
            std::vector<unsigned char> pong;
            pong.push_back(0x8A);
            pong.push_back(static_cast<unsigned char>(payload.size()));
            pong.insert(pong.end(), payload.begin(), payload.end());
            send(sock, reinterpret_cast<char*>(pong.data()), static_cast<int>(pong.size()), 0);
        }
    }

    return gotProgress;
}

static void wsClose(SOCKET sock) {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
}

/**
 * Progress parsed from ComfyUI log file tqdm/print output.
 */
struct ComfyLogProgress {
    int propagateCurrent = 0, propagateTotal = 0;   // propagate_in_video: N/M
    int loadingCurrent = 0, loadingTotal = 0;        // frame loading (image folder): N/M
    int processedCurrent = 0, processedTotal = 0;    // [SAM3 Video] Processed N/M frames
    int lastTqdmCurrent = 0, lastTqdmTotal = 0;     // last tqdm N/M in log (generic fallback)
    int detectionCount = -1;                         // [EWOCVJ2_SAM3_DETECT] count=N (-1 = not yet seen)
    bool detectionFailed = false;                    // true if detectionCount == 0
    bool oomFailed = false;                          // [EWOCVJ2_SAM3_OOM] seen
    int oomFramesDone = 0, oomFramesTotal = 0;       // frames completed before OOM
};

/**
 * Parse ComfyUI log file tail for progress lines:
 *   propagate_in_video:  50%|████| 364/726 [02:01<01:26, 4.16it/s]
 *   frame loading (image folder) [rank=0]:  61%|███| 443/726 [00:13<00:08, 31.96it/s]
 *   [SAM3 Video] Processed 350/726 frames
 */
static void parseComfyUILogProgress(const std::string& logPath, ComfyLogProgress& out, LONGLONG logBaseOffset = 0) {
    // Open log with sharing so ComfyUI can keep writing
    HANDLE hFile = CreateFileA(logPath.c_str(), GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Read from logBaseOffset (start of current run) or last 16KB, whichever is later
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    LONGLONG readStart = logBaseOffset;
    if (fileSize.QuadPart - readStart > 16384) readStart = fileSize.QuadPart - 16384;
    if (readStart < logBaseOffset) readStart = logBaseOffset;
    LARGE_INTEGER seekPos;
    seekPos.QuadPart = readStart;
    SetFilePointerEx(hFile, seekPos, NULL, FILE_BEGIN);

    char buffer[16385];
    DWORD bytesRead = 0;
    ReadFile(hFile, buffer, 16384, &bytesRead, NULL);
    CloseHandle(hFile);

    if (bytesRead == 0) return;
    buffer[bytesRead] = '\0';

    std::string content(buffer, bytesRead);
    int cur = 0, tot = 0;

    // Helper lambda: find last occurrence of a tqdm-style line and parse "| N/M"
    // tqdm format: tag:  XX%|bar_chars| N/M [time]
    // Must find "| " followed by a digit (the closing bar pipe), not the opening pipe
    // which at 0% is followed by spaces: 0%|          | 0/726
    auto parseTqdm = [&](const char* tag, int& outCur, int& outTot) {
        size_t pos = content.rfind(tag);
        if (pos == std::string::npos) return;
        std::string line = content.substr(pos);
        // Find "| " where the char after "| " is a digit (closing bar marker)
        size_t searchStart = 0;
        while (searchStart < line.size()) {
            size_t barPos = line.find("| ", searchStart);
            if (barPos == std::string::npos) break;
            if (barPos + 2 < line.size() && line[barPos + 2] >= '0' && line[barPos + 2] <= '9') {
                std::string afterBar = line.substr(barPos + 2);
                if (sscanf(afterBar.c_str(), "%d/%d", &cur, &tot) == 2 && tot > 0) {
                    outCur = cur;
                    outTot = tot;
                }
                return;
            }
            searchStart = barPos + 1;
        }
        // Fallback: try to find N/M pattern directly (no bar markers)
        // Some tqdm configs or non-tty modes may use simplified format
        size_t pctPos = line.find('%');
        if (pctPos != std::string::npos) {
            std::string afterPct = line.substr(pctPos);
            // Scan for first digit/digit pattern after the percentage
            for (size_t i = 1; i < afterPct.size(); i++) {
                if (afterPct[i] >= '0' && afterPct[i] <= '9') {
                    if (sscanf(afterPct.c_str() + i, "%d/%d", &cur, &tot) == 2 && tot > 0) {
                        outCur = cur;
                        outTot = tot;
                        return;
                    }
                }
            }
        }
    };

    // 1. propagate_in_video: N/M
    parseTqdm("propagate_in_video:", out.propagateCurrent, out.propagateTotal);

    // 2. frame loading — try several possible tags the VHS node might use
    const char* loadingTags[] = {
        "frame loading (image folder)",
        "Loading video",
        "Loading frames",
        "Decoding video",
        "Extracting frames",
        nullptr
    };
    for (int i = 0; loadingTags[i] && out.loadingTotal == 0; i++) {
        parseTqdm(loadingTags[i], out.loadingCurrent, out.loadingTotal);
    }

    // 3. [SAM3 Video] Processed N/M frames
    size_t procPos = content.rfind("[SAM3 Video] Processed ");
    if (procPos != std::string::npos) {
        std::string line = content.substr(procPos);
        if (sscanf(line.c_str(), "[SAM3 Video] Processed %d/%d", &cur, &tot) == 2 && tot > 0) {
            out.processedCurrent = cur;
            out.processedTotal = tot;
        }
    }

    // 4. Generic fallback: find the very last "| N/M" tqdm pattern in the entire log tail.
    //    During any phase, if the specific tag wasn't found, this catches whatever
    //    tqdm bar the current node is outputting regardless of its description text.
    {
        size_t searchStart = 0;
        while (searchStart < content.size()) {
            size_t barPos = content.find("| ", searchStart);
            if (barPos == std::string::npos) break;
            if (barPos + 2 < content.size() && content[barPos + 2] >= '0' && content[barPos + 2] <= '9') {
                if (sscanf(content.c_str() + barPos + 2, "%d/%d", &cur, &tot) == 2 && tot > 0) {
                    out.lastTqdmCurrent = cur;
                    out.lastTqdmTotal = tot;
                }
            }
            searchStart = barPos + 1;
        }
    }

    // 5. [EWOCVJ2_SAM3_DETECT] count=N — detection count from patched node
    size_t detectPos = content.rfind("[EWOCVJ2_SAM3_DETECT] count=");
    if (detectPos != std::string::npos) {
        int dCount = 0;
        if (sscanf(content.c_str() + detectPos, "[EWOCVJ2_SAM3_DETECT] count=%d", &dCount) == 1) {
            out.detectionCount = dCount;
            out.detectionFailed = (dCount == 0);
        }
    }

    // 6. [EWOCVJ2_SAM3_OOM] frames=N/M — out-of-VRAM during propagation
    size_t oomPos = content.rfind("[EWOCVJ2_SAM3_OOM] frames=");
    if (oomPos != std::string::npos) {
        int done = 0, total = 0;
        if (sscanf(content.c_str() + oomPos, "[EWOCVJ2_SAM3_OOM] frames=%d/%d", &done, &total) == 2) {
            out.oomFailed = true;
            out.oomFramesDone = done;
            out.oomFramesTotal = total;
        }
    }

    // Diagnostic: log what we found (once, when file has content)
    static bool loggedDiag = false;
    if (!loggedDiag && fileSize.QuadPart > 1000) {
        loggedDiag = true;
        std::cerr << "[SAMSeg] Log file size: " << fileSize.QuadPart << " bytes" << std::endl;
        size_t tailStart = content.size() > 300 ? content.size() - 300 : 0;
        std::string tail = content.substr(tailStart);
        std::string cleanTail;
        for (char ch : tail) {
            if (ch == '\r') cleanTail += "[CR]";
            else if (ch == '\n') cleanTail += "[LF]";
            else if (ch >= 32) cleanTail += ch;
        }
        std::cerr << "[SAMSeg] Log tail: " << cleanTail << std::endl;
        std::cerr << "[SAMSeg] Parsed: propagate=" << out.propagateCurrent << "/" << out.propagateTotal
                  << " loading=" << out.loadingCurrent << "/" << out.loadingTotal
                  << " processed=" << out.processedCurrent << "/" << out.processedTotal
                  << " lastTqdm=" << out.lastTqdmCurrent << "/" << out.lastTqdmTotal << std::endl;
    }
}
#endif

// ============================================================================
// Constructor / Destructor
// ============================================================================

SAMSegmentation::SAMSegmentation() {
}

SAMSegmentation::~SAMSegmentation() {
    if (segmentThread && segmentThread->joinable()) {
        segmentThread->join();
    }
    cleanupSam3Outputs();
}

void SAMSegmentation::cleanupSam3Outputs() {
    // Purge the sam3/ directory tree under ComfyUI outputs
    std::string sam3Dir = mainprogram->programData + "/EWOCvj2/comfyui/outputs/sam3";
    std::error_code ec;
    if (fs::exists(sam3Dir, ec)) {
        fs::remove_all(sam3Dir, ec);
        if (!ec) {
            std::cerr << "[SAMSeg] Cleaned up " << sam3Dir << std::endl;
        }
    }

    // Also clean up temp files
    std::string samTemp = mainprogram->temppath + "/sam_temp";
    if (fs::exists(samTemp, ec)) {
        fs::remove_all(samTemp, ec);
    }
    std::string samPropDir = mainprogram->temppath + "/sam_propagation_masks";
    if (fs::exists(samPropDir, ec)) {
        fs::remove_all(samPropDir, ec);
    }
    std::string samVisDir = mainprogram->temppath + "/sam_propagation_vis";
    if (fs::exists(samVisDir, ec)) {
        fs::remove_all(samVisDir, ec);
    }
    std::string samOrigDir = mainprogram->temppath + "/sam_propagation_orig";
    if (fs::exists(samOrigDir, ec)) {
        fs::remove_all(samOrigDir, ec);
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool SAMSegmentation::initialize(const std::string& host, int port) {
    comfyHost = host;
    comfyPort = port;
    initialized = true;
    return true;
}

// ============================================================================
// Cancel
// ============================================================================

void SAMSegmentation::cancelSegmentation() {
    if (!processing.load()) return;

    shouldCancel.store(true);

    // Tell ComfyUI to interrupt the current prompt and clear the queue
    nlohmann::json body = nlohmann::json::object();
    httpPost("/interrupt", body.dump());

    // Clear pending prompts from the ComfyUI queue
    nlohmann::json clearBody;
    clearBody["clear"] = true;
    httpPost("/queue", clearBody.dump());

    std::lock_guard<std::mutex> lock(statusMutex);
    statusText = "Cancelling...";
}

// ============================================================================
// Segmentation Entry Points
// ============================================================================

bool SAMSegmentation::segmentFrame(const std::string& inputImagePath, const std::string& prompt) {
    if (!initialized) {
        std::cerr << "[SAMSeg] Not initialized" << std::endl;
        return false;
    }

    // If still processing, cancel and wait for the old thread to finish
    if (processing.load()) {
        cancelSegmentation();
    }
    if (segmentThread && segmentThread->joinable()) {
        segmentThread->join();
    }

    // Clear any leftover prompts in the ComfyUI queue before starting fresh
    nlohmann::json clearBody;
    clearBody["clear"] = true;
    httpPost("/queue", clearBody.dump());

    shouldCancel.store(false);
    processing.store(true);
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Starting segmentation...";
        progressValue = 0.0f;
    }

    segmentThread = std::make_unique<std::thread>(&SAMSegmentation::segmentThreadFunc,
                                                   this, inputImagePath, prompt);
    return true;
}

bool SAMSegmentation::segmentVideo(const std::string& videoPath, const std::string& prompt) {
    if (!initialized) return false;

    // If still processing, cancel and wait for the old thread to finish
    if (processing.load()) {
        cancelSegmentation();
    }
    if (segmentThread && segmentThread->joinable()) {
        segmentThread->join();
    }

    // Clear any leftover prompts in the ComfyUI queue before starting fresh
    nlohmann::json clearBody;
    clearBody["clear"] = true;
    httpPost("/queue", clearBody.dump());

    shouldCancel.store(false);
    processing.store(true);
    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Starting video propagation...";
        progressValue = 0.0f;
    }

    segmentThread = std::make_unique<std::thread>(&SAMSegmentation::propagateThreadFunc,
                                                   this, videoPath, prompt);
    return true;
}

// ============================================================================
// Segmentation Thread
// ============================================================================

void SAMSegmentation::segmentThreadFunc(std::string imagePath, std::string prompt) {
    // Set output subdirectory from image filename (e.g. "sam3/myframe")
    sam3OutputSubdir = "sam3/" + fs::path(imagePath).stem().string();

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Loading input image...";
        progressValue = 5.0f;
    }

    // Load input image data for later composition
    auto imgPixels = ImageLoader::loadImageRGBA(imagePath, &inputImageWidth, &inputImageHeight);
    if (imgPixels.empty()) {
        std::cerr << "[SAMSeg] Failed to load image: " << imagePath << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to load image";
        }
        processing.store(false);
        return;
    }
    inputImageData = std::move(imgPixels);
    flipVertically(inputImageData, inputImageWidth, inputImageHeight);

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Uploading image to ComfyUI...";
        progressValue = 15.0f;
    }

    // Upload image to ComfyUI
    std::string uploadedName;
    if (!uploadImage(imagePath, uploadedName)) {
        std::cerr << "[SAMSeg] Failed to upload image" << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to upload image";
        }
        processing.store(false);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Building workflow...";
        progressValue = 25.0f;
    }

    // Build workflow JSON
    nlohmann::json workflow = prepareSegmentationWorkflow(uploadedName, prompt);

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Submitting to ComfyUI...";
        progressValue = 30.0f;
    }

    // Submit workflow
    nlohmann::json submitData;
    submitData["prompt"] = workflow;
    std::string response = httpPost("/prompt", submitData.dump());
    if (response.empty()) {
        std::cerr << "[SAMSeg] Failed to submit workflow" << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to submit workflow";
        }
        processing.store(false);
        return;
    }

    // Parse prompt ID
    std::string promptId;
    try {
        auto respJson = nlohmann::json::parse(response);
        promptId = respJson["prompt_id"].get<std::string>();
    } catch (...) {
        std::cerr << "[SAMSeg] Failed to parse submit response" << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to parse response";
        }
        processing.store(false);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Segmenting... (model loading may take a moment)";
        progressValue = 40.0f;
    }

    // Poll for completion
    bool completed = false;
    int pollCount = 0;
    while (!completed && pollCount < 600) {  // Up to 5 minutes
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        pollCount++;

        if (shouldCancel.load()) break;

        std::string historyResp = httpGet("/history/" + promptId);
        if (historyResp.empty()) continue;

        try {
            auto historyJson = nlohmann::json::parse(historyResp);
            if (historyJson.contains(promptId)) {
                auto& promptHistory = historyJson[promptId];
                if (promptHistory.contains("outputs") && !promptHistory["outputs"].empty()) {
                    completed = true;

                    {
                        std::lock_guard<std::mutex> lock(statusMutex);
                        statusText = "Processing results...";
                        progressValue = 80.0f;
                    }

                    parseSegmentationOutput(promptHistory);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[SAMSeg] Poll error: " << e.what() << std::endl;
        }

        if (!completed) {
            std::lock_guard<std::mutex> lock(statusMutex);
            int secs = (pollCount + 1) / 2;
            // Smooth progress: 40% to 75% over time (asymptotic)
            float timeProgress = 1.0f - std::exp(-(float)secs / 60.0f);
            progressValue = 40.0f + timeProgress * 35.0f;
            statusText = "Segmenting... (" + std::to_string(secs) + "s)";
        }
    }

    if (shouldCancel.load()) {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Cancelled";
        progressValue = 0.0f;
        processing.store(false);
        return;
    }

    if (!completed) {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Segmentation timed out";
        processing.store(false);
        return;
    }

    // Generate outline visualization and masked output (pixel data only, no GL calls)
    generateOutlineColors();
    renderOutlines();
    composeMaskedOutputInternal();

    // Signal that pixel data is ready for GL upload on main thread
    newResultsReady.store(true);

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Ready";
        progressValue = 100.0f;
    }

    processing.store(false);
}

// ============================================================================
// Video Propagation Thread
// ============================================================================

void SAMSegmentation::propagateThreadFunc(std::string videoPath, std::string prompt) {
    // Set output subdirectory from video filename (e.g. "sam3/myvideo")
    sam3OutputSubdir = "sam3/" + fs::path(videoPath).stem().string();

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Extracting first frame...";
        progressValue = 0.0f;
    }

    // Extract first frame for inputImageData (needed for preview composition)
    std::string tempDir = mainprogram->temppath + "/sam_temp";
    fs::create_directories(tempDir);
    std::string framePath = extractFirstFrame(videoPath, tempDir);
    if (framePath.empty()) {
        std::cerr << "[SAMSeg] Failed to extract first frame" << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to extract first frame";
        }
        processing.store(false);
        return;
    }

    // Load first frame into inputImageData for composition
    auto framePixels = ImageLoader::loadImageRGBA(framePath, &inputImageWidth, &inputImageHeight);
    if (framePixels.empty()) {
        std::cerr << "[SAMSeg] Failed to load first frame: " << framePath << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to load first frame";
        }
        processing.store(false);
        return;
    }
    inputImageData = std::move(framePixels);
    flipVertically(inputImageData, inputImageWidth, inputImageHeight);

    // Use the video path directly — VHS_LoadVideoPath reads from any filesystem path
    std::string uploadedName = videoPath;

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Building propagation workflow...";
        progressValue = 0.0f;
    }

    // Build propagation workflow
    nlohmann::json workflow = preparePropagationWorkflow(uploadedName, prompt);

    // Connect WebSocket BEFORE submitting so we don't miss early messages
#ifdef WINDOWS
    SOCKET wsSock = wsConnect(comfyHost, comfyPort);
    std::vector<unsigned char> wsFrameBuffer;
    int wsProgressVal = 0, wsProgressMax = 0;
    std::string wsCurrentNode;
#endif

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Submitting propagation workflow...";
        progressValue = 0.0f;
    }

    // Record log file size BEFORE submitting so we only parse new content
#ifdef WINDOWS
    std::string comfyLogPath = mainprogram->temppath + "/comfyui_output.log";
    LONGLONG logBaseOffset = 0;
    {
        HANDLE hLog = CreateFileA(comfyLogPath.c_str(), GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLog != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER sz;
            GetFileSizeEx(hLog, &sz);
            logBaseOffset = sz.QuadPart;
            CloseHandle(hLog);
        }
    }
#endif

    // Submit workflow (client_id must match WebSocket clientId for progress messages)
    nlohmann::json submitData;
    submitData["prompt"] = workflow;
    submitData["client_id"] = "sam_progress";
    std::string response = httpPost("/prompt", submitData.dump());
    if (response.empty()) {
        std::cerr << "[SAMSeg] Failed to submit propagation workflow" << std::endl;
#ifdef WINDOWS
        wsClose(wsSock);
#endif
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to submit workflow";
        }
        processing.store(false);
        return;
    }

    // Parse prompt ID
    std::string promptId;
    try {
        auto respJson = nlohmann::json::parse(response);
        promptId = respJson["prompt_id"].get<std::string>();
    } catch (...) {
        std::cerr << "[SAMSeg] Failed to parse submit response" << std::endl;
#ifdef WINDOWS
        wsClose(wsSock);
#endif
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "Failed to parse response";
        }
        processing.store(false);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Waiting for ComfyUI...";
        progressValue = 0.0f;
    }

    // Poll for completion
    bool completed = false;
    bool propagateFailed = false;   // set when ComfyUI reports execution error
    int pollCount = 0;
    std::string prevNode;
    int nodeStartCount = 0;
#ifdef WINDOWS
    ComfyLogProgress logProgress;
#endif
    while (!completed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        pollCount++;

        if (shouldCancel.load()) break;

        // Check WebSocket for progress messages
#ifdef WINDOWS
        if (wsSock != INVALID_SOCKET) {
            wsPollProgress(wsSock, wsProgressVal, wsProgressMax, wsCurrentNode, wsFrameBuffer);
        }

        // Track when node changes to measure elapsed time per node
        if (wsCurrentNode != prevNode) {
            prevNode = wsCurrentNode;
            nodeStartCount = pollCount;
        }
#endif

        // Check history every 4th iteration (~1 second)
        if (pollCount % 4 == 0) {
            std::string historyResp = httpGet("/history/" + promptId);
            if (!historyResp.empty()) {
                try {
                    auto historyJson = nlohmann::json::parse(historyResp);
                    if (historyJson.contains(promptId)) {
                        auto& promptHistory = historyJson[promptId];

                        // Detect ComfyUI execution failure (OOM, exception, etc.)
                        // When a node raises an exception, ComfyUI marks status="error"
                        // and outputs is empty. Without this check the loop runs forever.
                        bool execFailed = false;
                        if (promptHistory.contains("status")) {
                            auto& st = promptHistory["status"];
                            if (st.contains("status_str") &&
                                st["status_str"].get<std::string>() == "error") {
                                execFailed = true;
                            }
                        }
                        if (execFailed) {
                            propagateFailed = true;
                            break;
                        }

                        if (promptHistory.contains("outputs") && !promptHistory["outputs"].empty()) {
                            completed = true;

                            {
                                std::lock_guard<std::mutex> lock(statusMutex);
                                statusText = "Downloading propagation masks...";
                                progressValue = 80.0f;
                            }

                            parsePropagationOutput(promptHistory);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[SAMSeg] Poll error: " << e.what() << std::endl;
                }
            }
        }

        if (!completed) {
            std::lock_guard<std::mutex> lock(statusMutex);
#ifdef WINDOWS
            int nodeSecs = (pollCount - nodeStartCount) / 4;

            // Parse ComfyUI log file for tqdm/print progress (~every 1s)
            if (pollCount % 4 == 0) {
                parseComfyUILogProgress(comfyLogPath, logProgress, logBaseOffset);
            }

            // Early abort: detection found zero objects
            if (logProgress.detectionFailed) {
                std::cerr << "[SAMSeg] ABORTING: zero detections" << std::endl;
                statusText = "No objects detected: check prompt text";
                progressValue = 0.0f;
                { std::lock_guard<std::mutex> lk(resultMutex); currentResult.width = 0; currentResult.height = 0; }
                break;
            }

            // Early abort: ran out of VRAM during propagation
            if (logProgress.oomFailed) {
                std::cerr << "[SAMSeg] ABORTING: out of VRAM after "
                          << logProgress.oomFramesDone << "/" << logProgress.oomFramesTotal
                          << " frames" << std::endl;
                statusText = "Out of VRAM after " + std::to_string(logProgress.oomFramesDone)
                           + "/" + std::to_string(logProgress.oomFramesTotal)
                           + " frames — try fewer objects or shorter clip";
                progressValue = 0.0f;
                break;
            }

            // Progress bar allocation:
            //   0-10%  = loading video frames (nodes 1, 3)
            //  10-15%  = loading SAM3 model (node 2)
            //  15-70%  = propagating masks (node 4)
            //  70-90%  = streaming to disk (node 5)
            //  90-100% = downloading/post-processing

            // Asymptotic time-based fallback: rises toward ~90% of the phase range
            // over ~120 seconds, so the bar always moves even without frame counts
            float timeFill = 1.0f - std::exp(-(float)nodeSecs / 60.0f);

            if (wsCurrentNode == "1" || wsCurrentNode == "3") {
                // Use specific loading tag, or fall back to generic last tqdm
                int loadCur = logProgress.loadingTotal > 0 ? logProgress.loadingCurrent : logProgress.lastTqdmCurrent;
                int loadTot = logProgress.loadingTotal > 0 ? logProgress.loadingTotal : logProgress.lastTqdmTotal;
                if (loadTot > 0) {
                    float realProgress = (float)loadCur / (float)loadTot;
                    progressValue = realProgress * 10.0f;
                    statusText = "Loading video frames... " + std::to_string(loadCur) +
                                 "/" + std::to_string(loadTot) + " frames";
                } else {
                    progressValue = timeFill * 9.0f;
                    statusText = "Loading video frames... (" + std::to_string(nodeSecs) + "s)";
                }
            } else if (wsCurrentNode == "2") {
                progressValue = 10.0f + timeFill * 4.0f;
                statusText = "Loading SAM3 model... (" + std::to_string(nodeSecs) + "s)";
            } else if (wsCurrentNode == "4") {
                if (wsProgressMax > 0) {
                    float realProgress = (float)wsProgressVal / (float)wsProgressMax;
                    progressValue = 15.0f + realProgress * 55.0f;
                    statusText = "Propagating masks... " + std::to_string(wsProgressVal) +
                                 "/" + std::to_string(wsProgressMax) + " frames";
                } else if (logProgress.propagateTotal > 0) {
                    float realProgress = (float)logProgress.propagateCurrent / (float)logProgress.propagateTotal;
                    progressValue = 15.0f + realProgress * 55.0f;
                    statusText = "Propagating masks... " + std::to_string(logProgress.propagateCurrent) +
                                 "/" + std::to_string(logProgress.propagateTotal) + " frames";
                } else {
                    progressValue = 15.0f + timeFill * 50.0f;
                    statusText = "Detecting objects... (" + std::to_string(nodeSecs) + "s)";
                }
            } else if (wsCurrentNode == "5") {
                if (wsProgressMax > 0) {
                    float realProgress = (float)wsProgressVal / (float)wsProgressMax;
                    progressValue = 70.0f + realProgress * 20.0f;
                    statusText = "Streaming to disk... " + std::to_string(wsProgressVal) +
                                 "/" + std::to_string(wsProgressMax) + " frames";
                } else if (logProgress.processedTotal > 0) {
                    float realProgress = (float)logProgress.processedCurrent / (float)logProgress.processedTotal;
                    progressValue = 70.0f + realProgress * 20.0f;
                    statusText = "Streaming to disk... " + std::to_string(logProgress.processedCurrent) +
                                 "/" + std::to_string(logProgress.processedTotal) + " frames";
                } else {
                    progressValue = 70.0f + timeFill * 18.0f;
                    statusText = "Streaming to disk... (" + std::to_string(nodeSecs) + "s)";
                }
            } else {
                // Unknown/intermediate node (e.g. MaskToImage, SaveImage between phases)
                // Keep the progress bar where it was — just update the status text
                statusText = "Processing... (" + std::to_string(nodeSecs) + "s)";
            }
#else
            statusText = "Propagating masks...";
            progressValue = 0.0f;
#endif
        }
    }

    // Close WebSocket
#ifdef WINDOWS
    wsClose(wsSock);
#endif

    if (shouldCancel.load()) {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Cancelled";
        progressValue = 0.0f;
        processing.store(false);
        return;
    }

#ifdef WINDOWS
    // Final log parse — the in-loop parse may have been skipped on the
    // iteration where history completed, so check one more time.
    parseComfyUILogProgress(comfyLogPath, logProgress, logBaseOffset);
    std::cerr << "[SAMSeg] Post-loop: detectionCount=" << logProgress.detectionCount
              << " detectionFailed=" << logProgress.detectionFailed
              << " logBaseOffset=" << logBaseOffset << std::endl;

    // Abort if detection failed
    if (logProgress.detectionFailed) {
        std::cerr << "[SAMSeg] Post-loop ABORTING: zero detections, sending /interrupt" << std::endl;
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = "No objects detected: check prompt text";
            progressValue = 0.0f;
        }
        { std::lock_guard<std::mutex> lk(resultMutex); currentResult.width = 0; currentResult.height = 0; }
        nlohmann::json body = nlohmann::json::object();
        httpPost("/interrupt", body.dump());
        processing.store(false);
        return;
    }

    // Abort if ComfyUI reported an execution error (OOM or other exception)
    if (propagateFailed || logProgress.oomFailed) {
        std::string errMsg;
        if (logProgress.oomFailed) {
            std::cerr << "[SAMSeg] Post-loop ABORTING: OOM after "
                      << logProgress.oomFramesDone << "/" << logProgress.oomFramesTotal << " frames" << std::endl;
            errMsg = "Out of VRAM after " + std::to_string(logProgress.oomFramesDone)
                   + "/" + std::to_string(logProgress.oomFramesTotal)
                   + " frames — try fewer objects or shorter clip";
        } else {
            std::cerr << "[SAMSeg] Post-loop ABORTING: ComfyUI execution error" << std::endl;
            errMsg = "Propagation failed — check ComfyUI log";
        }
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            statusText = errMsg;
            progressValue = 0.0f;
        }
        processing.store(false);
        return;
    }
#endif

    // Parse the first frame's mask into currentResult for the preview UI
    if (numPropagationMasks > 0) {
        int maskW = 0, maskH = 0;
        std::vector<uint8_t> firstMask = loadPropagationMask(0, &maskW, &maskH);
        std::cerr << "[SAMSeg] First mask: " << firstMask.size() << " bytes, "
                  << maskW << "x" << maskH << " (input frame: "
                  << inputImageWidth << "x" << inputImageHeight << ")" << std::endl;

        if (!firstMask.empty() && maskW > 0 && maskH > 0) {
            // Use actual mask dimensions — they may differ from input frame
            if (maskW != inputImageWidth || maskH != inputImageHeight) {
                std::cerr << "[SAMSeg] WARNING: mask size differs from input frame, using mask dimensions" << std::endl;
                inputImageWidth = maskW;
                inputImageHeight = maskH;
                if ((int)inputImageData.size() != maskW * maskH * 4) {
                    inputImageData.assign(maskW * maskH * 4, 0);
                    for (int i = 0; i < maskW * maskH; i++) {
                        inputImageData[i * 4 + 3] = 255;
                    }
                }
            }

            // Reload inputImageData from VHS-decoded orig if available
            // (pixel-perfect match with vis frame, avoids temporal mismatch)
            // No flipVertically — data stays upper-left to match mask orientation.
            // The display code uses draw_box_letterbox_seg which handles GL convention.
            if (!firstOrigPath.empty() && fs::exists(firstOrigPath)) {
                int ow, oh;
                auto origPixels = ImageLoader::loadImageRGBA(firstOrigPath, &ow, &oh);
                if (!origPixels.empty()) {
                    inputImageWidth = ow;
                    inputImageHeight = oh;
                    inputImageData = std::move(origPixels);
                    std::cerr << "[SAMSeg] Reloaded inputImageData from VHS orig: "
                              << ow << "x" << oh << std::endl;
                }
            }

            // Instance separation via vis frame color demixing
            splitMasksByVisualization(firstMask, maskW, maskH);

            // Fallback: if vis demixing didn't produce multiple masks, use combined
            size_t numInstances;
            { std::lock_guard<std::mutex> lk(resultMutex); numInstances = currentResult.masks.size(); }
            if (numInstances <= 1) {
                std::lock_guard<std::mutex> lock(resultMutex);
                currentResult.masks.clear();
                SegmentationMask mask;
                mask.id = 0;
                mask.label = "propagation";
                mask.confidence = 1.0f;
                mask.selected = true;
                mask.mask = std::move(firstMask);

                int w = maskW, h = maskH;
                float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        if (mask.mask[y * w + x] > 128) {
                            float nx = (float)x / w;
                            float ny = (float)y / h;
                            minX = std::min(minX, nx);
                            minY = std::min(minY, ny);
                            maxX = std::max(maxX, nx);
                            maxY = std::max(maxY, ny);
                        }
                    }
                }
                mask.bbox[0] = minX; mask.bbox[1] = minY;
                mask.bbox[2] = maxX; mask.bbox[3] = maxY;

                currentResult.masks.push_back(std::move(mask));
                currentResult.width = w;
                currentResult.height = h;
            }
        } else {
            std::cerr << "[SAMSeg] WARNING: first propagation mask is empty or zero-sized" << std::endl;
        }
    }

    // Generate outline visualization and masked output for preview
    std::cerr << "[SAMSeg] Generating outlines..." << std::endl;
    generateOutlineColors();
    renderOutlines();
    std::cerr << "[SAMSeg] Composing masked output..." << std::endl;
    composeMaskedOutputInternal();
    std::cerr << "[SAMSeg] Outline data: " << outlinePixelData.size() << " bytes, "
              << outlinePixelWidth << "x" << outlinePixelHeight << std::endl;
    newResultsReady.store(true);

    {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusText = "Ready (" + std::to_string(numPropagationMasks) + " frames tracked)";
        progressValue = 100.0f;
    }

    processing.store(false);
}

// ============================================================================
// Instance Separation via Visualization Colors
// ============================================================================

void SAMSegmentation::splitMasksByVisualization(const std::vector<uint8_t>& combinedMask, int maskW, int maskH) {
    if (firstVisPath.empty() || !fs::exists(firstVisPath)) {
        std::cerr << "[SAMSeg] No vis frame available for instance separation" << std::endl;
        return;
    }

    // Load visualization image (RGBA, upper-left origin)
    int visW, visH;
    auto visData = ImageLoader::loadImageRGBA(firstVisPath, &visW, &visH);
    if (visData.empty()) {
        std::cerr << "[SAMSeg] Failed to load vis image" << std::endl;
        return;
    }
    const uint8_t* visPixels = visData.data();

    if (visW != maskW || visH != maskH) {
        std::cerr << "[SAMSeg] Vis size " << visW << "x" << visH
                  << " != mask size " << maskW << "x" << maskH << std::endl;
        return;
    }

    // Load original first frame to recover overlay color
    // SAM3 vis blends with alpha~0.5: vis = (1-a)*original + a*color
    // So: overlay color = (vis - (1-a)*original) / a
    // Prefer VHS-decoded orig (pixel-perfect match with vis) over FFmpeg extract
    // (which may decode a different frame due to GOP position differences)
    std::string origPath;
    if (!firstOrigPath.empty() && fs::exists(firstOrigPath)) {
        origPath = firstOrigPath;
        std::cerr << "[SAMSeg] Using VHS-decoded orig frame: " << origPath << std::endl;
    } else {
        origPath = mainprogram->temppath + "/sam_temp/sam_first_frame.png";
        std::cerr << "[SAMSeg] Falling back to FFmpeg-extracted orig frame" << std::endl;
    }
    int origW, origH;
    auto origData = ImageLoader::loadImageRGBA(origPath, &origW, &origH);
    if (origData.empty()) {
        std::cerr << "[SAMSeg] Failed to load original frame for instance separation" << std::endl;
        return;
    }
    const uint8_t* origPixels = origData.data();

    if (origW != maskW || origH != maskH) {
        std::cerr << "[SAMSeg] Original size " << origW << "x" << origH
                  << " != mask size " << maskW << "x" << maskH << std::endl;
        return;
    }

    // SAM3 8-color palette for vis overlays
    static const int PALETTE[][3] = {
        {  0, 128, 255},  // Blue
        {255,  77,  77},  // Red
        { 77, 255,  77},  // Green
        {255, 255,   0},  // Yellow
        {255,   0, 255},  // Magenta
        {  0, 255, 255},  // Cyan
        {255, 128,   0},  // Orange
        {128,   0, 255},  // Purple
    };
    static const int NUM_PALETTE = 8;

    std::map<int, int> paletteCounts;
    std::vector<int> pixelPalette(maskW * maskH, -1);
    int totalMasked = 0;

    // Auto-calibrate: for UNmasked pixels, vis == original (no overlay),
    // so any systematic diff reveals the color offset between FFmpeg and VHS extraction.
    double sumDr = 0, sumDg = 0, sumDb = 0;
    int calibCount = 0;
    for (int i = 0; i < maskW * maskH; i++) {
        if (combinedMask[i] > 128) continue;
        int pi = i * 4;
        sumDr += (double)visPixels[pi + 0] - (double)origPixels[pi + 0];
        sumDg += (double)visPixels[pi + 1] - (double)origPixels[pi + 1];
        sumDb += (double)visPixels[pi + 2] - (double)origPixels[pi + 2];
        calibCount++;
    }
    float offsetR = 0, offsetG = 0, offsetB = 0;
    if (calibCount > 100) {
        offsetR = (float)(sumDr / calibCount);
        offsetG = (float)(sumDg / calibCount);
        offsetB = (float)(sumDb / calibCount);
        std::cerr << "[SAMSeg] Color calibration from " << calibCount << " unmasked pixels: "
                  << "offset=(" << offsetR << ", " << offsetG << ", " << offsetB << ")" << std::endl;
    }

    // Classify ALL masked pixels by vis-comparison on chroma plane.
    // vis = 0.5*orig + 0.5*palette.  Match actual vis to expected vis for each
    // palette color using chroma-plane distance (luminance-invariant).
    static const float SQRT3_2 = 0.866025f;
    for (int i = 0; i < maskW * maskH; i++) {
        if (combinedMask[i] <= 128) continue;
        totalMasked++;

        int pi = i * 4;
        float vr = visPixels[pi + 0], vg = visPixels[pi + 1], vb = visPixels[pi + 2];
        float or_ = origPixels[pi + 0] + offsetR;
        float og = origPixels[pi + 1] + offsetG;
        float ob = origPixels[pi + 2] + offsetB;

        int bestPal = 0;
        float bestDist = 1e30f;
        for (int p = 0; p < NUM_PALETTE; p++) {
            float er = 0.5f * or_ + 0.5f * PALETTE[p][0];
            float eg = 0.5f * og + 0.5f * PALETTE[p][1];
            float eb = 0.5f * ob + 0.5f * PALETTE[p][2];
            float dr = vr - er;
            float dg = vg - eg;
            float db = vb - eb;
            float cx = dr - 0.5f * dg - 0.5f * db;
            float cy = SQRT3_2 * (dg - db);
            float dist = cx * cx + cy * cy;
            if (dist < bestDist) { bestDist = dist; bestPal = p; }
        }

        pixelPalette[i] = bestPal;
        paletteCounts[bestPal]++;
    }

    if (totalMasked == 0) {
        std::cerr << "[SAMSeg] No masked pixels for instance separation" << std::endl;
        return;
    }

    // Find which palette colors are actually used (> 1% of masked pixels)
    int minPixels = std::max(100, totalMasked / 100);
    std::vector<int> usedColors;
    for (auto& [pal, count] : paletteCounts) {
        if (count >= minPixels) {
            usedColors.push_back(pal);
        }
    }

    std::cerr << "[SAMSeg] Instance separation: " << totalMasked << " masked pixels, "
              << paletteCounts.size() << " palette colors matched, "
              << usedColors.size() << " significant instances" << std::endl;

    if (usedColors.size() <= 1) {
        std::cerr << "[SAMSeg] Only 1 instance detected, keeping combined mask" << std::endl;
        return;
    }

    // Build per-instance masks
    std::lock_guard<std::mutex> lock(resultMutex);
    currentResult.masks.clear();
    instancePaletteColors = usedColors;  // persist instance-to-palette mapping for export

    std::map<int, int> palToInstance;
    for (int si = 0; si < (int)usedColors.size(); si++) {
        palToInstance[usedColors[si]] = si;

        SegmentationMask mask;
        mask.id = si;
        mask.label = "instance_" + std::to_string(si);
        mask.confidence = 1.0f;
        mask.selected = true;
        mask.mask.resize(maskW * maskH, 0);
        mask.bbox[0] = 1.0f; mask.bbox[1] = 1.0f;
        mask.bbox[2] = 0.0f; mask.bbox[3] = 0.0f;
        currentResult.masks.push_back(std::move(mask));
    }

    // Assign every masked pixel to its instance
    for (int i = 0; i < maskW * maskH; i++) {
        if (combinedMask[i] <= 128) continue;

        int pal = pixelPalette[i];
        int instIdx;
        auto it = palToInstance.find(pal);
        if (it != palToInstance.end()) {
            instIdx = it->second;
        } else {
            // Minor palette color — assign to nearest significant palette
            int bestIdx = 0;
            int bestDist = INT_MAX;
            for (int si = 0; si < (int)usedColors.size(); si++) {
                int dr = PALETTE[pal][0] - PALETTE[usedColors[si]][0];
                int dg = PALETTE[pal][1] - PALETTE[usedColors[si]][1];
                int db = PALETTE[pal][2] - PALETTE[usedColors[si]][2];
                int dist = dr * dr + dg * dg + db * db;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = si;
                }
            }
            instIdx = bestIdx;
        }

        currentResult.masks[instIdx].mask[i] = 255;
        int x = i % maskW;
        int y = i / maskW;
        float nx = (float)x / maskW;
        float ny = (float)y / maskH;
        auto& bbox = currentResult.masks[instIdx].bbox;
        if (nx < bbox[0]) bbox[0] = nx;
        if (ny < bbox[1]) bbox[1] = ny;
        if (nx > bbox[2]) bbox[2] = nx;
        if (ny > bbox[3]) bbox[3] = ny;
    }

    for (int si = 0; si < (int)usedColors.size(); si++) {
        int pal = usedColors[si];
        auto& m = currentResult.masks[si];
        int pixCount = 0;
        for (int i = 0; i < maskW * maskH; i++) if (m.mask[i]) pixCount++;
        std::cerr << "[SAMSeg] Instance " << si << ": " << pixCount << " pixels, palette=("
                  << PALETTE[pal][0] << "," << PALETTE[pal][1] << "," << PALETTE[pal][2]
                  << "), bbox=(" << m.bbox[0] << "," << m.bbox[1] << ")-("
                  << m.bbox[2] << "," << m.bbox[3] << ")" << std::endl;
    }

    currentResult.width = maskW;
    currentResult.height = maskH;

    std::cerr << "[SAMSeg] Split into " << currentResult.masks.size() << " instance masks" << std::endl;

    // origData and visData freed automatically
}

std::vector<int> SAMSegmentation::getSelectedPaletteColors() const {
    std::lock_guard<std::mutex> lock(resultMutex);
    std::vector<int> selected;
    for (const auto& mask : currentResult.masks) {
        if (mask.selected && mask.id >= 0 && mask.id < (int)instancePaletteColors.size()) {
            selected.push_back(instancePaletteColors[mask.id]);
        }
    }
    return selected;
}

// ============================================================================
// Propagation Workflow
// ============================================================================

nlohmann::json SAMSegmentation::preparePropagationWorkflow(const std::string& videoName,
                                                            const std::string& prompt) {
    std::string workflowPath = mainprogram->programData + "/EWOCvj2/comfyui/workflows/sam/propagation.json";

    nlohmann::json workflow;

    if (fs::exists(workflowPath)) {
        try {
            std::ifstream f(workflowPath);
            workflow = nlohmann::json::parse(f);
        } catch (const std::exception& e) {
            std::cerr << "[SAMSeg] Failed to load propagation workflow: " << e.what() << std::endl;
        }
    }

    if (workflow.empty()) {
        // Built-in fallback
        workflow = {
            {"1", {
                {"class_type", "VHS_LoadVideoPath"},
                {"inputs", {
                    {"video", "INPUT_VIDEO"},
                    {"force_rate", 0},
                    {"custom_width", 0},
                    {"custom_height", 0},
                    {"frame_load_cap", 0},
                    {"skip_first_frames", 0},
                    {"select_every_nth", 1}
                }}
            }},
            {"2", {
                {"class_type", "LoadSAM3Model"},
                {"inputs", {
                    {"model_path", "models/sam3/sam3.pt"}
                }}
            }},
            {"3", {
                {"class_type", "SAM3VideoSegmentation"},
                {"inputs", {
                    {"video_frames", nlohmann::json::array({"1", 0})},
                    {"prompt_mode", "text"},
                    {"text_prompt", "TEXT_PROMPT"},
                    {"frame_idx", 0},
                    {"score_threshold", scoreThreshold}
                }}
            }},
            {"4", {
                {"class_type", "SAM3Propagate"},
                {"inputs", {
                    {"sam3_model", nlohmann::json::array({"2", 0})},
                    {"video_state", nlohmann::json::array({"3", 0})},
                    {"start_frame", 0},
                    {"end_frame", -1},
                    {"direction", "both"}
                }}
            }},
            {"5", {
                {"class_type", "SAM3VideoOutput"},
                {"inputs", {
                    {"masks", nlohmann::json::array({"4", 0})},
                    {"video_state", nlohmann::json::array({"3", 0})},
                    {"scores", nlohmann::json::array({"4", 1})},
                    {"obj_id", -1},
                    {"plot_all_masks", true}
                }}
            }},
            {"6", {
                {"class_type", "MaskToImage"},
                {"inputs", {
                    {"mask", nlohmann::json::array({"5", 0})}
                }}
            }},
            {"7", {
                {"class_type", "SaveImage"},
                {"inputs", {
                    {"images", nlohmann::json::array({"6", 0})},
                    {"filename_prefix", "sam_propagation_masks"}
                }}
            }},
            {"8", {
                {"class_type", "SaveImage"},
                {"inputs", {
                    {"images", nlohmann::json::array({"5", 2})},
                    {"filename_prefix", "sam_propagation_vis"}
                }}
            }},
            {"9", {
                {"class_type", "SaveImage"},
                {"inputs", {
                    {"images", nlohmann::json::array({"5", 1})},
                    {"filename_prefix", "sam_propagation_orig"}
                }}
            }}
        };
    }

    // Substitute placeholders
    for (auto& [key, node] : workflow.items()) {
        if (node.contains("inputs")) {
            auto& inputs = node["inputs"];
            for (auto& [inputKey, inputVal] : inputs.items()) {
                if (inputVal.is_string()) {
                    std::string val = inputVal.get<std::string>();
                    if (val == "INPUT_VIDEO") {
                        inputVal = videoName;
                    } else if (val == "TEXT_PROMPT") {
                        inputVal = prompt;
                    }
                }
            }
            // Override score_threshold with user setting
            if (inputs.contains("score_threshold")) {
                inputs["score_threshold"] = scoreThreshold;
            }
            // Prefix filename_prefix with sam3/<videoname>/ subdirectory
            if (!sam3OutputSubdir.empty() && inputs.contains("filename_prefix")) {
                std::string prefix = inputs["filename_prefix"].get<std::string>();
                inputs["filename_prefix"] = sam3OutputSubdir + "/" + prefix;
            }
        }
    }

    return workflow;
}

bool SAMSegmentation::parsePropagationOutput(const nlohmann::json& historyData) {
    try {
        if (!historyData.contains("outputs")) return false;

        auto& outputs = historyData["outputs"];

        // Create temp directory for mask storage
        propagationMaskDir = mainprogram->temppath + "/sam_propagation_masks";
        fs::create_directories(propagationMaskDir);

        // Clear any previous masks
        for (const auto& entry : fs::directory_iterator(propagationMaskDir)) {
            fs::remove(entry.path());
        }
        numPropagationMasks = 0;

        // Collect mask images (from node 7: sam_propagation_masks prefix),
        // visualization images (from node 8: sam_propagation_vis prefix),
        // and original frames (from node 9: sam_propagation_orig prefix)
        std::vector<std::pair<std::string, std::string>> maskFiles;  // filename, subfolder
        std::vector<std::pair<std::string, std::string>> visFiles;   // filename, subfolder
        std::vector<std::pair<std::string, std::string>> origFiles;  // filename, subfolder

        for (auto& [nodeId, nodeOutput] : outputs.items()) {
            if (!nodeOutput.contains("images")) continue;

            for (auto& imageInfo : nodeOutput["images"]) {
                std::string filename = imageInfo.value("filename", "");
                std::string subfolder = imageInfo.value("subfolder", "");
                if (filename.empty()) continue;

                if (filename.find("sam_propagation_masks") != std::string::npos) {
                    maskFiles.push_back({filename, subfolder});
                } else if (filename.find("sam_propagation_vis") != std::string::npos) {
                    visFiles.push_back({filename, subfolder});
                } else if (filename.find("sam_propagation_orig") != std::string::npos) {
                    origFiles.push_back({filename, subfolder});
                }
            }
        }

        // Sort mask files by name (they're numbered sequentially)
        std::sort(maskFiles.begin(), maskFiles.end());

        // Prepare vis directory
        propagationVisDir = mainprogram->temppath + "/sam_propagation_vis";
        fs::create_directories(propagationVisDir);
        for (const auto& entry : fs::directory_iterator(propagationVisDir)) {
            fs::remove(entry.path());
        }

        // Prepare orig directory
        propagationOrigDir = mainprogram->temppath + "/sam_propagation_orig";
        fs::create_directories(propagationOrigDir);
        for (const auto& entry : fs::directory_iterator(propagationOrigDir)) {
            fs::remove(entry.path());
        }

        // Build unified download task list (masks + vis + orig)
        struct DownloadTask {
            std::string url;
            std::string localPath;
            int type;  // 0=mask, 1=vis, 2=orig
        };
        std::vector<DownloadTask> downloadTasks;
        downloadTasks.reserve(maskFiles.size() + visFiles.size() + origFiles.size());

        for (int i = 0; i < (int)maskFiles.size(); i++) {
            const auto& [filename, subfolder] = maskFiles[i];
            std::string url = "/view?filename=" + filename;
            if (!subfolder.empty()) url += "&subfolder=" + subfolder;
            char outName[256];
            snprintf(outName, sizeof(outName), "mask_%05d.png", i);
            downloadTasks.push_back({url, propagationMaskDir + "/" + outName, 0});
        }

        if (!visFiles.empty()) {
            std::sort(visFiles.begin(), visFiles.end());
            for (int i = 0; i < (int)visFiles.size(); i++) {
                const auto& [filename, subfolder] = visFiles[i];
                std::string url = "/view?filename=" + filename;
                if (!subfolder.empty()) url += "&subfolder=" + subfolder;
                char outName[256];
                snprintf(outName, sizeof(outName), "vis_%05d.png", i);
                downloadTasks.push_back({url, propagationVisDir + "/" + outName, 1});
            }
        }

        // Download ALL original frames (VHS-decoded, for accurate vis-comparison
        // in both instance separation and per-frame export filtering)
        firstOrigPath = "";
        if (!origFiles.empty()) {
            std::sort(origFiles.begin(), origFiles.end());
            for (int i = 0; i < (int)origFiles.size(); i++) {
                const auto& [filename, subfolder] = origFiles[i];
                std::string url = "/view?filename=" + filename;
                if (!subfolder.empty()) url += "&subfolder=" + subfolder;
                char outName[256];
                snprintf(outName, sizeof(outName), "orig_%05d.png", i);
                downloadTasks.push_back({url, propagationOrigDir + "/" + outName, 2});
            }
            firstOrigPath = propagationOrigDir + "/orig_00000.png";
        }

        // Parallel download with thread pool
        int numThreads = std::min((int)std::thread::hardware_concurrency(), 8);
        if (numThreads < 1) numThreads = 1;

        std::atomic<int> taskIndex{0};
        std::atomic<int> maskSuccessCount{0};
        std::atomic<int> visSuccessCount{0};
        std::atomic<int> origSuccessCount{0};
        std::atomic<int> totalDone{0};
        int totalTasks = (int)downloadTasks.size();

        auto downloadWorker = [&]() {
            while (true) {
                int idx = taskIndex.fetch_add(1);
                if (idx >= totalTasks) break;

                const DownloadTask& task = downloadTasks[idx];
                std::string data = httpGet(task.url);

                if (!data.empty()) {
                    std::ofstream outFile(task.localPath, std::ios::binary);
                    outFile.write(data.data(), data.size());
                    outFile.close();

                    if (task.type == 0) maskSuccessCount.fetch_add(1);
                    else if (task.type == 1) visSuccessCount.fetch_add(1);
                    else origSuccessCount.fetch_add(1);
                }

                int done = totalDone.fetch_add(1) + 1;
                {
                    std::lock_guard<std::mutex> lock(statusMutex);
                    statusText = "Downloading frames... " + std::to_string(done) + "/" + std::to_string(totalTasks);
                    progressValue = 70.0f + (float)done / (float)totalTasks * 29.0f;
                }
            }
        };

        std::vector<std::thread> dlThreads;
        dlThreads.reserve(numThreads);
        for (int t = 0; t < numThreads; t++) {
            dlThreads.emplace_back(downloadWorker);
        }
        for (auto& t : dlThreads) {
            t.join();
        }

        numPropagationMasks = maskSuccessCount.load();
        numPropagationVis = 0;
        numPropagationOrig = 0;
        firstVisPath = "";

        if (visSuccessCount.load() > 0) {
            numPropagationVis = visSuccessCount.load();
            firstVisPath = propagationVisDir + "/vis_00000.png";
        }
        if (origSuccessCount.load() > 0) {
            numPropagationOrig = origSuccessCount.load();
        }

        std::cerr << "[SAMSeg] Downloaded " << numPropagationMasks << " masks, "
                  << numPropagationVis << " vis, "
                  << numPropagationOrig << " orig (" << numThreads << " threads)" << std::endl;

        return numPropagationMasks > 0;

    } catch (const std::exception& e) {
        std::cerr << "[SAMSeg] Failed to parse propagation output: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint8_t> SAMSegmentation::loadPropagationMask(int frameIndex, int* outWidth, int* outHeight) const {
    if (propagationMaskDir.empty() || frameIndex < 0 || frameIndex >= numPropagationMasks) {
        return {};
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "mask_%05d.png", frameIndex);
    std::string maskPath = propagationMaskDir + "/" + filename;

    if (!fs::exists(maskPath)) return {};

    int w, h;
    auto maskData = ImageLoader::loadImageGray(maskPath, &w, &h);
    if (!maskData.empty()) {
        if (outWidth) *outWidth = w;
        if (outHeight) *outHeight = h;
    }
    return maskData;
}

std::vector<uint8_t> SAMSegmentation::loadPropagationVis(int frameIndex, int* outWidth, int* outHeight) const {
    if (propagationVisDir.empty() || frameIndex < 0 || frameIndex >= numPropagationVis) {
        return {};
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "vis_%05d.png", frameIndex);
    std::string visPath = propagationVisDir + "/" + filename;

    if (!fs::exists(visPath)) return {};

    int w, h;
    auto visData = ImageLoader::loadImageRGBA(visPath, &w, &h);
    if (!visData.empty()) {
        if (outWidth) *outWidth = w;
        if (outHeight) *outHeight = h;
    }
    return visData;
}

std::vector<uint8_t> SAMSegmentation::loadPropagationOrig(int frameIndex, int* outWidth, int* outHeight) const {
    if (propagationOrigDir.empty() || frameIndex < 0 || frameIndex >= numPropagationOrig) {
        return {};
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "orig_%05d.png", frameIndex);
    std::string origPath = propagationOrigDir + "/" + filename;

    if (!fs::exists(origPath)) return {};

    int w, h;
    auto origData = ImageLoader::loadImageRGBA(origPath, &w, &h);
    if (!origData.empty()) {
        if (outWidth) *outWidth = w;
        if (outHeight) *outHeight = h;
    }
    return origData;
}

// ============================================================================
// Result Access
// ============================================================================

SegmentationResult SAMSegmentation::getResult() const {
    std::lock_guard<std::mutex> lock(resultMutex);
    return currentResult;
}

float SAMSegmentation::getProgress() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return progressValue;
}

std::string SAMSegmentation::getStatus() const {
    std::lock_guard<std::mutex> lock(statusMutex);
    return statusText;
}

std::string SAMSegmentation::testConnection() {
    // Try GET /system_stats to check if ComfyUI is reachable
    return httpGet("/system_stats");
}

// ============================================================================
// Mask Selection
// ============================================================================

void SAMSegmentation::toggleMaskSelection(int maskId) {
    std::lock_guard<std::mutex> lock(resultMutex);
    for (auto& mask : currentResult.masks) {
        if (mask.id == maskId) {
            mask.selected = !mask.selected;
            break;
        }
    }
}

int SAMSegmentation::getMaskAtPosition(float normalizedX, float normalizedY) {
    std::lock_guard<std::mutex> lock(resultMutex);

    if (currentResult.width <= 0 || currentResult.height <= 0) return -1;

    int pixelX = (int)(normalizedX * currentResult.width);
    int pixelY = (int)(normalizedY * currentResult.height);

    if (pixelX < 0 || pixelX >= currentResult.width ||
        pixelY < 0 || pixelY >= currentResult.height) {
        return -1;
    }

    // Check masks in reverse order (topmost first)
    for (int i = (int)currentResult.masks.size() - 1; i >= 0; i--) {
        auto& mask = currentResult.masks[i];
        if (mask.mask.empty()) continue;

        int idx = pixelY * currentResult.width + pixelX;
        if (idx >= 0 && idx < (int)mask.mask.size() && mask.mask[idx] > 128) {
            return mask.id;
        }
    }

    return -1;
}

void SAMSegmentation::setInverted(bool inv) {
    inverted = inv;
}

bool SAMSegmentation::isInverted() const {
    return inverted;
}

// ============================================================================
// Composition
// ============================================================================

void SAMSegmentation::composeMaskedOutput() {
    composeMaskedOutputInternal();
    newResultsReady.store(true);  // Signal main thread to re-upload textures
}

void SAMSegmentation::uploadResultTextures() {
    if (!newResultsReady.load()) return;
    newResultsReady.store(false);

    std::lock_guard<std::mutex> lock(resultMutex);

    // Upload outline pixel data to GL texture
    if (!outlinePixelData.empty() && outlinePixelWidth > 0 && outlinePixelHeight > 0) {
        if (currentResult.outlineTex == (GLuint)-1) {
            glGenTextures(1, &currentResult.outlineTex);
        }
        glBindTexture(GL_TEXTURE_2D, currentResult.outlineTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outlinePixelWidth, outlinePixelHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, outlinePixelData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Upload masked pixel data to GL texture
    if (!maskedPixelData.empty() && maskedPixelWidth > 0 && maskedPixelHeight > 0) {
        if (currentResult.maskedTex == (GLuint)-1) {
            glGenTextures(1, &currentResult.maskedTex);
        }
        glBindTexture(GL_TEXTURE_2D, currentResult.maskedTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, maskedPixelWidth, maskedPixelHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, maskedPixelData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void SAMSegmentation::composeMaskedOutputInternal() {
    std::lock_guard<std::mutex> lock(resultMutex);

    if (inputImageData.empty() || inputImageWidth <= 0 || inputImageHeight <= 0) return;

    int w = inputImageWidth;
    int h = inputImageHeight;

    // Create RGBA output with alpha based on selected masks
    std::vector<uint8_t> outputData(w * h * 4, 0);

    // Copy input image as base (y-flipped so row 0 = bottom, matching GL texture layout)
    std::vector<uint8_t> flippedData = inputImageData;
    flipVertically(flippedData, w, h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            int pixIdx = idx * 4;

            // Check if this pixel is in any selected mask
            bool inMask = false;
            for (const auto& mask : currentResult.masks) {
                if (!mask.selected) continue;
                if (mask.mask.empty()) continue;
                if (idx < (int)mask.mask.size() && mask.mask[idx] > 128) {
                    inMask = true;
                    break;
                }
            }

            if (inverted) inMask = !inMask;

            if (inMask) {
                // Copy RGB from input, full alpha
                outputData[pixIdx + 0] = flippedData[pixIdx + 0];
                outputData[pixIdx + 1] = flippedData[pixIdx + 1];
                outputData[pixIdx + 2] = flippedData[pixIdx + 2];
                outputData[pixIdx + 3] = 255;
            } else {
                // Transparent
                outputData[pixIdx + 0] = 0;
                outputData[pixIdx + 1] = 0;
                outputData[pixIdx + 2] = 0;
                outputData[pixIdx + 3] = 0;
            }
        }
    }

    maskedPixelData = std::move(outputData);
    maskedPixelWidth = w;
    maskedPixelHeight = h;

    currentResult.width = w;
    currentResult.height = h;
}

// ============================================================================
// Outline Rendering
// ============================================================================

void SAMSegmentation::generateOutlineColors() {
    std::lock_guard<std::mutex> lock(resultMutex);

    // SAM3 palette order — matches obj_id assignment
    float colors[][3] = {
        {0.0f, 0.5f, 1.0f},   // Blue     (SAM3 obj 0)
        {1.0f, 0.3f, 0.3f},   // Red      (SAM3 obj 1)
        {0.3f, 1.0f, 0.3f},   // Green    (SAM3 obj 2)
        {1.0f, 1.0f, 0.0f},   // Yellow   (SAM3 obj 3)
        {1.0f, 0.0f, 1.0f},   // Magenta  (SAM3 obj 4)
        {0.0f, 1.0f, 1.0f},   // Cyan     (SAM3 obj 5)
        {1.0f, 0.5f, 0.0f},   // Orange   (SAM3 obj 6)
        {0.5f, 0.0f, 1.0f},   // Purple   (SAM3 obj 7)
    };
    int numColors = 8;

    for (int i = 0; i < (int)currentResult.masks.size(); i++) {
        int ci = i % numColors;
        currentResult.masks[i].outlineColor[0] = colors[ci][0];
        currentResult.masks[i].outlineColor[1] = colors[ci][1];
        currentResult.masks[i].outlineColor[2] = colors[ci][2];
    }
}

void SAMSegmentation::renderOutlines() {
    std::lock_guard<std::mutex> lock(resultMutex);

    if (inputImageData.empty() || inputImageWidth <= 0 || inputImageHeight <= 0) return;

    int w = inputImageWidth;
    int h = inputImageHeight;

    // Copy input image as base (y-flipped so row 0 = bottom, matching GL texture layout)
    std::vector<uint8_t> outlineData = inputImageData;
    flipVertically(outlineData, w, h);

    // Draw outlines for each mask
    for (const auto& mask : currentResult.masks) {
        if (mask.mask.empty()) continue;

        uint8_t r = (uint8_t)(mask.outlineColor[0] * 255);
        uint8_t g = (uint8_t)(mask.outlineColor[1] * 255);
        uint8_t b = (uint8_t)(mask.outlineColor[2] * 255);

        // Find edge pixels (mask pixel adjacent to non-mask pixel)
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                int idx = y * w + x;
                if (idx >= (int)mask.mask.size()) continue;
                if (mask.mask[idx] <= 128) continue;

                // Check if this is an edge pixel
                bool isEdge = false;
                int neighbors[] = {-1, 0, 1, 0, 0, -1, 0, 1};
                for (int n = 0; n < 4; n++) {
                    int nx = x + neighbors[n * 2];
                    int ny = y + neighbors[n * 2 + 1];
                    int nidx = ny * w + nx;
                    if (nidx >= 0 && nidx < (int)mask.mask.size()) {
                        if (mask.mask[nidx] <= 128) {
                            isEdge = true;
                            break;
                        }
                    }
                }

                if (isEdge) {
                    // Draw outline pixel (2px thick)
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int px = x + dx;
                            int py = y + dy;
                            if (px >= 0 && px < w && py >= 0 && py < h) {
                                int pidx = (py * w + px) * 4;
                                // Blend: brighter for selected masks
                                float alpha = mask.selected ? 1.0f : 0.6f;
                                outlineData[pidx + 0] = (uint8_t)(r * alpha + outlineData[pidx + 0] * (1.0f - alpha));
                                outlineData[pidx + 1] = (uint8_t)(g * alpha + outlineData[pidx + 1] * (1.0f - alpha));
                                outlineData[pidx + 2] = (uint8_t)(b * alpha + outlineData[pidx + 2] * (1.0f - alpha));
                                outlineData[pidx + 3] = 255;
                            }
                        }
                    }
                }
            }
        }

        // If selected, also tint the interior slightly
        if (mask.selected) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int idx = y * w + x;
                    if (idx >= (int)mask.mask.size()) continue;
                    if (mask.mask[idx] <= 128) continue;

                    int pidx = idx * 4;
                    float tint = 0.15f;
                    outlineData[pidx + 0] = (uint8_t)(r * tint + outlineData[pidx + 0] * (1.0f - tint));
                    outlineData[pidx + 1] = (uint8_t)(g * tint + outlineData[pidx + 1] * (1.0f - tint));
                    outlineData[pidx + 2] = (uint8_t)(b * tint + outlineData[pidx + 2] * (1.0f - tint));
                }
            }
        }
    }

    outlinePixelData = std::move(outlineData);
    outlinePixelWidth = w;
    outlinePixelHeight = h;
}

// ============================================================================
// Instance Separation via Visualization Colors
// ============================================================================

// ============================================================================
// Export
// ============================================================================

bool SAMSegmentation::exportMaskedFrames(const std::string& videoPath, const std::string& outputDir,
                                          std::function<void(float)> progressCallback) {
    // Open video
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[SAMSeg] Failed to open video: " << videoPath << std::endl;
        return false;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }
    if (videoStreamIdx < 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }

    auto* codecPar = fmtCtx->streams[videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    // Get FPS
    AVRational frameRate = fmtCtx->streams[videoStreamIdx]->r_frame_rate;
    videoFps = (float)frameRate.num / (float)frameRate.den;
    videoWidth = codecCtx->width;
    videoHeight = codecCtx->height;

    // Setup scaler to RGBA
    SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                         codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbaFrame = av_frame_alloc();
    rgbaFrame->format = AV_PIX_FMT_RGBA;
    rgbaFrame->width = codecCtx->width;
    rgbaFrame->height = codecCtx->height;
    av_image_alloc(rgbaFrame->data, rgbaFrame->linesize,
                   rgbaFrame->width, rgbaFrame->height, AV_PIX_FMT_RGBA, 32);

    AVPacket* pkt = av_packet_alloc();
    fs::create_directories(outputDir);

    int frameCount = 0;
    int totalFrames = (int)(fmtCtx->duration / (double)AV_TIME_BASE * videoFps);
    if (totalFrames <= 0) totalFrames = 1;

    bool usePropagation = !propagationMaskDir.empty() && numPropagationMasks > 0;

    // Check if we need per-frame instance filtering (some instances deselected)
    std::vector<int> selectedPalettes = getSelectedPaletteColors();
    bool useInstanceFiltering = usePropagation && !instancePaletteColors.empty()
                                && (int)selectedPalettes.size() < (int)instancePaletteColors.size()
                                && numPropagationVis > 0;

    // Build lookup set for fast palette matching
    std::set<int> selectedPaletteSet(selectedPalettes.begin(), selectedPalettes.end());

    // SAM3 8-color palette for chroma-plane matching
    static const int PALETTE[][3] = {
        {  0, 128, 255}, {255,  77,  77}, { 77, 255,  77}, {255, 255,   0},
        {255,   0, 255}, {  0, 255, 255}, {255, 128,   0}, {128,   0, 255},
    };
    static const int NUM_PALETTE = 8;
    if (useInstanceFiltering) {
        std::cerr << "[SAMSeg] Export: per-frame instance filtering with VHS orig, "
                  << selectedPalettes.size() << "/" << instancePaletteColors.size()
                  << " instances selected" << std::endl;
    }

    // Build minor-palette remapping table
    std::vector<int> paletteRemap(NUM_PALETTE);
    std::set<int> significantSet(instancePaletteColors.begin(), instancePaletteColors.end());
    for (int p = 0; p < NUM_PALETTE; p++) {
        if (significantSet.count(p)) {
            paletteRemap[p] = p;
        } else {
            int bestPal = 0;
            int bestDist = INT_MAX;
            for (int sp : instancePaletteColors) {
                int dr = PALETTE[p][0] - PALETTE[sp][0];
                int dg = PALETTE[p][1] - PALETTE[sp][1];
                int db = PALETTE[p][2] - PALETTE[sp][2];
                int dist = dr * dr + dg * dg + db * db;
                if (dist < bestDist) { bestDist = dist; bestPal = sp; }
            }
            paletteRemap[p] = bestPal;
        }
    }

    // Build static combined mask (fallback when no propagation)
    std::vector<bool> combinedMask;
    if (!usePropagation) {
        std::lock_guard<std::mutex> lock(resultMutex);
        int maskSize = currentResult.width * currentResult.height;
        combinedMask.resize(maskSize, false);
        for (const auto& mask : currentResult.masks) {
            if (!mask.selected) continue;
            for (int i = 0; i < maskSize && i < (int)mask.mask.size(); i++) {
                if (mask.mask[i] > 128) combinedMask[i] = true;
            }
        }
    }

    // --- Parallel export pipeline ---
    struct ExportWorkItem {
        int frameIndex;
        int width, height;
        std::vector<uint8_t> rgbaPixels;  // packed W*H*4
    };

    int numWorkers = std::min((int)std::thread::hardware_concurrency(), 8);
    if (numWorkers < 1) numWorkers = 1;

    WorkQueue<ExportWorkItem> workQueue(numWorkers * 2);
    std::atomic<int> framesCompleted{0};

    // Capture read-only state for workers
    bool workerUsePropagation = usePropagation;
    bool workerUseInstanceFiltering = useInstanceFiltering;
    bool workerInverted = inverted;
    std::string workerMaskDir = propagationMaskDir;
    std::string workerVisDir = propagationVisDir;
    std::string workerOrigDir = propagationOrigDir;
    int workerNumMasks = numPropagationMasks;
    int workerNumVis = numPropagationVis;
    int workerNumOrig = numPropagationOrig;
    std::string workerOutputDir = outputDir;

    // Worker: load mask/vis, apply mask, flip, save PNG (all thread-safe via FFmpeg)
    auto exportWorker = [&]() {
        ExportWorkItem item;
        while (workQueue.pop(item)) {
            int w = item.width;
            int h = item.height;
            uint8_t* pixels = item.rgbaPixels.data();
            int stride = w * 4;

            // Load per-frame mask using FFmpeg (thread-safe)
            std::vector<uint8_t> frameMask;
            int mw = w, mh = h;  // mask dimensions (may differ from video)
            if (workerUsePropagation) {
                int maskFrame = std::min(item.frameIndex, workerNumMasks - 1);
                char maskName[512];
                snprintf(maskName, sizeof(maskName), "%s/mask_%05d.png",
                         workerMaskDir.c_str(), maskFrame);
                frameMask = ImageLoader::loadImageGray(maskName, &mw, &mh);
                if (item.frameIndex == 0 && (mw != w || mh != h)) {
                    std::cerr << "[SAMSeg] WARNING: mask " << mw << "x" << mh
                              << " != video " << w << "x" << h << " — using coordinate mapping" << std::endl;
                }
            }

            // Load VHS-decoded orig frame — use as output pixels AND for
            // vis-comparison classification. Both orig and vis come from VHS,
            // so they're temporally aligned (no mismatch at moving edges).
            std::vector<uint8_t> frameOrig;
            int origW = 0, origH = 0;
            if (workerUsePropagation && workerNumOrig > 0) {
                int origFrame = std::min(item.frameIndex, workerNumOrig - 1);
                char origName[512];
                snprintf(origName, sizeof(origName), "%s/orig_%05d.png",
                         workerOrigDir.c_str(), origFrame);
                frameOrig = ImageLoader::loadImageRGBA(origName, &origW, &origH);
                if (!frameOrig.empty() && origW == w && origH == h) {
                    memcpy(pixels, frameOrig.data(), w * h * 4);
                }
            }

            // Load vis frame for instance filtering
            std::vector<uint8_t> frameVis;
            int visW = 0, visH = 0;
            if (workerUseInstanceFiltering && !frameMask.empty()) {
                int visFrame = std::min(item.frameIndex, workerNumVis - 1);
                char visName[512];
                snprintf(visName, sizeof(visName), "%s/vis_%05d.png",
                         workerVisDir.c_str(), visFrame);
                frameVis = ImageLoader::loadImageRGBA(visName, &visW, &visH);
            }

            // Per-frame instance classification using vis + orig (or video pixels as fallback).
            // vis is at mask resolution (mw x mh); framePalette is at video resolution (w x h).
            // When VHS orig is unavailable, the video-decoded pixels buffer is used as the
            // "original" — the 50% blend formula still produces distinct chroma per palette entry.
            std::vector<int> framePalette;
            bool origAtMaskRes = !frameOrig.empty() && origW == mw && origH == mh;
            if (workerUseInstanceFiltering && !frameMask.empty()
                && !frameVis.empty() && visW == mw && visH == mh) {
                framePalette.assign(w * h, -1);

                // Auto-calibrate color offset between orig/video and vis for unmasked pixels.
                double calDr = 0, calDg = 0, calDb = 0;
                int calCount = 0;
                for (int ci = 0; ci < mw * mh; ci++) {
                    if (frameMask[ci] > 128) continue;
                    int vpi = ci * 4;
                    float or_, og, ob;
                    if (origAtMaskRes) {
                        or_ = frameOrig[vpi + 0];
                        og  = frameOrig[vpi + 1];
                        ob  = frameOrig[vpi + 2];
                    } else {
                        // Map mask coords to video coords and use video pixel as orig substitute
                        int cx = ci % mw, cy = ci / mw;
                        int vx = (mw == w) ? cx : cx * w / mw;
                        int vy = (mh == h) ? cy : cy * h / mh;
                        int opi = (vy * w + vx) * 4;
                        or_ = pixels[opi + 0];
                        og  = pixels[opi + 1];
                        ob  = pixels[opi + 2];
                    }
                    calDr += (double)frameVis[vpi + 0] - (double)or_;
                    calDg += (double)frameVis[vpi + 1] - (double)og;
                    calDb += (double)frameVis[vpi + 2] - (double)ob;
                    calCount++;
                }
                float offR = 0, offG = 0, offB = 0;
                if (calCount > 100) {
                    offR = (float)(calDr / calCount);
                    offG = (float)(calDg / calCount);
                    offB = (float)(calDb / calCount);
                }

                // Classify masked pixels by vis-comparison on chroma plane.
                // Outer loop at video resolution; lookups map to mask resolution.
                static const float SQRT3_2 = 0.866025f;
                for (int vy = 0; vy < h; vy++) {
                    for (int vx = 0; vx < w; vx++) {
                        int smy = (mh == h) ? vy : vy * mh / h;
                        int smx = (mw == w) ? vx : vx * mw / w;
                        int mi = smy * mw + smx;
                        if (mi >= (int)frameMask.size() || frameMask[mi] <= 128) continue;

                        int pi = mi * 4;
                        float vr = frameVis[pi + 0], vg = frameVis[pi + 1], vb = frameVis[pi + 2];
                        float or_, og, ob;
                        if (origAtMaskRes) {
                            or_ = frameOrig[pi + 0] + offR;
                            og  = frameOrig[pi + 1] + offG;
                            ob  = frameOrig[pi + 2] + offB;
                        } else {
                            int opi = (vy * w + vx) * 4;
                            or_ = pixels[opi + 0] + offR;
                            og  = pixels[opi + 1] + offG;
                            ob  = pixels[opi + 2] + offB;
                        }

                        int bestPal = 0;
                        float bestDist = 1e30f;
                        for (int p = 0; p < NUM_PALETTE; p++) {
                            float er = 0.5f * or_ + 0.5f * PALETTE[p][0];
                            float eg = 0.5f * og + 0.5f * PALETTE[p][1];
                            float eb = 0.5f * ob + 0.5f * PALETTE[p][2];
                            float dr = vr - er;
                            float dg = vg - eg;
                            float db = vb - eb;
                            float cx = dr - 0.5f * dg - 0.5f * db;
                            float cy = SQRT3_2 * (dg - db);
                            float dist = cx * cx + cy * cy;
                            if (dist < bestDist) { bestDist = dist; bestPal = p; }
                        }
                        framePalette[vy * w + vx] = paletteRemap[bestPal];
                    }
                }
            }

            // Apply mask to frame
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    bool inMask = false;

                    if (workerUsePropagation && !frameMask.empty()) {
                        int mx = (mw == w) ? x : x * mw / w;
                        int my = (mh == h) ? y : y * mh / h;
                        int maskIdx = my * mw + mx;
                        if (maskIdx >= 0 && maskIdx < (int)frameMask.size()) {
                            inMask = frameMask[maskIdx] > 128;
                        }

                        // Instance filtering via palette classification
                        if (inMask && !framePalette.empty()) {
                            int palIdx = y * w + x;
                            if (palIdx >= 0 && palIdx < (int)framePalette.size()) {
                                int pal = framePalette[palIdx];
                                if (pal < 0 || selectedPaletteSet.find(pal) == selectedPaletteSet.end()) {
                                    inMask = false;
                                }
                            }
                        }
                    } else {
                        int maskIdx = y * w + x;
                        if (maskIdx < (int)combinedMask.size()) {
                            inMask = combinedMask[maskIdx];
                        }
                    }

                    if (workerInverted) inMask = !inMask;

                    if (!inMask) {
                        int pixOff = y * stride + x * 4;
                        pixels[pixOff + 0] = 0;
                        pixels[pixOff + 1] = 0;
                        pixels[pixOff + 2] = 0;
                        pixels[pixOff + 3] = 0;
                    }
                }
            }

            // RGB edge padding: copy nearest opaque pixels' RGB into adjacent
            // transparent pixels (keeping alpha=0).  This prevents DXT5 block
            // compression from creating visible color artifacts at mask edges.
            // DXT5 operates on 4x4 blocks and compresses color + alpha channels
            // separately; without padding, boundary blocks interpolate content
            // colors into transparent areas, producing visible "buzz" lines.
            // Only alpha>0 pixels serve as sources, preventing cascading flood.
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int off = (y * w + x) * 4;
                    if (pixels[off + 3] != 0) continue;  // opaque, skip

                    int sumR = 0, sumG = 0, sumB = 0, count = 0;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = x + dx, ny = y + dy;
                            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                            int noff = (ny * w + nx) * 4;
                            if (pixels[noff + 3] > 0) {  // only opaque sources
                                sumR += pixels[noff + 0];
                                sumG += pixels[noff + 1];
                                sumB += pixels[noff + 2];
                                count++;
                            }
                        }
                    }
                    if (count > 0) {
                        pixels[off + 0] = (uint8_t)(sumR / count);
                        pixels[off + 1] = (uint8_t)(sumG / count);
                        pixels[off + 2] = (uint8_t)(sumB / count);
                        // alpha stays 0
                    }
                }
            }

            // Save PNG using FFmpeg (thread-safe)
            char filename[256];
            snprintf(filename, sizeof(filename), "frame_%05d.png", item.frameIndex);
            std::string outPath = workerOutputDir + "/" + filename;
            ImageLoader::saveImagePNG(outPath, pixels, w, h);

            int done = framesCompleted.fetch_add(1) + 1;
            if (progressCallback) {
                progressCallback((float)done / (float)totalFrames);
            }
        }
    };

    // Launch worker threads
    std::vector<std::thread> workers;
    workers.reserve(numWorkers);
    for (int t = 0; t < numWorkers; t++) {
        workers.emplace_back(exportWorker);
    }

    // Producer: decode video frames and push to queue
    frameCount = 0;
    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(codecCtx, pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }

        while (avcodec_receive_frame(codecCtx, frame) >= 0) {
            sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                      rgbaFrame->data, rgbaFrame->linesize);

            int w = codecCtx->width;
            int h = codecCtx->height;

            ExportWorkItem item;
            item.frameIndex = frameCount;
            item.width = w;
            item.height = h;
            item.rgbaPixels.resize(w * h * 4);

            // Copy from rgbaFrame (may have linesize padding) to packed buffer
            int srcStride = rgbaFrame->linesize[0];
            int dstStride = w * 4;
            for (int row = 0; row < h; row++) {
                memcpy(item.rgbaPixels.data() + row * dstStride,
                       rgbaFrame->data[0] + row * srcStride, dstStride);
            }

            workQueue.push(std::move(item));
            frameCount++;
        }

        av_packet_unref(pkt);
    }

    // Signal workers and wait
    workQueue.finish();
    for (auto& w : workers) {
        w.join();
    }

    int completedFrames = framesCompleted.load();

    // Cleanup
    av_packet_free(&pkt);
    av_freep(&rgbaFrame->data[0]);
    av_frame_free(&rgbaFrame);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    std::cerr << "[SAMSeg] Exported " << completedFrames << " masked frames to " << outputDir
              << " (" << numWorkers << " threads)" << std::endl;
    return completedFrames > 0;
}

// ============================================================================
// Video Frame Extraction
// ============================================================================

std::string SAMSegmentation::extractFirstFrame(const std::string& videoPath, const std::string& tempDir) {
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        return "";
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return "";
    }

    int videoStreamIdx = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }
    if (videoStreamIdx < 0) {
        avformat_close_input(&fmtCtx);
        return "";
    }

    auto* codecPar = fmtCtx->streams[videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    // Store video info
    AVRational frameRate = fmtCtx->streams[videoStreamIdx]->r_frame_rate;
    videoFps = (float)frameRate.num / (float)frameRate.den;
    videoWidth = codecCtx->width;
    videoHeight = codecCtx->height;

    SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                         codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbaFrame = av_frame_alloc();
    rgbaFrame->format = AV_PIX_FMT_RGBA;
    rgbaFrame->width = codecCtx->width;
    rgbaFrame->height = codecCtx->height;
    av_image_alloc(rgbaFrame->data, rgbaFrame->linesize,
                   rgbaFrame->width, rgbaFrame->height, AV_PIX_FMT_RGBA, 32);

    AVPacket* pkt = av_packet_alloc();
    std::string outputPath;

    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(codecCtx, pkt) >= 0) {
            if (avcodec_receive_frame(codecCtx, frame) >= 0) {
                sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                          rgbaFrame->data, rgbaFrame->linesize);

                // Save first frame as PNG
                outputPath = tempDir + "/sam_first_frame.png";
                ImageLoader::saveImagePNG(outputPath, rgbaFrame->data[0], codecCtx->width, codecCtx->height);

                av_packet_unref(pkt);
                break;
            }
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_freep(&rgbaFrame->data[0]);
    av_frame_free(&rgbaFrame);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    return outputPath;
}

// ============================================================================
// ComfyUI HTTP Communication
// ============================================================================

std::string SAMSegmentation::httpPost(const std::string& endpoint, const std::string& data) {
#ifdef WINDOWS
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"SAMSegmentation/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    std::wstring wHost(comfyHost.begin(), comfyHost.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                         static_cast<INTERNET_PORT>(comfyPort), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wEndpoint.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD timeout = 30000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    std::wstring headers = L"Content-Type: application/json";
    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1,
                                        (LPVOID)data.c_str(), static_cast<DWORD>(data.length()),
                                        static_cast<DWORD>(data.length()), 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (bResults) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize > 0) {
                std::vector<char> buffer(dwSize + 1, 0);
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer.data(), dwSize, &downloaded);
                result.append(buffer.data(), downloaded);
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
#else
    std::string result;
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    std::string url = "http://" + comfyHost + ":" + std::to_string(comfyPort) + endpoint;

    struct WriteCallbackData {
        std::string* output;
    };
    WriteCallbackData cbData{&result};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* d = static_cast<WriteCallbackData*>(userdata);
        d->output->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbData);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);

    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
#endif
}

std::string SAMSegmentation::httpGet(const std::string& endpoint) {
#ifdef WINDOWS
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"SAMSegmentation/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    std::wstring wHost(comfyHost.begin(), comfyHost.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                         static_cast<INTERNET_PORT>(comfyPort), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wEndpoint.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD timeout = 30000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (bResults) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize > 0) {
                std::vector<char> buffer(dwSize + 1, 0);
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer.data(), dwSize, &downloaded);
                result.append(buffer.data(), downloaded);
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
#else
    std::string result;
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    std::string url = "http://" + comfyHost + ":" + std::to_string(comfyPort) + endpoint;

    struct WriteCallbackData {
        std::string* output;
    };
    WriteCallbackData cbData{&result};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* d = static_cast<WriteCallbackData*>(userdata);
        d->output->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbData);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return result;
#endif
}

bool SAMSegmentation::uploadImage(const std::string& localPath, std::string& uploadedName) {
    // Read file data
    std::ifstream file(localPath, std::ios::binary);
    if (!file) return false;

    std::string fileData((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();

    std::string filename = fs::path(localPath).filename().string();

    // Build multipart form data
    std::string boundary = "----SAMSegBoundary" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());

    std::string body;
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"image\"; filename=\"" + filename + "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n\r\n";
    body += fileData;
    body += "\r\n--" + boundary + "--\r\n";

#ifdef WINDOWS
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"SAMSegmentation/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    std::wstring wHost(comfyHost.begin(), comfyHost.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                         static_cast<INTERNET_PORT>(comfyPort), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/upload/image",
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring contentType = L"Content-Type: multipart/form-data; boundary=" +
                                std::wstring(boundary.begin(), boundary.end());

    BOOL bResults = WinHttpSendRequest(hRequest, contentType.c_str(), -1,
                                        (LPVOID)body.c_str(), static_cast<DWORD>(body.length()),
                                        static_cast<DWORD>(body.length()), 0);
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (bResults) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize > 0) {
                std::vector<char> buffer(dwSize + 1, 0);
                DWORD downloaded = 0;
                WinHttpReadData(hRequest, buffer.data(), dwSize, &downloaded);
                result.append(buffer.data(), downloaded);
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    try {
        auto respJson = nlohmann::json::parse(result);
        if (respJson.contains("name")) {
            uploadedName = respJson["name"].get<std::string>();
            return true;
        }
    } catch (...) {}
    return false;
#else
    std::string result;
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "http://" + comfyHost + ":" + std::to_string(comfyPort) + "/upload/image";

    struct WriteCallbackData {
        std::string* output;
    };
    WriteCallbackData cbData{&result};

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "image");
    curl_mime_filedata(part, localPath.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        auto* d = static_cast<WriteCallbackData*>(userdata);
        d->output->append(ptr, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbData);

    curl_easy_perform(curl);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    try {
        auto respJson = nlohmann::json::parse(result);
        if (respJson.contains("name")) {
            uploadedName = respJson["name"].get<std::string>();
            return true;
        }
    } catch (...) {}
    return false;
#endif
}

// ============================================================================
// Workflow
// ============================================================================

nlohmann::json SAMSegmentation::prepareSegmentationWorkflow(const std::string& imageName,
                                                             const std::string& prompt) {
    // Load workflow from file if available, otherwise use built-in template
    std::string workflowPath = mainprogram->programData + "/EWOCvj2/comfyui/workflows/sam/segmentation.json";

    nlohmann::json workflow;

    if (fs::exists(workflowPath)) {
        try {
            std::ifstream f(workflowPath);
            workflow = nlohmann::json::parse(f);
        } catch (const std::exception& e) {
            std::cerr << "[SAMSeg] Failed to load workflow file: " << e.what() << std::endl;
        }
    }

    if (workflow.empty()) {
        // Built-in fallback workflow using SAM3Grounding (text-prompted segmentation)
        // SAM3Grounding outputs: [0]=masks(MASK), [1]=visualization(IMAGE), [2]=boxes(STRING), [3]=scores(STRING)
        workflow = {
            {"1", {
                {"class_type", "LoadImage"},
                {"inputs", {{"image", "INPUT_IMAGE"}}}
            }},
            {"2", {
                {"class_type", "LoadSAM3Model"},
                {"inputs", {{"model_path", "models/sam3/sam3.pt"}}}
            }},
            {"3", {
                {"class_type", "SAM3Grounding"},
                {"inputs", {
                    {"sam3_model", nlohmann::json::array({"2", 0})},
                    {"image", nlohmann::json::array({"1", 0})},
                    {"text_prompt", "TEXT_PROMPT"},
                    {"confidence_threshold", scoreThreshold}
                }}
            }},
            {"4", {
                {"class_type", "SaveImage"},
                {"inputs", {
                    {"images", nlohmann::json::array({"3", 1})},
                    {"filename_prefix", "sam_visualization"}
                }}
            }},
            {"5", {
                {"class_type", "MaskToImage"},
                {"inputs", {
                    {"mask", nlohmann::json::array({"3", 0})}
                }}
            }},
            {"6", {
                {"class_type", "SaveImage"},
                {"inputs", {
                    {"images", nlohmann::json::array({"5", 0})},
                    {"filename_prefix", "sam_masks"}
                }}
            }}
        };
    }

    // Substitute placeholders
    for (auto& [key, node] : workflow.items()) {
        if (node.contains("inputs")) {
            auto& inputs = node["inputs"];
            for (auto& [inputKey, inputVal] : inputs.items()) {
                if (inputVal.is_string()) {
                    std::string val = inputVal.get<std::string>();
                    if (val == "INPUT_IMAGE") {
                        inputVal = imageName;
                    } else if (val == "TEXT_PROMPT") {
                        inputVal = prompt;
                    }
                }
            }
            // Override threshold with user setting
            if (inputs.contains("confidence_threshold")) {
                inputs["confidence_threshold"] = scoreThreshold;
            }
            // Prefix filename_prefix with sam3/<name>/ subdirectory
            if (!sam3OutputSubdir.empty() && inputs.contains("filename_prefix")) {
                std::string prefix = inputs["filename_prefix"].get<std::string>();
                inputs["filename_prefix"] = sam3OutputSubdir + "/" + prefix;
            }
        }
    }

    return workflow;
}

bool SAMSegmentation::parseSegmentationOutput(const nlohmann::json& historyData) {
    std::lock_guard<std::mutex> lock(resultMutex);

    currentResult.masks.clear();

    try {
        // Parse outputs from ComfyUI history
        if (!historyData.contains("outputs")) return false;

        auto& outputs = historyData["outputs"];
        int maskId = 0;

        // Two passes: first collect mask images (sam_masks prefix from node 6),
        // then visualization (sam_visualization prefix from node 4)
        std::string visualizationFile;
        std::string visualizationSubfolder;

        // Diagnostic: dump what ComfyUI returned
        for (auto& [nodeId, nodeOutput] : outputs.items()) {
            if (!nodeOutput.contains("images")) continue;
            std::cerr << "[SAMSeg] Node " << nodeId << " returned " << nodeOutput["images"].size() << " images:";
            for (auto& img : nodeOutput["images"]) {
                std::cerr << " " << img.value("filename", "?");
            }
            std::cerr << std::endl;
        }

        for (auto& [nodeId, nodeOutput] : outputs.items()) {
            if (!nodeOutput.contains("images")) continue;

            for (auto& imageInfo : nodeOutput["images"]) {
                std::string filename = imageInfo.value("filename", "");
                std::string subfolder = imageInfo.value("subfolder", "");

                if (filename.empty()) continue;

                bool isMask = (filename.find("sam_masks") != std::string::npos);
                bool isVisualization = (filename.find("sam_visualization") != std::string::npos);

                if (isVisualization) {
                    visualizationFile = filename;
                    visualizationSubfolder = subfolder;
                    continue;
                }

                if (!isMask) continue;

                // Download mask image from ComfyUI
                std::string downloadUrl = "/view?filename=" + filename;
                if (!subfolder.empty()) {
                    downloadUrl += "&subfolder=" + subfolder;
                }

                std::string maskData = httpGet(downloadUrl);
                if (maskData.empty()) continue;

                // Save temporarily and load with DevIL
                std::string tempPath = mainprogram->temppath + "/sam_mask_" + std::to_string(maskId) + ".png";
                {
                    std::ofstream outFile(tempPath, std::ios::binary);
                    outFile.write(maskData.data(), maskData.size());
                }

                int w, h;
                auto maskPixels = ImageLoader::loadImageGray(tempPath, &w, &h);
                if (!maskPixels.empty()) {
                    SegmentationMask mask;
                    mask.id = maskId;
                    mask.label = "segment_" + std::to_string(maskId);
                    mask.confidence = 1.0f;
                    mask.selected = true;  // Default: all selected
                    mask.mask = std::move(maskPixels);
                    const uint8_t* data = mask.mask.data();

                    // Compute bounding box
                    float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++) {
                            if (data[y * w + x] > 128) {
                                float nx = (float)x / w;
                                float ny = (float)y / h;
                                minX = std::min(minX, nx);
                                minY = std::min(minY, ny);
                                maxX = std::max(maxX, nx);
                                maxY = std::max(maxY, ny);
                            }
                        }
                    }
                    mask.bbox[0] = minX;
                    mask.bbox[1] = minY;
                    mask.bbox[2] = maxX;
                    mask.bbox[3] = maxY;

                    currentResult.masks.push_back(std::move(mask));
                    currentResult.width = w;
                    currentResult.height = h;
                    maskId++;
                }

                // Clean up temp file
                fs::remove(tempPath);
            }
        }

        std::cerr << "[SAMSeg] Parsed " << currentResult.masks.size() << " separate mask instances" << std::endl;

        // Load visualization image as outline texture
        if (!visualizationFile.empty()) {
            std::string downloadUrl = "/view?filename=" + visualizationFile;
            if (!visualizationSubfolder.empty()) {
                downloadUrl += "&subfolder=" + visualizationSubfolder;
            }

            std::string vizData = httpGet(downloadUrl);
            if (!vizData.empty()) {
                std::string tempPath = mainprogram->temppath + "/sam_visualization.png";
                {
                    std::ofstream outFile(tempPath, std::ios::binary);
                    outFile.write(vizData.data(), vizData.size());
                }

                int w, h;
                auto vizPixels = ImageLoader::loadImageRGBA(tempPath, &w, &h);
                if (!vizPixels.empty()) {
                    // Store visualization as input image data (for outline rendering)
                    // GL texture will be created on the main thread
                    currentResult.width = w;
                    currentResult.height = h;
                }
                fs::remove(tempPath);
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[SAMSeg] Failed to parse output: " << e.what() << std::endl;
        return false;
    }

    return !currentResult.masks.empty();
}
