/**
 * segmentationroom.cpp
 *
 * UI room for SAM 3 video segmentation and masking
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
#include <algorithm>

#include "ImageLoader.h"

#include "program.h"
#include "segmentationroom.h"
#include "SAMInstaller.h"
#include "videogenroom.h"

// FFmpeg includes for HAP Alpha encoding
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

namespace fs = std::filesystem;

// ============================================================================
// Letterbox Drawing Helper (same pattern as videogenroom.cpp)
// ============================================================================

void draw_box_letterbox_seg(Boxx* box, GLuint tex, int texWidth, int texHeight) {
    if (!box || tex == (GLuint)-1 || texWidth <= 0 || texHeight <= 0) {
        draw_box(box, tex);
        return;
    }

    float boxX = box->vtxcoords->x1;
    float boxY = box->vtxcoords->y1;
    float boxW = box->vtxcoords->w;
    float boxH = box->vtxcoords->h;

    float screenAspect = (float)glob->w / (float)glob->h;
    float texAspect = (float)texWidth / (float)texHeight;
    float boxAspect = boxW / boxH;
    float boxPixelAspect = boxAspect * screenAspect;

    float drawX, drawY, drawW, drawH;

    if (texAspect > boxPixelAspect) {
        drawW = boxW;
        drawH = boxW * screenAspect / texAspect;
        drawX = boxX;
        drawY = boxY + (boxH - drawH) / 2.0f;
    } else {
        drawH = boxH;
        drawW = boxH * texAspect / screenAspect;
        drawX = boxX + (boxW - drawW) / 2.0f;
        drawY = boxY;
    }

    draw_box(nullptr, black, drawX, drawY, drawW, drawH, tex);
}

void draw_box_letterbox_seg_flip(Boxx* box, GLuint tex, int texWidth, int texHeight) {
    if (!box || tex == (GLuint)-1 || texWidth <= 0 || texHeight <= 0) {
        draw_box(box, tex);
        return;
    }

    float boxX = box->vtxcoords->x1;
    float boxY = box->vtxcoords->y1;
    float boxW = box->vtxcoords->w;
    float boxH = box->vtxcoords->h;

    float screenAspect = (float)glob->w / (float)glob->h;
    float texAspect = (float)texWidth / (float)texHeight;
    float boxAspect = boxW / boxH;
    float boxPixelAspect = boxAspect * screenAspect;

    float drawX, drawY, drawW, drawH;

    if (texAspect > boxPixelAspect) {
        drawW = boxW;
        drawH = boxW * screenAspect / texAspect;
        drawX = boxX;
        drawY = boxY + (boxH - drawH) / 2.0f;
    } else {
        drawH = boxH;
        drawW = boxH * texAspect / screenAspect;
        drawX = boxX + (boxW - drawW) / 2.0f;
        drawY = boxY;
    }

    draw_box(black, black, boxX, boxY, boxW, boxH, -1);
    draw_box(nullptr, nullptr, drawX, drawY + drawH, drawW, -drawH, tex);
}

// ============================================================================
// HAP Alpha Encoding
// ============================================================================

/**
 * Encode RGBA PNG frames to HAP Alpha video (DXT5 for alpha support)
 */
static bool hapAlphaEncodeFrames(const std::string& framesDir,
                                  const std::string& outputPath,
                                  float fps,
                                  std::function<void(float)> progressCallback = nullptr) {
    std::cerr << "[HAP Alpha] Starting frame-to-HAP Alpha encoding" << std::endl;

    // Collect and sort frame files
    std::vector<std::string> framePaths;
    for (const auto& entry : fs::directory_iterator(framesDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                framePaths.push_back(entry.path().string());
            }
        }
    }

    if (framePaths.empty()) {
        std::cerr << "[HAP Alpha] No frames found" << std::endl;
        return false;
    }

    std::sort(framePaths.begin(), framePaths.end());
    int numFrames = (int)framePaths.size();

    // Load first frame to get dimensions
    int srcWidth, srcHeight;
    if (!ImageLoader::getImageDimensions(framePaths[0], &srcWidth, &srcHeight)) {
        return false;
    }

    // Find HAP encoder
    const AVCodec* codec = avcodec_find_encoder_by_name("hap");
    if (!codec) {
        std::cerr << "[HAP Alpha] HAP codec not found" << std::endl;
        return false;
    }

    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) return false;

    // HAP alignment: width multiple of 32, height multiple of 4
    int rem = srcWidth % 32;
    c->width = srcWidth + (32 - rem) * (rem > 0);
    rem = srcHeight % 4;
    c->height = srcHeight + (4 - rem) * (rem > 0);

    int fpsInt = (int)(fps + 0.5f);
    if (fpsInt <= 0) fpsInt = 24;
    c->time_base = {1, fpsInt};
    c->framerate = {fpsInt, 1};
    c->pix_fmt = codec->pix_fmts[0];

    // HAP Alpha uses DXT5 format (section type 0x0E for HAP Alpha)
    // compressor 0xB0 = Snappy compression
    // format 0x0E = HAP Alpha (DXT5)
    AVDictionary* opts = nullptr;
    av_dict_set_int(&opts, "compressor", 0xB0, 0);
    av_dict_set(&opts, "format", "hap_alpha", 0);

    int openRet = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);

    if (openRet < 0) {
        std::cerr << "[HAP Alpha] Failed to open HAP Alpha encoder" << std::endl;
        avcodec_free_context(&c);
        return false;
    }

    AVFormatContext* dest = nullptr;
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"),
                                   nullptr, outputPath.c_str());
    if (!dest) {
        avcodec_free_context(&c);
        return false;
    }

    AVStream* destStream = avformat_new_stream(dest, codec);
    destStream->time_base = c->time_base;
    destStream->r_frame_rate = c->framerate;
    avcodec_parameters_from_context(destStream->codecpar, c);

    if (avio_open(&dest->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    if (avformat_write_header(dest, nullptr) < 0) {
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    // Use padded dimensions for both src and dst to avoid scaling.
    // Frames are padded with transparent pixels to the aligned size below.
    SwsContext* swsCtx = sws_getContext(
        c->width, c->height, AV_PIX_FMT_RGBA,
        c->width, c->height, c->pix_fmt,
        SWS_POINT, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        av_frame_free(&frame);
        avio_close(dest->pb);
        avformat_free_context(dest);
        avcodec_free_context(&c);
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    int frameIdx = 0;
    bool success = true;

    for (const auto& framePath : framePaths) {
        int imgW, imgH;
        auto imagePixels = ImageLoader::loadImageRGBA(framePath, &imgW, &imgH);
        if (imagePixels.empty()) continue;

        if (av_image_alloc(frame->data, frame->linesize,
                          c->width, c->height, c->pix_fmt, 32) < 0) {
            success = false;
            break;
        }

        // Pad source to aligned dimensions with transparent pixels (no rescaling)
        std::vector<uint8_t> paddedData(c->width * c->height * 4, 0);
        for (int row = 0; row < srcHeight; row++) {
            memcpy(&paddedData[row * c->width * 4], imagePixels.data() + row * srcWidth * 4, srcWidth * 4);
        }
        const uint8_t* srcData[4] = { paddedData.data(), nullptr, nullptr, nullptr };
        int srcLinesize[4] = { c->width * 4, 0, 0, 0 };
        sws_scale(swsCtx, srcData, srcLinesize, 0, c->height, frame->data, frame->linesize);

        frame->pts = frameIdx;

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
        std::cerr << "[HAP Alpha] Encoded " << frameIdx << " frames to " << outputPath << std::endl;
    }
    return success;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

SegmentationRoom::SegmentationRoom() {
    this->samBackend = new SAMSegmentation();
    this->samBackend->initialize();

    // 16:9 box height from width: h_vtx = w_vtx * glob->w * 9.0 / (16.0 * glob->h)
    float ar169 = (float)glob->w * 9.0f / (16.0f * (float)glob->h);

    // Outline preview box (left half)
    float previewW = 0.90f;
    float previewH = previewW * ar169;
    // Clamp height so boxes don't overflow past the top of the screen on narrow aspect ratios
    float maxH = 0.95f - 0.0f;  // top limit minus y1, leaving room for label above
    if (previewH > maxH) {
        previewH = maxH;
        previewW = previewH / ar169;
    }
    this->outlinePreviewBox = new Boxx;
    this->outlinePreviewBox->vtxcoords->x1 = -0.95f;
    this->outlinePreviewBox->vtxcoords->y1 = 0.0f;
    this->outlinePreviewBox->vtxcoords->w = previewW;
    this->outlinePreviewBox->vtxcoords->h = previewH;
    this->outlinePreviewBox->upvtxtoscr();
    this->outlinePreviewBox->lcolor[0] = 0.5f;
    this->outlinePreviewBox->lcolor[1] = 0.5f;
    this->outlinePreviewBox->lcolor[2] = 0.5f;
    this->outlinePreviewBox->lcolor[3] = 1.0f;
    this->outlinePreviewBox->tooltiptitle = "Outline Preview ";
    this->outlinePreviewBox->tooltip = "Input with segment outlines. Click segments to select/deselect. ";

    // Masked preview box (right half)
    this->maskedPreviewBox = new Boxx;
    this->maskedPreviewBox->vtxcoords->x1 = 0.0f;
    this->maskedPreviewBox->vtxcoords->y1 = 0.0f;
    this->maskedPreviewBox->vtxcoords->w = previewW;
    this->maskedPreviewBox->vtxcoords->h = previewH;
    this->maskedPreviewBox->upvtxtoscr();
    this->maskedPreviewBox->lcolor[0] = 0.5f;
    this->maskedPreviewBox->lcolor[1] = 0.5f;
    this->maskedPreviewBox->lcolor[2] = 0.5f;
    this->maskedPreviewBox->lcolor[3] = 1.0f;
    this->maskedPreviewBox->acolor[0] = 0.0f;
    this->maskedPreviewBox->acolor[1] = 0.0f;
    this->maskedPreviewBox->acolor[2] = 0.0f;
    this->maskedPreviewBox->acolor[3] = 1.0f;
    this->maskedPreviewBox->tooltiptitle = "Masked Preview ";
    this->maskedPreviewBox->tooltip = "Result with selected masks. Checkerboard = transparency. ";

    // Input box (drag target)
    float inputW = 0.15f;
    float inputH = inputW * ar169;
    this->inputBox = new Boxx;
    this->inputBox->vtxcoords->x1 = -0.95f;
    this->inputBox->vtxcoords->y1 = -0.45f;
    this->inputBox->vtxcoords->w = inputW;
    this->inputBox->vtxcoords->h = inputH;
    this->inputBox->upvtxtoscr();
    this->inputBox->lcolor[0] = 0.4f;
    this->inputBox->lcolor[1] = 0.4f;
    this->inputBox->lcolor[2] = 0.6f;
    this->inputBox->lcolor[3] = 1.0f;
    this->inputBox->tooltiptitle = "Input Video ";
    this->inputBox->tooltip = "Drag a video or image from bins here. ";

    // Output box (drag from)
    this->outputBox = new Boxx;
    this->outputBox->vtxcoords->x1 = 0.75f;
    this->outputBox->vtxcoords->y1 = -0.45f;
    this->outputBox->vtxcoords->w = inputW;
    this->outputBox->vtxcoords->h = inputH;
    this->outputBox->upvtxtoscr();
    this->outputBox->lcolor[0] = 0.4f;
    this->outputBox->lcolor[1] = 0.4f;
    this->outputBox->lcolor[2] = 0.6f;
    this->outputBox->lcolor[3] = 1.0f;
    this->outputBox->tooltiptitle = "Output Video ";
    this->outputBox->tooltip = "Drag this video or image to other rooms. ";

    // Prompt box
    this->promptBox = new Boxx;
    this->promptBox->vtxcoords->x1 = -0.75f;
    this->promptBox->vtxcoords->y1 = -0.55f;
    this->promptBox->vtxcoords->w = 1.45f;
    this->promptBox->vtxcoords->h = 0.40f;
    this->promptBox->upvtxtoscr();

    // Segment button
    this->segmentButton = new Boxx;
    this->segmentButton->vtxcoords->x1 = -0.95f;
    this->segmentButton->vtxcoords->y1 = -0.75f;
    this->segmentButton->vtxcoords->w = 0.20f;
    this->segmentButton->vtxcoords->h = 0.10f;
    this->segmentButton->upvtxtoscr();

    // Invert button
    this->invertButton = new Boxx;
    this->invertButton->vtxcoords->x1 = -0.70f;
    this->invertButton->vtxcoords->y1 = -0.75f;
    this->invertButton->vtxcoords->w = 0.15f;
    this->invertButton->vtxcoords->h = 0.10f;
    this->invertButton->upvtxtoscr();

    // Export button
    this->exportButton = new Boxx;
    this->exportButton->vtxcoords->x1 = -0.50f;
    this->exportButton->vtxcoords->y1 = -0.75f;
    this->exportButton->vtxcoords->w = 0.15f;
    this->exportButton->vtxcoords->h = 0.10f;
    this->exportButton->upvtxtoscr();

    // Threshold slider
    this->thresholdParam = new Param;
    this->thresholdParam->name = "Threshold";
    this->thresholdParam->value = 0.3f;
    this->thresholdParam->deflt = 0.3f;
    this->thresholdParam->range[0] = 0.0f;
    this->thresholdParam->range[1] = 1.0f;
    this->thresholdParam->sliding = true;
    this->thresholdParam->box->vtxcoords->x1 = -0.95f;
    this->thresholdParam->box->vtxcoords->y1 = -0.90f;
    this->thresholdParam->box->vtxcoords->w = 0.40f;
    this->thresholdParam->box->vtxcoords->h = 0.075f;
    this->thresholdParam->box->upvtxtoscr();
    this->thresholdParam->box->acolor[0] = 0.2f;
    this->thresholdParam->box->acolor[1] = 0.2f;
    this->thresholdParam->box->acolor[2] = 0.4f;
    this->thresholdParam->box->acolor[3] = 1.0f;
    this->thresholdParam->box->tooltiptitle = "Detection threshold ";
    this->thresholdParam->box->tooltip = "Score threshold for text prompt detection. Lower = more detections, higher = more confident. ";

    // Progress box
    this->progressBox = new Boxx;
    this->progressBox->vtxcoords->x1 = -0.30f;
    this->progressBox->vtxcoords->y1 = -0.75f;
    this->progressBox->vtxcoords->w = 0.50f;
    this->progressBox->vtxcoords->h = 0.10f;
    this->progressBox->upvtxtoscr();

}

SegmentationRoom::~SegmentationRoom() {
    if (exportThread && exportThread->joinable()) {
        exportThread->join();
    }
    delete samBackend;
    delete outlinePreviewBox;
    delete maskedPreviewBox;
    delete promptBox;
    delete segmentButton;
    delete invertButton;
    delete exportButton;
    delete progressBox;
    delete inputBox;
    delete outputBox;
    delete thresholdParam;
}

// ============================================================================
// Checkerboard Texture
// ============================================================================

void SegmentationRoom::generateCheckerboard() {
    // 16:9 texture so tiles stay square when stretched to the 16:9 preview box
    int w = 256;
    int h = 144;
    int tileSize = 8;
    std::vector<uint8_t> pixels(w * h * 4);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool white = ((x / tileSize) + (y / tileSize)) % 2 == 0;
            uint8_t val = white ? 200 : 160;
            int idx = (y * w + x) * 4;
            pixels[idx + 0] = val;
            pixels[idx + 1] = val;
            pixels[idx + 2] = val;
            pixels[idx + 3] = 255;
        }
    }

    glGenTextures(1, &checkerboardTex);
    glBindTexture(GL_TEXTURE_2D, checkerboardTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// Load First Frame Preview
// ============================================================================

void SegmentationRoom::loadFirstFramePreview(const std::string& path, bool inout) {
    if (path.empty()) return;

    if (isimage(path)) {
        // Image: load directly with FFmpeg
        int w, h;
        auto imgData = ImageLoader::loadImageRGBA(path, &w, &h);
        if (!imgData.empty()) {
            if (inout){
                if (outputTex == (GLuint)-1) {
                    glGenTextures(1, &outputTex);
                }
                glBindTexture(GL_TEXTURE_2D, outputTex);
            }
            else {
                if (inputTex == (GLuint) -1) {
                    glGenTextures(1, &inputTex);
                }
                glBindTexture(GL_TEXTURE_2D, inputTex);
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            if (inout) {
                outputTexWidth = w;
                outputTexHeight = h;
            }
            else {
                inputTexWidth = w;
                inputTexHeight = h;
            }
        }
        return;
    }

    // Video: extract first frame using FFmpeg
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0) return;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return;
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
        return;
    }

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
    av_image_alloc(rgbaFrame->data, rgbaFrame->linesize,
                   rgbaFrame->width, rgbaFrame->height, AV_PIX_FMT_RGBA, 32);

    AVPacket* pkt = av_packet_alloc();
    bool gotFrame = false;

    while (av_read_frame(fmtCtx, pkt) >= 0 && !gotFrame) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(codecCtx, pkt) >= 0) {
            if (avcodec_receive_frame(codecCtx, frame) >= 0) {
                sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                          rgbaFrame->data, rgbaFrame->linesize);

                int w = codecCtx->width;
                int h = codecCtx->height;

                if (inout){
                    if (outputTex == (GLuint)-1) {
                        glGenTextures(1, &outputTex);
                    }
                    glBindTexture(GL_TEXTURE_2D, outputTex);
                }
                else {
                    if (inputTex == (GLuint) -1) {
                        glGenTextures(1, &inputTex);
                    }
                    glBindTexture(GL_TEXTURE_2D, inputTex);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaFrame->data[0]);
                glBindTexture(GL_TEXTURE_2D, 0);
                if (inout) {
                    outputTexWidth = w;
                    outputTexHeight = h;
                }
                else {
                    inputTexWidth = w;
                    inputTexHeight = h;
                }

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

// ============================================================================
// Main Handle Loop
// ============================================================================

void SegmentationRoom::handle() {
    // Lazy-init checkerboard texture (needs active GL context)
    if (checkerboardTex == (GLuint)-1) {
        generateCheckerboard();
    }

    // Update progress from backend
    if (samBackend->isProcessing()) {
        progressPercent = samBackend->getProgress();
        progressStatus = samBackend->getStatus();
    }

    // Upload pixel data to GL textures on the main thread
    if (samBackend->hasNewResults()) {
        samBackend->uploadResultTextures();
        auto result = samBackend->getResult();
        outlineTex = result.outlineTex;
        maskedTex = result.maskedTex;
    }

    // =====================
    // Draw Outline Preview (left)
    // =====================
    draw_box(white, black, this->outlinePreviewBox, -1);
    render_text("OUTLINE PREVIEW", white, this->outlinePreviewBox->vtxcoords->x1,
                this->outlinePreviewBox->vtxcoords->y1 + this->outlinePreviewBox->vtxcoords->h + 0.01f,
                0.0006f, 0.001f);

    auto result = samBackend->getResult();
    if (outlineTex != (GLuint)-1 && result.width > 0) {
        draw_box_letterbox_seg(this->outlinePreviewBox, outlineTex, result.width, result.height);
    } else if (inputTex != (GLuint)-1 && inputTexWidth > 0 && inputTexHeight > 0) {
        draw_box_letterbox_seg(this->outlinePreviewBox, inputTex, inputTexWidth, inputTexHeight);
    }

    // Handle click on outline preview for mask selection
    if (this->outlinePreviewBox->in() && mainprogram->leftmouse && !samBackend->isProcessing()) {
        if (result.width > 0 && result.height > 0) {
            // Convert mouse position to normalized coords within the preview box
            float boxX = this->outlinePreviewBox->vtxcoords->x1;
            float boxY = this->outlinePreviewBox->vtxcoords->y1;
            float boxW = this->outlinePreviewBox->vtxcoords->w;
            float boxH = this->outlinePreviewBox->vtxcoords->h;

            float mx = -1.0f + mainprogram->xscrtovtx((float)mainprogram->mx);
            float my = 1.0f - mainprogram->yscrtovtx((float)mainprogram->my);

            float nx = (mx - boxX) / boxW;
            float ny = 1.0f - (my - boxY) / boxH;  // Flip Y

            if (nx >= 0.0f && nx <= 1.0f && ny >= 0.0f && ny <= 1.0f) {
                int maskId = samBackend->getMaskAtPosition(nx, ny);
                if (maskId >= 0) {
                    samBackend->toggleMaskSelection(maskId);
                    // Re-render outlines and masked output (pixel data only, no GL)
                    samBackend->composeMaskedOutput();
                    // Textures will be uploaded on next frame via hasNewResults()
                }
            }
            mainprogram->leftmouse = false;
        }
    }

    // =====================
    // Draw Masked Preview (right)
    // =====================
    draw_box(white, black, this->maskedPreviewBox, checkerboardTex);
    render_text("MASKED PREVIEW", white, this->maskedPreviewBox->vtxcoords->x1,
                this->maskedPreviewBox->vtxcoords->y1 + this->maskedPreviewBox->vtxcoords->h + 0.01f,
                0.0006f, 0.001f);

    // Draw masked texture on top of checkerboard with alpha blending + letterbox
    if (maskedTex != (GLuint)-1 && result.width > 0) {
        float boxX = maskedPreviewBox->vtxcoords->x1;
        float boxY = maskedPreviewBox->vtxcoords->y1;
        float boxW = maskedPreviewBox->vtxcoords->w;
        float boxH = maskedPreviewBox->vtxcoords->h;
        float screenAspect = (float)glob->w / (float)glob->h;
        float texAspect = (float)result.width / (float)result.height;
        float boxPixelAspect = boxW / boxH * screenAspect;
        float drawX, drawY, drawW, drawH;
        if (texAspect > boxPixelAspect) {
            drawW = boxW;
            drawH = boxW * screenAspect / texAspect;
            drawX = boxX;
            drawY = boxY + (boxH - drawH) / 2.0f;
        } else {
            drawH = boxH;
            drawW = boxH * texAspect / screenAspect;
            drawX = boxX + (boxW - drawW) / 2.0f;
            drawY = boxY;
        }
        mainprogram->frontbatch = true;
        draw_box(nullptr, black, drawX, drawY, drawW, drawH, maskedTex);
        mainprogram->frontbatch = false;
    }

    // =====================
    // Draw Input Box
    // =====================
    render_text("INPUT", white, this->inputBox->vtxcoords->x1,
                this->inputBox->vtxcoords->y1 + this->inputBox->vtxcoords->h + 0.01f,
                0.00045f, 0.00075f);
    if (inputTex != (GLuint)-1 && inputTexWidth > 0 && inputTexHeight > 0) {
        draw_box_letterbox_seg(this->inputBox, inputTex, inputTexWidth, inputTexHeight);
    } else {
        draw_box(this->inputBox, -1);
    }

    if (this->inputBox->in()) {
        // Drag from bins
        if (mainprogram->lmover && mainprogram->dragbinel) {
            this->inputVideoPath = mainprogram->dragbinel->path;

            mainprogram->rightmouse = true;
            binsmain->handle(0);
            enddrag();
            mainprogram->rightmouse = false;

            // Load full first frame as preview (not the small bin thumbnail)
            loadFirstFramePreview(this->inputVideoPath, 0);
        }

        // Right-click menu for browse
        if (mainprogram->menuactivation) {
            // Right-click menu
            this->menuoptions.clear();
            std::vector<std::string> opts;
            opts.push_back("Browse");
            this->menuoptions.push_back(SEG_BROWSEINPUT);
            mainprogram->make_menu("segmenu", this->segmenu, opts);
            this->segmenu->state = 2;
            this->segmenu->menux = mainprogram->mx;
            this->segmenu->menuy = mainprogram->my;
            mainprogram->menuactivation = false;
        }
    }

    // =====================
    // Draw Output Box
    // =====================
    render_text("OUTPUT", white, this->outputBox->vtxcoords->x1,
                this->outputBox->vtxcoords->y1 + this->outputBox->vtxcoords->h + 0.01f,
                0.00045f, 0.00075f);
    if (outputTex != (GLuint)-1 && outputTexWidth > 0 && outputTexHeight > 0) {
        draw_box_letterbox_seg(this->outputBox, outputTex, outputTexWidth, outputTexHeight);
    } else {
        draw_box(this->outputBox, -1);
    }

    if (this->outputBox->in()) {
        // Drag from bins
        if (mainprogram->leftmousedown) {
            mainprogram->dragbinel = new BinElement;
            if (isimage(this->exportedpath)) {
                mainprogram->dragbinel->type = ELEM_IMAGE;
            }
            else {
                mainprogram->dragbinel->type = ELEM_FILE;
            }
            mainprogram->dragbinel->path = this->exportedpath;
            mainprogram->dragbinel->tex = this->outputTex;
            this->dragging = true;
            mainprogram->leftmousedown = false;
        }
    }

    // Browse result handled in start.cpp pathto handler

    // =====================
    // Draw Prompt Box
    // =====================
    draw_box(white, darkgreen2, this->promptBox, -1);
    render_text("PROMPT", white, this->promptBox->vtxcoords->x1,
                this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h + 0.01f,
                0.0006f, 0.001f);

    if (mainprogram->renaming == EDIT_SEGPROMPT) {
        this->promptlines = do_text_input_multiple_lines(
            this->promptBox->vtxcoords->x1 + 0.025f,
            this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h - 0.1f,
            0.00072f, 0.00120f,
            mainprogram->mx, mainprogram->my,
            mainprogram->xvtxtoscr(this->promptBox->vtxcoords->w - 0.05f),
            0.05f, 6, 0, nullptr);
        this->promptstr = "";
        for (auto& line : this->promptlines) {
            if (!this->promptstr.empty()) this->promptstr += " ";
            this->promptstr += line;
        }
    } else {
        int count = 0;
        for (auto& line : this->promptlines) {
            if (count == 6) break;
            render_text(line, white,
                        this->promptBox->vtxcoords->x1 + 0.025f,
                        this->promptBox->vtxcoords->y1 + this->promptBox->vtxcoords->h - 0.1f - (0.05f * count),
                        0.00072f, 0.00120f);
            count++;
        }
    }

    if (this->promptBox->in()) {
        if (mainprogram->renaming == EDIT_NONE && mainprogram->leftmouse) {
            mainprogram->renaming = EDIT_SEGPROMPT;
            this->oldpromptstr = this->promptstr;
            mainprogram->inputtext = this->promptstr;
            mainprogram->cursorpos0 = mainprogram->inputtext.length();
            SDL_StartTextInput();
            mainprogram->leftmouse = false;
            mainprogram->recundo = false;
        }
    } else {
        if (mainprogram->renaming == EDIT_SEGPROMPT) {
            if (mainprogram->leftmouse) {
                mainprogram->renaming = EDIT_NONE;
                SDL_StopTextInput();
                mainprogram->rightmouse = false;
                mainprogram->menuactivation = false;
            }
        }
    }

    // =====================
    // Draw Segment / Cancel Button
    // =====================
    bool isWorking = samBackend->isProcessing() || exporting.load();
    bool canSegment = !inputVideoPath.empty() && !promptstr.empty() && !isWorking;

    if (isWorking) {
        draw_box(white, darkred1, this->segmentButton, -1);
        render_text("CANCEL", white,
                    this->segmentButton->vtxcoords->x1 + 0.02f,
                    this->segmentButton->vtxcoords->y1 + 0.03f,
                    0.00060f, 0.00100f);
    } else if (canSegment) {
        draw_box(white, darkgreen1, this->segmentButton, -1);
        render_text("SEGMENT", white,
                    this->segmentButton->vtxcoords->x1 + 0.02f,
                    this->segmentButton->vtxcoords->y1 + 0.03f,
                    0.00060f, 0.00100f);
    } else {
        draw_box(white, darkgrey, this->segmentButton, -1);
        render_text("SEGMENT", white,
                    this->segmentButton->vtxcoords->x1 + 0.02f,
                    this->segmentButton->vtxcoords->y1 + 0.03f,
                    0.00060f, 0.00100f);
    }

    if (this->segmentButton->in() && mainprogram->leftmouse) {
        if (isWorking) {
            if (samBackend->isProcessing()) {
                samBackend->cancelSegmentation();
            }
            if (exporting.load()) {
                exportCancelled.store(true);
            }
        } else if (canSegment) {
            startSegmentation();
        }
        mainprogram->leftmouse = false;
    }

    // =====================
    // Draw Invert Button
    // =====================
    if (inverted) {
        draw_box(white, darkgreen1, this->invertButton, -1);
    } else {
        draw_box(white, darkgrey, this->invertButton, -1);
    }
    render_text("INVERT", white,
                this->invertButton->vtxcoords->x1 + 0.02f,
                this->invertButton->vtxcoords->y1 + 0.03f,
                0.00060f, 0.00100f);

    if (this->invertButton->in() && mainprogram->leftmouse) {
        toggleInvert();
        mainprogram->leftmouse = false;
    }

    // =====================
    // Draw Export Button
    // =====================
    bool canExport = !inputVideoPath.empty() && !samBackend->isProcessing() &&
                     !exporting.load() && result.masks.size() > 0;

    if (canExport) {
        draw_box(white, darkgreen1, this->exportButton, -1);
    } else {
        draw_box(white, darkgrey, this->exportButton, -1);
    }
    render_text("EXPORT", white,
                this->exportButton->vtxcoords->x1 + 0.02f,
                this->exportButton->vtxcoords->y1 + 0.03f,
                0.00060f, 0.00100f);

    if (this->exportButton->in() && mainprogram->leftmouse && canExport) {
        // Launch file dialog in detached thread
        std::thread filereq(&Program::get_outname, mainprogram,
                            "Export masked video", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
        filereq.detach();
        mainprogram->pathto = "EXPORTSEGMENTATION";
        mainprogram->leftmouse = false;
    }

    // =====================
    // Draw Progress Box
    // =====================
    draw_box(white, black, this->progressBox, -1);
    if (isWorking) {
        float fillWidth = this->progressBox->vtxcoords->w * (progressPercent / 100.0f);
        draw_box(nullptr, darkgreen1, this->progressBox->vtxcoords->x1,
                 this->progressBox->vtxcoords->y1,
                 fillWidth, this->progressBox->vtxcoords->h, -1);
    }

    std::string statusStr = progressStatus;
    if (samBackend->isProcessing()) {
        statusStr = samBackend->getStatus();
    } else if (exporting.load()) {
        statusStr = "Exporting...";
    }

    render_text(statusStr, white,
                this->progressBox->vtxcoords->x1 + 0.01f,
                this->progressBox->vtxcoords->y1 + 0.03f,
                0.00050f, 0.00085f);

    // Load output preview after successful export (main thread for OpenGL)
    if (exportFinishedSuccess.load()) {
        exportFinishedSuccess.store(false);
        this->loadFirstFramePreview(this->exportedpath, 1);
    }

    // =====================
    // Draw Threshold Slider
    // =====================
    this->thresholdParam->handle();



    // Quit right-click menu
    if (mainprogram->menuactivation) {
        // Right-click menu
        this->menuoptions.clear();
        std::vector<std::string> opts;
        opts.push_back("Quit");
        this->menuoptions.push_back(SEG_QUIT);
        mainprogram->make_menu("segmenu", this->segmenu, opts);
        this->segmenu->state = 2;
        this->segmenu->menux = mainprogram->mx;
        this->segmenu->menuy = mainprogram->my;
        mainprogram->menuactivation = false;
    }

    // =====================
    // Handle menu
    // =====================
    if (this->segmenu) {
        int k = mainprogram->handle_menu(this->segmenu);
        if (k > -1) {
            if (this->menuoptions[k] == SEG_BROWSEINPUT) {
                // Launch file dialog in detached thread
                std::thread filereq(&Program::get_inname, mainprogram,
                                    "Select video or image", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
                mainprogram->pathto = "SEGMENTATIONINPUT";
            }
            else if (this->menuoptions[k] == SEG_QUIT) {
                // quit program
                mainprogram->quitting = "quitted";
            }
        }
    }
}

// ============================================================================
// Actions
// ============================================================================

void SegmentationRoom::startSegmentation() {
    if (inputVideoPath.empty() || promptstr.empty()) return;

    // Apply threshold from slider
    samBackend->scoreThreshold = thresholdParam->value;

    progressStatus = "Starting ComfyUI server...";
    progressPercent = 0.0f;

    // Launch segmentation in a thread so ComfyUI startup doesn't block the UI
    std::thread segThread([this]() {
        // Start ComfyUI server if not already running
        if (!startComfyUIServer([this](const std::string& status) {
            this->progressStatus = status;
        })) {
            this->progressStatus = "Failed to start ComfyUI server";
            return;
        }

        // Initialize backend connection if needed
        if (!samBackend->isProcessing()) {
            samBackend->initialize("127.0.0.1", 8188);
        }

        // Wait for ComfyUI to be reachable (retry connection for up to 60s)
        this->progressStatus = "Connecting to ComfyUI server...";
        bool connected = false;
        for (int retry = 0; retry < 60; retry++) {
            // Try a simple HTTP GET to check if server is up
            std::string testResult = samBackend->testConnection();
            if (!testResult.empty()) {
                connected = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->progressStatus = "Waiting for ComfyUI server... (" + std::to_string(retry + 1) + "s)";
        }

        if (!connected) {
            this->progressStatus = "ComfyUI server not responding after 60s";
            return;
        }

        this->progressStatus = "Starting segmentation...";
        this->progressPercent = 0.0f;

        if (isvideo(this->inputVideoPath)) {
            samBackend->segmentVideo(this->inputVideoPath, this->promptstr);
        } else {
            samBackend->segmentFrame(this->inputVideoPath, this->promptstr);
        }
    });
    segThread.detach();
}

void SegmentationRoom::toggleInvert() {
    inverted = !inverted;
    samBackend->setInverted(inverted);
    samBackend->composeMaskedOutput();
    // Textures will be uploaded on next frame via hasNewResults()
}

void SegmentationRoom::startExport(const std::string& outputPath) {
    if (exporting.load()) return;
    if (outputPath.empty()) return;

    std::string outPath = outputPath;
    if (isimage(inputVideoPath)) {
        // Image input: export as PNG
        std::string ext = fs::path(outPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".png") {
            // Strip existing extension if any, add .png
            if (!ext.empty()) outPath = outPath.substr(0, outPath.size() - ext.size());
            outPath += ".png";
        }
    } else {
        // Video input: export as HAP Alpha .mov
        if (outPath.size() < 4 || outPath.substr(outPath.size() - 4) != ".mov") {
            outPath += ".mov";
        }
    }
    this->exportedpath = outPath;
    exportCancelled.store(false);
    exporting.store(true);
    if (exportThread && exportThread->joinable()) {
        exportThread->join();
    }
    exportThread = std::make_unique<std::thread>(&SegmentationRoom::exportThreadFunc,
                                                  this, inputVideoPath, outPath);
}

void SegmentationRoom::exportThreadFunc(std::string videoPath, std::string outputPath) {
    // Image input: save masked pixel data directly as RGBA PNG
    if (isimage(videoPath)) {
        progressStatus = "Exporting masked image...";
        progressPercent = 0.0f;

        const auto& pixelData = samBackend->getMaskedPixelData();
        int pixW = samBackend->getMaskedPixelWidth();
        int pixH = samBackend->getMaskedPixelHeight();
        if (pixelData.empty() || pixW <= 0 || pixH <= 0) {
            progressStatus = "No masked data to export";
            exporting.store(false);
            return;
        }

        // Flip from bottom-up (GL convention) to top-down for PNG export
        std::vector<uint8_t> flipped(pixelData.begin(), pixelData.end());
        int rowSize = pixW * 4;
        for (int y = 0; y < pixH / 2; y++) {
            int topOff = y * rowSize;
            int botOff = (pixH - 1 - y) * rowSize;
            for (int i = 0; i < rowSize; i++) {
                std::swap(flipped[topOff + i], flipped[botOff + i]);
            }
        }

        bool success = ImageLoader::saveImagePNG(outputPath, flipped.data(), pixW, pixH);

        if (exportCancelled.load()) {
            progressStatus = "Export cancelled";
            try { fs::remove(outputPath); } catch (...) {}
        } else if (success) {
            progressStatus = "Export complete: " + fs::path(outputPath).filename().string();
            exportFinishedSuccess.store(true);
        } else {
            progressStatus = "PNG export failed";
        }

        exporting.store(false);
        return;
    }

    // Video input: export frames then encode to HAP Alpha
    progressStatus = "Exporting masked frames...";

    // Create temp directory for frames
    std::string tempDir = mainprogram->temppath + "/sam_export_frames";
    fs::create_directories(tempDir);

    // Export masked frames as RGBA PNGs
    bool success = samBackend->exportMaskedFrames(videoPath, tempDir,
        [this](float progress) {
            if (exportCancelled.load()) return;
            progressPercent = progress * 50.0f;  // First 50% for frame export
            progressStatus = "Exporting frames... " + std::to_string((int)(progress * 100.0f)) + "%";
        });

    if (exportCancelled.load()) {
        progressStatus = "Export cancelled";
        try { fs::remove_all(tempDir); } catch (...) {}
        exporting.store(false);
        return;
    }

    if (!success) {
        progressStatus = "Frame export failed";
        exporting.store(false);
        return;
    }

    progressStatus = "Encoding HAP Alpha video...";

    // Determine FPS (use video FPS if available, fallback to 24)
    float fps = 24.0f;
    // Get video info for FPS
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, videoPath.c_str(), nullptr, nullptr) >= 0) {
        if (avformat_find_stream_info(fmtCtx, nullptr) >= 0) {
            for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
                if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    AVRational fr = fmtCtx->streams[i]->r_frame_rate;
                    if (fr.den > 0) fps = (float)fr.num / (float)fr.den;
                    break;
                }
            }
        }
        avformat_close_input(&fmtCtx);
    }

    if (exportCancelled.load()) {
        progressStatus = "Export cancelled";
        try { fs::remove_all(tempDir); } catch (...) {}
        exporting.store(false);
        return;
    }

    // Encode to HAP Alpha
    success = hapAlphaEncodeFrames(tempDir, outputPath, fps,
        [this](float progress) {
            if (exportCancelled.load()) return;
            progressPercent = 50.0f + progress * 50.0f;  // Last 50% for encoding
            progressStatus = "Encoding HAP Alpha... " + std::to_string((int)(50.0f + progress * 50.0f)) + "%";
        });


    // Cleanup temp frames
    try {
        fs::remove_all(tempDir);
    } catch (...) {}

    if (exportCancelled.load()) {
        progressStatus = "Export cancelled";
        // Remove partial output file
        try { fs::remove(outputPath); } catch (...) {}
    } else if (success) {
        progressStatus = "Export complete: " + fs::path(outputPath).filename().string();
        exportFinishedSuccess.store(true);
    } else {
        progressStatus = "Encoding failed";
    }

    exporting.store(false);
}
