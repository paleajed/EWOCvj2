#include "FFGLHost.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <atomic>
#include <chrono>
#include <numeric>

#ifdef _WIN32
#define LoadLib(path) LoadLibraryA(path)
#define GetProc(lib, name) GetProcAddress(lib, name)
#define FreeLib(lib) FreeLibrary(lib)
#else
#define LoadLib(path) dlopen(path, RTLD_LAZY)
    #define GetProc(lib, name) dlsym(lib, name)
    #define FreeLib(lib) dlclose(lib)
#endif

#define USE_ULTRA_MINIMAL_PROTECTION

// GUIStateProtector Implementation
GUIStateProtector::GUIStateProtector(int maxTextureUnits) : maxTexUnits(maxTextureUnits) {
    savedTexture2D.resize(maxTexUnits);
    savedTextureBuffer.resize(maxTexUnits);
}

void GUIStateProtector::saveGUIState() {
    std::cout << "=== SAVING GUI STATE ===" << std::endl;

    // Save shader program
    glGetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
    std::cout << "Saved program: " << savedProgram << std::endl;

    // Save active texture unit
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);

    // Save ALL texture unit bindings (this is critical for your GUI)
    for (int i = 0; i < maxTexUnits; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture2D[i]);
        glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &savedTextureBuffer[i]);
    }
    std::cout << "Saved " << maxTexUnits << " texture unit bindings" << std::endl;

    // Restore the original active texture
    glActiveTexture(savedActiveTexture);

    // Save buffer bindings
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &savedElementArrayBuffer);
    glGetIntegerv(GL_TEXTURE_BUFFER_BINDING, &savedCurrentTextureBuffer);

    std::cout << "Saved buffers - VAO: " << savedVAO << ", Array: " << savedArrayBuffer
              << ", Element: " << savedElementArrayBuffer << ", TexBuffer: " << savedCurrentTextureBuffer << std::endl;

    // Save render state
    savedBlendEnabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC, &savedBlendSrc);
    glGetIntegerv(GL_BLEND_DST, &savedBlendDst);
    savedDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glGetIntegerv(GL_DEPTH_FUNC, &savedDepthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &savedDepthMask);

    // Save framebuffer state
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFramebuffer);
    glGetIntegerv(GL_DRAW_BUFFER, &savedDrawBuffer);
    glGetIntegerv(GL_VIEWPORT, savedViewport);

    std::cout << "Saved render state - Blend: " << (savedBlendEnabled ? "ON" : "OFF")
              << ", Depth: " << (savedDepthTestEnabled ? "ON" : "OFF") << std::endl;
}

void GUIStateProtector::restoreGUIState() {
    std::cout << "=== RESTORING GUI STATE ===" << std::endl;

    // Restore shader program FIRST
    glUseProgram(savedProgram);
    std::cout << "Restored program: " << savedProgram << std::endl;

    // Restore ALL texture unit bindings (this is the most critical part)
    for (int i = 0; i < maxTexUnits; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, savedTexture2D[i]);
        glBindTexture(GL_TEXTURE_BUFFER, savedTextureBuffer[i]);
    }

    // Restore the original active texture
    glActiveTexture(savedActiveTexture);
    std::cout << "Restored all " << maxTexUnits << " texture units" << std::endl;

    // Restore buffer bindings
    glBindVertexArray(savedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, savedElementArrayBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, savedCurrentTextureBuffer);

    std::cout << "Restored buffers - VAO: " << savedVAO << ", Array: " << savedArrayBuffer
              << ", Element: " << savedElementArrayBuffer << ", TexBuffer: " << savedCurrentTextureBuffer << std::endl;

    // Restore render state
    if (savedBlendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glBlendFunc(savedBlendSrc, savedBlendDst);

    if (savedDepthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthFunc(savedDepthFunc);
    glDepthMask(savedDepthMask);

    // Restore framebuffer state
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, savedDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFramebuffer);
    glDrawBuffer(savedDrawBuffer);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);

    std::cout << "Restored render and framebuffer state" << std::endl;

    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error during state restoration: " << error << std::endl;
    }
}

// MinimalGUIStateProtector Implementation
MinimalGUIStateProtector::MinimalGUIStateProtector(int maxTextureUnits) : maxTexUnits(maxTextureUnits) {
}

void MinimalGUIStateProtector::saveGUIState() {
    std::cout << "=== SAVING MINIMAL GUI STATE ===" << std::endl;

    // Save shader program (most critical)
    glGetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
    std::cout << "Saved program: " << savedProgram << std::endl;

    // Save active texture unit
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);

    // Save only the texture units your GUI actually uses
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture2D_0);

    // Save the high-numbered texture units your GUI uses
    if (maxTexUnits >= 2) {
        glActiveTexture(GL_TEXTURE0 + maxTexUnits - 2);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture2D_high1);
        glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &savedTextureBuffer_high1);

        glActiveTexture(GL_TEXTURE0 + maxTexUnits - 1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture2D_high2);
        glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &savedTextureBuffer_high2);
    }

    // Restore original active texture
    glActiveTexture(savedActiveTexture);

    // Save buffer bindings (critical for your GUI)
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &savedElementArrayBuffer);
    glGetIntegerv(GL_TEXTURE_BUFFER_BINDING, &savedTextureBuffer);

    std::cout << "Saved minimal state - Program: " << savedProgram
              << ", VAO: " << savedVAO << ", Buffers: " << savedArrayBuffer
              << "/" << savedElementArrayBuffer << "/" << savedTextureBuffer << std::endl;
}

void MinimalGUIStateProtector::restoreGUIState() {
    std::cout << "=== RESTORING MINIMAL GUI STATE ===" << std::endl;

    // Restore shader program FIRST (most critical)
    glUseProgram(savedProgram);
    std::cout << "Restored program: " << savedProgram << std::endl;

    // Restore only the texture units we saved
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, savedTexture2D_0);

    if (maxTexUnits >= 2) {
        glActiveTexture(GL_TEXTURE0 + maxTexUnits - 2);
        glBindTexture(GL_TEXTURE_2D, savedTexture2D_high1);
        glBindTexture(GL_TEXTURE_BUFFER, savedTextureBuffer_high1);

        glActiveTexture(GL_TEXTURE0 + maxTexUnits - 1);
        glBindTexture(GL_TEXTURE_2D, savedTexture2D_high2);
        glBindTexture(GL_TEXTURE_BUFFER, savedTextureBuffer_high2);
    }

    // Restore original active texture
    glActiveTexture(savedActiveTexture);
    std::cout << "Restored key texture units" << std::endl;

    // Restore buffer bindings
    glBindVertexArray(savedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, savedElementArrayBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, savedTextureBuffer);

    std::cout << "Restored minimal GUI state" << std::endl;

    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error during minimal state restoration: " << error << std::endl;
    }
}

// UltraMinimalGUIStateProtector Implementation
UltraMinimalGUIStateProtector::UltraMinimalGUIStateProtector(int maxTextureUnits) {
    // Ignore texture units parameter
}

void UltraMinimalGUIStateProtector::saveGUIState() {
    // Only save the absolutely essential state
    glGetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &savedElementArrayBuffer);
}

void UltraMinimalGUIStateProtector::restoreGUIState() {
    // Restore only the essential state
    glUseProgram(savedProgram);
    glBindVertexArray(savedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, savedElementArrayBuffer);
}



// FFGLParameter Implementation
FFGLParameter::FFGLParameter(FFUInt32 idx, const std::string& n, FFUInt32 t)
        : index(idx), name(n), type(t), visible(true), usage(FF_USAGE_STANDARD),
          bufferSize(0), bufferData(nullptr), bufferNeedsUpdate(false) {
    range = FFGLUtils::getParameterDefaultRange(type);
    defaultValue.UIntValue = 0;
    currentValue.UIntValue = 0;

    if (type == FF_TYPE_BOOLEAN) {
        defaultValue.UIntValue = 0;
    } else if (isColorParameter() || isPositionParameter() || type == FF_TYPE_STANDARD) {
        setFloatValue(0.5f);
        defaultValue = currentValue;
    }
}

bool FFGLParameter::getBoolValue() const {
    return getFloatValue() > 0.5f;
}

void FFGLParameter::setBoolValue(bool value) {
    setFloatValue(value ? 1.0f : 0.0f);
}

float FFGLParameter::getFloatValue() const {
    return FFGLUtils::FFMixedToFloat(currentValue);
}

void FFGLParameter::setFloatValue(float value) {
    currentValue = FFGLUtils::FloatToFFMixed(value);
}

FFUInt32 FFGLParameter::getIntValue() const {
    return FFGLUtils::FFMixedToUInt(currentValue);
}

void FFGLParameter::setIntValue(FFUInt32 value) {
    currentValue = FFGLUtils::UIntToFFMixed(value);
}

std::string FFGLParameter::getDisplayValue() const {
    if (isTextParameter() || isFileParameter()) {
        return textValue;
    } else if (type == FF_TYPE_BOOLEAN) {
        return getBoolValue() ? "On" : "Off";
    } else if (isColorParameter()) {
        return std::to_string(static_cast<int>(getFloatValue() * 255));
    } else if (type == FF_TYPE_INTEGER) {
        return std::to_string(getIntValue());
    } else if (isOptionParameter() && !elements.empty()) {
        FFUInt32 selectedIndex = getIntValue();
        if (selectedIndex < elements.size()) {
            return elements[selectedIndex].name;
        }
    }
    return std::to_string(getFloatValue());
}

bool FFGLParameter::isTextParameter() const {
    return FFGLUtils::isTextParameter(type);
}

bool FFGLParameter::isFileParameter() const {
    return FFGLUtils::isFileParameter(type);
}

bool FFGLParameter::isColorParameter() const {
    return FFGLUtils::isColorParameter(type);
}

bool FFGLParameter::isPositionParameter() const {
    return FFGLUtils::isPositionParameter(type);
}

bool FFGLParameter::isOptionParameter() const {
    return FFGLUtils::isOptionParameter(type);
}

bool FFGLParameter::isBufferParameter() const {
    return FFGLUtils::isBufferParameter(type);
}

bool FFGLParameter::isIntegerParameter() const {
    return FFGLUtils::isIntegerParameter(type);
}

void FFGLParameter::setBufferData(const void* data, size_t size) {
    if (type != FF_TYPE_BUFFER) {
        std::cerr << "Attempting to set buffer data on non-buffer parameter" << std::endl;
        return;
    }
    
    bufferData = const_cast<void*>(data);
    bufferSize = size;
    bufferNeedsUpdate = true;
}

void* FFGLParameter::getBufferData() const {
    return bufferData;
}

size_t FFGLParameter::getBufferSize() const {
    return bufferSize;
}

void FFGLParameter::cleanupBuffer() {
    bufferData = nullptr;
    bufferSize = 0;
    bufferNeedsUpdate = false;
}

// FFGLPlugin Implementation
FFGLPlugin::FFGLPlugin()
        : library(nullptr), mainFunc(nullptr), setLogCallbackFunc(nullptr),
          extendedInfo(nullptr), loaded(false), numParameters(0) {
    memset(&pluginInfo, 0, sizeof(pluginInfo));
}

// FFGLPlugin destructor - clean up pool
FFGLPlugin::~FFGLPlugin() {
    clearInstancePool();  // Clean up any pooled instances
    unload();  // Call existing unload logic
}

bool FFGLPlugin::load(const std::string& path) {
    if (loaded) {
        unload();
    }

    pluginPath = path;
    library = LoadLib(path.c_str());
    if (!library) {
        std::cerr << "Failed to load plugin library: " << path << std::endl;
        return false;
    }

    mainFunc = (FF_Main_FuncPtr)GetProc(library, "plugMain");
    setLogCallbackFunc = (FF_SetLogCallback_FuncPtr)GetProc(library, "SetLogCallback");

    if (!mainFunc) {
        std::cerr << "Failed to get plugMain function from: " << path << std::endl;
        FreeLib(library);
        library = nullptr;
        return false;
    }

    // Get plugin info
    FFMixed result = callPlugin(FF_GET_INFO, FFGLUtils::UIntToFFMixed(0));
    if (result.PointerValue == nullptr) {
        std::cerr << "Failed to get plugin info from: " << path << std::endl;
        FreeLib(library);
        library = nullptr;
        return false;
    }

    pluginInfo = *static_cast<PluginInfoStruct*>(result.PointerValue);

    // Validate plugin compatibility
    if (!validatePlugin()) {
        std::cerr << "Plugin validation failed: " << path << std::endl;
        FreeLib(library);
        library = nullptr;
        return false;
    }

    // Get extended info if available
    result = callPlugin(FF_GET_EXTENDED_INFO, FFGLUtils::UIntToFFMixed(0));
    if (result.PointerValue != nullptr) {
        extendedInfo = static_cast<PluginExtendedInfoStruct*>(result.PointerValue);
    }

    loadParameterTemplates();
    loaded = true;

    std::cout << "Loaded FFGL plugin: " << getDisplayName()              << " (ID: " << std::string(pluginInfo.PluginUniqueID, 4)
              << ", Type: " << FFGLUtils::getPluginTypeName(pluginInfo.PluginType)
              << ", Version: " << FFGLUtils::getFFGLVersionString(pluginInfo)
              << ", Parameters: " << numParameters << ")" << std::endl;

    return true;
}

void FFGLPlugin::unload() {
    if (!loaded) return;

    auto it = instances.begin();
    while (it != instances.end()) {
        if (auto instance = it->second.lock()) {
            instance->deinitialize();
        }
        it = instances.erase(it);
    }

    if (library) {
        FreeLib(library);
        library = nullptr;
    }

    mainFunc = nullptr;
    setLogCallbackFunc = nullptr;
    extendedInfo = nullptr;
    parameterTemplates.clear();
    loaded = false;
    numParameters = 0;
    memset(&pluginInfo, 0, sizeof(pluginInfo));
}

bool FFGLPlugin::validatePlugin() {
    if (!mainFunc) {
        return false;
    }

    if (!FFGLUtils::isFFGL2Compatible(pluginInfo)) {
        std::cerr << "Unsupported FFGL version: "
                  << FFGLUtils::getFFGLVersionString(pluginInfo) << std::endl;
        return false;
    }

    if (pluginInfo.PluginType != FF_EFFECT &&
        pluginInfo.PluginType != FF_SOURCE &&
        pluginInfo.PluginType != FF_MIXER) {
        std::cerr << "Unknown plugin type: " << pluginInfo.PluginType << std::endl;
        return false;
    }

    return true;
}

bool FFGLPlugin::queryParameterInfo(FFUInt32 paramIndex, FFGLParameter& param) {
    if (!mainFunc) return false;

    // Get parameter name
    FFMixed result = callPlugin(FF_GET_PARAMETER_NAME, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.PointerValue != nullptr && isValidStringPointer(result.PointerValue)) {
        param.name = static_cast<char*>(result.PointerValue);
    } else {
        param.name = "Param" + std::to_string(paramIndex);
    }

    // Get parameter type
    result = callPlugin(FF_GET_PARAMETER_TYPE, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue != FF_FAIL) {
        param.type = result.UIntValue;
    } else {
        param.type = FF_TYPE_STANDARD;
    }

    // Get default value
    result = callPlugin(FF_GET_PARAMETER_DEFAULT, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue != FF_FAIL) {
        param.defaultValue = result;
        param.currentValue = result;
    }

    // Get parameter range
    GetRangeStruct rangeStruct;
    rangeStruct.parameterNumber = paramIndex;
    result = callPlugin(FF_GET_RANGE, FFGLUtils::PointerToFFMixed(&rangeStruct));
    if (result.UIntValue != FF_FAIL) {
        param.range = rangeStruct.range;
    } else {
        param.range = FFGLUtils::getParameterDefaultRange(param.type);
    }
    if (param.type == FF_TYPE_OPTION) {
        // Query number of elements
        FFMixed numElements = callPlugin(FF_GET_NUM_PARAMETER_ELEMENTS, FFGLUtils::UIntToFFMixed(paramIndex));
        if (numElements.UIntValue != FF_FAIL) {
            param.range.min = 0.0f;
            param.range.max = (float)(numElements.UIntValue - 1); // 0 to 3 for 4 options
        }
    }

    // *** ENHANCED: Get parameter usage for buffer parameters ***
    if (param.type == FF_TYPE_BUFFER) {
        result = callPlugin(FF_GET_PARAMETER_USAGE, FFGLUtils::UIntToFFMixed(paramIndex));
        if (result.UIntValue != FF_FAIL) {
            param.usage = result.UIntValue;
        } else {
            param.usage = FF_USAGE_STANDARD;
        }

        printf("Buffer parameter '%s': type=%d, usage=%d\n", param.name.c_str(), param.type, param.usage);
    }

    // Get parameter group (FFGL 2.2)
    if (supportsFFGL22Features()) {
        result = callPlugin(FF_GET_PARAM_GROUP, FFGLUtils::UIntToFFMixed(paramIndex));
        if (result.UIntValue != FF_FAIL && isValidStringPointer(result.PointerValue)) {
            param.group = static_cast<char*>(result.PointerValue);
        }
    }

    // Get parameter visibility
    result = callPlugin(FF_GET_PRAMETER_VISIBILITY, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue != FF_FAIL) {
        param.visible = (result.UIntValue != FF_FALSE);
    } else {
        param.visible = true;
    }

    if (!validateParameterInfo(param)) {
        fixParameterInfo(param);
    }

    return true;
}

bool FFGLPlugin::isValidStringPointer(void* ptr) const{
    if (ptr == nullptr) return false;

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false;

    try {
        char firstChar = *static_cast<char*>(ptr);
        return firstChar >= 0;
    } catch (...) {
        return false;
    }
}

bool FFGLPlugin::supportsFFGL22Features() {
    return pluginInfo.APIMajorVersion > 2 ||
           (pluginInfo.APIMajorVersion == 2 && pluginInfo.APIMinorVersion >= 2);
}

void FFGLPlugin::queryParameterElements(FFUInt32 paramIndex, FFGLParameter& param) {
    if (param.type != FF_TYPE_OPTION) return;

    FFMixed result = callPlugin(FF_GET_NUM_PARAMETER_ELEMENTS, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue == FF_FAIL) return;

    FFUInt32 numElements = result.UIntValue;
    param.elements.clear();
    param.elements.reserve(numElements);

    for (FFUInt32 i = 0; i < numElements; i++) {
        GetParameterElementNameStruct nameStruct;
        nameStruct.ParameterNumber = paramIndex;
        nameStruct.ElementNumber = i;

        result = callPlugin(FF_GET_PARAMETER_ELEMENT_NAME, FFGLUtils::PointerToFFMixed(&nameStruct));
        std::string elementName = "Element" + std::to_string(i);
        if (result.PointerValue != nullptr) {
            elementName = static_cast<char*>(result.PointerValue);
        }

        GetParameterElementValueStruct valueStruct;
        valueStruct.ParameterNumber = paramIndex;
        valueStruct.ElementNumber = i;

        result = callPlugin(FF_GET_PARAMETER_ELEMENT_VALUE, FFGLUtils::PointerToFFMixed(&valueStruct));
        FFMixed elementValue;
        elementValue.UIntValue = (result.UIntValue != FF_FAIL) ? result.UIntValue : i;

        param.elements.emplace_back(elementName, elementValue.UIntValue);
    }
}

void FFGLPlugin::queryFileParameterExtensions(FFUInt32 paramIndex, FFGLParameter& param) {
    if (param.type != FF_TYPE_FILE) return;

    FFMixed result = callPlugin(FF_GET_NUM_FILE_PARAMETER_EXTENSIONS, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue == FF_FAIL) return;

    FFUInt32 numExtensions = result.UIntValue;
    param.fileExtensions.clear();
    param.fileExtensions.reserve(numExtensions);

    for (FFUInt32 i = 0; i < numExtensions; i++) {
        GetFileParameterExtensionStruct extStruct;
        extStruct.ParameterNumber = paramIndex;
        extStruct.ExtensionNumber = i;

        result = callPlugin(FF_GET_FILE_PARAMETER_EXTENSION, FFGLUtils::PointerToFFMixed(&extStruct));
        if (result.PointerValue != nullptr) {
            param.fileExtensions.emplace_back(static_cast<char*>(result.PointerValue));
        }
    }
}

void FFGLPlugin::loadParameterTemplates() {
    parameterTemplates.clear();
    numParameters = 0;

    if (!mainFunc) return;

    FFMixed result = callPlugin(FF_GET_NUM_PARAMETERS, FFGLUtils::UIntToFFMixed(0));
    if (result.UIntValue == FF_FAIL) {
        std::cout << "Plugin doesn't support parameter queries, assuming no parameters" << std::endl;
        return;
    }

    numParameters = result.UIntValue;
    parameterTemplates.reserve(numParameters);

    for (FFUInt32 i = 0; i < numParameters; i++) {
        FFGLParameter param(i, "", FF_TYPE_STANDARD);

        if (queryParameterInfo(i, param)) {
            queryParameterElements(i, param);
            queryFileParameterExtensions(i, param);

            if (param.isBufferParameter()) {
                printf("Found buffer parameter: '%s' (index %d, type: %d, usage: %d)\n",
                       param.name.c_str(), i, param.type, param.usage);
            }

            parameterTemplates.push_back(std::move(param));
        } else {
            param.name = "Param" + std::to_string(i);
            param.type = FF_TYPE_STANDARD;
            parameterTemplates.push_back(std::move(param));
        }
    }

    std::cout << "Discovered " << numParameters << " parameters for plugin: "
              << std::string(pluginInfo.PluginName, 16) << std::endl;
}

std::string FFGLPlugin::getDisplayName() const {
    // Method 1: Try FF_GET_PLUGIN_SHORT_NAME (for well-behaved plugins)
    FFMixed result = callPlugin(FF_GET_PLUGIN_SHORT_NAME, FFGLUtils::UIntToFFMixed(0));
    if (result.PointerValue != nullptr && isValidStringPointer(result.PointerValue)) {
        std::string shortName = static_cast<char*>(result.PointerValue);
        if (!shortName.empty()) {
            return shortName;
        }
    }

    // Method 2: Parse from extended info description (for ShaderToySender2 and similar)
    if (extendedInfo && extendedInfo->Description) {
        std::string description = extendedInfo->Description;

        // Look for pattern "Plugin: NAME (ID"
        size_t pluginPos = description.find("Plugin: ");
        if (pluginPos != std::string::npos) {
            size_t nameStart = pluginPos + 8; // Length of "Plugin: "
            size_t nameEnd = description.find(" (ID", nameStart);
            if (nameEnd != std::string::npos) {
                return description.substr(nameStart, nameEnd - nameStart);
            }
        }
    }

    // Method 3: Fallback to standard plugin name
    return std::string(pluginInfo.PluginName, 16);
}

std::string FFGLPlugin::getShortName() const {
    if (!mainFunc) return "";

    FFMixed result = callPlugin(FF_GET_PLUGIN_SHORT_NAME, FFGLUtils::UIntToFFMixed(0));
    if (result.PointerValue != nullptr) {
        return static_cast<char*>(result.PointerValue);
    }
    return std::string(pluginInfo.PluginName, 16);
}

bool FFGLPlugin::supportsCapability(FFUInt32 capability) const {
    if (!mainFunc) return false;

    FFMixed result = callPlugin(FF_GET_PLUGIN_CAPS, FFGLUtils::UIntToFFMixed(capability));
    return result.UIntValue == FF_SUPPORTED;
}

FFUInt32 FFGLPlugin::getMinimumInputFrames() const {
    if (!supportsCapability(FF_CAP_MINIMUM_INPUT_FRAMES)) return 0;

    FFMixed result = callPlugin(FF_GET_PLUGIN_CAPS, FFGLUtils::UIntToFFMixed(FF_CAP_MINIMUM_INPUT_FRAMES));
    return result.UIntValue;
}

FFUInt32 FFGLPlugin::getMaximumInputFrames() const {
    if (!supportsCapability(FF_CAP_MAXIMUM_INPUT_FRAMES)) return 1;

    FFMixed result = callPlugin(FF_GET_PLUGIN_CAPS, FFGLUtils::UIntToFFMixed(FF_CAP_MAXIMUM_INPUT_FRAMES));
    return result.UIntValue;
}

bool FFGLPlugin::supportsTopLeftTextureOrientation() const {
    return supportsCapability(FF_CAP_TOP_LEFT_TEXTURE_ORIENTATION);
}

bool FFGLPlugin::getThumbnail(FFUInt32& width, FFUInt32& height, std::vector<FFUInt32>& rgbaPixels) const {
    if (!mainFunc) return false;

    GetThumbnailStruct thumbnailStruct;
    thumbnailStruct.width = 0;
    thumbnailStruct.height = 0;
    thumbnailStruct.rgbaPixelBuffer = nullptr;

    FFMixed result = callPlugin(FF_GET_THUMBNAIL, FFGLUtils::PointerToFFMixed(&thumbnailStruct));
    if (result.UIntValue == FF_FAIL || thumbnailStruct.width == 0 || thumbnailStruct.height == 0) {
        return false;
    }

    width = thumbnailStruct.width;
    height = thumbnailStruct.height;

    rgbaPixels.resize(width * height);
    thumbnailStruct.rgbaPixelBuffer = rgbaPixels.data();

    result = callPlugin(FF_GET_THUMBNAIL, FFGLUtils::PointerToFFMixed(&thumbnailStruct));
    return result.UIntValue != FF_FAIL;
}

void FFGLPlugin::setHostInfo(const std::string& hostName, const std::string& hostVersion) {
    if (!mainFunc) return;

    SetHostinfoStruct hostInfo;
    hostInfo.name = hostName.c_str();
    hostInfo.version = hostVersion.c_str();

    callPlugin(FF_SET_HOSTINFO, FFGLUtils::PointerToFFMixed(&hostInfo));
}

void FFGLPlugin::setSampleRate(float sampleRate) {
    if (!mainFunc) return;

    callPlugin(FF_SET_SAMPLERATE, FFGLUtils::FloatToFFMixed(sampleRate));
}

void FFGLPlugin::destroyInstance(FFInstanceID instanceID) {
    std::lock_guard<std::mutex> poolLock(poolMutex_);
    std::lock_guard<std::mutex> instanceLock(instancesMutex_);

    auto it = instances.find(instanceID);
    if (it != instances.end()) {
        if (auto instance = it->second.lock()) {
            // Remove from active tracking but don't actually destroy - use pooling
            instances.erase(it);
            activeInstances_--;

            // Keep initialized and add to pool (for instant reuse)
            if (instancePool_.size() < MAX_POOL_SIZE) {
                instance->setPooled(true);
                instancePool_.push_back(instance);

                std::cout << "Moved initialized instance to pool instead of destroying (Pool size: "
                          << instancePool_.size() << ")" << std::endl;
            } else {
                // Pool full - deinitialize and destroy
                instance->deinitialize();
                std::cout << "Pool full - instance will be destroyed" << std::endl;
            }
        } else {
            instances.erase(it);
        }
    }
}

size_t FFGLPlugin::getInstanceCount() const {
    std::lock_guard<std::mutex> lock(instancesMutex_);
    size_t count = 0;
    for (const auto& pair : instances) {
        if (!pair.second.expired()) {
            count++;
        }
    }
    return count;
}

FFMixed FFGLPlugin::callPlugin(FFUInt32 functionCode, FFMixed inputValue, FFInstanceID instanceID) const {
    if (!mainFunc) {
        FFMixed result;
        result.UIntValue = FF_FAIL;
        return result;
    }

    return mainFunc(functionCode, inputValue, instanceID);
}

FFGLInstanceHandle FFGLPlugin::createInstance(FFUInt32 width, FFUInt32 height) {
    if (!loaded) {
        std::cerr << "Cannot create instance: plugin not loaded" << std::endl;
        return nullptr;
    }

    FFGLViewportStruct viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = width;
    viewport.height = height;

    return createInstance(viewport);
}

FFGLInstanceHandle FFGLPlugin::createInstance(const FFGLViewportStruct& viewport) {
    if (!loaded) {
        std::cerr << "Cannot create instance: plugin not loaded" << std::endl;
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(poolMutex_);

    std::shared_ptr<FFGLPluginInstance> instance = nullptr;
    bool fromPool = false;

    // Try to get an instance from the pool
    if (!instancePool_.empty()) {
        instance = instancePool_.back();
        instancePool_.pop_back();

        instance->setPooled(false);
        instance->resetForReuse();  // Clean state for reuse (keeps it initialized)

        fromPool = true;
        std::cout << "Reused FFGL instance from pool (Pool size: " << instancePool_.size() << ")" << std::endl;
    } else {
        // Pool is empty - create new instance with shared_ptr to this plugin
        FFGLViewportStruct defaultViewport = viewport;
        instance = std::make_shared<FFGLPluginInstance>(shared_from_this(), defaultViewport);
        instance->setPooled(false);

        std::cout << "Created new FFGL instance (Active: " << activeInstances_ + 1 << ")" << std::endl;
    }

    // Initialize or resize the instance based on whether it came from the pool
    if (fromPool && instance->isInitialized()) {
        // Instance is already initialized, just resize if viewport changed
        const FFGLViewportStruct& currentViewport = instance->getViewport();
        if (viewport.width != currentViewport.width || viewport.height != currentViewport.height ||
            viewport.x != currentViewport.x || viewport.y != currentViewport.y) {
            if (!instance->resize(viewport)) {
                std::cerr << "Failed to resize pooled instance" << std::endl;
                // Try full re-initialization as fallback
                instance->deinitialize();
                if (!instance->initialize(viewport)) {
                    std::cerr << "Failed to re-initialize plugin instance after resize failure" << std::endl;
                    return nullptr;
                }
            }
        }
        std::cout << "Reused initialized instance (no re-initialization needed)" << std::endl;
    } else {
        // New instance or uninitialized - do full initialization
        if (!instance->initialize(viewport)) {
            std::cerr << "Failed to initialize plugin instance" << std::endl;
            return nullptr;
        }
    }

    // Update instance tracking - store as weak_ptr
    FFInstanceID pluginInstanceID = instance->getPluginInstanceID();
    {
        std::lock_guard<std::mutex> lock(instancesMutex_);
        instances[pluginInstanceID] = std::weak_ptr<FFGLPluginInstance>(instance);
    }
    activeInstances_++;

    std::cout << "Instance ready with plugin ID: " << pluginInstanceID << std::endl;
    return instance;
}

void FFGLPlugin::releaseInstance(std::shared_ptr<FFGLPluginInstance> instance) {
    if (!instance || instance->getParentPlugin().get() != this) {
        std::cerr << "Invalid instance passed to releaseInstance!" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(poolMutex_);

    // Remove from active instance tracking
    FFInstanceID pluginInstanceID = instance->getPluginInstanceID();
    auto it = instances.find(pluginInstanceID);
    if (it != instances.end()) {
        instances.erase(it);
    }
    activeInstances_--;

    // Keep the instance initialized and add to pool (for instant reuse)
    // NOTE: We do NOT deinitialize here anymore - instances stay "warm" in the pool
    if (instancePool_.size() < MAX_POOL_SIZE) {
        instance->setPooled(true);
        instancePool_.push_back(instance);
        std::cout << "Released initialized instance to pool (Pool size: " << instancePool_.size() << ")" << std::endl;
    } else {
        // Pool is full - deinitialize this instance and let it be destroyed
        instance->deinitialize();
        std::cout << "Pool full - instance will be destroyed" << std::endl;
    }
}

void FFGLPlugin::clearInstancePool() {
    std::lock_guard<std::mutex> lock(poolMutex_);

    size_t poolSize = instancePool_.size();

    // Properly deinitialize all pooled instances before clearing
    for (auto& instance : instancePool_) {
        if (instance && instance->isInitialized()) {
            instance->deinitialize();
        }
    }

    instancePool_.clear();
}

void FFGLPlugin::prewarmInstancePool(size_t count) {
    if (!loaded) {
        std::cerr << "Cannot prewarm pool: plugin not loaded" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(poolMutex_);

    std::cout << "Pre-warming " << count << " instance(s) for plugin: " << getDisplayName() << std::endl;

    for (size_t i = 0; i < count; i++) {
        if (instancePool_.size() >= MAX_POOL_SIZE) {
            std::cout << "Pool size limit reached during pre-warming" << std::endl;
            break;
        }

        // Create default viewport (1920x1080)
        FFGLViewportStruct defaultViewport;
        defaultViewport.x = 0;
        defaultViewport.y = 0;
        defaultViewport.width = 1920;
        defaultViewport.height = 1080;

        // Create instance
        auto instance = std::make_shared<FFGLPluginInstance>(shared_from_this(), defaultViewport);
        instance->setPooled(false);

        // Initialize (this is the expensive part we want to do at startup)
        if (instance->initialize(defaultViewport)) {
            // Keep it initialized and add to pool for instant reuse
            instance->setPooled(true);
            instancePool_.push_back(instance);

            std::cout << "  Pre-warmed instance " << (i + 1) << "/" << count << " for " << getDisplayName() << std::endl;
        } else {
            std::cerr << "  Failed to pre-warm instance " << (i + 1) << " for " << getDisplayName() << std::endl;
        }
    }

    std::cout << "Pre-warming complete. Pool size: " << instancePool_.size() << std::endl;
}

// Reset instance to clean state for reuse (works on initialized instances)
void FFGLPluginInstance::resetForReuse() {
    // Reset all parameters to their default values
    for (auto& param : parameters) {
        // Reset parameter value
        if (initialized && parentPlugin_ && parentPlugin_->mainFunc) {
            // If initialized, send default value to plugin
            SetParameterStruct paramData;
            paramData.ParameterNumber = param.index;
            paramData.NewParameterValue = param.defaultValue;
            callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));
        }

        param.currentValue = param.defaultValue;
        param.textValue.clear();

        // Clean up any buffer data
        if (param.isBufferParameter()) {
            param.cleanupBuffer();
        }
    }

    // Clear audio data
    {
        std::lock_guard<std::mutex> lock(audioDataMutex);
        latestFFTData.clear();
        hasNewAudioData = false;
    }

    {
        std::lock_guard<std::mutex> lock(audioSamplesMutex);
        latestAudioSamples.clear();
        hasNewAudioSamples = false;
    }

    // Reset timing
    timeInitialized = false;

    // Note: We don't reset currentViewport here - it will be updated via resize() when reused
}

void FFGLPluginInstance::copyTimingFrom(const FFGLPluginInstance* other) {
    if (!other) return;

    // Copy timing state from source instance - use the EXACT same start time
    // This ensures both instances calculate the same elapsed time going forward
    timeInitialized = other->timeInitialized;
    pluginStartTime = other->pluginStartTime;

    // Immediately synchronize the plugin's internal state by setting it to the
    // exact same elapsed time as the source at this moment
    if (timeInitialized) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - pluginStartTime;
        float elapsedSeconds = elapsed.count();

        // Tell the plugin to jump to this exact time point
        // This initializes the plugin's internal timing to match the source
        setTime(elapsedSeconds);
    }
}


// FFGLPluginInstance Implementation
FFGLPluginInstance::FFGLPluginInstance(std::shared_ptr<FFGLPlugin> parent, const FFGLViewportStruct& viewport)
        : parentPlugin_(parent), pluginInstanceID(nullptr), initialized(false),
          currentViewport(viewport), timeInitialized(false) {
    copyParametersFromTemplate();
}

// Modified FFGLPluginInstance destructor to work with pooling
FFGLPluginInstance::~FFGLPluginInstance() {
    // Only clean up if we're not being pooled
    if (!isInPool_) {
        deinitialize();

        // Clean up any remaining buffer data
        for (auto& param : parameters) {
            if (param.isBufferParameter()) {
                param.cleanupBuffer();
            }
        }
    }
}

// Thread-safe audio data storage (called from audio thread)
void FFGLPluginInstance::storeAudioData(const float* fftData, size_t binCount) {
    std::lock_guard<std::mutex> lock(audioDataMutex);
    latestFFTData.assign(fftData, fftData + binCount);
    hasNewAudioData = true;
}

// Apply stored audio data (called from video thread during processFrame)
void FFGLPluginInstance::applyStoredAudioData() {
    std::lock_guard<std::mutex> lock(audioDataMutex);
    if (hasNewAudioData && !latestFFTData.empty()) {
        sendAudioData(latestFFTData.data(), latestFFTData.size());
        hasNewAudioData = false;
    }
}

bool FFGLPluginInstance::initialize(const FFGLViewportStruct& viewport) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || initialized) {
        std::cerr << "Initialize failed: invalid state" << std::endl;
        return false;
    }

    currentViewport = viewport;

    GLint maxTexUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

    ActiveGUIStateProtector stateProtector(maxTexUnits);
    stateProtector.saveGUIState();
    FFMixed result = parentPlugin_->callPlugin(FF_INSTANTIATE_GL,
                                              FFGLUtils::PointerToFFMixed(const_cast<FFGLViewportStruct *>(&viewport)),
                                              nullptr);


    if (result.UIntValue == FF_FAIL) {
        std::cerr << "FF_INSTANTIATE_GL failed" << std::endl;
        stateProtector.restoreGUIState();
        return false;
    }

    if (result.PointerValue != nullptr) {
        pluginInstanceID = reinterpret_cast<FFInstanceID>(result.PointerValue);
    } else if (result.UIntValue != FF_SUCCESS && result.UIntValue != FF_FAIL) {
        pluginInstanceID = reinterpret_cast<FFInstanceID>(static_cast<uintptr_t>(result.UIntValue));
    } else {
        static uintptr_t instanceCounter = 1;
        uintptr_t uniqueID = instanceCounter++;
        pluginInstanceID = reinterpret_cast<FFInstanceID>(uniqueID + 0x1000000);
    }

    result = callPluginInstance(FF_INITIALISE_V2,
                                FFGLUtils::PointerToFFMixed(const_cast<FFGLViewportStruct *>(&viewport)));

    if (result.UIntValue == FF_SUCCESS) {
        initialized = true;
        //resetTime(); // Reset internal timer
        //setTime(0.0f); // First call: Explicitly set time to zero
        stateProtector.restoreGUIState();
        return true;
    }

    result = callPluginInstance(1,
                                FFGLUtils::PointerToFFMixed(const_cast<FFGLViewportStruct *>(&viewport)));

    if (result.UIntValue == FF_SUCCESS) {
        initialized = true;
        //resetTime(); // Reset internal timer
        //setTime(0.0f); // First call: Explicitly set time to zero
        stateProtector.restoreGUIState();
        return true;
    }

    std::cerr << "All initialization methods failed" << std::endl;

    if (pluginInstanceID) {
        parentPlugin_->callPlugin(FF_DEINSTANTIATE_GL, FFGLUtils::UIntToFFMixed(0), pluginInstanceID);
        pluginInstanceID = nullptr;
    }

    stateProtector.restoreGUIState();
    std::cout << "Restored GUI state after failed initialization" << std::endl;

    return false;
}

void FFGLPluginInstance::deinitialize() {
    if (parentPlugin_ && parentPlugin_->mainFunc && initialized && pluginInstanceID) {
        callPluginInstance(FF_DEINITIALISE, FFGLUtils::UIntToFFMixed(0));
        parentPlugin_->callPlugin(FF_DEINSTANTIATE_GL, FFGLUtils::UIntToFFMixed(0), pluginInstanceID);
        initialized = false;
        pluginInstanceID = nullptr;
    }
}

bool FFGLPluginInstance::processFrame(const std::vector<FFGLFramebuffer> &inputFBOs,
                                      const FFGLFramebuffer &outputFBO) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) return false;
    if (!outputFBO.isValid()) return false;

    GLint maxTexUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

    ActiveGUIStateProtector stateProtector(maxTexUnits);
    stateProtector.saveGUIState();

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO.fbo);
    glViewport(0, 0, outputFBO.width, outputFBO.height);

    glActiveTexture(GL_TEXTURE0);
    if (!inputFBOs.empty() && inputFBOs[0].isValid()) {
        glBindTexture(GL_TEXTURE_2D, inputFBOs[0].colorTexture);
    }

    glEnable(GL_TEXTURE_2D);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::vector<FFGLTextureStruct> inputTextures;
    std::vector<FFGLTextureStruct *> inputTexturePointers;

    for (const auto &fbo: inputFBOs) {
        if (fbo.isValid()) {
            FFGLTextureStruct texStruct;
            texStruct.Handle = fbo.colorTexture;
            texStruct.Width = fbo.width;
            texStruct.Height = fbo.height;
            texStruct.HardwareWidth = fbo.width;
            texStruct.HardwareHeight = fbo.height;
            inputTextures.push_back(texStruct);
        }
    }

    for (auto &tex: inputTextures) {
        inputTexturePointers.push_back(&tex);
    }

    ProcessOpenGLStruct processStruct;
    processStruct.numInputTextures = static_cast<FFUInt32>(inputTexturePointers.size());
    processStruct.inputTextures = inputTexturePointers.data();
    processStruct.HostFBO = outputFBO.fbo;

    FFMixed result = callPluginInstance(FF_PROCESS_OPENGL, FFGLUtils::PointerToFFMixed(&processStruct));

    if (result.UIntValue != FF_SUCCESS) {
        processStruct.HostFBO = 0;
        result = callPluginInstance(FF_PROCESS_OPENGL, FFGLUtils::PointerToFFMixed(&processStruct));
    }

    stateProtector.restoreGUIState();

    return result.UIntValue == FF_SUCCESS;
}

void FFGLPluginInstance::setParameter(FFUInt32 paramIndex, FFMixed value) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return;
    }

    const auto& param = parameters[paramIndex];

    // Add range clamping for numeric parameters
    if (param.type != FF_TYPE_TEXT && param.type != FF_TYPE_FILE &&
        param.type != FF_TYPE_EVENT && param.type != FF_TYPE_OPTION) {

        float floatValue = FFGLUtils::FFMixedToFloat(value);
        floatValue = std::max(param.range.min, std::min(param.range.max, floatValue));
        value = FFGLUtils::FloatToFFMixed(floatValue);
    }

    SetParameterStruct paramData;
    paramData.ParameterNumber = paramIndex;
    paramData.NewParameterValue = value;

    callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));
    parameters[paramIndex].currentValue = value;
}

void FFGLPluginInstance::setParameter(FFUInt32 paramIndex, float value) {
    if (paramIndex >= parameters.size()) return;

    const auto &param = parameters[paramIndex];
    value = std::max(param.range.min, std::min(param.range.max, value));

    setParameter(paramIndex, FFGLUtils::FloatToFFMixed(value));
}

void FFGLPluginInstance::setParameter(FFUInt32 paramIndex, FFUInt32 value) {
    setParameter(paramIndex, FFGLUtils::UIntToFFMixed(value));
}

void FFGLPluginInstance::setParameterText(FFUInt32 paramIndex, const std::string &text) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return;
    }

    if (!parameters[paramIndex].isTextParameter() && !parameters[paramIndex].isFileParameter()) {
        std::cerr << "Parameter " << paramIndex << " is not a text/file parameter" << std::endl;
        return;
    }

    parameters[paramIndex].textValue = text;
    setParameter(paramIndex, FFGLUtils::PointerToFFMixed(const_cast<char *>(text.c_str())));
}

void FFGLPluginInstance::setParameterElementValue(FFUInt32 paramIndex, FFUInt32 elementIndex, FFMixed value) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return;
    }

    if (!parameters[paramIndex].isOptionParameter()) {
        std::cerr << "Parameter " << paramIndex << " is not an option parameter" << std::endl;
        return;
    }

    SetParameterElementValueStruct elementData;
    elementData.ParameterNumber = paramIndex;
    elementData.ElementNumber = elementIndex;
    elementData.NewParameterValue = value;

    callPluginInstance(FF_SET_PARAMETER_ELEMENT_VALUE, FFGLUtils::PointerToFFMixed(&elementData));
}

FFMixed FFGLPluginInstance::getParameter(FFUInt32 paramIndex) const {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        FFMixed result;
        result.UIntValue = 0;
        return result;
    }

    FFMixed result = callPluginInstance(FF_GET_PARAMETER, FFGLUtils::UIntToFFMixed(paramIndex));
    if (result.UIntValue != FF_FAIL) {
        return result;
    }

    return parameters[paramIndex].currentValue;
}

float FFGLPluginInstance::getParameterFloat(FFUInt32 paramIndex) const {
    if (paramIndex >= parameters.size()) return 0.0f;

    FFMixed result = getParameter(paramIndex);

    if (parameters[paramIndex].type == FF_TYPE_OPTION) {
        // Option parameters are stored as floats but represent integer choices
// After setting the select parameter to 2, test what the plugin internally reports:
        FFMixed result = callPluginInstance(FF_GET_PARAMETER, FFGLUtils::UIntToFFMixed(9)); // select parameter
        if (result.UIntValue != FF_FAIL) {
            float pluginInternalValue = FFGLUtils::FFMixedToFloat(result);
        }

        float floatValue = FFGLUtils::FFMixedToFloat(result);
        return floatValue;  // This should be 2.0, not 1065353216
    } else {
        return FFGLUtils::FFMixedToFloat(result);
    }
}

FFUInt32 FFGLPluginInstance::getParameterInt(FFUInt32 paramIndex) const {
    return FFGLUtils::FFMixedToUInt(getParameter(paramIndex));
}

std::string FFGLPluginInstance::getParameterText(FFUInt32 paramIndex) const {
    if (paramIndex >= parameters.size()) {
        return "";
    }

    if (parameters[paramIndex].isTextParameter() || parameters[paramIndex].isFileParameter()) {
        return parameters[paramIndex].textValue;
    }

    return parameters[paramIndex].getDisplayValue();
}

std::string FFGLPluginInstance::getParameterDisplay(FFUInt32 paramIndex) const {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return "";
    }

    FFMixed result = callPluginInstance(FF_GET_PARAMETER_DISPLAY, FFGLUtils::UIntToFFMixed(paramIndex));

    if (result.PointerValue != nullptr) {
        return static_cast<char *>(result.PointerValue);
    }

    return parameters[paramIndex].getDisplayValue();
}

std::string FFGLPluginInstance::getParameterDisplayName(FFUInt32 paramIndex) const {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return "";
    }

    FFMixed result = callPluginInstance(FF_GET_PARAM_DISPLAY_NAME, FFGLUtils::UIntToFFMixed(paramIndex));

    if (result.PointerValue != nullptr) {
        return static_cast<char *>(result.PointerValue);
    }

    return parameters[paramIndex].displayName.empty() ? parameters[paramIndex].name
                                                      : parameters[paramIndex].displayName;
}

RangeStruct FFGLPluginInstance::getParameterRange(FFUInt32 paramIndex) const {
    if (paramIndex >= parameters.size()) {
        return FFGLUtils::getParameterDefaultRange(FF_TYPE_STANDARD);
    }

    return parameters[paramIndex].range;
}

bool FFGLPluginInstance::getParameterVisibility(FFUInt32 paramIndex) const {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return true;
    }

    FFMixed result = callPluginInstance(FF_GET_PRAMETER_VISIBILITY, FFGLUtils::UIntToFFMixed(paramIndex));
    return result.UIntValue != FF_FALSE;
}

std::vector<ParamEventStruct> FFGLPluginInstance::getParameterEvents() {
    std::vector<ParamEventStruct> events;

    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return events;
    }

    GetParamEventsStruct eventQuery;
    eventQuery.numEvents = 0;
    eventQuery.events = nullptr;

    FFMixed result = callPluginInstance(FF_GET_PARAMETER_EVENTS, FFGLUtils::PointerToFFMixed(&eventQuery));
    if (result.UIntValue == FF_FAIL || eventQuery.numEvents == 0) {
        return events;
    }

    events.resize(eventQuery.numEvents);
    eventQuery.events = events.data();

    result = callPluginInstance(FF_GET_PARAMETER_EVENTS, FFGLUtils::PointerToFFMixed(&eventQuery));
    if (result.UIntValue == FF_FAIL) {
        events.clear();
    }

    return events;
}

void FFGLPluginInstance::setTime(float time) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return;
    }

    callPluginInstance(FF_SET_TIME, FFGLUtils::FloatToFFMixed(time));
}

void FFGLPluginInstance::setBeatInfo(float bpm, float barPhase) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return;
    }

    SetBeatinfoStruct beatInfo;
    beatInfo.bpm = bpm;
    beatInfo.barPhase = barPhase;

    callPluginInstance(FF_SET_BEATINFO, FFGLUtils::PointerToFFMixed(&beatInfo));
}

bool FFGLPluginInstance::resize(const FFGLViewportStruct &viewport) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return false;
    }

    if (viewport.width == currentViewport.width && viewport.height == currentViewport.height &&
        viewport.x == currentViewport.x && viewport.y == currentViewport.y) {
        return true;
    }

    FFMixed result = callPluginInstance(FF_RESIZE,
                                        FFGLUtils::PointerToFFMixed(const_cast<FFGLViewportStruct *>(&viewport)));

    if (result.UIntValue == FF_SUCCESS) {
        currentViewport = viewport;
        return true;
    }

    return false;
}

bool FFGLPluginInstance::connect(FFUInt32 inputIndex) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return false;
    }

    FFMixed result = callPluginInstance(FF_CONNECT, FFGLUtils::UIntToFFMixed(inputIndex));
    return result.UIntValue == FF_SUCCESS;
}

bool FFGLPluginInstance::disconnect(FFUInt32 inputIndex) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return false;
    }

    FFMixed result = callPluginInstance(FF_DISCONNECT, FFGLUtils::UIntToFFMixed(inputIndex));
    return result.UIntValue == FF_SUCCESS;
}

FFUInt32 FFGLPluginInstance::getInputStatus(FFUInt32 inputIndex) const {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return FF_INPUT_NOTINUSE;
    }

    FFMixed result = callPluginInstance(FF_GET_INPUT_STATUS, FFGLUtils::UIntToFFMixed(inputIndex));
    return (result.UIntValue != FF_FAIL) ? result.UIntValue : FF_INPUT_NOTINUSE;
}

void FFGLPluginInstance::copyParametersFromTemplate() {
    parameters = parentPlugin_->parameterTemplates;
}

FFMixed FFGLPluginInstance::callPluginInstance(FFUInt32 functionCode, FFMixed inputValue) const {
    FFMixed failResult;
    failResult.UIntValue = FF_FAIL;

    if (!parentPlugin_ || !parentPlugin_->mainFunc || !pluginInstanceID) {
        std::cerr << "Invalid state for plugin call" << std::endl;
        return failResult;
    }

    try {
        FFMixed result = parentPlugin_->mainFunc(functionCode, inputValue, pluginInstanceID);
        return result;
    } catch (...) {
        std::cerr << "Exception caught in plugin function call!" << std::endl;
        return failResult;
    }
}

void FFGLPluginInstance::setAudioBuffer(FFUInt32 paramIndex, const float* audioData, size_t sampleCount) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized || paramIndex >= parameters.size()) {
        return;
    }
    
    if (!parameters[paramIndex].isBufferParameter()) {
        std::cerr << "Parameter " << paramIndex << " is not a buffer parameter" << std::endl;
        return;
    }
    
    // Check if this is an audio buffer parameter
    if (parameters[paramIndex].usage != FF_USAGE_STANDARD) {
        std::cerr << "Parameter " << paramIndex << " is not an audio buffer (usage: " 
                  << parameters[paramIndex].usage << ")" << std::endl;
        return;
    }
    
    // Set buffer data
    parameters[paramIndex].setBufferData(audioData, sampleCount * sizeof(float));
    
    // Send buffer to plugin
    SetParameterStruct paramData;
    paramData.ParameterNumber = paramIndex;
    paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(const_cast<float*>(audioData));
    
    callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));
}

void FFGLPluginInstance::sendAudioData(const float* fftData, size_t binCount) {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return;
    }

    bool foundFFTParam = false;
    bool foundAudioParam = false;

    // Look for both FFT and general audio buffer parameters
    for (auto& param : parameters) {
        if (param.isBufferParameter()) {
            if (param.usage == FF_USAGE_FFT) {

                // Send FFT data
                if (sendFFTDataToParameter(param.index, fftData, binCount)) {
                    foundFFTParam = true;
                }

            } else if (param.usage == FF_USAGE_STANDARD) {

                // Send raw audio data
                if (sendAudioDataToParameter(param.index)) {
                    foundAudioParam = true;
                }
            }
        }
    }
}

void FFGLPluginInstance::updateBufferParameters() {
    if (!parentPlugin_ || !parentPlugin_->mainFunc || !initialized) {
        return;
    }
    
    for (auto& param : parameters) {
        if (param.isBufferParameter() && param.bufferNeedsUpdate && param.bufferData) {
            SetParameterStruct paramData;
            paramData.ParameterNumber = param.index;
            paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(param.bufferData);
            
            callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));
            param.bufferNeedsUpdate = false;
        }
    }
}

// FFGLHost Implementation
FFGLHost::FFGLHost(
const std::string &name,
const std::string &version)
: hostName(name), hostVersion(version), sampleRate(44100.0f)
{
}

FFGLHost::~FFGLHost()
{
    unloadAllPlugins();
}

void FFGLHost::setHostInfo(const std::string &name, const std::string &version) {
    hostName = name;
    hostVersion = version;

    for (auto &pair: loadedPlugins) {
        pair.second->setHostInfo(hostName, hostVersion);
    }
}

void FFGLHost::setSampleRate(float rate) {
    sampleRate = rate;

    for (auto &pair: loadedPlugins) {
        pair.second->setSampleRate(sampleRate);
    }
}

bool FFGLHost::loadPlugin(const std::string& pluginPath) {
    if (loadedPlugins.find(pluginPath) != loadedPlugins.end()) {
        std::cout << "Plugin already loaded: " << pluginPath << std::endl;
        return true;
    }

    auto plugin = std::make_shared<FFGLPlugin>();
    if (!plugin->load(pluginPath)) {
        std::cerr << "Failed to load plugin: " << pluginPath << std::endl;
        return false;
    }

    plugin->setHostInfo(hostName, hostVersion);
    plugin->setSampleRate(sampleRate);

    // Store the plugin in the map - this keeps it alive
    loadedPlugins[pluginPath] = plugin;

    std::cout << "Successfully loaded and stored plugin: " << plugin->getDisplayName() << std::endl;

    // Pre-warm instance pool with 1 initialized instance for instant availability
    plugin->prewarmInstancePool(1);

    return true;
}

void FFGLHost::unloadPlugin(const std::string& pluginPath) {
    auto it = loadedPlugins.find(pluginPath);
    if (it != loadedPlugins.end()) {
        // Clear the instance pool before removing the plugin
        it->second->clearInstancePool();

        // Remove from map - this will destroy the plugin when no more references exist
        loadedPlugins.erase(it);
        std::cout << "Unloaded plugin: " << pluginPath << std::endl;
    }
}

void FFGLHost::unloadAllPlugins() {
    loadedPlugins.clear();
    std::cout << "All plugins unloaded" << std::endl;
}

std::vector<std::string> FFGLHost::getLoadedPluginPaths() const {
    std::vector<std::string> paths;
    for (const auto &pair: loadedPlugins) {
        paths.push_back(pair.first);
    }
    return paths;
}

std::shared_ptr<FFGLPlugin> FFGLHost::getPlugin(const std::string &pluginPath) {
    auto it = loadedPlugins.find(pluginPath);
    return (it != loadedPlugins.end()) ? it->second : nullptr;
}

std::shared_ptr<const FFGLPlugin> FFGLHost::getPlugin(const std::string &pluginPath) const {
    auto it = loadedPlugins.find(pluginPath);
    return (it != loadedPlugins.end()) ? it->second : nullptr;
}

void FFGLHost::listLoadedPlugins() const {
    std::cout << "Loaded FFGL plugins (" << loadedPlugins.size() << "):" << std::endl;

    for (const auto &pair: loadedPlugins) {
        const auto &info = pair.second->getInfo();
        const auto *extInfo = pair.second->getExtendedInfo();
        size_t instanceCount = pair.second->getInstanceCount();

        std::cout << "  " << pair.first << std::endl;
        std::cout << "    Name: " << std::string(info.PluginName, 16) << std::endl;
        std::cout << "    Short Name: " << pair.second->getShortName() << std::endl;
        std::cout << "    ID: " << std::string(info.PluginUniqueID, 4) << std::endl;
        std::cout << "    Type: " << FFGLUtils::getPluginTypeName(info.PluginType) << std::endl;
        std::cout << "    FFGL Version: " << FFGLUtils::getFFGLVersionString(info) << std::endl;
        std::cout << "    Instances: " << instanceCount << std::endl;

        if (extInfo) {
            std::cout << "    Plugin Version: " << extInfo->PluginMajorVersion << "." << extInfo->PluginMinorVersion
                      << std::endl;
            if (extInfo->Description) {
                std::cout << "    Description: " << extInfo->Description << std::endl;
            }
        }

        std::cout << "    Capabilities:" << std::endl;
        std::cout << "      Set Time: " << (pair.second->supportsCapability(FF_CAP_SET_TIME) ? "Yes" : "No")
                  << std::endl;
        std::cout << "      Min Input Frames: " << pair.second->getMinimumInputFrames() << std::endl;
        std::cout << "      Max Input Frames: " << pair.second->getMaximumInputFrames() << std::endl;
        std::cout << "      Top-Left Orientation: "
                  << (pair.second->supportsTopLeftTextureOrientation() ? "Yes" : "No") << std::endl;

        const auto &params = pair.second->getParameterTemplates();
        if (!params.empty()) {
            std::cout << "    Parameters:" << std::endl;
            for (const auto &param: params) {
                std::cout << "      " << param.index << ": " << param.name
                          << " (" << FFGLUtils::getParameterTypeName(param.type) << ")";

                if (!param.group.empty()) {
                    std::cout << " [Group: " << param.group << "]";
                }

                std::cout << " = " << param.getDisplayValue()
                          << " [" << param.range.min << "-" << param.range.max << "]" << std::endl;

                if (param.isOptionParameter() && !param.elements.empty()) {
                    std::cout << "        Options: ";
                    for (size_t i = 0; i < param.elements.size(); i++) {
                        if (i > 0) std::cout << ", ";
                        std::cout << param.elements[i].name;
                    }
                    std::cout << std::endl;
                }

                if (param.isFileParameter() && !param.fileExtensions.empty()) {
                    std::cout << "        Extensions: ";
                    for (size_t i = 0; i < param.fileExtensions.size(); i++) {
                        if (i > 0) std::cout << ", ";
                        std::cout << param.fileExtensions[i];
                    }
                    std::cout << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
}

size_t FFGLHost::getTotalInstanceCount() const {
    size_t total = 0;
    for (const auto &pair: loadedPlugins) {
        total += pair.second->getInstanceCount();
    }
    return total;
}

std::vector<std::string> FFGLHost::findPluginsInDirectory(const std::string &directory) {
    std::vector<std::string> plugins;

    try {
        if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
            std::vector<std::string> extensions = FFGLUtils::getPluginExtensions();

            for (const auto &entry: std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    std::string extension = entry.path().extension().string();

                    bool isPluginFile = false;
                    for (const auto &ext: extensions) {
                        if (extension == ext) {
                            isPluginFile = true;
                            break;
                        }
                    }

                    if (isPluginFile && FFGLUtils::isValidFFGLPlugin(path)) {
                        plugins.push_back(path);
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error scanning plugin directory: " << e.what() << std::endl;
    }

    return plugins;
}

void FFGLHost::setLogCallback(PFNLog logCallback) {
    // This would be called on all loaded plugins if we wanted to set logging for existing plugins
}

// Pool management for all loaded plugins
void FFGLHost::clearAllInstancePools() {
    for (auto& pair : loadedPlugins) {
        pair.second->clearInstancePool();
    }
    std::cout << "Cleared all FFGL instance pools" << std::endl;
}

// Get pool statistics
void FFGLHost::getPoolStatistics(size_t& totalPooled, size_t& totalActive) const {
    totalPooled = 0;
    totalActive = 0;
    for (const auto& pair : loadedPlugins) {
        totalPooled += pair.second->getPoolSize();
        totalActive += pair.second->getActiveInstanceCount();
    }
}

// Helper method to release an instance back to its plugin's pool
void FFGLHost::releaseInstance(FFGLInstanceHandle instance) {
    if (!instance) return;

    std::shared_ptr<FFGLPlugin> plugin = instance->getParentPlugin();
    if (plugin) {
        // Cast away const since we need to call releaseInstance
        plugin->releaseInstance(instance);
    }
}

// FFGLUtils Implementation
namespace FFGLUtils {
    std::string getParameterTypeName(FFUInt32 type) {
        switch (type) {
            case FF_TYPE_BOOLEAN:
                return "Boolean";
            case FF_TYPE_EVENT:
                return "Event";
            case FF_TYPE_RED:
                return "Red";
            case FF_TYPE_GREEN:
                return "Green";
            case FF_TYPE_BLUE:
                return "Blue";
            case FF_TYPE_XPOS:
                return "X Position";
            case FF_TYPE_YPOS:
                return "Y Position";
            case FF_TYPE_STANDARD:
                return "Standard";
            case FF_TYPE_OPTION:
                return "Option";
            case FF_TYPE_BUFFER:
                return "Buffer";
            case FF_TYPE_INTEGER:
                return "Integer";
            case FF_TYPE_FILE:
                return "File";
            case FF_TYPE_TEXT:
                return "Text";
            case FF_TYPE_HUE:
                return "Hue";
            case FF_TYPE_SATURATION:
                return "Saturation";
            case FF_TYPE_BRIGHTNESS:
                return "Brightness";
            case FF_TYPE_ALPHA:
                return "Alpha";
            default:
                return "Unknown (" + std::to_string(type) + ")";
        }
    }

    std::string getPluginTypeName(FFUInt32 type) {
        switch (type) {
            case FF_EFFECT:
                return "Effect";
            case FF_SOURCE:
                return "Source";
            case FF_MIXER:
                return "Mixer";
            default:
                return "Unknown (" + std::to_string(type) + ")";
        }
    }

    std::string getCapabilityName(FFUInt32 capability) {
        switch (capability) {
            case FF_CAP_SET_TIME:
                return "Set Time";
            case FF_CAP_MINIMUM_INPUT_FRAMES:
                return "Minimum Input Frames";
            case FF_CAP_MAXIMUM_INPUT_FRAMES:
                return "Maximum Input Frames";
            case FF_CAP_TOP_LEFT_TEXTURE_ORIENTATION:
                return "Top-Left Texture Orientation";
            default:
                return "Unknown (" + std::to_string(capability) + ")";
        }
    }

    bool isColorParameter(FFUInt32 type) {
        return type == FF_TYPE_RED || type == FF_TYPE_GREEN || type == FF_TYPE_BLUE ||
               type == FF_TYPE_HUE || type == FF_TYPE_SATURATION || type == FF_TYPE_BRIGHTNESS ||
               type == FF_TYPE_ALPHA;
    }

    bool isPositionParameter(FFUInt32 type) {
        return type == FF_TYPE_XPOS || type == FF_TYPE_YPOS;
    }

    bool isTextParameter(FFUInt32 type) {
        return type == FF_TYPE_TEXT;
    }

    bool isFileParameter(FFUInt32 type) {
        return type == FF_TYPE_FILE;
    }

    bool isEventParameter(FFUInt32 type) {
        return type == FF_TYPE_EVENT;
    }

    bool isBooleanParameter(FFUInt32 type) {
        return type == FF_TYPE_BOOLEAN;
    }

    bool isOptionParameter(FFUInt32 type) {
        return type == FF_TYPE_OPTION;
    }

    bool isBufferParameter(FFUInt32 type) {
        return type == FF_TYPE_BUFFER;
    }

    bool isIntegerParameter(FFUInt32 type) {
        return type == FF_TYPE_INTEGER;
    }

    RangeStruct getParameterDefaultRange(FFUInt32 type) {
        RangeStruct range;

        switch (type) {
            case FF_TYPE_BOOLEAN:
            case FF_TYPE_EVENT:
                range.min = 0.0f;
                range.max = 1.0f;
                break;
            case FF_TYPE_RED:
            case FF_TYPE_GREEN:
            case FF_TYPE_BLUE:
            case FF_TYPE_XPOS:
            case FF_TYPE_YPOS:
            case FF_TYPE_STANDARD:
            case FF_TYPE_HUE:
            case FF_TYPE_SATURATION:
            case FF_TYPE_BRIGHTNESS:
            case FF_TYPE_ALPHA:
                range.min = 0.0f;
                range.max = 1.0f;
                break;
            case FF_TYPE_INTEGER:
                range.min = 0.0f;
                range.max = 100.0f;
                break;
            case FF_TYPE_TEXT:
            case FF_TYPE_FILE:
            case FF_TYPE_OPTION:
            case FF_TYPE_BUFFER:
                range.min = 0.0f;
                range.max = 1.0f;
                break;
            default:
                range.min = 0.0f;
                range.max = 1.0f;
                break;
        }

        return range;
    }

    bool isValidFFGLPlugin(const std::string &pluginPath) {
        LibHandle lib = LoadLib(pluginPath.c_str());
        if (!lib) return false;

        bool valid = false;
        FF_Main_FuncPtr testMainFunc = (FF_Main_FuncPtr) GetProc(lib, "plugMain");

        if (testMainFunc) {
            FFMixed result = testMainFunc(FF_GET_INFO, UIntToFFMixed(0), nullptr);
            if (result.PointerValue != nullptr) {
                PluginInfoStruct *info = static_cast<PluginInfoStruct *>(result.PointerValue);
                valid = isFFGL2Compatible(*info);
            }
        }

        FreeLib(lib);
        return valid;
    }

    bool isFFGL2Compatible(const PluginInfoStruct &info) {
        return info.APIMajorVersion >= 2;
    }

    std::string getFFGLVersionString(const PluginInfoStruct &info) {
        return std::to_string(info.APIMajorVersion) + "." + std::to_string(info.APIMinorVersion);
    }

    std::vector<std::string> getPluginExtensions() {
        std::vector<std::string> extensions;

#ifdef _WIN32
        extensions.push_back(".dll");
        extensions.push_back(".ffgl");
#elif defined(__APPLE__)
        extensions.push_back(".dylib");
    extensions.push_back(".ffgl");
    extensions.push_back(".bundle");
#else
    extensions.push_back(".so");
    extensions.push_back(".ffgl");
#endif

        return extensions;
    }

    float FFMixedToFloat(FFMixed mixed) {
        return *reinterpret_cast<const float *>(&mixed.UIntValue);
    }

    FFMixed FloatToFFMixed(float value) {
        FFMixed mixed;
        mixed.UIntValue = *reinterpret_cast<const FFUInt32 *>(&value);
        return mixed;
    }

    FFUInt32 FFMixedToUInt(FFMixed mixed) {
        return mixed.UIntValue;
    }

    FFMixed UIntToFFMixed(FFUInt32 value) {
        FFMixed mixed;
        mixed.UIntValue = value;
        return mixed;
    }

    void *FFMixedToPointer(FFMixed mixed) {
        return mixed.PointerValue;
    }

    FFMixed PointerToFFMixed(void *pointer) {
        FFMixed mixed;
        mixed.PointerValue = pointer;
        return mixed;
    }
}


std::chrono::high_resolution_clock::time_point EffectTimer::startTime;
bool EffectTimer::initialized = false;

float EffectTimer::getTime() {
    if (!initialized) {
        startTime = std::chrono::high_resolution_clock::now();
        initialized = true;
    }

    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = now - startTime;
    return std::chrono::duration<float>(elapsed).count();
}


bool FFGLPlugin::validateParameterInfo(const FFGLParameter& param) {
    // Skip validation for non-numeric parameters
    if (param.type == FF_TYPE_TEXT || param.type == FF_TYPE_FILE ||
        param.type == FF_TYPE_EVENT || param.type == FF_TYPE_OPTION) {
        return true;
    }

    float defaultValue = FFGLUtils::FFMixedToFloat(param.defaultValue);

    // Check if default is within range
    if (defaultValue < param.range.min || defaultValue > param.range.max) {
        std::cout << "Warning: Parameter '" << param.name
                  << "' default value " << defaultValue
                  << " is outside range [" << param.range.min
                  << ", " << param.range.max << "]" << std::endl;
        return false;
    }

    return true;
}

void FFGLPlugin::fixParameterInfo(FFGLParameter& param) {
    // Skip non-numeric parameters
    if (param.type == FF_TYPE_TEXT || param.type == FF_TYPE_FILE ||
        param.type == FF_TYPE_EVENT || param.type == FF_TYPE_OPTION) {
        return;
    }

    float defaultValue = FFGLUtils::FFMixedToFloat(param.defaultValue);

    // Option 1: Clamp the default value to range
    if (defaultValue < param.range.min) {
        std::cout << "  Fixing: Clamping default from " << defaultValue
                  << " to " << param.range.min << std::endl;
        param.defaultValue = FFGLUtils::FloatToFFMixed(param.range.min);
        param.currentValue = param.defaultValue;
    } else if (defaultValue > param.range.max) {
        std::cout << "  Fixing: Clamping default from " << defaultValue
                  << " to " << param.range.max << std::endl;
        param.defaultValue = FFGLUtils::FloatToFFMixed(param.range.max);
        param.currentValue = param.defaultValue;
    }
}





// Method 1: Set FFT elements directly (this is what the SDK expects)
bool FFGLPluginInstance::setFFTElements(FFUInt32 paramIndex, const float* fftData, size_t binCount) {
    // First check if this parameter has elements
    FFMixed result = callPluginInstance(FF_GET_NUM_PARAMETER_ELEMENTS, FFGLUtils::UIntToFFMixed(paramIndex));

    if (result.UIntValue == FF_FAIL || result.UIntValue == 0) {
        printf("Parameter %d has no elements (result: %d)\n", paramIndex, result.UIntValue);
        return false;
    }

    FFUInt32 numElements = result.UIntValue;

    // Set individual elements for each FFT bin
    size_t elementsToSet = std::min(binCount, std::min(size_t(numElements), size_t(512)));

    for (size_t i = 0; i < elementsToSet; ++i) {
        SetParameterElementValueStruct elementStruct;
        elementStruct.ParameterNumber = paramIndex;
        elementStruct.ElementNumber = static_cast<FFUInt32>(i);
        elementStruct.NewParameterValue = FFGLUtils::FloatToFFMixed(fftData[i]);

        FFMixed elementResult = callPluginInstance(FF_SET_PARAMETER_ELEMENT_VALUE,
                                                   FFGLUtils::PointerToFFMixed(&elementStruct));

        if (elementResult.UIntValue == FF_FAIL) {
            if (i == 0) {
                printf("Failed to set first element, stopping\n");
                return false;
            } else {
                printf("Failed to set element %zu, continuing\n", i);
            }
        }
    }

    return true;
}

// Method 2: Set FFT buffer with proper structure
bool FFGLPluginInstance::setFFTBuffer(FFUInt32 paramIndex, const float* fftData, size_t binCount) {
    // Create a proper FFT data structure
    struct FFTBufferStruct {
        FFUInt32 numElements;
        const float* data;
        FFUInt32 sampleRate;
    };

    FFTBufferStruct fftBuffer;
    fftBuffer.numElements = static_cast<FFUInt32>(binCount);
    fftBuffer.data = fftData;
    fftBuffer.sampleRate = 44100; // Default sample rate

    SetParameterStruct paramData;
    paramData.ParameterNumber = paramIndex;
    paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(&fftBuffer);

    FFMixed result = callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));

    if (result.UIntValue == FF_SUCCESS) {
        printf("FFT buffer set successfully using structured approach\n");
        return true;
    }

    return false;
}

// Method 3: Legacy buffer approach
bool FFGLPluginInstance::setFFTLegacy(FFUInt32 paramIndex, const float* fftData, size_t binCount) {
    SetParameterStruct paramData;
    paramData.ParameterNumber = paramIndex;
    paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(const_cast<float*>(fftData));

    FFMixed result = callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));

    if (result.UIntValue == FF_SUCCESS) {
        printf("FFT buffer set successfully using legacy approach\n");
        return true;
    }

    return false;
}


// Simplified method to send FFT data to a parameter
bool FFGLPluginInstance::sendFFTDataToParameter(FFUInt32 paramIndex, const float* fftData, size_t binCount) {
    if (paramIndex >= parameters.size()) {
        return false;
    }

    // Method 1: Try setting individual elements (for FFGL SDK plugins with element arrays)
    FFMixed numElementsResult = callPluginInstance(FF_GET_NUM_PARAMETER_ELEMENTS,
                                                   FFGLUtils::UIntToFFMixed(paramIndex));

    if (numElementsResult.UIntValue != FF_FAIL && numElementsResult.UIntValue > 0) {
        FFUInt32 numElements = numElementsResult.UIntValue;
        size_t elementsToSet = numElements;

        bool success = true;
        for (size_t i = 0; i < elementsToSet; ++i) {
            SetParameterElementValueStruct elementStruct;
            elementStruct.ParameterNumber = paramIndex;
            elementStruct.ElementNumber = static_cast<FFUInt32>(i);
            elementStruct.NewParameterValue = FFGLUtils::FloatToFFMixed(fftData[i]);

            FFMixed result = callPluginInstance(FF_SET_PARAMETER_ELEMENT_VALUE,
                                                FFGLUtils::PointerToFFMixed(&elementStruct));

            if (result.UIntValue == FF_FAIL) {
                if (i == 0) {
                    printf("Failed to set first element, aborting element method\n");
                    success = false;
                    break;
                }
                printf("Warning: Failed to set element %zu\n", i);
            }
        }

        if (success) {
            return true;
        }
    }

    // Method 2: Try setting as buffer data directly
    printf("Trying direct buffer approach\n");

    SetParameterStruct paramData;
    paramData.ParameterNumber = paramIndex;
    paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(const_cast<float*>(fftData));

    FFMixed result = callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));

    if (result.UIntValue == FF_SUCCESS) {
        printf("Successfully set FFT data using direct buffer method\n");
        return true;
    }

    // Method 3: Try with buffer size information
    printf("Trying buffer with size information\n");

    struct FFTBuffer {
        const float* data;
        size_t size;
    };

    FFTBuffer buffer;
    buffer.data = fftData;
    buffer.size = binCount;

    paramData.NewParameterValue = FFGLUtils::PointerToFFMixed(&buffer);
    result = callPluginInstance(FF_SET_PARAMETER, FFGLUtils::PointerToFFMixed(&paramData));

    if (result.UIntValue == FF_SUCCESS) {
        printf("Successfully set FFT data using structured buffer method\n");
        return true;
    }

    printf("All FFT methods failed for parameter %d\n", paramIndex);
    return false;
}



// Store raw audio samples (called from audio thread)
void FFGLPluginInstance::storeAudioSamples(const float* audioSamples, size_t sampleCount) {
    std::lock_guard<std::mutex> lock(audioSamplesMutex);
    latestAudioSamples.assign(audioSamples, audioSamples + sampleCount);
    hasNewAudioSamples = true;
}

// Send raw audio samples to general buffer parameters
bool FFGLPluginInstance::sendAudioDataToParameter(FFUInt32 paramIndex) {
    std::lock_guard<std::mutex> lock(audioSamplesMutex);

    if (!hasNewAudioSamples || latestAudioSamples.empty()) {
        return false;
    }

    if (paramIndex >= parameters.size()) {
        return false;
    }

    // Check how many elements this parameter expects
    FFMixed numElementsResult = callPluginInstance(FF_GET_NUM_PARAMETER_ELEMENTS,
                                                   FFGLUtils::UIntToFFMixed(paramIndex));

    if (numElementsResult.UIntValue == FF_FAIL || numElementsResult.UIntValue == 0) {
        return false;
    }

    FFUInt32 numElements = numElementsResult.UIntValue;
    printf("Audio buffer parameter has %d elements, we have %zu samples\n",
           numElements, latestAudioSamples.size());

    // Prepare audio data for the buffer
    std::vector<float> audioBuffer;
    if (numElements > latestAudioSamples.size()) {
        // Interpolate/repeat audio samples to fill buffer
        audioBuffer.resize(numElements);
        for (FFUInt32 i = 0; i < numElements; i++) {
            float ratio = (float)i / (float)(numElements - 1);
            size_t sourceIndex = (size_t)(ratio * (latestAudioSamples.size() - 1));
            sourceIndex = std::min(sourceIndex, latestAudioSamples.size() - 1);
            audioBuffer[i] = latestAudioSamples[sourceIndex];
        }
    } else {
        // Use a subset of audio samples
        audioBuffer.assign(latestAudioSamples.begin(),
                           latestAudioSamples.begin() + std::min((size_t)numElements, latestAudioSamples.size()));
    }

    // Set all elements
    for (FFUInt32 i = 0; i < numElements && i < audioBuffer.size(); i++) {
        SetParameterElementValueStruct elementStruct;
        elementStruct.ParameterNumber = paramIndex;
        elementStruct.ElementNumber = i;
        elementStruct.NewParameterValue = FFGLUtils::FloatToFFMixed(audioBuffer[i]);

        FFMixed result = callPluginInstance(FF_SET_PARAMETER_ELEMENT_VALUE,
                                            FFGLUtils::PointerToFFMixed(&elementStruct));

        if (result.UIntValue == FF_FAIL && i == 0) {
            return false;
        }
    }

    return true;
}




