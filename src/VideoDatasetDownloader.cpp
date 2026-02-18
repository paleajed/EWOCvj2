/**
 * VideoDatasetDownloader.cpp
 *
 * Implementation of video dataset downloading and frame extraction
 *
 * License: GPL3
 */

#include "VideoDatasetDownloader.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <set>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <turbojpeg.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Diverse search queries for varied content
const std::vector<std::string> VideoDatasetDownloader::SEARCH_QUERIES = {
    // Nature
    "forest", "ocean waves", "mountains", "waterfall", "sunset clouds",
    "river flowing", "wind trees", "rain drops", "snow falling", "flowers blooming",
    // Animals
    "birds flying", "fish swimming", "dog running", "cat playing", "horses galloping",
    "butterflies", "wildlife", "underwater life", "safari animals", "insects",
    // Urban/City
    "city traffic", "street walking", "neon lights", "subway train", "skyscrapers",
    "busy intersection", "night city", "urban crowd", "shopping street", "cafe scene",
    // People/Action
    "dancing", "sports action", "running", "cooking", "painting art",
    "musician playing", "yoga", "skateboarding", "surfing", "climbing",
    // Abstract/Effects
    "fire flames", "smoke", "water splash", "light particles", "abstract motion",
    "ink water", "bubbles", "sparks", "fog mist", "aurora",
    // Technology/Industrial
    "factory machines", "data center", "robotics", "3d printing", "laboratory",
    // Weather/Sky
    "thunderstorm", "clouds timelapse", "starry night", "northern lights", "foggy morning",
    // Food/Objects
    "pouring liquid", "food preparation", "coffee brewing", "candle flame", "fabric flowing"
};

VideoDatasetDownloader::VideoDatasetDownloader() {
}

VideoDatasetDownloader::~VideoDatasetDownloader() {
    cancelDownload();
    if (downloadThread && downloadThread->joinable()) {
        downloadThread->join();
    }
}

bool VideoDatasetDownloader::startDownload(const VideoDatasetConfig& config) {
    if (downloading.load()) {
        return false;
    }

    currentConfig = config;
    shouldCancel.store(false);
    downloading.store(true);

    if (downloadThread && downloadThread->joinable()) {
        downloadThread->join();
    }
    downloadThread = std::make_unique<std::thread>(&VideoDatasetDownloader::downloadThreadFunc, this, config);

    return true;
}

void VideoDatasetDownloader::cancelDownload() {
    shouldCancel.store(true);
}

VideoDatasetProgress VideoDatasetDownloader::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void VideoDatasetDownloader::setProgressCallback(std::function<void(const VideoDatasetProgress&)> callback) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progressCallback = std::move(callback);
}

int VideoDatasetDownloader::countExistingSequences(const std::string& outputDir) {
    if (!fs::exists(outputDir)) {
        return 0;
    }
    int count = 0;
    for (const auto& entry : fs::directory_iterator(outputDir)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            if (name.rfind("seq_", 0) == 0) {
                count++;
            }
        }
    }
    return count;
}

void VideoDatasetDownloader::updateProgress(const VideoDatasetProgress& newProgress) {
    std::lock_guard<std::mutex> lock(progressMutex);
    progress = newProgress;
    if (progressCallback) {
        progressCallback(progress);
    }
}

std::string VideoDatasetDownloader::getVideoIdHash(int videoId) {
    // Simple hash for folder naming
    std::stringstream ss;
    ss << std::hex << (videoId * 2654435761 % 0xFFFFFFFF);
    return ss.str().substr(0, 8);
}

// ============================================================================
// HTTP Helpers
// ============================================================================

std::string VideoDatasetDownloader::httpGet(const std::string& url, const std::string& authHeader) {
#ifdef _WIN32
    std::wstring wurl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
        return "";
    }

    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-VideoDataset/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        return "";
    }

    INTERNET_PORT port = urlComp.nPort;
    if (port == 0) {
        port = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                             NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Add authorization header if provided
    if (!authHeader.empty()) {
        std::wstring wauthHeader(authHeader.begin(), authHeader.end());
        WinHttpAddRequestHeaders(hRequest, wauthHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::string result;
    char buffer[8192];
    DWORD bytesRead = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
#else
    return "";
#endif
}

bool VideoDatasetDownloader::downloadFile(const std::string& url, const std::string& localPath) {
#ifdef _WIN32
    std::wstring wurl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {0};
    wchar_t urlPath[2048] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp)) {
        return false;
    }

    // Create parent directories
    fs::path localFilePath(localPath);
    if (localFilePath.has_parent_path()) {
        fs::create_directories(localFilePath.parent_path());
    }

    HINTERNET hSession = WinHttpOpen(L"EWOCvj2-VideoDataset/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        return false;
    }

    // Set longer timeout for video downloads
    DWORD timeout = 120000; // 2 minutes
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    INTERNET_PORT port = urlComp.nPort;
    if (port == 0) {
        port = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                             NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Enable redirects
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Check status
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        NULL, &statusCode, &statusCodeSize, NULL);

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Open output file
    std::ofstream outFile(localPath, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Download data
    char buffer[65536];
    DWORD bytesRead = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (shouldCancel.load()) {
            outFile.close();
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            fs::remove(localPath);
            return false;
        }
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
#else
    return false;
#endif
}

// ============================================================================
// Pexels API
// ============================================================================

std::vector<std::pair<int, std::string>> VideoDatasetDownloader::searchPexelsVideos(const std::string& query, int perPage) {
    std::vector<std::pair<int, std::string>> results;

    // URL encode the query
    std::string encodedQuery;
    for (char c : query) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedQuery += c;
        } else if (c == ' ') {
            encodedQuery += "%20";
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
            encodedQuery += hex;
        }
    }

    std::string url = "https://api.pexels.com/videos/search?query=" + encodedQuery + "&per_page=" + std::to_string(perPage);
    std::string authHeader = "Authorization: " + currentConfig.pexelsApiKey;

    std::string response = httpGet(url, authHeader);
    if (response.empty()) {
        return results;
    }

    try {
        json data = json::parse(response);
        auto videos = data["videos"];

        for (const auto& video : videos) {
            int videoId = video["id"].get<int>();

            // Find best video file (prefer 720p-1080p)
            std::string bestUrl;
            int bestHeight = 0;

            for (const auto& vf : video["video_files"]) {
                int height = vf.value("height", 0);
                std::string link = vf.value("link", "");

                if (!link.empty() && height >= 720) {
                    // Prefer height closest to 1080
                    if (bestUrl.empty() || std::abs(height - 1080) < std::abs(bestHeight - 1080)) {
                        bestUrl = link;
                        bestHeight = height;
                    }
                }
            }

            // Fallback to any video file
            if (bestUrl.empty()) {
                for (const auto& vf : video["video_files"]) {
                    std::string link = vf.value("link", "");
                    if (!link.empty()) {
                        bestUrl = link;
                        break;
                    }
                }
            }

            if (!bestUrl.empty()) {
                results.push_back({videoId, bestUrl});
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[VideoDatasetDownloader] JSON parse error: " << e.what() << std::endl;
    }

    return results;
}

// ============================================================================
// Video Processing with FFmpeg
// ============================================================================

int VideoDatasetDownloader::extractFramePairs(const std::string& videoPath, const std::string& videoHash,
                                               int startSeqNum, const VideoDatasetConfig& config) {
    int sequencesCreated = 0;

    // Open video file
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[VideoDatasetDownloader] Failed to open video: " << videoPath << std::endl;
        return 0;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        avformat_close_input(&formatCtx);
        return 0;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx < 0) {
        avformat_close_input(&formatCtx);
        return 0;
    }

    AVStream* videoStream = formatCtx->streams[videoStreamIdx];
    AVCodecParameters* codecParams = videoStream->codecpar;

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        avformat_close_input(&formatCtx);
        return 0;
    }

    // Create decoder context
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return 0;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return 0;
    }

    // Get video info
    double fps = av_q2d(videoStream->r_frame_rate);
    if (fps <= 0) fps = 30.0;

    double duration = 0;
    if (videoStream->duration > 0) {
        duration = videoStream->duration * av_q2d(videoStream->time_base);
    } else if (formatCtx->duration > 0) {
        duration = formatCtx->duration / (double)AV_TIME_BASE;
    }

    if (duration < 2.0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return 0;
    }

    // Calculate extraction points (skip first/last 10%)
    double startTime = duration * 0.1;
    double endTime = duration * 0.9;
    double usableDuration = endTime - startTime;

    int numPairs = std::min(config.sequencesPerVideo, (int)(usableDuration * fps / 10));
    if (numPairs < 1) numPairs = 1;

    double timeInterval = usableDuration / numPairs;

    // Create scaler for output resolution
    SwsContext* swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        config.targetResolution, config.targetResolution, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return 0;
    }

    // Allocate frames
    AVFrame* frame = av_frame_alloc();
    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = config.targetResolution;
    rgbFrame->height = config.targetResolution;
    av_image_alloc(rgbFrame->data, rgbFrame->linesize, config.targetResolution, config.targetResolution, AV_PIX_FMT_RGB24, 32);

    AVPacket* packet = av_packet_alloc();

    // Extract frame pairs
    for (int i = 0; i < numPairs && !shouldCancel.load(); i++) {
        double timePos = startTime + (i * timeInterval);
        int64_t seekTarget = (int64_t)(timePos * AV_TIME_BASE);

        // Create sequence directory
        int seqNum = startSeqNum + sequencesCreated;
        std::string seqDir = config.outputDir + "/seq_" + videoHash + "_" + std::to_string(seqNum);

        if (fs::exists(seqDir)) {
            continue;
        }

        fs::create_directories(seqDir);

        // Seek to position
        av_seek_frame(formatCtx, -1, seekTarget, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);

        // Decode frames
        std::vector<std::vector<uint8_t>> decodedFrames;
        int framesNeeded = 2;

        while (decodedFrames.size() < framesNeeded && av_read_frame(formatCtx, packet) >= 0) {
            if (packet->stream_index == videoStreamIdx) {
                if (avcodec_send_packet(codecCtx, packet) >= 0) {
                    while (avcodec_receive_frame(codecCtx, frame) >= 0) {
                        // Scale to target resolution
                        sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                                  rgbFrame->data, rgbFrame->linesize);

                        // Copy frame data
                        int dataSize = config.targetResolution * config.targetResolution * 3;
                        std::vector<uint8_t> frameData(dataSize);
                        for (int y = 0; y < config.targetResolution; y++) {
                            memcpy(frameData.data() + y * config.targetResolution * 3,
                                   rgbFrame->data[0] + y * rgbFrame->linesize[0],
                                   config.targetResolution * 3);
                        }
                        decodedFrames.push_back(std::move(frameData));

                        if (decodedFrames.size() >= framesNeeded) break;
                    }
                }
            }
            av_packet_unref(packet);
            if (decodedFrames.size() >= framesNeeded) break;
        }

        // Save frames if we got enough
        if (decodedFrames.size() >= 2) {
            std::string frame0Path = seqDir + "/frame_0000.jpg";
            std::string frame1Path = seqDir + "/frame_0001.jpg";

            bool saved0 = saveFrameAsJpeg(frame0Path, decodedFrames[0].data(),
                                          config.targetResolution, config.targetResolution, config.jpegQuality);
            bool saved1 = saveFrameAsJpeg(frame1Path, decodedFrames[1].data(),
                                          config.targetResolution, config.targetResolution, config.jpegQuality);

            if (saved0 && saved1) {
                sequencesCreated++;
            } else {
                fs::remove_all(seqDir);
            }
        } else {
            fs::remove_all(seqDir);
        }
    }

    // Cleanup
    av_packet_free(&packet);
    av_freep(&rgbFrame->data[0]);
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    return sequencesCreated;
}

// ============================================================================
// JPEG Saving with turbojpeg
// ============================================================================

bool VideoDatasetDownloader::saveFrameAsJpeg(const std::string& path, uint8_t* rgbData, int width, int height, int quality) {
    tjhandle jpegCompressor = tjInitCompress();
    if (!jpegCompressor) {
        return false;
    }

    unsigned char* compressedImage = nullptr;
    unsigned long jpegSize = 0;

    int result = tjCompress2(jpegCompressor, rgbData, width, 0, height, TJPF_RGB,
                             &compressedImage, &jpegSize, TJSAMP_444, quality, TJFLAG_FASTDCT);

    tjDestroy(jpegCompressor);

    if (result != 0 || !compressedImage) {
        if (compressedImage) tjFree(compressedImage);
        return false;
    }

    std::ofstream outfile(path, std::ios::binary | std::ios::trunc);
    if (!outfile) {
        tjFree(compressedImage);
        return false;
    }

    outfile.write(reinterpret_cast<const char*>(compressedImage), jpegSize);
    outfile.close();

    tjFree(compressedImage);
    return true;
}

// ============================================================================
// Download Thread
// ============================================================================

void VideoDatasetDownloader::downloadThreadFunc(VideoDatasetConfig config) {
    VideoDatasetProgress prog;
    prog.status = "Video Frame Pairs: Starting...";
    prog.sequencesTotal = config.targetSequences;
    updateProgress(prog);

    // Create output directory
    fs::create_directories(config.outputDir);
    std::string tempDir = config.outputDir + "/_temp_videos";
    fs::create_directories(tempDir);

    // Check existing sequences
    int existing = countExistingSequences(config.outputDir);
    prog.sequencesCompleted = existing;
    prog.status = "Video Frame Pairs: " + std::to_string(existing) + " sequences exist, target is " + std::to_string(config.targetSequences);
    updateProgress(prog);

    if (existing >= config.targetSequences) {
        prog.status = "Video Frame Pairs: Already complete (" + std::to_string(existing) + " sequences)";
        prog.percentComplete = 100.0f;
        updateProgress(prog);
        downloading.store(false);
        return;
    }

    int needed = config.targetSequences - existing;
    prog.status = "Video Frame Pairs: Need " + std::to_string(needed) + " more sequences";
    updateProgress(prog);

    // Shuffle queries for variety
    std::vector<std::string> queries = SEARCH_QUERIES;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(queries.begin(), queries.end(), g);

    int totalCreated = 0;
    std::set<int> usedVideoIds;

    for (const auto& query : queries) {
        if (totalCreated >= needed || shouldCancel.load()) {
            break;
        }

        std::cerr << "[VideoDatasetDownloader] Searching for '" << query << "'..." << std::endl;
        auto videos = searchPexelsVideos(query, 10);

        for (const auto& [videoId, videoUrl] : videos) {
            if (totalCreated >= needed || shouldCancel.load()) {
                break;
            }

            if (usedVideoIds.count(videoId)) {
                continue;
            }

            std::string videoHash = getVideoIdHash(videoId);
            std::string videoPath = tempDir + "/" + videoHash + ".mp4";

            try {
                std::cerr << "[VideoDatasetDownloader] Downloading video " << videoId << "..." << std::endl;

                if (!downloadFile(videoUrl, videoPath)) {
                    prog.videosFailed++;
                    continue;
                }

                usedVideoIds.insert(videoId);

                int sequences = extractFramePairs(videoPath, videoHash, totalCreated, config);

                if (sequences > 0) {
                    totalCreated += sequences;
                    prog.videosProcessed++;
                    std::cerr << "[VideoDatasetDownloader] Extracted " << sequences << " sequences from video " << videoId << std::endl;
                } else {
                    prog.videosFailed++;
                    std::cerr << "[VideoDatasetDownloader] Failed to extract frames from video " << videoId << std::endl;
                }

                // Clean up video file
                if (fs::exists(videoPath)) {
                    fs::remove(videoPath);
                }

            } catch (const std::exception& e) {
                prog.videosFailed++;
                std::cerr << "[VideoDatasetDownloader] Exception: " << e.what() << std::endl;
                if (fs::exists(videoPath)) {
                    try { fs::remove(videoPath); } catch (...) {}
                }
            }

            // Update progress
            prog.sequencesCompleted = existing + totalCreated;
            prog.percentComplete = (prog.sequencesCompleted * 100.0f) / config.targetSequences;
            prog.status = "Video Frame Pairs: " + std::to_string(prog.sequencesCompleted) + "/" +
                          std::to_string(config.targetSequences) + " sequences (" +
                          std::to_string(prog.videosProcessed) + " videos OK, " +
                          std::to_string(prog.videosFailed) + " failed)";
            updateProgress(prog);
        }
    }

    // Final status
    int finalCount = countExistingSequences(config.outputDir);
    if (finalCount >= config.targetSequences) {
        prog.status = "Video Frame Pairs: Complete! " + std::to_string(finalCount) + " total sequences";
    } else {
        prog.status = "Video Frame Pairs: Partial - " + std::to_string(finalCount) + "/" + std::to_string(config.targetSequences) + " sequences";
    }
    prog.sequencesCompleted = finalCount;
    prog.percentComplete = (finalCount * 100.0f) / config.targetSequences;
    updateProgress(prog);

    downloading.store(false);
}
