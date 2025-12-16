/**
 * AIStyleEffect.cpp
 *
 * Implementation of AIStyleEffect - wrapper around AIStyleTransfer
 *
 * USAGE:
 * ------
 * To use the compute shader path:
 *
 *   AIStyleEffect* effect = new AIStyleEffect();
 *
 *   // Process a frame (input and output are GL textures)
 *   effect->applyStyle(inputTexture, outputTexture, width, height);
 *
 * The effect will automatically use the GPU compute shader path if available.
 * Currently implements normalize→denormalize pass-through for testing.
 *
 * License: GPL3
 */

#include "effect.h"
#include "AIStyleTransfer.h"  // Includes FBOstruct definition
#include "ComputeStyleTransfer.h"
#include <iostream>
#include <fstream>
#include <cstdlib>  // For system()

AIStyleEffect::AIStyleEffect(int styleIndex) : Effect() {
    type = AI_STYLE;
    initialized = false;
    lastStyleIndex = -1;
    lastProcessingScale = 1.0f;

    // Create style transfer instance
    styleTransfer = std::make_unique<AIStyleTransfer>();

    // Initialize ONNX Runtime
    if (!styleTransfer->initialize()) {
        std::cerr << "[AIStyleEffect] Initialization failed: "
                  << styleTransfer->getLastError() << std::endl;
        return;
    }

    // Load styles from platform-specific models directory
    std::string modelsPath;
    #ifdef _WIN32
        modelsPath = "C:/ProgramData/EWOCvj2/models/styles/";
    #else
        modelsPath = "/usr/share/EWOCvj2/models/styles/";
    #endif

    int numStyles = styleTransfer->loadStyles(modelsPath);
    if (numStyles == 0) {
        std::cerr << "[AIStyleEffect] No style models found in " << modelsPath << std::endl;
        std::cerr << "[AIStyleEffect] Place .onnx model files in " << modelsPath << " directory" << std::endl;
        // Continue anyway - effect can be used without models (passthrough mode)
    } else {
        std::cerr << "[AIStyleEffect] Loaded " << numStyles << " style models" << std::endl;

        /*// Auto-extract weights for any models that don't have .weights files
        auto availableStyles = styleTransfer->getAvailableStyles();
        int extractedCount = 0;
        for (const auto& styleName : availableStyles) {
            std::string onnxPath = modelsPath + styleName;
            if (onnxPath.find(".onnx") == std::string::npos) {
                onnxPath += ".onnx";
            }

            // Check if .weights file exists
            std::string weightsPath = onnxPath;
            size_t dotPos = weightsPath.rfind(".onnx");
            if (dotPos != std::string::npos) {
                weightsPath.replace(dotPos, 5, ".weights");
            }

            std::ifstream weightsCheck(weightsPath);
            if (!weightsCheck.good()) {
                // Weights file doesn't exist - extract it
                std::cerr << "[AIStyleEffect] Extracting weights for: " << styleName << std::endl;

                // Find Python executable with onnx module installed
                // Check EWOCVJ2_PYTHON environment variable first, then fall back to PATH
                std::string pythonCmd;

                // Try environment variable first
                const char* envPython = std::getenv("EWOCVJ2_PYTHON");
                if (envPython && envPython[0] != '\0') {
                    #ifdef _WIN32
                        std::string testCmd = std::string("\"") + envPython + "\" -c \"import onnx\" >nul 2>&1";
                    #else
                        std::string testCmd = std::string("\"") + envPython + "\" -c \"import onnx\" >/dev/null 2>&1";
                    #endif
                    if (system(testCmd.c_str()) == 0) {
                        pythonCmd = std::string("\"") + envPython + "\"";
                        std::cerr << "[AIStyleEffect] Using Python from EWOCVJ2_PYTHON: " << envPython << std::endl;
                    }
                }

                // Fall back to PATH
                if (pythonCmd.empty()) {
                    #ifdef _WIN32
                        const char* candidates[] = {"py", "python"};
                        for (const char* candidate : candidates) {
                            std::string testCmd = std::string(candidate) + " -c \"import onnx\" >nul 2>&1";
                            if (system(testCmd.c_str()) == 0) {
                                pythonCmd = candidate;
                                break;
                            }
                        }
                    #else
                        const char* candidates[] = {"python3", "python"};
                        for (const char* candidate : candidates) {
                            std::string testCmd = std::string(candidate) + " -c \"import onnx\" >/dev/null 2>&1";
                            if (system(testCmd.c_str()) == 0) {
                                pythonCmd = candidate;
                                break;
                            }
                        }
                    #endif
                }

                if (pythonCmd.empty()) {
                    std::cerr << "[AIStyleEffect] ⚠ Python with 'onnx' module not found - cannot extract weights for " << styleName << std::endl;
                    std::cerr << "[AIStyleEffect]   Install Python and run: pip install onnx" << std::endl;
                    std::cerr << "[AIStyleEffect]   Or set EWOCVJ2_PYTHON environment variable to Python path" << std::endl;
                    std::cerr << "[AIStyleEffect]   Model will not work without .weights file" << std::endl;
                    continue;
                }

                // Build path to extraction script
                std::string scriptPath;
                #ifdef _WIN32
                    // Try development path first
                    scriptPath = "C:/Users/gertd/source/EWOCvj2/CLion/EWOCvj2-git/scripts/extract_weights.py";
                    std::ifstream devScript(scriptPath);
                    if (!devScript.good()) {
                        // Try installed path
                        scriptPath = "C:/ProgramData/EWOCvj2/scripts/extract_weights.py";
                    }
                #else
                    scriptPath = "/usr/share/EWOCvj2/scripts/extract_weights.py";
                #endif

                // Check if script exists
                std::ifstream scriptCheck(scriptPath);
                if (!scriptCheck.good()) {
                    std::cerr << "[AIStyleEffect] ⚠ Extraction script not found: " << scriptPath << std::endl;
                    std::cerr << "[AIStyleEffect]   Model will not work without .weights file" << std::endl;
                    continue;
                }

                // Run extraction
                std::string command = pythonCmd + " \"" + scriptPath + "\" \"" + onnxPath + "\" \"" + weightsPath + "\" 2>&1";
                std::cerr << "[AIStyleEffect] Running: " << command << std::endl;
                int result = system(command.c_str());
                if (result == 0) {
                    extractedCount++;
                    std::cerr << "[AIStyleEffect] ✓ Successfully extracted weights for " << styleName << std::endl;
                } else {
                    std::cerr << "[AIStyleEffect] ✗ Failed to extract weights for " << styleName << " (exit code " << result << ")" << std::endl;
                    std::cerr << "[AIStyleEffect]   Install Python 'onnx' module: pip install onnx" << std::endl;
                }
            }
        }

        if (extractedCount > 0) {
            std::cerr << "[AIStyleEffect] Extracted weights for " << extractedCount << " model(s)" << std::endl;
        }*/
    }

    /*// Create parameters
    // Style selection parameter
    Param* styleParam = new Param();
    styleParam->name = "Style";
    styleParam->type = FF_TYPE_OPTION;  // Discrete option parameter
    styleParam->sliding = false;  // Integer/discrete values, not continuous

    // Use provided styleIndex if valid, otherwise default to 0
    float initialStyle = (styleIndex >= 0 && styleIndex < numStyles) ? static_cast<float>(styleIndex) : 0.0f;
    styleParam->value = initialStyle;
    styleParam->deflt = initialStyle;
    styleParam->range[0] = 0.0f;
    styleParam->range[1] = numStyles > 0 ? static_cast<float>(numStyles - 1) : 0.0f;

    // Populate option names with style names
    if (numStyles > 0) {
        auto styleNames = styleTransfer->getAvailableStyles();
        for (const auto& styleName : styleNames) {
            styleParam->options.push_back(styleName);
        }
    }

    styleParam->shadervar = "ai_style_index";
    styleParam->effect = this;
    params.push_back(styleParam);

    // Processing quality/scale parameter (1.0 = full res, 0.5 = half res, etc.)
    Param* qualityParam = new Param();
    qualityParam->name = "Quality";
    qualityParam->value = 0.75f;  // Default to 75% resolution for better performance
    qualityParam->deflt = 0.75f;
    qualityParam->range[0] = 0.25f;  // Quarter res minimum
    qualityParam->range[1] = 1.0f;    // Full res maximum
    qualityParam->shadervar = "ai_quality";
    qualityParam->effect = this;
    params.push_back(qualityParam);*/

    // Set initial style if available
    if (numStyles > 0) {
        int initStyle = (styleIndex >= 0 && styleIndex < numStyles) ? styleIndex : 0;
        styleTransfer->setStyle(initStyle);
        lastStyleIndex = initStyle;
    }

    // Note: processingResolution is auto-detected from model input shape
    // Models with fixed dimensions (e.g. 224×224) will auto-downscale
    // Models with dynamic dimensions will use full input resolution

    // Use ONNX Runtime (compute shaders disabled for now)
    useComputeShaders = false;

    initialized = true;
    numrows = 1;  // UI layout

    std::cerr << "[AIStyleEffect] Initialized successfully" << std::endl;
}

AIStyleEffect::~AIStyleEffect() {
    // Cleanup handled by unique_ptr
    std::cerr << "[AIStyleEffect] Destroyed" << std::endl;
}

bool AIStyleEffect::applyStyle(GLuint inputTexture, GLuint outputTexture, int width, int height) {
    if (!initialized) {
        std::cerr << "[AIStyleEffect] Not initialized" << std::endl;
        return false;
    }

    // ONNX Runtime Path
    if (styleTransfer) {
        // Create temporary FBOs wrapping the textures (needed for async PBO pipeline)
        static GLuint tempInputFBO = 0;
        static GLuint tempOutputFBO = 0;

        if (tempInputFBO == 0) {
            glGenFramebuffers(1, &tempInputFBO);
        }
        if (tempOutputFBO == 0) {
            glGenFramebuffers(1, &tempOutputFBO);
        }

        // Attach input texture to temp FBO
        glBindFramebuffer(GL_FRAMEBUFFER, tempInputFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, inputTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Attach output texture to temp FBO
        glBindFramebuffer(GL_FRAMEBUFFER, tempOutputFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create FBOstruct wrappers
        FBOstruct inputFBO;
        inputFBO.texture = inputTexture;
        inputFBO.width = width;
        inputFBO.height = height;
        inputFBO.framebuffer = tempInputFBO;

        FBOstruct outputFBO;
        outputFBO.texture = outputTexture;
        outputFBO.width = width;
        outputFBO.height = height;
        outputFBO.framebuffer = tempOutputFBO;

        // Use async rendering
        bool result = styleTransfer->render(inputFBO, outputFBO, false);

        // Print timing every 60 frames
        static int frameCount = 0;
        if (++frameCount % 60 == 0) {
            std::cerr << "[ONNX Runtime] Inference time: " << styleTransfer->getLastInferenceTime()
                      << "ms, FPS: " << styleTransfer->getLastFPS() << std::endl;
        }

        return result;
    } else {
        std::cerr << "[AIStyleEffect] No rendering path available!" << std::endl;
        return false;
    }
}

void AIStyleEffect::updateStyle() {
    if (!initialized || !styleTransfer || params.empty()) return;

    int newStyleIndex = static_cast<int>(params[0]->value);
    auto availableStyles = styleTransfer->getAvailableStyles();

    if (newStyleIndex < 0 || newStyleIndex >= static_cast<int>(availableStyles.size())) {
        return;
    }

    if (newStyleIndex != lastStyleIndex) {
        if (styleTransfer->setStyle(newStyleIndex)) {
            lastStyleIndex = newStyleIndex;
            std::cerr << "[AIStyleEffect] Switched to style: "
                      << availableStyles[newStyleIndex] << std::endl;
        }
    }
}

void AIStyleEffect::updateProcessingResolution(int width, int height) {
    if (!initialized || !styleTransfer || params.size() < 2) return;

    float scale = params[1]->value;

    if (scale <= 0.0f || scale > 1.0f) {
        scale = 1.0f;
    }

    if (scale != lastProcessingScale) {
        int procWidth = static_cast<int>(width * scale);
        int procHeight = static_cast<int>(height * scale);

        styleTransfer->setProcessingResolution(procWidth, procHeight);
        lastProcessingScale = scale;

        std::cerr << "[AIStyleEffect] Processing resolution: "
                  << procWidth << "x" << procHeight
                  << " (" << (scale * 100.0f) << "%)" << std::endl;
    }
}
