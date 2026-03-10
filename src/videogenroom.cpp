/**
 * videogenroom.cpp
 *
 * UI room for ComfyUI-based video and image generation
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

#include <filesystem>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <functional>

#include "ImageLoader.h"

#include "program.h"
#include "videogenroom.h"
#include "BeatDetektor.h"
#include "ComfyUIInstaller.h"
#include "VideoUpscaler.h"

// FFmpeg includes for HAP encoding
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

// Forward declarations
void enddrag();
static void processPendingOutput(VideoGenRoom* room);

// ComfyUI server process tracking
static bool comfyUIServerStarted = false;
static bool comfyUIServerStartAttempted = false;
static bool comfyUIPipInstalled = false;
#ifdef _WIN32
static HANDLE comfyUIProcessHandle = NULL;
#endif

// ============================================================================
// Helper: Run command without console window (Windows)
// ============================================================================

#ifdef _WIN32
/**
 * Run a command silently without showing a console window
 * @param cmd Command to run
 * @param output Output from the command (stdout + stderr)
 * @param exitCode Exit code from the process
 * @param waitForCompletion If true, wait for process to finish
 * @return true if process was created successfully
 */
static bool runCommandHidden(const std::string& cmd, std::string* output = nullptr,
                              int* exitCode = nullptr, bool waitForCompletion = true) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe = NULL, hWritePipe = NULL;

    // Only create pipes if we need output
    if (output) {
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return false;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (output) {
        si.hStdError = hWritePipe;
        si.hStdOutput = hWritePipe;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = cmd;

    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        if (hReadPipe) CloseHandle(hReadPipe);
        if (hWritePipe) CloseHandle(hWritePipe);
        return false;
    }

    if (hWritePipe) CloseHandle(hWritePipe);

    if (output) {
        output->clear();
        char buffer[256];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            *output += buffer;
        }
        CloseHandle(hReadPipe);
    }

    if (waitForCompletion) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        if (exitCode) {
            DWORD dwExitCode;
            GetExitCodeProcess(pi.hProcess, &dwExitCode);
            *exitCode = static_cast<int>(dwExitCode);
        }
        CloseHandle(pi.hProcess);
    } else {
        // Return process handle for background processes
        if (exitCode) *exitCode = 0;
    }

    CloseHandle(pi.hThread);
    return true;
}

/**
 * Launch a background process without console window
 * @param cmd Command to run
 * @param workingDir Working directory (optional)
 * @param outProcessHandle Output: process handle for later termination
 * @return true if process was created successfully
 */
static HANDLE comfyUILogHandle = INVALID_HANDLE_VALUE;

static bool launchProcessHidden(const std::string& cmd, const std::string& workingDir,
                                 HANDLE* outProcessHandle) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdCopy = cmd;
    const char* workDir = workingDir.empty() ? NULL : workingDir.c_str();

    // Redirect stdout/stderr to a log file so we can parse progress
    std::string logPath = mainprogram->temppath + "/comfyui_output.log";
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    comfyUILogHandle = CreateFileA(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comfyUILogHandle != INVALID_HANDLE_VALUE) {
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = comfyUILogHandle;
        si.hStdError = comfyUILogHandle;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    } else {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;
    }

    // Force unbuffered Python output so tqdm progress appears in the log file immediately
    SetEnvironmentVariableA("PYTHONUNBUFFERED", "1");

    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, workDir, &si, &pi)) {
        DWORD err = GetLastError();
        std::cerr << "[VideoGenRoom] CreateProcess failed with error: " << err << std::endl;
        if (comfyUILogHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(comfyUILogHandle);
            comfyUILogHandle = INVALID_HANDLE_VALUE;
        }
        return false;
    }

    std::cerr << "[VideoGenRoom] Process created with PID: " << pi.dwProcessId << std::endl;
    std::cerr << "[VideoGenRoom] Log file: " << logPath << std::endl;

    if (outProcessHandle) {
        *outProcessHandle = pi.hProcess;
    } else {
        CloseHandle(pi.hProcess);
    }
    CloseHandle(pi.hThread);
    return true;
}
#endif

// Stop ComfyUI server and Python processes
void stopComfyUIServer() {
    if (!comfyUIServerStarted) {
        return;
    }

    std::cerr << "[VideoGenRoom] Stopping ComfyUI server..." << std::endl;

#ifdef _WIN32
    // Close the log file handle first — child processes inherited it and
    // won't fully terminate while the parent still holds it open
    if (comfyUILogHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(comfyUILogHandle);
        comfyUILogHandle = INVALID_HANDLE_VALUE;
    }

    // Kill the process tree (/T = tree, kills all child processes too)
    if (comfyUIProcessHandle != NULL) {
        DWORD pid = GetProcessId(comfyUIProcessHandle);
        if (pid != 0) {
            std::string killCmd = "taskkill /F /T /PID " + std::to_string(pid) + " >nul 2>&1";
            runCommandHidden(killCmd);
        }
        TerminateProcess(comfyUIProcessHandle, 0);
        CloseHandle(comfyUIProcessHandle);
        comfyUIProcessHandle = NULL;
    }

    // Kill by port as final fallback
    runCommandHidden("cmd /c \"for /f \"tokens=5\" %a in ('netstat -aon ^| findstr :8188 ^| findstr LISTENING') do taskkill /F /PID %a\" >nul 2>&1");
#else
    // On Linux/Mac, use pkill to find and kill the process
    system("pkill -f 'python.*ComfyUI.*main.py' 2>/dev/null");
    system("pkill -f 'python3.*ComfyUI.*main.py' 2>/dev/null");

    // Also try killing by port
    system("fuser -k 8188/tcp 2>/dev/null");
#endif

    comfyUIServerStarted = false;
    comfyUIServerStartAttempted = false;  // Allow restart later
    std::cerr << "[VideoGenRoom] ComfyUI server stopped" << std::endl;
}

// ============================================================================
// Letterbox Drawing Helper
// ============================================================================

/**
 * Draw a texture in letterbox/pillarbox style within a box
 * Maintains aspect ratio with black bars as needed
 *
 * @param box The bounding box to draw within
 * @param tex The texture to draw
 * @param texWidth Actual width of the texture
 * @param texHeight Actual height of the texture
 */
static void draw_box_letterbox(Boxx* box, GLuint tex, int texWidth, int texHeight) {
    if (!box || tex == (GLuint)-1 || texWidth <= 0 || texHeight <= 0) {
        draw_box(box, tex);
        return;
    }

    float boxX = box->vtxcoords->x1;
    float boxY = box->vtxcoords->y1;
    float boxW = box->vtxcoords->w;
    float boxH = box->vtxcoords->h;

    // Account for screen aspect ratio - GL coords don't map 1:1 to pixels
    float screenAspect = (float)glob->w / (float)glob->h;

    // Calculate aspect ratios
    float texAspect = (float)texWidth / (float)texHeight;
    float boxAspect = boxW / boxH;

    // Box aspect in actual pixels (not GL coords)
    float boxPixelAspect = boxAspect * screenAspect;

    float drawX, drawY, drawW, drawH;

    if (texAspect > boxPixelAspect) {
        // Texture is wider than box - letterbox (black bars top/bottom)
        drawW = boxW;
        drawH = boxW * screenAspect / texAspect;
        drawX = boxX;
        drawY = boxY + (boxH - drawH) / 2.0f;
    } else {
        // Texture is taller than box - pillarbox (black bars left/right)
        drawH = boxH;
        drawW = boxH * texAspect / screenAspect;
        drawX = boxX + (boxW - drawW) / 2.0f;
        drawY = boxY;
    }

    // Draw black background first
    draw_box(black, black, boxX, boxY, boxW, boxH, -1);

    // Draw texture with letterbox coordinates
    draw_box(nullptr, nullptr, drawX, drawY, drawW, drawH, tex);
}

// ============================================================================
// HAP Encoding from Frame Sequence
// ============================================================================

/**
 * Encode a sequence of PNG frames to HAP video format
 *
 * @param framesDir Directory containing PNG frames (frame_00001.png, frame_00002.png, etc.)
 * @param outputPath Output path for the .mov file
 * @param fps Frame rate for the output video
 * @param progressCallback Optional callback for progress updates (0.0-1.0)
 * @return true on success, false on failure
 */
static bool hapEncodeFrames(const std::string& framesDir,
                            const std::string& outputPath,
                            float fps,
                            std::function<void(float)> progressCallback = nullptr) {
    std::cerr << "[HAP Encode] Starting frame-to-HAP encoding" << std::endl;
    std::cerr << "[HAP Encode] Frames dir: " << framesDir << std::endl;
    std::cerr << "[HAP Encode] Output: " << outputPath << std::endl;
    std::cerr << "[HAP Encode] FPS: " << fps << std::endl;

    // Collect and sort frame files
    std::vector<std::string> framePaths;
    for (const auto& entry : std::filesystem::directory_iterator(framesDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                framePaths.push_back(entry.path().string());
            }
        }
    }

    if (framePaths.empty()) {
        std::cerr << "[HAP Encode] No frames found in " << framesDir << std::endl;
        return false;
    }

    std::sort(framePaths.begin(), framePaths.end());
    int numFrames = (int)framePaths.size();
    std::cerr << "[HAP Encode] Found " << numFrames << " frames" << std::endl;

    // Load first frame to get dimensions
    int srcWidth, srcHeight;
    if (!ImageLoader::getImageDimensions(framePaths[0], &srcWidth, &srcHeight)) {
        std::cerr << "[HAP Encode] Failed to load first frame: " << framePaths[0] << std::endl;
        return false;
    }

    std::cerr << "[HAP Encode] Frame dimensions: " << srcWidth << "x" << srcHeight << std::endl;

    // Find HAP encoder
    const AVCodec* codec = avcodec_find_encoder_by_name("hap");
    if (!codec) {
        std::cerr << "[HAP Encode] HAP codec not found - ffmpeg compiled without snappy support" << std::endl;
        return false;
    }

    // Setup encoder context
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        std::cerr << "[HAP Encode] Failed to allocate codec context" << std::endl;
        return false;
    }

    // HAP alignment requirements: width multiple of 32, height multiple of 4
    int rem = srcWidth % 32;
    c->width = srcWidth + (32 - rem) * (rem > 0);
    rem = srcHeight % 4;
    c->height = srcHeight + (4 - rem) * (rem > 0);

    // Use simple integer time_base: 1 tick per frame
    int fpsInt = (int)(fps + 0.5f);  // Round to nearest integer
    if (fpsInt <= 0) fpsInt = 24;
    c->time_base = {1, fpsInt};
    c->framerate = {fpsInt, 1};
    c->pix_fmt = codec->pix_fmts[0];  // HAP uses RGBA or similar

    // Force Snappy compression: compressor value is the high nibble of section type
    // Range is [160-176] = [0xA0-0xB0]: 160/0xA0=none, 176/0xB0=snappy
    // 0xB0 (176) + 0x0B (DXT1) = 0xBB (187) = Snappy-compressed DXT1
    AVDictionary* opts = nullptr;
    av_dict_set_int(&opts, "compressor", 0xB0, 0);  // 176 = Snappy compression

    int openRet = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);

    if (openRet < 0) {
        std::cerr << "[HAP Encode] Failed to open HAP encoder" << std::endl;
        avcodec_free_context(&c);
        return false;
    }

    // Setup output format context
    AVFormatContext* dest = nullptr;
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"),
                                   nullptr, outputPath.c_str());
    if (!dest) {
        std::cerr << "[HAP Encode] Failed to allocate output context" << std::endl;
        avcodec_free_context(&c);
        return false;
    }

    AVStream* destStream = avformat_new_stream(dest, codec);
    destStream->time_base = c->time_base;
    destStream->r_frame_rate = c->framerate;
    avcodec_parameters_from_context(destStream->codecpar, c);

    if (avio_open(&dest->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        std::cerr << "[HAP Encode] Failed to open output file" << std::endl;
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    if (avformat_write_header(dest, nullptr) < 0) {
        std::cerr << "[HAP Encode] Failed to write header" << std::endl;
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    // Allocate frame for encoding
    AVFrame* frame = av_frame_alloc();
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    // Setup color space converter (RGBA -> HAP format)
    SwsContext* swsCtx = sws_getContext(
        srcWidth, srcHeight, AV_PIX_FMT_RGBA,
        c->width, c->height, c->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "[HAP Encode] Failed to create SwsContext" << std::endl;
        av_frame_free(&frame);
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    int frameIdx = 0;
    bool success = true;

    // Process each frame
    for (const auto& framePath : framePaths) {
        int imgW, imgH;
        auto imagePixels = ImageLoader::loadImageRGBA(framePath, &imgW, &imgH);
        if (imagePixels.empty()) {
            std::cerr << "[HAP Encode] Failed to load frame: " << framePath << std::endl;
            continue;
        }

        // Allocate frame data
        if (av_image_alloc(frame->data, frame->linesize,
                          c->width, c->height, c->pix_fmt, 32) < 0) {
            std::cerr << "[HAP Encode] Failed to allocate frame data" << std::endl;
            success = false;
            break;
        }

        // Setup source pointers for sws_scale
        const uint8_t* srcData[4] = { imagePixels.data(), nullptr, nullptr, nullptr };
        int srcLinesize[4] = { srcWidth * 4, 0, 0, 0 };

        // Convert RGBA to HAP format
        sws_scale(swsCtx, srcData, srcLinesize, 0, srcHeight, frame->data, frame->linesize);

        frame->pts = frameIdx;  // Simple: 1 tick per frame with time_base {1, fps}

        // Encode frame
        int ret = avcodec_send_frame(c, frame);
        if (ret < 0) {
            std::cerr << "[HAP Encode] Error sending frame to encoder" << std::endl;
            av_freep(&frame->data[0]);
            success = false;
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                std::cerr << "[HAP Encode] Error receiving packet" << std::endl;
                success = false;
                break;
            }

            av_packet_rescale_ts(pkt, c->time_base, destStream->time_base);
            pkt->stream_index = destStream->index;
            av_interleaved_write_frame(dest, pkt);
            av_packet_unref(pkt);
        }

        av_freep(&frame->data[0]);

        frameIdx++;
        if (progressCallback) {
            progressCallback((float)frameIdx / (float)numFrames);
        }

        if (!success) break;
    }

    // Flush encoder
    if (success) {
        avcodec_send_frame(c, nullptr);
        while (true) {
            int ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;
            av_packet_rescale_ts(pkt, c->time_base, destStream->time_base);
            pkt->stream_index = destStream->index;
            av_interleaved_write_frame(dest, pkt);
            av_packet_unref(pkt);
        }
    }

    // Write trailer and cleanup
    av_write_trailer(dest);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    avio_close(dest->pb);
    avformat_free_context(dest);
    avcodec_free_context(&c);

    if (success) {
        std::cerr << "[HAP Encode] Successfully encoded " << frameIdx << " frames to " << outputPath << std::endl;
    }
    return success;
}

/**
 * Encode a sequence of PNG frames to H.264 MP4 video
 * Used when HAP output is disabled
 */
static bool h264EncodeFrames(const std::string& framesDir,
                              const std::string& outputPath,
                              float fps,
                              std::function<void(float)> progressCallback = nullptr) {
    std::cerr << "[H264 Encode] Starting frame-to-H264 encoding" << std::endl;
    std::cerr << "[H264 Encode] Frames dir: " << framesDir << std::endl;
    std::cerr << "[H264 Encode] Output: " << outputPath << std::endl;

    // Collect and sort frame files
    std::vector<std::string> framePaths;
    for (const auto& entry : std::filesystem::directory_iterator(framesDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                framePaths.push_back(entry.path().string());
            }
        }
    }

    if (framePaths.empty()) {
        std::cerr << "[H264 Encode] No frames found" << std::endl;
        return false;
    }

    std::sort(framePaths.begin(), framePaths.end());
    int numFrames = (int)framePaths.size();
    std::cerr << "[H264 Encode] Found " << numFrames << " frames" << std::endl;

    // Load first frame to get dimensions
    int srcWidth, srcHeight;
    if (!ImageLoader::getImageDimensions(framePaths[0], &srcWidth, &srcHeight)) {
        std::cerr << "[H264 Encode] Failed to load first frame" << std::endl;
        return false;
    }

    // H.264 requires even dimensions
    int encWidth = (srcWidth + 1) & ~1;
    int encHeight = (srcHeight + 1) & ~1;

    std::cerr << "[H264 Encode] Frame dimensions: " << srcWidth << "x" << srcHeight
              << " -> " << encWidth << "x" << encHeight << std::endl;

    // Find H.264 encoder
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "[H264 Encode] H.264 codec not found" << std::endl;
        return false;
    }

    // Setup encoder context
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        std::cerr << "[H264 Encode] Failed to allocate codec context" << std::endl;
        return false;
    }

    c->width = encWidth;
    c->height = encHeight;

    // Use simple integer time_base: 1 tick per frame
    int fpsInt = (int)(fps + 0.5f);  // Round to nearest integer
    if (fpsInt <= 0) fpsInt = 24;
    c->time_base = {1, fpsInt};
    c->framerate = {fpsInt, 1};
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->gop_size = 12;
    c->max_b_frames = 2;

    // Quality settings
    av_opt_set(c->priv_data, "preset", "medium", 0);
    av_opt_set(c->priv_data, "crf", "18", 0);  // High quality

    if (avcodec_open2(c, codec, nullptr) < 0) {
        std::cerr << "[H264 Encode] Failed to open H.264 encoder" << std::endl;
        avcodec_free_context(&c);
        return false;
    }

    // Setup output format context
    AVFormatContext* dest = nullptr;
    avformat_alloc_output_context2(&dest, nullptr, "mp4", outputPath.c_str());
    if (!dest) {
        std::cerr << "[H264 Encode] Failed to allocate output context" << std::endl;
        avcodec_free_context(&c);
        return false;
    }

    AVStream* destStream = avformat_new_stream(dest, codec);
    destStream->time_base = c->time_base;
    destStream->r_frame_rate = c->framerate;
    avcodec_parameters_from_context(destStream->codecpar, c);

    if (avio_open(&dest->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        std::cerr << "[H264 Encode] Failed to open output file" << std::endl;
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    if (avformat_write_header(dest, nullptr) < 0) {
        std::cerr << "[H264 Encode] Failed to write header" << std::endl;
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    // Allocate frame for encoding
    AVFrame* frame = av_frame_alloc();
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    // Setup color space converter (RGBA -> YUV420P)
    SwsContext* swsCtx = sws_getContext(
        srcWidth, srcHeight, AV_PIX_FMT_RGBA,
        c->width, c->height, c->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "[H264 Encode] Failed to create SwsContext" << std::endl;
        av_frame_free(&frame);
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    int frameIdx = 0;
    bool success = true;

    // Process each frame
    for (const auto& framePath : framePaths) {
        int imgW, imgH;
        auto imagePixels = ImageLoader::loadImageRGBA(framePath, &imgW, &imgH);
        if (imagePixels.empty()) continue;

        if (av_image_alloc(frame->data, frame->linesize,
                          c->width, c->height, c->pix_fmt, 32) < 0) {
            success = false;
            break;
        }

        const uint8_t* srcData[4] = { imagePixels.data(), nullptr, nullptr, nullptr };
        int srcLinesize[4] = { srcWidth * 4, 0, 0, 0 };

        sws_scale(swsCtx, srcData, srcLinesize, 0, srcHeight, frame->data, frame->linesize);
        frame->pts = frameIdx;  // Simple: 1 tick per frame with time_base {1, fps}

        int ret = avcodec_send_frame(c, frame);
        if (ret < 0) {
            av_freep(&frame->data[0]);
            success = false;
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) { success = false; break; }

            av_packet_rescale_ts(pkt, c->time_base, destStream->time_base);
            pkt->stream_index = destStream->index;
            av_interleaved_write_frame(dest, pkt);
            av_packet_unref(pkt);
        }

        av_freep(&frame->data[0]);

        frameIdx++;
        if (progressCallback) {
            progressCallback((float)frameIdx / (float)numFrames);
        }

        if (!success) break;
    }

    // Flush encoder
    if (success) {
        avcodec_send_frame(c, nullptr);
        while (true) {
            int ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;
            av_packet_rescale_ts(pkt, c->time_base, destStream->time_base);
            pkt->stream_index = destStream->index;
            av_interleaved_write_frame(dest, pkt);
            av_packet_unref(pkt);
        }
    }

    av_write_trailer(dest);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    avio_close(dest->pb);
    avformat_free_context(dest);
    avcodec_free_context(&c);

    if (success) {
        std::cerr << "[H264 Encode] Successfully encoded " << frameIdx << " frames" << std::endl;
    }
    return success;
}

/**
 * Re-encode a video file to HAP format (video-to-HAP)
 * Uses the video's original frame rate
 */
static bool hapEncodeVideo(const std::string& inputPath,
                           const std::string& outputPath,
                           std::function<void(float)> progressCallback = nullptr) {
    std::cerr << "[HAP Encode] Re-encoding video to HAP: " << inputPath << std::endl;

    // Open input video
    AVFormatContext* source = nullptr;
    if (avformat_open_input(&source, inputPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[HAP Encode] Failed to open input video" << std::endl;
        return false;
    }

    if (avformat_find_stream_info(source, nullptr) < 0) {
        std::cerr << "[HAP Encode] Failed to find stream info" << std::endl;
        avformat_close_input(&source);
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned i = 0; i < source->nb_streams; i++) {
        if (source->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx < 0) {
        std::cerr << "[HAP Encode] No video stream found" << std::endl;
        avformat_close_input(&source);
        return false;
    }

    AVStream* srcStream = source->streams[videoStreamIdx];
    AVCodecParameters* srcParams = srcStream->codecpar;

    // Check if already HAP
    if (srcParams->codec_id == AV_CODEC_ID_HAP) {
        std::cerr << "[HAP Encode] Video is already HAP encoded" << std::endl;
        avformat_close_input(&source);
        return true;
    }

    // Setup decoder
    const AVCodec* decoder = avcodec_find_decoder(srcParams->codec_id);
    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, srcParams);
    avcodec_open2(decCtx, decoder, nullptr);

    // Find HAP encoder
    const AVCodec* encoder = avcodec_find_encoder_by_name("hap");
    if (!encoder) {
        std::cerr << "[HAP Encode] HAP codec not available" << std::endl;
        avcodec_free_context(&decCtx);
        avformat_close_input(&source);
        return false;
    }

    // Setup encoder
    AVCodecContext* encCtx = avcodec_alloc_context3(encoder);
    int rem = srcParams->width % 32;
    encCtx->width = srcParams->width + (32 - rem) * (rem > 0);
    rem = srcParams->height % 4;
    encCtx->height = srcParams->height + (4 - rem) * (rem > 0);
    encCtx->time_base = srcStream->time_base;
    encCtx->framerate = srcStream->r_frame_rate;
    encCtx->pix_fmt = AV_PIX_FMT_RGBA;  // Force RGBA input -> DXT1 output
    // Force Snappy compression
    AVDictionary* opts = nullptr;
    av_dict_set_int(&opts, "compressor", 0xB0, 0);  // 176 = Snappy compression
    avcodec_open2(encCtx, encoder, &opts);
    av_dict_free(&opts);

    // Setup output
    AVFormatContext* dest = nullptr;
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"),
                                   nullptr, outputPath.c_str());
    AVStream* destStream = avformat_new_stream(dest, encoder);
    destStream->time_base = srcStream->time_base;
    destStream->r_frame_rate = srcStream->r_frame_rate;
    avcodec_parameters_from_context(destStream->codecpar, encCtx);

    avio_open(&dest->pb, outputPath.c_str(), AVIO_FLAG_WRITE);
    avformat_write_header(dest, nullptr);

    // Setup color converter
    SwsContext* swsCtx = sws_getContext(
        decCtx->width, decCtx->height, decCtx->pix_fmt,
        encCtx->width, encCtx->height, encCtx->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVFrame* decFrame = av_frame_alloc();
    AVFrame* encFrame = av_frame_alloc();
    encFrame->format = encCtx->pix_fmt;
    encFrame->width = encCtx->width;
    encFrame->height = encCtx->height;

    AVPacket* pkt = av_packet_alloc();
    int64_t numFrames = srcStream->nb_frames > 0 ? srcStream->nb_frames :
        (int64_t)(source->duration * srcStream->avg_frame_rate.num /
                  srcStream->avg_frame_rate.den / 1000000);
    int frameCount = 0;

    // Transcode
    while (av_read_frame(source, pkt) >= 0) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }

        avcodec_send_packet(decCtx, pkt);
        av_packet_unref(pkt);

        while (avcodec_receive_frame(decCtx, decFrame) >= 0) {
            av_image_alloc(encFrame->data, encFrame->linesize,
                          encCtx->width, encCtx->height, encCtx->pix_fmt, 32);

            sws_scale(swsCtx, decFrame->data, decFrame->linesize, 0,
                     decFrame->height, encFrame->data, encFrame->linesize);

            encFrame->pts = decFrame->pts;

            avcodec_send_frame(encCtx, encFrame);
            while (avcodec_receive_packet(encCtx, pkt) >= 0) {
                av_packet_rescale_ts(pkt, encCtx->time_base, destStream->time_base);
                pkt->stream_index = destStream->index;
                av_interleaved_write_frame(dest, pkt);
                av_packet_unref(pkt);
            }

            av_freep(&encFrame->data[0]);
            av_frame_unref(decFrame);

            frameCount++;
            if (progressCallback && numFrames > 0) {
                progressCallback((float)frameCount / (float)numFrames);
            }
        }
    }

    // Flush
    avcodec_send_frame(encCtx, nullptr);
    while (avcodec_receive_packet(encCtx, pkt) >= 0) {
        av_packet_rescale_ts(pkt, encCtx->time_base, destStream->time_base);
        pkt->stream_index = destStream->index;
        av_interleaved_write_frame(dest, pkt);
        av_packet_unref(pkt);
    }

    av_write_trailer(dest);

    // Cleanup
    av_packet_free(&pkt);
    av_frame_free(&decFrame);
    av_frame_free(&encFrame);
    sws_freeContext(swsCtx);
    avio_close(dest->pb);
    avformat_free_context(dest);
    avcodec_free_context(&encCtx);
    avcodec_free_context(&decCtx);
    avformat_close_input(&source);

    std::cerr << "[HAP Encode] Successfully re-encoded to HAP: " << outputPath << std::endl;
    return true;
}

/**
 * Extract all frames from a video to a directory as PNG files
 * Used for VIDEO_CONTINUATION to prepend source video frames
 * @param videoPath Path to source video
 * @param outputDir Directory to write frames (frame_00001.png, etc.)
 * @param startIndex Starting frame number (for proper naming)
 * @return Number of frames extracted, or -1 on error
 */
static int extractVideoFrames(const std::string& videoPath,
                               const std::string& outputDir,
                               int startIndex = 1) {
    std::cerr << "[Frame Extract] Extracting frames from: " << videoPath << std::endl;

    AVFormatContext* source = nullptr;
    if (avformat_open_input(&source, videoPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[Frame Extract] Failed to open video" << std::endl;
        return -1;
    }

    if (avformat_find_stream_info(source, nullptr) < 0) {
        avformat_close_input(&source);
        return -1;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned i = 0; i < source->nb_streams; i++) {
        if (source->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx < 0) {
        avformat_close_input(&source);
        return -1;
    }

    AVStream* srcStream = source->streams[videoStreamIdx];
    AVCodecParameters* srcParams = srcStream->codecpar;

    // Setup decoder
    const AVCodec* decoder = avcodec_find_decoder(srcParams->codec_id);
    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, srcParams);
    avcodec_open2(decCtx, decoder, nullptr);

    // Setup PNG encoder
    const AVCodec* pngEncoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    AVCodecContext* pngCtx = avcodec_alloc_context3(pngEncoder);
    pngCtx->width = srcParams->width;
    pngCtx->height = srcParams->height;
    pngCtx->pix_fmt = AV_PIX_FMT_RGB24;
    pngCtx->time_base = {1, 25};
    avcodec_open2(pngCtx, pngEncoder, nullptr);

    // Setup scaler to RGB
    SwsContext* swsCtx = sws_getContext(
        srcParams->width, srcParams->height, decCtx->pix_fmt,
        srcParams->width, srcParams->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = srcParams->width;
    rgbFrame->height = srcParams->height;
    av_frame_get_buffer(rgbFrame, 32);

    int frameCount = 0;

    while (av_read_frame(source, pkt) >= 0) {
        if (pkt->stream_index == videoStreamIdx) {
            if (avcodec_send_packet(decCtx, pkt) >= 0) {
                while (avcodec_receive_frame(decCtx, frame) >= 0) {
                    // Convert to RGB
                    sws_scale(swsCtx, frame->data, frame->linesize, 0,
                              srcParams->height, rgbFrame->data, rgbFrame->linesize);

                    rgbFrame->pts = 0;

                    // Encode to PNG
                    AVPacket* pngPkt = av_packet_alloc();
                    avcodec_send_frame(pngCtx, rgbFrame);
                    if (avcodec_receive_packet(pngCtx, pngPkt) >= 0) {
                        // Write PNG file
                        char filename[256];
                        snprintf(filename, sizeof(filename), "%s/frame_%05d.png",
                                 outputDir.c_str(), startIndex + frameCount);
                        std::ofstream outFile(filename, std::ios::binary);
                        if (outFile.is_open()) {
                            outFile.write(reinterpret_cast<const char*>(pngPkt->data), pngPkt->size);
                            outFile.close();
                            frameCount++;
                        }
                    }
                    av_packet_free(&pngPkt);
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Cleanup
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    sws_freeContext(swsCtx);
    avcodec_free_context(&pngCtx);
    avcodec_free_context(&decCtx);
    avformat_close_input(&source);

    std::cerr << "[Frame Extract] Extracted " << frameCount << " frames" << std::endl;
    return frameCount;
}

/**
 * Renumber/copy frames to start at a given index
 * Used to append generated frames after source frames
 */
static bool renumberFrames(const std::string& srcDir, const std::string& destDir, int startIndex) {
    std::vector<std::string> framePaths;
    for (const auto& entry : std::filesystem::directory_iterator(srcDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                framePaths.push_back(entry.path().string());
            }
        }
    }

    std::sort(framePaths.begin(), framePaths.end());

    for (size_t i = 0; i < framePaths.size(); i++) {
        char destFilename[256];
        snprintf(destFilename, sizeof(destFilename), "%s/frame_%05d.png",
                 destDir.c_str(), startIndex + (int)i);
        std::filesystem::copy_file(framePaths[i], destFilename,
                                   std::filesystem::copy_options::overwrite_existing);
    }

    return true;
}

/**
 * Check if a video file is HAP encoded
 */
static bool isHAPVideo(const std::string& videoPath) {
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, videoPath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    if (avformat_find_stream_info(ctx, nullptr) < 0) {
        avformat_close_input(&ctx);
        return false;
    }

    bool isHAP = false;
    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // HAP codec IDs: 187 (HAP), 188 (HAP Alpha), 189 (HAP Q)
            int codecId = ctx->streams[i]->codecpar->codec_id;
            isHAP = (codecId == AV_CODEC_ID_HAP || codecId == 187 || codecId == 188 || codecId == 189);
            break;
        }
    }

    avformat_close_input(&ctx);
    return isHAP;
}

/**
 * Optimized HAP continuation: passthrough source HAP packets + encode only new frames
 * Avoids re-encoding source video when it's already HAP
 *
 * @param sourceHapPath Path to source HAP video (must be HAP encoded)
 * @param newFramesDir Directory containing new PNG frames to append
 * @param outputPath Output HAP .mov path
 * @param fps Frame rate for output
 * @return true on success
 */
static bool hapContinuationPassthrough(const std::string& sourceHapPath,
                                        const std::string& newFramesDir,
                                        const std::string& outputPath,
                                        float fps) {
    std::cerr << "[HAP Passthrough] Source: " << sourceHapPath << std::endl;
    std::cerr << "[HAP Passthrough] New frames: " << newFramesDir << std::endl;
    std::cerr << "[HAP Passthrough] Output: " << outputPath << std::endl;

    // Open source HAP video
    AVFormatContext* source = nullptr;
    if (avformat_open_input(&source, sourceHapPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[HAP Passthrough] Failed to open source" << std::endl;
        return false;
    }
    avformat_find_stream_info(source, nullptr);

    // Find video stream
    int srcVideoIdx = -1;
    for (unsigned i = 0; i < source->nb_streams; i++) {
        if (source->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            srcVideoIdx = i;
            break;
        }
    }
    if (srcVideoIdx < 0) {
        avformat_close_input(&source);
        return false;
    }

    AVStream* srcStream = source->streams[srcVideoIdx];
    AVCodecParameters* srcParams = srcStream->codecpar;

    // Collect and sort new frame files
    std::vector<std::string> newFramePaths;
    for (const auto& entry : std::filesystem::directory_iterator(newFramesDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                newFramePaths.push_back(entry.path().string());
            }
        }
    }
    std::sort(newFramePaths.begin(), newFramePaths.end());
    int newFrameCount = (int)newFramePaths.size();

    if (newFrameCount == 0) {
        std::cerr << "[HAP Passthrough] No new frames found" << std::endl;
        avformat_close_input(&source);
        return false;
    }

    // Setup HAP encoder for new frames
    const AVCodec* hapCodec = avcodec_find_encoder_by_name("hap");
    if (!hapCodec) {
        std::cerr << "[HAP Passthrough] HAP codec not found" << std::endl;
        avformat_close_input(&source);
        return false;
    }

    AVCodecContext* encCtx = avcodec_alloc_context3(hapCodec);
    // Match source dimensions (with HAP alignment)
    int rem = srcParams->width % 32;
    encCtx->width = srcParams->width + (32 - rem) * (rem > 0);
    rem = srcParams->height % 4;
    encCtx->height = srcParams->height + (4 - rem) * (rem > 0);

    int fpsInt = (int)(fps + 0.5f);
    if (fpsInt <= 0) fpsInt = 24;
    encCtx->time_base = {1, fpsInt};
    encCtx->framerate = {fpsInt, 1};
    encCtx->pix_fmt = AV_PIX_FMT_RGBA;

    // Force Snappy compression
    AVDictionary* opts = nullptr;
    av_dict_set_int(&opts, "compressor", 0xB0, 0);  // 176 = Snappy compression
    if (avcodec_open2(encCtx, hapCodec, &opts) < 0) {
        av_dict_free(&opts);
        std::cerr << "[HAP Passthrough] Failed to open HAP encoder" << std::endl;
        avcodec_free_context(&encCtx);
        avformat_close_input(&source);
        return false;
    }
    av_dict_free(&opts);

    // Setup output
    AVFormatContext* dest = nullptr;
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"),
                                   nullptr, outputPath.c_str());
    if (!dest) {
        avcodec_free_context(&encCtx);
        avformat_close_input(&source);
        return false;
    }

    AVStream* destStream = avformat_new_stream(dest, hapCodec);
    destStream->time_base = encCtx->time_base;
    destStream->r_frame_rate = encCtx->framerate;
    destStream->avg_frame_rate = encCtx->framerate;
    // Copy codec params from source (they're compatible since source is HAP)
    avcodec_parameters_copy(destStream->codecpar, srcParams);
    // Update dimensions to match encoder (aligned)
    destStream->codecpar->width = encCtx->width;
    destStream->codecpar->height = encCtx->height;

    if (avio_open(&dest->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        avformat_free_context(dest);
        avcodec_free_context(&encCtx);
        avformat_close_input(&source);
        return false;
    }

    avformat_write_header(dest, nullptr);

    // Log actual time_base after muxer initialization (muxer may have changed it)
    std::cerr << "[HAP Passthrough] Stream time_base: " << destStream->time_base.num
              << "/" << destStream->time_base.den << std::endl;

    // Phase 1: Copy source HAP packets directly (passthrough - no re-encoding!)
    std::cerr << "[HAP Passthrough] Copying source packets..." << std::endl;
    AVPacket* pkt = av_packet_alloc();
    int64_t srcFrameCount = 0;

    // Calculate duration per frame in actual dest time_base
    // If time_base is {1, fps}, frameDuration = 1
    // If time_base is {1, 1000}, frameDuration = 1000/fps
    int64_t frameDuration = av_rescale_q(1, encCtx->time_base, destStream->time_base);
    if (frameDuration <= 0) frameDuration = 1;  // Safety fallback
    std::cerr << "[HAP Passthrough] Frame duration: " << frameDuration << std::endl;

    while (av_read_frame(source, pkt) >= 0) {
        if (pkt->stream_index == srcVideoIdx) {
            // Set PTS/DTS based on frame count for consistent timing
            pkt->pts = srcFrameCount * frameDuration;
            pkt->dts = pkt->pts;
            pkt->duration = frameDuration;
            pkt->stream_index = destStream->index;
            av_interleaved_write_frame(dest, pkt);
            srcFrameCount++;
        }
        av_packet_unref(pkt);
    }
    std::cerr << "[HAP Passthrough] Copied " << srcFrameCount << " source packets" << std::endl;

    // New frames will continue from source frame count
    // We'll use encoder time_base (simple frame index) then rescale to dest time_base
    int64_t newFrameStartIdx = srcFrameCount;  // Frame index where new frames start

    // Phase 2: Encode new frames and append
    std::cerr << "[HAP Passthrough] Encoding " << newFrameCount << " new frames..." << std::endl;

    // Setup color converter for new frames
    SwsContext* swsCtx = sws_getContext(
        srcParams->width, srcParams->height, AV_PIX_FMT_RGBA,
        encCtx->width, encCtx->height, encCtx->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVFrame* frame = av_frame_alloc();
    frame->format = encCtx->pix_fmt;
    frame->width = encCtx->width;
    frame->height = encCtx->height;

    for (int i = 0; i < newFrameCount; i++) {
        int imgWidth, imgHeight;
        auto imagePixels = ImageLoader::loadImageRGBA(newFramePaths[i], &imgWidth, &imgHeight);
        if (imagePixels.empty()) continue;

        // Allocate frame buffer
        if (av_image_alloc(frame->data, frame->linesize,
                          encCtx->width, encCtx->height, encCtx->pix_fmt, 32) < 0) {
            continue;
        }

        // Scale/convert to HAP format
        const uint8_t* srcData[4] = { imagePixels.data(), nullptr, nullptr, nullptr };
        int srcLinesize[4] = { imgWidth * 4, 0, 0, 0 };
        sws_scale(swsCtx, srcData, srcLinesize, 0, imgHeight, frame->data, frame->linesize);

        // Frame PTS in encoder time_base (simple frame index starting from 0)
        frame->pts = i;

        // Encode
        avcodec_send_frame(encCtx, frame);
        while (avcodec_receive_packet(encCtx, pkt) >= 0) {
            // Rescale from encoder time_base to dest time_base
            av_packet_rescale_ts(pkt, encCtx->time_base, destStream->time_base);
            // Add offset for frames after source
            pkt->pts += newFrameStartIdx * frameDuration;
            pkt->dts = pkt->pts;
            pkt->stream_index = destStream->index;
            pkt->duration = frameDuration;
            av_interleaved_write_frame(dest, pkt);
            av_packet_unref(pkt);
        }

        av_freep(&frame->data[0]);
    }

    // Flush encoder
    avcodec_send_frame(encCtx, nullptr);
    while (avcodec_receive_packet(encCtx, pkt) >= 0) {
        // Rescale from encoder time_base to dest time_base
        av_packet_rescale_ts(pkt, encCtx->time_base, destStream->time_base);
        // Add offset for frames after source
        pkt->pts += newFrameStartIdx * frameDuration;
        pkt->dts = pkt->pts;
        pkt->stream_index = destStream->index;
        pkt->duration = frameDuration;
        av_interleaved_write_frame(dest, pkt);
        av_packet_unref(pkt);
    }

    int64_t totalFrames = srcFrameCount + newFrameCount;
    float totalDuration = (float)totalFrames / fpsInt;
    std::cerr << "[HAP Passthrough] Complete: " << srcFrameCount << " source + "
              << newFrameCount << " new = " << totalFrames << " total frames"
              << " (~" << totalDuration << "s at " << fpsInt << "fps)" << std::endl;

    // Cleanup
    av_packet_free(&pkt);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    av_write_trailer(dest);
    avio_close(dest->pb);
    avformat_free_context(dest);
    avcodec_free_context(&encCtx);
    avformat_close_input(&source);

    return true;
}

// Helper to install ComfyUI Python requirements
static bool installComfyUIRequirements(const std::string& comfyDir) {
    if (comfyUIPipInstalled) {
        return true;
    }

    std::cerr << "[VideoGenRoom] Installing ComfyUI Python requirements..." << std::endl;

    // Get Python path from environment
    const char* envPython = std::getenv("EWOCVJ2_PYTHON");
    if (!envPython || envPython[0] == '\0') {
        std::cerr << "[VideoGenRoom] Error: EWOCVJ2_PYTHON not set" << std::endl;
        return false;
    }

    // Find pip in Scripts directory relative to Python
    std::filesystem::path pythonPath(envPython);
    std::filesystem::path scriptsDir = pythonPath.parent_path() / "Scripts";
    std::filesystem::path pipPath = scriptsDir / "pip.exe";

    if (!std::filesystem::exists(pipPath)) {
        // Try pip3
        pipPath = scriptsDir / "pip3.exe";
    }

    if (!std::filesystem::exists(pipPath)) {
        std::cerr << "[VideoGenRoom] Warning: pip not found at " << scriptsDir.string() << std::endl;
        std::cerr << "[VideoGenRoom] Trying python -m pip instead..." << std::endl;
        pipPath = "";  // Will use python -m pip
    }

    // Find requirements.txt in ComfyUI directory
    std::filesystem::path requirementsPath = std::filesystem::path(comfyDir) / "requirements.txt";
    if (!std::filesystem::exists(requirementsPath)) {
        std::cerr << "[VideoGenRoom] Warning: requirements.txt not found at " << requirementsPath.string() << std::endl;
        comfyUIPipInstalled = true;  // Don't retry
        return true;  // Not fatal - might already be installed
    }

    std::cerr << "[VideoGenRoom] Found requirements.txt: " << requirementsPath.string() << std::endl;

    // Build pip install command
    std::string cmd;
#ifdef _WIN32
    if (!pipPath.empty()) {
        cmd = "\"" + pipPath.string() + "\" install -r \"" + requirementsPath.string() + "\"";
    } else {
        cmd = "\"" + std::string(envPython) + "\" -m pip install -r \"" + requirementsPath.string() + "\"";
    }

    std::cerr << "[VideoGenRoom] Running: " << cmd << std::endl;

    // Run pip install hidden (no console window)
    // OLD: int result = system(cmd.c_str());
    std::string pipOutput;
    int result = 0;
    if (!runCommandHidden(cmd, &pipOutput, &result)) {
        std::cerr << "[VideoGenRoom] Warning: Failed to run pip install" << std::endl;
    } else if (result != 0) {
        std::cerr << "[VideoGenRoom] Warning: pip install returned " << result << std::endl;
        std::cerr << "[VideoGenRoom] pip output: " << pipOutput << std::endl;
        // Don't fail - requirements might already be satisfied
    }
#else
    cmd = "\"" + std::string(envPython) + "\" -m pip install -r \"" + requirementsPath.string() + "\"";
    std::cerr << "[VideoGenRoom] Running: " << cmd << std::endl;
    system(cmd.c_str());
#endif

    comfyUIPipInstalled = true;
    std::cerr << "[VideoGenRoom] Python requirements installation complete" << std::endl;
    return true;
}

// Helper to start ComfyUI server using EWOCVJ2_PYTHON
// statusCallback is called with status updates for UI display
bool startComfyUIServer(std::function<void(const std::string&)> statusCallback) {
    auto updateStatus = [&](const std::string& status) {
        if (statusCallback) statusCallback(status);
        std::cerr << "[VideoGenRoom] " << status << std::endl;
    };

    if (comfyUIServerStarted || comfyUIServerStartAttempted) {
        return comfyUIServerStarted;
    }
    comfyUIServerStartAttempted = true;

    updateStatus("Checking ComfyUI installation...");

    // Check if ComfyUI is installed
    std::string installDir = mainprogram->programData + "/EWOCvj2/ComfyUI";
    if (!ComfyUIInstaller::isComfyUIInstalled(installDir)) {
        updateStatus("Installing ComfyUI (first-time setup)...");

        ComfyUIInstaller installer;
        InstallConfig config;
        config.installDir = installDir;
        config.tempDir = installDir + "/temp";

        installer.setProgressCallback([&updateStatus](const InstallProgress& p) {
            std::string msg = "Installing: " + p.status;
            if (p.percentComplete >= 0) {
                msg += " (" + std::to_string((int)p.percentComplete) + "%)";
            }
            updateStatus(msg);
        });

        if (!installer.installComfyUIBase(config)) {
            std::cerr << "[VideoGenRoom] Error: Failed to start ComfyUI installation: "
                      << installer.getLastError() << std::endl;
            return false;
        }

        // Wait for installation to complete
        while (installer.isInstalling()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        auto progress = installer.getProgress();
        if (progress.state != InstallProgress::State::COMPLETE) {
            std::cerr << "[VideoGenRoom] Error: ComfyUI installation failed: "
                      << progress.errorMessage << std::endl;
            return false;
        }

        updateStatus("ComfyUI installation complete");
    }

    updateStatus("Locating Python environment...");

    // Use the embedded Python from ComfyUI installation (has correct dependencies)
    std::string pythonPath = mainprogram->programData + "/EWOCvj2/ComfyUI/ComfyUI_windows_portable/python_embeded/python.exe";

    // Fall back to EWOCVJ2_PYTHON if embedded Python doesn't exist
    if (!std::filesystem::exists(pythonPath)) {
        const char* envPython = std::getenv("EWOCVJ2_PYTHON");
        if (!envPython || envPython[0] == '\0') {
            std::cerr << "[VideoGenRoom] Error: Embedded Python not found and EWOCVJ2_PYTHON not set" << std::endl;
            return false;
        }
        pythonPath = envPython;
        std::cerr << "[VideoGenRoom] Using Python from EWOCVJ2_PYTHON: " << pythonPath << std::endl;
    } else {
        std::cerr << "[VideoGenRoom] Using embedded Python: " << pythonPath << std::endl;
    }

    // ComfyUI paths to try (portable version structure)
    std::vector<std::string> comfyPaths = {
        mainprogram->programData + "/EWOCvj2/ComfyUI/ComfyUI_windows_portable/ComfyUI/main.py",
        mainprogram->programData + "/EWOCvj2/ComfyUI/main.py",
        mainprogram->programData + "/EWOCvj2/comfyui/ComfyUI/main.py"
    };

    std::string comfyMainPy = "";
    for (const auto& path : comfyPaths) {
        if (std::filesystem::exists(path)) {
            comfyMainPy = path;
            break;
        }
    }

    if (comfyMainPy.empty()) {
        std::cerr << "[VideoGenRoom] Error: ComfyUI main.py not found. Tried:" << std::endl;
        for (const auto& path : comfyPaths) {
            std::cerr << "  - " << path << std::endl;
        }
        return false;
    }

    std::cerr << "[VideoGenRoom] Found ComfyUI at: " << comfyMainPy << std::endl;

    // Get ComfyUI directory for working directory
    std::filesystem::path comfyDir = std::filesystem::path(comfyMainPy).parent_path();

    // Install Python requirements first
    updateStatus("Checking Python dependencies...");
    installComfyUIRequirements(comfyDir.string());

    updateStatus("Launching ComfyUI server...");

    // Build command to start ComfyUI server in background
    // --lowvram: keeps models on CPU, moves to GPU only when needed (saves VRAM)
    // --disable-smart-memory: prevents ComfyUI from trying to keep all models loaded
#ifdef _WIN32
    // On Windows, use CreateProcess with CREATE_NO_WINDOW to run without console
    // Use --output-directory to ensure ComfyUI outputs to our configured directory
    std::string outputDir = mainprogram->programData + "/EWOCvj2/comfyui/outputs";
    std::string cmd = "\"" + pythonPath + "\" -u \"" + comfyMainPy +
                      "\" --listen 127.0.0.1 --port 8188 --lowvram --output-directory \"" + outputDir + "\"";

    std::cerr << "[VideoGenRoom] Starting ComfyUI server: " << cmd << std::endl;

    // OLD: Used system() with start /B - shows console window briefly
    // std::string oldCmd = "start /B \"\" \"" + pythonPath + "\" \"" + comfyMainPy +
    //                      "\" --listen 127.0.0.1 --port 8188 --lowvram --output-directory \"" + outputDir + "\"";
    // std::string fullCmd = "cd /d \"" + comfyDir.string() + "\" && " + oldCmd;
    // system(fullCmd.c_str());

    // Launch hidden process and store handle for later termination
    if (!launchProcessHidden(cmd, comfyDir.string(), &comfyUIProcessHandle)) {
        std::cerr << "[VideoGenRoom] Error: Failed to launch ComfyUI process" << std::endl;
        return false;
    }
#else
    // On Linux/Mac, use nohup and & for background
    std::string outputDir = "/usr/share/EWOCvj2/comfyui/outputs";
    std::string cmd = "cd \"" + comfyDir.string() + "\" && nohup \"" + pythonPath +
                      "\" \"" + comfyMainPy + "\" --listen 127.0.0.1 --port 8188 --lowvram --output-directory \"" + outputDir + "\" > /dev/null 2>&1 &";

    std::cerr << "[VideoGenRoom] Starting ComfyUI server: " << cmd << std::endl;
    system(cmd.c_str());
#endif

    updateStatus("ComfyUI server launched, waiting for startup...");
    comfyUIServerStarted = true;

    // Give the server a moment to start
    // The actual connection retry loop will handle waiting for it
    return true;
}

VideoGenHistoryItem::VideoGenHistoryItem() {
    this->box = new Boxx;
    this->box->vtxcoords->w = 0.15f;
    this->box->vtxcoords->h = 0.10f;
    this->box->lcolor[0] = 0.5f;
    this->box->lcolor[1] = 0.5f;
    this->box->lcolor[2] = 0.5f;
    this->box->lcolor[3] = 1.0f;
}

VideoGenHistoryItem::~VideoGenHistoryItem() {
    if (this->box) delete this->box;
    if (this->tex != (GLuint)-1) glDeleteTextures(1, &this->tex);
    this->layer->close();
}

VideoGenRoom::VideoGenRoom() {
    // Create ComfyUI manager (initialization deferred to startGeneration)
    this->comfyManager = new ComfyUIManager();

    // Set up progress callback
    this->comfyManager->setProgressCallback([this](const GenerationProgress& p) {
        this->progressPercent = p.percentComplete;
        this->progressStatus = p.status;
        this->progressState = p.state;
        if (p.state == GenerationProgress::State::COMPLETE) {
            this->loadOutputToHistory(p.outputPath);
            // Reset to idle state after completion
            this->progressStatus = "Ready";
            this->progressState = GenerationProgress::State::IDLE;
        }
    });

    // Create menu
    this->videogenmenu = new Menu;

    // prompt box
    this->promptBox = new Boxx;
    this->promptBox->vtxcoords->x1 = 0.30f;
    this->promptBox->vtxcoords->y1 = -0.65f;
    this->promptBox->vtxcoords->w = 0.6f;
    this->promptBox->vtxcoords->h = 0.6f;
    this->promptBox->upvtxtoscr();

    // negative prompt box
    this->negpromptBox = new Boxx;
    this->negpromptBox->vtxcoords->x1 = 0.30f;
    this->negpromptBox->vtxcoords->y1 = -0.95f;
    this->negpromptBox->vtxcoords->w = 0.6f;
    this->negpromptBox->vtxcoords->h = 0.2f;
    this->negpromptBox->upvtxtoscr();

    // Initialize preview box (large, left side)
    this->previewBox = new Boxx;
    this->previewBox->vtxcoords->x1 = -0.80f;
    this->previewBox->vtxcoords->y1 = 0.05f;
    this->previewBox->vtxcoords->w = 0.6f;
    this->previewBox->vtxcoords->h = 0.5f;
    this->previewBox->upvtxtoscr();
    this->previewBox->lcolor[0] = 0.5f;
    this->previewBox->lcolor[1] = 0.5f;
    this->previewBox->lcolor[2] = 0.5f;
    this->previewBox->lcolor[3] = 1.0f;
    this->previewBox->tooltiptitle = "Video Preview ";
    this->previewBox->tooltip = "Shows the current or last generated video. ";

    // History container box
    this->historyBox = new Boxx;
    this->historyBox->vtxcoords->x1 = -0.80f;
    this->historyBox->vtxcoords->y1 = -0.25f;
    this->historyBox->vtxcoords->w = 0.6f;
    this->historyBox->vtxcoords->h = 0.15f;
    this->historyBox->upvtxtoscr();

    // History scroll buttons
    this->historyScrollLeft = new Boxx;
    this->historyScrollLeft->vtxcoords->x1 = -0.82f;
    this->historyScrollLeft->vtxcoords->y1 = -0.30f;
    this->historyScrollLeft->vtxcoords->w = 0.02f;
    this->historyScrollLeft->vtxcoords->h = 0.05f;
    this->historyScrollLeft->upvtxtoscr();

    this->historyScrollRight = new Boxx;
    this->historyScrollRight->vtxcoords->x1 = -0.20f;
    this->historyScrollRight->vtxcoords->y1 = -0.30f;
    this->historyScrollRight->vtxcoords->w = 0.02f;
    this->historyScrollRight->vtxcoords->h = 0.05f;
    this->historyScrollRight->upvtxtoscr();


    float presetsx = 0.35f;
    float presetsy = 0.1f;
    float presetsw = 0.3f;
    float presetsh = 0.7f;

    // Presets container box (right side)
    this->presetsBox = new Boxx;
    this->presetsBox->vtxcoords->x1 = presetsx;
    this->presetsBox->vtxcoords->y1 = presetsy;
    this->presetsBox->vtxcoords->w = presetsw;
    this->presetsBox->vtxcoords->h = presetsh;
    this->presetsBox->upvtxtoscr();

    // Presets scroll buttons
    this->presetsScrollUp = new Boxx;
    this->presetsScrollUp->vtxcoords->x1 = presetsx + 0.31f;
    this->presetsScrollUp->vtxcoords->y1 = presetsy + 1.40f;
    this->presetsScrollUp->vtxcoords->w = 0.025f;
    this->presetsScrollUp->vtxcoords->h = 0.05f;
    this->presetsScrollUp->upvtxtoscr();

    this->presetsScrollDown = new Boxx;
    this->presetsScrollDown->vtxcoords->x1 = 0.66f;
    this->presetsScrollDown->vtxcoords->y1 = presetsy + 0.1f;
    this->presetsScrollDown->vtxcoords->w = 0.025f;
    this->presetsScrollDown->vtxcoords->h = 0.05f;
    this->presetsScrollDown->upvtxtoscr();

    // Input image boxes (bottom left area)
    float inputBoxX = -0.80f;
    float inputBoxY = -0.58f;
    float inputBoxW = 0.15f;
    float inputBoxH = inputBoxW * glob->w * 9.0f / (glob->h * 16.0f);  // 16:9 in pixel space

    this->inputImageBox = new Boxx;
    this->inputImageBox->vtxcoords->x1 = inputBoxX + 0.22f;
    this->inputImageBox->vtxcoords->y1 = inputBoxY;
    this->inputImageBox->vtxcoords->w = inputBoxW;
    this->inputImageBox->vtxcoords->h = inputBoxH;
    this->inputImageBox->upvtxtoscr();
    this->inputImageBox->lcolor[0] = 0.4f;
    this->inputImageBox->lcolor[1] = 0.4f;
    this->inputImageBox->lcolor[2] = 0.6f;
    this->inputImageBox->lcolor[3] = 1.0f;
    this->inputImageBox->tooltiptitle = "Input Media ";
    this->inputImageBox->tooltip = "Drag an image or video here. Image for I2V presets, video for Remix. ";

    /*this->controlNetBox = new Boxx;
    this->controlNetBox->vtxcoords->x1 = inputBoxX + 0.22f;
    this->controlNetBox->vtxcoords->y1 = inputBoxY;
    this->controlNetBox->vtxcoords->w = inputBoxW;
    this->controlNetBox->vtxcoords->h = inputBoxH;
    this->controlNetBox->upvtxtoscr();
    this->controlNetBox->lcolor[0] = 0.6f;
    this->controlNetBox->lcolor[1] = 0.4f;
    this->controlNetBox->lcolor[2] = 0.4f;
    this->controlNetBox->lcolor[3] = 1.0f;
    this->controlNetBox->tooltiptitle = "ControlNet Image/Video ";
    this->controlNetBox->tooltip = "Drag a depth/edge/pose image or video here for ControlNet guidance. Video provides per-frame control. ";
*/
    /*this->styleImageBox = new Boxx;
    this->styleImageBox->vtxcoords->x1 = inputBoxX + 0.31f;
    this->styleImageBox->vtxcoords->y1 = inputBoxY;
    this->styleImageBox->vtxcoords->w = inputBoxW;
    this->styleImageBox->vtxcoords->h = inputBoxH;
    this->styleImageBox->upvtxtoscr();
    this->styleImageBox->lcolor[0] = 0.4f;
    this->styleImageBox->lcolor[1] = 0.6f;
    this->styleImageBox->lcolor[2] = 0.4f;
    this->styleImageBox->lcolor[3] = 1.0f;
    this->styleImageBox->tooltiptitle = "Style Image ";
    this->styleImageBox->tooltip = "Drag an image here for style transfer. ";
*/
    // Generate/Cancel buttons
    this->generateButton = new Boxx;
    this->generateButton->vtxcoords->x1 = inputBoxX;
    this->generateButton->vtxcoords->y1 = -0.80f;
    this->generateButton->vtxcoords->w = 0.15f;
    this->generateButton->vtxcoords->h = 0.10f;
    this->generateButton->upvtxtoscr();

    this->cancelButton = new Boxx;
    this->cancelButton->vtxcoords->x1 = inputBoxX + 0.17f;
    this->cancelButton->vtxcoords->y1 = -0.80f;
    this->cancelButton->vtxcoords->w = 0.12f;
    this->cancelButton->vtxcoords->h = 0.10f;
    this->cancelButton->upvtxtoscr();

    // Progress box
    this->progressBox = new Boxx;
    this->progressBox->vtxcoords->x1 = inputBoxX + 0.31f;
    this->progressBox->vtxcoords->y1 = -0.80f;
    this->progressBox->vtxcoords->w = 0.30f;
    this->progressBox->vtxcoords->h = 0.10f;
    this->progressBox->upvtxtoscr();

    // =====================
    // Create Parameters
    // =====================

    float paramx = -0.05f;
    float paramy = 0.6f;
    float paramw = 0.24f;
    float paramh = 0.1f;

    // Backend selection - options will be built dynamically based on installation status
    this->backendParam = new Param;
    this->backendParam->type = FF_TYPE_OPTION;
    this->backendParam->name = "Backend";
    // Options are populated by rebuildBackendOptions()
    this->backendParam->value = 0;
    this->backendParam->deflt = 0;
    this->backendParam->range[0] = 0;
    this->backendParam->range[1] = 1;  // Will be updated by rebuildBackendOptions()
    this->backendParam->sliding = false;
    this->backendParam->box->vtxcoords->x1 = paramx;
    this->backendParam->box->vtxcoords->y1 = paramy - paramh * 2.0f;
    this->backendParam->box->vtxcoords->w = paramw;
    this->backendParam->box->vtxcoords->h = 0.075f;
    this->backendParam->box->upvtxtoscr();
    this->backendParam->box->acolor[0] = 0.2f;
    this->backendParam->box->acolor[1] = 0.3f;
    this->backendParam->box->acolor[2] = 0.5f;
    this->backendParam->box->acolor[3] = 1.0f;
    this->backendParam->box->tooltiptitle = "Select generation backend ";
    this->backendParam->box->tooltip = "Only installed backends are shown. ";

    // Build backend options based on installation status
    rebuildBackendOptions();

    // Seed
    this->seed = new Param;
    this->seed->name = "Seed";
    this->seed->value = -1;
    this->seed->deflt = -1;
    this->seed->range[0] = -1;
    this->seed->range[1] = 999999;
    this->seed->sliding = false;
    this->seed->box->vtxcoords->x1 = paramx;
    this->seed->box->vtxcoords->y1 = paramy - paramh * 3;
    this->seed->box->vtxcoords->w = paramw * 0.5f;
    this->seed->box->vtxcoords->h = 0.075f;
    this->seed->box->upvtxtoscr();
    this->seed->box->acolor[0] = 0.2f;
    this->seed->box->acolor[1] = 0.2f;
    this->seed->box->acolor[2] = 0.4f;
    this->seed->box->acolor[3] = 1.0f;
    this->seed->box->tooltiptitle = "Random seed ";
    this->seed->box->tooltip = "-1 for random seed, or set a specific value for reproducibility. ";

    // Steps
    this->steps = new Param;
    this->steps->name = "Steps";
    this->steps->value = 20;
    this->steps->deflt = 20;
    this->steps->range[0] = 1;
    this->steps->range[1] = 50;
    this->steps->sliding = false;
    this->steps->box->vtxcoords->x1 = paramx + paramw * 0.55f;
    this->steps->box->vtxcoords->y1 = paramy - paramh * 3;
    this->steps->box->vtxcoords->w = paramw * 0.45f;
    this->steps->box->vtxcoords->h = 0.075f;
    this->steps->box->upvtxtoscr();
    this->steps->box->acolor[0] = 0.2f;
    this->steps->box->acolor[1] = 0.2f;
    this->steps->box->acolor[2] = 0.4f;
    this->steps->box->acolor[3] = 1.0f;
    this->steps->box->tooltiptitle = "Inference steps ";
    this->steps->box->tooltip = "More steps = higher quality but slower. ";

    // CFG Scale
    this->cfgScale = new Param;
    this->cfgScale->name = "Prompt adherence";
    this->cfgScale->value = 10.0f;
    this->cfgScale->deflt = 10.0f;
    this->cfgScale->range[0] = 1.0f;
    this->cfgScale->range[1] = 20.0f;
    this->cfgScale->sliding = true;
    this->cfgScale->box->vtxcoords->x1 = paramx;
    this->cfgScale->box->vtxcoords->y1 = paramy - paramh * 4;
    this->cfgScale->box->vtxcoords->w = paramw;
    this->cfgScale->box->vtxcoords->h = 0.075f;
    this->cfgScale->box->upvtxtoscr();
    this->cfgScale->box->acolor[0] = 0.2f;
    this->cfgScale->box->acolor[1] = 0.2f;
    this->cfgScale->box->acolor[2] = 0.4f;
    this->cfgScale->box->acolor[3] = 1.0f;
    this->cfgScale->box->tooltiptitle = "Prompt adherence ";
    this->cfgScale->box->tooltip = "Classifier-free guidance strength. Higher = more prompt adherence. ";

    // Prompt Improve (Flux only) - AI enhancement of prompts
    this->promptImprove = new Param;
    this->promptImprove->name = "Prompt improve";
    this->promptImprove->value = 0.0f;  // OFF by default
    this->promptImprove->deflt = 0.0f;
    this->promptImprove->range[0] = 0.0f;
    this->promptImprove->range[1] = 1.0f;
    this->promptImprove->sliding = false;
    this->promptImprove->box->vtxcoords->x1 = paramx;
    this->promptImprove->box->vtxcoords->y1 = paramy - paramh * 5;
    this->promptImprove->box->vtxcoords->w = paramw;
    this->promptImprove->box->vtxcoords->h = 0.075f;
    this->promptImprove->box->upvtxtoscr();
    this->promptImprove->box->acolor[0] = 0.3f;
    this->promptImprove->box->acolor[1] = 0.5f;
    this->promptImprove->box->acolor[2] = 0.3f;
    this->promptImprove->box->acolor[3] = 1.0f;
    this->promptImprove->box->tooltiptitle = "AI Prompt Enhancement ";
    this->promptImprove->box->tooltip = "Use AI to expand and improve your prompt for better image generation. ";

    // Frames (HunyuanVideo requires 1+4n: 5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65...129)
    this->frames = new Param;
    this->frames->name = "Frames";
    this->frames->value = 65;
    this->frames->deflt = 65;
    this->frames->range[0] = 5;
    this->frames->range[1] = 129;
    this->frames->sliding = false;
    this->frames->box->vtxcoords->x1 = paramx;
    this->frames->box->vtxcoords->y1 = paramy - paramh * 5;
    this->frames->box->vtxcoords->w = paramw * 0.5f;
    this->frames->box->vtxcoords->h = 0.075f;
    this->frames->box->upvtxtoscr();
    this->frames->box->acolor[0] = 0.4f;
    this->frames->box->acolor[1] = 0.2f;
    this->frames->box->acolor[2] = 0.2f;
    this->frames->box->acolor[3] = 1.0f;
    this->frames->box->tooltiptitle = "Number of frames ";
    this->frames->box->tooltip = "HunyuanVideo: use 1+4n (5,9,13,17,21,25...129). ";

    // Width
    this->width = new Param;
    this->width->name = "Width";
    this->width->value = 640;
    this->width->deflt = 640;
    this->width->range[0] = 128;
    this->width->range[1] = 1280;
    this->width->sliding = false;
    this->width->box->vtxcoords->x1 = paramx;
    this->width->box->vtxcoords->y1 = paramy - paramh * 6;
    this->width->box->vtxcoords->w = paramw * 0.5f;
    this->width->box->vtxcoords->h = 0.075f;
    this->width->box->upvtxtoscr();
    this->width->box->acolor[0] = 0.4f;
    this->width->box->acolor[1] = 0.2f;
    this->width->box->acolor[2] = 0.2f;
    this->width->box->acolor[3] = 1.0f;
    this->width->box->tooltiptitle = "Output width ";
    this->width->box->tooltip = "Width of generated video in pixels. ";

    // Height
    this->height = new Param;
    this->height->name = "Height";
    this->height->value = 368;
    this->height->deflt = 368;
    this->height->range[0] = 128;
    this->height->range[1] = 720;
    this->height->sliding = false;
    this->height->box->vtxcoords->x1 = paramx + paramw * 0.55f;
    this->height->box->vtxcoords->y1 = paramy - paramh * 6;
    this->height->box->vtxcoords->w = paramw * 0.45f;
    this->height->box->vtxcoords->h = 0.075f;
    this->height->box->upvtxtoscr();
    this->height->box->acolor[0] = 0.4f;
    this->height->box->acolor[1] = 0.2f;
    this->height->box->acolor[2] = 0.2f;
    this->height->box->acolor[3] = 1.0f;
    this->height->box->tooltiptitle = "Output height ";
    this->height->box->tooltip = "Height of generated video in pixels. ";

    // FPS
    this->fps = new Param;
    this->fps->name = "FPS";
    this->fps->value = 8.0f;
    this->fps->deflt = 8.0f;
    this->fps->range[0] = 1.0f;
    this->fps->range[1] = 30.0f;
    this->fps->sliding = true;
    this->fps->box->vtxcoords->x1 = paramx + paramw * 0.55f;
    this->fps->box->vtxcoords->y1 = paramy - paramh * 5;
    this->fps->box->vtxcoords->w = paramw * 0.45f;
    this->fps->box->vtxcoords->h = 0.075f;
    this->fps->box->upvtxtoscr();
    this->fps->box->acolor[0] = 0.4f;
    this->fps->box->acolor[1] = 0.2f;
    this->fps->box->acolor[2] = 0.2f;
    this->fps->box->acolor[3] = 1.0f;
    this->fps->box->tooltiptitle = "Frames per second ";
    this->fps->box->tooltip = "Output video frame rate. ";

    // Denoise strength (for I2V) - higher = more motion/creativity, lower = more faithful
    this->denoiseStrength = new Param;
    this->denoiseStrength->name = "Keep original";
    this->denoiseStrength->value = 1.0f;
    this->denoiseStrength->deflt = 1.0f;
    this->denoiseStrength->range[0] = 0.0f;
    this->denoiseStrength->range[1] = 1.0f;
    this->denoiseStrength->sliding = true;
    this->denoiseStrength->box->vtxcoords->x1 = paramx;
    this->denoiseStrength->box->vtxcoords->y1 = paramy - paramh * 11;
    this->denoiseStrength->box->vtxcoords->w = paramw;
    this->denoiseStrength->box->vtxcoords->h = 0.075f;
    this->denoiseStrength->box->upvtxtoscr();
    this->denoiseStrength->box->acolor[0] = 0.4f;
    this->denoiseStrength->box->acolor[1] = 0.3f;
    this->denoiseStrength->box->acolor[2] = 0.4f;
    this->denoiseStrength->box->acolor[3] = 1.0f;
    this->denoiseStrength->box->tooltiptitle = "Keep input image ";
    this->denoiseStrength->box->tooltip = "In how much do we adhere to the input image? Higher = more faithful to input, lower = more creative freedom. ";

    // Flux denoise strength (direct, not inverted) - higher = more creative
    this->fluxDenoiseStrength = new Param;
    this->fluxDenoiseStrength->name = "Keep original";
    this->fluxDenoiseStrength->value = 0.25f;
    this->fluxDenoiseStrength->deflt = 0.25f;
    this->fluxDenoiseStrength->range[0] = 0.0f;
    this->fluxDenoiseStrength->range[1] = 1.0f;
    this->fluxDenoiseStrength->sliding = true;
    this->fluxDenoiseStrength->box->vtxcoords->x1 = paramx;
    this->fluxDenoiseStrength->box->vtxcoords->y1 = paramy - paramh * 11;
    this->fluxDenoiseStrength->box->vtxcoords->w = paramw;
    this->fluxDenoiseStrength->box->vtxcoords->h = 0.075f;
    this->fluxDenoiseStrength->box->upvtxtoscr();
    this->fluxDenoiseStrength->box->acolor[0] = 0.3f;
    this->fluxDenoiseStrength->box->acolor[1] = 0.3f;
    this->fluxDenoiseStrength->box->acolor[2] = 0.5f;
    this->fluxDenoiseStrength->box->acolor[3] = 1.0f;
    this->fluxDenoiseStrength->box->tooltiptitle = "Keep input image ";
    this->fluxDenoiseStrength->box->tooltip = "In how much do we adhere to the input image? Higher = more faithful to input, lower = more creative freedom. ";

    // Remix strength
    this->remixStrength = new Param;
    this->remixStrength->name = "Remix";
    this->remixStrength->value = 0.5f;
    this->remixStrength->deflt = 0.5f;
    this->remixStrength->range[0] = 0.0f;
    this->remixStrength->range[1] = 1.0f;
    this->remixStrength->sliding = true;
    this->remixStrength->box->vtxcoords->x1 = paramx;
    this->remixStrength->box->vtxcoords->y1 = paramy - paramh * 11;
    this->remixStrength->box->vtxcoords->w = paramw;
    this->remixStrength->box->vtxcoords->h = 0.075f;
    this->remixStrength->box->upvtxtoscr();
    this->remixStrength->box->acolor[0] = 0.3f;
    this->remixStrength->box->acolor[1] = 0.4f;
    this->remixStrength->box->acolor[2] = 0.3f;
    this->remixStrength->box->acolor[3] = 1.0f;
    this->remixStrength->box->tooltiptitle = "Remix strength ";
    this->remixStrength->box->tooltip = "How much to transform from input. Lower = preserves input better (smooth continuation), higher = more creative freedom. ";

    // Style strength for IP2V / style transfer
    this->styleStrength = new Param;
    this->styleStrength->name = "Style";
    this->styleStrength->value = 0.8f;
    this->styleStrength->deflt = 0.8f;
    this->styleStrength->range[0] = 0.0f;
    this->styleStrength->range[1] = 1.0f;
    this->styleStrength->sliding = true;
    this->styleStrength->box->vtxcoords->x1 = paramx;
    this->styleStrength->box->vtxcoords->y1 = paramy - paramh * 10;  // Same row as remixStrength (they're exclusive)
    this->styleStrength->box->vtxcoords->w = paramw;
    this->styleStrength->box->vtxcoords->h = 0.075f;
    this->styleStrength->box->upvtxtoscr();
    this->styleStrength->box->acolor[0] = 0.4f;
    this->styleStrength->box->acolor[1] = 0.5f;
    this->styleStrength->box->acolor[2] = 0.3f;
    this->styleStrength->box->acolor[3] = 1.0f;
    this->styleStrength->box->tooltiptitle = "Style strength ";
    this->styleStrength->box->tooltip = "How strongly to apply the style image. Higher = more style influence, lower = more prompt influence. ";

    // Batch size for variation generator
    this->batchSize = new Param;
    this->batchSize->name = "Batch";
    this->batchSize->value = 4;
    this->batchSize->deflt = 4;
    this->batchSize->range[0] = 1;
    this->batchSize->range[1] = 8;
    this->batchSize->sliding = false;
    this->batchSize->box->vtxcoords->x1 = paramx;
    this->batchSize->box->vtxcoords->y1 = paramy - paramh * 12;
    this->batchSize->box->vtxcoords->w = paramw * 0.5f;
    this->batchSize->box->vtxcoords->h = 0.075f;
    this->batchSize->box->upvtxtoscr();
    this->batchSize->box->acolor[0] = 0.4f;
    this->batchSize->box->acolor[1] = 0.3f;
    this->batchSize->box->acolor[2] = 0.5f;
    this->batchSize->box->acolor[3] = 1.0f;
    this->batchSize->box->tooltiptitle = "Batch size ";
    this->batchSize->box->tooltip = "Number of variations to generate with different seeds. ";

    // Frame interpolation multiplier (2x, 4x, 8x)
    this->frameMultiplier = new Param;
    this->frameMultiplier->name = "Multiplier";
    this->frameMultiplier->value = 2.0f;
    this->frameMultiplier->deflt = 2.0f;
    this->frameMultiplier->range[0] = 2.0f;
    this->frameMultiplier->range[1] = 8.0f;
    this->frameMultiplier->sliding = false;
    this->frameMultiplier->box->vtxcoords->x1 = paramx;
    this->frameMultiplier->box->vtxcoords->y1 = paramy - paramh * 2;
    this->frameMultiplier->box->vtxcoords->w = paramw;
    this->frameMultiplier->box->vtxcoords->h = 0.075f;
    this->frameMultiplier->box->upvtxtoscr();
    this->frameMultiplier->box->acolor[0] = 0.2f;
    this->frameMultiplier->box->acolor[1] = 0.4f;
    this->frameMultiplier->box->acolor[2] = 0.4f;
    this->frameMultiplier->box->acolor[3] = 1.0f;
    this->frameMultiplier->box->tooltiptitle = "Frame multiplier ";
    this->frameMultiplier->box->tooltip = "Multiply frame count: 2x doubles FPS, 4x quadruples. Output FPS = input FPS x multiplier. ";

    // Append to source (for video continuation)
    this->appendToSource = new Param;
    this->appendToSource->type = FF_TYPE_OPTION;
    this->appendToSource->name = "Append";
    this->appendToSource->options.push_back("OFF");
    this->appendToSource->options.push_back("ON");
    this->appendToSource->value = 1;   // Default ON - append to source video
    this->appendToSource->deflt = 1;
    this->appendToSource->range[0] = 0;
    this->appendToSource->range[1] = 1;
    this->appendToSource->sliding = false;
    this->appendToSource->box->vtxcoords->x1 = paramx;
    this->appendToSource->box->vtxcoords->y1 = paramy - paramh * 13;
    this->appendToSource->box->vtxcoords->w = paramw * 0.5f;
    this->appendToSource->box->vtxcoords->h = 0.075f;
    this->appendToSource->box->upvtxtoscr();
    this->appendToSource->box->acolor[0] = 0.5f;
    this->appendToSource->box->acolor[1] = 0.3f;
    this->appendToSource->box->acolor[2] = 0.4f;
    this->appendToSource->box->acolor[3] = 1.0f;
    this->appendToSource->box->tooltiptitle = "Append to source ";
    this->appendToSource->box->tooltip = "ON: Append new frames to input video. OFF: Create separate clip. ";

    // HAP output encoding (for VJ-optimized playback)
    this->hapOutput = new Param;
    this->hapOutput->type = FF_TYPE_OPTION;
    this->hapOutput->name = "HAP Out";
    this->hapOutput->options.push_back("OFF");
    this->hapOutput->options.push_back("ON");
    this->hapOutput->value = 1;   // Default ON for VJ use
    this->hapOutput->deflt = 1;
    this->hapOutput->range[0] = 0;
    this->hapOutput->range[1] = 1;
    this->hapOutput->sliding = false;
    this->hapOutput->box->vtxcoords->x1 = paramx + paramw * 0.55f;
    this->hapOutput->box->vtxcoords->y1 = paramy - paramh * 6;  // Next to Height
    this->hapOutput->box->vtxcoords->w = paramw * 0.45f;
    this->hapOutput->box->vtxcoords->h = 0.075f;
    this->hapOutput->box->upvtxtoscr();
    this->hapOutput->box->acolor[0] = 0.4f;
    this->hapOutput->box->acolor[1] = 0.5f;
    this->hapOutput->box->acolor[2] = 0.2f;
    this->hapOutput->box->acolor[3] = 1.0f;
    this->hapOutput->box->tooltiptitle = "HAP output encoding ";
    this->hapOutput->box->tooltip = "Encode to HAP codec for VJ-optimized playback. GPU-accelerated, minimal quality loss. ";
}

VideoGenRoom::~VideoGenRoom() {
    // Wait for startup thread to finish
    if (this->startupThread && this->startupThread->joinable()) {
        this->startupThread->join();
    }

    if (this->comfyManager) delete this->comfyManager;
    if (this->previewBox) delete this->previewBox;
    if (this->historyBox) delete this->historyBox;
    if (this->historyScrollLeft) delete this->historyScrollLeft;
    if (this->historyScrollRight) delete this->historyScrollRight;
    if (this->presetsBox) delete this->presetsBox;
    if (this->presetsScrollUp) delete this->presetsScrollUp;
    if (this->presetsScrollDown) delete this->presetsScrollDown;
    if (this->inputImageBox) delete this->inputImageBox;
    if (this->controlNetBox) delete this->controlNetBox;
    if (this->styleImageBox) delete this->styleImageBox;
    if (this->generateButton) delete this->generateButton;
    if (this->cancelButton) delete this->cancelButton;
    if (this->progressBox) delete this->progressBox;
    if (this->videogenmenu) delete this->videogenmenu;

    // Delete params
    if (this->backendParam) delete this->backendParam;
    if (this->seed) delete this->seed;
    if (this->steps) delete this->steps;
    if (this->cfgScale) delete this->cfgScale;
    if (this->promptImprove) delete this->promptImprove;
    if (this->frames) delete this->frames;
    if (this->fps) delete this->fps;
    if (this->width) delete this->width;
    if (this->height) delete this->height;
    if (this->seamlessLoop) delete this->seamlessLoop;
    if (this->controlNetType) delete this->controlNetType;
    if (this->controlNetStrength) delete this->controlNetStrength;
    if (this->styleStrength) delete this->styleStrength;
    if (this->preserveColors) delete this->preserveColors;
    if (this->motionType) delete this->motionType;
    if (this->motionStrength) delete this->motionStrength;
    if (this->denoiseStrength) delete this->denoiseStrength;
    if (this->fluxDenoiseStrength) delete this->fluxDenoiseStrength;
    if (this->bpmMode) delete this->bpmMode;
    if (this->manualBpm) delete this->manualBpm;
    if (this->barLength) delete this->barLength;
    if (this->startTexture) delete this->startTexture;
    if (this->endTexture) delete this->endTexture;
    if (this->remixStrength) delete this->remixStrength;
    if (this->frameMultiplier) delete this->frameMultiplier;
    if (this->hapOutput) delete this->hapOutput;

    // Delete history items
    for (auto item : this->historyItems) {
        delete item;
    }
}

void VideoGenRoom::rebuildBackendOptions() {
    // Clear existing options
    this->backendParam->options.clear();
    this->backendOptionMapping.clear();

    // Add only installed backends
    if (this->hunyuaninstalled) {
        this->backendParam->options.push_back("HunyuanVideo");
        this->backendOptionMapping.push_back((int)GenerationBackend::HUNYUAN_SLIM);
    }
    /*if (this->hunyuanfullinstalled) {
        this->backendParam->options.push_back("Hunyuan Full");
        this->backendOptionMapping.push_back((int)GenerationBackend::HUNYUAN_FULL);
    }*/
    if (this->fluxinstalled) {
        this->backendParam->options.push_back("Flux Schnell");
        this->backendOptionMapping.push_back((int)GenerationBackend::FLUX_SCHNELL);
    }

    // If nothing is installed, show placeholder
    if (this->backendOptionMapping.empty()) {
        this->backendParam->options.push_back("(No backends installed)");
        this->backendOptionMapping.push_back((int)GenerationBackend::HUNYUAN_SLIM);  // Default
    }

    // Update range
    this->backendParam->range[1] = (float)this->backendParam->options.size();

    // Clamp current value to valid range
    if (this->backendParam->value >= this->backendParam->options.size()) {
        this->backendParam->value = 0;
    }
}

GenerationBackend VideoGenRoom::getSelectedBackend() {
    int optionIndex = (int)this->backendParam->value;
    if (optionIndex >= 0 && optionIndex < (int)this->backendOptionMapping.size()) {
        return (GenerationBackend)this->backendOptionMapping[optionIndex];
    }
    return GenerationBackend::HUNYUAN_SLIM;  // Default fallback
}

void VideoGenRoom::handle() {
    // Process any pending output from ComfyUI (must be done in main thread for OpenGL)
    processPendingOutput(this);

    // Update progress from ComfyUI manager
    this->updateProgress();

    float border = 0.05f;

    // Check if current preset needs prompts (frame interpolation doesn't)
    bool needsPrompt = (this->selectedPreset != PresetType::FRAME_INTERPOLATION);
    GenerationBackend currentBackendEnum = getSelectedBackend();
    bool isHunyuanBackend = (currentBackendEnum == GenerationBackend::HUNYUAN_SLIM || currentBackendEnum == GenerationBackend::HUNYUAN_FULL);

    // handle prompt editing (not for frame interpolation)
    if (needsPrompt) {
        if (mainprogram->renaming == EDIT_PROMPT) {
            // prompt renaming with keyboard
            this->promptlines = do_text_input_multiple_lines(this->promptBox->vtxcoords->x1 + 0.025f, this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h - 0.1f, 0.00072f, 0.00120f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(this->promptBox->vtxcoords->w - 0.05f), 0.05f, 10, 0, nullptr);
            this->promptstr = "";
            for (auto line : this->promptlines) {
                if (!this->promptstr.empty()) this->promptstr += " ";
                this->promptstr += line;
            }
        }
        else {
            int count = 0;
            for (auto line : this->promptlines) {
                if (count == 10) break;
                render_text(line, white, this->promptBox->vtxcoords->x1 + 0.025f, this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h - 0.1f - (0.05f * count), 0.00072f, 0.00120f);
                count++;
            }
        }

        // handle negative prompt editing (Hunyuan only - Flux doesn't use negative prompts)
        if (isHunyuanBackend) {
            if (mainprogram->renaming == EDIT_NEGPROMPT) {
                // prompt renaming with keyboard
                this->negpromptlines = do_text_input_multiple_lines(this->negpromptBox->vtxcoords->x1 + 0.025f, this->negpromptBox->vtxcoords->y1 + this->negpromptBox->vtxcoords->h - 0.1f, 0.00072f, 0.00120f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(this->promptBox->vtxcoords->w - 0.05f), 0.05f, 2, 0, nullptr);
                this->negpromptstr = "";
                for (auto line : this->negpromptlines) {
                    if (!this->negpromptstr.empty()) this->negpromptstr += " ";
                    this->negpromptstr += line;
                }
            }
            else {
                int count = 0;
                for (auto line : this->negpromptlines) {
                    if (count == 2) break;
                    render_text(line, white, this->negpromptBox->vtxcoords->x1 + 0.025f, this->negpromptBox->vtxcoords->y1 + this->negpromptBox->vtxcoords->h - 0.1f - (0.05f * count), 0.00072f, 0.00120f);
                    count++;
                }
            }
        }

        // =====================
        // Draw Prompt Area
        // =====================

        draw_box(white, darkgreen2, this->promptBox, -1);
        render_text("PROMPT", white, this->promptBox->vtxcoords->x1,
                    this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h + 0.01f,
                    0.0006f, 0.001f);
        if (this->promptBox->in()) {
            if (mainprogram->renaming == EDIT_NONE && mainprogram->leftmouse) {
                mainprogram->renaming = EDIT_PROMPT;
                this->oldpromptstr = this->promptstr;
                mainprogram->inputtext = this->promptstr;
                mainprogram->cursorpos0 = mainprogram->inputtext.length();
                SDL_StartTextInput();
                mainprogram->leftmouse = false;
                mainprogram->recundo = false;
            }
        }
        else {
            if (mainprogram->renaming == EDIT_PROMPT) {
                if (mainprogram->leftmouse) {
                    mainprogram->renaming = EDIT_NONE;
                    SDL_StopTextInput();
                    mainprogram->rightmouse = false;
                    mainprogram->menuactivation = false;
                }
            }
        }

        // Draw negative prompt box (Hunyuan only)
        if (isHunyuanBackend) {
            draw_box(white, darkgreen2, this->negpromptBox, -1);
            render_text("NEGATIVE PROMPT", white, this->negpromptBox->vtxcoords->x1,
                        this->negpromptBox->vtxcoords->y1 + this->negpromptBox->vtxcoords->h + 0.01f,
                        0.0006f, 0.001f);
            if (this->negpromptBox->in()) {
                if (mainprogram->renaming == EDIT_NONE && mainprogram->leftmouse) {
                    mainprogram->renaming = EDIT_NEGPROMPT;
                    this->negoldpromptstr = this->negpromptstr;
                    mainprogram->inputtext = this->negpromptstr;
                    mainprogram->cursorpos0 = mainprogram->inputtext.length();
                    SDL_StartTextInput();
                    mainprogram->leftmouse = false;
                    mainprogram->recundo = false;
                }
            }
            else {
                if (mainprogram->renaming == EDIT_NEGPROMPT) {
                    if (mainprogram->leftmouse) {
                        mainprogram->renaming = EDIT_NONE;
                        SDL_StopTextInput();
                        mainprogram->rightmouse = false;
                        mainprogram->menuactivation = false;
                    }
                }
            }
        }
    }

    // =====================
    // Draw Preview Area
    // =====================
    draw_box(white, darkgreen2, this->previewBox->vtxcoords->x1 - border,
             this->previewBox->vtxcoords->y1 - border,
             this->previewBox->vtxcoords->w + border * 2,
             this->previewBox->vtxcoords->h + border * 2, -1);

    GLuint tex = -1;
    int previewTexW = 0, previewTexH = 0;
    if (this->currentPreviewItem) {
        tex = this->currentPreviewItem->tex;
        if (this->currentPreviewItem->layer && this->currentPreviewItem->layer->decresult) {
            previewTexW = this->currentPreviewItem->layer->decresult->width;
            previewTexH = this->currentPreviewItem->layer->decresult->height;
        }
    }
    if (tex != (GLuint)-1 && previewTexW > 0 && previewTexH > 0) {
        draw_box_letterbox(this->previewBox, tex, previewTexW, previewTexH);
    } else if (tex != (GLuint)-1) {
        draw_box(this->previewBox, tex);  // Fallback if dimensions unknown
    } else {
        draw_box(this->previewBox, -1);
        render_text("No preview", grey, this->previewBox->vtxcoords->x1 + 0.2f,
                    this->previewBox->vtxcoords->y1 + 0.22f, 0.0006f, 0.001f);
    }
    render_text("PREVIEW", white, this->previewBox->vtxcoords->x1,
                this->previewBox->vtxcoords->y1 + this->previewBox->vtxcoords->h + 0.01f,
                0.0006f, 0.001f);

    // =====================
    // Draw History Videos
    // =====================
    draw_box(white, darkgreen2, this->historyBox->vtxcoords->x1 - border * 0.5f,
             this->historyBox->vtxcoords->y1 - border * 0.5f,
             this->historyBox->vtxcoords->w + border,
             this->historyBox->vtxcoords->h + border, -1);

    render_text("HISTORY", white, this->historyBox->vtxcoords->x1,
                this->historyBox->vtxcoords->y1 + this->historyBox->vtxcoords->h - 0.01f,
                0.00045f, 0.00075f);

    float thumbX = this->historyBox->vtxcoords->x1;
    float thumbY = this->historyBox->vtxcoords->y1;
    float thumbW = 0.12f;
    float thumbH = 0.10f;
    int maxVisible = 4;
    int startIdx = this->historyScroll;
    int endIdx = std::min(startIdx + maxVisible, (int)this->historyItems.size());

    for (int i = startIdx; i < endIdx; i++) {
        VideoGenHistoryItem* item = this->historyItems[i];
        item->box->vtxcoords->x1 = thumbX + (i - startIdx) * (thumbW + 0.02f);
        item->box->vtxcoords->y1 = thumbY;
        item->box->vtxcoords->w = thumbW;
        item->box->vtxcoords->h = thumbH;
        item->box->upvtxtoscr();

        if (!isimage(item->path) && (item->box->in() || item == this->currentPreviewItem)) {
             item->layer->progress(0, false, false);

            // Mark for new decode
            {
                std::lock_guard<std::mutex> lock(item->layer->decresult_mutex);
                item->layer->decresult->newdata = false;
            }

            // Trigger decode
            item->layer->ready = true;
            while (item->layer->ready) {
                item->layer->startdecode.notify_all();
            }

            // Wait for decode
            std::unique_lock<std::mutex> lock(item->layer->enddecodelock);
            item->layer->enddecodevar.wait(lock, [&] { return item->layer->processed; });
            item->layer->processed = false;
            lock.unlock();

            // Update texture
            glBindTexture(GL_TEXTURE_2D, item->tex);

            // HAP format uses compressed textures (DXT1/DXT5)
            int vidformat = item->layer->vidformat;
            int compression = item->layer->decresult->compression;
            if (vidformat == 188 || vidformat == 187) {
                if (compression == 187 || compression == 171) {
                    // DXT1 (HAP basic)
                    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                                           item->layer->decresult->width,
                                           item->layer->decresult->height, 0,
                                           item->layer->decresult->size,
                                           item->layer->decresult->data);
                } else if (compression == 190) {
                    // DXT5 (HAP Alpha / HAP Q)
                    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                           item->layer->decresult->width,
                                           item->layer->decresult->height, 0,
                                           item->layer->decresult->size,
                                           item->layer->decresult->data);
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                 item->layer->decresult->width,
                                 item->layer->decresult->height,
                                 0, GL_BGRA, GL_UNSIGNED_BYTE,
                                 item->layer->decresult->data);
                }
            } else {
                // Standard uncompressed texture
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                             item->layer->decresult->width,
                             item->layer->decresult->height,
                             0, GL_BGRA, GL_UNSIGNED_BYTE,
                             item->layer->decresult->data);
            }
        }

        if (item->tex != (GLuint)-1) {
            int texW = item->layer->decresult->width;
            int texH = item->layer->decresult->height;
            if (texW > 0 && texH > 0) {
                draw_box_letterbox(item->box, item->tex, texW, texH);
            } else {
                draw_box(item->box, item->tex);
            }
        } else {
            draw_box(item->box, -1);
        }

        if (item->box->in()) {
            if (mainprogram->leftmousedown) {
                mainprogram->dragbinel = new BinElement;
                mainprogram->dragbinel->type = ELEM_FILE;
                mainprogram->dragbinel->path = item->path;
                mainprogram->dragbinel->tex = item->tex;
                mainprogram->draglay = item->layer;
                this->dragging = true;
                mainprogram->leftmousedown = false;
            }
            if (mainprogram->lmover) {
                // Load to preview
                this->currentPreviewItem = item;
                this->promptstr = item->prompt;
                this->promptlines = item->promptlines;
                this->negpromptstr = item->negprompt;
                this->negpromptlines = item->negpromptlines;
            }
            if (mainprogram->menuactivation) {
                // Right-click menu
                this->menuitem = item;
                this->menuoptions.clear();
                std::vector<std::string> opts;
                opts.push_back("Export");
                this->menuoptions.push_back(VGEN_EXPORT);
                opts.push_back("Delete");
                this->menuoptions.push_back(VGEN_DELETE);
                mainprogram->make_menu("videogenmenu", this->videogenmenu, opts);
                this->videogenmenu->state = 2;
                this->videogenmenu->menux = mainprogram->mx;
                this->videogenmenu->menuy = mainprogram->my;
                mainprogram->menuactivation = false;
            }
        }
    }

    // History scroll buttons
    if (this->historyItems.size() > (size_t)maxVisible) {
        draw_box(this->historyScrollLeft, -1);
        render_text("<", white, this->historyScrollLeft->vtxcoords->x1 + 0.005f,
                    this->historyScrollLeft->vtxcoords->y1 + 0.015f, 0.0006f, 0.001f);
        if (this->historyScrollLeft->in() && mainprogram->leftmouse) {
            if (this->historyScroll > 0) this->historyScroll--;
            mainprogram->leftmouse = false;
        }

        draw_box(this->historyScrollRight, -1);
        render_text(">", white, this->historyScrollRight->vtxcoords->x1 + 0.005f,
                    this->historyScrollRight->vtxcoords->y1 + 0.015f, 0.0006f, 0.001f);
        if (this->historyScrollRight->in() && mainprogram->leftmouse) {
            if (this->historyScroll < (int)this->historyItems.size() - maxVisible)
                this->historyScroll++;
            mainprogram->leftmouse = false;
        }
    }

    // Handle menu
    int k = mainprogram->handle_menu(this->videogenmenu);
    if (k > -1) {
        if (this->menuoptions[k] == VGEN_EXPORT) {
            mainprogram->pathto = "EXPORTITEM";
            std::thread filereq(&Program::get_outname, mainprogram, "Export video", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
        else if (this->menuoptions[k] == VGEN_DELETE) {
            auto it = std::find(this->historyItems.begin(), this->historyItems.end(), this->menuitem);
            if (it != this->historyItems.end()) {
                delete *it;
                this->historyItems.erase(it);
            }
            this->menuitem = nullptr;
        }
        else if (this->menuoptions[k] == VGEN_CLEARIMAGE) {
            switch (this->menuboxnr) {
                case 0:
                    this->inputImagePath = "";
                    blacken(this->inputImageTex);
                    break;
                case 1:
                    this->controlNetImagePath = "";
                    blacken(this->controlNetImageTex);
                    break;
                case 2:
                    this->styleImagePath = "";
                    blacken(this->styleImageTex);
                    break;
            }
            this->menuboxnr = -1;
        }
        else if (this->menuoptions[k] == VGEN_BROWSEIMAGE) {
            mainprogram->pathto = "OPENVIDEOGENIMAGE";
            std::thread filereq(&Program::get_inname, mainprogram, "Open image", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
        else if (this->menuoptions[k] == VGEN_QUIT) {
            // quit program
            mainprogram->quitting = "quitted";
        }
    }

    // =====================
    // Draw Input Image Boxes
    // =====================
    render_text("INPUT", white, this->inputImageBox->vtxcoords->x1,
                this->inputImageBox->vtxcoords->y1 + this->inputImageBox->vtxcoords->h + 0.01f,
                0.00045f, 0.00075f);
    if (this->inputImageTex != (GLuint)-1) {
        draw_box(this->inputImageBox, (GLuint)-1);
        int texW = 1, texH = 1;
        glBindTexture(GL_TEXTURE_2D, this->inputImageTex);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
        float bx = this->inputImageBox->vtxcoords->x1;
        float by = this->inputImageBox->vtxcoords->y1;
        float bw = this->inputImageBox->vtxcoords->w;
        float bh = this->inputImageBox->vtxcoords->h;
        float screenAspect = glob->w / glob->h;
        float texAspect = (float)texW / (float)texH;
        float boxPixelAspect = (bw / bh) * screenAspect;
        float drawX, drawY, drawW, drawH;
        if (texAspect > boxPixelAspect) {
            drawW = bw;
            drawH = bw * screenAspect / texAspect;
            drawX = bx;
            drawY = by + (bh - drawH) / 2.0f;
        } else {
            drawH = bh;
            drawW = bh * texAspect / screenAspect;
            drawX = bx + (bw - drawW) / 2.0f;
            drawY = by;
        }
        draw_box(nullptr, black, drawX, drawY, drawW, drawH, this->inputImageTex);
    } else {
        draw_box(this->inputImageBox, this->inputImageTex);
    }
    if (this->inputImageBox->in()) {
        if (mainprogram->dropfiles.size()) {
            // SDL drag'n'drop
            for (char *df: mainprogram->dropfiles) {
                bool wrong = false;
                std::string path = df;
                if (isdeckfile(path)) {
                    wrong = true;
                }
                if (ismixfile(path)) {
                    wrong = true;
                }
                if (!wrong && (isvideo(path) || isimage(path))) {
                    this->inputImagePath = df;
                    this->loadFirstFramePreview(this->inputImagePath);
                    break;
                }
            }
        }

        if (mainprogram->lmover && (mainprogram->dragbinel || mainmix->moving)) {
            if (mainmix->moving)
            {
                this->inputImagePath = mainprogram->draglay->filename;
            }
            else
            {
                this->inputImagePath = mainprogram->dragbinel->path;
            }

            this->loadFirstFramePreview(this->inputImagePath);

            // If it's a video, detect FPS and update the fps parameter
            if (isvideo(this->inputImagePath)) {
                VideoUpscaler upscaler;
                auto videoInfo = upscaler.getVideoInfo(this->inputImagePath);
                if (videoInfo.fps > 0) {
                    this->inputVideoFps = videoInfo.fps;  // Store for frame interpolation
                    this->fps->value = videoInfo.fps;
                    this->fps->deflt = videoInfo.fps;
                }
            } else {
                this->inputVideoFps = 0.0f;  // Not a video
            }

            mainprogram->rightmouse = true;
            binsmain->handle(0);
            enddrag();
            mainprogram->rightmouse = false;
        }
        this->menuboxnr = 0;
        if (mainprogram->menuactivation) {
            std::vector<std::string> opts;
            this->menuoptions.clear();
            opts.push_back("Clear");
            this->menuoptions.push_back(VGEN_CLEARIMAGE);
            opts.push_back("Browse...");
            this->menuoptions.push_back(VGEN_BROWSEIMAGE);
            mainprogram->make_menu("videogenmenu", this->videogenmenu, opts);
            this->videogenmenu->state = 2;
            this->videogenmenu->menux = mainprogram->mx;
            this->videogenmenu->menuy = mainprogram->my;
            mainprogram->menuactivation = false;
        }
    }

    /*if (this->controlNetBox->in()) {
        this->menuboxnr = 1;
        if (mainprogram->menuactivation) {
            std::vector<std::string> opts;
            this->menuoptions.clear();
            opts.push_back("Clear");
            this->menuoptions.push_back(VGEN_CLEARIMAGE);
            opts.push_back("Browse...");
            this->menuoptions.push_back(VGEN_BROWSEIMAGE);
            mainprogram->make_menu("videogenmenu", this->videogenmenu, opts);
            this->videogenmenu->state = 2;
            this->videogenmenu->menux = mainprogram->mx;
            this->videogenmenu->menuy = mainprogram->my;
        }
    }*/

    /*if (this->styleImageBox->in()) {
        this->menuboxnr = 2;
        if (mainprogram->menuactivation) {
            std::vector<std::string> opts;
            this->menuoptions.clear();
            opts.push_back("Clear");
            this->menuoptions.push_back(VGEN_CLEARIMAGE);
            opts.push_back("Browse...");
            this->menuoptions.push_back(VGEN_BROWSEIMAGE);
            mainprogram->make_menu("videogenmenu", this->videogenmenu, opts);
            this->videogenmenu->state = 2;
            this->videogenmenu->menux = mainprogram->mx;
            this->videogenmenu->menuy = mainprogram->my;
        }
    }*/

    /*render_text("CNET", white, this->controlNetBox->vtxcoords->x1,
                this->controlNetBox->vtxcoords->y1 + this->controlNetBox->vtxcoords->h + 0.01f,
                0.00045f, 0.00075f);
    draw_box(this->controlNetBox, this->controlNetImageTex);
    if (this->controlNetBox->in()) {
        if (mainprogram->lmover && mainprogram->dragbinel) {
            this->controlNetImagePath = mainprogram->dragbinel->path;
            this->controlNetImageTex = copy_tex(mainprogram->dragbinel->tex);

            // Detect if it's a video (use !isimage since isvideo incorrectly matches images)
            this->controlNetIsVideo = !isimage(this->controlNetImagePath);

            mainprogram->rightmouse = true;
            binsmain->handle(0);
            enddrag();
            mainprogram->rightmouse = false;
        }
    }*/

    /*render_text("STYLE", white, this->styleImageBox->vtxcoords->x1,
                this->styleImageBox->vtxcoords->y1 + this->styleImageBox->vtxcoords->h + 0.01f,
                0.00045f, 0.00075f);
    draw_box(this->styleImageBox, this->styleImageTex);
    if (this->styleImageBox->in()) {
        if (mainprogram->lmover && mainprogram->dragbinel) {
            this->styleImagePath = mainprogram->dragbinel->path;
            this->styleImageTex = mainprogram->dragbinel->tex;
            mainprogram->rightmouse = true;
            binsmain->handle(0);
            enddrag();
            mainprogram->rightmouse = false;
        }
    }*/

    // =====================
    // Draw Presets Panel
    // =====================
    draw_box(white, darkgreen2, this->presetsBox->vtxcoords->x1 - border,
             this->presetsBox->vtxcoords->y1 - border,
             this->presetsBox->vtxcoords->w + border * 2,
             this->presetsBox->vtxcoords->h + border * 2, -1);

    render_text("PRESETS", white, this->presetsBox->vtxcoords->x1,
                this->presetsBox->vtxcoords->y1 + this->presetsBox->vtxcoords->h - 0.02f,
                0.0006f, 0.001f);

    // Get filtered presets
    std::vector<PresetInfo> filteredPresets = this->getFilteredPresets();

    // Draw preset list
    float presetY = this->presetsBox->vtxcoords->y1 + this->presetsBox->vtxcoords->h - 0.12f;
    float presetH = 0.08f;
    int maxPresets = 17;
    int presetStart = this->presetsScroll;
    int presetEnd = std::min(presetStart + maxPresets, (int)filteredPresets.size());

    for (int i = presetStart; i < presetEnd; i++) {
        PresetInfo& preset = filteredPresets[i];
        Boxx presetBox;
        presetBox.vtxcoords->x1 = this->presetsBox->vtxcoords->x1;
        presetBox.vtxcoords->y1 = presetY - (i - presetStart) * presetH;
        presetBox.vtxcoords->w = this->presetsBox->vtxcoords->w;
        presetBox.vtxcoords->h = presetH - 0.01f;
        presetBox.upvtxtoscr();

        // Highlight selected preset
        if ((int)preset.type == this->selectedPresetIndex) {
            presetBox.acolor[0] = 0.2f;
            presetBox.acolor[1] = 0.4f;
            presetBox.acolor[2] = 0.6f;
            presetBox.acolor[3] = 1.0f;
        } else {
            presetBox.acolor[0] = 0.15f;
            presetBox.acolor[1] = 0.15f;
            presetBox.acolor[2] = 0.15f;
            presetBox.acolor[3] = 1.0f;
        }

        draw_box(&presetBox, -1);
        render_text(preset.name, white, presetBox.vtxcoords->x1 + 0.01f,
                    presetBox.vtxcoords->y1 + 0.025f, 0.00045f, 0.00075f);

        if (presetBox.in()) {
            if (mainprogram->leftmouse) {
                this->selectPreset((int)preset.type);
                mainprogram->leftmouse = false;
            }
            // Show description tooltip
            presetBox.tooltiptitle = preset.name;
            presetBox.tooltip = preset.description;
        }
    }

    // Presets scroll
    if (filteredPresets.size() > (size_t)maxPresets) {
        draw_box(this->presetsScrollUp, -1);
        render_text("^", white, this->presetsScrollUp->vtxcoords->x1 + 0.005f,
                    this->presetsScrollUp->vtxcoords->y1 + 0.015f, 0.0006f, 0.001f);
        if (this->presetsScrollUp->in() && mainprogram->leftmouse) {
            if (this->presetsScroll > 0) this->presetsScroll--;
            mainprogram->leftmouse = false;
        }

        draw_box(this->presetsScrollDown, -1);
        render_text("v", white, this->presetsScrollDown->vtxcoords->x1 + 0.005f,
                    this->presetsScrollDown->vtxcoords->y1 + 0.015f, 0.0006f, 0.001f);
        if (this->presetsScrollDown->in() && mainprogram->leftmouse) {
            if (this->presetsScroll < (int)filteredPresets.size() - maxPresets)
                this->presetsScroll++;
            mainprogram->leftmouse = false;
        }
    }

    // =====================
    // Draw Parameters Panel
    // =====================
    draw_box(white, darkgreen2, -0.07f, -0.65f, 0.28f, 1.10f, -1);
    render_text("PARAMETERS", white, -0.0f, 0.52f, 0.0006f, 0.001f);

    // Get current preset info to determine which params to show
    const PresetInfo& presetInfo = ComfyUIManager::getPresetInfo(this->selectedPreset);

    // Always show: backend selection
    this->backendParam->handle();

    // Update frames range for HunyuanVideo
    this->frames->range[1] = 129;  // HunyuanVideo max

    // Check backend type early for UI decisions
    GenerationBackend currentBackend = getSelectedBackend();
    bool isFluxBackend = (currentBackend == GenerationBackend::FLUX_SCHNELL);
    isHunyuanBackend = (currentBackend == GenerationBackend::HUNYUAN_SLIM || currentBackend == GenerationBackend::HUNYUAN_FULL);

    // Reset preset to first valid one when backend changes
    if (!this->lastBackendInitialized || this->lastBackend != currentBackend) {
        // Save current dimensions and steps for the old backend (only if initialized)
        if (this->lastBackendInitialized) {
            if (this->lastBackend == GenerationBackend::HUNYUAN_SLIM ||
                this->lastBackend == GenerationBackend::HUNYUAN_FULL) {
                // Was Hunyuan (Slim or Full)
                this->savedHunyuanWidth = (int)this->width->value;
                this->savedHunyuanHeight = (int)this->height->value;
                this->savedHunyuanSteps = (int)this->steps->value;
            } else if (this->lastBackend == GenerationBackend::FLUX_SCHNELL) {
                // Was Flux
                this->savedFluxWidth = (int)this->width->value;
                this->savedFluxHeight = (int)this->height->value;
                this->savedFluxSteps = (int)this->steps->value;
            }
        }

        // Restore dimensions and steps for the new backend
        if (isHunyuanBackend) {
            // Switching to Hunyuan (Slim or Full)
            this->width->value = (float)this->savedHunyuanWidth;
            this->height->value = (float)this->savedHunyuanHeight;
            this->steps->value = (float)this->savedHunyuanSteps;
        } else if (isFluxBackend) {
            // Switching to Flux
            this->width->value = (float)this->savedFluxWidth;
            this->height->value = (float)this->savedFluxHeight;
            this->steps->value = (float)this->savedFluxSteps;
        }

        this->lastBackend = currentBackend;
        this->lastBackendInitialized = true;
        std::vector<PresetInfo> validPresets = this->getFilteredPresets();
        if (!validPresets.empty()) {
            this->selectPreset((int)validPresets[0].type);
        }
    }

    // Update dimension ranges based on backend
    if (isFluxBackend) {
        // Flux supports up to 2176x1448 (or 1448x2176)
        this->width->range[1] = 2176;
        this->height->range[1] = 1448;
    } else {
        // HunyuanVideo: 1280x720
        this->width->range[1] = 1280;
        this->height->range[1] = 720;
    }

    // Core generation params (always shown)
    // Frame interpolation doesn't need any diffusion params - just multiplier
    if (this->selectedPreset != PresetType::FRAME_INTERPOLATION) {
        this->seed->handle();
        this->steps->handle();

        // CFG Scale - Hunyuan only (Flux doesn't use CFG)
        if (!isFluxBackend) {
            this->cfgScale->handle();
        }

        // Prompt improve - Flux only
        if (isFluxBackend) {
            this->promptImprove->handle();
        }
    }

    // Video dimension params (not for frame interpolation, video continuation - those are fixed/from input)

    if (this->selectedPreset == PresetType::FRAME_INTERPOLATION) {
        // Frame interpolation only needs multiplier
        this->frameMultiplier->handle();
    } else if (this->selectedPreset == PresetType::VIDEO_CONTINUATION) {
        // Video continuation: frames only, width/height come from input video
        this->frames->handle();
    } else if (isFluxBackend) {
        // Flux generates single images - no frames param, only width/height
        this->width->handle();
        this->height->handle();
    } else {
        this->frames->handle();
        this->width->handle();
        this->height->handle();
    }

    // Remix strength - for remix preset (direct denoise value)
    if (this->selectedPreset == PresetType::REMIX_EXISTING_CLIP) {
        this->remixStrength->handle();
    }

    // Style strength - shown when style image is provided
    // Not applicable for frame interpolation (RIFE doesn't use style)
    // Not applicable for HunyuanVideo backends (native 1.5 doesn't support separate style images)
    if (!this->styleImagePath.empty() &&
        this->selectedPreset != PresetType::FRAME_INTERPOLATION &&
        !isHunyuanBackend) {
        this->styleStrength->handle();
    }

    // Denoise strength - for image-to-motion and video continuation (inverted in workflow)
    if (this->selectedPreset == PresetType::IMAGE_TO_MOTION ||
        this->selectedPreset == PresetType::VIDEO_CONTINUATION) {
        this->denoiseStrength->handle();
    }

    // Flux denoise strength - for Flux image-to-image (direct, not inverted)
    if (this->selectedPreset == PresetType::IMAGE_TO_IMAGE) {
        this->fluxDenoiseStrength->handle();
    }

    // Batch size - for BATCH_VARIATION_GENERATOR presets
    if (this->selectedPreset == PresetType::BATCH_VARIATION_GENERATOR_T2V ||
        this->selectedPreset == PresetType::BATCH_VARIATION_GENERATOR_I2V) {
        this->batchSize->handle();
    }

    // Append to source - for VIDEO_CONTINUATION preset
    if (this->selectedPreset == PresetType::VIDEO_CONTINUATION) {
        this->appendToSource->handle();
    }

    // =====================
    // Draw Generate/Cancel Buttons
    // =====================
    bool isGenerating = this->comfyManager->isGenerating() || this->startupInProgress.load();

    if (isGenerating) {
        // Show Cancel button
        draw_box(white, darkred1, this->cancelButton, -1);
        render_text("CANCEL", white, this->cancelButton->vtxcoords->x1 + 0.02f,
                    this->cancelButton->vtxcoords->y1 + 0.035f, 0.0007f, 0.0012f);
        if (this->cancelButton->in() && mainprogram->leftmouse) {
            this->cancelGeneration();
            mainprogram->leftmouse = false;
        }
    } else {
        // Show Generate button
        draw_box(white, darkgreen1, this->generateButton, -1);
        render_text("GENERATE", white, this->generateButton->vtxcoords->x1 + 0.015f,
                    this->generateButton->vtxcoords->y1 + 0.035f, 0.0007f, 0.0012f);
        if (this->generateButton->in() && mainprogram->leftmouse) {
            this->startGeneration();
            mainprogram->leftmouse = false;
        }
    }

    // Always show status box with current status
    if (this->progressState == GenerationProgress::State::FAILED) {
        draw_box(white, darkred1, this->progressBox, -1);
    } else if (isGenerating) {
        draw_box(white, black, this->progressBox, -1);
        // Progress bar fill
        float fillWidth = this->progressBox->vtxcoords->w * (this->progressPercent / 100.0f);
        draw_box(nullptr, darkgreen1, this->progressBox->vtxcoords->x1,
                 this->progressBox->vtxcoords->y1,
                 fillWidth, this->progressBox->vtxcoords->h, -1);
    } else {
        draw_box(white, black, this->progressBox, -1);
    }
    render_text(this->progressStatus, white, this->progressBox->vtxcoords->x1 + 0.01f,
                this->progressBox->vtxcoords->y1 + 0.035f, 0.00045f, 0.00075f);
    // Quit right-click menu
    if (mainprogram->menuactivation) {
        // Right-click menu
        this->menuoptions.clear();
        std::vector<std::string> opts;
        opts.push_back("Quit");
        this->menuoptions.push_back(VGEN_QUIT);
        mainprogram->make_menu("segmenu", this->videogenmenu, opts);
        this->videogenmenu->state = 2;
        this->videogenmenu->menux = mainprogram->mx;
        this->videogenmenu->menuy = mainprogram->my;
        mainprogram->menuactivation = false;
    }
}

void VideoGenRoom::startGeneration() {
    // Don't start if already in progress
    if (this->startupInProgress.load() || this->comfyManager->isGenerating()) {
        return;
    }

    // Build params and do basic validation synchronously (immediate feedback)
    GenerationParams params = this->buildGenerationParams();

    // Validate required inputs for preset
    const PresetInfo& presetInfo = ComfyUIManager::getPresetInfo(params.preset);
    if (presetInfo.requiresControlNet) {
        // HunyuanVideo uses CLIP Vision - needs input image
        if (params.inputImagePath.empty()) {
            this->progressStatus = "This preset requires an input image";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
    }

    // Validate style image for style transfer presets
    if (params.preset == PresetType::STYLE_TRANSFER_LOOP) {
        if (params.styleImagePath.empty()) {
            this->progressStatus = "This preset requires a style image";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
    }

    // Validate input media type matches preset requirements
    if (presetInfo.requiresImage) {
        if (params.inputImagePath.empty()) {
            this->progressStatus = "This preset requires an input image";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
        if (!isimage(params.inputImagePath)) {
            this->progressStatus = "This preset requires an image, not a video";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
    }
    if (presetInfo.requiresVideo) {
        if (params.inputImagePath.empty()) {
            this->progressStatus = "This preset requires an input video file";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
        if (isimage(params.inputImagePath)) {
            this->progressStatus = "This preset requires a video, not an image";
            this->progressState = GenerationProgress::State::FAILED;
            return;
        }
    }

    // Reset state for startup
    this->progressState = GenerationProgress::State::CONNECTING;
    this->progressStatus = "Initializing...";

    // Clean up previous thread if any
    if (this->startupThread && this->startupThread->joinable()) {
        this->startupThread->join();
    }

    // Start background thread for server launch and generation
    this->startupInProgress.store(true);
    this->startupThread = std::make_unique<std::thread>(&VideoGenRoom::startupThreadFunc, this);
}

// VIDEO_CONTINUATION state for frame concatenation (declared here for startupThreadFunc access)
static std::string continuationSourceVideo = "";
static bool continuationAppendToSource = false;

void VideoGenRoom::startupThreadFunc() {
    // Initialize ComfyUI manager if needed
    if (!this->comfyManager->isInitialized()) {
        this->progressStatus = "Initializing ComfyUI manager...";

        ComfyUIConfig config;
#ifdef _WIN32
        config.workflowsDir = mainprogram->programData + "/EWOCvj2/comfyui/workflows";
        config.outputDir = mainprogram->programData + "/EWOCvj2/comfyui/outputs";
        config.inputDir = mainprogram->programData + "/EWOCvj2/comfyui/inputs";
#else
        config.workflowsDir = "/usr/share/EWOCvj2/comfyui/workflows";
        config.outputDir = "/usr/share/EWOCvj2/comfyui/outputs";
        config.inputDir = "/usr/share/EWOCvj2/comfyui/inputs";
#endif
        // Create directories if they don't exist
        std::filesystem::create_directories(config.workflowsDir);
        std::filesystem::create_directories(config.outputDir);
        std::filesystem::create_directories(config.inputDir);

        if (!this->comfyManager->initialize(config)) {
            this->progressStatus = "Failed to initialize ComfyUI manager";
            this->progressState = GenerationProgress::State::FAILED;
            this->startupInProgress.store(false);
            return;
        }
    }

    // Start ComfyUI server if not already running
    if (!comfyUIServerStarted) {
        // Pass status callback to show launch progress in UI
        if (!startComfyUIServer([this](const std::string& status) {
            this->progressStatus = status;
        })) {
            this->progressStatus = "Failed to start ComfyUI server";
            this->progressState = GenerationProgress::State::FAILED;
            this->startupInProgress.store(false);
            return;
        }
    }

    // Try to connect to ComfyUI server
    if (!this->comfyManager->isConnected()) {
        this->progressStatus = "Connecting to ComfyUI server...";

        // Retry connection a few times with delay (server may still be starting)
        bool connected = false;
        for (int retry = 0; retry < 60; retry++) {  // Try for up to 60 seconds (first start can take longer)
            if (this->comfyManager->connect()) {
                connected = true;
                this->progressStatus = "Connected to ComfyUI";
                if (retry > 0) {
                    std::cerr << "[VideoGenRoom] Connected to ComfyUI after " << (retry + 1) << " seconds" << std::endl;
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->progressStatus = "Waiting for ComfyUI server... (" + std::to_string(retry + 1) + "s)";
        }

        if (!connected) {
            this->progressStatus = "ComfyUI server not responding after 60s";
            this->progressState = GenerationProgress::State::FAILED;
            this->startupInProgress.store(false);
            return;
        }
    }

    // Build params fresh (UI state captured at this point)
    this->progressStatus = "Starting generation...";
    GenerationParams params = this->buildGenerationParams();

    // Handle video preset input path
    const PresetInfo& presetInfo = ComfyUIManager::getPresetInfo(params.preset);
    if (presetInfo.requiresVideo) {
        params.inputVideoPath = params.inputImagePath;
    }

    // VIDEO_CONTINUATION: Set up state for frame concatenation after generation
    if (params.preset == PresetType::VIDEO_CONTINUATION && params.appendToSource) {
        continuationSourceVideo = params.sourceVideoPath;
        continuationAppendToSource = true;
        std::cerr << "[VideoGenRoom] VIDEO_CONTINUATION: Will append to source video: "
                  << continuationSourceVideo << std::endl;
    } else {
        continuationSourceVideo = "";
        continuationAppendToSource = false;
    }

    // Start the actual generation (this spawns ComfyUIManager's own thread)
    if (!this->comfyManager->startGeneration(params)) {
        this->progressStatus = "Failed to start generation";
        this->progressState = GenerationProgress::State::FAILED;
    }

    this->startupInProgress.store(false);
}

void VideoGenRoom::cancelGeneration() {
    this->comfyManager->cancelGeneration();
}

void VideoGenRoom::updateProgress() {
    if (this->comfyManager->isGenerating()) {
        GenerationProgress progress = this->comfyManager->getProgress();
        this->progressPercent = progress.percentComplete;
        this->progressStatus = progress.status;
        this->progressState = progress.state;
    }
}

// Pending output path from callback (for main thread to process)
static std::string pendingOutputPath = "";
static bool hasPendingOutput = false;

void VideoGenRoom::loadOutputToHistory(const std::string& path) {
    std::cerr << "[VideoGenRoom] loadOutputToHistory queued: " << path << std::endl;

    // Just store the path - will be processed in main thread's handle()
    // This is called from the ComfyUI callback thread, so we can't do OpenGL here
    pendingOutputPath = path;
    hasPendingOutput = true;
}

// Called from handle() in main thread to process pending output
static void processPendingOutput(VideoGenRoom* room) {
    if (!hasPendingOutput) return;
    hasPendingOutput = false;

    std::string path = pendingOutputPath;
    std::cerr << "[VideoGenRoom] Processing pending output: " << path << std::endl;

    // Check if path exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "[VideoGenRoom] ERROR: Output path does not exist: " << path << std::endl;
        return;
    }

    // Check if this is a frame directory or a video file
    bool isFrameDirectory = std::filesystem::is_directory(path);

    if (isFrameDirectory) {
        // Count frames
        int frameCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) frameCount++;
        }
        std::cerr << "[VideoGenRoom] Frame directory found with " << frameCount << " frames" << std::endl;
    } else {
        std::cerr << "[VideoGenRoom] File exists, size: "
                  << std::filesystem::file_size(path) << " bytes" << std::endl;
    }

    // VIDEO_CONTINUATION: Combine source + generated frames
    // Optimized path: if source is already HAP, use packet passthrough (no re-encoding of source)
    if (isFrameDirectory && continuationAppendToSource && !continuationSourceVideo.empty()) {
        std::cerr << "[VideoGenRoom] VIDEO_CONTINUATION: Combining source + generated frames" << std::endl;
        std::cerr << "[VideoGenRoom] Source video: " << continuationSourceVideo << std::endl;

        bool sourceIsHAP = isHAPVideo(continuationSourceVideo);
        // HunyuanVideo outputs at 24fps
        float fps = 24.0f;

        if (sourceIsHAP && room->hapOutput && room->hapOutput->value > 0.5f) {
            // OPTIMIZED PATH: Source is HAP, output is HAP - use packet passthrough!
            std::cerr << "[VideoGenRoom] Source is HAP - using optimized passthrough (no re-encode)" << std::endl;

            std::string hapPath = path + "_continued.mov";
            if (hapContinuationPassthrough(continuationSourceVideo, path, hapPath, fps)) {
                path = hapPath;
                std::cerr << "[VideoGenRoom] HAP passthrough complete" << std::endl;

                // Clear continuation state and skip normal HAP encoding
                continuationSourceVideo = "";
                continuationAppendToSource = false;

                // Skip to history item creation (HAP encoding already done)
                goto skip_encoding;
            } else {
                std::cerr << "[VideoGenRoom] HAP passthrough failed, falling back to full re-encode" << std::endl;
            }
        }

        // STANDARD PATH: Extract source frames and combine (source is not HAP or passthrough failed)
        std::string combinedDir = path + "_combined";
        std::filesystem::create_directories(combinedDir);

        int sourceFrameCount = extractVideoFrames(continuationSourceVideo, combinedDir, 1);
        if (sourceFrameCount > 0) {
            std::cerr << "[VideoGenRoom] Appending generated frames after " << sourceFrameCount << " source frames" << std::endl;
            renumberFrames(path, combinedDir, sourceFrameCount + 1);
            path = combinedDir;
            std::cerr << "[VideoGenRoom] Combined frames directory: " << path << std::endl;
        } else {
            std::cerr << "[VideoGenRoom] Failed to extract source frames, using generated only" << std::endl;
        }

        // Clear continuation state
        continuationSourceVideo = "";
        continuationAppendToSource = false;
    }

    // HAP encode if enabled
    if (room->hapOutput && room->hapOutput->value > 0.5f) {
        if (isFrameDirectory) {
            // Encode frames directly to HAP (lossless path)
            std::string hapPath = path + ".mov";  // hunyuan_t2v/123456789.mov
            std::cerr << "[VideoGenRoom] HAP encoding frames from: " << path << std::endl;
            std::cerr << "[VideoGenRoom] HAP output: " << hapPath << std::endl;

            // HunyuanVideo outputs at 24fps
            float fps = 24.0f;
            if (hapEncodeFrames(path, hapPath, fps, nullptr)) {
                // Success - use HAP version, update path
                path = hapPath;
                std::cerr << "[VideoGenRoom] HAP encoding complete" << std::endl;
            } else {
                std::cerr << "[VideoGenRoom] HAP encoding failed" << std::endl;
                // Fall back to using a frame as preview
            }
        } else {
            // Re-encode existing video to HAP
            std::string ext = path.substr(path.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // Only encode video files (not already HAP)
            if (ext == "mp4" || ext == "webm" || ext == "avi" || ext == "mkv") {
                std::string hapPath = path.substr(0, path.find_last_of(".")) + "_hap.mov";
                std::cerr << "[VideoGenRoom] HAP re-encoding video to: " << hapPath << std::endl;

                if (hapEncodeVideo(path, hapPath, nullptr)) {
                    // Success - use HAP version
                    path = hapPath;
                    std::cerr << "[VideoGenRoom] HAP encoding complete" << std::endl;
                } else {
                    std::cerr << "[VideoGenRoom] HAP encoding failed, using original" << std::endl;
                }
            }
        }
    } else if (isFrameDirectory) {
        // HAP disabled - encode frames to H.264
        std::string mp4Path = path + ".mp4";
        std::cerr << "[VideoGenRoom] HAP disabled, encoding frames to H.264: " << mp4Path << std::endl;

        float fps = room->fps ? room->fps->value : 24.0f;
        if (h264EncodeFrames(path, mp4Path, fps, nullptr)) {
            path = mp4Path;
            std::cerr << "[VideoGenRoom] H.264 encoding complete" << std::endl;
        } else {
            std::cerr << "[VideoGenRoom] H.264 encoding failed" << std::endl;
        }
    }

skip_encoding:
    VideoGenHistoryItem* item = new VideoGenHistoryItem();
    item->path = path;
    item->name = basename(path);
    item->preset = room->selectedPreset;
    item->prompt = mainvideogenroom->promptstr;
    item->promptlines = mainvideogenroom->promptlines;
    item->negprompt = mainvideogenroom->negpromptstr;
    item->negpromptlines = mainvideogenroom->negpromptlines;
    item->tex = -1;

    room->historyItems.insert(room->historyItems.begin(), item);

    // Keep history limited
    while (room->historyItems.size() > 20) {
        delete room->historyItems.back();
        room->historyItems.pop_back();
    }

    // Set as preview path
    room->currentPreviewItem = item;

    // Check if video file
    room->previewIsVideo = !isimage(path);

    if (room->previewIsVideo) {
        std::cerr << "[VideoGenRoom] Opening video with Layer system..." << std::endl;

        // Create new Layer for history video playback
        item->layer = new Layer(true);
        item->layer->dummy = true;
        item->layer->transfered = true;
        item->layer->open_video(0, path, true);

        // Wait for video to open
        std::unique_lock<std::mutex> olock2(item->layer->endopenlock);
        item->layer->endopenvar.wait(olock2, [&] { return item->layer->opened; });
        item->layer->opened = false;
        olock2.unlock();

        item->layer->frame = 0.0f;
        item->layer->prevframe = -1;

        // Start decode for first frame
        item->layer->ready = true;
        while (item->layer->ready) {
            item->layer->startdecode.notify_all();
        }

        // Wait for decode
        std::unique_lock<std::mutex> lock3(item->layer->enddecodelock);
        item->layer->enddecodevar.wait(lock3, [&] { return item->layer->processed; });
        item->layer->processed = false;
        lock3.unlock();

        // Check decode result is valid
        if (!item->layer->decresult || !item->layer->decresult->data) {
            std::cerr << "[VideoGenRoom] ERROR: Decode result is null" << std::endl;
            delete item;
            return;
        }

        int w = item->layer->decresult->width;
        int h = item->layer->decresult->height;
        std::cerr << "[VideoGenRoom] Video decoded: " << w << "x" << h << std::endl;

        // Initialize layer texture
        item->layer->initialize(w, h);

        // Create preview texture from first frame
        if (item->tex == (GLuint)-1) {
            glGenTextures(1, &item->tex);
        }
        if (item->tex == 0 || item->tex == (GLuint)-1) {
            std::cerr << "[VideoGenRoom] ERROR: Failed to generate texture" << std::endl;
            delete item;
            return;
        }
        glBindTexture(GL_TEXTURE_2D, item->tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // HAP format uses compressed textures (DXT1/DXT5)
        int vidformat = item->layer->vidformat;
        int compression = item->layer->decresult->compression;
        if (vidformat == 188 || vidformat == 187) {
            // HAP compressed texture upload
            if (compression == 187) {
                // DXT1 (HAP basic)
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                                       w, h, 0, item->layer->decresult->size,
                                       item->layer->decresult->data);
            } else if (compression == 190) {
                // DXT5 (HAP Alpha / HAP Q)
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                       w, h, 0, item->layer->decresult->size,
                                       item->layer->decresult->data);
            } else {
                std::cerr << "[VideoGenRoom] Unknown HAP compression: " << compression << std::endl;
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                             item->layer->decresult->data);
            }
        } else {
            // Standard uncompressed texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                         item->layer->decresult->data);
        }
    } else {
        // Try to load as image with FFmpeg
        std::cerr << "[VideoGenRoom] Loading as image..." << std::endl;

        int w, h;
        auto imgData = ImageLoader::loadImageRGBA(path, &w, &h);

        if (!imgData.empty()) {
            item->layer = new Layer(true);
            item->layer->dummy = true;
            item->layer->decresult->width = w;
            item->layer->decresult->height = h;

            if (item->tex == (GLuint)-1) {
                glGenTextures(1, &item->tex);
            }
            glBindTexture(GL_TEXTURE_2D, item->tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData.data());

            std::cerr << "[VideoGenRoom] Image loaded: " << w << "x" << h << std::endl;
        } else {
            std::cerr << "[VideoGenRoom] Could not load file" << std::endl;
        }
    }

    std::cerr << "[VideoGenRoom] Added to history, total items: "
              << room->historyItems.size() << std::endl;
}

float VideoGenRoom::getDetectedBpm() {
    if (mainprogram->beatdet && mainprogram->beatdet->winning_bpm > 0) {
        return 60.0f / mainprogram->beatdet->winning_bpm;
    }
    return 120.0f;  // fallback
}

std::vector<PresetInfo> VideoGenRoom::getFilteredPresets() {
    std::vector<PresetInfo> result;

    // Check current backend using the mapping
    GenerationBackend backend = getSelectedBackend();
    bool isFluxBackend = (backend == GenerationBackend::FLUX_SCHNELL);
    bool isHunyuanFull = (backend == GenerationBackend::HUNYUAN_FULL);

    // Get all presets and filter by backend support
    for (int i = 0; i < (int)PresetType::PRESET_COUNT; i++) {
        PresetInfo preset = ComfyUIManager::getPresetInfo((PresetType)i);

        bool supported = false;
        if (isFluxBackend) {
            // Flux only supports image presets
            supported = preset.supportedByFlux;
        } else {
            // HunyuanVideo - include full support and partial support
            supported = preset.supportedByHunyuan || preset.hunyuanPartialSupport;

            // Check if preset requires Hunyuan Full (FP8) but we're on Hunyuan Slim (GGUF)
            if (supported && preset.requiresHunyuanFull && !isHunyuanFull) {
                supported = false;
            }
        }

        if (supported) {
            result.push_back(preset);
        }
    }

    return result;
}

void VideoGenRoom::selectPreset(int presetIndex) {
    this->selectedPresetIndex = presetIndex;
    this->selectedPreset = (PresetType)presetIndex;
    this->applyPresetDefaults();
}

void VideoGenRoom::applyPresetDefaults() {
    // Frames, width, height persist across preset changes
    // Only apply defaults that are truly preset-specific
}

GenerationParams VideoGenRoom::buildGenerationParams() {
    GenerationParams params;

    params.preset = this->selectedPreset;
    params.backend = getSelectedBackend();

    params.prompt = this->promptstr;
    params.negativePrompt = this->negpromptstr;
    params.seed = (int)this->seed->value;
    params.cfgScale = this->cfgScale->value;
    params.promptImprove = (this->promptImprove->value > 0.5f);

    params.steps = (int)this->steps->value;
    // Flux generates single images
    if (params.backend == GenerationBackend::FLUX_SCHNELL) {
        params.frames = 1;
    } else {
        params.frames = (int)this->frames->value;
    }
    params.width = (int)this->width->value;
    params.height = (int)this->height->value;
    params.fps = (params.backend == GenerationBackend::HUNYUAN_SLIM || params.backend == GenerationBackend::HUNYUAN_FULL) ? 24.0f : this->fps->value;

    params.inputImagePath = this->inputImagePath;
    params.controlNetImagePath = this->controlNetImagePath;
    params.styleImagePath = this->styleImagePath;

    // Denoise strength from GUI (for image-to-motion and video continuation)
    params.denoiseStrength = this->denoiseStrength->value;

    // Flux denoise strength from GUI (for Flux image-to-image)
    params.fluxDenoiseStrength = this->fluxDenoiseStrength->value;

    // Remix strength from GUI (for remix workflow)
    params.remixStrength = this->remixStrength->value;

    // Style strength from GUI (for IP2V / style-to-video)
    params.styleStrength = this->styleStrength->value;

    // Batch variation
    params.batchSize = (int)this->batchSize->value;

    // Video continuation
    params.appendToSource = (this->appendToSource->value > 0.5f);
    params.sourceVideoPath = this->inputImagePath;  // Input video path for continuation

    // Frame interpolation - round to nearest integer (2-8)
    int mult = (int)(this->frameMultiplier->value + 0.5f);
    if (mult < 2) mult = 2;
    if (mult > 8) mult = 8;
    params.interpolationFactor = mult;

    // For frame interpolation, set FPS to input FPS * multiplier
    if (params.preset == PresetType::FRAME_INTERPOLATION && this->inputVideoFps > 0) {
        params.fps = this->inputVideoFps * mult;
    }

    return params;
}

void VideoGenRoom::loadFirstFramePreview(const std::string& path) {
    if (path.empty()) return;

    if (isimage(path)) {
        int w, h;
        auto imgData = ImageLoader::loadImageRGBA(path, &w, &h);
        if (!imgData.empty()) {
            if (this->inputImageTex == (GLuint)-1) glGenTextures(1, &this->inputImageTex);
            glBindTexture(GL_TEXTURE_2D, this->inputImageTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        return;
    }

    // Video: extract first frame using FFmpeg
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0) return;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) { avformat_close_input(&fmtCtx); return; }

    int videoStreamIdx = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { videoStreamIdx = i; break; }
    }
    if (videoStreamIdx < 0) { avformat_close_input(&fmtCtx); return; }

    auto* codecPar = fmtCtx->streams[videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                         codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbaFrame = av_frame_alloc();
    rgbaFrame->format = AV_PIX_FMT_RGBA;
    rgbaFrame->width = codecCtx->width;
    rgbaFrame->height = codecCtx->height;
    av_image_alloc(rgbaFrame->data, rgbaFrame->linesize, rgbaFrame->width, rgbaFrame->height, AV_PIX_FMT_RGBA, 32);

    AVPacket* pkt = av_packet_alloc();
    bool gotFrame = false;
    while (av_read_frame(fmtCtx, pkt) >= 0 && !gotFrame) {
        if (pkt->stream_index != videoStreamIdx) { av_packet_unref(pkt); continue; }
        if (avcodec_send_packet(codecCtx, pkt) >= 0) {
            if (avcodec_receive_frame(codecCtx, frame) >= 0) {
                sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                          rgbaFrame->data, rgbaFrame->linesize);
                int w = codecCtx->width, h = codecCtx->height;
                if (this->inputImageTex == (GLuint)-1) glGenTextures(1, &this->inputImageTex);
                glBindTexture(GL_TEXTURE_2D, this->inputImageTex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaFrame->data[0]);
                glBindTexture(GL_TEXTURE_2D, 0);
                gotFrame = true;
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
}

void VideoGenRoom::clearInputImage() {
    this->inputImagePath = "";
    this->inputVideoFps = 0.0f;
    if (this->inputImageTex != (GLuint)-1) {
        glDeleteTextures(1, &this->inputImageTex);
        this->inputImageTex = -1;
    }
}

void VideoGenRoom::clearControlNetImage() {
    this->controlNetImagePath = "";
    this->controlNetIsVideo = false;
    if (this->controlNetImageTex != (GLuint)-1) {
        glDeleteTextures(1, &this->controlNetImageTex);
        this->controlNetImageTex = -1;
    }
}

void VideoGenRoom::clearStyleImage() {
    this->styleImagePath = "";
    if (this->styleImageTex != (GLuint)-1) {
        glDeleteTextures(1, &this->styleImageTex);
        this->styleImageTex = -1;
    }
}
