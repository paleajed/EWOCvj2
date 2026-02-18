#include "ImageLoader.h"

#include <fstream>
#include <algorithm>
#include <cstring>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
}

// Internal helper: decode a single image frame with specified pixel format
static std::vector<uint8_t> decodeImage(const std::string& path, int* outW, int* outH, AVPixelFormat targetFmt, int channels) {
    std::vector<uint8_t> result;

    AVFormatContext* fmtCtx = nullptr;
    const AVInputFormat* imgFmt = av_find_input_format("image2");
    if (avformat_open_input(&fmtCtx, path.c_str(), imgFmt, nullptr) < 0) return result;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return result;
    }
    if (fmtCtx->nb_streams == 0) { avformat_close_input(&fmtCtx); return result; }

    const AVCodec* decoder = avcodec_find_decoder(fmtCtx->streams[0]->codecpar->codec_id);
    if (!decoder) { avformat_close_input(&fmtCtx); return result; }

    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, fmtCtx->streams[0]->codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&fmtCtx);
        return result;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (av_read_frame(fmtCtx, pkt) >= 0) {
        if (avcodec_send_packet(decCtx, pkt) >= 0) {
            if (avcodec_receive_frame(decCtx, frame) >= 0) {
                int w = frame->width;
                int h = frame->height;

                SwsContext* swsCtx = sws_getContext(
                    w, h, (AVPixelFormat)frame->format,
                    w, h, targetFmt,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);

                if (swsCtx) {
                    result.resize(w * h * channels);
                    uint8_t* dstData[1] = { result.data() };
                    int dstLinesize[1] = { w * channels };
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, h, dstData, dstLinesize);
                    sws_freeContext(swsCtx);

                    if (outW) *outW = w;
                    if (outH) *outH = h;
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&decCtx);
    avformat_close_input(&fmtCtx);
    return result;
}

std::vector<uint8_t> ImageLoader::loadImageRGBA(const std::string& path, int* outW, int* outH) {
    return decodeImage(path, outW, outH, AV_PIX_FMT_RGBA, 4);
}

std::vector<uint8_t> ImageLoader::loadImageRGB(const std::string& path, int* outW, int* outH) {
    return decodeImage(path, outW, outH, AV_PIX_FMT_RGB24, 3);
}

std::vector<uint8_t> ImageLoader::loadImageGray(const std::string& path, int* outW, int* outH) {
    return decodeImage(path, outW, outH, AV_PIX_FMT_GRAY8, 1);
}

bool ImageLoader::getImageDimensions(const std::string& path, int* outW, int* outH) {
    AVFormatContext* fmtCtx = nullptr;
    const AVInputFormat* imgFmt = av_find_input_format("image2");
    if (avformat_open_input(&fmtCtx, path.c_str(), imgFmt, nullptr) < 0) return false;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }
    if (fmtCtx->nb_streams == 0) { avformat_close_input(&fmtCtx); return false; }

    if (outW) *outW = fmtCtx->streams[0]->codecpar->width;
    if (outH) *outH = fmtCtx->streams[0]->codecpar->height;
    avformat_close_input(&fmtCtx);
    return true;
}

std::shared_ptr<LoadedImage> ImageLoader::loadImageMultiFrame(const std::string& path) {
    auto img = std::make_shared<LoadedImage>();

    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0) return nullptr;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return nullptr;
    }
    if (fmtCtx->nb_streams == 0) { avformat_close_input(&fmtCtx); return nullptr; }

    int streamIdx = 0;
    const AVCodec* decoder = avcodec_find_decoder(fmtCtx->streams[streamIdx]->codecpar->codec_id);
    if (!decoder) { avformat_close_input(&fmtCtx); return nullptr; }

    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, fmtCtx->streams[streamIdx]->codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        avcodec_free_context(&decCtx);
        avformat_close_input(&fmtCtx);
        return nullptr;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    SwsContext* swsCtx = nullptr;
    int prevW = 0, prevH = 0;
    AVPixelFormat prevFmt = AV_PIX_FMT_NONE;

    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index != streamIdx) {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(decCtx, pkt) >= 0) {
            while (avcodec_receive_frame(decCtx, frame) >= 0) {
                int w = frame->width;
                int h = frame->height;
                AVPixelFormat srcFmt = (AVPixelFormat)frame->format;

                // Recreate sws context if dimensions or format changed
                if (!swsCtx || w != prevW || h != prevH || srcFmt != prevFmt) {
                    if (swsCtx) sws_freeContext(swsCtx);
                    swsCtx = sws_getContext(
                        w, h, srcFmt,
                        w, h, AV_PIX_FMT_RGBA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                    prevW = w;
                    prevH = h;
                    prevFmt = srcFmt;
                }

                if (swsCtx) {
                    ImageFrame imgFrame;
                    imgFrame.width = w;
                    imgFrame.height = h;
                    imgFrame.channels = 4;
                    imgFrame.pixels.resize(w * h * 4);

                    uint8_t* dstData[1] = { imgFrame.pixels.data() };
                    int dstLinesize[1] = { w * 4 };
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, h, dstData, dstLinesize);

                    // Frame duration from packet
                    AVStream* stream = fmtCtx->streams[streamIdx];
                    if (pkt->duration > 0) {
                        imgFrame.duration_ms = pkt->duration * av_q2d(stream->time_base) * 1000.0;
                    } else {
                        imgFrame.duration_ms = 100.0;  // default 100ms for GIF frames
                    }

                    img->frames.push_back(std::move(imgFrame));
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Flush decoder
    avcodec_send_packet(decCtx, nullptr);
    while (avcodec_receive_frame(decCtx, frame) >= 0) {
        int w = frame->width;
        int h = frame->height;
        AVPixelFormat srcFmt = (AVPixelFormat)frame->format;

        if (!swsCtx || w != prevW || h != prevH || srcFmt != prevFmt) {
            if (swsCtx) sws_freeContext(swsCtx);
            swsCtx = sws_getContext(
                w, h, srcFmt,
                w, h, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            prevW = w;
            prevH = h;
            prevFmt = srcFmt;
        }

        if (swsCtx) {
            ImageFrame imgFrame;
            imgFrame.width = w;
            imgFrame.height = h;
            imgFrame.channels = 4;
            imgFrame.pixels.resize(w * h * 4);

            uint8_t* dstData[1] = { imgFrame.pixels.data() };
            int dstLinesize[1] = { w * 4 };
            sws_scale(swsCtx, frame->data, frame->linesize, 0, h, dstData, dstLinesize);
            imgFrame.duration_ms = 100.0;
            img->frames.push_back(std::move(imgFrame));
        }
    }

    if (swsCtx) sws_freeContext(swsCtx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&decCtx);
    avformat_close_input(&fmtCtx);

    if (img->frames.empty()) return nullptr;

    // Set convenience fields from frame 0
    img->width = img->frames[0].width;
    img->height = img->frames[0].height;
    img->channels = img->frames[0].channels;
    img->bpp = img->channels;

    return img;
}

bool ImageLoader::isImage(const std::string& path) {
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0) return false;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }
    if (fmtCtx->nb_streams == 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }
    // Check if the detected format is an image format
    std::string name = fmtCtx->iformat->name;
    avformat_close_input(&fmtCtx);
    // FFmpeg image demuxer names
    if (name == "image2" || name == "png_pipe" || name == "bmp_pipe" ||
        name == "jpeg_pipe" || name == "jpegls_pipe" || name == "jpegxl_pipe" ||
        name == "tiff_pipe" || name == "webp_pipe" || name == "ppm_pipe" ||
        name == "pgm_pipe" || name == "pbm_pipe" || name == "pam_pipe" ||
        name == "svg_pipe" || name == "gif" || name == "ico") {
        return true;
    }
    return false;
}

bool ImageLoader::saveImagePNG(const std::string& path, const uint8_t* pixels, int w, int h, int channels) {
    AVPixelFormat srcFmt;
    if (channels == 4) srcFmt = AV_PIX_FMT_RGBA;
    else if (channels == 3) srcFmt = AV_PIX_FMT_RGB24;
    else if (channels == 1) srcFmt = AV_PIX_FMT_GRAY8;
    else return false;

    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encoder) return false;

    AVCodecContext* encCtx = avcodec_alloc_context3(encoder);
    encCtx->width = w;
    encCtx->height = h;
    encCtx->pix_fmt = srcFmt;
    encCtx->time_base = {1, 1};
    encCtx->compression_level = 1;

    if (avcodec_open2(encCtx, encoder, nullptr) < 0) {
        avcodec_free_context(&encCtx);
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = srcFmt;
    frame->width = w;
    frame->height = h;
    av_frame_get_buffer(frame, 32);

    int srcStride = w * channels;
    for (int y = 0; y < h; y++) {
        memcpy(frame->data[0] + y * frame->linesize[0],
               pixels + y * srcStride, srcStride);
    }
    frame->pts = 0;

    AVPacket* pkt = av_packet_alloc();
    bool ok = false;

    if (avcodec_send_frame(encCtx, frame) >= 0) {
        if (avcodec_receive_packet(encCtx, pkt) >= 0) {
            std::ofstream out(path, std::ios::binary);
            if (out.is_open()) {
                out.write(reinterpret_cast<const char*>(pkt->data), pkt->size);
                ok = true;
            }
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&encCtx);
    return ok;
}
