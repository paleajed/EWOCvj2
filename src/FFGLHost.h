#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

// Include official FFGL SDK header
#include <FFGL.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibHandle;
#else
#include <dlfcn.h>
    typedef void* LibHandle;
#endif

// VJ Application FBO Structure
struct FFGLFramebuffer {
    GLuint fbo;           // Framebuffer object ID
    GLuint colorTexture;  // Color attachment texture ID
    GLuint depthBuffer;   // Depth/stencil buffer ID (optional)
    FFUInt32 width;       // Framebuffer width
    FFUInt32 height;      // Framebuffer height

    FFGLFramebuffer() : fbo(0), colorTexture(0), depthBuffer(0), width(0), height(0) {}

    FFGLFramebuffer(GLuint f, GLuint tex, FFUInt32 w, FFUInt32 h, GLuint depth = 0)
            : fbo(f), colorTexture(tex), depthBuffer(depth), width(w), height(h) {}

    bool isValid() const {
        return colorTexture != 0 && width > 0 && height > 0;
    }
};

// Parameter element for FF_TYPE_OPTION parameters
struct FFGLParameterElement {
    std::string name;
    FFMixed value;

    FFGLParameterElement(const std::string& n, FFUInt32 val) : name(n) {
        value.UIntValue = val;
    }

    FFGLParameterElement(const std::string& n, float val) : name(n) {
        value.UIntValue = *reinterpret_cast<const FFUInt32*>(&val);
    }
};

class FFGLParameter {
public:
    FFUInt32 index;
    std::string name;
    std::string displayName;  // Dynamic display name (FFGL 2.3)
    FFUInt32 type;
    FFMixed defaultValue;
    FFMixed currentValue;
    RangeStruct range;
    std::string group;        // Parameter group (FFGL 2.2)
    bool visible;
    FFUInt32 usage;          // For FF_TYPE_BUFFER parameters

    // For FF_TYPE_OPTION parameters
    std::vector<FFGLParameterElement> elements;

    // For FF_TYPE_FILE parameters
    std::vector<std::string> fileExtensions;

    // For text/file parameters
    std::string textValue;

    FFGLParameter(FFUInt32 idx, const std::string& n, FFUInt32 t);

    // Type-specific value handling
    bool getBoolValue() const;
    void setBoolValue(bool value);
    float getFloatValue() const;
    void setFloatValue(float value);
    FFUInt32 getIntValue() const;
    void setIntValue(FFUInt32 value);

    std::string getDisplayValue() const;
    bool isTextParameter() const;
    bool isFileParameter() const;
    bool isColorParameter() const;
    bool isPositionParameter() const;
    bool isOptionParameter() const;
    bool isBufferParameter() const;
    bool isIntegerParameter() const;
};

// Add this helper class for GUI state protection
class GUIStateProtector {
private:
    // Shader state
    GLint savedProgram;

    // All texture unit bindings (up to maximum)
    std::vector<GLint> savedTexture2D;
    std::vector<GLint> savedTextureBuffer;
    GLint savedActiveTexture;
    int maxTexUnits;

    // Buffer bindings critical for GUI
    GLint savedVAO;
    GLint savedArrayBuffer;
    GLint savedElementArrayBuffer;
    GLint savedCurrentTextureBuffer;

    // Render state
    GLboolean savedBlendEnabled;
    GLint savedBlendSrc, savedBlendDst;
    GLboolean savedDepthTestEnabled;
    GLint savedDepthFunc;
    GLboolean savedDepthMask;

    // Framebuffer state
    GLint savedDrawFramebuffer;
    GLint savedReadFramebuffer;
    GLint savedDrawBuffer;
    GLint savedViewport[4];

public:
    GUIStateProtector(int maxTextureUnits);
    void saveGUIState();
    void restoreGUIState();
};

// Minimal GUIStateProtector - saves only critical state
class MinimalGUIStateProtector {
private:
    // Only the most critical state for your GUI
    GLint savedProgram;
    GLint savedActiveTexture;
    GLint savedVAO;
    GLint savedArrayBuffer;
    GLint savedElementArrayBuffer;
    GLint savedTextureBuffer;

    // Just save a few key texture units that your GUI uses most
    GLint savedTexture2D_0;  // GL_TEXTURE0
    GLint savedTexture2D_high1;  // GL_TEXTURE0 + maxTexUnits - 2
    GLint savedTexture2D_high2;  // GL_TEXTURE0 + maxTexUnits - 1
    GLint savedTextureBuffer_high1;
    GLint savedTextureBuffer_high2;

    int maxTexUnits;

public:
    MinimalGUIStateProtector(int maxTextureUnits);
    void saveGUIState();
    void restoreGUIState();
};

// Ultra-minimal version - only shader program and buffers
class UltraMinimalGUIStateProtector {
private:
    GLint savedProgram;
    GLint savedVAO;
    GLint savedArrayBuffer;
    GLint savedElementArrayBuffer;

public:
    UltraMinimalGUIStateProtector(int maxTextureUnits);
    void saveGUIState();
    void restoreGUIState();
};

// Easy way to swap between protectors using typedefs
#ifdef USE_MINIMAL_PROTECTION
typedef MinimalGUIStateProtector ActiveGUIStateProtector;
#elif defined(USE_ULTRA_MINIMAL_PROTECTION)
typedef UltraMinimalGUIStateProtector ActiveGUIStateProtector;
#else
typedef GUIStateProtector ActiveGUIStateProtector;
#endif



// Forward declarations
class FFGLPlugin;
class FFGLPluginInstance;

// Plugin instance handle for managing multiple instances
typedef std::shared_ptr<FFGLPluginInstance> FFGLInstanceHandle;

class FFGLPlugin {
private:
    LibHandle library;
    FF_Main_FuncPtr mainFunc;
    FF_SetLogCallback_FuncPtr setLogCallbackFunc;
    PluginExtendedInfoStruct* extendedInfo;
    std::vector<FFGLParameter> parameterTemplates;
    bool loaded;
    FFUInt32 numParameters;

    // CHANGE: Use plugin's internal instance ID as key
    std::map<FFInstanceID, std::weak_ptr<FFGLPluginInstance>> instances;

public:
    std::string pluginPath;
    PluginInfoStruct pluginInfo;

    FFGLPlugin();
    ~FFGLPlugin();

    // Plugin loading and info
    bool load(const std::string& pluginPath);
    void unload();
    bool isLoaded() const { return loaded; }

    const PluginInfoStruct& getInfo() const { return pluginInfo; }
    const PluginExtendedInfoStruct* getExtendedInfo() const { return extendedInfo; }
    const std::vector<FFGLParameter>& getParameterTemplates() const { return parameterTemplates; }
    const std::string& getPath() const { return pluginPath; }
    FFUInt32 getParameterCount() const { return numParameters; }
    std::string getDisplayName() const;
    std::string getShortName() const;

    // Capabilities
    bool supportsCapability(FFUInt32 capability) const;
    FFUInt32 getMinimumInputFrames() const;
    FFUInt32 getMaximumInputFrames() const;
    bool supportsTopLeftTextureOrientation() const;

    // Instance management
    void destroyInstance(FFInstanceID instanceID);
    size_t getInstanceCount() const;

    // Thumbnail support (FFGL 2.1)
    bool getThumbnail(FFUInt32& width, FFUInt32& height, std::vector<FFUInt32>& rgbaPixels) const;

    // Host info
    void setHostInfo(const std::string& hostName, const std::string& hostVersion);
    void setSampleRate(float sampleRate);

    FFGLInstanceHandle createInstance(FFUInt32 width, FFUInt32 height);
    FFGLInstanceHandle createInstance(const FFGLViewportStruct& viewport);

private:
    void loadParameterTemplates();
    bool validatePlugin();
    bool queryParameterInfo(FFUInt32 paramIndex, FFGLParameter& param);
    bool isValidStringPointer(void* ptr) const;
    bool supportsFFGL22Features();
    void queryParameterElements(FFUInt32 paramIndex, FFGLParameter& param);
    void queryFileParameterExtensions(FFUInt32 paramIndex, FFGLParameter& param);
    FFMixed callPlugin(FFUInt32 functionCode, FFMixed inputValue, FFInstanceID instanceID = nullptr) const;

    friend class FFGLPluginInstance;
};

class FFGLPluginInstance {
private:
    FFGLPlugin* parentPlugin;

    // CHANGE: Rename instanceID to pluginInstanceID to be clearer
    FFInstanceID pluginInstanceID;  // This is the plugin's internal instance pointer

    bool initialized;
    FFGLViewportStruct currentViewport;

public:
    std::vector<FFGLParameter> parameters;

    FFGLPluginInstance(FFGLPlugin* parent, const FFGLViewportStruct& viewport);
    ~FFGLPluginInstance();

    // Instance lifecycle
    bool initialize(const FFGLViewportStruct& viewport);
    void deinitialize();
    bool isInitialized() const { return initialized; }

    // ADD THIS METHOD:
    FFInstanceID getPluginInstanceID() const { return pluginInstanceID; }

    // Frame processing
    bool processFrame(const std::vector<FFGLFramebuffer>& inputFBOs,
                      const FFGLFramebuffer& outputFBO);

    // Parameter management
    void setParameter(FFUInt32 paramIndex, FFMixed value);
    void setParameter(FFUInt32 paramIndex, float value);
    void setParameter(FFUInt32 paramIndex, FFUInt32 value);
    void setParameterText(FFUInt32 paramIndex, const std::string& text);
    void setParameterElementValue(FFUInt32 paramIndex, FFUInt32 elementIndex, FFMixed value);
    void triggerEvent(FFUInt32 paramIndex);

    FFMixed getParameter(FFUInt32 paramIndex) const;
    float getParameterFloat(FFUInt32 paramIndex) const;
    FFUInt32 getParameterInt(FFUInt32 paramIndex) const;
    std::string getParameterText(FFUInt32 paramIndex) const;
    std::string getParameterDisplay(FFUInt32 paramIndex) const;
    std::string getParameterDisplayName(FFUInt32 paramIndex) const; // FFGL 2.3
    RangeStruct getParameterRange(FFUInt32 paramIndex) const;
    bool getParameterVisibility(FFUInt32 paramIndex) const;

    const std::vector<FFGLParameter>& getParameters() const { return parameters; }

    // Parameter events (for dynamic parameters)
    std::vector<ParamEventStruct> getParameterEvents();

    // Utility
    void setTime(float time);
    void setBeatInfo(float bpm, float barPhase);
    bool resize(const FFGLViewportStruct& viewport);
    bool connect(FFUInt32 inputIndex);
    bool disconnect(FFUInt32 inputIndex);
    FFUInt32 getInputStatus(FFUInt32 inputIndex) const;

    // CHANGE: Update getter method name
    FFInstanceID getInstanceID() const { return pluginInstanceID; }
    const FFGLPlugin* getParentPlugin() const { return parentPlugin; }
    const FFGLViewportStruct& getViewport() const { return currentViewport; }

private:
    void copyParametersFromTemplate();
    void updateParameterFromEvents();
    FFMixed callPluginInstance(FFUInt32 functionCode, FFMixed inputValue) const;
};

class FFGLHost {
private:
    std::map<std::string, std::unique_ptr<FFGLPlugin>> loadedPlugins;
    std::string hostName;
    std::string hostVersion;
    float sampleRate;

public:
    FFGLHost(const std::string& name = "FFGL Host", const std::string& version = "1.0");
    ~FFGLHost();

    // Host configuration
    void setHostInfo(const std::string& name, const std::string& version);
    void setSampleRate(float rate);

    // Plugin management
    bool loadPlugin(const std::string& pluginPath);
    void unloadPlugin(const std::string& pluginPath);
    void unloadAllPlugins();

    // Plugin queries
    std::vector<std::string> getLoadedPluginPaths() const;
    FFGLPlugin* getPlugin(const std::string& pluginPath);
    const FFGLPlugin* getPlugin(const std::string& pluginPath) const;

    // Utility functions
    void listLoadedPlugins() const;
    size_t getTotalInstanceCount() const;

    // Plugin discovery
    static std::vector<std::string> findPluginsInDirectory(const std::string& directory);

    // Logging support (FFGL 2.2)
    static void setLogCallback(PFNLog logCallback);
};

// Utility functions
namespace FFGLUtils {
    std::string getParameterTypeName(FFUInt32 type);
    std::string getPluginTypeName(FFUInt32 type);
    std::string getCapabilityName(FFUInt32 capability);
    bool isValidFFGLPlugin(const std::string& pluginPath);

    // FFGL version checking
    bool isFFGL2Compatible(const PluginInfoStruct& info);
    std::string getFFGLVersionString(const PluginInfoStruct& info);

    // Parameter type checking utilities
    bool isColorParameter(FFUInt32 type);
    bool isPositionParameter(FFUInt32 type);
    bool isTextParameter(FFUInt32 type);
    bool isFileParameter(FFUInt32 type);
    bool isEventParameter(FFUInt32 type);
    bool isBooleanParameter(FFUInt32 type);
    bool isOptionParameter(FFUInt32 type);
    bool isBufferParameter(FFUInt32 type);
    bool isIntegerParameter(FFUInt32 type);

    // Parameter range utilities
    RangeStruct getParameterDefaultRange(FFUInt32 type);

    // Plugin file detection
    std::vector<std::string> getPluginExtensions();

    // FFMixed utilities
    float FFMixedToFloat(FFMixed mixed);
    FFMixed FloatToFFMixed(float value);
    FFUInt32 FFMixedToUInt(FFMixed mixed);
    FFMixed UIntToFFMixed(FFUInt32 value);
    void* FFMixedToPointer(FFMixed mixed);
    FFMixed PointerToFFMixed(void* pointer);
}


class EffectTimer {
private:
    static std::chrono::high_resolution_clock::time_point startTime;
    static bool initialized;

public:
    static float getTime();
};



