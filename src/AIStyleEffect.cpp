/**
 * AIStyleEffect.cpp
 *
 * Implementation of AIStyleEffect - wrapper around AIStyleTransfer
 *
 * License: GPL3
 */

#include "effect.h"
#include "AIStyleTransfer.h"
#include <iostream>

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

    initialized = true;
    numrows = 1;  // UI layout

    std::cerr << "[AIStyleEffect] Initialized successfully" << std::endl;
}

AIStyleEffect::~AIStyleEffect() {
    // Cleanup handled by unique_ptr
    std::cerr << "[AIStyleEffect] Destroyed" << std::endl;
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
