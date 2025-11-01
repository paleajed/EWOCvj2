// ISFLoader.cpp
#include "ISFLoader.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <set>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#elif __APPLE__
#include <pwd.h>
#include <unistd.h>
#elif __linux__
#include <pwd.h>
#include <unistd.h>
#endif

#include "program.h""

// Static member definition
const char* ISFLoader::vertexShaderSource_ = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 isf_FragNormCoord;
out vec2 vv_FragNormCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);

    // ISF expects inverted Y coordinate (0,0 = top-left, 1,1 = bottom-right)
    // vec2 isfCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
    vec2 isfCoord = vec2(aTexCoord.x, aTexCoord.y);

    isf_FragNormCoord = isfCoord;        // Inverted Y for ISF compatibility
    vv_FragNormCoord = isfCoord;         // Keep them the same for compatibility
}
)";

const char* ISFLoader::isfVertexBuiltins_ = R"(
// ISF built-in uniforms (will be set by the application)
uniform float TIME;
uniform float TIMEDELTA;
uniform vec2 RENDERSIZE;
uniform int FRAMEINDEX;
uniform vec4 DATE;
uniform int PASSINDEX;

// Input attributes
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

// Output varying to fragment shader
out vec2 isf_FragNormCoord;
out vec2 vv_FragNormCoord;

// ISF vertex shader initialization function
void isf_vertShaderInit() {
    gl_Position = vec4(aPos, 0.0, 1.0);

    // ISF expects inverted Y coordinate (0,0 = top-left, 1,1 = bottom-right)
    //vec2 isfCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
    vec2 isfCoord = vec2(aTexCoord.x, aTexCoord.y);

    isf_FragNormCoord = isfCoord;        // Inverted Y for ISF compatibility
    vv_FragNormCoord = isfCoord;         // Keep them the same for compatibility
}

)";

// ISF built-in functions to prepend to fragment shaders
const char* ISFLoader::isfBuiltinFunctions_ = R"(
// ISF built-in uniforms (will be set by the application)
uniform float TIME;
uniform float TIMEDELTA;
uniform vec2 RENDERSIZE;
uniform int FRAMEINDEX;
uniform vec4 DATE;
uniform int PASSINDEX;

// Input varying from vertex shader
in vec2 isf_FragNormCoord;
in vec2 vv_FragNormCoord;

// Output
out vec4 fragColor;

// ISF built-in functions for texture sampling
vec4 IMG_NORM_PIXEL(sampler2D sampler, vec2 coord) {
    return texture(sampler, coord);
}

vec4 IMG_PIXEL(sampler2D sampler, vec2 coord) {
    return texture(sampler, coord / RENDERSIZE);
}

vec4 IMG_THIS_PIXEL(sampler2D sampler) {
    return texture(sampler, gl_FragCoord.xy / RENDERSIZE);
}

vec4 IMG_THIS_NORM_PIXEL(sampler2D sampler) {
    vec2 normCoord = gl_FragCoord.xy / RENDERSIZE;
    return texture(sampler, normCoord);
}

vec2 IMG_SIZE(sampler2D sampler) {
    return textureSize(sampler, 0);
}

)";


// ===== ISFLoader Implementation =====

ISFLoader::ISFLoader() {
    devilInitialized_ = false;
    initializeShaderCache();  // Add this line
}

ISFLoader::~ISFLoader() {
    clear();
}

bool ISFLoader::loadISFDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "ISF directory does not exist: " << directory << std::endl;
        return false;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Check if ARB_parallel_shader_compile is supported
    bool supportsAsync = glewIsSupported("GL_ARB_parallel_shader_compile");
    if (supportsAsync) {
        std::cout << "Using BATCHED async shader compilation with ARB_parallel_shader_compile" << std::endl;
    } else {
        std::cout << "ARB_parallel_shader_compile not supported, using sequential compilation" << std::endl;
    }

    // Collect all ISF files first
    std::vector<std::string> isfFiles;
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".fs") {
                isfFiles.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning ISF directory: " << e.what() << std::endl;
        return false;
    }

    int total = isfFiles.size();
    std::cout << "Found " << total << " ISF shaders to compile" << std::endl;

    if (supportsAsync) {
        // BATCHED ASYNC COMPILATION
        // Structure to hold shader compilation state
        struct ShaderBatch {
            std::string filepath;
            std::unique_ptr<ISFShader> shader;
            GLuint vertexShader = 0;
            GLuint fragmentShader = 0;
            GLuint program = 0;
            std::string vertexSource;
            std::string fragmentSource;
            bool compileFailed = false;
            bool linkFailed = false;
        };

        std::vector<ShaderBatch> batches;
        batches.reserve(total);

        // PHASE 1: Parse all ISF files and prepare shader sources
        std::cout << "Phase 1: Parsing ISF files..." << std::endl;
        for (const auto& filepath : isfFiles) {
            ShaderBatch batch;
            batch.filepath = filepath;
            batch.shader = std::make_unique<ISFShader>();
            batch.shader->filepath_ = filepath;
            batch.shader->name_ = std::filesystem::path(filepath).stem().string();

            // Load file content
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Failed to open ISF file: " << filepath << std::endl;
                continue;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string fileContent = buffer.str();
            file.close();

            // Check for custom vertex shader
            std::string basePath = std::filesystem::path(filepath).replace_extension().string();
            std::string vertexShaderPath = basePath + ".vs";
            if (std::filesystem::exists(vertexShaderPath)) {
                std::ifstream vsFile(vertexShaderPath);
                if (vsFile.is_open()) {
                    std::stringstream vsBuffer;
                    vsBuffer << vsFile.rdbuf();
                    batch.shader->customVertexShader_ = vsBuffer.str();
                    vsFile.close();
                }
            }

            // Parse ISF JSON to extract shader components (without compiling yet)
            std::string jsonContent, fragmentSource;
            if (!extractISFComponents(fileContent, jsonContent, fragmentSource)) {
                std::cerr << "Failed to extract ISF components: " << filepath << std::endl;
                continue;
            }

            try {
                json root = json::parse(jsonContent);
                if (!validateISFStructure(root)) {
                    std::cerr << "Invalid ISF structure: " << filepath << std::endl;
                    continue;
                }

                // Parse metadata
                if (root.contains("DESCRIPTION")) {
                    batch.shader->description_ = root["DESCRIPTION"].get<std::string>();
                }

                // Determine shader type
                batch.shader->type_ = GENERATOR;
                if (root.contains("INPUTS")) {
                    for (const auto& input : root["INPUTS"]) {
                        if (input.contains("TYPE")) {
                            std::string typeStr = input["TYPE"].get<std::string>();
                            if (typeStr == "image") {
                                batch.shader->type_ = EFFECT;
                                break;
                            }
                        }
                    }
                }

                // Parse all the metadata
                if (root.contains("INPUTS")) parseParameters(root["INPUTS"], *batch.shader);
                if (root.contains("INPUTS")) parseInputs(root["INPUTS"], *batch.shader);
                if (root.contains("IMPORTED")) parseImported(root["IMPORTED"], *batch.shader);
                if (root.contains("PASSES")) parsePasses(root["PASSES"], *batch.shader);
                if (root.contains("FLOAT") && root["FLOAT"].is_boolean()) {
                    batch.shader->usesFloatBuffers_ = root["FLOAT"].get<bool>();
                }

                // Build parameter uniform declarations
                std::string parameterUniforms;
                for (const auto& param : batch.shader->parameterTemplates_) {
                    switch (param.type) {
                        case PARAM_FLOAT:
                            parameterUniforms += "uniform float " + param.name + ";\n";
                            break;
                        case PARAM_LONG:
                            parameterUniforms += "uniform int " + param.name + ";\n";
                            break;
                        case PARAM_BOOL:
                        case PARAM_EVENT:
                            parameterUniforms += "uniform bool " + param.name + ";\n";
                            break;
                        case PARAM_COLOR:
                            parameterUniforms += "uniform vec4 " + param.name + ";\n";
                            break;
                        case PARAM_POINT2D:
                            parameterUniforms += "uniform vec2 " + param.name + ";\n";
                            break;
                    }
                }

                // Build input texture uniform declarations
                std::string inputUniforms;
                for (const auto& input : batch.shader->inputs_) {
                    inputUniforms += "uniform sampler2D " + input.name + ";\n";

                    // Add image rectangle uniforms for ALL image inputs
                    if (input.type == INPUT_IMAGE || input.type == INPUT_EXTERNAL_IMAGE) {
                        inputUniforms += "uniform vec4 _" + input.name + "_imgRect;\n";
                    }
                }

                // Build pass buffer uniform declarations
                std::string bufferUniforms;
                for (const auto& pass : batch.shader->passes_) {
                    if (!pass.target.empty()) {
                        bufferUniforms += "uniform sampler2D " + pass.target + ";\n";
                    }
                }

                // Build shader sources (without compiling)
                std::string vertexSource;
                std::string customVaryingInputs = "";  // For fragment shader (initialize to empty)
                std::string customVaryingOutputs = ""; // For vertex shader (NEW)

                if (!batch.shader->customVertexShader_.empty()) {
                    // PROCESS custom vertex shader (replicate logic from compileShader)
                    std::stringstream vertexStream(batch.shader->customVertexShader_);
                    std::stringstream processedVertexStream;
                    std::string line;
                    bool skipLegacyBlock = false;
                    bool inVersionConditional = false;
                    std::set<std::string> uniqueVaryings;

                    int totalLines = 0;
                    int includedLines = 0;
                    int skippedLines = 0;

                    // Process vertex shader line by line to remove legacy blocks and extract varyings
                    while (std::getline(vertexStream, line)) {
                        totalLines++;
                        // Trim whitespace
                        std::string trimmedLine = line;
                        size_t start = trimmedLine.find_first_not_of(" \t");
                        if (start != std::string::npos) {
                            trimmedLine = trimmedLine.substr(start);
                        }

                        // Check for version conditional start
                        if (trimmedLine.find("#if __VERSION__ <= 120") == 0) {
                            skipLegacyBlock = true;
                            inVersionConditional = true;
                            skippedLines++;
                            continue;
                        }
                        else if (trimmedLine.find("#else") == 0 && inVersionConditional) {
                            skipLegacyBlock = false;
                            skippedLines++;
                            continue;
                        }
                        else if (trimmedLine.find("#endif") == 0 && inVersionConditional) {
                            skipLegacyBlock = false;
                            inVersionConditional = false;
                            skippedLines++;
                            continue;
                        }

                        // Include line if not skipping legacy block
                        if (!skipLegacyBlock) {
                            // Check if this is an "out" declaration (varying output)
                            bool isOutDeclaration = (line.find("out ") != std::string::npos && line.find("vec") != std::string::npos);

                            if (isOutDeclaration) {
                                // Extract variable name
                                size_t nameStart = line.find_last_of(' ');
                                size_t nameEnd = line.find(';');
                                std::string varyingName;
                                if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                                    varyingName = line.substr(nameStart + 1, nameEnd - nameStart - 1);
                                }

                                // Add to unique set
                                if (!varyingName.empty() && uniqueVaryings.find(varyingName) == uniqueVaryings.end()) {
                                    uniqueVaryings.insert(varyingName);

                                    // Add to vertex shader outputs (keep as "out")
                                    customVaryingOutputs += line + "\n";

                                    // Convert "out vec2 left_coord;" to "in vec2 left_coord;" for fragment shader
                                    std::string varyingDeclaration = line;
                                    size_t outPos = varyingDeclaration.find("out ");
                                    if (outPos != std::string::npos) {
                                        varyingDeclaration.replace(outPos, 4, "in ");
                                    }
                                    customVaryingInputs += varyingDeclaration + "\n";
                                }

                                // Don't add this line to the processed vertex stream (we'll add it to the header instead)
                                skippedLines++;
                            } else {
                                // Not an out declaration, keep it in the processed vertex shader
                                includedLines++;
                                processedVertexStream << line << "\n";
                            }
                        } else {
                            skippedLines++;
                        }
                    }
                    batch.shader->customVertexShader_ = processedVertexStream.str();

                    if (!customVaryingInputs.empty()) {
                        customVaryingInputs = "// Custom varying inputs from vertex shader\n" + customVaryingInputs + "\n";
                    }

                    // Build parameter uniforms for vertex shader
                    std::string customVertexUniforms;
                    for (const auto& param : batch.shader->parameterTemplates_) {
                        switch (param.type) {
                            case PARAM_FLOAT:
                                customVertexUniforms += "uniform float " + param.name + ";\n";
                                break;
                            case PARAM_LONG:
                                customVertexUniforms += "uniform int " + param.name + ";\n";
                                break;
                            case PARAM_BOOL:
                            case PARAM_EVENT:
                                customVertexUniforms += "uniform bool " + param.name + ";\n";
                                break;
                            case PARAM_COLOR:
                                customVertexUniforms += "uniform vec4 " + param.name + ";\n";
                                break;
                            case PARAM_POINT2D:
                                customVertexUniforms += "uniform vec2 " + param.name + ";\n";
                                break;
                        }
                    }

                    vertexSource = "#version 330 core\n"
                                   "// ISF built-in uniforms\n"
                                   "uniform float TIME;\n"
                                   "uniform float TIMEDELTA;\n"
                                   "uniform vec2 RENDERSIZE;\n"
                                   "uniform int FRAMEINDEX;\n"
                                   "uniform vec4 DATE;\n"
                                   "uniform int PASSINDEX;\n"
                                   "\n"
                                   "// Input attributes\n"
                                   "layout (location = 0) in vec2 aPos;\n"
                                   "layout (location = 1) in vec2 aTexCoord;\n"
                                   "\n"
                                   "// Output varying to fragment shader\n"
                                   "out vec2 isf_FragNormCoord;\n"
                                   "out vec2 vv_FragNormCoord;\n"
                                   "\n";

                    // Add custom varying outputs from the custom vertex shader
                    if (!customVaryingOutputs.empty()) {
                        vertexSource += "// Custom varying outputs\n" + customVaryingOutputs + "\n";
                    }

                    vertexSource += customVertexUniforms +
                                   "// ISF vertex shader initialization function\n"
                                   "void isf_vertShaderInit() {\n"
                                   "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                   "    \n"
                                   "    // ISF expects inverted Y coordinate (0,0 = top-left, 1,1 = bottom-right)\n"
                                   "    vec2 isfCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
                                   "    \n"
                                   "    isf_FragNormCoord = isfCoord;        // Inverted Y for ISF compatibility\n"
                                   "    vv_FragNormCoord = isfCoord;         // Keep them the same for compatibility\n"
                                   "}\n"
                                   "\n" +
                                   batch.shader->customVertexShader_;
                } else {
                    vertexSource = vertexShaderSource_;
                }

                // PROCESS fragment shader to remove legacy GLSL 120 blocks
                std::string modernFragmentSource = fragmentSource;
                std::stringstream fragStream(modernFragmentSource);
                std::stringstream processedFragmentStream;
                std::string fragLine;
                bool fragSkipLegacy = false;
                bool fragInVersionConditional = false;
                std::set<std::string> fragmentVaryingNames;

                while (std::getline(fragStream, fragLine)) {
                    std::string trimmedFragLine = fragLine;
                    size_t fragStart = trimmedFragLine.find_first_not_of(" \t");
                    if (fragStart != std::string::npos) {
                        trimmedFragLine = trimmedFragLine.substr(fragStart);
                    }

                    if (trimmedFragLine.find("#if __VERSION__ <= 120") == 0) {
                        fragSkipLegacy = true;
                        fragInVersionConditional = true;
                        continue;
                    }
                    else if (trimmedFragLine.find("#else") == 0 && fragInVersionConditional) {
                        fragSkipLegacy = false;
                        continue;
                    }
                    else if (trimmedFragLine.find("#endif") == 0 && fragInVersionConditional) {
                        fragSkipLegacy = false;
                        fragInVersionConditional = false;
                        continue;
                    }

                    if (!fragSkipLegacy) {
                        processedFragmentStream << fragLine << "\n";

                        // Track varying variable names that fragment shader declares
                        if (fragLine.find("in vec") != std::string::npos && fragLine.find(";") != std::string::npos) {
                            size_t nameStart = fragLine.find_last_of(' ');
                            size_t nameEnd = fragLine.find(';');
                            if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                                std::string varyingName = fragLine.substr(nameStart + 1, nameEnd - nameStart - 1);
                                fragmentVaryingNames.insert(varyingName);
                            }
                        }
                    }
                }

                modernFragmentSource = processedFragmentStream.str();

                // Filter out duplicate varyings from customVaryingInputs
                if (!fragmentVaryingNames.empty() && !customVaryingInputs.empty()) {
                    std::stringstream customVaryingStream(customVaryingInputs);
                    std::stringstream filteredVaryingStream;
                    std::string varyingLine;
                    bool hasHeader = false;

                    while (std::getline(customVaryingStream, varyingLine)) {
                        if (varyingLine.find("// Custom varying") != std::string::npos) {
                            continue;
                        }

                        bool isDuplicate = false;
                        for (const auto& fragVaryingName : fragmentVaryingNames) {
                            if (varyingLine.find(fragVaryingName) != std::string::npos) {
                                isDuplicate = true;
                                break;
                            }
                        }

                        if (!isDuplicate && !varyingLine.empty()) {
                            if (!hasHeader) {
                                filteredVaryingStream << "// Custom varying inputs from vertex shader\n";
                                hasHeader = true;
                            }
                            filteredVaryingStream << varyingLine << "\n";
                        }
                    }

                    if (hasHeader) {
                        filteredVaryingStream << "\n";
                    }
                    customVaryingInputs = filteredVaryingStream.str();
                }

                // Replace gl_FragColor with fragColor
                size_t pos = 0;
                while ((pos = modernFragmentSource.find("gl_FragColor", pos)) != std::string::npos) {
                    modernFragmentSource.replace(pos, 12, "fragColor");
                    pos += 9; // Length of "fragColor"
                }

                // Build complete fragment source with uniforms and filtered custom varyings
                std::string fullFragmentSource = "#version 330 core\n" +
                                                  std::string(isfBuiltinFunctions_) +
                                                  customVaryingInputs +
                                                  parameterUniforms +
                                                  inputUniforms +
                                                  bufferUniforms +
                                                  modernFragmentSource;

                batch.vertexSource = vertexSource;
                batch.fragmentSource = fullFragmentSource;

                batches.push_back(std::move(batch));

            } catch (const json::exception& e) {
                std::cerr << "JSON parse error in " << filepath << ": " << e.what() << std::endl;
                continue;
            }
        }

        // PHASE 2: Kick off ALL shader compilations (non-blocking)
        std::cout << "Phase 2: Starting compilation of " << batches.size() << " shaders..." << std::endl;
        for (auto& batch : batches) {
            batch.vertexShader = glCreateShader(GL_VERTEX_SHADER);
            const char* vsSrc = batch.vertexSource.c_str();
            glShaderSource(batch.vertexShader, 1, &vsSrc, NULL);
            glCompileShader(batch.vertexShader);

            batch.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fsSrc = batch.fragmentSource.c_str();
            glShaderSource(batch.fragmentShader, 1, &fsSrc, NULL);
            glCompileShader(batch.fragmentShader);
        }

        // PHASE 3: Poll for compilation completion
        std::cout << "Phase 3: Waiting for shader compilation..." << std::endl;
        int compiledCount = 0;
        while (compiledCount < batches.size()) {
            for (auto& batch : batches) {
                if (batch.compileFailed || (batch.vertexShader == 0 && batch.fragmentShader == 0)) continue;
                if (batch.vertexShader == 0 || batch.fragmentShader == 0) continue;

                GLint vsComplete = GL_FALSE, fsComplete = GL_FALSE;
                glGetShaderiv(batch.vertexShader, GL_COMPLETION_STATUS_ARB, &vsComplete);
                glGetShaderiv(batch.fragmentShader, GL_COMPLETION_STATUS_ARB, &fsComplete);

                if (vsComplete == GL_TRUE && fsComplete == GL_TRUE) {
                    // Check for compilation errors
                    GLint vsSuccess, fsSuccess;
                    glGetShaderiv(batch.vertexShader, GL_COMPILE_STATUS, &vsSuccess);
                    glGetShaderiv(batch.fragmentShader, GL_COMPILE_STATUS, &fsSuccess);

                    if (!vsSuccess || !fsSuccess) {
                        if (!vsSuccess) {
                            std::cerr << "Shader " << batch.shader->name_ << " VERTEX compilation FAILED!\n";
                            logShaderError(batch.vertexShader, "VERTEX");
                        }
                        if (!fsSuccess) {
                            std::cerr << "Shader " << batch.shader->name_ << " FRAGMENT compilation FAILED!\n";
                            logShaderError(batch.fragmentShader, "FRAGMENT");
                        }
                        glDeleteShader(batch.vertexShader);
                        glDeleteShader(batch.fragmentShader);
                        batch.vertexShader = 0;
                        batch.fragmentShader = 0;
                        batch.compileFailed = true;
                    }
                    compiledCount++;
                }
            }
            // Sleep for 1ms instead of 100us - reduces CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // PHASE 4: Rolling window linking - link hardware_concurrency() programs at a time
        std::cout << "Phase 4: Starting rolling window linking..." << std::endl;
        int maxConcurrentLinks = std::thread::hardware_concurrency();
        std::cout << "Using rolling window of " << maxConcurrentLinks << " concurrent links" << std::endl;

        // Helper lambda to start linking for a batch
        auto startLinking = [](ShaderBatch& batch) {
            batch.program = glCreateProgram();
            glAttachShader(batch.program, batch.vertexShader);
            glAttachShader(batch.program, batch.fragmentShader);
            glBindAttribLocation(batch.program, 0, "aPos");
            glBindAttribLocation(batch.program, 1, "aTexCoord");
            // Enable binary retrievability BEFORE linking (required for caching)
            glProgramParameteri(batch.program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
            glLinkProgram(batch.program);
        };

        // Count total to link and initialize first batch
        int totalToLink = 0;
        int nextBatchToStart = 0;
        for (const auto& batch : batches) {
            if (!batch.compileFailed) totalToLink++;
        }

        // Start initial batch of links (up to maxConcurrentLinks)
        int activeLinkCount = 0;
        for (size_t i = 0; i < batches.size() && activeLinkCount < maxConcurrentLinks; i++) {
            if (!batches[i].compileFailed) {
                startLinking(batches[i]);
                activeLinkCount++;
                nextBatchToStart = i + 1;
            }
        }

        // PHASE 5: Poll for linking completion with rolling window
        std::cout << "Phase 5: Waiting for program linking (rolling window)..." << std::endl;
        int linkedCount = 0;
        int lastProgressUpdate = 0;
        auto lastProgressTime = std::chrono::high_resolution_clock::now();

        while (linkedCount < totalToLink) {
            int progressThisIteration = 0;

            for (auto& batch : batches) {
                if (batch.compileFailed || batch.linkFailed || batch.program == 0) continue;

                GLint linkComplete = GL_FALSE;
                glGetProgramiv(batch.program, GL_COMPLETION_STATUS_ARB, &linkComplete);

                if (linkComplete == GL_TRUE) {
                    linkedCount++;
                    activeLinkCount--;

                    // Check for linking errors
                    GLint linkSuccess;
                    glGetProgramiv(batch.program, GL_LINK_STATUS, &linkSuccess);

                    if (!linkSuccess) {
                        std::cerr << "Shader " << batch.shader->name_ << " PROGRAM LINKING FAILED!\n";
                        logProgramError(batch.program);
                        glDeleteProgram(batch.program);
                        batch.program = 0;
                        batch.linkFailed = true;
                    } else {
                        // Success! Store the program (uniform caching deferred to Phase 6)
                        batch.shader->program_ = batch.program;
                        // Cache the successfully linked program
                        cacheProgram(batch.shader->name_, batch.vertexSource, batch.fragmentSource, batch.program);
                    }

                    // Clean up shaders
                    glDeleteShader(batch.vertexShader);
                    glDeleteShader(batch.fragmentShader);
                    batch.vertexShader = 0;
                    batch.fragmentShader = 0;

                    progressThisIteration++;

                    // Start next shader in queue (rolling window)
                    while (nextBatchToStart < batches.size()) {
                        if (!batches[nextBatchToStart].compileFailed) {
                            startLinking(batches[nextBatchToStart]);
                            activeLinkCount++;
                            nextBatchToStart++;
                            break;
                        }
                        nextBatchToStart++;
                    }
                } else if (!batch.shader->customVertexShader_.empty() && linkedCount >= totalToLink - 10 && 0) {
                    // Shader with custom vertex shader is stuck - force check the link status anyway
                    // (This seems to be a bug with GL_ARB_parallel_shader_compile for custom vertex shaders)
                    GLint linkSuccess;
                    glGetProgramiv(batch.program, GL_LINK_STATUS, &linkSuccess);

                    if (!linkSuccess) {
                        glDeleteProgram(batch.program);
                        batch.program = 0;
                        batch.linkFailed = true;
                    } else {
                        // Store the program (uniform caching deferred to Phase 6)
                        batch.shader->program_ = batch.program;
                        // Cache the successfully linked program
                        cacheProgram(batch.shader->name_, batch.vertexSource, batch.fragmentSource, batch.program);

                        // Clean up shaders
                        glDeleteShader(batch.vertexShader);
                        glDeleteShader(batch.fragmentShader);
                        batch.vertexShader = 0;
                        batch.fragmentShader = 0;

                        progressThisIteration++;
                    }
                    linkedCount++;
                    activeLinkCount--;

                    // Start next shader in queue (rolling window)
                    while (nextBatchToStart < batches.size()) {
                        if (!batches[nextBatchToStart].compileFailed) {
                            startLinking(batches[nextBatchToStart]);
                            activeLinkCount++;
                            nextBatchToStart++;
                            break;
                        }
                        nextBatchToStart++;
                    }
                }
            }

            // Update progress every 500ms OR when shaders complete OR when done (for smooth visual feedback)
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProgressTime).count();

            if (elapsed >= 500 || linkedCount > lastProgressUpdate || linkedCount == totalToLink) {
                SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDrawBuffer(GL_FRONT);
                glViewport(0, 0, glob->h / 2.0f, glob->h / 2.0f);
                mainprogram->bvao = mainprogram->splboxvao;
                mainprogram->bvbuf = mainprogram->splboxvbuf;
                mainprogram->btbuf = mainprogram->splboxtbuf;
                draw_box(white, black, -0.25f, -0.9f, 0.5f, 0.1f, 0.0f, 0.0f,
                         1.0f, 1.0f, 0, -1, glob->w, glob->h, false);
                draw_box(white, white, -0.25f, -0.9f, 0.5f * (float)linkedCount / (float)totalToLink, 0.1f, 0.0f, 0.0f,
                         1.0f, 1.0f, 0, -1, glob->w, glob->h, false);
                glFlush();
                lastProgressUpdate = linkedCount;
                lastProgressTime = now;
            }

            // Sleep for 1ms - reduces CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // PHASE 6: Move successful shaders to the main list and cache uniform locations
        std::cout << "Phase 6: Finalizing shaders and caching uniform locations..." << std::endl;
        for (auto& batch : batches) {
            if (!batch.compileFailed && !batch.linkFailed && batch.program != 0) {
                cacheUniformLocations(*batch.shader);
                shaders_.push_back(std::move(batch.shader));
            }
        }

    } else {
        // SEQUENTIAL FALLBACK (no ARB extension)
        int count = 0;
        for (const auto& filepath : isfFiles) {
            count++;
            if (count % 8 == 1) {
                SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDrawBuffer(GL_FRONT);
                glViewport(0, 0, glob->h / 2.0f, glob->h / 2.0f);
                mainprogram->bvao = mainprogram->splboxvao;
                mainprogram->bvbuf = mainprogram->splboxvbuf;
                mainprogram->btbuf = mainprogram->splboxtbuf;
                draw_box(white, black, -0.25f, -0.9f, 0.5f, 0.1f, 0.0f, 0.0f,
                         1.0f, 1.0f, 0, -1, glob->w, glob->h, false);
                draw_box(white, white, -0.25f, -0.9f, 0.5f * (float)count / (float)total, 0.1f, 0.0f, 0.0f,
                         1.0f, 1.0f, 0, -1, glob->w, glob->h, false);
                glFlush();
            }
            loadISFFile(filepath);
        }
    }

    std::cout << "Loaded " << shaders_.size() << " ISF shaders from " << directory << std::endl;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Loaded " << shaders_.size() << " ISF shaders from " << directory
              << " in " << duration.count() << "ms" << std::endl;

    saveCacheToDisk();
    printCacheStats();
    return true;
}

bool ISFLoader::loadISFFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open ISF file: " << filepath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        file.close();

        auto shader = std::make_unique<ISFShader>();
        shader->filepath_ = filepath;
        shader->name_ = std::filesystem::path(filepath).stem().string();

        // MOVED: Check for custom vertex shader BEFORE parsing JSON
        // (since parseISFJson calls compileShader which needs the custom vertex shader)
        std::string basePath = std::filesystem::path(filepath).replace_extension().string();
        std::string vertexShaderPath = basePath + ".vs";

        std::string customVertexSource;
        if (std::filesystem::exists(vertexShaderPath)) {
            std::ifstream vsFile(vertexShaderPath);
            if (vsFile.is_open()) {
                std::stringstream vsBuffer;
                vsBuffer << vsFile.rdbuf();
                customVertexSource = vsBuffer.str();
                vsFile.close();

                std::cout << "Loaded custom vertex shader: " << vertexShaderPath << std::endl;
            }
        }

        // Set the custom vertex shader BEFORE parsing
        shader->customVertexShader_ = customVertexSource;

        // Now parse the ISF JSON (which will call compileShader)
        if (!parseISFJson(fileContent, *shader)) {
            std::cerr << "Failed to parse ISF file: " << filepath << std::endl;
            return false;
        }

        shaders_.push_back(std::move(shader));
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception loading ISF file " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

std::string ISFLoader::getUserDataDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "/EWOCvj2";
    }
    return "shader_cache";
#elif __APPLE__
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    return std::string(homeDir) + "/Library/Application Support/EWOCvj2";
#elif __linux__
    const char* xdgDataHome = getenv("XDG_DATA_HOME");
    if (xdgDataHome && strlen(xdgDataHome) > 0) {
        return std::string(xdgDataHome) + "/EWOCvj2";
    }
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    return std::string(homeDir) + "/.local/share/EWOCvj2";
#else
    return "shader_cache";
#endif
}

void ISFLoader::initializeShaderCache() {
    cacheDirectory_ = getUserDataDirectory() + "/shader_cache";
    std::filesystem::create_directories(cacheDirectory_);

    std::string markerFile = cacheDirectory_ + "/cache_info.txt";
    isFirstRun_ = !std::filesystem::exists(markerFile);

    if (isFirstRun_) {
        showFirstRunMessage();
        std::ofstream marker(markerFile);
        if (marker.is_open()) {
            marker << "VJ Program Shader Cache\n";
            marker << "Created: " << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
            const char* vendor = (const char*)glGetString(GL_VENDOR);
            const char* renderer = (const char*)glGetString(GL_RENDERER);
            const char* version = (const char*)glGetString(GL_VERSION);
            if (vendor) marker << "GPU Vendor: " << vendor << "\n";
            if (renderer) marker << "GPU Renderer: " << renderer << "\n";
            if (version) marker << "OpenGL Version: " << version << "\n";
            marker.close();
        }
    } else {
        loadCacheFromDisk();
        std::cout << "Shader cache initialized: " << shaderCache_.size() << " entries loaded" << std::endl;
    }
}

void ISFLoader::showFirstRunMessage() {
    std::cout << "===============================================" << std::endl;
    std::cout << "FIRST RUN: Building shader cache..." << std::endl;
    std::cout << "Cache location: " << cacheDirectory_ << std::endl;
    std::cout << "===============================================" << std::endl;
}

std::string ISFLoader::generateHash(const std::string& vertexSource, const std::string& fragmentSource) {
    std::hash<std::string> hasher;
    std::string combined = vertexSource + "|" + fragmentSource;
    size_t hash1 = hasher(combined);
    size_t hash2 = hasher(std::to_string(hash1));
    std::stringstream ss;
    ss << std::hex << hash1 << hash2;
    return ss.str();
}

bool ISFLoader::isCacheValid(const CacheEntry& entry, const std::string& currentHash) {
    if (!entry.isValid || entry.sourceHash != currentHash) {
        return false;
    }
    GLint numFormats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);
    return numFormats > 0;
}

GLuint ISFLoader::loadCachedProgram(const std::string& shaderName,
                                    const std::string& vertexSource,
                                    const std::string& fragmentSource) {
    std::string hash = generateHash(vertexSource, fragmentSource);
    auto it = shaderCache_.find(shaderName);
    if (it == shaderCache_.end() || !isCacheValid(it->second, hash)) {
        return 0;
    }

    const CacheEntry& entry = it->second;
    GLuint program = glCreateProgram();
    if (program == 0) return 0;

    glProgramBinary(program, entry.binaryFormat,
                    entry.binaryData.data(), entry.binaryData.size());

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_TRUE) {
        return program;
    } else {
        glDeleteProgram(program);
        shaderCache_.erase(it);
        return 0;
    }
}

void ISFLoader::cacheProgram(const std::string& shaderName,
                             const std::string& vertexSource,
                             const std::string& fragmentSource,
                             GLuint program) {
    GLint numFormats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);
    if (numFormats == 0) return;

    GLint binaryLength = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    if (binaryLength == 0) return;

    CacheEntry entry;
    entry.binaryData.resize(binaryLength);
    GLsizei actualLength = 0;

    glGetProgramBinary(program, binaryLength, &actualLength,
                       &entry.binaryFormat, entry.binaryData.data());

    if (actualLength != binaryLength) return;

    entry.vertexSource = vertexSource;
    entry.fragmentSource = fragmentSource;
    entry.sourceHash = generateHash(vertexSource, fragmentSource);
    entry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    entry.isValid = true;

    shaderCache_[shaderName] = std::move(entry);
}

bool ISFLoader::validateCacheEnvironment() {
    std::string markerFile = cacheDirectory_ + "/cache_info.txt";
    if (!std::filesystem::exists(markerFile)) return false;

    std::ifstream marker(markerFile);
    if (!marker.is_open()) return false;

    std::string line, cachedVendor, cachedRenderer;
    while (std::getline(marker, line)) {
        if (line.find("GPU Vendor: ") == 0) {
            cachedVendor = line.substr(12);
        } else if (line.find("GPU Renderer: ") == 0) {
            cachedRenderer = line.substr(14);
        }
    }
    marker.close();

    const char* currentVendor = (const char*)glGetString(GL_VENDOR);
    const char* currentRenderer = (const char*)glGetString(GL_RENDERER);

    if (currentVendor && cachedVendor != std::string(currentVendor)) {
        std::cout << "GPU vendor changed - invalidating cache" << std::endl;
        return false;
    }
    if (currentRenderer && cachedRenderer != std::string(currentRenderer)) {
        std::cout << "GPU renderer changed - invalidating cache" << std::endl;
        return false;
    }
    return true;
}

void ISFLoader::loadCacheFromDisk() {
    if (!validateCacheEnvironment()) {
        std::cout << "Cache environment changed - starting fresh" << std::endl;
        clearShaderCache();
        return;
    }

    std::string indexFile = cacheDirectory_ + "/cache_index.dat";
    if (!std::filesystem::exists(indexFile)) return;

    std::ifstream file(indexFile, std::ios::binary);
    if (!file.is_open()) return;

    try {
        uint32_t numEntries;
        file.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));

        if (numEntries > 10000) {
            std::cout << "Cache file appears corrupted - starting fresh" << std::endl;
            clearShaderCache();
            return;
        }

        for (uint32_t i = 0; i < numEntries; ++i) {
            uint32_t nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            if (nameLength > 1000) {
                std::cout << "Cache corruption detected - starting fresh" << std::endl;
                clearShaderCache();
                return;
            }

            std::string shaderName(nameLength, '\0');
            file.read(shaderName.data(), nameLength);

            CacheEntry entry;
            uint32_t hashLength;
            file.read(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
            entry.sourceHash.resize(hashLength);
            file.read(entry.sourceHash.data(), hashLength);

            file.read(reinterpret_cast<char*>(&entry.timestamp), sizeof(entry.timestamp));
            file.read(reinterpret_cast<char*>(&entry.binaryFormat), sizeof(entry.binaryFormat));

            uint32_t binaryLength;
            file.read(reinterpret_cast<char*>(&binaryLength), sizeof(binaryLength));
            if (binaryLength > 1024 * 1024) {
                std::cout << "Shader binary too large - cache corruption?" << std::endl;
                clearShaderCache();
                return;
            }

            entry.binaryData.resize(binaryLength);
            file.read(entry.binaryData.data(), binaryLength);
            entry.isValid = true;
            shaderCache_[shaderName] = std::move(entry);
        }

        std::cout << "Successfully loaded " << shaderCache_.size() << " shaders from cache" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading shader cache: " << e.what() << std::endl;
        clearShaderCache();
    }
}

void ISFLoader::saveCacheToDisk() {
    std::string indexFile = cacheDirectory_ + "/cache_index.dat";
    std::ofstream file(indexFile, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to save shader cache" << std::endl;
        return;
    }

    try {
        uint32_t numEntries = static_cast<uint32_t>(shaderCache_.size());
        file.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));

        for (const auto& [shaderName, entry] : shaderCache_) {
            if (!entry.isValid) continue;

            uint32_t nameLength = static_cast<uint32_t>(shaderName.length());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(shaderName.data(), nameLength);

            uint32_t hashLength = static_cast<uint32_t>(entry.sourceHash.length());
            file.write(reinterpret_cast<const char*>(&hashLength), sizeof(hashLength));
            file.write(entry.sourceHash.data(), hashLength);

            file.write(reinterpret_cast<const char*>(&entry.timestamp), sizeof(entry.timestamp));
            file.write(reinterpret_cast<const char*>(&entry.binaryFormat), sizeof(entry.binaryFormat));

            uint32_t binaryLength = static_cast<uint32_t>(entry.binaryData.size());
            file.write(reinterpret_cast<const char*>(&binaryLength), sizeof(binaryLength));
            file.write(entry.binaryData.data(), binaryLength);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving shader cache: " << e.what() << std::endl;
    }
}

void ISFLoader::clearShaderCache() {
    shaderCache_.clear();
    try {
        std::filesystem::remove_all(cacheDirectory_);
        std::filesystem::create_directories(cacheDirectory_);
    } catch (const std::exception& e) {
        std::cerr << "Error clearing cache directory: " << e.what() << std::endl;
    }
}

void ISFLoader::printCacheStats() const {
    std::cout << "=== Shader Cache Statistics ===" << std::endl;
    std::cout << "Total cached shaders: " << shaderCache_.size() << std::endl;
    size_t totalSize = 0;
    for (const auto& [name, entry] : shaderCache_) {
        totalSize += entry.binaryData.size();
    }
    std::cout << "Total cache size: " << (totalSize / 1024) << " KB" << std::endl;
    std::cout << "Cache directory: " << cacheDirectory_ << std::endl;
    std::cout << "===============================" << std::endl;
}

bool ISFLoader::parseISFJson(const std::string& fileContent, ISFShader& shader) {
    // Extract JSON from comment block
    std::string jsonContent;
    std::string fragmentSource;

    if (!extractISFComponents(fileContent, jsonContent, fragmentSource)) {
        std::cerr << "Failed to extract JSON header from ISF file: " << shader.filepath_ << std::endl;
        return false;
    }

    json root;

    try {
        root = json::parse(jsonContent);
    } catch (const json::parse_error& e) {
        std::cerr << "Invalid JSON in ISF file: " << shader.filepath_ << " - " << e.what() << std::endl;
        return false;
    }

    if (!validateISFStructure(root)) {
        return false;
    }

    // Parse basic info
    if (root.contains("DESCRIPTION")) {
        shader.description_ = root["DESCRIPTION"].get<std::string>();
    }

    // NEW: Parse FLOAT flag (defaults to false if not specified)
    if (root.contains("FLOAT")) {
        shader.usesFloatBuffers_ = root["FLOAT"].get<bool>();
    } else {
        shader.usesFloatBuffers_ = false;
    }

    // Determine shader type
    bool hasInputImage = false;
    if (root.contains("INPUTS")) {
        for (const auto& input : root["INPUTS"]) {
            if (input.contains("TYPE") && input["TYPE"].get<std::string>() == "image") {
                hasInputImage = true;
                break;
            }
        }
    }
    shader.type_ = hasInputImage ? EFFECT : GENERATOR;

    // Parse inputs
    if (root.contains("INPUTS")) {
        if (!parseInputs(root["INPUTS"], shader)) {
            return false;
        }
    }

    // Parse imported images
    if (root.contains("IMPORTED")) {
        if (!parseImported(root["IMPORTED"], shader)) {
            return false;
        }
    }

    // Parse parameters (from INPUTS that aren't images)
    if (root.contains("INPUTS")) {
        if (!parseParameters(root["INPUTS"], shader)) {
            return false;
        }
    }

    // Parse passes
    if (root.contains("PASSES")) {
        if (!parsePasses(root["PASSES"], shader)) {
            return false;
        }
    } else {
        // Single pass shader - create default pass
        ISFLoader::PassInfo defaultPass;
        defaultPass.target = "";
        defaultPass.persistent = false;
        defaultPass.widthExpr = "$WIDTH";
        defaultPass.heightExpr = "$HEIGHT";
        shader.passes_.push_back(defaultPass);
    }

    // Use the extracted fragment shader
    if (!compileShader(fragmentSource, shader)) {
        return false;
    }

    return true;
}

bool ISFLoader::extractISFComponents(const std::string& fileContent, std::string& jsonContent, std::string& fragmentSource) {
    // Find the comment block that contains JSON
    size_t commentStart = fileContent.find("/*");
    if (commentStart == std::string::npos) {
        std::cerr << "No comment block found in ISF file" << std::endl;
        return false;
    }

    size_t commentEnd = fileContent.find("*/", commentStart);
    if (commentEnd == std::string::npos) {
        std::cerr << "Unterminated comment block in ISF file" << std::endl;
        return false;
    }

    // Extract JSON (skip the /* and */ markers)
    jsonContent = fileContent.substr(commentStart + 2, commentEnd - commentStart - 2);

    // Extract fragment shader (everything after the comment block)
    fragmentSource = fileContent.substr(commentEnd + 2);

    // Trim whitespace from fragment source
    size_t start = fragmentSource.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        fragmentSource = fragmentSource.substr(start);
    }

    if (fragmentSource.empty()) {
        std::cerr << "No fragment shader code found after JSON header" << std::endl;
        return false;
    }

    return true;
}

bool ISFLoader::validateISFStructure(const json& root) {
    // Basic validation - ISF files don't need FRAGMENTSHADER in JSON since it's the actual shader code
    return true;
}

bool ISFLoader::parseInputs(const json& inputs, ISFShader& shader) {
    int imageInputCount = 0;

    for (const auto& input : inputs) {
        if (!input.contains("TYPE")) continue;

        std::string type = input["TYPE"].get<std::string>();
        if (type != "image" && type != "audioFFT" && type != "audio") continue;

        if (type == "image") {
            imageInputCount++;
            if (imageInputCount > 2) {
                std::cerr << "ISF file has too many image inputs (max 2): " << shader.filepath_ << std::endl;
                return false;
            }
        }

        InputInfo inputInfo;
        inputInfo.name = input.contains("NAME") ? input["NAME"].get<std::string>() : ("INPUT" + std::to_string(shader.inputs_.size()));

        if (type == "audioFFT") {
            // Audio FFT input
            inputInfo.type = INPUT_AUDIO_FFT;
            inputInfo.width = 2048;  // Match your FFT data size
            inputInfo.height = 1;    // Single row of FFT data
        } else if (type == "audio") {
            // Audio waveform input
            inputInfo.type = INPUT_AUDIO;
            inputInfo.width = 1024;  // Match your audio sample buffer size
            inputInfo.height = 2;    // Two channels (stereo)
        } else if (input.contains("PATH")) {
            // External image
            inputInfo.type = INPUT_EXTERNAL_IMAGE;
            inputInfo.filepath = input["PATH"].get<std::string>();

            std::string baseDir = std::filesystem::path(shader.filepath_).parent_path().string();
            if (!loadExternalImage(inputInfo.filepath, inputInfo, baseDir)) {
                std::cerr << "Failed to load external image: " << inputInfo.filepath << std::endl;
                return false;
            }
        } else {
            // Regular input image
            inputInfo.type = INPUT_IMAGE;
        }

        shader.inputs_.push_back(inputInfo);
    }

    return true;
}

bool ISFLoader::parseParameters(const json& inputs, ISFShader& shader) {
    for (const auto& input : inputs) {
        if (!input.contains("TYPE")) continue;

        std::string type = input["TYPE"].get<std::string>();

        // Skip image, audio, and audioFFT inputs - these are handled as texture inputs, not parameters
        if (type == "image" || type == "audio" || type == "audioFFT") continue;

        ParamInfo param;
        param.name = input.contains("NAME") ? input["NAME"].get<std::string>() : "";
        param.label = input.contains("LABEL") ? input["LABEL"].get<std::string>() : param.name;
        param.type = stringToParamType(type);

        // Parse type-specific values
        switch (param.type) {
            case PARAM_FLOAT:
                param.minVal = input.contains("MIN") ? input["MIN"].get<float>() : 0.0f;
                param.maxVal = input.contains("MAX") ? input["MAX"].get<float>() : 1.0f;
                param.defaultVal = input.contains("DEFAULT") ? input["DEFAULT"].get<float>() : 0.0f;
                break;

            case PARAM_LONG:
                // Handle standard MIN/MAX for continuous ranges
                param.minInt = input.contains("MIN") ? input["MIN"].get<int>() : 0;
                param.maxInt = input.contains("MAX") ? input["MAX"].get<int>() : 100;
                param.defaultInt = input.contains("DEFAULT") ? input["DEFAULT"].get<int>() : 0;

                // NEW: Handle VALUES and LABELS for discrete enum-style parameters
                if (input.contains("VALUES") && input["VALUES"].is_array()) {
                    param.hasDiscreteValues = true;

                    // Parse VALUES array
                    for (const auto& value : input["VALUES"]) {
                        if (value.is_number_integer()) {
                            param.values.push_back(value.get<int>());
                        }
                    }

                    // Parse LABELS array (optional)
                    if (input.contains("LABELS") && input["LABELS"].is_array()) {
                        for (const auto& label : input["LABELS"]) {
                            if (label.is_string()) {
                                param.labels.push_back(label.get<std::string>());
                            }
                        }
                    }

                    // Validate that we have matching VALUES and LABELS (if labels exist)
                    if (!param.labels.empty() && param.labels.size() != param.values.size()) {
                        std::cerr << "Warning: VALUES and LABELS arrays have different sizes for parameter "
                                  << param.name << std::endl;
                        // Truncate to shorter size
                        size_t minSize = std::min(param.values.size(), param.labels.size());
                        param.values.resize(minSize);
                        param.labels.resize(minSize);
                    }

                    // If no labels provided, generate default ones
                    if (param.labels.empty()) {
                        for (int value : param.values) {
                            param.labels.push_back(std::to_string(value));
                        }
                    }

                    // Validate that default value is in the VALUES array
                    if (!param.isValidValue(param.defaultInt)) {
                        std::cerr << "Warning: Default value " << param.defaultInt
                                  << " is not in VALUES array for parameter " << param.name << std::endl;
                        if (!param.values.empty()) {
                            param.defaultInt = param.values[0]; // Use first valid value
                        }
                    }
                }
                break;

            case PARAM_BOOL:
            case PARAM_EVENT:
                if (input.contains("DEFAULT")) {
                    if (input["DEFAULT"].is_boolean()) {
                        param.defaultInt = input["DEFAULT"].get<bool>() ? 1 : 0;
                    } else if (input["DEFAULT"].is_number()) {
                        // Handle numeric boolean values (1/0)
                        param.defaultInt = input["DEFAULT"].get<int>() != 0 ? 1 : 0;
                    } else {
                        param.defaultInt = 0;
                    }
                } else {
                    param.defaultInt = 0;
                }
                break;

            case PARAM_COLOR:
                if (input.contains("DEFAULT") && input["DEFAULT"].is_array()) {
                    auto defaultArray = input["DEFAULT"];
                    for (int i = 0; i < std::min(4, (int)defaultArray.size()); ++i) {
                        param.defaultColor[i] = defaultArray[i].get<float>();
                    }
                }
                break;

            case PARAM_POINT2D:
                // Parse DEFAULT values for point2D
                if (input.contains("DEFAULT") && input["DEFAULT"].is_array() && input["DEFAULT"].size() >= 2) {
                    auto defaultArray = input["DEFAULT"];
                    param.defaultPoint[0] = defaultArray[0].get<float>();
                    param.defaultPoint[1] = defaultArray[1].get<float>();
                }

                // Parse MIN and MAX ranges for point2D
                if (input.contains("MIN") && input["MIN"].is_array() && input["MIN"].size() >= 2) {
                    auto minArray = input["MIN"];
                    param.minPoint[0] = minArray[0].get<float>();  // Min X
                    param.minPoint[1] = minArray[1].get<float>();  // Min Y
                } else {
                    param.minPoint[0] = 0.0f;
                    param.minPoint[1] = 0.0f;
                }

                if (input.contains("MAX") && input["MAX"].is_array() && input["MAX"].size() >= 2) {
                    auto maxArray = input["MAX"];
                    param.maxPoint[0] = maxArray[0].get<float>();  // Max X
                    param.maxPoint[1] = maxArray[1].get<float>();  // Max Y
                } else {
                    param.maxPoint[0] = 1.0f;
                    param.maxPoint[1] = 1.0f;
                }
                break;
        }

        shader.parameterTemplates_.push_back(param);
    }

    return true;
}

bool ISFLoader::parsePasses(const json& passes, ISFShader& shader) {
    for (const auto& pass : passes) {
        ISFLoader::PassInfo passInfo;

        if (pass.contains("TARGET")) {
            passInfo.target = pass["TARGET"].get<std::string>();
        }

        if (pass.contains("PERSISTENT")) {
            passInfo.persistent = pass["PERSISTENT"].get<bool>();
        }

        // Handle WIDTH - can be string or number
        if (pass.contains("WIDTH")) {
            if (pass["WIDTH"].is_string()) {
                passInfo.widthExpr = pass["WIDTH"].get<std::string>();
            } else if (pass["WIDTH"].is_number()) {
                // Convert number to string
                if (pass["WIDTH"].is_number_integer()) {
                    passInfo.widthExpr = std::to_string(pass["WIDTH"].get<int>());
                } else {
                    passInfo.widthExpr = std::to_string(pass["WIDTH"].get<float>());
                }
            } else {
                passInfo.widthExpr = "$WIDTH";
            }
        } else {
            passInfo.widthExpr = "$WIDTH";
        }

        // Handle HEIGHT - can be string or number
        if (pass.contains("HEIGHT")) {
            if (pass["HEIGHT"].is_string()) {
                passInfo.heightExpr = pass["HEIGHT"].get<std::string>();
            } else if (pass["HEIGHT"].is_number()) {
                // Convert number to string
                if (pass["HEIGHT"].is_number_integer()) {
                    passInfo.heightExpr = std::to_string(pass["HEIGHT"].get<int>());
                } else {
                    passInfo.heightExpr = std::to_string(pass["HEIGHT"].get<float>());
                }
            } else {
                passInfo.heightExpr = "$HEIGHT";
            }
        } else {
            passInfo.heightExpr = "$HEIGHT";
        }

        shader.passes_.push_back(passInfo);
    }

    return true;
}

bool ISFLoader::parseImported(const json& imported, ISFShader& shader) {
    for (auto it = imported.begin(); it != imported.end(); ++it) {
        const std::string& imageName = it.key();
        const json& imageSpec = it.value();

        if (!imageSpec.contains("PATH")) {
            std::cerr << "Imported image " << imageName << " missing PATH" << std::endl;
            continue;
        }

        InputInfo inputInfo;
        inputInfo.name = imageName;
        inputInfo.type = INPUT_EXTERNAL_IMAGE;
        inputInfo.filepath = imageSpec["PATH"].get<std::string>();

        std::string baseDir = std::filesystem::path(shader.filepath_).parent_path().string();
        if (!loadExternalImage(inputInfo.filepath, inputInfo, baseDir)) {
            std::cerr << "Failed to load imported image: " << inputInfo.filepath << std::endl;
            return false;
        }

        shader.inputs_.push_back(inputInfo);
    }

    return true;
}

bool ISFLoader::loadExternalImage(const std::string& imagePath, InputInfo& inputInfo, const std::string& baseDir) {
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);

    std::string fullPath = imagePath;

    // If relative path, make it relative to ISF file directory
    if (!std::filesystem::path(imagePath).is_absolute()) {
        fullPath = baseDir + "/" + imagePath;
    }

    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "External image not found: " << fullPath << std::endl;
        return false;
    }

    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(fullPath.c_str())) {
        std::cerr << "Failed to load image with DevIL: " << fullPath << std::endl;
        ilDeleteImages(1, &imageID);
        return false;
    }

    inputInfo.width = ilGetInteger(IL_IMAGE_WIDTH);
    inputInfo.height = ilGetInteger(IL_IMAGE_HEIGHT);

    // Convert to RGBA if needed
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    // Try to get texture from pool first
    GLint format = GL_RGBA8; // Standard RGBA format
    inputInfo.textureId = mainprogram->grab_from_texpool(inputInfo.width, inputInfo.height, format);

    if (inputInfo.textureId == -1) {
        // Pool miss - create new texture
        glGenTextures(1, &inputInfo.textureId);
        glBindTexture(GL_TEXTURE_2D, inputInfo.textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputInfo.width, inputInfo.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

        // Register the texture format for pooling
        mainprogram->texintfmap[inputInfo.textureId] = format;
    } else {
        // Pool hit - just upload data to existing texture
        glBindTexture(GL_TEXTURE_2D, inputInfo.textureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, inputInfo.width, inputInfo.height,
                        GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    ilDeleteImages(1, &imageID);
    return true;
}

bool ISFLoader::compileShader(const std::string& fragmentSource, ISFShader& shader) {
    // Build parameter uniform declarations
    std::string parameterUniforms;
    for (const auto& param : shader.parameterTemplates_) {
        switch (param.type) {
            case PARAM_FLOAT:
                parameterUniforms += "uniform float " + param.name + ";\n";
                break;
            case PARAM_LONG:
                parameterUniforms += "uniform int " + param.name + ";\n";
                break;
            case PARAM_BOOL:
            case PARAM_EVENT:
                parameterUniforms += "uniform bool " + param.name + ";\n";  // Use bool instead of int
                break;
            case PARAM_COLOR:
                parameterUniforms += "uniform vec4 " + param.name + ";\n";
                break;
            case PARAM_POINT2D:
                parameterUniforms += "uniform vec2 " + param.name + ";\n";
                break;
        }
    }

    // Build input texture uniform declarations
    std::string inputUniforms;
    for (const auto& input : shader.inputs_) {
        inputUniforms += "uniform sampler2D " + input.name + ";\n";

        // Add image rectangle uniforms for ALL image inputs (not just external)
        if (input.type == INPUT_IMAGE || input.type == INPUT_EXTERNAL_IMAGE) {
            // Add _inputName_imgRect uniform (x, y, width, height)
            inputUniforms += "uniform vec4 _" + input.name + "_imgRect;\n";
        }
    }

    // Build pass buffer uniform declarations
    std::string bufferUniforms;
    for (const auto& pass : shader.passes_) {
        if (!pass.target.empty()) {
            bufferUniforms += "uniform sampler2D " + pass.target + ";\n";
        }
    }

    // Choose vertex shader source
    std::string vertexSource;
    std::string customVaryingInputs;
    std::string customVaryingOutputs;  // NEW: For vertex shader outputs

    if (!shader.customVertexShader_.empty()) {
        // FIRST: Process the custom vertex shader to remove conditional blocks and extract only modern declarations
        std::stringstream vertexStream(shader.customVertexShader_);
        std::stringstream processedVertexStream;
        std::string line;
        bool skipLegacyBlock = false;
        bool inVersionConditional = false;
        std::set<std::string> uniqueVaryings;  // Track unique varying names to avoid duplicates

        // Process vertex shader line by line
        while (std::getline(vertexStream, line)) {
            // Trim whitespace for easier matching
            std::string trimmedLine = line;
            size_t start = trimmedLine.find_first_not_of(" \t");
            if (start != std::string::npos) {
                trimmedLine = trimmedLine.substr(start);
            }

            // Check for version conditional start
            if (trimmedLine.find("#if __VERSION__ <= 120") == 0) {
                skipLegacyBlock = true;
                inVersionConditional = true;
                continue; // Skip the #if line
            }
                // Check for else block
            else if (trimmedLine.find("#else") == 0 && inVersionConditional) {
                skipLegacyBlock = false;
                continue; // Skip the #else line, but start including content after it
            }
                // Check for endif
            else if (trimmedLine.find("#endif") == 0 && inVersionConditional) {
                skipLegacyBlock = false;
                inVersionConditional = false;
                continue; // Skip the #endif line
            }

            // Include line if we're not skipping the legacy block
            if (!skipLegacyBlock) {
                // Check if this is an "out" declaration (varying output)
                bool isOutDeclaration = (line.find("out ") != std::string::npos && line.find("vec") != std::string::npos);

                if (isOutDeclaration) {
                    // Extract out variable name for duplicate check
                    size_t nameStart = line.find_last_of(' ');
                    size_t nameEnd = line.find(';');
                    std::string varyingName;
                    if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                        varyingName = line.substr(nameStart + 1, nameEnd - nameStart - 1);
                    }

                    // Add to unique set if we found a valid varying and it's not already added
                    if (!varyingName.empty() && uniqueVaryings.find(varyingName) == uniqueVaryings.end()) {
                        uniqueVaryings.insert(varyingName);

                        // Add to vertex shader outputs (keep as "out")
                        customVaryingOutputs += line + "\n";

                        // Convert "out vec2 left_coord;" to "in vec2 left_coord;" for fragment shader
                        std::string varyingDeclaration = line;
                        size_t outPos = varyingDeclaration.find("out ");
                        if (outPos != std::string::npos) {
                            varyingDeclaration.replace(outPos, 4, "in ");
                        }
                        customVaryingInputs += varyingDeclaration + "\n";
                    }

                    // Don't add this line to the processed vertex stream (we'll add it to the header instead)
                } else {
                    // Not an out declaration, keep it in the processed vertex shader
                    processedVertexStream << line << "\n";
                }
            }
        }

        // Update the custom vertex shader to the processed version (without conditionals)
        shader.customVertexShader_ = processedVertexStream.str();

        if (!customVaryingInputs.empty()) {
            customVaryingInputs = "// Custom varying inputs from vertex shader\n" + customVaryingInputs + "\n";
            std::cout << "DEBUG: Added varying inputs for " << shader.name_ << ": " << uniqueVaryings.size() << " variables" << std::endl;
        }

        // Check what the custom vertex shader already declares to avoid conflicts
        bool hasCornerVars = (shader.customVertexShader_.find("topleft") != std::string::npos ||
                              shader.customVertexShader_.find("topright") != std::string::npos ||
                              shader.customVertexShader_.find("bottomleft") != std::string::npos ||
                              shader.customVertexShader_.find("bottomright") != std::string::npos);

        // Check for specific uniform declarations to avoid conflicts
        bool hasAngleUniform = shader.customVertexShader_.find("uniform float angle") != std::string::npos;
        bool hasIntensityUniform = shader.customVertexShader_.find("uniform float intensity") != std::string::npos;
        bool hasAmountUniform = shader.customVertexShader_.find("uniform float amount") != std::string::npos;
        bool hasLengthUniform = shader.customVertexShader_.find("uniform float length") != std::string::npos;

        // Also check if these are declared as parameters in the fragment shader
        std::set<std::string> paramNames;
        for (const auto& param : shader.parameterTemplates_) {
            paramNames.insert(param.name);
        }

        // Build parameter uniforms for vertex shader
        std::string customVertexUniforms;
        for (const auto& param : shader.parameterTemplates_) {
            switch (param.type) {
                case PARAM_FLOAT:
                    customVertexUniforms += "uniform float " + param.name + ";\n";
                    break;
                case PARAM_LONG:
                    customVertexUniforms += "uniform int " + param.name + ";\n";
                    break;
                case PARAM_BOOL:
                case PARAM_EVENT:
                    customVertexUniforms += "uniform bool " + param.name + ";\n";
                    break;
                case PARAM_COLOR:
                    customVertexUniforms += "uniform vec4 " + param.name + ";\n";
                    break;
                case PARAM_POINT2D:
                    customVertexUniforms += "uniform vec2 " + param.name + ";\n";
                    break;
            }
        }

        // Add common uniforms that vertex shaders might expect (only if not already declared)
        std::string commonUniforms;
        bool needsCommonHeader = false;

        if (!hasAngleUniform && paramNames.find("angle") == paramNames.end()) {
            if (!needsCommonHeader) {
                commonUniforms += "// Common uniforms for vertex shader compatibility\n";
                needsCommonHeader = true;
            }
            commonUniforms += "uniform float angle = 0.0;\n";
        }

        if (!hasIntensityUniform && paramNames.find("intensity") == paramNames.end()) {
            if (!needsCommonHeader) {
                commonUniforms += "// Common uniforms for vertex shader compatibility\n";
                needsCommonHeader = true;
            }
            commonUniforms += "uniform float intensity = 1.0;\n";
        }

        if (!hasAmountUniform && paramNames.find("amount") == paramNames.end()) {
            if (!needsCommonHeader) {
                commonUniforms += "// Common uniforms for vertex shader compatibility\n";
                needsCommonHeader = true;
            }
            commonUniforms += "uniform float amount = 1.0;\n";
        }

        if (!hasLengthUniform && paramNames.find("length") == paramNames.end()) {
            if (!needsCommonHeader) {
                commonUniforms += "// Common uniforms for vertex shader compatibility\n";
                needsCommonHeader = true;
            }
            commonUniforms += "uniform float length = 1.0;\n";
        }

        if (needsCommonHeader) {
            commonUniforms += "\n";
        }

        // Add corner variables only if not already declared
        std::string cornerVars;
        if (!hasCornerVars) {
            cornerVars = "// Corner position variables (for v002 compatibility)\n"
                         "vec2 topleft = vec2(-1.0, 1.0);\n"
                         "vec2 topright = vec2(1.0, 1.0);\n"
                         "vec2 bottomleft = vec2(-1.0, -1.0);\n"
                         "vec2 bottomright = vec2(1.0, -1.0);\n"
                         "\n";
        }

        vertexSource = "#version 330 core\n"
                       "// ISF built-in uniforms\n"
                       "uniform float TIME;\n"
                       "uniform float TIMEDELTA;\n"
                       "uniform vec2 RENDERSIZE;\n"
                       "uniform int FRAMEINDEX;\n"
                       "uniform vec4 DATE;\n"
                       "uniform int PASSINDEX;\n"
                       "\n"
                       "// Input attributes\n"
                       "layout (location = 0) in vec2 aPos;\n"
                       "layout (location = 1) in vec2 aTexCoord;\n"
                       "\n"
                       "// Output varying to fragment shader\n"
                       "out vec2 isf_FragNormCoord;\n"
                       "out vec2 vv_FragNormCoord;\n"
                       "\n";

        // Add custom varying outputs from the custom vertex shader
        if (!customVaryingOutputs.empty()) {
            vertexSource += "// Custom varying outputs\n" + customVaryingOutputs + "\n";
        }

        vertexSource += commonUniforms +
                       customVertexUniforms +
                       cornerVars +
                       "// ISF vertex shader initialization function\n"
                       "void isf_vertShaderInit() {\n"
                       "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                       "    \n"
                       "    // ISF expects inverted Y coordinate (0,0 = top-left, 1,1 = bottom-right)\n"
                       "    vec2 isfCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
                       "    \n"
                       "    isf_FragNormCoord = isfCoord;        // Inverted Y for ISF compatibility\n"
                       "    vv_FragNormCoord = isfCoord;         // Keep them the same for compatibility\n"
                       "}\n"
                       "\n" +
                       shader.customVertexShader_;

        std::cout << "DEBUG: Using custom vertex shader for " << shader.name_ << " with varying variables" << std::endl;

    } else {
        // Use default ISF vertex shader
        vertexSource = vertexShaderSource_;
        std::cout << "DEBUG: Using default vertex shader for " << shader.name_ << std::endl;
    }

    // Convert ISF shader from GLSL 120 to 330 core and fix compatibility issues
    std::string modernFragmentSource = fragmentSource;

    // FIRST: Remove legacy GLSL version conditional blocks from fragment shader
    // This prevents conflicts between old "varying" and new "in" declarations
    std::stringstream fragStream(modernFragmentSource);
    std::stringstream processedFragmentStream;
    std::string line;
    bool skipLegacyBlock = false;
    bool inVersionConditional = false;
    std::set<std::string> fragmentVaryingNames; // Track what the fragment shader already declares

    while (std::getline(fragStream, line)) {
        // Trim whitespace for easier matching
        std::string trimmedLine = line;
        size_t start = trimmedLine.find_first_not_of(" \t");
        if (start != std::string::npos) {
            trimmedLine = trimmedLine.substr(start);
        }

        // Check for version conditional start
        if (trimmedLine.find("#if __VERSION__ <= 120") == 0) {
            skipLegacyBlock = true;
            inVersionConditional = true;
            continue; // Skip the #if line
        }
            // Check for else block
        else if (trimmedLine.find("#else") == 0 && inVersionConditional) {
            skipLegacyBlock = false;
            continue; // Skip the #else line, but start including content after it
        }
            // Check for endif
        else if (trimmedLine.find("#endif") == 0 && inVersionConditional) {
            skipLegacyBlock = false;
            inVersionConditional = false;
            continue; // Skip the #endif line
        }

        // Include line if we're not skipping the legacy block
        if (!skipLegacyBlock) {
            processedFragmentStream << line << "\n";

            // Track varying variable names that the fragment shader declares
            if (line.find("in vec") != std::string::npos && line.find(";") != std::string::npos) {
                size_t nameStart = line.find_last_of(' ');
                size_t nameEnd = line.find(';');
                if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                    std::string varyingName = line.substr(nameStart + 1, nameEnd - nameStart - 1);
                    fragmentVaryingNames.insert(varyingName);
                }
            }
        }
    }

    modernFragmentSource = processedFragmentStream.str();

    // Remove any varying inputs that the fragment shader already declares
    if (!fragmentVaryingNames.empty() && !customVaryingInputs.empty()) {
        std::stringstream customVaryingStream(customVaryingInputs);
        std::stringstream filteredVaryingStream;
        std::string varyingLine;
        bool hasHeader = false;

        while (std::getline(customVaryingStream, varyingLine)) {
            if (varyingLine.find("// Custom varying") != std::string::npos) {
                // Skip header for now, add it back if we have any varying inputs
                continue;
            }

            // Check if this varying is already declared in fragment shader
            bool isDuplicate = false;
            for (const auto& fragVaryingName : fragmentVaryingNames) {
                if (varyingLine.find(fragVaryingName) != std::string::npos) {
                    isDuplicate = true;
                    break;
                }
            }

            if (!isDuplicate && !varyingLine.empty()) {
                if (!hasHeader) {
                    filteredVaryingStream << "// Custom varying inputs from vertex shader\n";
                    hasHeader = true;
                }
                filteredVaryingStream << varyingLine << "\n";
            }
        }

        if (hasHeader) {
            filteredVaryingStream << "\n";
        }
        customVaryingInputs = filteredVaryingStream.str();

        std::cout << "DEBUG: Filtered varying inputs for " << shader.name_ << " to avoid conflicts" << std::endl;
    }

    std::cout << "DEBUG: Processed fragment shader for " << shader.name_ << ", conditional blocks removed" << std::endl;

    // Replace gl_FragColor with fragColor
    size_t pos = 0;
    while ((pos = modernFragmentSource.find("gl_FragColor", pos)) != std::string::npos) {
        modernFragmentSource.replace(pos, 12, "fragColor");
        pos += 9; // length of "fragColor"
    }

    // Fix bool/int comparisons for event parameters
    pos = 0;
    while ((pos = modernFragmentSource.find("==true", pos)) != std::string::npos) {
        modernFragmentSource.replace(pos, 6, "==true");  // Keep as bool comparison
        pos += 6;
    }
    pos = 0;
    while ((pos = modernFragmentSource.find("==false", pos)) != std::string::npos) {
        modernFragmentSource.replace(pos, 7, "==false");  // Keep as bool comparison
        pos += 7;
    }

    // Fix boolean comparisons in if statements
    pos = 0;
    while ((pos = modernFragmentSource.find(" == true", pos)) != std::string::npos) {
        modernFragmentSource.replace(pos, 8, " == true");  // Keep as bool comparison
        pos += 8;
    }
    pos = 0;
    while ((pos = modernFragmentSource.find(" == false", pos)) != std::string::npos) {
        modernFragmentSource.replace(pos, 9, " == false");  // Keep as bool comparison
        pos += 9;
    }

    // Fifth pass: Fix shift operations with uint
    pos = 0;
    while ((pos = modernFragmentSource.find(">>", pos)) != std::string::npos) {
        // Look for patterns like: (8*localRow)
        size_t lineStart = modernFragmentSource.rfind('\n', pos);
        if (lineStart == std::string::npos) lineStart = 0;
        size_t lineEnd = modernFragmentSource.find('\n', pos);
        if (lineEnd == std::string::npos) lineEnd = modernFragmentSource.length();

        std::string line = modernFragmentSource.substr(lineStart, lineEnd - lineStart);

        // Check for shift operations that need uint casting
        if (line.find("localRow") != std::string::npos || line.find("localCol") != std::string::npos) {
            // Find the opening parenthesis before the shift
            size_t openParen = modernFragmentSource.rfind("(", pos);
            if (openParen != std::string::npos && openParen > lineStart) {
                size_t shiftStart = openParen + 1;
                size_t shiftEnd = pos;

                std::string shiftExpr = modernFragmentSource.substr(shiftStart, shiftEnd - shiftStart);

                // Check if it needs uint() wrapper
                if (shiftExpr.find("uint(") == std::string::npos &&
                    shiftExpr.find("8*") != std::string::npos) {
                    std::string replacement = "uint(" + shiftExpr + ")";
                    modernFragmentSource.replace(shiftStart, shiftEnd - shiftStart, replacement);
                    pos = shiftStart + replacement.length() + 2; // +2 for ">>"
                } else {
                    pos += 2;
                }
            } else {
                pos += 2;
            }
        } else {
            pos += 2;
        }
    }

    // Combine everything: version + built-ins + custom varying inputs + parameters + inputs + buffers + shader code
    std::string completeFragmentSource = "#version 330 core\n" +
                                         std::string(isfBuiltinFunctions_) +
                                         customVaryingInputs +  // Custom varying inputs from vertex shader
                                         parameterUniforms +
                                         inputUniforms +
                                         bufferUniforms +
                                         "\n" + modernFragmentSource;

    // DEBUG: Add debug output for Sorting Smear to troubleshoot the effect
    if (shader.name_.find("Sorting Smear") != std::string::npos) {
        std::cout << "=== SORTING SMEAR VERTEX SHADER ===" << std::endl;
        std::cout << vertexSource << std::endl;
        std::cout << "=== END SORTING SMEAR VERTEX SHADER ===" << std::endl;

        std::cout << "=== SORTING SMEAR FRAGMENT SHADER ===" << std::endl;
        std::cout << completeFragmentSource << std::endl;
        std::cout << "=== END SORTING SMEAR FRAGMENT SHADER ===" << std::endl;
    }

    // **NEW: Try to load from cache first**
    shader.program_ = loadCachedProgram(shader.name_, vertexSource, completeFragmentSource);

    if (shader.program_ != 0) {
        std::cout << "Cache HIT for shader: " << shader.name_ << std::endl;
        cacheUniformLocations(shader);
        return true;
    }

    std::cout << "Cache MISS for shader: " << shader.name_ << " - compiling..." << std::endl;

    shader.program_ = createShaderProgram(vertexSource.c_str(), completeFragmentSource.c_str());

    if (shader.program_ == 0) {
        // Check if this looks like an older shader with uint casting issues
        if (modernFragmentSource.find("uint") != std::string::npos &&
            modernFragmentSource.find("uvec") != std::string::npos) {
            std::cout << "WARNING: Shader '" << shader.name_ << "' failed to compile - may be using older GLSL syntax with uint casting issues." << std::endl;
            std::cout << "         This shader needs manual updating for OpenGL 3.3 core profile compatibility." << std::endl;
        }
        std::cerr << "Failed to compile shader program for: " << shader.filepath_ << std::endl;
        return false;
    }

    // **NEW: Cache the successfully compiled program**
    cacheProgram(shader.name_, vertexSource, completeFragmentSource, shader.program_);

    std::cout << "DEBUG: Returned from cacheProgram() for: " << shader.name_ << std::endl;

    cacheUniformLocations(shader);
    return true;
}

void ISFLoader::cacheUniformLocations(ISFShader& shader) {
    // Cache built-in uniform locations
    shader.timeLocation_ = glGetUniformLocation(shader.program_, "TIME");
    shader.timeDeltaLocation_ = glGetUniformLocation(shader.program_, "TIMEDELTA");
    shader.renderSizeLocation_ = glGetUniformLocation(shader.program_, "RENDERSIZE");
    shader.frameIndexLocation_ = glGetUniformLocation(shader.program_, "FRAMEINDEX");
    shader.dateLocation_ = glGetUniformLocation(shader.program_, "DATE");
    shader.passIndexLocation_ = glGetUniformLocation(shader.program_, "PASSINDEX");


    // Cache parameter uniform locations
    for (const auto& param : shader.parameterTemplates_) {
        GLint location = glGetUniformLocation(shader.program_, param.name.c_str());
        shader.paramLocations_[param.name] = location;
    }

    // Cache input uniform locations and imgRect locations
    for (const auto& input : shader.inputs_) {
        GLint location = glGetUniformLocation(shader.program_, input.name.c_str());
        shader.inputLocations_[input.name] = location;

        // Cache image rectangle uniform locations for ALL image inputs
        if (input.type == INPUT_IMAGE || input.type == INPUT_EXTERNAL_IMAGE) {
            std::string imgRectName = "_" + input.name + "_imgRect";
            GLint imgRectLocation = glGetUniformLocation(shader.program_, imgRectName.c_str());
            shader.imgRectLocations_[input.name] = imgRectLocation;

        }
    }

    // Also check for common input names
    shader.inputLocations_["inputImage"] = glGetUniformLocation(shader.program_, "inputImage");
    shader.inputLocations_["INPUT"] = glGetUniformLocation(shader.program_, "INPUT");
    for (int i = 0; i < 4; ++i) {
        std::string inputName = "INPUT" + std::to_string(i);
        shader.inputLocations_[inputName] = glGetUniformLocation(shader.program_, inputName.c_str());
    }

    // Cache pass buffer uniform locations
    for (const auto& pass : shader.passes_) {
        if (!pass.target.empty()) {
            GLint location = glGetUniformLocation(shader.program_, pass.target.c_str());
            shader.bufferLocations_[pass.target] = location;
        }
    }
}

GLuint ISFLoader::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // If ARB_parallel_shader_compile is supported, poll for completion
    if (glewIsSupported("GL_ARB_parallel_shader_compile")) {
        // Poll until both shaders are compiled (non-blocking check)
        GLint vsComplete = GL_FALSE, fsComplete = GL_FALSE;
        while (vsComplete == GL_FALSE || fsComplete == GL_FALSE) {
            if (vsComplete == GL_FALSE) {
                glGetShaderiv(vertexShader, GL_COMPLETION_STATUS_ARB, &vsComplete);
            }
            if (fsComplete == GL_FALSE) {
                glGetShaderiv(fragmentShader, GL_COMPLETION_STATUS_ARB, &fsComplete);
            }
            // Tiny sleep to avoid busy-waiting
            if (vsComplete == GL_FALSE || fsComplete == GL_FALSE) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    // Now check compile status (shaders are done compiling)
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        logShaderError(vertexShader, "VERTEX");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        logShaderError(fragmentShader, "FRAGMENT");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // Bind attribute locations BEFORE linking for GLSL 120
    glBindAttribLocation(program, 0, "aPos");
    glBindAttribLocation(program, 1, "aTexCoord");

    // Enable binary retrievability BEFORE linking (required for caching)
    glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

    glLinkProgram(program);

    // If ARB_parallel_shader_compile is supported, poll for linking completion
    if (glewIsSupported("GL_ARB_parallel_shader_compile")) {
        // Poll until program linking is complete (non-blocking check)
        GLint linkComplete = GL_FALSE;
        while (linkComplete == GL_FALSE) {
            glGetProgramiv(program, GL_COMPLETION_STATUS_ARB, &linkComplete);
            if (linkComplete == GL_FALSE) {
                // Tiny sleep to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    // Now check link status (linking is done)
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        logProgramError(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void ISFLoader::logShaderError(GLuint shader, const std::string& type) {
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> log(length);
        glGetShaderInfoLog(shader, length, NULL, log.data());
        std::cerr << "Shader " << type << " compilation error: " << log.data() << std::endl;
    }
}

void ISFLoader::logProgramError(GLuint program) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> log(length);
        glGetProgramInfoLog(program, length, NULL, log.data());
        std::cerr << "Program linking error: " << log.data() << std::endl;
    }
}

ISFLoader::ParamType ISFLoader::stringToParamType(const std::string& typeStr) {
    if (typeStr == "float") return PARAM_FLOAT;
    if (typeStr == "long") return PARAM_LONG;
    if (typeStr == "bool") return PARAM_BOOL;
    if (typeStr == "event") return PARAM_EVENT;
    if (typeStr == "color") return PARAM_COLOR;
    if (typeStr == "point2D") return PARAM_POINT2D;
    return PARAM_FLOAT; // Default fallback
}

std::string ISFLoader::paramTypeToString(ParamType type) {
    switch (type) {
        case PARAM_FLOAT: return "float";
        case PARAM_LONG: return "long";
        case PARAM_BOOL: return "bool";
        case PARAM_EVENT: return "event";
        case PARAM_COLOR: return "color";
        case PARAM_POINT2D: return "point2D";
        case PARAM_RED: return "red";
        case PARAM_GREEN: return "green";
        case PARAM_BLUE: return "blue";
        case PARAM_ALPHA: return "alpha";
        default: return "unknown";
    }
}

ISFShader* ISFLoader::findShader(const std::string& name) const {
    for (const auto& shader : shaders_) {
        std::string uppername = shader->name_;
        std::transform(uppername.begin(), uppername.end(), uppername.begin(), ::toupper);
        if (uppername == name) {
            return shader.get();
        }
    }
    return nullptr;
}

std::vector<std::string> ISFLoader::getShaderNames() const {
    std::vector<std::string> names;
    names.reserve(shaders_.size());
    for (const auto& shader : shaders_) {
        names.push_back(shader->name_);
    }
    return names;
}

void ISFLoader::getShaderNames(std::vector<std::string>& names) const {
    names.clear();
    names.reserve(shaders_.size());
    for (const auto& shader : shaders_) {
        names.push_back(shader->name_);
    }
}

void ISFLoader::clear() {
    shaders_.clear();
}

// Pool management for all loaded shaders
void ISFLoader::clearAllInstancePools() {
    for (auto& shader : shaders_) {
        shader->clearInstancePool();
    }
    std::cout << "Cleared all ISF instance pools" << std::endl;
}

// Get pool statistics
void ISFLoader::getPoolStatistics(size_t& totalPooled, size_t& totalActive) const {
    totalPooled = 0;
    totalActive = 0;
    for (const auto& shader : shaders_) {
        totalPooled += shader->getPoolSize();
        totalActive += shader->getActiveInstanceCount();
    }
}

void ISFLoader::printPoolStatistics() const {
    size_t totalPooled, totalActive;
    getPoolStatistics(totalPooled, totalActive);

    std::cout << "=== ISF Instance Pool Statistics ===" << std::endl;
    std::cout << "Total pooled instances: " << totalPooled << std::endl;
    std::cout << "Total active instances: " << totalActive << std::endl;
    std::cout << "Shader breakdown:" << std::endl;

    for (const auto& shader : shaders_) {
        if (shader->getPoolSize() > 0 || shader->getActiveInstanceCount() > 0) {
            std::cout << "  " << shader->getName()
                      << " - Pooled: " << shader->getPoolSize()
                      << ", Active: " << shader->getActiveInstanceCount() << std::endl;
        }
    }
    std::cout << "====================================" << std::endl;
}




// ===== ISFShader Implementation =====

const std::vector<ISFLoader::ParamInfo>& ISFShader::getParameterInfo() const {
    return parameterTemplates_;
}

const std::vector<ISFLoader::InputInfo>& ISFShader::getInputInfo() const {
    return inputs_;
}

const std::vector<ISFLoader::PassInfo>& ISFShader::getPassInfo() const {
    return passes_;
}

size_t ISFShader::getParameterCount() const {
    return parameterTemplates_.size();
}

size_t ISFShader::getInputCount() const {
    return inputs_.size();
}

size_t ISFShader::getPassCount() const {
    return passes_.size();
}

ISFShader::~ISFShader() {
    clearInstancePool();  // Clean up any pooled instances

    if (program_ != 0) {
        glDeleteProgram(program_);
    }
    // Clean up loaded textures using pooling
    for (const auto& input : inputs_) {
        if (input.textureId != 0) {
            mainprogram->add_to_texpool(input.textureId);
        }
    }
}




// ===== ISFShaderInstance Implementation =====

ISFShaderInstance* ISFShader::createInstance() {
    std::lock_guard<std::mutex> lock(poolMutex_);

    ISFShaderInstance* instance = nullptr;

    // Try to get an instance from the pool
    if (!instancePool_.empty()) {
        auto pooledInstance = std::move(instancePool_.back());
        instancePool_.pop_back();

        instance = pooledInstance.release();  // Transfer ownership
        instance->isInPool_ = false;
        instance->resetForReuse();  // Clean state for reuse

        std::cout << "Reused ISF instance from pool for shader: " << name_
                  << " (Pool size: " << instancePool_.size() << ")" << std::endl;
    } else {
        // Pool is empty - create new instance
        instance = new ISFShaderInstance(this);
        instance->isInPool_ = false;

        std::cout << "Created new ISF instance for shader: " << name_
                  << " (Active: " << activeInstances_ + 1 << ")" << std::endl;
    }

    activeInstances_++;
    return instance;
}

void ISFShader::clearInstancePool() {
    std::lock_guard<std::mutex> lock(poolMutex_);

    size_t poolSize = instancePool_.size();
    instancePool_.clear();

    std::cout << "Cleared ISF instance pool for shader: " << name_
              << " (Released " << poolSize << " instances)" << std::endl;
}

void ISFShader::releaseInstance(ISFShaderInstance* instance) {
    if (!instance || instance->parentShader_ != this) {
        std::cerr << "Invalid instance passed to releaseInstance!" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(poolMutex_);

    activeInstances_--;

    // Add to pool if we haven't exceeded maximum pool size
    if (instancePool_.size() < MAX_POOL_SIZE) {
        instance->isInPool_ = true;
        instancePool_.push_back(std::unique_ptr<ISFShaderInstance>(instance));

        std::cout << "Returned ISF instance to pool for shader: " << name_
                  << " (Pool size: " << instancePool_.size() << ")" << std::endl;
    } else {
        // Pool is full - just delete the instance
        delete instance;

        std::cout << "Pool full - deleted ISF instance for shader: " << name_
                  << " (Pool size: " << instancePool_.size() << ")" << std::endl;
    }
}

ISFShaderInstance::ISFShaderInstance(ISFShader* parent)
        : parentShader_(parent), fullscreenQuadVAO_(0) {
    // Initialize parameter values with defaults
    paramValues_.resize(parentShader_->parameterTemplates_.size());
    resetParametersToDefaults();
    createFullscreenQuad();

    // Initialize input texture storage
    boundInputTextures_.resize(std::max(1, (int)parentShader_->inputs_.size()));
    std::fill(boundInputTextures_.begin(), boundInputTextures_.end(), 0);

    // Initialize audio textures
    audioTextures_.resize(std::max(1, (int)parentShader_->inputs_.size()));
    std::fill(audioTextures_.begin(), audioTextures_.end(), 0);

    // NEW: Initialize instance-specific pass storage
    instancePasses_.resize(parentShader_->passes_.size());
}

ISFShaderInstance::~ISFShaderInstance() {
    // Only clean up if we're not being pooled
    if (!isInPool_) {
        // Clean up audio textures using pooling
        for (GLuint texture : audioTextures_) {
            if (texture != 0) {
                mainprogram->add_to_texpool(texture);
            }
        }
        audioTextures_.clear();

        // Instance passes will be cleaned up by their destructors which now use pooling
    }
}

bool ISFShaderInstance::setParameter(const std::string& name, float value) {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_FLOAT) {
        paramValues_[index].floatVal = value;
        return true;
    }
    return false;
}

bool ISFShaderInstance::setParameter(const std::string& name, int value) {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG ||
        paramInfo.type == ISFLoader::PARAM_BOOL ||
        paramInfo.type == ISFLoader::PARAM_EVENT) {

        // For discrete parameters, validate that the value is in the VALUES array
        if (paramInfo.hasDiscreteValues && !paramInfo.isValidValue(value)) {
            std::cerr << "Warning: Invalid value " << value << " for discrete parameter "
                      << name << ". Valid values are: ";
            for (size_t i = 0; i < paramInfo.values.size(); ++i) {
                std::cerr << paramInfo.values[i];
                if (i < paramInfo.labels.size()) {
                    std::cerr << " (\"" << paramInfo.labels[i] << "\")";
                }
                if (i < paramInfo.values.size() - 1) std::cerr << ", ";
            }
            std::cerr << std::endl;
            return false;
        }

        paramValues_[index].intVal = value;
        return true;
    }
    return false;
}

bool ISFShaderInstance::setParameter(const std::string& name, bool value) {
    return setParameter(name, value ? 1 : 0);
}

bool ISFShaderInstance::setParameter(const std::string& name, float x, float y) {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_POINT2D) {
        paramValues_[index].pointVal[0] = x;
        paramValues_[index].pointVal[1] = y;
        return true;
    }
    return false;
}

bool ISFShaderInstance::setParameter(const std::string& name, float r, float g, float b, float a) {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_COLOR) {
        paramValues_[index].colorVal[0] = r;
        paramValues_[index].colorVal[1] = g;
        paramValues_[index].colorVal[2] = b;
        paramValues_[index].colorVal[3] = a;
        return true;
    }
    return false;
}

bool ISFShaderInstance::getParameter(const std::string& name, float& value) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_FLOAT) {
        value = paramValues_[index].floatVal;
        return true;
    }
    return false;
}

bool ISFShaderInstance::getParameter(const std::string& name, int& value) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG ||
        paramInfo.type == ISFLoader::PARAM_BOOL ||
        paramInfo.type == ISFLoader::PARAM_EVENT) {
        value = paramValues_[index].intVal;
        return true;
    }
    return false;
}

bool ISFShaderInstance::getParameter(const std::string& name, bool& value) const {
    int intVal;
    if (getParameter(name, intVal)) {
        value = (intVal != 0);
        return true;
    }
    return false;
}

bool ISFShaderInstance::getParameter(const std::string& name, float& x, float& y) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_POINT2D) {
        x = paramValues_[index].pointVal[0];
        y = paramValues_[index].pointVal[1];
        return true;
    }
    return false;
}

bool ISFShaderInstance::getParameter(const std::string& name, float& r, float& g, float& b, float& a) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_COLOR) {
        r = paramValues_[index].colorVal[0];
        g = paramValues_[index].colorVal[1];
        b = paramValues_[index].colorVal[2];
        a = paramValues_[index].colorVal[3];
        return true;
    }
    return false;
}

void ISFShaderInstance::resetParametersToDefaults() {
    for (size_t i = 0; i < parentShader_->parameterTemplates_.size(); ++i) {
        const auto& paramInfo = parentShader_->parameterTemplates_[i];

        switch (paramInfo.type) {
            case ISFLoader::PARAM_FLOAT:
                paramValues_[i].floatVal = paramInfo.defaultVal;
                break;

            case ISFLoader::PARAM_LONG:
            case ISFLoader::PARAM_BOOL:
            case ISFLoader::PARAM_EVENT:
                paramValues_[i].intVal = paramInfo.defaultInt;
                break;

            case ISFLoader::PARAM_COLOR:
                for (int j = 0; j < 4; ++j) {
                    paramValues_[i].colorVal[j] = paramInfo.defaultColor[j];
                }
                break;

            case ISFLoader::PARAM_POINT2D:
                paramValues_[i].pointVal[0] = paramInfo.defaultPoint[0];
                paramValues_[i].pointVal[1] = paramInfo.defaultPoint[1];
                break;
        }
    }
}

void ISFShaderInstance::render(float time, float renderWidth, float renderHeight, int frameIndex) {
    // For generators and autonomous shaders, use auto-incrementing frame counter
    // For effects, could use the provided frameIndex from input video
    int actualFrameIndex = instanceFrameIndex_;

    // Apply any new audio data before rendering
    applyStoredAudioData();

    // Store original OpenGL state
    GLint originalFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFramebuffer);

    GLint originalDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &originalDrawBuffer);

    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);

    if (parentShader_->passes_.size() >= 1) {
        // Multi-pass rendering
        setupPassFramebuffers(renderWidth, renderHeight);

        // Double buffer initialization removed since we disabled double buffering

        glUseProgram(parentShader_->program_);

        // Render all passes to their instance buffers
        for (int passIndex = 0; passIndex < parentShader_->passes_.size(); ++passIndex) {
            renderPass(passIndex, time, renderWidth, renderHeight, actualFrameIndex,
                       originalFramebuffer, originalDrawBuffer, originalViewport);
        }

        // Display final buffer to screen
        const auto& lastPassTemplate = parentShader_->passes_.back();
        const auto& lastInstancePass = instancePasses_.back();

        if (!lastPassTemplate.target.empty()) {

            glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
            glDrawBuffer(originalDrawBuffer);
            glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, lastInstancePass.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, originalFramebuffer);

            // For GL_RGBA32F textures, we need tone mapping to prevent feedback explosion
            // Enable GL_CLAMP_TO_EDGE to prevent undefined behavior at texture edges
            glBindTexture(GL_TEXTURE_2D, lastInstancePass.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBlitFramebuffer(
                    0, 0, lastInstancePass.width, lastInstancePass.height,
                    0, 0, originalViewport[2], originalViewport[3],
                    GL_COLOR_BUFFER_BIT, GL_LINEAR
            );

            glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);

        }

    } else {
        // Single-pass rendering
        renderSinglePass(time, renderWidth, renderHeight, actualFrameIndex);
    }

    // Remove buffer swapping since we disabled double buffering
    // The odd/even flickering might be caused by buffer swap timing issues

    // Auto-increment frame counter for next render
    instanceFrameIndex_++;
}

void ISFShaderInstance::bindInputTexture(GLuint textureId, int inputIndex) {
    if (inputIndex < boundInputTextures_.size()) {
        boundInputTextures_[inputIndex] = textureId;
    }
}

// Thread-safe audio data storage (called from audio thread)
void ISFShaderInstance::storeAudioFFTData(const float* fftData, size_t binCount) {
    std::lock_guard<std::mutex> lock(audioDataMutex_);
    latestFFTData_.assign(fftData, fftData + binCount);
    hasNewFFTData_ = true;
}

void ISFShaderInstance::storeAudioSamples(const float* audioSamples, size_t sampleCount) {
    std::lock_guard<std::mutex> lock(audioDataMutex_);
    latestAudioSamples_.assign(audioSamples, audioSamples + sampleCount);
    hasNewAudioSamples_ = true;
}

// Apply stored audio data (called from video thread during render)
void ISFShaderInstance::applyStoredAudioData() {
    std::lock_guard<std::mutex> lock(audioDataMutex_);

    // Process FFT data
    if (hasNewFFTData_ && !latestFFTData_.empty()) {
        for (size_t i = 0; i < parentShader_->inputs_.size(); ++i) {
            const auto& input = parentShader_->inputs_[i];
            if (input.type == ISFLoader::INPUT_AUDIO_FFT) {
                updateAudioTexture(i, latestFFTData_.data(), latestFFTData_.size(), 1);
                break; // Assume only one FFT input per shader
            }
        }
        hasNewFFTData_ = false;
    }

    // Process audio samples
    if (hasNewAudioSamples_ && !latestAudioSamples_.empty()) {
        for (size_t i = 0; i < parentShader_->inputs_.size(); ++i) {
            const auto& input = parentShader_->inputs_[i];
            if (input.type == ISFLoader::INPUT_AUDIO) {
                updateAudioTexture(i, latestAudioSamples_.data(), latestAudioSamples_.size(), 2);
                break; // Assume only one audio input per shader
            }
        }
        hasNewAudioSamples_ = false;
    }
}

void ISFShaderInstance::createAudioTexture(int inputIndex, int width, int height) {
    if (inputIndex >= audioTextures_.size()) return;

    const auto& input = parentShader_->inputs_[inputIndex];
    GLint format;

    if (input.type == ISFLoader::INPUT_AUDIO_FFT) {
        format = GL_RGB32F; // RGB format for FFT data
    } else {
        format = GL_R32F; // Single channel for audio waveform
    }

    // Try to get texture from pool first
    audioTextures_[inputIndex] = mainprogram->grab_from_texpool(width, height, format);

    if (audioTextures_[inputIndex] == -1) {
        // Pool miss - create new texture
        glGenTextures(1, &audioTextures_[inputIndex]);
        glBindTexture(GL_TEXTURE_2D, audioTextures_[inputIndex]);

        if (input.type == ISFLoader::INPUT_AUDIO_FFT) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        }

        // Register the texture format for pooling
        mainprogram->texintfmap[audioTextures_[inputIndex]] = format;
    } else {
        // Pool hit - texture already has correct format and size
        glBindTexture(GL_TEXTURE_2D, audioTextures_[inputIndex]);
    }

    // Set texture parameters for audio data
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ISFShaderInstance::updateAudioTexture(int inputIndex, const float* audioData, size_t dataSize, int channels) {
    if (inputIndex >= audioTextures_.size() || !audioData || dataSize == 0) return;


    const auto& input = parentShader_->inputs_[inputIndex];

    // Calculate proper texture dimensions
    int textureWidth, textureHeight;
    if (input.type == ISFLoader::INPUT_AUDIO && channels == 2) {
        // For stereo audio: dataSize should be total samples, width = samples per channel
        textureWidth = dataSize / 2;  // Samples per channel
        textureHeight = 2;            // Two channels
        std::cout << "Stereo audio: " << textureWidth << " samples per channel" << std::endl;
    } else {
        // For FFT or mono audio
        textureWidth = dataSize;
        textureHeight = 1;
    }

    // Create texture if it doesn't exist with correct dimensions
    if (audioTextures_[inputIndex] == 0) {
        createAudioTexture(inputIndex, textureWidth, textureHeight);
    }

    glBindTexture(GL_TEXTURE_2D, audioTextures_[inputIndex]);

    if (input.type == ISFLoader::INPUT_AUDIO_FFT) {
        // Convert single-channel FFT data to RGB format
        std::vector<float> rgbData(dataSize * 3);
        for (size_t i = 0; i < dataSize; ++i) {
            // Put FFT magnitude in all RGB channels for shader compatibility
            rgbData[i * 3 + 0] = audioData[i];     // R
            rgbData[i * 3 + 1] = audioData[i];     // G
            rgbData[i * 3 + 2] = audioData[i];     // B
        }

        // Upload the texture data UPSIDE DOWN to flip it vertically
        // Instead of uploading to row 0, we can flip by using different texture coordinates
        // or by using a negative height (but that's not standard)
        // Actually, let's upload normally but flip the sampling coordinates
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dataSize, 1, GL_RGB, GL_FLOAT, rgbData.data());

    } else if (input.type == ISFLoader::INPUT_AUDIO && channels == 2) {
        // For interleaved stereo data: separate into two texture rows
        std::vector<float> textureData(dataSize);

        size_t samplesPerChannel = dataSize / 2;
        std::cout << "Separating " << dataSize << " interleaved samples into " << samplesPerChannel << " per channel" << std::endl;

        for (size_t i = 0; i < samplesPerChannel; ++i) {
            textureData[i] = audioData[i * 2];                        // Left channel (row 0)
            textureData[i + samplesPerChannel] = audioData[i * 2 + 1]; // Right channel (row 1)
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, samplesPerChannel, 2, GL_RED, GL_FLOAT, textureData.data());

    } else {
        // Single channel data (mono audio or other)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dataSize, 1, GL_RED, GL_FLOAT, audioData);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // Set this as the bound texture for the input
    if (inputIndex < boundInputTextures_.size()) {
        boundInputTextures_[inputIndex] = audioTextures_[inputIndex];
    }
}

void ISFShaderInstance::renderSinglePass(float time, float renderWidth, float renderHeight, int frameIndex) {
    // Store and restore original OpenGL state for consistency with multi-pass
    GLint originalFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFramebuffer);

    GLint originalDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &originalDrawBuffer);

    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);

    glUseProgram(parentShader_->program_);

    // Render to original framebuffer with original viewport
    glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
    glDrawBuffer(originalDrawBuffer);
    glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

    // Set built-in uniforms with original viewport size
    setBuiltinUniforms(time, originalViewport[2], originalViewport[3], frameIndex, 0);

    // Set parameter uniforms
    setParameterUniforms();

    // Bind input textures
    bindTextures(0);

    // Draw fullscreen quad
    drawFullscreenQuad();
}

void ISFShaderInstance::renderPass(int passIndex, float time, float renderWidth, float renderHeight, int frameIndex,
                                   GLint originalFramebuffer, GLint originalDrawBuffer, GLint originalViewport[4]) {
    const auto& passTemplate = parentShader_->passes_[passIndex];
    auto& instancePass = instancePasses_[passIndex];

    if (!passTemplate.target.empty()) {
        // CHANGE THIS LINE:
        // glBindFramebuffer(GL_FRAMEBUFFER, instancePass.framebuffer);
        // TO:
        glBindFramebuffer(GL_FRAMEBUFFER, instancePass.getCurrentWriteFramebuffer());

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, instancePass.width, instancePass.height);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR: Framebuffer not complete! Status: " << status << std::endl;
        }

        // Clear logic - only clear persistent buffers on first frame
        if (!passTemplate.persistent) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        } else if (frameIndex == 0) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        setBuiltinUniforms(time, instancePass.width, instancePass.height, frameIndex, passIndex);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
        glDrawBuffer(originalDrawBuffer);
        glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
        setBuiltinUniforms(time, originalViewport[2], originalViewport[3], frameIndex, passIndex);
    }

    setParameterUniforms();
    bindTextures(passIndex);
    drawFullscreenQuad();

    if (!passTemplate.target.empty() && passTemplate.persistent) {
        instancePass.swapBuffers();
    }
}

void ISFShaderInstance::setBuiltinUniforms(float time, float renderWidth, float renderHeight, int frameIndex, int passIndex) {
    // Calculate proper TIMEDELTA (time since last frame)
    float timeDelta;
    if (frameIndex <= 1 || lastFrameTime_ == 0.0f) {
        // First frame or initialization - use default 60fps as fallback
        timeDelta = 1.0f / 60.0f;
    } else {
        // Calculate actual time delta between frames
        timeDelta = time - lastFrameTime_;

        // Clamp to reasonable values to prevent issues with paused/resumed apps
        timeDelta = std::max(0.001f, std::min(timeDelta, 2.0f));  // Between 1ms and 100ms
    }

    // Update last frame time (only on first pass to avoid updating multiple times per frame)
    if (passIndex == 0) {
        lastFrameTime_ = time;
    }

    // Clear any existing OpenGL errors first
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }

    // Set TIME
    if (parentShader_->timeLocation_ != -1) {
        glUniform1f(parentShader_->timeLocation_, time);
    }

    // Set TIMEDELTA (ensure it's never zero or negative to prevent division issues)
    if (parentShader_->timeDeltaLocation_ != -1) {
        float safeDelta = std::max(0.001f, timeDelta); // Ensure minimum positive value
        glUniform1f(parentShader_->timeDeltaLocation_, safeDelta);
    }

    // Set RENDERSIZE
    if (parentShader_->renderSizeLocation_ != -1) {
        glUniform2f(parentShader_->renderSizeLocation_, renderWidth, renderHeight);
    }

    // Set FRAMEINDEX
    if (parentShader_->frameIndexLocation_ != -1) {
        glUniform1i(parentShader_->frameIndexLocation_, frameIndex);
    }

    // Set DATE
    if (parentShader_->dateLocation_ != -1) {
        std::time_t now = std::time(nullptr);
        std::tm* timeinfo = std::localtime(&now);

        float year = static_cast<float>(timeinfo->tm_year + 1900);
        float month = static_cast<float>(timeinfo->tm_mon + 1);
        float day = static_cast<float>(timeinfo->tm_mday);
        float seconds = static_cast<float>(timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec);

        glUniform4f(parentShader_->dateLocation_, year, month, day, seconds);
    }

    // Set PASSINDEX
    if (parentShader_->passIndexLocation_ != -1) {
        glUniform1i(parentShader_->passIndexLocation_, passIndex);
    }
}

void ISFShaderInstance::setParameterUniforms() {
    for (size_t i = 0; i < parentShader_->parameterTemplates_.size(); ++i) {
        const auto& paramInfo = parentShader_->parameterTemplates_[i];

        auto it = parentShader_->paramLocations_.find(paramInfo.name);
        if (it == parentShader_->paramLocations_.end() || it->second == -1) {
            continue;
        }

        GLint location = it->second;

        switch (paramInfo.type) {
            case ISFLoader::PARAM_FLOAT:
                glUniform1f(location, paramValues_[i].floatVal);
                break;
            case ISFLoader::PARAM_LONG:
                glUniform1i(location, paramValues_[i].intVal);
                break;
            case ISFLoader::PARAM_BOOL:
            case ISFLoader::PARAM_EVENT:
                // Use glUniform1i for bool uniforms (OpenGL doesn't have glUniform1b)
                glUniform1i(location, paramValues_[i].intVal);
                break;
            case ISFLoader::PARAM_COLOR:
                glUniform4f(location,
                            paramValues_[i].colorVal[0],
                            paramValues_[i].colorVal[1],
                            paramValues_[i].colorVal[2],
                            paramValues_[i].colorVal[3]);
                break;
            case ISFLoader::PARAM_POINT2D:
                glUniform2f(location,
                            paramValues_[i].pointVal[0],
                            paramValues_[i].pointVal[1]);
                break;
        }
    }
}

void ISFShaderInstance::bindTextures(int passIndex) {
    int textureUnit = 0;

    // CRITICAL: Bind pass buffer textures (from ALL instance passes, including current pass for feedback)
    for (size_t i = 0; i < parentShader_->passes_.size(); ++i) {
        const auto& passTemplate = parentShader_->passes_[i];
        const auto& instancePass = instancePasses_[i];

        if (!passTemplate.target.empty()) {
            auto it = parentShader_->bufferLocations_.find(passTemplate.target);
            if (it != parentShader_->bufferLocations_.end()) {
                if (it->second != -1) {
                    // Use current texture for feedback effects (no double buffering)
                    GLuint textureToUse = instancePass.getCurrentReadTexture();
                    glActiveTexture(GL_TEXTURE0 + textureUnit);
                    glBindTexture(GL_TEXTURE_2D, textureToUse);
                    glUniform1i(it->second, textureUnit);

                    textureUnit++;
                }
            }
        }
    }

    // Bind regular input textures, external images, and audio textures
    for (size_t i = 0; i < parentShader_->inputs_.size(); ++i) {
        const auto& input = parentShader_->inputs_[i];
        GLuint textureId = 0;

        if (input.type == ISFLoader::INPUT_EXTERNAL_IMAGE) {
            textureId = input.textureId;
        } else if (input.type == ISFLoader::INPUT_AUDIO_FFT || input.type == ISFLoader::INPUT_AUDIO) {
            // Use the audio texture if available
            if (i < audioTextures_.size() && audioTextures_[i] != 0) {
                textureId = audioTextures_[i];
            }
        } else if (i < boundInputTextures_.size()) {
            textureId = boundInputTextures_[i];
        }

        if (textureId != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, textureId);

            auto it = parentShader_->inputLocations_.find(input.name);
            if (it != parentShader_->inputLocations_.end() && it->second != -1) {
                glUniform1i(it->second, textureUnit);
            } else {
                // Try common input names for first texture - CRITICAL for effects like FastMosh
                if (textureUnit == 0) {
                    auto commonIt = parentShader_->inputLocations_.find("inputImage");
                    if (commonIt != parentShader_->inputLocations_.end() && commonIt->second != -1) {
                        glUniform1i(commonIt->second, textureUnit);
                    }

                    // Also try INPUT for compatibility
                    auto inputIt = parentShader_->inputLocations_.find("INPUT");
                    if (inputIt != parentShader_->inputLocations_.end() && inputIt->second != -1) {
                        glUniform1i(inputIt->second, textureUnit);
                    }
                }
            }

            // Set image rectangle uniform for regular images
            if (input.type == ISFLoader::INPUT_IMAGE || input.type == ISFLoader::INPUT_EXTERNAL_IMAGE) {
                auto imgRectIt = parentShader_->imgRectLocations_.find(input.name);
                if (imgRectIt != parentShader_->imgRectLocations_.end() && imgRectIt->second != -1) {
                    // Get texture dimensions
                    int texWidth, texHeight;
                    if (input.type == ISFLoader::INPUT_EXTERNAL_IMAGE) {
                        texWidth = input.width;
                        texHeight = input.height;
                    } else {
                        // For regular input images, get dimensions from last instance pass
                        texWidth = instancePasses_.back().width;
                        texHeight = instancePasses_.back().height;
                        if (texWidth == 0 || texHeight == 0) {
                            // Fallback to getting texture size via OpenGL
                            glBindTexture(GL_TEXTURE_2D, textureId);
                            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
                            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
                        }
                    }
                    // Set imgRect as (x, y, width, height) - typically (0, 0, width, height)
                    glUniform4f(imgRectIt->second, 0.0f, 0.0f, (float)texWidth, (float)texHeight);
                }
            }

            textureUnit++;
        }
    }
}

void ISFShaderInstance::drawFullscreenQuad() {
    // Use VAO for OpenGL core profile
    if (fullscreenQuadVAO_ == 0) {
        std::cerr << "ERROR: VAO not created!" << std::endl;
        return;
    }

    glBindVertexArray(fullscreenQuadVAO_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // Don't unbind VAO to avoid interfering with blit operations
    // glBindVertexArray(0);
}

void ISFShaderInstance::setupPassFramebuffers(float renderWidth, float renderHeight) {
    for (size_t i = 0; i < parentShader_->passes_.size(); ++i) {
        const auto& passTemplate = parentShader_->passes_[i];
        auto& instancePass = instancePasses_[i];

        if (passTemplate.target.empty()) {
            instancePass.width = (int)renderWidth;
            instancePass.height = (int)renderHeight;
            continue;
        }

        int newWidth = evaluateExpression(passTemplate.widthExpr, renderWidth, renderHeight);
        int newHeight = evaluateExpression(passTemplate.heightExpr, renderWidth, renderHeight);
        bool needsDoubleBuffer = passTemplate.persistent;

        if (instancePass.width != newWidth || instancePass.height != newHeight ||
            instancePass.framebuffer == 0 || (needsDoubleBuffer && !instancePass.useDoubleBuffer)) {

            // Clean up old resources
            if (instancePass.framebuffer != 0) {
                mainprogram->add_to_fbopool(instancePass.framebuffer);
                mainprogram->add_to_texpool(instancePass.texture);
            }
            if (instancePass.framebufferPing != 0) {
                mainprogram->add_to_fbopool(instancePass.framebufferPing);
                mainprogram->add_to_texpool(instancePass.texturePing);
                instancePass.framebufferPing = 0;
                instancePass.texturePing = 0;
            }

            instancePass.width = newWidth;
            instancePass.height = newHeight;
            instancePass.useDoubleBuffer = needsDoubleBuffer;
            instancePass.currentPingPong = false; // Reset ping pong state

            GLint format = parentShader_->usesFloatBuffers() ? GL_RGBA32F : GL_RGBA8;
            GLenum dataType = parentShader_->usesFloatBuffers() ? GL_FLOAT : GL_UNSIGNED_BYTE;

            // Create main texture
            instancePass.texture = mainprogram->grab_from_texpool(instancePass.width, instancePass.height, format);
            if (instancePass.texture == -1) {
                glGenTextures(1, &instancePass.texture);
                glBindTexture(GL_TEXTURE_2D, instancePass.texture);

                if (parentShader_->usesFloatBuffers()) {
                    std::vector<float> blackPixels(instancePass.width * instancePass.height * 4, 0.0f);
                    for (size_t j = 3; j < blackPixels.size(); j += 4) blackPixels[j] = 1.0f;
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, instancePass.width, instancePass.height, 0,
                                 GL_RGBA, GL_FLOAT, blackPixels.data());
                } else {
                    std::vector<unsigned char> blackPixels(instancePass.width * instancePass.height * 4, 0);
                    for (size_t j = 3; j < blackPixels.size(); j += 4) blackPixels[j] = 255;
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, instancePass.width, instancePass.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, blackPixels.data());
                }
                mainprogram->texintfmap[instancePass.texture] = format;
            }

            // Create ping texture for double buffering (persistent passes only)
            if (needsDoubleBuffer) {
                instancePass.texturePing = mainprogram->grab_from_texpool(instancePass.width, instancePass.height, format);
                if (instancePass.texturePing == -1) {
                    glGenTextures(1, &instancePass.texturePing);
                    glBindTexture(GL_TEXTURE_2D, instancePass.texturePing);

                    if (parentShader_->usesFloatBuffers()) {
                        std::vector<float> blackPixels(instancePass.width * instancePass.height * 4, 0.0f);
                        for (size_t j = 3; j < blackPixels.size(); j += 4) blackPixels[j] = 1.0f;
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, instancePass.width, instancePass.height, 0,
                                     GL_RGBA, GL_FLOAT, blackPixels.data());
                    } else {
                        std::vector<unsigned char> blackPixels(instancePass.width * instancePass.height * 4, 0);
                        for (size_t j = 3; j < blackPixels.size(); j += 4) blackPixels[j] = 255;
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, instancePass.width, instancePass.height, 0,
                                     GL_RGBA, GL_UNSIGNED_BYTE, blackPixels.data());
                    }
                    mainprogram->texintfmap[instancePass.texturePing] = format;
                }

                // Create ping framebuffer
                instancePass.framebufferPing = mainprogram->grab_from_fbopool();
                if (instancePass.framebufferPing == -1) {
                    glGenFramebuffers(1, &instancePass.framebufferPing);
                }
                glBindFramebuffer(GL_FRAMEBUFFER, instancePass.framebufferPing);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instancePass.texturePing, 0);

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    std::cerr << "Error creating ping framebuffer for pass: " << passTemplate.target << std::endl;
                }
            }

            // Set texture parameters for both textures
            for (GLuint tex : {instancePass.texture, instancePass.texturePing}) {
                if (tex != 0) {
                    glBindTexture(GL_TEXTURE_2D, tex);
                    if (instancePass.width <= 4 && instancePass.height <= 4) {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    } else {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    }
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
            }

            // Create main framebuffer
            instancePass.framebuffer = mainprogram->grab_from_fbopool();
            if (instancePass.framebuffer == -1) {
                glGenFramebuffers(1, &instancePass.framebuffer);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, instancePass.framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instancePass.texture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Error creating main framebuffer for pass: " << passTemplate.target << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            std::cout << "Created " << (needsDoubleBuffer ? "double-buffered" : "single")
                      << " pass: " << passTemplate.target << " (" << instancePass.width
                      << "x" << instancePass.height << ")" << std::endl;
        }
    }
}

int ISFShaderInstance::evaluateExpression(const std::string& expr, float width, float height) {
    std::string expression = expr;

    // Replace $WIDTH and $HEIGHT
    size_t pos = 0;
    while ((pos = expression.find("$WIDTH", pos)) != std::string::npos) {
        expression.replace(pos, 6, std::to_string(width));
        pos += std::to_string(width).length();
    }

    pos = 0;
    while ((pos = expression.find("$HEIGHT", pos)) != std::string::npos) {
        expression.replace(pos, 7, std::to_string(height));
        pos += std::to_string(height).length();
    }

    // Handle floor() function
    pos = 0;
    while ((pos = expression.find("floor(", pos)) != std::string::npos) {
        size_t openParen = pos + 5; // Position after "floor"
        size_t closeParen = expression.find(")", openParen);

        if (closeParen != std::string::npos) {
            // Extract the expression inside floor()
            std::string innerExpr = expression.substr(openParen + 1, closeParen - openParen - 1);

            // Evaluate the inner expression recursively
            float innerResult = 0.0f;
            try {
                if (innerExpr.find('/') != std::string::npos) {
                    size_t divPos = innerExpr.find('/');
                    float numerator = std::stof(innerExpr.substr(0, divPos));
                    float denominator = std::stof(innerExpr.substr(divPos + 1));
                    innerResult = numerator / denominator;
                } else {
                    innerResult = std::stof(innerExpr);
                }

                // Apply floor and replace the entire floor() expression
                float floorResult = std::floor(innerResult);
                expression.replace(pos, closeParen - pos + 1, std::to_string(floorResult));
                pos += std::to_string(floorResult).length();

            } catch (...) {
                std::cerr << "Error evaluating floor() inner expression: " << innerExpr << std::endl;
                pos = closeParen + 1;
            }
        } else {
            // Malformed floor() - skip
            pos += 6;
        }
    }

    // Handle ceil() function (similar to floor)
    pos = 0;
    while ((pos = expression.find("ceil(", pos)) != std::string::npos) {
        size_t openParen = pos + 4; // Position after "ceil"
        size_t closeParen = expression.find(")", openParen);

        if (closeParen != std::string::npos) {
            std::string innerExpr = expression.substr(openParen + 1, closeParen - openParen - 1);

            float innerResult = 0.0f;
            try {
                if (innerExpr.find('/') != std::string::npos) {
                    size_t divPos = innerExpr.find('/');
                    float numerator = std::stof(innerExpr.substr(0, divPos));
                    float denominator = std::stof(innerExpr.substr(divPos + 1));
                    innerResult = numerator / denominator;
                } else {
                    innerResult = std::stof(innerExpr);
                }

                float ceilResult = std::ceil(innerResult);
                expression.replace(pos, closeParen - pos + 1, std::to_string(ceilResult));
                pos += std::to_string(ceilResult).length();

            } catch (...) {
                std::cerr << "Error evaluating ceil() inner expression: " << innerExpr << std::endl;
                pos = closeParen + 1;
            }
        } else {
            pos += 5;
        }
    }

    // Simple evaluation - handle basic math operations
    try {
        // For now, handle simple division (most common case)
        if (expression.find('/') != std::string::npos) {
            size_t divPos = expression.find('/');
            float numerator = std::stof(expression.substr(0, divPos));
            float denominator = std::stof(expression.substr(divPos + 1));
            return static_cast<int>(numerator / denominator);
        } else {
            return static_cast<int>(std::stof(expression));
        }
    } catch (...) {
        std::cerr << "Error evaluating expression: " << expr << std::endl;
        return 512; // Fallback size
    }
}

void ISFShaderInstance::createFullscreenQuad() {
    if (fullscreenQuadVAO_ != 0) return; // Already created

    // Fullscreen quad vertices for OpenGL core profile
    float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
    };

    GLuint quadVBO;
    glGenVertexArrays(1, &fullscreenQuadVAO_);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(fullscreenQuadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // For OpenGL core profile, attributes must be explicitly bound
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::cout << "Created fullscreen quad VAO: " << fullscreenQuadVAO_ << " VBO: " << quadVBO << std::endl;
}

int ISFShaderInstance::findParameterIndex(const std::string& name) const {
    for (size_t i = 0; i < parentShader_->parameterTemplates_.size(); ++i) {
        if (parentShader_->parameterTemplates_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Set parameter by label (for discrete parameters with VALUES/LABELS)
bool ISFShaderInstance::setParameterByLabel(const std::string& name, const std::string& label) {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG && paramInfo.hasDiscreteValues) {
        int value = paramInfo.getValueForLabel(label);
        if (paramInfo.isValidValue(value)) {
            paramValues_[index].intVal = value;
            return true;
        }
    }
    return false;
}

// Get parameter label (for discrete parameters)
bool ISFShaderInstance::getParameterLabel(const std::string& name, std::string& label) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG && paramInfo.hasDiscreteValues) {
        label = paramInfo.getLabelForValue(paramValues_[index].intVal);
        return true;
    }
    return false;
}

// Get all available labels for a discrete parameter
bool ISFShaderInstance::getParameterLabels(const std::string& name, std::vector<std::string>& labels) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG && paramInfo.hasDiscreteValues) {
        labels = paramInfo.labels;
        return true;
    }
    return false;
}

// Get all available values for a discrete parameter
bool ISFShaderInstance::getParameterValues(const std::string& name, std::vector<int>& values) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    if (paramInfo.type == ISFLoader::PARAM_LONG && paramInfo.hasDiscreteValues) {
        values = paramInfo.values;
        return true;
    }
    return false;
}

// Check if parameter uses discrete values
bool ISFShaderInstance::isDiscreteParameter(const std::string& name) const {
    int index = findParameterIndex(name);
    if (index == -1) return false;

    const auto& paramInfo = parentShader_->parameterTemplates_[index];
    return paramInfo.hasDiscreteValues;
}

// Reset instance to clean state for reuse
void ISFShaderInstance::resetForReuse() {
    // Reset parameters to defaults
    //resetParametersToDefaults();

    // Clear bound input textures
    std::fill(boundInputTextures_.begin(), boundInputTextures_.end(), 0);

    // Reset frame counter for generators
    instanceFrameIndex_ = 0;
    lastFrameTime_ = 0.0f;

    // Clear audio data
    {
        std::lock_guard<std::mutex> lock(audioDataMutex_);
        latestFFTData_.clear();
        latestAudioSamples_.clear();
        hasNewFFTData_ = false;
        hasNewAudioSamples_ = false;
    }

    // Note: We don't reset instancePasses_ or audio textures as they will be
    // automatically managed by the pooling system when needed

    std::cout << "Reset ISF instance for reuse: " << getName() << std::endl;
}

ISFLoader::InstancePassInfo::~InstancePassInfo() {
    if (framebuffer != 0) {
        mainprogram->add_to_fbopool(framebuffer);
        framebuffer = 0;
    }
    if (texture != 0) {
        mainprogram->add_to_texpool(texture);
        texture = 0;
    }
    // NEW: Clean up ping buffers
    if (framebufferPing != 0) {
        mainprogram->add_to_fbopool(framebufferPing);
        framebufferPing = 0;
    }
    if (texturePing != 0) {
        mainprogram->add_to_texpool(texturePing);
        texturePing = 0;
    }
}