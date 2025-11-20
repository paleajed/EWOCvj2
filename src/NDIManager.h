#ifndef NDI_MANAGER_H
#define NDI_MANAGER_H

#include <Processing.NDI.Lib.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>

#ifdef _WIN32
#include <GL/glew.h>
#include <GL/wglew.h>
#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/glew.h>
#include <GL/glx.h>
#endif

// Forward declarations
class NDISource;
class NDIOutput;
class NDIManager;
class NDIYUVConverter;

// NDI Source information structure
struct NDISourceInfo {
    std::string name;
    std::string url;
    std::string ip_address;
    bool is_available = false;
    int width;
    int height;
    double fps;
    std::string pixel_format;

    NDISourceInfo() : is_available(false), width(0), height(0), fps(0.0) {}
};


// OpenGL texture wrapper for NDI frames
class NDITexture {
public:
    NDITexture();
    ~NDITexture();

    // Upload frame data to OpenGL texture
    bool uploadFrame(const NDIlib_video_frame_v2_t& frame);

    // Create empty texture with specified dimensions
    bool createTexture(int width, int height, GLenum format = GL_RGBA);

    // Create from existing OpenGL texture
    bool setFromExistingTexture(GLuint texture_id, int width, int height, GLenum format = GL_RGBA);

    // Download texture data for NDI output
    bool downloadToFrame(NDIlib_video_frame_v2_t& frame);

    // Getters
    GLuint getTextureID() const { return texture_id_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    GLenum getFormat() const { return format_; }

    // Utility methods
    void bind(GLenum texture_unit = GL_TEXTURE0) const;
    void unbind() const;
    bool isValid() const { return texture_id_ != 0; }

    // Async download methods
    bool startAsyncDownload();
    bool getDownloadedData(std::vector<uint8_t>& data);

private:
    GLuint texture_id_;
    GLuint pbo_upload_;   // Pixel buffer object for async uploads (legacy)
    GLuint pbo_download_; // Pixel buffer object for async downloads
    GLuint upload_pbos_[2]; // Double-buffered PBOs for async upload
    int upload_pbo_index_;  // Current PBO index for upload
    int upload_pbo_allocated_size_;  // Actual allocated size of upload PBOs
    int width_, height_;
    GLenum format_;
    GLenum internal_format_;

    struct PBOPair {
        GLuint pbo[2];
        int current_index;
        GLsync fence[2];
        bool ready[2] = {false};
    } download_pbos_;

    bool setupDownloadPBOs();
    void cleanupDownloadPBOs();

    void cleanup();
    bool setupPixelBuffers();
    GLenum determineOpenGLFormat(NDIlib_FourCC_video_type_e ndi_format);
};

// NDI Input Source
class NDISource {
public:
    NDISource(const std::string& source_name);
    ~NDISource();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }

    // Reference-counted disconnect (only disconnects when ref count reaches 0)
    void releaseReference();  // Decrements ref count, disconnects if 0

    // Utility
    int getCurrentRefCount() const;

    // Frame retrieval
    bool getLatestFrame(NDITexture& texture);
    bool hasNewFrame() const { return has_new_frame_; }

    // Information
    const NDISourceInfo& getSourceInfo() const { return source_info_; }

    // Settings
    void setLowLatencyMode(bool enabled);
    void setBufferSize(int frames) { buffer_size_ = frames; }

    // Callbacks
    void setFrameCallback(std::function<void(const NDITexture&)> callback);
    void setConnectionCallback(std::function<void(bool)> callback);

private:
    std::string source_name_;
    NDISourceInfo source_info_;

    NDIlib_recv_instance_t ndi_recv_;
    NDIlib_source_t ndi_source_;

    std::thread receiver_thread_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::atomic<bool> has_new_frame_;

    std::mutex frame_mutex_;
    NDIlib_video_frame_v2_t current_frame_;
    bool frame_valid_;

    int buffer_size_;
    bool low_latency_mode_;

    std::function<void(const NDITexture&)> frame_callback_;
    std::function<void(bool)> connection_callback_;

    void receiverLoop();
    void cleanup();
};

// NDI Output Stream
class NDIOutput {
public:
    NDIOutput(const std::string& output_name, int width, int height, double fps = 30.0);
    ~NDIOutput();

    // Stream management
    bool startStream();
    void stopStream();
    bool isStreaming() const { return streaming_; }

    // Frame sending
    bool sendFrame(const NDITexture& texture);
    bool sendFrame(const NDIlib_video_frame_v2_t& frame);

    // Settings
    void setQuality(int quality); // 0-100
    void setFormat(const std::string& format);
    void setMetadata(const std::map<std::string, std::string>& metadata);

    void setMaxBufferedFrames(int count) { max_buffered_frames_ = count; }

    void setGPUConversion(bool enabled) { use_gpu_conversion_ = enabled; }
    bool isGPUConversionEnabled() const { return use_gpu_conversion_; }

    // Set target NDI frame rate (independent of VJ program frame rate)
    void setTargetFrameRate(double fps) {
        target_fps_ = fps;
        target_interval_ = std::chrono::microseconds(static_cast<int64_t>(1000000.0 / fps));
    }

    double getTargetFrameRate() const { return target_fps_; }
    double getActualFrameRate() const { return frame_stats_.actual_fps; }

private:
    std::string output_name_;
    int width_, height_;
    double fps_;

    NDIlib_send_instance_t ndi_send_;

    std::atomic<bool> streaming_;

    std::mutex send_mutex_;
    int64_t frame_counter_;
    std::vector<uint8_t> frame_buffer_;  // Buffer for texture data

    struct FrameBuffer {
        std::vector<uint8_t> data;
        int width, height;
        std::chrono::steady_clock::time_point timestamp;
        bool valid;

        FrameBuffer() : width(0), height(0), valid(false) {}
    };

    struct PBODownloader {
        GLuint pbo[3];  // Triple buffering for PBOs
        GLsync fence[3];
        FrameBuffer frames[3];
        int write_index;    // Where to start next download
        int read_index;     // Where to read completed frames
        int pending_count;  // Number of downloads in progress

        PBODownloader() : write_index(0), read_index(0), pending_count(0) {
            for (int i = 0; i < 3; i++) {
                pbo[i] = 0;
                fence[i] = nullptr;
            }
        }
    } pbo_downloader_;

    // Frame buffer queue for temporal smoothing
    std::queue<FrameBuffer> ready_frames_;
    std::mutex frame_queue_mutex_;
    int max_buffered_frames_;

    bool setupPBODownloader(int width, int height);
    void cleanupPBODownloader();
    bool startAsyncTextureDownload(const NDITexture& texture);
    bool processCompletedDownloads();
    bool getBufferedFrame(std::vector<uint8_t>& data, int& width, int& height);

    std::unique_ptr<NDIYUVConverter> yuv_converter_;
    NDITexture uyvy_texture_;
    bool use_gpu_conversion_;

    double target_fps_;
    std::chrono::microseconds target_interval_;
    std::chrono::steady_clock::time_point last_send_time_;

    // Frame rate statistics
    struct FrameRateStats {
        int frames_sent;
        std::chrono::steady_clock::time_point stats_start_time;
        double actual_fps;

        FrameRateStats() : frames_sent(0), actual_fps(0.0) {
            stats_start_time = std::chrono::steady_clock::now();
        }
    } frame_stats_;

    bool initializeSender();
    void cleanup();
};

// Main NDI Manager
class NDIManager {
public:
    static NDIManager& getInstance();

    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }

    // Source discovery
    std::vector<NDISourceInfo> discoverSources();
    void startSourceDiscovery(std::function<void(const std::vector<NDISourceInfo>&)> callback);
    void stopSourceDiscovery();

    // Direct access to current sources (no waiting)
    const NDIlib_source_t* getCurrentSources(uint32_t& num_sources) const;

    // Source management
    std::shared_ptr<NDISource> createSource(const std::string& source_name);
    void removeSource(const std::string& source_name);
    std::shared_ptr<NDISource> getSource(const std::string& source_name);
    std::vector<std::shared_ptr<NDISource>> getActiveSources();

    // Reference counting
    int getSourceRefCount(const std::string& source_name) const;
    std::map<std::string, int> getAllSourceRefCounts() const;

    // Output management
    std::shared_ptr<NDIOutput> createOutput(const std::string& output_name,
                                            int width, int height, double fps = 30.0);
    void removeOutput(const std::string& output_name);
    std::shared_ptr<NDIOutput> getOutput(const std::string& output_name);
    std::vector<std::shared_ptr<NDIOutput>> getActiveOutputs();

    void setGlobalGPUConversion(bool enabled);

    // Global settings
    void setGlobalLowLatencyMode(bool enabled);
    void setDiscoveryInterval(int seconds);

    // Network settings
    void setNetworkInterface(const std::string& interface_name);
    void setMulticastGroup(const std::string& group);

    // Utility methods
    static std::string getVersionString();
    static std::vector<std::string> getSupportedFormats();
    void printSourceUsage() const;

    // Cleanup methods
    void disconnectAllSources();
    void stopAllOutputs();

private:
    NDIManager();
    ~NDIManager();

    // Singleton pattern
    NDIManager(const NDIManager&) = delete;
    NDIManager& operator=(const NDIManager&) = delete;

    bool initialized_;
    NDIlib_find_instance_t ndi_find_;

    std::map<std::string, std::shared_ptr<NDISource>> sources_;
    std::map<std::string, std::shared_ptr<NDIOutput>> outputs_;
    mutable std::mutex sources_mutex_;
    mutable std::mutex outputs_mutex_;

    // Reference counting for sources
    std::map<std::string, int> source_ref_counts_;
    mutable std::mutex ref_count_mutex_;

    // Source discovery
    std::thread discovery_thread_;
    std::atomic<bool> discovery_running_;
    int discovery_interval_;
    std::function<void(const std::vector<NDISourceInfo>&)> discovery_callback_;

    // Settings
    bool global_low_latency_;
    std::string network_interface_;
    std::string multicast_group_;

    void discoveryLoop();
    void cleanupInactiveSources();
    void cleanupInactiveOutputs();
};

// Utility functions
namespace NDIUtils {
    // Format conversion helpers
    std::string ndiFormatToString(NDIlib_FourCC_video_type_e format);
    NDIlib_FourCC_video_type_e stringToNDIFormat(const std::string& format);

    // OpenGL helpers
    bool checkGLError(const std::string& operation);
    std::string getGLErrorString(GLenum error);

    // Network helpers
    std::vector<std::string> getNetworkInterfaces();
    bool isValidIPAddress(const std::string& ip);

    // Performance helpers
    double calculateFPS(const std::vector<std::chrono::steady_clock::time_point>& timestamps);
    void logPerformanceMetrics(const std::string& operation,
                               std::chrono::steady_clock::time_point start,
                               std::chrono::steady_clock::time_point end);
}

// Exception classes
class NDIException : public std::exception {
public:
    NDIException(const std::string& message) : message_(message) {}
    virtual const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

class NDIConnectionException : public NDIException {
public:
    NDIConnectionException(const std::string& source, const std::string& reason)
            : NDIException("NDI Connection failed for '" + source + "': " + reason) {}
};

class NDIStreamException : public NDIException {
public:
    NDIStreamException(const std::string& stream, const std::string& reason)
            : NDIException("NDI Stream error for '" + stream + "': " + reason) {}
};




class NDIYUVConverter {
public:
    NDIYUVConverter();
    ~NDIYUVConverter();

    bool initialize();
    void cleanup();

    // Convert RGBA texture to UYVY format on GPU
    bool convertToUYVY(const NDITexture& rgba_texture, NDITexture& uyvy_texture);

    // Check if converter is ready
    bool isInitialized() const { return initialized_; }

private:
    GLuint shader_program_;
    GLuint vertex_shader_;
    GLuint fragment_shader_;
    GLuint vao_;
    GLuint vbo_;
    GLuint framebuffer_;
    bool initialized_;

    // Shader uniform locations
    GLint u_texture_location_;
    GLint u_texel_size_location_;

    GLuint previous_program_;  // Store previous shader program

    bool compileShader(GLuint shader, const char* source);
    bool linkProgram();
    void setupQuadGeometry();
};

#endif // NDI_MANAGER_H