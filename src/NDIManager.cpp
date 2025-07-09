#include <cstddef>
#include "NDIManager.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cstring>

// ============================================================================
// NDITexture Implementation
// ============================================================================

NDITexture::NDITexture()
        : texture_id_(0), pbo_upload_(0), pbo_download_(0),
          width_(0), height_(0), format_(GL_RGBA), internal_format_(GL_RGBA8) {
}

NDITexture::~NDITexture() {
    cleanup();
}

bool NDITexture::uploadFrame(const NDIlib_video_frame_v2_t& frame) {
    if (!frame.p_data || frame.xres <= 0 || frame.yres <= 0) {
        return false;
    }

    // Create texture if it doesn't exist or dimensions changed
    if (texture_id_ == 0 || width_ != frame.xres || height_ != frame.yres) {
        if (!createTexture(frame.xres, frame.yres)) {
            return false;
        }
    }

    // Upload frame data to texture
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // Determine format based on NDI FourCC
    GLenum gl_format = GL_RGBA;
    GLenum gl_type = GL_UNSIGNED_BYTE;

    switch (frame.FourCC) {
        case NDIlib_FourCC_video_type_RGBA:
            gl_format = GL_RGBA;
            break;
        case NDIlib_FourCC_video_type_RGBX:
            gl_format = GL_RGB;
            break;
        case NDIlib_FourCC_video_type_BGRA:
            gl_format = GL_BGRA;
            break;
        case NDIlib_FourCC_video_type_BGRX:
            gl_format = GL_BGR;
            break;
        default:
            // Default to RGBA
            gl_format = GL_RGBA;
            break;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, gl_format, gl_type, frame.p_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return NDIUtils::checkGLError("uploadFrame");
}

bool NDITexture::createTexture(int width, int height, GLenum format) {
    cleanup();

    width_ = width;
    height_ = height;
    format_ = format;

    // Determine internal format
    switch (format) {
        case GL_RGB:
            internal_format_ = GL_RGB8;
            break;
        case GL_RGBA:
            internal_format_ = GL_RGBA8;
            break;
        case GL_BGR:
            internal_format_ = GL_RGB8;
            break;
        case GL_BGRA:
            internal_format_ = GL_RGBA8;
            break;
        default:
            internal_format_ = GL_RGBA8;
            format_ = GL_RGBA;
    }

    // Create OpenGL texture
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format_, width_, height_, 0, format_, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return NDIUtils::checkGLError("createTexture");
}

bool NDITexture::setFromExistingTexture(GLuint texture_id, int width, int height, GLenum format) {
    // Clean up any existing texture
    if (texture_id_ != 0 && texture_id_ != texture_id) {
        glDeleteTextures(1, &texture_id_);
    }

    // Set the new texture properties
    texture_id_ = texture_id;
    width_ = width;
    height_ = height;
    format_ = format;

    // Don't delete this texture in cleanup since we don't own it
    // You might want to add a flag to track ownership

    return texture_id != 0;
}

void NDITexture::bind(GLenum texture_unit) const {
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void NDITexture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void NDITexture::cleanup() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }

    if (pbo_upload_ != 0) {
        glDeleteBuffers(1, &pbo_upload_);
        pbo_upload_ = 0;
    }

    if (pbo_download_ != 0) {
        glDeleteBuffers(1, &pbo_download_);
        pbo_download_ = 0;
    }
}

bool NDITexture::setupDownloadPBOs() {
    if (!isValid()) return false;

    int frame_size = width_ * height_ * 4; // RGBA

    glGenBuffers(2, download_pbos_.pbo);

    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, download_pbos_.pbo[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, frame_size, nullptr, GL_STREAM_READ);
        download_pbos_.fence[i] = nullptr;
        download_pbos_.ready[i] = false;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    download_pbos_.current_index = 0;

    return NDIUtils::checkGLError("setupDownloadPBOs");
}

bool NDITexture::startAsyncDownload() {
    if (!isValid()) return false;

    int index = download_pbos_.current_index;

    // Check if previous transfer for this buffer is complete
    if (download_pbos_.fence[index]) {
        GLint result;
        glGetSynciv(download_pbos_.fence[index], GL_SYNC_STATUS, sizeof(result), nullptr, &result);
        if (result != GL_SIGNALED) {
            return false; // Previous transfer still in progress
        }
        glDeleteSync(download_pbos_.fence[index]);
        download_pbos_.fence[index] = nullptr;
    }

    // Start async download
    glBindBuffer(GL_PIXEL_PACK_BUFFER, download_pbos_.pbo[index]);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // This call returns immediately with PBO bound
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create fence to track completion
    download_pbos_.fence[index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    download_pbos_.ready[index] = false;

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Switch to other buffer for next frame
    download_pbos_.current_index = (index + 1) % 2;

    return NDIUtils::checkGLError("startAsyncDownload");
}

bool NDITexture::getDownloadedData(std::vector<uint8_t>& data) {
    int prev_index = (download_pbos_.current_index + 1) % 2; // Previous buffer

    if (!download_pbos_.fence[prev_index]) {
        return false; // No transfer started
    }

    // Check if transfer is complete
    GLint result;
    glGetSynciv(download_pbos_.fence[prev_index], GL_SYNC_STATUS, sizeof(result), nullptr, &result);
    if (result != GL_SIGNALED) {
        return false; // Transfer not complete yet
    }

    // Transfer is complete, map buffer and copy data
    glBindBuffer(GL_PIXEL_PACK_BUFFER, download_pbos_.pbo[prev_index]);

    void* mapped_data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (mapped_data) {
        int frame_size = width_ * height_ * 4;
        data.resize(frame_size);
        memcpy(data.data(), mapped_data, frame_size);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

        // Cleanup fence
        glDeleteSync(download_pbos_.fence[prev_index]);
        download_pbos_.fence[prev_index] = nullptr;
        download_pbos_.ready[prev_index] = true;

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        return true;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return false;
}

void NDITexture::cleanupDownloadPBOs() {
    for (int i = 0; i < 2; i++) {
        if (download_pbos_.fence[i]) {
            glDeleteSync(download_pbos_.fence[i]);
            download_pbos_.fence[i] = nullptr;
        }
    }
    glDeleteBuffers(2, download_pbos_.pbo);
}




// ============================================================================
// NDISource Implementation
// ============================================================================

NDISource::NDISource(const std::string& source_name)
        : source_name_(source_name), ndi_recv_(nullptr), connected_(false), running_(false),
          has_new_frame_(false), frame_valid_(false), buffer_size_(3), low_latency_mode_(false) {

    source_info_.name = source_name;
    memset(&ndi_source_, 0, sizeof(ndi_source_));
    memset(&current_frame_, 0, sizeof(current_frame_));
}

NDISource::~NDISource() {
    // Auto-release reference on destruction (optional)
    // Uncomment if you want automatic reference management
    // releaseReference();

    disconnect();
}

bool NDISource::connect() {
    if (connected_) {
        return true;
    }

    cleanup();

    // Find the source first
    NDIManager& manager = NDIManager::getInstance();
    auto sources = manager.discoverSources();

    bool found = false;
    for (const auto& source_info : sources) {
        if (source_info.name == source_name_) {
            ndi_source_.p_ndi_name = source_name_.c_str();
            ndi_source_.p_url_address = source_info.url.empty() ? nullptr : source_info.url.c_str();
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "NDI source '" << source_name_ << "' not found" << std::endl;
        return false;
    }

    // Create receiver
    NDIlib_recv_create_v3_t recv_desc = {};
    recv_desc.source_to_connect_to = ndi_source_;
    recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
    recv_desc.allow_video_fields = false;

    ndi_recv_ = NDIlib_recv_create_v3(&recv_desc);
    if (!ndi_recv_) {
        std::cerr << "Failed to create NDI receiver" << std::endl;
        cleanup();
        return false;
    }

    connected_ = true;
    running_ = true;
    receiver_thread_ = std::thread(&NDISource::receiverLoop, this);

    std::cout << "Connected to NDI source: " << source_name_ << std::endl;

    if (connection_callback_) {
        connection_callback_(true);
    }

    return true;
}

void NDISource::disconnect() {
    if (!connected_) {
        return;
    }

    running_ = false;

    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }

    cleanup();
    connected_ = false;

    if (connection_callback_) {
        connection_callback_(false);
    }
}

bool NDISource::getLatestFrame(NDITexture& texture) {
    if (!connected_ || !has_new_frame_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(frame_mutex_);

    if (frame_valid_) {
        bool success = texture.uploadFrame(current_frame_);
        if (success) {
            has_new_frame_ = false;

            if (frame_callback_) {
                frame_callback_(texture);
            }
        }
        return success;
    }

    return false;
}

void NDISource::receiverLoop() {
    while (running_) {
        NDIlib_video_frame_v2_t video_frame = {};

        switch (NDIlib_recv_capture_v2(ndi_recv_, &video_frame, nullptr, nullptr, 0)) {
            case NDIlib_frame_type_video:
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);

                // Free previous frame if exists
                if (frame_valid_) {
                    NDIlib_recv_free_video_v2(ndi_recv_, &current_frame_);
                }

                // Store new frame
                current_frame_ = video_frame;
                frame_valid_ = true;
                has_new_frame_ = true;
                updateStats();

                // Don't free here - we'll free when we get the next frame
            }
                break;

            case NDIlib_frame_type_none:
                // No new frame, continue
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                break;

            default:
                // Other frame types (audio, metadata) - ignore for now
                break;
        }
    }
}

void NDISource::updateStats() {
    auto now = std::chrono::steady_clock::now();
    stats_.frames_received++;

    if (stats_.last_frame_time.time_since_epoch().count() > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                now - stats_.last_frame_time).count();

        if (duration > 0) {
            stats_.current_fps = 1000000.0 / duration;
        }
    }

    stats_.last_frame_time = now;
}

void NDISource::setLowLatencyMode(bool enabled) {
    low_latency_mode_ = enabled;
    // Note: This will take effect on next connection
}

void NDISource::setFrameCallback(std::function<void(const NDITexture&)> callback) {
    frame_callback_ = callback;
}

void NDISource::setConnectionCallback(std::function<void(bool)> callback) {
    connection_callback_ = callback;
}

void NDISource::releaseReference() {
    NDIManager& manager = NDIManager::getInstance();

    // Check current ref count before decrementing
    int current_refs = manager.getSourceRefCount(source_name_);

    if (current_refs <= 1) {
        // This is the last reference, disconnect before removing
        std::cout << "Last reference being released, disconnecting source: " << source_name_ << std::endl;
        disconnect();
    }

    // Now decrement ref count and potentially remove the source
    manager.removeSource(source_name_);
}

int NDISource::getCurrentRefCount() const {
    NDIManager& manager = NDIManager::getInstance();
    return manager.getSourceRefCount(source_name_);
}

void NDISource::cleanup() {
    if (frame_valid_ && ndi_recv_) {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        NDIlib_recv_free_video_v2(ndi_recv_, &current_frame_);
        frame_valid_ = false;
    }

    if (ndi_recv_) {
        NDIlib_recv_destroy(ndi_recv_);
        ndi_recv_ = nullptr;
    }
}






// ============================================================================
// NDIOutput Implementation
// ============================================================================

NDIOutput::NDIOutput(const std::string& output_name, int width, int height, double fps)
        : output_name_(output_name), width_(width), height_(height), fps_(fps),
          ndi_send_(nullptr), streaming_(false), frame_counter_(0),
          max_buffered_frames_(3), use_gpu_conversion_(true) {

    yuv_converter_ = std::make_unique<NDIYUVConverter>();
}

NDIOutput::~NDIOutput() {
    stopStream();
    cleanupPBODownloader();
}

bool NDIOutput::startStream() {
    if (streaming_) {
        return true;
    }

    cleanup();

    if (!initializeSender()) {
        cleanup();
        return false;
    }

    streaming_ = true;
    frame_counter_ = 0;

    return true;
}

void NDIOutput::stopStream() {
    if (!streaming_) {
        return;
    }

    streaming_ = false;
    cleanup();
}

bool NDIOutput::sendFrame(const NDITexture& texture) {
    if (!streaming_ || !texture.isValid()) {
        return false;
    }

    // Time-based frame rate limiting for 30fps NDI output
    static auto last_ndi_send_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();

    // Calculate time since last NDI frame (targeting 30fps = 33.33ms intervals)
    auto target_interval = std::chrono::microseconds(33333);  // 1/30 second
    auto elapsed = now - last_ndi_send_time;

    if (elapsed < target_interval) {
        // Too soon for next NDI frame - always process downloads though
        processCompletedDownloads();  // Keep buffer flowing
        return true;  // Return success but don't send frame
    }

    // Time to send a new NDI frame
    last_ndi_send_time = now;

    // Initialize GPU converter if needed (only once)
    if (use_gpu_conversion_ && !yuv_converter_->isInitialized()) {
        if (!yuv_converter_->initialize()) {
            std::cerr << "Failed to initialize GPU UYVY converter, falling back to RGBA" << std::endl;
            use_gpu_conversion_ = false;
        }
    }

    // Setup PBO downloader if needed (only once)
    if (pbo_downloader_.pbo[0] == 0) {
        int download_width = use_gpu_conversion_ ? texture.getWidth() / 2 : texture.getWidth();
        if (!setupPBODownloader(download_width, texture.getHeight())) {
            return false;
        }
    }

    // GPU conversion and async download (optimized path)
    if (use_gpu_conversion_) {
        // Fast GPU conversion (typically <1ms)
        if (!yuv_converter_->convertToUYVY(texture, uyvy_texture_)) {
            return false;  // Fail fast, no error logging in hot path
        }
        startAsyncTextureDownload(uyvy_texture_);
    } else {
        startAsyncTextureDownload(texture);
    }

    // Process completed downloads (non-blocking)
    processCompletedDownloads();

    // Try to get buffered frame (non-blocking)
    std::vector<uint8_t> frame_data;
    int frame_width, frame_height;

    if (!getBufferedFrame(frame_data, frame_width, frame_height)) {
        return true; // No frame ready, but that's okay
    }

    // Fast NDI frame setup (minimal lock time)
    {
        std::lock_guard<std::mutex> lock(send_mutex_);

        NDIlib_video_frame_v2_t ndi_frame = {};
        ndi_frame.xres = use_gpu_conversion_ ? frame_width * 2 : frame_width;
        ndi_frame.yres = frame_height;
        ndi_frame.frame_rate_N = 30;  // Fixed 30fps
        ndi_frame.frame_rate_D = 1;
        ndi_frame.p_data = frame_data.data();
        ndi_frame.line_stride_in_bytes = frame_width * 4;

        if (use_gpu_conversion_) {
            ndi_frame.FourCC = NDIlib_FourCC_video_type_UYVY;
        } else {
            ndi_frame.FourCC = NDIlib_FourCC_video_type_RGBA;
        }

        // Send to NDI (should be very fast with UYVY)
        NDIlib_send_send_video_v2(ndi_send_, &ndi_frame);

        frame_counter_++;
    }

    // Update stats less frequently
    if (frame_counter_ % 30 == 0) {  // Only every 30 frames
        updateStats();
    }

    return true;
}

bool NDIOutput::sendFrame(const NDIlib_video_frame_v2_t& frame) {
    if (!streaming_ || !ndi_send_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(send_mutex_);

    // Send frame to NDI
    NDIlib_send_send_video_v2(ndi_send_, &frame);

    frame_counter_++;
    updateStats();

    return true;
}

void NDIOutput::setQuality(int quality) {
    // Quality settings would be implemented here
    // NDI doesn't have direct quality settings like this
}

void NDIOutput::setFormat(const std::string& format) {
    // Format settings would be implemented here
}

void NDIOutput::setMetadata(const std::map<std::string, std::string>& metadata) {
    // Metadata settings would be implemented here
}

bool NDIOutput::initializeSender() {
    // Create sender
    NDIlib_send_create_t send_desc = {};
    send_desc.p_ndi_name = output_name_.c_str();

    ndi_send_ = NDIlib_send_create(&send_desc);
    if (!ndi_send_) {
        std::cerr << "Failed to create NDI sender" << std::endl;
        return false;
    }

    std::cout << "NDI sender created: " << output_name_ << std::endl;
    return true;
}

void NDIOutput::updateStats() {
    auto now = std::chrono::steady_clock::now();
    stats_.frames_received++;

    if (stats_.last_frame_time.time_since_epoch().count() > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                now - stats_.last_frame_time).count();

        if (duration > 0) {
            stats_.current_fps = 1000000.0 / duration;
        }
    }

    stats_.last_frame_time = now;
}

void NDIOutput::cleanup() {
    if (ndi_send_) {
        NDIlib_send_destroy(ndi_send_);
        ndi_send_ = nullptr;
    }
}

bool NDIOutput::setupPBODownloader(int width, int height) {
    int frame_size = width * height * 4; // RGBA

    glGenBuffers(3, pbo_downloader_.pbo);

    for (int i = 0; i < 3; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_downloader_.pbo[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, frame_size, nullptr, GL_STREAM_READ);
        pbo_downloader_.fence[i] = nullptr;
        pbo_downloader_.frames[i].data.resize(frame_size);
        pbo_downloader_.frames[i].width = width;
        pbo_downloader_.frames[i].height = height;
        pbo_downloader_.frames[i].valid = false;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    pbo_downloader_.write_index = 0;
    pbo_downloader_.read_index = 0;
    pbo_downloader_.pending_count = 0;
    max_buffered_frames_ = 3; // Default buffer size

    return NDIUtils::checkGLError("setupPBODownloader");
}

bool NDIOutput::startAsyncTextureDownload(const NDITexture& texture) {
    if (!texture.isValid()) return false;

    // Check if we have space for new download
    if (pbo_downloader_.pending_count >= 3) {
        return false; // All PBOs busy
    }

    int index = pbo_downloader_.write_index;

    // Start async download
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_downloader_.pbo[index]);
    texture.bind(GL_TEXTURE0);

    // This call returns immediately with PBO bound
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create fence to track completion
    pbo_downloader_.fence[index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    pbo_downloader_.frames[index].timestamp = std::chrono::steady_clock::now();
    pbo_downloader_.frames[index].valid = false;

    texture.unbind();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Move to next PBO
    pbo_downloader_.write_index = (index + 1) % 3;
    pbo_downloader_.pending_count++;

    return NDIUtils::checkGLError("startAsyncTextureDownload");
}

bool NDIOutput::processCompletedDownloads() {
    bool any_completed = false;

    // Check all pending downloads
    for (int i = 0; i < 3; i++) {
        if (pbo_downloader_.fence[i] && !pbo_downloader_.frames[i].valid) {
            // Check if transfer is complete
            GLint result;
            glGetSynciv(pbo_downloader_.fence[i], GL_SYNC_STATUS, sizeof(result), nullptr, &result);

            if (result == GL_SIGNALED) {
                // Transfer complete, copy data
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_downloader_.pbo[i]);

                void* mapped_data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
                if (mapped_data) {
                    int frame_size = pbo_downloader_.frames[i].width * pbo_downloader_.frames[i].height * 4;
                    memcpy(pbo_downloader_.frames[i].data.data(), mapped_data, frame_size);
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

                    pbo_downloader_.frames[i].valid = true;

                    // Add to ready queue
                    {
                        std::lock_guard<std::mutex> lock(frame_queue_mutex_);
                        ready_frames_.push(pbo_downloader_.frames[i]);

                        // Limit buffer size
                        while (ready_frames_.size() > static_cast<size_t>(max_buffered_frames_)) {
                            ready_frames_.pop();
                        }
                    }

                    any_completed = true;
                }

                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

                // Cleanup fence
                glDeleteSync(pbo_downloader_.fence[i]);
                pbo_downloader_.fence[i] = nullptr;
                pbo_downloader_.pending_count--;
            }
        }
    }

    return any_completed;
}

bool NDIOutput::getBufferedFrame(std::vector<uint8_t>& data, int& width, int& height) {
    std::lock_guard<std::mutex> lock(frame_queue_mutex_);

    if (ready_frames_.empty()) {
        return false;
    }

    FrameBuffer frame = ready_frames_.front();
    ready_frames_.pop();

    data = std::move(frame.data);
    width = frame.width;
    height = frame.height;

    return true;
}

void NDIOutput::cleanupPBODownloader() {
    for (int i = 0; i < 3; i++) {
        if (pbo_downloader_.fence[i]) {
            glDeleteSync(pbo_downloader_.fence[i]);
            pbo_downloader_.fence[i] = nullptr;
        }
    }
    glDeleteBuffers(3, pbo_downloader_.pbo);
}






// ============================================================================
// NDIManager Implementation
// ============================================================================

NDIManager::NDIManager()
        : initialized_(false), ndi_find_(nullptr), discovery_running_(false),
          discovery_interval_(5), global_low_latency_(false) {
    global_stats_.start_time = std::chrono::steady_clock::now();
}

NDIManager::~NDIManager() {
    std::cout << "NDIManager destructor called" << std::endl;
    shutdown();
}

NDIManager& NDIManager::getInstance() {
    static NDIManager instance;
    return instance;
}

bool NDIManager::initialize() {
    if (initialized_) {
        return true;
    }

    // Initialize NDI
    if (!NDIlib_initialize()) {
        std::cerr << "Failed to initialize NDI SDK" << std::endl;
        return false;
    }

    // Create finder
    NDIlib_find_create_t find_desc = {};
    find_desc.show_local_sources = true;
    find_desc.p_groups = nullptr;
    find_desc.p_extra_ips = nullptr;

    ndi_find_ = NDIlib_find_create_v2(&find_desc);
    if (!ndi_find_) {
        std::cerr << "Failed to create NDI finder" << std::endl;
        NDIlib_destroy();
        return false;
    }

    initialized_ = true;
    std::cout << "NDI SDK initialized successfully" << std::endl;
    std::cout << "NDI SDK Version: " << getVersionString() << std::endl;

    return true;
}

void NDIManager::setGlobalGPUConversion(bool enabled) {
    std::lock_guard<std::mutex> lock(outputs_mutex_);
    for (auto& pair : outputs_) {
        pair.second->setGPUConversion(enabled);
    }
}

void NDIManager::shutdown() {
    if (!initialized_) {
        return;
    }

    std::cout << "NDI shutdown: Cleaning up all streams..." << std::endl;

    // Stop source discovery first
    stopSourceDiscovery();

    // Clean up all sources - disconnect and clear references
    {
        std::lock_guard<std::mutex> sources_lock(sources_mutex_);
        std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);

        std::cout << "Disconnecting " << sources_.size() << " NDI sources..." << std::endl;

        for (auto& pair : sources_) {
            const std::string& name = pair.first;
            auto& source = pair.second;

            if (source && source->isConnected()) {
                std::cout << "  Disconnecting source: " << name << std::endl;
                source->disconnect();
            }
        }

        sources_.clear();
        source_ref_counts_.clear();
        std::cout << "All NDI sources cleaned up" << std::endl;
    }

    // Clean up all outputs - stop streaming
    {
        std::lock_guard<std::mutex> outputs_lock(outputs_mutex_);

        std::cout << "Stopping " << outputs_.size() << " NDI outputs..." << std::endl;

        for (auto& pair : outputs_) {
            const std::string& name = pair.first;
            auto& output = pair.second;

            if (output && output->isStreaming()) {
                std::cout << "  Stopping output: " << name << std::endl;
                output->stopStream();
            }
        }

        outputs_.clear();
        std::cout << "All NDI outputs cleaned up" << std::endl;
    }

    // Clean up finder
    if (ndi_find_) {
        std::cout << "Destroying NDI finder..." << std::endl;
        NDIlib_find_destroy(ndi_find_);
        ndi_find_ = nullptr;
    }

    // Shutdown NDI SDK
    std::cout << "Shutting down NDI SDK..." << std::endl;
    NDIlib_destroy();

    initialized_ = false;
    std::cout << "NDI shutdown complete - all streams cleaned up" << std::endl;
}

std::vector<NDISourceInfo> NDIManager::discoverSources() {
    std::vector<NDISourceInfo> sources;

    if (!initialized_ || !ndi_find_) {
        std::cout << "NDI not initialized for discovery" << std::endl;
        return sources;
    }

    std::cout << "Getting current NDI sources..." << std::endl;

    // Get sources immediately without waiting
    uint32_t num_sources = 0;
    const NDIlib_source_t* ndi_sources = NDIlib_find_get_current_sources(ndi_find_, &num_sources);

    std::cout << "Found " << num_sources << " sources" << std::endl;

    for (uint32_t i = 0; i < num_sources; i++) {
        NDISourceInfo source_info;
        source_info.name = ndi_sources[i].p_ndi_name ? ndi_sources[i].p_ndi_name : "";
        source_info.url = ndi_sources[i].p_url_address ? ndi_sources[i].p_url_address : "";
        source_info.ip_address = source_info.url; // Simplified
        source_info.is_available = true;

        sources.push_back(source_info);

        std::cout << "  [" << i << "] " << source_info.name;
        if (!source_info.url.empty()) {
            std::cout << " (" << source_info.url << ")";
        }
        std::cout << std::endl;
    }

    return sources;
}

std::shared_ptr<NDISource> NDIManager::createSource(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(sources_mutex_);

    auto it = sources_.find(source_name);
    if (it != sources_.end()) {
        // Increment reference count for existing source
        {
            std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);
            source_ref_counts_[source_name]++;
            std::cout << "Returning existing source '" << source_name
                      << "' (ref count: " << source_ref_counts_[source_name] << ")" << std::endl;
        }
        return it->second;
    }

    // Create new source
    auto source = std::make_shared<NDISource>(source_name);
    source->setLowLatencyMode(global_low_latency_);
    sources_[source_name] = source;

    // Initialize reference count
    {
        std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);
        source_ref_counts_[source_name] = 1;
        std::cout << "Created new source '" << source_name
                  << "' (ref count: 1)" << std::endl;
    }

    updateGlobalStats();
    return source;
}

void NDIManager::startSourceDiscovery(std::function<void(const std::vector<NDISourceInfo>&)> callback) {
    if (discovery_running_) {
        return;
    }

    discovery_callback_ = callback;
    discovery_running_ = true;
    discovery_thread_ = std::thread(&NDIManager::discoveryLoop, this);
}

void NDIManager::stopSourceDiscovery() {
    if (!discovery_running_) {
        return;
    }

    discovery_running_ = false;
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
}

void NDIManager::discoveryLoop() {
    while (discovery_running_) {
        auto sources = discoverSources();

        if (discovery_callback_) {
            discovery_callback_(sources);
        }

        // Wait before next discovery
        std::this_thread::sleep_for(std::chrono::seconds(discovery_interval_));
    }
}

void NDIManager::removeSource(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(sources_mutex_);

    {
        std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);
        auto ref_it = source_ref_counts_.find(source_name);
        if (ref_it != source_ref_counts_.end()) {
            ref_it->second--;
            std::cout << "Decremented ref count for '" << source_name
                      << "' (new count: " << ref_it->second << ")" << std::endl;

            // Only remove if no more references
            if (ref_it->second <= 0) {
                std::cout << "Removing source '" << source_name << "' (no more references)" << std::endl;
                sources_.erase(source_name);
                source_ref_counts_.erase(ref_it);
                updateGlobalStats();
            }
            return;
        }
    }

    // Fallback: remove immediately if no ref count tracking
    std::cout << "Removing source '" << source_name << "' (no ref count found)" << std::endl;
    sources_.erase(source_name);
    updateGlobalStats();
}

std::shared_ptr<NDISource> NDIManager::getSource(const std::string& source_name) {
    std::lock_guard<std::mutex> lock(sources_mutex_);
    auto it = sources_.find(source_name);
    return (it != sources_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<NDISource>> NDIManager::getActiveSources() {
    std::lock_guard<std::mutex> lock(sources_mutex_);
    std::vector<std::shared_ptr<NDISource>> active_sources;

    for (const auto& pair : sources_) {
        if (pair.second->isConnected()) {
            active_sources.push_back(pair.second);
        }
    }

    return active_sources;
}

std::shared_ptr<NDIOutput> NDIManager::createOutput(const std::string& output_name,
                                                    int width, int height, double fps) {
    std::lock_guard<std::mutex> lock(outputs_mutex_);

    auto it = outputs_.find(output_name);
    if (it != outputs_.end()) {
        return it->second;
    }

    auto output = std::make_shared<NDIOutput>(output_name, width, height, fps);
    outputs_[output_name] = output;

    updateGlobalStats();
    return output;
}

void NDIManager::removeOutput(const std::string& output_name) {
    std::lock_guard<std::mutex> lock(outputs_mutex_);
    outputs_.erase(output_name);
    updateGlobalStats();
}

std::shared_ptr<NDIOutput> NDIManager::getOutput(const std::string& output_name) {
    std::lock_guard<std::mutex> lock(outputs_mutex_);
    auto it = outputs_.find(output_name);
    return (it != outputs_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<NDIOutput>> NDIManager::getActiveOutputs() {
    std::lock_guard<std::mutex> lock(outputs_mutex_);
    std::vector<std::shared_ptr<NDIOutput>> active_outputs;

    for (const auto& pair : outputs_) {
        if (pair.second->isStreaming()) {
            active_outputs.push_back(pair.second);
        }
    }

    return active_outputs;
}

void NDIManager::setGlobalLowLatencyMode(bool enabled) {
    global_low_latency_ = enabled;

    // Apply to existing sources
    std::lock_guard<std::mutex> lock(sources_mutex_);
    for (auto& pair : sources_) {
        pair.second->setLowLatencyMode(enabled);
    }
}

void NDIManager::setDiscoveryInterval(int seconds) {
    discovery_interval_ = std::max(1, seconds);
}

void NDIManager::setNetworkInterface(const std::string& interface_name) {
    network_interface_ = interface_name;
}

void NDIManager::setMulticastGroup(const std::string& group) {
    multicast_group_ = group;
}

NDIManager::GlobalStats NDIManager::getGlobalStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return global_stats_;
}

std::vector<std::string> NDIManager::getSupportedFormats() {
    return {"RGBA", "RGBX", "BGRA", "BGRX", "UYVY", "YV12", "NV12", "I420"};
}

void NDIManager::cleanupInactiveSources() {
    std::lock_guard<std::mutex> lock(sources_mutex_);
    auto it = sources_.begin();
    while (it != sources_.end()) {
        if (!it->second->isConnected()) {
            it = sources_.erase(it);
        } else {
            ++it;
        }
    }
}

void NDIManager::cleanupInactiveOutputs() {
    std::lock_guard<std::mutex> lock(outputs_mutex_);
    auto it = outputs_.begin();
    while (it != outputs_.end()) {
        if (!it->second->isStreaming()) {
            it = outputs_.erase(it);
        } else {
            ++it;
        }
    }
}

void NDIManager::printSourceUsage() const {
    std::cout << "\n=== NDI Source Usage ===" << std::endl;

    std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);
    std::lock_guard<std::mutex> sources_lock(sources_mutex_);

    if (source_ref_counts_.empty()) {
        std::cout << "No sources currently tracked" << std::endl;
        return;
    }

    for (const auto& pair : source_ref_counts_) {
        const std::string& name = pair.first;
        int ref_count = pair.second;

        auto source_it = sources_.find(name);
        bool connected = (source_it != sources_.end()) ? source_it->second->isConnected() : false;

        std::cout << "  '" << name << "': " << ref_count << " reference(s)";
        if (connected) {
            std::cout << " [CONNECTED]";
        } else {
            std::cout << " [disconnected]";
        }
        std::cout << std::endl;
    }

    std::cout << "Total unique sources: " << source_ref_counts_.size() << std::endl;
    std::cout << "=========================\n" << std::endl;
}

void NDIManager::disconnectAllSources() {
    std::lock_guard<std::mutex> sources_lock(sources_mutex_);
    std::lock_guard<std::mutex> ref_lock(ref_count_mutex_);

    std::cout << "Disconnecting all " << sources_.size() << " NDI sources..." << std::endl;

    for (auto& pair : sources_) {
        const std::string& name = pair.first;
        auto& source = pair.second;

        if (source && source->isConnected()) {
            std::cout << "  Disconnecting: " << name << std::endl;
            source->disconnect();
        }
    }

    std::cout << "All sources disconnected" << std::endl;
}

void NDIManager::stopAllOutputs() {
    std::lock_guard<std::mutex> lock(outputs_mutex_);

    std::cout << "Stopping all " << outputs_.size() << " NDI outputs..." << std::endl;

    for (auto& pair : outputs_) {
        const std::string& name = pair.first;
        auto& output = pair.second;

        if (output && output->isStreaming()) {
            std::cout << "  Stopping: " << name << std::endl;
            output->stopStream();
        }
    }

    std::cout << "All outputs stopped" << std::endl;
}

std::string NDIManager::getVersionString() {
    return std::string("NDI SDK ") + NDIlib_version();
}

const NDIlib_source_t* NDIManager::getCurrentSources(uint32_t& num_sources) const {
    if (!initialized_ || !ndi_find_) {
        num_sources = 0;
        return nullptr;
    }

    return NDIlib_find_get_current_sources(ndi_find_, &num_sources);
}

int NDIManager::getSourceRefCount(const std::string& source_name) const {
    std::lock_guard<std::mutex> lock(ref_count_mutex_);
    auto it = source_ref_counts_.find(source_name);
    return (it != source_ref_counts_.end()) ? it->second : 0;
}

std::map<std::string, int> NDIManager::getAllSourceRefCounts() const {
    std::lock_guard<std::mutex> lock(ref_count_mutex_);
    return source_ref_counts_;
}

void NDIManager::updateGlobalStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    global_stats_.active_sources = sources_.size();
    global_stats_.active_outputs = outputs_.size();
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace NDIUtils {

    bool checkGLError(const std::string& operation) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error in " << operation << ": " << getGLErrorString(error) << std::endl;
            return false;
        }
        return true;
    }

    std::string getGLErrorString(GLenum error) {
        switch (error) {
            case GL_NO_ERROR: return "No error";
            case GL_INVALID_ENUM: return "Invalid enum";
            case GL_INVALID_VALUE: return "Invalid value";
            case GL_INVALID_OPERATION: return "Invalid operation";
            case GL_OUT_OF_MEMORY: return "Out of memory";
            case GL_STACK_OVERFLOW: return "Stack overflow";
            case GL_STACK_UNDERFLOW: return "Stack underflow";
            default: return "Unknown error";
        }
    }

    std::string ndiFormatToString(NDIlib_FourCC_video_type_e format) {
        switch (format) {
            case NDIlib_FourCC_video_type_RGBA: return "RGBA";
            case NDIlib_FourCC_video_type_RGBX: return "RGBX";
            case NDIlib_FourCC_video_type_BGRA: return "BGRA";
            case NDIlib_FourCC_video_type_BGRX: return "BGRX";
            case NDIlib_FourCC_video_type_UYVY: return "UYVY";
            case NDIlib_FourCC_video_type_YV12: return "YV12";
            case NDIlib_FourCC_video_type_NV12: return "NV12";
            case NDIlib_FourCC_video_type_I420: return "I420";
            default: return "Unknown";
        }
    }

    std::vector<std::string> getNetworkInterfaces() {
        // Platform-specific implementation would go here
        return {"eth0", "wlan0", "lo"};
    }

    bool isValidIPAddress(const std::string& ip) {
        // Simple IP validation
        std::istringstream iss(ip);
        std::string token;
        int count = 0;

        while (std::getline(iss, token, '.') && count < 4) {
            try {
                int num = std::stoi(token);
                if (num < 0 || num > 255) return false;
                count++;
            } catch (...) {
                return false;
            }
        }

        return count == 4;
    }

}



// ============================================================================
// NDIYUVConverter Implementation
// ============================================================================

const char* UYVY_VERTEX_SHADER = R"(
#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

out vec2 v_texCoord;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
)";

// Optimized UYVY Fragment Shader - reduce precision for better performance
const char* UYVY_FRAGMENT_SHADER = R"(
#version 330 core

uniform sampler2D u_texture;
uniform vec2 u_texel_size;

in vec2 v_texCoord;
out vec4 fragColor;

// Optimized RGB to YUV conversion matrix (BT.709)
// Using mediump precision for better GPU performance
const mediump mat3 rgb_to_yuv = mat3(
    0.2126,  0.7152,  0.0722,    // Y
   -0.1146, -0.3854,  0.5,      // U (simplified)
    0.5,    -0.4542, -0.0458     // V
);

void main() {
    // Calculate pixel coordinates more efficiently
    mediump vec2 pixel_pos = v_texCoord / u_texel_size;
    mediump float pixel_pair = floor(pixel_pos.x * 0.5);

    // Sample coordinates
    mediump vec2 coord1 = vec2((pixel_pair * 2.0) * u_texel_size.x, v_texCoord.y);
    mediump vec2 coord2 = vec2((pixel_pair * 2.0 + 1.0) * u_texel_size.x, v_texCoord.y);

    // Sample both pixels
    mediump vec3 rgb1 = texture(u_texture, coord1).rgb;
    mediump vec3 rgb2 = texture(u_texture, coord2).rgb;

    // Convert to YUV (fast path)
    mediump vec3 yuv1 = rgb_to_yuv * rgb1;
    mediump vec3 yuv2 = rgb_to_yuv * rgb2;

    // Fast video range scaling
    mediump float y1 = yuv1.x * 0.859375 + 0.0625;
    mediump float y2 = yuv2.x * 0.859375 + 0.0625;
    mediump float u = (yuv1.y + yuv2.y) * 0.4375 + 0.5;
    mediump float v = (yuv1.z + yuv2.z) * 0.4375 + 0.5;

    // Output UYVY
    fragColor = vec4(u, y1, v, y2);
}
)";

NDIYUVConverter::NDIYUVConverter()
        : shader_program_(0), vertex_shader_(0), fragment_shader_(0),
          vao_(0), vbo_(0), framebuffer_(0), initialized_(false),
          u_texture_location_(-1), u_texel_size_location_(-1) {
}

NDIYUVConverter::~NDIYUVConverter() {
    cleanup();
}

bool NDIYUVConverter::initialize() {
    if (initialized_) {
        return true;
    }

    // Create and compile vertex shader
    vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vertex_shader_, UYVY_VERTEX_SHADER)) {
        std::cerr << "Failed to compile vertex shader" << std::endl;
        cleanup();
        return false;
    }

    // Create and compile fragment shader
    fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fragment_shader_, UYVY_FRAGMENT_SHADER)) {
        std::cerr << "Failed to compile fragment shader" << std::endl;
        cleanup();
        return false;
    }

    // Create and link shader program
    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader_);
    glAttachShader(shader_program_, fragment_shader_);

    if (!linkProgram()) {
        std::cerr << "Failed to link shader program" << std::endl;
        cleanup();
        return false;
    }

    // Get uniform locations
    u_texture_location_ = glGetUniformLocation(shader_program_, "u_texture");
    u_texel_size_location_ = glGetUniformLocation(shader_program_, "u_texel_size");

    if (u_texture_location_ == -1 || u_texel_size_location_ == -1) {
        std::cerr << "Failed to get uniform locations" << std::endl;
        cleanup();
        return false;
    }

    // Setup geometry for full-screen quad
    setupQuadGeometry();

    // Create framebuffer for render-to-texture
    glGenFramebuffers(1, &framebuffer_);

    initialized_ = NDIUtils::checkGLError("NDIYUVConverter::initialize");

    if (initialized_) {
        std::cout << "NDI GPU UYVY converter initialized successfully" << std::endl;
    }

    return initialized_;
}

bool NDIYUVConverter::compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "Shader compilation error: " << info_log << std::endl;
        return false;
    }

    return true;
}

bool NDIYUVConverter::linkProgram() {
    glLinkProgram(shader_program_);

    GLint success;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);

    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(shader_program_, 512, nullptr, info_log);
        std::cerr << "Shader program linking error: " << info_log << std::endl;
        return false;
    }

    return true;
}

void NDIYUVConverter::setupQuadGeometry() {
    // Full-screen quad vertices
    float quad_vertices[] = {
            // Position   // TexCoord
            -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
            1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
            1.0f,  1.0f,  1.0f, 1.0f,  // Top-right
            -1.0f,  1.0f,  0.0f, 1.0f   // Top-left
    };

    unsigned int indices[] = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
    };

    GLuint ebo;
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// Optimized convertToUYVY with minimal state changes
bool NDIYUVConverter::convertToUYVY(const NDITexture& rgba_texture, NDITexture& uyvy_texture) {
    if (!initialized_ || !rgba_texture.isValid()) {
        return false;
    }

    int width = rgba_texture.getWidth();
    int height = rgba_texture.getHeight();
    int uyvy_width = width / 2;

    // Create UYVY texture if needed
    if (!uyvy_texture.isValid() ||
        uyvy_texture.getWidth() != uyvy_width ||
        uyvy_texture.getHeight() != height) {

        if (!uyvy_texture.createTexture(uyvy_width, height, GL_RGBA)) {
            return false;
        }
    }

    // Minimal state saving - only what we actually change
    GLint current_framebuffer;
    GLint viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_framebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&previous_program_);

    // Setup render target
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           uyvy_texture.getTextureID(), 0);

    glViewport(0, 0, uyvy_width, height);
    glUseProgram(shader_program_);

    // Bind input texture (assume GL_TEXTURE0 is active)
    glBindTexture(GL_TEXTURE_2D, rgba_texture.getTextureID());
    // u_texture_location_ should be 0, so no glUniform1i needed if set at init

    // Set texel size
    glUniform2f(u_texel_size_location_, 1.0f / width, 1.0f / height);

    // Render (fast path - no state changes)
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Restore minimal state
    glUseProgram(previous_program_);
    glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    return true; // Skip error checking in hot path
}

void NDIYUVConverter::cleanup() {
    if (framebuffer_) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }

    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (shader_program_) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }

    if (vertex_shader_) {
        glDeleteShader(vertex_shader_);
        vertex_shader_ = 0;
    }

    if (fragment_shader_) {
        glDeleteShader(fragment_shader_);
        fragment_shader_ = 0;
    }

    initialized_ = false;
}
