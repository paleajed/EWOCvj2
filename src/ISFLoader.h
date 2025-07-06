// ISFLoader.h
#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <chrono>

// Forward declarations
class ISFShader;
class ISFShaderInstance;

class ISFLoader {
public:
    enum ShaderType {
        GENERATOR,
        EFFECT
    };

    enum ParamType {
        PARAM_FLOAT = 1000,
        PARAM_LONG,
        PARAM_BOOL,
        PARAM_EVENT,
        PARAM_COLOR,
        PARAM_POINT2D,
        PARAM_RED,
        PARAM_GREEN,
        PARAM_BLUE,
        PARAM_ALPHA
    };

    enum InputType {
        INPUT_IMAGE,
        INPUT_EXTERNAL_IMAGE,
        INPUT_AUDIO_FFT,
        INPUT_AUDIO
    };

    struct ParamInfo {
        std::string name;
        std::string label;
        ParamType type;

        // Value ranges and defaults
        float minVal = 0.0f;
        float maxVal = 1.0f;
        float defaultVal = 0.0f;

        // For colors (RGBA)
        float defaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

        // For point2D (XY)
        float defaultPoint[2] = {0.5f, 0.5f};
        float minPoint[2] = {0.0f, 0.0f};       // Min X, Y for point2D
        float maxPoint[2] = {1.0f, 1.0f};       // Max X, Y for point2D

        // For long/int types
        int defaultInt = 0;
        int minInt = 0;
        int maxInt = 100;

        // NEW: For discrete VALUES and LABELS (enum-style parameters)
        std::vector<int> values;              // Discrete values (e.g., [0, 1, 2, 3])
        std::vector<std::string> labels;      // Corresponding labels (e.g., ["Left", "Right", "Up", "Down"])
        bool hasDiscreteValues = false;       // Whether this parameter uses VALUES/LABELS

        // Helper method to get label for a value
        std::string getLabelForValue(int value) const {
            for (size_t i = 0; i < values.size() && i < labels.size(); ++i) {
                if (values[i] == value) {
                    return labels[i];
                }
            }
            return std::to_string(value); // Fallback to numeric value
        }

        // Helper method to get value for a label
        int getValueForLabel(const std::string& label) const {
            for (size_t i = 0; i < labels.size() && i < values.size(); ++i) {
                if (labels[i] == label) {
                    return values[i];
                }
            }
            return defaultInt; // Fallback to default
        }

        // Check if a value is valid for this parameter
        bool isValidValue(int value) const {
            if (!hasDiscreteValues) return true; // No restriction
            return std::find(values.begin(), values.end(), value) != values.end();
        }
    };

    struct InputInfo {
        std::string name;
        InputType type;
        std::string filepath;  // For external images
        GLuint textureId = 0;  // Loaded texture
        int width = 0;
        int height = 0;
    };

    struct PassInfo {
        std::string target;           // Buffer name (empty for final pass)
        bool persistent = false;      // Whether buffer persists between frames
        std::string widthExpr;        // Width expression (e.g., "$WIDTH/16.0")
        std::string heightExpr;       // Height expression (e.g., "$HEIGHT/16.0")

        // Runtime calculated values (but NO OpenGL resources here)
        int width = 0;
        int height = 0;

        // REMOVED: These are now per-instance
        // GLuint framebuffer = 0;
        // GLuint texture = 0;
    };

    struct InstancePassInfo {
        int width = 0;
        int height = 0;
        GLuint framebuffer = 0;
        GLuint texture = 0;

        // NEW: Double buffering members
        GLuint framebufferPing = 0;
        GLuint texturePing = 0;
        bool useDoubleBuffer = false;
        bool currentPingPong = false;

        // NEW: Helper methods (add these to the struct)
        GLuint getCurrentReadTexture() const {
            if (!useDoubleBuffer) return texture;
            return currentPingPong ? texture : texturePing;
        }

        GLuint getCurrentWriteTexture() const {
            if (!useDoubleBuffer) return texture;
            return currentPingPong ? texturePing : texture;
        }

        GLuint getCurrentWriteFramebuffer() const {
            if (!useDoubleBuffer) return framebuffer;
            return currentPingPong ? framebufferPing : framebuffer;
        }

        void swapBuffers() {
            if (useDoubleBuffer) {
                currentPingPong = !currentPingPong;
            }
        }

        ~InstancePassInfo();
    };

    // Union to store parameter values efficiently
    union ParamValue {
        float floatVal;
        int intVal;
        float colorVal[4];
        float pointVal[2];

        ParamValue() : floatVal(0.0f) {}
    };

private:
    // Shader cache implementation
    struct CacheEntry {
        std::string vertexSource;
        std::string fragmentSource;
        std::vector<char> binaryData;
        GLenum binaryFormat;
        std::string sourceHash;
        uint64_t timestamp;
        bool isValid = false;
    };

    std::unordered_map<std::string, CacheEntry> shaderCache_;
    std::string cacheDirectory_;

    std::vector<std::unique_ptr<ISFShader>> shaders_;
    bool devilInitialized_ = false;

    // Vertex shader for ISF (standard quad)
    static const char* vertexShaderSource_;

    // ISF built-in functions
    static const char* isfBuiltinFunctions_;

    // ISF vertex shader built-in functions
    static const char* isfVertexBuiltins_;

    // Private helper methods
    bool loadISFFile(const std::string& filepath);
    bool parseISFJson(const std::string& fileContent, ISFShader& shader);
    bool extractISFComponents(const std::string& fileContent, std::string& jsonContent, std::string& fragmentSource);
    bool validateISFStructure(const json& root);
    bool parseParameters(const json& inputs, ISFShader& shader);
    bool parseInputs(const json& inputs, ISFShader& shader);
    bool parseImported(const json& imported, ISFShader& shader);
    bool parsePasses(const json& passes, ISFShader& shader);
    bool loadExternalImage(const std::string& imagePath, InputInfo& inputInfo, const std::string& baseDir);
    bool compileShader(const std::string& fragmentSource, ISFShader& shader);
    void cacheUniformLocations(ISFShader& shader);
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
    void logShaderError(GLuint shader, const std::string& type);
    void logProgramError(GLuint program);
    ParamType stringToParamType(const std::string& typeStr);

    // Shader cache methods
    std::string getUserDataDirectory();
    void initializeShaderCache();
    std::string generateHash(const std::string& vertexSource, const std::string& fragmentSource);
    bool isCacheValid(const CacheEntry& entry, const std::string& currentHash);
    GLuint loadCachedProgram(const std::string& shaderName,
                             const std::string& vertexSource,
                             const std::string& fragmentSource);
    void cacheProgram(const std::string& shaderName,
                      const std::string& vertexSource,
                      const std::string& fragmentSource,
                      GLuint program);
    bool validateCacheEnvironment();
    void loadCacheFromDisk();
    void saveCacheToDisk();
    void showFirstRunMessage();

public:
    ISFLoader();
    ~ISFLoader();

    // Main loading function
    bool loadISFDirectory(const std::string& directory);
    bool isFirstRun_ = false;  // Add this line

    // Access loaded shaders (templates)
    const std::vector<std::unique_ptr<ISFShader>>& getShaders() const { return shaders_; }
    size_t getShaderCount() const { return shaders_.size(); }

    // Get shader template by index
    ISFShader* getShader(size_t index) const {
        if (index < shaders_.size()) {
            return shaders_[index].get();
        }
        return nullptr;
    }

    // Find shader template by name
    ISFShader* findShader(const std::string& name) const;

    // Get all shader names
    std::vector<std::string> getShaderNames() const;
    void getShaderNames(std::vector<std::string>& names) const;

    // Clear all loaded shaders
    void clear();

    // Utility functions
    std::string paramTypeToString(ParamType type);

    // Pool management for all loaded shaders
    void clearAllInstancePools();
    // Get pool statistics
    void getPoolStatistics(size_t& totalPooled, size_t& totalActive) const;
    void printPoolStatistics() const;

    // Cache management
    void clearShaderCache();
    void printCacheStats() const;
    size_t getCacheSize() const { return shaderCache_.size(); }
};

// ISF Shader Template (immutable after loading)
class ISFShader {
    friend class ISFLoader;
    friend class ISFShaderInstance;

public:
    ~ISFShader();

    // Factory method
    ISFShaderInstance* createInstance();

    // Return instance to pool for reuse
    void releaseInstance(ISFShaderInstance* instance);

    // Pool management
    void clearInstancePool();
    size_t getPoolSize() const { return instancePool_.size(); }
    size_t getActiveInstanceCount() const { return activeInstances_; }

    // Template information access
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    const std::string& getFilepath() const { return filepath_; }
    ISFLoader::ShaderType getType() const { return type_; }
    GLuint getProgram() const { return program_; }

    const std::vector<ISFLoader::ParamInfo>& getParameterInfo() const;
    const std::vector<ISFLoader::InputInfo>& getInputInfo() const;
    const std::vector<ISFLoader::PassInfo>& getPassInfo() const;

    size_t getParameterCount() const;
    size_t getInputCount() const;
    size_t getPassCount() const;

    bool usesFloatBuffers() const { return usesFloatBuffers_; }

private:
    GLuint program_ = 0;
    std::string name_;
    std::string description_;
    std::string filepath_;
    std::string customVertexShader_;  // Custom vertex shader source
    ISFLoader::ShaderType type_;

    std::vector<ISFLoader::ParamInfo> parameterTemplates_;
    std::vector<ISFLoader::InputInfo> inputs_;
    std::vector<ISFLoader::PassInfo> passes_;  // Multi-pass information

    // Cached uniform locations for built-ins
    GLint timeLocation_ = -1;
    GLint timeDeltaLocation_ = -1;
    GLint renderSizeLocation_ = -1;
    GLint frameIndexLocation_ = -1;
    GLint dateLocation_ = -1;
    GLint passIndexLocation_ = -1;

    // Cached uniform locations for parameters
    std::unordered_map<std::string, GLint> paramLocations_;

    // Cached uniform locations for inputs
    std::unordered_map<std::string, GLint> inputLocations_;

    // Cached uniform locations for pass buffers
    std::unordered_map<std::string, GLint> bufferLocations_;

    // Cached uniform locations for input image rectangles
    std::unordered_map<std::string, GLint> imgRectLocations_;

    // Instance pooling
    std::vector<std::unique_ptr<ISFShaderInstance>> instancePool_;
    std::mutex poolMutex_;  // Thread-safe pool access
    size_t activeInstances_ = 0;  // Track active instances
    static const size_t MAX_POOL_SIZE = 16;  // Maximum instances to keep in pool

    bool usesFloatBuffers_ = false;  // True if FLOAT: true in JSON
};

// ISF Shader Instance (parameter state + rendering)
class ISFShaderInstance {
    friend class ISFShader;

private:
    ISFShader* parentShader_;
    std::vector<ISFLoader::ParamValue> paramValues_;
    bool isInPool_ = false;  // Track if instance is currently pooled

    ISFShaderInstance(ISFShader* parent);

public:
    ~ISFShaderInstance();

    // Access template info through instance
    const std::string& getName() const { return parentShader_->getName(); }
    const std::string& getDescription() const { return parentShader_->getDescription(); }
    ISFLoader::ShaderType getType() const { return parentShader_->getType(); }
    GLuint getProgram() const { return parentShader_->getProgram(); }

    const std::vector<ISFLoader::ParamInfo>& getParameterInfo() const {
        return parentShader_->getParameterInfo();
    }
    const std::vector<ISFLoader::InputInfo>& getInputInfo() const {
        return parentShader_->getInputInfo();
    }
    const std::vector<ISFLoader::PassInfo>& getPassInfo() const {
        return parentShader_->getPassInfo();
    }

    // Parameter value access
    bool setParameter(const std::string& name, float value);
    bool setParameter(const std::string& name, int value);
    bool setParameter(const std::string& name, bool value);
    bool setParameter(const std::string& name, float x, float y);  // point2D
    bool setParameter(const std::string& name, float r, float g, float b, float a = 1.0f);  // color

    bool getParameter(const std::string& name, float& value) const;
    bool getParameter(const std::string& name, int& value) const;
    bool getParameter(const std::string& name, bool& value) const;
    bool getParameter(const std::string& name, float& x, float& y) const;  // point2D
    bool getParameter(const std::string& name, float& r, float& g, float& b, float& a) const;  // color

    // Reset parameters to defaults
    void resetParametersToDefaults();

    // Unified rendering interface - automatically handles single and multi-pass
    void render(float time = 0.0f, float renderWidth = 1920.0f, float renderHeight = 1080.0f, int frameIndex = 0);

    // Set built-in uniforms manually if needed
    void setBuiltinUniforms(float time, float renderWidth, float renderHeight, int frameIndex = 0, int passIndex = 0);

    // Bind input textures (call before render())
    void bindInputTexture(GLuint textureId, int inputIndex = 0);

    // Thread-safe audio data storage (called from audio thread)
    void storeAudioFFTData(const float* fftData, size_t binCount);
    void storeAudioSamples(const float* audioSamples, size_t sampleCount);

    bool setParameterByLabel(const std::string& name, const std::string& label);
    bool getParameterLabel(const std::string& name, std::string& label) const;
    bool getParameterLabels(const std::string& name, std::vector<std::string>& labels) const;
    bool getParameterValues(const std::string& name, std::vector<int>& values) const;
    bool isDiscreteParameter(const std::string& name) const;

    // Check if instance is currently in use (not pooled)
    bool isActive() const { return !isInPool_; }
    // Reset instance to clean state for reuse
    void resetForReuse();

private:
    int findParameterIndex(const std::string& name) const;
    void setupPassFramebuffers(float renderWidth, float renderHeight);
    int evaluateExpression(const std::string& expr, float width, float height);
    void renderPass(int passIndex, float time, float renderWidth, float renderHeight, int frameIndex,
                    GLint originalFramebuffer, GLint originalDrawBuffer, GLint originalViewport[4]);
    void renderSinglePass(float time, float renderWidth, float renderHeight, int frameIndex);
    void setParameterUniforms();
    void bindTextures(int passIndex);
    void drawFullscreenQuad();
    GLuint fullscreenQuadVAO_ = 0;
    void createFullscreenQuad();

    // Input texture binding state
    std::vector<GLuint> boundInputTextures_;

    // Audio data management (thread-safe)
    std::mutex audioDataMutex_;
    std::vector<float> latestFFTData_;
    std::vector<float> latestAudioSamples_;
    bool hasNewFFTData_ = false;
    bool hasNewAudioSamples_ = false;

    float lastFrameTime_ = 0.0f;  // Track previous frame time for TIMEDELTA calculation

    std::vector<ISFLoader::InstancePassInfo> instancePasses_;

    int instanceFrameIndex_ = 0;  // Auto-incrementing frame counter for generators

    // Audio texture management
    std::vector<GLuint> audioTextures_;
    void createAudioTexture(int inputIndex, int width, int height);
    void updateAudioTexture(int inputIndex, const float* audioData, size_t dataSize, int channels);
    void applyStoredAudioData(); // Called during render
};