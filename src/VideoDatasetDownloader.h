/**
 * VideoDatasetDownloader.h
 *
 * Downloads videos from Pexels API and extracts frame pairs for temporal training.
 * Uses FFmpeg C API for video decoding and libjpeg-turbo for JPEG encoding.
 *
 * License: GPL3
 */

#ifndef VIDEO_DATASET_DOWNLOADER_H
#define VIDEO_DATASET_DOWNLOADER_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

/**
 * Progress information for video dataset download
 */
struct VideoDatasetProgress {
    std::string status;
    int sequencesCompleted = 0;
    int sequencesTotal = 0;
    int videosProcessed = 0;
    int videosFailed = 0;
    float percentComplete = 0.0f;
};

/**
 * Configuration for video dataset download
 */
struct VideoDatasetConfig {
    std::string outputDir = "C:/ProgramData/EWOCvj2/datasets/video";
    std::string pexelsApiKey = "YJApZD6dShnhYgWIvD9WCC3OB18anYV7Zg4Bio0UVES92e3oCW4xsP2e";
    int targetSequences = 500;      // Total frame pairs to collect
    int sequencesPerVideo = 20;     // Max frame pairs per video
    int targetResolution = 1024;    // Output frame size (square)
    int jpegQuality = 90;           // JPEG quality (1-100)
};

/**
 * VideoDatasetDownloader - Downloads videos and extracts frame pairs for training
 */
class VideoDatasetDownloader {
public:
    VideoDatasetDownloader();
    ~VideoDatasetDownloader();

    // Prevent copying
    VideoDatasetDownloader(const VideoDatasetDownloader&) = delete;
    VideoDatasetDownloader& operator=(const VideoDatasetDownloader&) = delete;

    /**
     * Start downloading and extracting frame pairs
     * @param config Download configuration
     * @return true if started successfully
     */
    bool startDownload(const VideoDatasetConfig& config);

    /**
     * Cancel ongoing download
     */
    void cancelDownload();

    /**
     * Check if download is in progress
     */
    bool isDownloading() const { return downloading.load(); }

    /**
     * Get current progress
     */
    VideoDatasetProgress getProgress() const;

    /**
     * Set progress callback
     */
    void setProgressCallback(std::function<void(const VideoDatasetProgress&)> callback);

    /**
     * Count existing sequence directories
     */
    static int countExistingSequences(const std::string& outputDir);

private:
    // State
    std::atomic<bool> downloading{false};
    std::atomic<bool> shouldCancel{false};
    VideoDatasetConfig currentConfig;

    // Threading
    std::unique_ptr<std::thread> downloadThread;

    // Progress
    mutable std::mutex progressMutex;
    VideoDatasetProgress progress;
    std::function<void(const VideoDatasetProgress&)> progressCallback;

    // Search queries for video variety
    static const std::vector<std::string> SEARCH_QUERIES;

    // Download thread
    void downloadThreadFunc(VideoDatasetConfig config);

    // HTTP helpers
    std::string httpGet(const std::string& url, const std::string& authHeader = "");
    bool downloadFile(const std::string& url, const std::string& localPath);

    // Pexels API
    std::vector<std::pair<int, std::string>> searchPexelsVideos(const std::string& query, int perPage = 10);
    std::string getBestVideoUrl(const std::string& jsonResponse, int videoIndex);

    // Video processing with FFmpeg
    int extractFramePairs(const std::string& videoPath, const std::string& videoHash,
                          int startSeqNum, const VideoDatasetConfig& config);

    // JPEG saving with turbojpeg
    bool saveFrameAsJpeg(const std::string& path, uint8_t* rgbData, int width, int height, int quality);

    // Progress update
    void updateProgress(const VideoDatasetProgress& newProgress);

    // Utility
    std::string getVideoIdHash(int videoId);
};

#endif // VIDEO_DATASET_DOWNLOADER_H
