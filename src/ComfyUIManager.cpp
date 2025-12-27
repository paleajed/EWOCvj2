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
        PresetType::TEXT_TO_LOOP,
        "Text-to-Loop",
        "Generate seamless video loops from text prompts",
        PresetTier::BEGINNER,
        true, true, true,  // SD full, Hunyuan partial
        "No seamless loop control - generates standard video",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "text_to_loop"
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
        true, true, true,
        "Basic interpolation only - no advanced prompt scheduling",
        true, false, false, false, false, {},
        24, 512, 512, 8.0f,
        "morphing"
    };

    presetRegistry[5] = {
        PresetType::REACTIVE_GEOMETRY,
        "Reactive Geometry",
        "Geometric patterns with ControlNet guidance",
        PresetTier::INTERMEDIATE,
        true, true, true,
        "Only depth and canny ControlNet supported",
        true, false, false, false, true,
        {ControlNetType::DEPTH, ControlNetType::CANNY, ControlNetType::NORMAL},
        16, 512, 512, 8.0f,
        "reactive_geometry"
    };

    presetRegistry[6] = {
        PresetType::GLITCH_DATABEND,
        "Glitch / Databend",
        "Digital artifact aesthetics and data corruption effects",
        PresetTier::INTERMEDIATE,
        true, true, false, "",
        true, false, true, false, false, {},
        16, 512, 512, 8.0f,
        "glitch"
    };

    // Tier 3 - Advanced
    presetRegistry[7] = {
        PresetType::MULTI_LAYER_COMPOSITE,
        "Multi-Layer Composite",
        "Combine multiple generated elements with blend modes",
        PresetTier::ADVANCED,
        true, true, true,
        "Single pass only - no true layer separation",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "multi_layer"
    };

    presetRegistry[8] = {
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

    presetRegistry[9] = {
        PresetType::TEXTURE_EVOLUTION,
        "Texture Evolution",
        "Organic material transformations between textures",
        PresetTier::ADVANCED,
        true, true, false, "",
        true, false, false, false, false, {},
        24, 512, 512, 8.0f,
        "texture_evolution"
    };

    // Tier 4 - Power User
    presetRegistry[10] = {
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

    presetRegistry[11] = {
        PresetType::BATCH_VARIATION_GENERATOR,
        "Batch Variation Generator",
        "Generate multiple variations with seed sweeps or parameter sweeps",
        PresetTier::POWER_USER,
        true, true, false, "",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "batch_variation"
    };

    presetRegistry[12] = {
        PresetType::BEAT_SYNC_PATTERN,
        "Beat-Sync Pattern",
        "Generate content aligned to musical BPM and structure",
        PresetTier::POWER_USER,
        true, true, false, "",
        true, false, false, false, false, {},
        16, 512, 512, 8.0f,
        "beat_sync"
    };

    presetRegistry[13] = {
        PresetType::CONTROLNET_DIRECTOR,
        "ControlNet Director",
        "Guide generation with sketch, depth, pose, or edge maps",
        PresetTier::POWER_USER,
        true, true, true,
        "Only depth and canny supported - no pose or sketch",
        true, true, false, false, true,
        {ControlNetType::DEPTH, ControlNetType::CANNY, ControlNetType::POSE,
         ControlNetType::SKETCH, ControlNetType::NORMAL},
        16, 512, 512, 8.0f,
        "controlnet_director"
    };

    presetRegistry[14] = {
        PresetType::UPSCALE_ENHANCE,
        "Upscale & Enhance",
        "Upscale video resolution and optionally interpolate frames",
        PresetTier::POWER_USER,
        true, true, false, "",
        false, false, true, false, false, {},
        0, 1920, 1080, 24.0f,
        "upscale"
    };

    // Interactive
    presetRegistry[15] = {
        PresetType::PROMPT_LAB,
        "Prompt Lab",
        "Experiment with prompts and compare results side-by-side",
        PresetTier::INTERACTIVE,
        true, true, false, "",
        true, false, false, false, false, {},
        8, 512, 512, 8.0f,
        "prompt_lab"
    };

    presetRegistry[16] = {
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

    // Message loop
    std::string messageBuffer;
    while (wsThreadRunning.load()) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            // Parse WebSocket frame (simplified)
            // In production, properly handle WebSocket framing
            if (bytesReceived >= 2) {
                int payloadLen = buffer[1] & 0x7F;
                int offset = 2;

                if (payloadLen == 126) {
                    payloadLen = (static_cast<unsigned char>(buffer[2]) << 8) |
                                  static_cast<unsigned char>(buffer[3]);
                    offset = 4;
                } else if (payloadLen == 127) {
                    offset = 10;  // 8-byte length
                    // For simplicity, assume payload fits in int
                    payloadLen = 0;
                    for (int i = 0; i < 8; i++) {
                        payloadLen = (payloadLen << 8) | static_cast<unsigned char>(buffer[2 + i]);
                    }
                }

                if (offset + payloadLen <= bytesReceived) {
                    std::string message(buffer + offset, payloadLen);
                    handleWebSocketMessage(message);
                }
            }
        } else if (bytesReceived == 0) {
            // Connection closed
            break;
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
            prog.state = GenerationProgress::State::DOWNLOADING;
            prog.status = "Downloading output...";
        } else {
            prog.currentNode = data["node"].get<std::string>();
            prog.nodesCompleted++;
            prog.state = GenerationProgress::State::GENERATING;
            prog.status = "Executing: " + prog.currentNode;
        }
    }

    updateProgress(prog);
}

void ComfyUIManager::parseExecutedMessage(const nlohmann::json& msg) {
    if (!msg.contains("data")) return;
    auto& data = msg["data"];

    if (data.contains("prompt_id") &&
        data["prompt_id"].get<std::string>() == currentPromptId) {
        // This prompt completed
        GenerationProgress prog = getProgress();
        prog.nodesCompleted++;
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

    // Prepare workflow
    nlohmann::json workflow = prepareWorkflow(params.preset, params);
    if (workflow.is_null()) {
        prog.state = GenerationProgress::State::FAILED;
        prog.status = "Failed to prepare workflow";
        prog.errorMessage = getLastError();
        updateProgress(prog);
        generating.store(false);
        return;
    }

    // Upload input images if needed
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
        // Update workflow with uploaded image name
        substituteNode(workflow, "${INPUT_IMAGE}", params);
    }

    // Submit workflow
    prog.state = GenerationProgress::State::QUEUED;
    prog.status = "Submitting to queue...";
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
    prog.status = "Waiting for generation...";
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
    prog.status = "Retrieving output...";
    updateProgress(prog);

    nlohmann::json history = getHistory(currentPromptId);
    if (!processOutput(history)) {
        prog.state = GenerationProgress::State::FAILED;
        prog.status = "Failed to retrieve output";
        updateProgress(prog);
        generating.store(false);
        return;
    }

    // Complete
    auto endTime = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(endTime - startTime).count();

    prog.state = GenerationProgress::State::COMPLETE;
    prog.status = "Complete";
    prog.percentComplete = 100.0f;
    prog.elapsedTime = elapsed;
    prog.outputPath = getLastOutputPath();
    updateProgress(prog);

    generating.store(false);
}

bool ComfyUIManager::waitForCompletion(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (!shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        if (elapsed.count() > timeoutMs) {
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

        // Update elapsed time
        prog.elapsedTime = elapsed.count() / 1000.0f;
        updateProgress(prog);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

bool ComfyUIManager::processOutput(const nlohmann::json& historyData) {
    if (historyData.is_null() || !historyData.contains(currentPromptId)) {
        setError("No history data for prompt");
        return false;
    }

    auto& promptHistory = historyData[currentPromptId];
    if (!promptHistory.contains("outputs")) {
        setError("No outputs in history");
        return false;
    }

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
            // Handle image sequences if needed
            for (auto& image : nodeOutputs["images"]) {
                std::string filename = image["filename"].get<std::string>();
                std::string subfolder = image.value("subfolder", "");

                std::string localPath = config.outputDir + "/" + filename;
                if (downloadOutput(filename, subfolder, localPath)) {
                    addToHistory(localPath);
                    // Continue for all images in sequence
                }
            }
            if (!outputHistory.empty()) return true;
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

    if (workflowMap.find(info.workflowFile) == workflowMap.end()) {
        // Try loading from file
        std::string path = getWorkflowPath(preset, params.backend);
        if (!loadWorkflowFile(path, params.backend)) {
            setError("Workflow not found: " + info.workflowFile);
            return nlohmann::json();
        }
    }

    workflow = workflowMap[info.workflowFile];

    // Substitute parameters
    substituteParameters(workflow, params);

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

    if (params.preset == PresetType::BEAT_SYNC_PATTERN) {
        setupBeatSync(workflow, params);
    }

    return workflow;
}

void ComfyUIManager::substituteParameters(nlohmann::json& workflow,
                                           const GenerationParams& params) {
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

            replace("${PROMPT}", params.prompt);
            replace("${NEGATIVE_PROMPT}", params.negativePrompt);
            replace("${SEED}", params.seed >= 0 ? std::to_string(params.seed) :
                              std::to_string(std::random_device()()));
            replace("${STEPS}", std::to_string(params.steps));
            replace("${CFG_SCALE}", std::to_string(params.cfgScale));
            replace("${FRAMES}", std::to_string(params.frames));
            replace("${WIDTH}", std::to_string(params.width));
            replace("${HEIGHT}", std::to_string(params.height));
            replace("${FPS}", std::to_string(params.fps));

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
