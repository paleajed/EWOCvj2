/**
 * ComfyUIManager.cpp
 *
 * Implementation of ComfyUI video generation backend integration
 *
 * License: GPL3
 */

#include "ComfyUIManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <filesystem>
#include <regex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <curl/curl.h>
#endif

// FFmpeg includes for video frame extraction
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

#include "VideoUpscaler.h"

namespace fs = std::filesystem;

// ============================================================================
// Static Members
// ============================================================================

std::vector<PresetInfo> ComfyUIManager::presetRegistry;
bool ComfyUIManager::registryInitialized = false;

// ============================================================================
// Preset Registry Initialization
// ============================================================================

void ComfyUIManager::initPresetRegistry() {
    if (registryInitialized) return;

    presetRegistry.resize(static_cast<size_t>(PresetType::PRESET_COUNT));

    // Tier 1 - Beginner
    presetRegistry[0] = {
        PresetType::TEXT_TO_VIDEO,
        "Text-to-Video",
        "Generate video from text prompts",
        PresetTier::BEGINNER,
        true, true, true,  // SD full, Hunyuan partial
        "No seamless loop control - generates standard video",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "text_to_video"
    };

    presetRegistry[1] = {
        PresetType::IMAGE_TO_MOTION,
        "Image-to-Motion",
        "Animate a still image with camera or object motion",
        PresetTier::BEGINNER,
        true, true, false, "",
        true, true, false, false, false, {},
        16, 512, 512, 8.0f,
        "image_to_motion"
    };

    presetRegistry[2] = {
        PresetType::KALEIDOSCOPE_GENERATOR,
        "Kaleidoscope Generator",
        "Create symmetric, psychedelic patterns",
        PresetTier::BEGINNER,
        true, false, false,  // SD only
        "Not supported - requires symmetry nodes unavailable in Hunyuan",
        true, false, false, false, false, {},
        24, 512, 512, 12.0f,
        "kaleidoscope"
    };

    // Tier 2 - Intermediate
    presetRegistry[3] = {
        PresetType::STYLE_TRANSFER_LOOP,
        "Style Transfer Loop",
        "Apply artistic style from a reference image using IPAdapter",
        PresetTier::INTERMEDIATE,
        true, false, false,  // SD only
        "Not supported - requires IPAdapter unavailable in Hunyuan",
        true, false, false, true, false, {},
        16, 512, 512, 8.0f,
        "style_transfer"
    };

    presetRegistry[4] = {
        PresetType::MORPHING_SEQUENCES,
        "Morphing Sequences",
        "Smooth transitions between different concepts",
        PresetTier::INTERMEDIATE,
        true, false, false,
        "Not supported - requires BatchPromptSchedule unavailable in Hunyuan",
        true, false, false, false, false, {},
        24, 512, 512, 8.0f,
        "morphing"
    };

    presetRegistry[5] = {
        PresetType::VIDEO_CONTINUATION,
        "Video Continuation",
        "Continue video from last frame with new prompt (like Veo3)",
        PresetTier::INTERMEDIATE,
        false, true, false,  // Hunyuan only
        "Not supported - requires Hunyuan i2v architecture",
        true, false, true, false, false, {},  // requires prompt, requires input video
        65, 0, 0, 24.0f,  // frames only, width/height from input
        "video_continuation"
    };

    // Tier 3 - Advanced
    presetRegistry[6] = {
        PresetType::CONTROLLABLE_CHARACTER,
        "Controllable Character",
        "Maintain consistent character across clips using reference images",
        PresetTier::ADVANCED,
        true, false, false,  // SD only
        "Not supported - requires InstantID/IPAdapter unavailable in Hunyuan",
        true, true, false, false, false, {},
        16, 512, 512, 8.0f,
        "character"
    };

    presetRegistry[7] = {
        PresetType::TEXTURE_EVOLUTION,
        "Texture Evolution",
        "Organic material transformations between textures",
        PresetTier::ADVANCED,
        true, false, false,  // SD only
        "Not supported - requires BatchPromptSchedule",
        true, false, false, false, false, {},
        24, 512, 512, 8.0f,
        "texture_evolution"
    };

    // Tier 4 - Power User
    presetRegistry[8] = {
        PresetType::LORA_TRAINING_ASSISTANT,
        "LoRA Training Assistant",
        "Train custom style models from inspiration images",
        PresetTier::POWER_USER,
        true, false, false,  // SD only
        "Not supported - different training architecture",
        false, true, false, false, false, {},
        0, 0, 0, 0.0f,  // No video output for training
        "lora_training"
    };

    presetRegistry[9] = {
        PresetType::BATCH_VARIATION_GENERATOR,
        "Batch Variation Generator",
        "Generate multiple variations with seed sweeps or parameter sweeps",
        PresetTier::POWER_USER,
        true, true, false, "",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "batch_variation"
    };

    presetRegistry[10] = {
        PresetType::CONTROLNET_DIRECTOR,
        "ControlNet Director",
        "Guide generation with sketch, depth, pose, or edge maps",
        PresetTier::POWER_USER,
        true, false, false,  // SD only
        "Not supported - Hunyuan lacks ControlNet integration",
        true, true, false, false, true,
        {ControlNetType::DEPTH, ControlNetType::CANNY, ControlNetType::POSE,
         ControlNetType::SKETCH, ControlNetType::NORMAL},
        16, 512, 512, 8.0f,
        "controlnet_director"
    };

    presetRegistry[11] = {
        PresetType::FRAME_INTERPOLATION,
        "Frame Interpolation",
        "Increase video FPS using RIFE motion interpolation",
        PresetTier::POWER_USER,
        true, true, false, "",
        false, false, true, false, false, {},
        0, 0, 0, 0.0f,  // Resolution/FPS from input video
        "frame_interpolation"
    };

    // Interactive
    presetRegistry[12] = {
        PresetType::REMIX_EXISTING_CLIP,
        "Remix Existing Clip",
        "Create variations on a previously generated clip",
        PresetTier::INTERACTIVE,
        true, true, true,
        "Basic variations only - no full remixing capabilities",
        true, false, true, false, false, {},
        16, 512, 512, 8.0f,
        "remix_clip"
    };

    registryInitialized = true;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

ComfyUIManager::ComfyUIManager() {
    initPresetRegistry();
    currentClientId = generateClientId();

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

ComfyUIManager::~ComfyUIManager() {
    disconnect();

    if (generationThread && generationThread->joinable()) {
        shouldStop.store(true);
        generationThread->join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

// ============================================================================
// Initialization
// ============================================================================

bool ComfyUIManager::initialize(const ComfyUIConfig& cfg) {
    if (initialized) {
        disconnect();
    }

    config = cfg;

    // Validate and create directories
    if (!config.workflowsDir.empty()) {
        if (!fs::exists(config.workflowsDir)) {
            setError("Workflows directory does not exist: " + config.workflowsDir);
            return false;
        }
        if (!loadWorkflows(config.workflowsDir)) {
            return false;
        }
    }

    if (!config.outputDir.empty()) {
        try {
            fs::create_directories(config.outputDir);
        } catch (const std::exception& e) {
            setError("Failed to create output directory: " + std::string(e.what()));
            return false;
        }
    }

    if (!config.inputDir.empty()) {
        try {
            fs::create_directories(config.inputDir);
        } catch (const std::exception& e) {
            setError("Failed to create input directory: " + std::string(e.what()));
            return false;
        }
    }

    initialized = true;
    clearError();
    return true;
}

// ============================================================================
// Connection Management
// ============================================================================

bool ComfyUIManager::connect() {
    if (!initialized) {
        setError("Manager not initialized");
        return false;
    }

    if (connected.load()) {
        return true;
    }

    // Test HTTP connection first
    if (!testConnection()) {
        return false;
    }

    // Start WebSocket thread
    wsThreadRunning.store(true);
    webSocketThread = std::make_unique<std::thread>(&ComfyUIManager::webSocketThreadFunc, this);

    // Wait briefly for connection
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!connected.load()) {
        setError("Failed to establish WebSocket connection");
        wsThreadRunning.store(false);
        if (webSocketThread && webSocketThread->joinable()) {
            webSocketThread->join();
        }
        return false;
    }

    clearError();
    return true;
}

void ComfyUIManager::disconnect() {
    wsThreadRunning.store(false);
    connected.store(false);

    if (webSocketThread && webSocketThread->joinable()) {
        webSocketThread->join();
        webSocketThread.reset();
    }
}

bool ComfyUIManager::testConnection() {
    std::string response = httpGet("/system_stats");
    if (response.empty()) {
        setError("Failed to connect to ComfyUI at " + config.getHTTPURL());
        return false;
    }

    try {
        nlohmann::json stats = nlohmann::json::parse(response);
        // Connection successful
        return true;
    } catch (const std::exception& e) {
        setError("Invalid response from ComfyUI: " + std::string(e.what()));
        return false;
    }
}

// ============================================================================
// Preset Information (Static)
// ============================================================================

const PresetInfo& ComfyUIManager::getPresetInfo(PresetType preset) {
    initPresetRegistry();
    size_t index = static_cast<size_t>(preset);
    if (index >= presetRegistry.size()) {
        static PresetInfo empty;
        return empty;
    }
    return presetRegistry[index];
}

std::vector<PresetType> ComfyUIManager::getPresetsForTier(PresetTier tier) {
    initPresetRegistry();
    std::vector<PresetType> result;
    for (const auto& info : presetRegistry) {
        if (info.tier == tier) {
            result.push_back(info.type);
        }
    }
    return result;
}

std::vector<PresetType> ComfyUIManager::getPresetsForBackend(GenerationBackend backend,
                                                               bool includePartial) {
    initPresetRegistry();
    std::vector<PresetType> result;
    for (const auto& info : presetRegistry) {
        bool supported = false;
        if (backend == GenerationBackend::SD_ANIMATEDIFF) {
            supported = info.supportedBySD;
        } else {
            supported = info.supportedByHunyuan ||
                       (includePartial && info.hunyuanPartialSupport);
        }
        if (supported) {
            result.push_back(info.type);
        }
    }
    return result;
}

bool ComfyUIManager::isPresetSupported(PresetType preset, GenerationBackend backend) {
    const auto& info = getPresetInfo(preset);
    if (backend == GenerationBackend::SD_ANIMATEDIFF) {
        return info.supportedBySD;
    }
    return info.supportedByHunyuan || info.hunyuanPartialSupport;
}

std::string ComfyUIManager::presetToString(PresetType preset) {
    return getPresetInfo(preset).name;
}

std::string ComfyUIManager::backendToString(GenerationBackend backend) {
    switch (backend) {
        case GenerationBackend::SD_ANIMATEDIFF: return "StableDiffusion + AnimateDiff";
        case GenerationBackend::HUNYUAN_VIDEO: return "HunyuanVideo";
        default: return "Unknown";
    }
}

std::string ComfyUIManager::tierToString(PresetTier tier) {
    switch (tier) {
        case PresetTier::BEGINNER: return "Beginner";
        case PresetTier::INTERMEDIATE: return "Intermediate";
        case PresetTier::ADVANCED: return "Advanced";
        case PresetTier::POWER_USER: return "Power User";
        case PresetTier::INTERACTIVE: return "Interactive";
        default: return "Unknown";
    }
}

// ============================================================================
// Generation
// ============================================================================

bool ComfyUIManager::startGeneration(const GenerationParams& params) {
    if (!initialized || !connected.load()) {
        setError("Not connected to ComfyUI");
        return false;
    }

    if (generating.load()) {
        setError("Generation already in progress");
        return false;
    }

    // Validate preset and backend compatibility
    if (!isPresetSupported(params.preset, params.backend)) {
        const auto& info = getPresetInfo(params.preset);
        setError("Preset '" + info.name + "' not supported by " +
                backendToString(params.backend));
        return false;
    }

    // Make a copy and apply defaults
    GenerationParams workParams = params;
    applyPresetDefaults(workParams);

    // Start generation thread
    generating.store(true);
    shouldStop.store(false);

    if (generationThread && generationThread->joinable()) {
        generationThread->join();
    }

    generationThread = std::make_unique<std::thread>(
        &ComfyUIManager::generationThreadFunc, this, workParams);

    return true;
}

void ComfyUIManager::cancelGeneration() {
    if (!generating.load()) return;

    shouldStop.store(true);
    interruptGeneration();

    GenerationProgress prog;
    prog.state = GenerationProgress::State::CANCELLED;
    prog.status = "Cancelled by user";
    updateProgress(prog);
}

GenerationProgress ComfyUIManager::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex);
    return progress;
}

void ComfyUIManager::setProgressCallback(
    std::function<void(const GenerationProgress&)> callback) {
    progressCallback = std::move(callback);
}

// ============================================================================
// Queue Management
// ============================================================================

int ComfyUIManager::getQueueLength() {
    nlohmann::json queue = getQueue();
    if (queue.is_null()) return -1;

    int count = 0;
    if (queue.contains("queue_running")) {
        count += queue["queue_running"].size();
    }
    if (queue.contains("queue_pending")) {
        count += queue["queue_pending"].size();
    }
    return count;
}

bool ComfyUIManager::clearQueue() {
    std::string response = httpPost("/queue", nlohmann::json{{"clear", true}});
    return !response.empty();
}

// ============================================================================
// Workflow Management
// ============================================================================

bool ComfyUIManager::loadWorkflows(const std::string& dir) {
    workflowsDir = dir;
    workflowsSD.clear();
    workflowsHunyuan.clear();

    // Load SD+AnimateDiff workflows
    std::string sdDir = dir + "/sd_animatediff";
    if (fs::exists(sdDir)) {
        for (const auto& entry : fs::directory_iterator(sdDir)) {
            if (entry.path().extension() == ".json") {
                loadWorkflowFile(entry.path().string(), GenerationBackend::SD_ANIMATEDIFF);
            }
        }
    }

    // Load HunyuanVideo workflows
    std::string hunyuanDir = dir + "/hunyuan";
    if (fs::exists(hunyuanDir)) {
        for (const auto& entry : fs::directory_iterator(hunyuanDir)) {
            if (entry.path().extension() == ".json") {
                loadWorkflowFile(entry.path().string(), GenerationBackend::HUNYUAN_VIDEO);
            }
        }
    }

    return true;
}

bool ComfyUIManager::reloadWorkflow(PresetType preset) {
    const auto& info = getPresetInfo(preset);

    // Reload for both backends if applicable
    if (info.supportedBySD) {
        std::string path = workflowsDir + "/sd_animatediff/" + info.workflowFile + ".json";
        if (fs::exists(path)) {
            loadWorkflowFile(path, GenerationBackend::SD_ANIMATEDIFF);
        }
    }

    if (info.supportedByHunyuan || info.hunyuanPartialSupport) {
        std::string path = workflowsDir + "/hunyuan/" + info.workflowFile + ".json";
        if (fs::exists(path)) {
            loadWorkflowFile(path, GenerationBackend::HUNYUAN_VIDEO);
        }
    }

    return true;
}

std::vector<std::string> ComfyUIManager::getAvailableWorkflows() {
    std::vector<std::string> result;

    for (const auto& [key, _] : workflowsSD) {
        result.push_back("sd_animatediff/" + key);
    }
    for (const auto& [key, _] : workflowsHunyuan) {
        result.push_back("hunyuan/" + key);
    }

    return result;
}

bool ComfyUIManager::loadWorkflowFile(const std::string& path, GenerationBackend backend) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            setError("Cannot open workflow file: " + path);
            return false;
        }

        nlohmann::json workflow;
        file >> workflow;

        std::string name = fs::path(path).stem().string();

        if (backend == GenerationBackend::SD_ANIMATEDIFF) {
            workflowsSD[name] = workflow;
        } else {
            workflowsHunyuan[name] = workflow;
        }

        return true;
    } catch (const std::exception& e) {
        setError("Error loading workflow: " + std::string(e.what()));
        return false;
    }
}

// ============================================================================
// Backend Detection
// ============================================================================

bool ComfyUIManager::checkBackendAvailability(GenerationBackend backend) {
    std::vector<std::string> models = getAvailableModels(backend);
    return !models.empty();
}

std::vector<std::string> ComfyUIManager::getAvailableModels(GenerationBackend backend) {
    std::vector<std::string> result;

    std::string response = httpGet("/object_info");
    if (response.empty()) return result;

    try {
        nlohmann::json info = nlohmann::json::parse(response);

        if (backend == GenerationBackend::SD_ANIMATEDIFF) {
            // Check for AnimateDiff loader node
            if (info.contains("ADE_AnimateDiffLoaderWithContext")) {
                // Get checkpoint models
                if (info.contains("CheckpointLoaderSimple")) {
                    auto& ckpt = info["CheckpointLoaderSimple"]["input"]["required"]["ckpt_name"];
                    if (ckpt.is_array() && ckpt.size() > 0 && ckpt[0].is_array()) {
                        for (const auto& model : ckpt[0]) {
                            result.push_back(model.get<std::string>());
                        }
                    }
                }
            }
        } else {
            // Check for HunyuanVideo loader node
            if (info.contains("HunyuanVideoLoader") ||
                info.contains("HunyuanVideoModelLoader")) {
                // Get Hunyuan models
                for (const auto& [key, value] : info.items()) {
                    if (key.find("Hunyuan") != std::string::npos &&
                        key.find("Loader") != std::string::npos) {
                        if (value.contains("input") &&
                            value["input"].contains("required")) {
                            for (const auto& [pkey, pval] : value["input"]["required"].items()) {
                                if (pval.is_array() && pval.size() > 0 && pval[0].is_array()) {
                                    for (const auto& model : pval[0]) {
                                        result.push_back(model.get<std::string>());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        // Ignore parsing errors
    }

    return result;
}

std::vector<std::string> ComfyUIManager::getAvailableLoRAs() {
    std::vector<std::string> result;

    std::string response = httpGet("/object_info/LoraLoader");
    if (response.empty()) return result;

    try {
        nlohmann::json info = nlohmann::json::parse(response);
        if (info.contains("LoraLoader") &&
            info["LoraLoader"].contains("input") &&
            info["LoraLoader"]["input"].contains("required") &&
            info["LoraLoader"]["input"]["required"].contains("lora_name")) {

            auto& loras = info["LoraLoader"]["input"]["required"]["lora_name"];
            if (loras.is_array() && loras.size() > 0 && loras[0].is_array()) {
                for (const auto& lora : loras[0]) {
                    result.push_back(lora.get<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        // Ignore parsing errors
    }

    return result;
}

std::vector<std::string> ComfyUIManager::getAvailableControlNets() {
    std::vector<std::string> result;

    std::string response = httpGet("/object_info/ControlNetLoader");
    if (response.empty()) return result;

    try {
        nlohmann::json info = nlohmann::json::parse(response);
        if (info.contains("ControlNetLoader") &&
            info["ControlNetLoader"].contains("input") &&
            info["ControlNetLoader"]["input"].contains("required") &&
            info["ControlNetLoader"]["input"]["required"].contains("control_net_name")) {

            auto& nets = info["ControlNetLoader"]["input"]["required"]["control_net_name"];
            if (nets.is_array() && nets.size() > 0 && nets[0].is_array()) {
                for (const auto& net : nets[0]) {
                    result.push_back(net.get<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        // Ignore parsing errors
    }

    return result;
}

// ============================================================================
// Output
// ============================================================================

std::string ComfyUIManager::getLastOutputPath() const {
    std::lock_guard<std::mutex> lock(historyMutex);
    if (outputHistory.empty()) return "";
    return outputHistory.back();
}

std::string ComfyUIManager::getComfyOutputDir() const {
    // ComfyUI is configured to output to config.outputDir
    // (set via --output-directory when starting ComfyUI)
    return config.outputDir;
}

std::string ComfyUIManager::getFramesDirectory() const {
    // Get the frames output directory for the current batch
    // ComfyUI SaveImage outputs to: output/<prefix>/<batchId>/frame_00001.png
    // The prefix is based on the workflow (hunyuan_t2v, hunyuan_i2v, etc.)

    if (currentBatchId.empty()) {
        return "";
    }

    // Search for a directory containing the batch ID
    std::string outputDir = getComfyOutputDir();
    if (!fs::exists(outputDir)) {
        return "";
    }

    // Look for directories containing our batch ID
    for (const auto& entry : fs::directory_iterator(outputDir)) {
        if (entry.is_directory()) {
            fs::path batchDir = entry.path() / currentBatchId;
            if (fs::exists(batchDir) && fs::is_directory(batchDir)) {
                return batchDir.string();
            }
        }
    }

    return "";
}

std::vector<std::string> ComfyUIManager::getOutputHistory(int count) {
    std::lock_guard<std::mutex> lock(historyMutex);
    int start = std::max(0, static_cast<int>(outputHistory.size()) - count);
    return std::vector<std::string>(outputHistory.begin() + start, outputHistory.end());
}

void ComfyUIManager::addToHistory(const std::string& path) {
    std::lock_guard<std::mutex> lock(historyMutex);
    outputHistory.push_back(path);
    // Keep last 100 entries
    if (outputHistory.size() > 100) {
        outputHistory.erase(outputHistory.begin());
    }
}

// ============================================================================
// Error Handling
// ============================================================================

std::string ComfyUIManager::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
}

void ComfyUIManager::clearError() {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError.clear();
}

void ComfyUIManager::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
    std::cerr << "[ComfyUIManager] Error: " << error << std::endl;
}

// ============================================================================
// Configuration
// ============================================================================

void ComfyUIManager::setConfig(const ComfyUIConfig& cfg) {
    bool needReconnect = (config.host != cfg.host || config.port != cfg.port);
    config = cfg;

    if (needReconnect && connected.load()) {
        disconnect();
        connect();
    }
}

// ============================================================================
// Private Methods - HTTP
// ============================================================================

std::string ComfyUIManager::httpGet(const std::string& endpoint) {
    return httpPost(endpoint, "", "");  // Empty POST is treated as GET
}

std::string ComfyUIManager::httpPost(const std::string& endpoint, const nlohmann::json& data) {
    return httpPost(endpoint, data.dump(), "application/json");
}

std::string ComfyUIManager::httpPost(const std::string& endpoint, const std::string& data,
                                      const std::string& contentType) {
#ifdef _WIN32
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"ComfyUIManager/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    std::wstring wHost(config.host.begin(), config.host.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                         static_cast<INTERNET_PORT>(config.port), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    LPCWSTR verb = data.empty() ? L"GET" : L"POST";

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, verb, wEndpoint.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Set timeout
    DWORD timeout = config.connectionTimeout;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    BOOL bResults = FALSE;
    if (!contentType.empty()) {
        std::wstring wContentType(contentType.begin(), contentType.end());
        std::wstring headers = L"Content-Type: " + wContentType;
        bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1,
                                       (LPVOID)data.c_str(), static_cast<DWORD>(data.length()),
                                       static_cast<DWORD>(data.length()), 0);
    } else {
        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                       WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (bResults) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                if (dwSize > 0) {
                    std::vector<char> buffer(dwSize + 1);
                    DWORD dwDownloaded = 0;
                    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                        result.append(buffer.data(), dwDownloaded);
                    }
                }
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
#else
    // Linux/macOS: Use libcurl
    std::string result;
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    std::string url = config.getHTTPURL() + endpoint;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, config.connectionTimeout);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, std::string* data) -> size_t {
            data->append(ptr, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    if (!data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        if (!contentType.empty()) {
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Content-Type: " + contentType).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result.clear();
    }

    return result;
#endif
}

nlohmann::json ComfyUIManager::submitWorkflow(const nlohmann::json& workflow) {
    nlohmann::json payload;
    payload["prompt"] = workflow;
    payload["client_id"] = currentClientId;

    std::string response = httpPost("/prompt", payload);
    if (response.empty()) {
        return nlohmann::json();
    }

    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        setError("Failed to parse workflow response: " + std::string(e.what()));
        return nlohmann::json();
    }
}

bool ComfyUIManager::interruptGeneration() {
    std::string response = httpPost("/interrupt", nlohmann::json{});
    return !response.empty();
}

nlohmann::json ComfyUIManager::getHistory(const std::string& promptId) {
    std::string response = httpGet("/history/" + promptId);
    if (response.empty()) return nlohmann::json();

    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        return nlohmann::json();
    }
}

nlohmann::json ComfyUIManager::getQueue() {
    std::string response = httpGet("/queue");
    if (response.empty()) return nlohmann::json();

    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        return nlohmann::json();
    }
}

nlohmann::json ComfyUIManager::getSystemStats() {
    std::string response = httpGet("/system_stats");
    if (response.empty()) return nlohmann::json();

    try {
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        return nlohmann::json();
    }
}

bool ComfyUIManager::downloadOutput(const std::string& filename, const std::string& subfolder,
                                     const std::string& localPath) {
    std::string endpoint = "/view?filename=" + filename;
    if (!subfolder.empty()) {
        endpoint += "&subfolder=" + subfolder;
    }
    endpoint += "&type=output";

    std::string data = httpGet(endpoint);
    if (data.empty()) {
        setError("Failed to download output: " + filename);
        return false;
    }

    std::ofstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to create output file: " + localPath);
        return false;
    }

    file.write(data.data(), data.size());
    file.close();
    return true;
}

// ============================================================================
// Private Methods - WebSocket
// ============================================================================

void ComfyUIManager::webSocketThreadFunc() {
    // Simplified WebSocket implementation
    // In production, use a proper WebSocket library like Boost.Beast or libwebsockets

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        setError("Failed to create WebSocket socket");
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(config.port));
    inet_pton(AF_INET, config.host.c_str(), &serverAddr.sin_addr);

    if (::connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        setError("Failed to connect WebSocket");
        return;
    }

    // Send WebSocket upgrade request
    std::string upgradeRequest =
        "GET /ws?clientId=" + currentClientId + " HTTP/1.1\r\n"
        "Host: " + config.host + ":" + std::to_string(config.port) + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sock, upgradeRequest.c_str(), static_cast<int>(upgradeRequest.length()), 0);

    // Receive upgrade response
    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(sock);
        setError("WebSocket upgrade failed");
        return;
    }
    buffer[bytesReceived] = '\0';

    std::string response(buffer);
    if (response.find("101") == std::string::npos) {
        closesocket(sock);
        setError("WebSocket upgrade rejected");
        return;
    }

    connected.store(true);

    // Set non-blocking mode
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    // Message loop with proper frame accumulation
    std::vector<unsigned char> frameBuffer;
    std::string messageAccumulator;  // For fragmented messages

    while (wsThreadRunning.load()) {
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            // Append received data to frame buffer
            frameBuffer.insert(frameBuffer.end(), buffer, buffer + bytesReceived);

            // Process complete frames from buffer
            while (frameBuffer.size() >= 2) {
                unsigned char opcode = frameBuffer[0] & 0x0F;
                bool fin = (frameBuffer[0] & 0x80) != 0;
                bool masked = (frameBuffer[1] & 0x80) != 0;
                uint64_t payloadLen = frameBuffer[1] & 0x7F;
                size_t headerLen = 2;

                // Extended payload length
                if (payloadLen == 126) {
                    if (frameBuffer.size() < 4) break;  // Need more data
                    payloadLen = (static_cast<uint64_t>(frameBuffer[2]) << 8) |
                                  static_cast<uint64_t>(frameBuffer[3]);
                    headerLen = 4;
                } else if (payloadLen == 127) {
                    if (frameBuffer.size() < 10) break;  // Need more data
                    payloadLen = 0;
                    for (int i = 0; i < 8; i++) {
                        payloadLen = (payloadLen << 8) | static_cast<uint64_t>(frameBuffer[2 + i]);
                    }
                    headerLen = 10;
                }

                // Masking key (server->client messages should not be masked, but handle it)
                size_t maskOffset = headerLen;
                if (masked) {
                    headerLen += 4;
                }

                // Check if we have the complete frame
                size_t totalFrameLen = headerLen + static_cast<size_t>(payloadLen);
                if (frameBuffer.size() < totalFrameLen) break;  // Need more data

                // Extract payload
                std::string payload;
                if (payloadLen > 0) {
                    payload.resize(static_cast<size_t>(payloadLen));
                    for (size_t i = 0; i < payloadLen; i++) {
                        unsigned char byte = frameBuffer[headerLen + i];
                        if (masked) {
                            byte ^= frameBuffer[maskOffset + (i % 4)];
                        }
                        payload[i] = static_cast<char>(byte);
                    }
                }

                // Remove processed frame from buffer
                frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + totalFrameLen);

                // Handle frame based on opcode
                if (opcode == 0x01 || opcode == 0x00) {  // Text frame or continuation
                    messageAccumulator += payload;
                    if (fin) {
                        // Complete message received
                        handleWebSocketMessage(messageAccumulator);
                        messageAccumulator.clear();
                    }
                } else if (opcode == 0x08) {  // Close frame
                    wsThreadRunning.store(false);
                    break;
                } else if (opcode == 0x09) {  // Ping - send pong
                    // Build pong frame (opcode 0x0A)
                    std::vector<unsigned char> pong;
                    pong.push_back(0x8A);  // FIN + Pong opcode
                    pong.push_back(static_cast<unsigned char>(payload.size()));
                    pong.insert(pong.end(), payload.begin(), payload.end());
                    send(sock, reinterpret_cast<char*>(pong.data()), static_cast<int>(pong.size()), 0);
                }
                // Ignore pong (0x0A) and other frames
            }
        } else if (bytesReceived == 0) {
            // Connection closed
            break;
        } else {
            // WSAEWOULDBLOCK is expected for non-blocking
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                break;  // Real error
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    closesocket(sock);
#else
    // Linux/macOS implementation would be similar
    // Use proper WebSocket library in production
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        setError("Failed to create WebSocket socket");
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(config.port));
    inet_pton(AF_INET, config.host.c_str(), &serverAddr.sin_addr);

    if (::connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sock);
        setError("Failed to connect WebSocket");
        return;
    }

    // Similar WebSocket handshake and message loop as Windows
    // ...

    connected.store(true);

    while (wsThreadRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(sock);
#endif

    connected.store(false);
}

void ComfyUIManager::handleWebSocketMessage(const std::string& message) {
    try {
        nlohmann::json msg = nlohmann::json::parse(message);

        if (!msg.contains("type")) return;

        std::string type = msg["type"].get<std::string>();
        std::cerr << "[ComfyUI WS] Message type: " << type << std::endl;

        if (type == "status") {
            parseStatusMessage(msg);
        } else if (type == "progress") {
            parseProgressMessage(msg);
        } else if (type == "executing") {
            parseExecutionMessage(msg);
        } else if (type == "executed") {
            parseExecutedMessage(msg);
        } else if (type == "execution_error") {
            parseErrorMessage(msg);
        }

    } catch (const std::exception& e) {
        // Ignore parsing errors for malformed messages
        std::cerr << "[ComfyUI WS] Parse error: " << e.what() << std::endl;
    }
}

void ComfyUIManager::parseStatusMessage(const nlohmann::json& msg) {
    if (!msg.contains("data")) return;
    auto& data = msg["data"];

    GenerationProgress prog = getProgress();

    if (data.contains("status") && data["status"].contains("exec_info")) {
        auto& execInfo = data["status"]["exec_info"];
        if (execInfo.contains("queue_remaining")) {
            prog.queueSize = execInfo["queue_remaining"].get<int>();
        }
    }

    updateProgress(prog);
}

void ComfyUIManager::parseProgressMessage(const nlohmann::json& msg) {
    if (!msg.contains("data")) return;
    auto& data = msg["data"];

    GenerationProgress prog = getProgress();
    prog.state = GenerationProgress::State::GENERATING;

    if (data.contains("value")) {
        prog.currentStep = data["value"].get<int>();
    }
    if (data.contains("max")) {
        prog.totalSteps = data["max"].get<int>();
    }

    if (prog.totalSteps > 0) {
        prog.percentComplete = (static_cast<float>(prog.currentStep) / prog.totalSteps) * 100.0f;
    }

    prog.status = "Generating... " + std::to_string(prog.currentStep) + "/" +
                  std::to_string(prog.totalSteps);

    updateProgress(prog);
}

void ComfyUIManager::parseExecutionMessage(const nlohmann::json& msg) {
    if (!msg.contains("data")) return;
    auto& data = msg["data"];

    GenerationProgress prog = getProgress();

    if (data.contains("node")) {
        if (data["node"].is_null()) {
            // Execution completed for this prompt
            std::cerr << "[ComfyUI WS] Execution complete (node=null), setting DOWNLOADING state" << std::endl;
            prog.state = GenerationProgress::State::DOWNLOADING;
            prog.status = "Downloading output...";
        } else {
            prog.currentNode = data["node"].get<std::string>();
            prog.nodesCompleted++;
            prog.state = GenerationProgress::State::GENERATING;
            prog.status = "Executing: " + prog.currentNode;
            std::cerr << "[ComfyUI WS] Executing node: " << prog.currentNode << std::endl;
        }
    }

    updateProgress(prog);
}

void ComfyUIManager::parseExecutedMessage(const nlohmann::json& msg) {
    std::cerr << "[ComfyUI WS] Received 'executed' message" << std::endl;

    if (!msg.contains("data")) return;
    auto& data = msg["data"];

    if (data.contains("prompt_id") &&
        data["prompt_id"].get<std::string>() == currentPromptId) {
        // This prompt completed - set to DOWNLOADING state
        std::cerr << "[ComfyUI WS] Our prompt completed: " << currentPromptId << std::endl;
        GenerationProgress prog = getProgress();
        prog.nodesCompleted++;
        prog.state = GenerationProgress::State::DOWNLOADING;
        prog.status = "Downloading output...";
        std::cerr << "[ComfyUI WS] Setting DOWNLOADING state from executed message" << std::endl;
        updateProgress(prog);
    }
}

void ComfyUIManager::parseErrorMessage(const nlohmann::json& msg) {
    GenerationProgress prog = getProgress();
    prog.state = GenerationProgress::State::FAILED;

    if (msg.contains("data") && msg["data"].contains("exception_message")) {
        prog.errorMessage = msg["data"]["exception_message"].get<std::string>();
    } else {
        prog.errorMessage = "Unknown error during execution";
    }

    prog.status = "Failed: " + prog.errorMessage;
    setError(prog.errorMessage);
    updateProgress(prog);
}

// ============================================================================
// Video Utilities - Frame Extraction
// ============================================================================

/**
 * Extract the last frame from a video and save it as PNG
 * Used for VIDEO_CONTINUATION preset
 * @param videoPath Path to input video
 * @param outputPngPath Path where to save the extracted frame
 * @param outWidth Output: video width
 * @param outHeight Output: video height
 * @return true if extraction successful
 */
static bool extractLastFrameFromVideo(const std::string& videoPath,
                                      const std::string& outputPngPath,
                                      int& outWidth, int& outHeight) {
    std::cerr << "[ComfyUI] Extracting last frame from: " << videoPath << std::endl;

    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[ComfyUI] Could not open video: " << videoPath << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "[ComfyUI] Could not find stream information" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = static_cast<int>(i);
            break;
        }
    }

    if (videoStreamIdx < 0) {
        std::cerr << "[ComfyUI] No video stream found" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    AVStream* videoStream = formatCtx->streams[videoStreamIdx];
    AVCodecParameters* codecpar = videoStream->codecpar;
    outWidth = codecpar->width;
    outHeight = codecpar->height;

    // Find decoder
    const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        std::cerr << "[ComfyUI] Could not find video decoder" << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecContext* decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decCtx, codecpar);
    if (avcodec_open2(decCtx, decoder, nullptr) < 0) {
        std::cerr << "[ComfyUI] Could not open video decoder" << std::endl;
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // Seek to near the end of the video
    // We use the duration to calculate where to seek
    int64_t duration = formatCtx->duration;
    if (duration <= 0 && videoStream->duration > 0) {
        duration = av_rescale_q(videoStream->duration, videoStream->time_base, AV_TIME_BASE_Q);
    }

    // Seek to 99% of the video to ensure we're near the last frame
    if (duration > 0) {
        int64_t seekTarget = duration - (duration / 100);  // 99% position
        if (seekTarget < 0) seekTarget = 0;
        av_seek_frame(formatCtx, -1, seekTarget, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(decCtx);
    }

    // Read frames until we get to the last one
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* lastFrame = av_frame_alloc();
    bool hasLastFrame = false;

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == videoStreamIdx) {
            if (avcodec_send_packet(decCtx, packet) >= 0) {
                while (avcodec_receive_frame(decCtx, frame) >= 0) {
                    // Copy frame data to lastFrame (we want to keep the last one)
                    av_frame_unref(lastFrame);
                    av_frame_ref(lastFrame, frame);
                    hasLastFrame = true;
                }
            }
        }
        av_packet_unref(packet);
    }

    // Flush decoder to get any remaining frames
    avcodec_send_packet(decCtx, nullptr);
    while (avcodec_receive_frame(decCtx, frame) >= 0) {
        av_frame_unref(lastFrame);
        av_frame_ref(lastFrame, frame);
        hasLastFrame = true;
    }

    if (!hasLastFrame) {
        std::cerr << "[ComfyUI] Could not decode any frames from video" << std::endl;
        av_frame_free(&frame);
        av_frame_free(&lastFrame);
        av_packet_free(&packet);
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    std::cerr << "[ComfyUI] Got last frame: " << outWidth << "x" << outHeight << std::endl;

    // Convert frame to RGB for PNG encoding
    SwsContext* swsCtx = sws_getContext(
        outWidth, outHeight, decCtx->pix_fmt,
        outWidth, outHeight, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "[ComfyUI] Could not create SWS context" << std::endl;
        av_frame_free(&frame);
        av_frame_free(&lastFrame);
        av_packet_free(&packet);
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = outWidth;
    rgbFrame->height = outHeight;
    av_frame_get_buffer(rgbFrame, 32);

    sws_scale(swsCtx, lastFrame->data, lastFrame->linesize, 0, outHeight,
              rgbFrame->data, rgbFrame->linesize);

    // Encode to PNG
    const AVCodec* pngEncoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!pngEncoder) {
        std::cerr << "[ComfyUI] PNG encoder not found" << std::endl;
        sws_freeContext(swsCtx);
        av_frame_free(&rgbFrame);
        av_frame_free(&frame);
        av_frame_free(&lastFrame);
        av_packet_free(&packet);
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecContext* pngCtx = avcodec_alloc_context3(pngEncoder);
    pngCtx->width = outWidth;
    pngCtx->height = outHeight;
    pngCtx->pix_fmt = AV_PIX_FMT_RGB24;
    pngCtx->time_base = {1, 25};

    if (avcodec_open2(pngCtx, pngEncoder, nullptr) < 0) {
        std::cerr << "[ComfyUI] Could not open PNG encoder" << std::endl;
        avcodec_free_context(&pngCtx);
        sws_freeContext(swsCtx);
        av_frame_free(&rgbFrame);
        av_frame_free(&frame);
        av_frame_free(&lastFrame);
        av_packet_free(&packet);
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // Encode the frame
    AVPacket* pngPacket = av_packet_alloc();
    rgbFrame->pts = 0;

    if (avcodec_send_frame(pngCtx, rgbFrame) < 0) {
        std::cerr << "[ComfyUI] Error sending frame to PNG encoder" << std::endl;
        av_packet_free(&pngPacket);
        avcodec_free_context(&pngCtx);
        sws_freeContext(swsCtx);
        av_frame_free(&rgbFrame);
        av_frame_free(&frame);
        av_frame_free(&lastFrame);
        av_packet_free(&packet);
        avcodec_free_context(&decCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    bool success = false;
    if (avcodec_receive_packet(pngCtx, pngPacket) >= 0) {
        // Write PNG to file
        std::ofstream outFile(outputPngPath, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(pngPacket->data), pngPacket->size);
            outFile.close();
            success = true;
            std::cerr << "[ComfyUI] Saved last frame to: " << outputPngPath << std::endl;
        } else {
            std::cerr << "[ComfyUI] Could not write PNG file: " << outputPngPath << std::endl;
        }
    }

    // Cleanup
    av_packet_free(&pngPacket);
    avcodec_free_context(&pngCtx);
    sws_freeContext(swsCtx);
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    av_frame_free(&lastFrame);
    av_packet_free(&packet);
    avcodec_free_context(&decCtx);
    avformat_close_input(&formatCtx);

    return success;
}

// ============================================================================
// Private Methods - Generation
// ============================================================================

void ComfyUIManager::generationThreadFunc(GenerationParams params) {
    auto startTime = std::chrono::steady_clock::now();

    GenerationProgress prog;
    prog.state = GenerationProgress::State::CONNECTING;
    prog.status = "Preparing workflow...";
    prog.totalFrames = params.frames;
    updateProgress(prog);

    currentParams = params;

    // Determine batch count - only for BATCH_VARIATION_GENERATOR preset
    int batchCount = 1;
    if (params.preset == PresetType::BATCH_VARIATION_GENERATOR && params.batchSize > 1) {
        batchCount = params.batchSize;
    }

    // Generate base seed if random
    int baseSeed = params.seed;
    if (baseSeed < 0) {
        baseSeed = static_cast<int>(std::random_device()() % 999999999);
    }

    // VIDEO_CONTINUATION: Extract last frame from source video
    std::string extractedFramePath;
    std::string sourceVideoForConcat;  // Store for later concatenation
    if (params.preset == PresetType::VIDEO_CONTINUATION && !params.sourceVideoPath.empty()) {
        prog.status = "Extracting last frame from source video...";
        updateProgress(prog);

        // Create temp path for extracted frame
        std::string tempDir = fs::temp_directory_path().string();
        extractedFramePath = tempDir + "/comfyui_continuation_frame_" +
                             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".png";

        int videoWidth = 0, videoHeight = 0;
        if (!extractLastFrameFromVideo(params.sourceVideoPath, extractedFramePath, videoWidth, videoHeight)) {
            prog.state = GenerationProgress::State::FAILED;
            prog.status = "Failed to extract last frame from source video";
            prog.errorMessage = "Could not extract frame from: " + params.sourceVideoPath;
            updateProgress(prog);
            generating.store(false);
            return;
        }

        // Use extracted frame as input image
        params.inputImagePath = extractedFramePath;

        // Use source video dimensions (override any user-set values)
        if (videoWidth > 0 && videoHeight > 0) {
            params.width = videoWidth;
            params.height = videoHeight;
            std::cerr << "[ComfyUI] Using source video dimensions: " << videoWidth << "x" << videoHeight << std::endl;
        }

        // Store source video path for later concatenation
        if (params.appendToSource) {
            sourceVideoForConcat = params.sourceVideoPath;
        }
    }

    // Upload input images if needed (only once, before batch loop)
    if (!params.inputImagePath.empty()) {
        std::string uploadedName;
        if (!uploadImage(params.inputImagePath, uploadedName)) {
            prog.state = GenerationProgress::State::FAILED;
            prog.status = "Failed to upload input image";
            prog.errorMessage = getLastError();
            updateProgress(prog);
            generating.store(false);
            return;
        }
    }

    // Batch generation loop
    for (int batchIdx = 0; batchIdx < batchCount; batchIdx++) {
        if (shouldStop.load()) {
            prog.state = GenerationProgress::State::CANCELLED;
            prog.status = "Cancelled";
            updateProgress(prog);
            generating.store(false);
            return;
        }

        // For batch variation, use incrementing seeds
        GenerationParams batchParams = params;
        if (batchCount > 1) {
            batchParams.seed = baseSeed + batchIdx;
            prog.status = "Batch " + std::to_string(batchIdx + 1) + "/" +
                          std::to_string(batchCount) + " - Preparing...";
            updateProgress(prog);
        }

        // Prepare workflow with this batch's parameters
        nlohmann::json workflow = prepareWorkflow(batchParams.preset, batchParams);
        if (workflow.is_null()) {
            prog.state = GenerationProgress::State::FAILED;
            prog.status = "Failed to prepare workflow";
            prog.errorMessage = getLastError();
            updateProgress(prog);
            generating.store(false);
            return;
        }

        // Update workflow with uploaded image name if needed
        if (!params.inputImagePath.empty()) {
            substituteNode(workflow, "${INPUT_IMAGE}", batchParams);
        }

        // Submit workflow
        prog.state = GenerationProgress::State::QUEUED;
        if (batchCount > 1) {
            prog.status = "Batch " + std::to_string(batchIdx + 1) + "/" +
                          std::to_string(batchCount) + " - Queued...";
        } else {
            prog.status = "Submitting to queue...";
        }
        updateProgress(prog);

        nlohmann::json response = submitWorkflow(workflow);
        if (response.is_null() || !response.contains("prompt_id")) {
            prog.state = GenerationProgress::State::FAILED;
            prog.status = "Failed to submit workflow";
            prog.errorMessage = response.contains("error") ?
                               response["error"].get<std::string>() : "Unknown error";
            updateProgress(prog);
            generating.store(false);
            return;
        }

        currentPromptId = response["prompt_id"].get<std::string>();

        // Wait for completion
        prog.state = GenerationProgress::State::GENERATING;
        if (batchCount > 1) {
            prog.status = "Batch " + std::to_string(batchIdx + 1) + "/" +
                          std::to_string(batchCount) + " - Generating...";
        } else {
            prog.status = "Waiting for generation...";
        }
        updateProgress(prog);

        if (!waitForCompletion(config.generationTimeout)) {
            if (shouldStop.load()) {
                prog.state = GenerationProgress::State::CANCELLED;
                prog.status = "Cancelled";
            } else {
                prog.state = GenerationProgress::State::FAILED;
                prog.status = "Generation timed out";
            }
            updateProgress(prog);
            generating.store(false);
            return;
        }

        // Get history and process output
        prog.state = GenerationProgress::State::DOWNLOADING;
        if (batchCount > 1) {
            prog.status = "Batch " + std::to_string(batchIdx + 1) + "/" +
                          std::to_string(batchCount) + " - Downloading...";
        } else {
            prog.status = "Retrieving output...";
        }
        updateProgress(prog);

        std::cerr << "[ComfyUI] Getting history for prompt: " << currentPromptId << std::endl;
        nlohmann::json history = getHistory(currentPromptId);
        std::cerr << "[ComfyUI] Got history, processing output..." << std::endl;

        if (!processOutput(history)) {
            std::cerr << "[ComfyUI] processOutput failed" << std::endl;
            prog.state = GenerationProgress::State::FAILED;
            prog.status = "Failed to retrieve output";
            updateProgress(prog);
            generating.store(false);
            return;
        }
        std::cerr << "[ComfyUI] Output processed successfully (batch " << (batchIdx + 1) << "/" << batchCount << ")" << std::endl;

        // Update progress percentage for batch
        if (batchCount > 1) {
            prog.percentComplete = ((float)(batchIdx + 1) / batchCount) * 100.0f;
            updateProgress(prog);
        }
    }

    // Cleanup temp extracted frame if we created one
    if (!extractedFramePath.empty() && fs::exists(extractedFramePath)) {
        fs::remove(extractedFramePath);
    }

    // Complete
    auto endTime = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = GenerationProgress::State::COMPLETE;
    if (batchCount > 1) {
        prog.status = "Complete - " + std::to_string(batchCount) + " variations";
    } else {
        prog.status = "Complete";
    }
    prog.percentComplete = 100.0f;
    prog.elapsedTime = elapsed;
    prog.outputPath = getLastOutputPath();
    updateProgress(prog);

    generating.store(false);
}

bool ComfyUIManager::waitForCompletion(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    auto lastPollTime = startTime;
    const int pollIntervalMs = 2000;  // Poll history API every 2 seconds as fallback

    while (!shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        // Only apply timeout if timeoutMs > 0 (0 = wait indefinitely)
        if (timeoutMs > 0 && elapsed.count() > timeoutMs) {
            return false;
        }

        GenerationProgress prog = getProgress();
        if (prog.state == GenerationProgress::State::FAILED ||
            prog.state == GenerationProgress::State::CANCELLED) {
            return false;
        }
        if (prog.state == GenerationProgress::State::DOWNLOADING) {
            return true;  // Ready to download
        }

        // Polling fallback: check history API periodically in case websocket isn't working
        auto sincePoll = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPollTime);
        if (sincePoll.count() >= pollIntervalMs && !currentPromptId.empty()) {
            lastPollTime = now;
            try {
                nlohmann::json history = getHistory(currentPromptId);
                if (!history.is_null() && history.contains(currentPromptId)) {
                    auto& promptData = history[currentPromptId];
                    if (promptData.contains("status")) {
                        auto& status = promptData["status"];
                        if (status.contains("completed") && status["completed"].get<bool>()) {
                            std::cerr << "[ComfyUI] Polling detected completion" << std::endl;
                            prog.state = GenerationProgress::State::DOWNLOADING;
                            prog.status = "Downloading output...";
                            updateProgress(prog);
                            return true;
                        }
                        if (status.contains("status_str") && status["status_str"].get<std::string>() == "error") {
                            std::cerr << "[ComfyUI] Polling detected error" << std::endl;
                            prog.state = GenerationProgress::State::FAILED;
                            prog.status = "Generation failed";
                            updateProgress(prog);
                            return false;
                        }
                    }
                }
            } catch (...) {
                // Ignore polling errors
            }
        }

        // Update elapsed time
        prog.elapsedTime = elapsed.count() / 1000.0f;
        updateProgress(prog);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

bool ComfyUIManager::processOutput(const nlohmann::json& historyData) {
    std::cerr << "[ComfyUI] processOutput called" << std::endl;

    if (historyData.is_null() || !historyData.contains(currentPromptId)) {
        std::cerr << "[ComfyUI] No history data for prompt: " << currentPromptId << std::endl;
        std::cerr << "[ComfyUI] History data: " << historyData.dump(2).substr(0, 500) << std::endl;
        setError("No history data for prompt");
        return false;
    }

    auto& promptHistory = historyData[currentPromptId];
    if (!promptHistory.contains("outputs")) {
        std::cerr << "[ComfyUI] No outputs in history. Keys: ";
        for (auto& [key, val] : promptHistory.items()) {
            std::cerr << key << " ";
        }
        std::cerr << std::endl;
        setError("No outputs in history");
        return false;
    }

    std::cerr << "[ComfyUI] Found outputs. Nodes: ";
    for (auto& [nodeId, nodeOutputs] : promptHistory["outputs"].items()) {
        std::cerr << nodeId << "(keys:";
        for (auto& [key, val] : nodeOutputs.items()) {
            std::cerr << key << ",";
        }
        std::cerr << ") ";
    }
    std::cerr << std::endl;

    // Find video/image outputs
    for (auto& [nodeId, nodeOutputs] : promptHistory["outputs"].items()) {
        if (nodeOutputs.contains("gifs")) {
            for (auto& gif : nodeOutputs["gifs"]) {
                std::string filename = gif["filename"].get<std::string>();
                std::string subfolder = gif.value("subfolder", "");

                std::string localPath = generateOutputPath(currentParams);
                if (downloadOutput(filename, subfolder, localPath)) {
                    addToHistory(localPath);
                    return true;
                }
            }
        }
        if (nodeOutputs.contains("videos")) {
            for (auto& video : nodeOutputs["videos"]) {
                std::string filename = video["filename"].get<std::string>();
                std::string subfolder = video.value("subfolder", "");

                std::string localPath = generateOutputPath(currentParams);
                if (downloadOutput(filename, subfolder, localPath)) {
                    addToHistory(localPath);
                    return true;
                }
            }
        }
        if (nodeOutputs.contains("images")) {
            // Handle image sequences - collect all frame paths
            std::vector<std::string> framePaths;
            std::string framesSubfolder;

            for (auto& image : nodeOutputs["images"]) {
                std::string filename = image["filename"].get<std::string>();
                std::string subfolder = image.value("subfolder", "");
                framePaths.push_back(filename);
                framesSubfolder = subfolder;
            }

            std::cerr << "[ComfyUI] Found " << framePaths.size() << " images" << std::endl;

            // Get ComfyUI's output directory (files are already on disk)
            // Use fs::path for proper path handling across platforms
            fs::path comfyOutputPath = fs::path(getComfyOutputDir());
            if (!framesSubfolder.empty()) {
                // Normalize subfolder separators - ComfyUI may use either
                std::string normalizedSubfolder = framesSubfolder;
                std::replace(normalizedSubfolder.begin(), normalizedSubfolder.end(), '\\', '/');
                comfyOutputPath = comfyOutputPath / normalizedSubfolder;
            }
            std::string comfyOutputDir = comfyOutputPath.string();

            std::cerr << "[ComfyUI] ComfyUI output dir: " << comfyOutputDir << std::endl;

            // If we have multiple frames, this is a sequence
            if (framePaths.size() > 1) {
                // The frames are already in comfyOutputDir, just use that as the frames directory
                // No need to move them - they're already organized by batch ID
                if (fs::exists(comfyOutputPath) && fs::is_directory(comfyOutputPath)) {
                    // Verify at least one frame exists
                    fs::path firstFrame = comfyOutputPath / framePaths[0];
                    if (fs::exists(firstFrame)) {
                        std::cerr << "[ComfyUI] Using existing frames directory: " << comfyOutputDir << std::endl;
                        addToHistory(comfyOutputDir);
                        return true;
                    }
                }

                // Fallback: Create a unique frames directory and move frames there
                std::string framesDir = config.outputDir + "/frames_" + currentBatchId;
                fs::create_directories(framesDir);

                std::cerr << "[ComfyUI] Creating frames directory: " << framesDir << std::endl;

                // Move frames from ComfyUI output to our frames directory
                int moved = 0;
                for (const auto& filename : framePaths) {
                    fs::path srcPath = comfyOutputPath / filename;
                    fs::path dstPath = fs::path(framesDir) / filename;

                    std::cerr << "[ComfyUI] Trying to move: " << srcPath.string() << std::endl;

                    if (fs::exists(srcPath)) {
                        try {
                            fs::rename(srcPath, dstPath);  // Move (faster than copy)
                            moved++;
                        } catch (...) {
                            // If move fails (cross-device), try copy
                            try {
                                fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing);
                                fs::remove(srcPath);
                                moved++;
                            } catch (...) {
                                std::cerr << "[ComfyUI] Failed to move/copy: " << srcPath.string() << std::endl;
                            }
                        }
                    } else {
                        std::cerr << "[ComfyUI] Source file not found: " << srcPath.string() << std::endl;
                    }
                }

                if (moved > 0) {
                    std::cerr << "[ComfyUI] Moved " << moved << " frames to: " << framesDir << std::endl;
                    addToHistory(framesDir);
                    return true;
                }
            }

            // Single image - use directly from ComfyUI output
            if (!framePaths.empty()) {
                fs::path srcPath = comfyOutputPath / framePaths[0];
                if (fs::exists(srcPath)) {
                    addToHistory(srcPath.string());
                    return true;
                }
                // Fallback to download
                std::string localPath = config.outputDir + "/" + framePaths[0];
                if (downloadOutput(framePaths[0], framesSubfolder, localPath)) {
                    addToHistory(localPath);
                    return true;
                }
            }
        }
    }

    setError("No video/image output found");
    return false;
}

bool ComfyUIManager::uploadImage(const std::string& localPath, std::string& uploadedName) {
    // Read file
    std::ifstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        setError("Cannot open input image: " + localPath);
        return false;
    }

    std::vector<char> fileData((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
    file.close();

    // Create multipart form data
    std::string boundary = "----ComfyUIBoundary" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());

    std::string filename = fs::path(localPath).filename().string();

    std::ostringstream body;
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"image\"; filename=\"" << filename << "\"\r\n";
    body << "Content-Type: application/octet-stream\r\n\r\n";
    body.write(fileData.data(), fileData.size());
    body << "\r\n--" << boundary << "--\r\n";

    std::string response = httpPost("/upload/image", body.str(),
                                     "multipart/form-data; boundary=" + boundary);

    if (response.empty()) {
        setError("Failed to upload image");
        return false;
    }

    try {
        nlohmann::json result = nlohmann::json::parse(response);
        if (result.contains("name")) {
            uploadedName = result["name"].get<std::string>();
            return true;
        }
    } catch (const std::exception& e) {
        setError("Failed to parse upload response: " + std::string(e.what()));
    }

    return false;
}

// ============================================================================
// Private Methods - Workflow
// ============================================================================

std::string ComfyUIManager::getWorkflowPath(PresetType preset, GenerationBackend backend) {
    const auto& info = getPresetInfo(preset);
    std::string subdir = (backend == GenerationBackend::SD_ANIMATEDIFF) ?
                         "sd_animatediff" : "hunyuan";
    return workflowsDir + "/" + subdir + "/" + info.workflowFile + ".json";
}

nlohmann::json ComfyUIManager::prepareWorkflow(PresetType preset, const GenerationParams& params) {
    const auto& info = getPresetInfo(preset);

    // Get base workflow
    nlohmann::json workflow;
    auto& workflowMap = (params.backend == GenerationBackend::SD_ANIMATEDIFF) ?
                        workflowsSD : workflowsHunyuan;

    // Always reload workflow from disk to pick up any changes
    std::string path = getWorkflowPath(preset, params.backend);
    if (!loadWorkflowFile(path, params.backend)) {
        setError("Workflow not found: " + info.workflowFile);
        return nlohmann::json();
    }

    workflow = workflowMap[info.workflowFile];

    // Substitute parameters
    substituteParameters(workflow, params);

    // Debug: Log the prompt being sent
    for (auto& [nodeId, node] : workflow.items()) {
        if (node.contains("inputs") && node["inputs"].contains("text")) {
            std::cerr << "[ComfyUI] Node " << nodeId << " text: "
                      << node["inputs"]["text"].get<std::string>() << std::endl;
        }
    }

    // Apply backend-specific adjustments
    if (params.backend == GenerationBackend::HUNYUAN_VIDEO) {
        adjustForHunyuan(workflow, params);
    }

    // Add optional components
    if (params.controlNetType != ControlNetType::NONE) {
        addControlNet(workflow, params);
    }

    if (!params.styleImagePath.empty() &&
        params.backend == GenerationBackend::SD_ANIMATEDIFF) {
        addIPAdapter(workflow, params);
    }

    if (params.seamlessLoop && params.backend == GenerationBackend::SD_ANIMATEDIFF) {
        setupSeamlessLoop(workflow, true);
    }

    return workflow;
}

void ComfyUIManager::substituteParameters(nlohmann::json& workflow,
                                           const GenerationParams& params) {
    // Model names based on backend
    std::string modelName = (params.backend == GenerationBackend::SD_ANIMATEDIFF) ?
                            "v1-5-pruned-emaonly-fp16.safetensors" :
                            "hunyuan-video-t2v-720p-Q4_0.gguf";
    std::string motionModuleName = "mm_sd_v15_v2.fp16.safetensors";
    std::string vaeName = (params.backend == GenerationBackend::SD_ANIMATEDIFF) ?
                          "vae-ft-mse-840000-ema-pruned.safetensors" :
                          "hunyuan_video_vae_bf16.safetensors";

    // ControlNet model names
    std::string controlNetModel = "";
    switch (params.controlNetType) {
        case ControlNetType::DEPTH:
            controlNetModel = "control_v11f1p_sd15_depth.pth";
            break;
        case ControlNetType::CANNY:
            controlNetModel = "control_v11p_sd15_canny.pth";
            break;
        case ControlNetType::POSE:
            controlNetModel = "control_v11p_sd15_openpose.pth";
            break;
        case ControlNetType::SKETCH:
            controlNetModel = "control_v11p_sd15_scribble.pth";
            break;
        case ControlNetType::NORMAL:
            controlNetModel = "control_v11p_sd15_normalbae.pth";
            break;
        default:
            controlNetModel = "none";
            break;
    }

    // Motion type string
    std::string motionTypeStr = "";
    switch (params.motionType) {
        case MotionType::ZOOM_IN: motionTypeStr = "zoom_in"; break;
        case MotionType::ZOOM_OUT: motionTypeStr = "zoom_out"; break;
        case MotionType::PAN_LEFT: motionTypeStr = "pan_left"; break;
        case MotionType::PAN_RIGHT: motionTypeStr = "pan_right"; break;
        case MotionType::ROTATE_CW: motionTypeStr = "rotate_cw"; break;
        case MotionType::ROTATE_CCW: motionTypeStr = "rotate_ccw"; break;
        case MotionType::DRIFT: motionTypeStr = "drift"; break;
        case MotionType::PULSE: motionTypeStr = "pulse"; break;
    }

    // Generate seed if random
    int actualSeed = params.seed >= 0 ? params.seed :
                     static_cast<int>(std::random_device()() % 999999999);

    // Recursive substitution
    std::function<void(nlohmann::json&)> substitute = [&](nlohmann::json& node) {
        if (node.is_string()) {
            std::string str = node.get<std::string>();

            // Replace placeholders
            auto replace = [&str](const std::string& from, const std::string& to) {
                size_t pos = 0;
                while ((pos = str.find(from, pos)) != std::string::npos) {
                    str.replace(pos, from.length(), to);
                    pos += to.length();
                }
            };

            // Fix hardcoded model names that don't match actual files
            replace("mm_sd_v15_v2.ckpt", motionModuleName);
            replace("mm_sd15_v2.ckpt", motionModuleName);
            replace("v1-5-pruned.ckpt", modelName);
            replace("v1-5-pruned-emaonly.ckpt", modelName);
            replace("v1-5-pruned-emaonly.safetensors", modelName);

            // Core params
            replace("${PROMPT}", params.prompt);
            replace("${NEGATIVE_PROMPT}", params.negativePrompt);
            replace("${SEED}", std::to_string(actualSeed));
            replace("${STEPS}", std::to_string(params.steps));
            replace("${CFG_SCALE}", std::to_string(params.cfgScale));
            replace("${CFG}", std::to_string(params.cfgScale));

            // Video params
            replace("${FRAMES}", std::to_string(params.frames));
            replace("${WIDTH}", std::to_string(params.width));
            replace("${HEIGHT}", std::to_string(params.height));
            replace("${FPS}", std::to_string(params.fps));

            // Frame interpolation params
            replace("${INTERPOLATION_FACTOR}", std::to_string(params.interpolationFactor));
            replace("${TARGET_FPS}", std::to_string(params.fps));

            // AnimateDiff context params
            replace("${CONTEXT_LENGTH}", std::to_string(params.contextLength));
            replace("${CONTEXT_OVERLAP}", std::to_string(params.contextOverlap));
            replace("${SEAMLESS_LOOP}", params.seamlessLoop ? "true" : "false");

            // Model names
            replace("${MODEL_NAME}", modelName);
            replace("${MOTION_MODULE}", motionModuleName);
            replace("${VAE_NAME}", vaeName);
            replace("${CONTROLNET_MODEL}", controlNetModel);

            // Motion params
            replace("${MOTION_TYPE}", motionTypeStr);
            replace("${MOTION_INTENSITY}", std::to_string(params.motionStrength));
            replace("${MOTION_STRENGTH}", std::to_string(params.motionStrength));
            replace("${MOTION_SCALE}", std::to_string(params.motionStrength));

            // Style params
            replace("${STYLE_STRENGTH}", std::to_string(params.styleStrength));
            replace("${CONTROLNET_STRENGTH}", std::to_string(params.controlNetStrength));
            replace("${REMIX_STRENGTH}", std::to_string(params.remixStrength));

            replace("${DENOISE_STRENGTH}", std::to_string(params.denoiseStrength));

            // Beat sync params
            replace("${BPM}", std::to_string(params.bpm));
            replace("${BAR_LENGTH}", std::to_string(params.barLength));

            // Kaleidoscope
            replace("${SYMMETRY_FOLD}", std::to_string(params.symmetryFold));

            // Morphing prompts - escape for JSON
            auto escapeJson = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    switch (c) {
                        case '"': result += "\\\""; break;
                        case '\\': result += "\\\\"; break;
                        case '\n': result += "\\n"; break;
                        case '\r': result += "\\r"; break;
                        case '\t': result += "\\t"; break;
                        default: result += c; break;
                    }
                }
                return result;
            };
            replace("${MORPH_PROMPT_START}", escapeJson(params.morphStartPrompt));
            replace("${MORPH_PROMPT_END}", escapeJson(params.morphEndPrompt));
            // Midpoint is the frame where transition completes (percentage of total frames)
            int midpointFrame = (params.frames * params.morphMidpoint) / 100;
            replace("${MORPH_MIDPOINT}", std::to_string(midpointFrame));
            replace("${MORPH_AFTER_MID}", std::to_string(midpointFrame + 1));
            // Last frame for full-video interpolation
            replace("${LAST_FRAME}", std::to_string(params.frames - 1));

            // Texture evolution
            replace("${START_TEXTURE}", escapeJson(params.startTexture));
            replace("${END_TEXTURE}", escapeJson(params.endTexture));

            // Batch ID for frame output directories (timestamp-based unique ID)
            auto now = std::chrono::system_clock::now();
            auto epoch = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
            std::string batchId = std::to_string(millis);
            replace("${BATCH_ID}", batchId);

            // Store batch ID for later use in output processing
            currentBatchId = batchId;

            // Input paths (if provided)
            if (!params.inputImagePath.empty()) {
                replace("${INPUT_IMAGE}", params.inputImagePath);
            }
            if (!params.inputVideoPath.empty()) {
                replace("${INPUT_VIDEO}", params.inputVideoPath);
            }
            if (!params.styleImagePath.empty()) {
                replace("${STYLE_IMAGE}", params.styleImagePath);
            }
            if (!params.controlNetImagePath.empty()) {
                replace("${CONTROLNET_IMAGE}", params.controlNetImagePath);
            }

            node = str;
        } else if (node.is_object()) {
            for (auto& [key, value] : node.items()) {
                substitute(value);
            }
        } else if (node.is_array()) {
            for (auto& element : node) {
                substitute(element);
            }
        }
    };

    substitute(workflow);

    // Debug: print the BatchPromptSchedule text after substitution
    if (workflow.contains("4") && workflow["4"].contains("inputs") &&
        workflow["4"]["inputs"].contains("text")) {
        std::cout << "[ComfyUIManager] BatchPromptSchedule text: "
                  << workflow["4"]["inputs"]["text"].dump() << std::endl;
    }
}

void ComfyUIManager::substituteNode(nlohmann::json& node, const std::string& key,
                                     const GenerationParams& params) {
    // Substitute specific node values based on key
    // Implementation depends on workflow structure
}

void ComfyUIManager::applyPresetDefaults(GenerationParams& params) {
    const auto& info = getPresetInfo(params.preset);

    // Apply defaults if not set
    if (params.frames == 0) params.frames = info.defaultFrames;
    if (params.width == 0) params.width = info.defaultWidth;
    if (params.height == 0) params.height = info.defaultHeight;
    if (params.fps == 0.0f) params.fps = info.defaultFPS;

    // Backend-specific validation
    if (params.backend == GenerationBackend::HUNYUAN_VIDEO) {
        // HunyuanVideo requires frames = 1 + 4n (5, 9, 13, 17, 21, 25, ...)
        int n = (params.frames - 1 + 2) / 4;  // Round to nearest
        if (n < 1) n = 1;
        params.frames = 1 + 4 * n;
        if (params.frames > 129) params.frames = 129;

        // HunyuanVideo requires width/height to be multiples of 16
        params.width = ((params.width + 8) / 16) * 16;
        params.height = ((params.height + 8) / 16) * 16;
        if (params.width < 256) params.width = 256;
        if (params.height < 256) params.height = 256;
    } else {
        // SD AnimateDiff: multiples of 8 work best, max ~64 frames
        params.frames = ((params.frames + 4) / 8) * 8;  // Round to nearest 8
        if (params.frames < 8) params.frames = 8;
        if (params.frames > 64) params.frames = 64;

        // SD requires multiples of 8
        params.width = ((params.width + 4) / 8) * 8;
        params.height = ((params.height + 4) / 8) * 8;
    }

    // Generate output path if not set
    if (params.outputPath.empty()) {
        params.outputPath = generateOutputPath(params);
    }
}

void ComfyUIManager::adjustForHunyuan(nlohmann::json& workflow,
                                       const GenerationParams& params) {
    // Hunyuan-specific adjustments
    // - Remove AnimateDiff nodes
    // - Adjust model loader to Hunyuan
    // - Adjust resolution constraints (720p max recommended)

    if (params.width > 1280 || params.height > 720) {
        // Scale down for Hunyuan
        float scale = std::min(1280.0f / params.width, 720.0f / params.height);
        // Workflow should handle this internally
    }
}

void ComfyUIManager::addControlNet(nlohmann::json& workflow,
                                    const GenerationParams& params) {
    // Add ControlNet nodes to workflow
    // Implementation depends on workflow structure
}

void ComfyUIManager::addIPAdapter(nlohmann::json& workflow,
                                   const GenerationParams& params) {
    // Add IPAdapter nodes for style transfer
    // Only for SD+AnimateDiff
}

void ComfyUIManager::setupSeamlessLoop(nlohmann::json& workflow, bool enable) {
    // Configure AnimateDiff for seamless looping
    // Set context options for closed_loop
}

void ComfyUIManager::setupBeatSync(nlohmann::json& workflow,
                                    const GenerationParams& params) {
    // Calculate frame count based on BPM
    // Adjust seed changes to align with beats
    float beatsPerSecond = params.bpm / 60.0f;
    float framesPerBeat = params.fps / beatsPerSecond;

    // Ensure frame count aligns to bars
    int framesPerBar = static_cast<int>(framesPerBeat * 4);  // 4 beats per bar
    int targetFrames = framesPerBar * params.barLength;

    // Update workflow with calculated values
}

// ============================================================================
// Private Methods - Utility
// ============================================================================

void ComfyUIManager::updateProgress(const GenerationProgress& newProgress) {
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        progress = newProgress;
    }

    if (progressCallback) {
        progressCallback(newProgress);
    }
}

std::string ComfyUIManager::generateOutputPath(const GenerationParams& params) {
    // Generate unique output filename
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    std::string presetName = presetToString(params.preset);
    std::replace(presetName.begin(), presetName.end(), ' ', '_');
    std::transform(presetName.begin(), presetName.end(), presetName.begin(), ::tolower);

    std::string filename = presetName + "_" + std::to_string(timestamp);

    if (params.seed >= 0) {
        filename += "_s" + std::to_string(params.seed);
    }

    filename += "." + params.outputFormat;

    return config.outputDir + "/" + filename;
}

std::string ComfyUIManager::generateClientId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    const char* hex = "0123456789abcdef";
    std::string uuid;
    for (int i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            uuid += '-';
        }
        uuid += hex[dis(gen)];
    }
    return uuid;
}
