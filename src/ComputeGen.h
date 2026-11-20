// ComputeGen.h
// Compute-shader generators: a third category of layer sources next to FFGL and ISF.
//
// A generator is a single ".comp" file: a leading /* { json } */ header (ISF-style INPUTS, plus
// RESOURCES and PASSES) followed by GLSL compute code. The host compiles that code once per pass,
// with "#define KERNEL_<name>" so one file can hold all kernels (see docs/compute-generators.md).
//
// Desktop GL 4.3+ only; the whole category is compiled out for GLES builds.
#pragma once

#ifndef USE_GLES

#include <GL/glew.h>
#include "ISFLoader.h"   // ISFLoader::ParamInfo / ParamType are shared so Layer can reuse Param::isfset_parameter_to
#include <vector>
#include <string>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>

class ComputeGenShader;
class ComputeGenInstance;

// One GPU resource declared in the header's RESOURCES array.
struct CGResource {
    enum Kind { TEXTURE, BUFFER };
    std::string name;
    Kind kind = TEXTURE;
    // texture
    std::string format = "rgba16f";   // GLSL image format qualifier: r32f rg32f rgba32f r16f rg16f rgba16f rgba8 r32ui
    GLenum internalFormat = GL_RGBA16F;
    bool isUint = false;              // r32ui -> uimage2D, not samplable
    bool persistent = false;          // ping-pong pair: NAME (write image) + NAME_prev (sampler2D)
    bool repeat = true;               // sampler wrap mode (WRAP: "repeat" | "clamp")
    bool linear = true;               // sampler filtering
    std::string widthExpr = "RENDERSIZE_X";
    std::string heightExpr = "RENDERSIZE_Y";
    // buffer
    std::string countExpr = "1024";
    int stride = 16;                  // bytes per element
};

// One compute dispatch in the PASSES array.
struct CGPass {
    std::string kernel;               // becomes #define KERNEL_<kernel>
    std::string sizeName;             // resource name, "OUTPUT", or empty when sizeExprs is used
    std::vector<std::string> sizeExprs; // explicit 1-3 dimension expressions
    std::string repeatExpr = "1";
    std::vector<std::string> swap;    // persistent textures to swap after each execution
    std::vector<std::string> uses;    // resources visible to this kernel (empty = all, OUTPUT included)
    bool initOnly = false;            // WHEN: "init" - only on the first frame after (re)allocation or reset
    bool usesOutput = true;
    GLuint program = 0;
    int dims = 2;                     // 1 or 2
    // filled by ComputeGenShader::compile(): indices into the shader's resource list, in binding order
    std::vector<int> imgIdx;          // image2D / uimage2D bindings (texture resources)
    std::vector<int> smpIdx;          // sampler2D NAME_prev bindings (persistent textures)
    std::vector<int> bufIdx;          // SSBO binding points (buffers), exposed as BIND_<name>
    std::vector<int> swapIdx;         // resolved SWAP entries
    int sizeIdx = -1;                 // resolved sizeName (-1 = OUTPUT or explicit expressions)
    std::unordered_map<std::string, GLint> locs;   // lazily filled uniform locations
};

class ComputeGenLoader {
public:
    ~ComputeGenLoader();

    // true if the GL context can run compute generators (GL 4.3+ and enough image units)
    static bool supported();

    // Recursively loads every *.comp under dir. Returns the number of generators added.
    int loadDirectory(const std::string& dir);
    bool loadFile(const std::string& path);

    int getShaderCount() const { return (int)shaders_.size(); }
    ComputeGenShader* getShader(int i) const;
    ComputeGenShader* findShader(const std::string& name) const;   // case-insensitive
    int findIndex(const std::string& name) const;                  // -1 if absent

private:
    std::vector<std::unique_ptr<ComputeGenShader>> shaders_;
};

// Immutable, compiled template of one generator.
class ComputeGenShader {
public:
    ~ComputeGenShader();
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    const std::vector<ISFLoader::ParamInfo>& parameterInfo() const { return params_; }
    std::shared_ptr<ComputeGenInstance> createInstance();

private:
    friend class ComputeGenLoader;
    friend class ComputeGenInstance;
    bool parse(const std::string& fileContent);
    bool compile();
    std::string buildPrelude(CGPass& pass) const;   // also fills the pass binding lists

    std::string name_;
    std::string description_;
    std::string source_;                       // GLSL after the header
    std::vector<ISFLoader::ParamInfo> params_;
    std::vector<CGResource> resources_;
    std::vector<CGPass> passes_;
};

// Per-layer running simulation. Owns the GL resources; not thread-safe, call from the render thread.
class ComputeGenInstance {
public:
    explicit ComputeGenInstance(ComputeGenShader* shader);
    ~ComputeGenInstance();

    const std::vector<ISFLoader::ParamInfo>& getParameterInfo() const { return shader_->params_; }
    void setParameter(const std::string& name, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f);
    void reset();                              // re-run the init passes and clear state next frame
    void render(float time, int width, int height);
    GLuint getOutputTexture() const { return outTex_; }
    ComputeGenShader* shader() const { return shader_; }

private:
    struct ResState {
        GLuint tex[2] = {0, 0};                // tex[cur_] = latest state (NAME_prev), tex[1-cur_] = write target (NAME)
        int cur = 0;
        GLuint buf = 0;
        int w = 0, h = 0;                      // textures: size; buffers: w = count
    };

    double eval(const std::string& expr, int rw, int rh) const;
    int evalInt(const std::string& expr, int rw, int rh, int lo, int hi) const;
    bool allocate(int rw, int rh);
    void release();
    void clearState();
    void runPass(CGPass& pass, int rw, int rh, int repeatIndex);
    GLint location(CGPass& pass, const std::string& uniform);
    void setUniforms(CGPass& pass, int targetW, int targetH, int repeatIndex);

    ComputeGenShader* shader_;
    std::map<std::string, std::array<float, 4>> values_;
    std::vector<ResState> res_;                // parallel to shader_->resources_
    std::vector<int> resW_, resH_;             // evaluated sizes the current allocation was made for
    GLuint outTex_ = 0;
    int outW_ = 0, outH_ = 0;
    int frameIndex_ = 0;
    bool allocated_ = false;
    bool resetRequested_ = false;
    float lastTime_ = -1.0f;
    float time_ = 0.0f;
    float timeBase_ = 0.0f;       // clock value at the last (re)start: shaders see TIME relative to it
    float dt_ = 1.0f / 60.0f;
};

#endif // USE_GLES
