// ISFLoader.cpp
#include "ISFLoader.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cmath>

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
    // DevIL already initialized in main program
    devilInitialized_ = false;
}

ISFLoader::~ISFLoader() {
    clear();
}

bool ISFLoader::loadISFDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "ISF directory does not exist: " << directory << std::endl;
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".fs") {
                loadISFFile(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning ISF directory: " << e.what() << std::endl;
        return false;
    }

    std::cout << "Loaded " << shaders_.size() << " ISF shaders from " << directory << std::endl;
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

        if (!parseISFJson(fileContent, *shader)) {
            std::cerr << "Failed to parse ISF file: " << filepath << std::endl;
            return false;
        }

        // Check for custom vertex shader (.vs file with same name)
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

        shader->customVertexShader_ = customVertexSource;

        shaders_.push_back(std::move(shader));
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Exception loading ISF file " << filepath << ": " << e.what() << std::endl;
        return false;
    }
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
                param.minInt = input.contains("MIN") ? input["MIN"].get<int>() : 0;
                param.maxInt = input.contains("MAX") ? input["MAX"].get<int>() : 100;
                param.defaultInt = input.contains("DEFAULT") ? input["DEFAULT"].get<int>() : 0;
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

    // Create OpenGL texture
    glGenTextures(1, &inputInfo.textureId);
    glBindTexture(GL_TEXTURE_2D, inputInfo.textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputInfo.width, inputInfo.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

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
    if (!shader.customVertexShader_.empty()) {
        // Use custom vertex shader with built-ins
        vertexSource = "#version 330 core\n" +
                       std::string(isfVertexBuiltins_) +
                       "\n" + shader.customVertexShader_;
        std::cout << "=== USING CUSTOM VERTEX SHADER FOR: " << shader.name_ << " ===" << std::endl;
        std::cout << "Custom vertex shader content:" << std::endl;
        std::cout << shader.customVertexShader_.substr(0, 200) << "..." << std::endl;
        std::cout << "=== END CUSTOM VERTEX SHADER ===" << std::endl;
    } else {
        // Use default ISF vertex shader
        vertexSource = vertexShaderSource_;
        std::cout << "Using DEFAULT vertex shader for: " << shader.name_ << std::endl;
    }

    // Convert ISF shader from GLSL 120 to 330 core and fix compatibility issues
    std::string modernFragmentSource = fragmentSource;

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

    // Combine everything: version + built-ins + parameters + inputs + buffers + shader code
    std::string completeFragmentSource = "#version 330 core\n" +
                                         std::string(isfBuiltinFunctions_) +
                                         parameterUniforms +
                                         inputUniforms +
                                         bufferUniforms +
                                         "\n" + modernFragmentSource;

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

            // Debug output
            std::cout << "Cached imgRect uniform: " << imgRectName << " -> location " << imgRectLocation << std::endl;
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
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        logShaderError(vertexShader, "VERTEX");
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

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

    glLinkProgram(program);

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
        if (shader->name_ == name) {
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
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
    // Clean up loaded textures
    for (const auto& input : inputs_) {
        if (input.textureId != 0) {
            glDeleteTextures(1, &input.textureId);
        }
    }
    // Clean up pass framebuffers
    for (const auto& pass : passes_) {
        if (pass.framebuffer != 0) {
            glDeleteFramebuffers(1, &pass.framebuffer);
        }
        if (pass.texture != 0) {
            glDeleteTextures(1, &pass.texture);
        }
    }
}

ISFShaderInstance* ISFShader::createInstance() {
    return new ISFShaderInstance(this);
}

// ===== ISFShaderInstance Implementation =====

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
    // Apply any new audio data before rendering
    applyStoredAudioData();

    // Store original OpenGL state
    GLint originalFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFramebuffer);

    GLint originalDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &originalDrawBuffer);

    GLint originalViewport[4];
    glGetIntegerv(GL_VIEWPORT, originalViewport);

    if (parentShader_->passes_.size() > 1) {
        // Multi-pass rendering
        setupPassFramebuffers(renderWidth, renderHeight);
        glUseProgram(parentShader_->program_);

        for (int passIndex = 0; passIndex < parentShader_->passes_.size(); ++passIndex) {
            renderPass(passIndex, time, renderWidth, renderHeight, frameIndex,
                       originalFramebuffer, originalDrawBuffer, originalViewport);
        }
    } else {
        // Single-pass rendering
        renderSinglePass(time, renderWidth, renderHeight, frameIndex);
    }
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

    glGenTextures(1, &audioTextures_[inputIndex]);
    glBindTexture(GL_TEXTURE_2D, audioTextures_[inputIndex]);

    // For FFT data, use RGB format to match shader expectations
    const auto& input = parentShader_->inputs_[inputIndex];
    if (input.type == ISFLoader::INPUT_AUDIO_FFT) {
        // Create RGB texture for FFT data (shader expects fft.r + fft.g + fft.b)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    } else {
        // Audio waveform can stay single channel
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
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

    // Debug output
    float minVal = audioData[0], maxVal = audioData[0];
    for (size_t i = 0; i < dataSize; i++) {
        minVal = std::min(minVal, audioData[i]);
        maxVal = std::max(maxVal, audioData[i]);
    }
    std::cout << "Updating audio texture " << inputIndex << ": " << dataSize
              << " samples, " << channels << " channels, range: " << minVal << " to " << maxVal << std::endl;

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
    const auto& pass = parentShader_->passes_[passIndex];

    std::cout << "Rendering pass " << passIndex << " (target: '" << pass.target << "')" << std::endl;

    // Bind appropriate framebuffer BEFORE any drawing
    if (passIndex == parentShader_->passes_.size() - 1) {
        // Final pass - restore original framebuffer FIRST
        std::cout << "Final pass - rendering to screen" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
        glDrawBuffer(originalDrawBuffer);
        glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

        // Set built-in uniforms with original viewport size
        setBuiltinUniforms(time, originalViewport[2], originalViewport[3], frameIndex, passIndex);
    } else {
        // Intermediate pass - render to pass framebuffer
        std::cout << "Intermediate pass - rendering to framebuffer " << pass.framebuffer
                  << " (" << pass.width << "x" << pass.height << ")" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glViewport(0, 0, pass.width, pass.height);

        // Clear the framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Set built-in uniforms with pass size
        setBuiltinUniforms(time, pass.width, pass.height, frameIndex, passIndex);
    }

    // Set parameter uniforms
    setParameterUniforms();

    // Bind input textures and pass buffers
    bindTextures(passIndex);

    // Draw fullscreen quad
    drawFullscreenQuad();

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL error after pass " << passIndex << ": " << error << std::endl;
    }
}

void ISFShaderInstance::setBuiltinUniforms(float time, float renderWidth, float renderHeight, int frameIndex, int passIndex) {
    // Calculate TIMEDELTA (time since last frame)
    static float lastTime = 0.0f;
    float timeDelta = time - lastTime;
    lastTime = time;

    // For first frame, use a reasonable default (assuming 60fps)
    if (frameIndex <= 1) {
        timeDelta = 1.0f / 60.0f;
    }

    // Clear any existing OpenGL errors first
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }

    // Set TIME
    if (parentShader_->timeLocation_ != -1) {
        glUniform1f(parentShader_->timeLocation_, time);
    }

    // Set TIMEDELTA
    if (parentShader_->timeDeltaLocation_ != -1) {
        glUniform1f(parentShader_->timeDeltaLocation_, timeDelta);
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

    // Bind pass buffer textures (from previous passes)
    for (int i = 0; i < passIndex; ++i) {
        const auto& prevPass = parentShader_->passes_[i];
        if (!prevPass.target.empty()) {
            auto it = parentShader_->bufferLocations_.find(prevPass.target);
            if (it != parentShader_->bufferLocations_.end() && it->second != -1) {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, prevPass.texture);
                glUniform1i(it->second, textureUnit);
                textureUnit++;
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
                // Try common input names for first texture
                if (textureUnit == 0) {
                    auto commonIt = parentShader_->inputLocations_.find("inputImage");
                    if (commonIt != parentShader_->inputLocations_.end() && commonIt->second != -1) {
                        glUniform1i(commonIt->second, textureUnit);
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
                        // For regular input images, we'll assume they match render size
                        // You might want to track actual input texture dimensions
                        texWidth = (int)parentShader_->passes_.back().width;
                        texHeight = (int)parentShader_->passes_.back().height;
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
    glBindVertexArray(0);
}

void ISFShaderInstance::setupPassFramebuffers(float renderWidth, float renderHeight) {
    for (auto& pass : parentShader_->passes_) {
        if (pass.target.empty()) {
            // Final pass - use render dimensions, no framebuffer needed
            pass.width = (int)renderWidth;
            pass.height = (int)renderHeight;
            std::cout << "Final pass: " << pass.width << "x" << pass.height << std::endl;
            continue;
        }

        // Calculate pass dimensions
        int newWidth = evaluateExpression(pass.widthExpr, renderWidth, renderHeight);
        int newHeight = evaluateExpression(pass.heightExpr, renderWidth, renderHeight);

        std::cout << "Pass '" << pass.target << "': " << pass.widthExpr << " -> " << newWidth
                  << ", " << pass.heightExpr << " -> " << newHeight << std::endl;

        // Only recreate if dimensions changed or framebuffer doesn't exist
        if (pass.width != newWidth || pass.height != newHeight || pass.framebuffer == 0) {
            // Clean up old resources
            if (pass.framebuffer != 0) {
                glDeleteFramebuffers(1, &pass.framebuffer);
                glDeleteTextures(1, &pass.texture);
            }

            pass.width = newWidth;
            pass.height = newHeight;

            // Create texture
            glGenTextures(1, &pass.texture);
            glBindTexture(GL_TEXTURE_2D, pass.texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pass.width, pass.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create framebuffer
            glGenFramebuffers(1, &pass.framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass.texture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Error creating framebuffer for pass: " << pass.target << std::endl;
            } else {
                std::cout << "Created framebuffer for pass '" << pass.target << "': "
                          << pass.width << "x" << pass.height << " (texture ID: " << pass.texture << ")" << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
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