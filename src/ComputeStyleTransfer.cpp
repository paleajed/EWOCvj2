/**
 * ComputeStyleTransfer.cpp
 *
 * GPU-accelerated style transfer implementation
 *
 * License: GPL3
 */

#include "ComputeStyleTransfer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>  // For system()

// ONNX protobuf headers for weight extraction
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4146 4244 4267 4456)
#endif

// Check if ONNX is available
#if __has_include(<onnx/onnx_pb.h>)
#define HAS_ONNX_PROTO 1
#include <onnx/onnx_pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#else
#define HAS_ONNX_PROTO 0
#warning "ONNX protobuf headers not found - compute shader weight loading will be unavailable"
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ============================================================================
// GLSL Compute Shader Sources
// ============================================================================

// Normalize input: [0,1] -> [-1,1]
// Input: Regular GL_TEXTURE_2D, Output: GL_TEXTURE_2D_ARRAY (1 layer)
static const char* NORMALIZE_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform readonly image2D inputImage;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);

    if (coord.x >= size.x || coord.y >= size.y) return;

    vec4 pixel = imageLoad(inputImage, coord);

    // Convert [0,1] to [-1,1] (ReCoNet models expect this range)
    pixel.rgb = pixel.rgb * 2.0 - 1.0;

    // Write to first layer of output array
    imageStore(outputImages, ivec3(coord, 0), pixel);
}
)";

// Denormalize output: [-1,1] -> [0,1]
// Input: GL_TEXTURE_2D_ARRAY (1 layer), Output: Regular GL_TEXTURE_2D
static const char* DENORMALIZE_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba8) uniform writeonly image2D outputImage;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Read from first layer of input array
    vec4 pixel = imageLoad(inputImages, ivec3(coord, 0));

    // Convert [-1,1] to [0,1]
    pixel.rgb = (pixel.rgb + 1.0) * 0.5;
    pixel.rgb = clamp(pixel.rgb, 0.0, 1.0);
    pixel.a = 1.0;

    imageStore(outputImage, coord, pixel);
}
)";

// Lanczos3 resampling (high-quality up/downscaling)
static const char* LANCZOS3_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform readonly image2D inputImage;
layout(binding = 1, rgba8) uniform writeonly image2D outputImage;

uniform ivec2 inputSize;
uniform ivec2 outputSize;

#define PI 3.14159265359

// Lanczos kernel with a=3
float lanczos3(float x) {
    if (x == 0.0) return 1.0;
    if (abs(x) >= 3.0) return 0.0;

    float pix = PI * x;
    return (3.0 * sin(pix) * sin(pix / 3.0)) / (pix * pix);
}

void main() {
    ivec2 outCoord = ivec2(gl_GlobalInvocationID.xy);

    if (outCoord.x >= outputSize.x || outCoord.y >= outputSize.y) return;

    // Calculate source position in input texture
    vec2 scale = vec2(inputSize) / vec2(outputSize);
    vec2 srcPos = (vec2(outCoord) + 0.5) * scale;

    // Calculate sample region (6x6 for Lanczos3)
    ivec2 srcCenter = ivec2(floor(srcPos));

    vec4 sum = vec4(0.0);
    float weightSum = 0.0;

    // Sample 6x6 neighborhood
    for (int dy = -2; dy <= 3; dy++) {
        for (int dx = -2; dx <= 3; dx++) {
            ivec2 samplePos = srcCenter + ivec2(dx, dy);

            // Clamp to input bounds
            samplePos = clamp(samplePos, ivec2(0), inputSize - 1);

            // Calculate Lanczos weights
            vec2 diff = srcPos - vec2(samplePos) - 0.5;
            float wx = lanczos3(diff.x);
            float wy = lanczos3(diff.y);
            float weight = wx * wy;

            vec4 texel = imageLoad(inputImage, samplePos);
            sum += texel * weight;
            weightSum += weight;
        }
    }

    // Normalize by total weight
    if (weightSum > 0.0) {
        sum /= weightSum;
    }

    imageStore(outputImage, outCoord, sum);
}
)";

// ReLU activation
static const char* RELU_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

uniform int channels;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Process all texture layers
    int numLayers = (channels + 3) / 4;
    for (int layer = 0; layer < numLayers; layer++) {
        vec4 pixel = imageLoad(inputImages, ivec3(coord, layer));
        pixel = max(pixel, vec4(0.0));
        imageStore(outputImages, ivec3(coord, layer), pixel);
    }
}
)";

// Tanh activation
static const char* TANH_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

uniform int channels;
uniform float scale;  // Scaling factor before tanh

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Process all texture layers
    int numLayers = (channels + 3) / 4;
    for (int layer = 0; layer < numLayers; layer++) {
        vec4 pixel = imageLoad(inputImages, ivec3(coord, layer));
        // Scale before tanh to prevent saturation
        pixel = tanh(pixel * scale);
        imageStore(outputImages, ivec3(coord, layer), pixel);
    }
}
)";

// Clamp and normalize: [0,255] → [0,1]
static const char* CLAMP_NORMALIZE_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

uniform int channels;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Process all texture layers
    int numLayers = (channels + 3) / 4;
    for (int layer = 0; layer < numLayers; layer++) {
        vec4 pixel = imageLoad(inputImages, ivec3(coord, layer));
        // Clamp to [0, 255] and normalize to [0, 1]
        pixel = clamp(pixel, 0.0, 255.0) / 255.0;
        imageStore(outputImages, ivec3(coord, layer), pixel);
    }
}
)";

// Copy from texture array layer 0 to regular 2D texture (with format conversion)
static const char* COPY_ARRAY_TO_2D_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputArray;
layout(binding = 1, rgba8) uniform writeonly image2D outputImage;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Read from layer 0 of input array
    vec4 pixel = imageLoad(inputArray, ivec3(coord, 0));

    // Set alpha to 1 and write to output
    pixel.a = 1.0;
    imageStore(outputImage, coord, pixel);
}
)";

// Residual addition
static const char* RESIDUAL_ADD_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform readonly image2DArray residualImages;
layout(binding = 2, rgba32f) uniform writeonly image2DArray outputImages;

uniform int channels;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Process all texture layers
    int numLayers = (channels + 3) / 4;
    for (int layer = 0; layer < numLayers; layer++) {
        vec4 input = imageLoad(inputImages, ivec3(coord, layer));
        vec4 residual = imageLoad(residualImages, ivec3(coord, layer));
        imageStore(outputImages, ivec3(coord, layer), input + residual);
    }
}
)";

// Conv2D with multi-channel support via texture arrays
// Processes output layers using Z dimension (or uniform for fallback)
static const char* CONV2D_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Input/output texture arrays
layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

// Weight and bias buffers
layout(std430, binding = 2) readonly buffer WeightBuffer { float weights[]; };
layout(std430, binding = 3) readonly buffer BiasBuffer { float biases[]; };

// Layer parameters
uniform int inChannels;
uniform int outChannels;
uniform int kernelSize;
uniform int stride;
uniform int padding;
uniform ivec2 inputSize;
uniform int outLayerIndex;  // Single output layer to process

// Reflection padding for coordinates
ivec2 reflectCoord(ivec2 coord, ivec2 size) {
    ivec2 result = coord;

    // Reflect x coordinate
    if (result.x < 0) result.x = -result.x;
    else if (result.x >= size.x) result.x = 2 * size.x - result.x - 2;

    // Reflect y coordinate
    if (result.y < 0) result.y = -result.y;
    else if (result.y >= size.y) result.y = 2 * size.y - result.y - 2;

    // Clamp to valid range
    return clamp(result, ivec2(0), size - 1);
}

void main() {
    ivec2 outCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 outputSize = imageSize(outputImages).xy;

    if (outCoord.x >= outputSize.x || outCoord.y >= outputSize.y) return;

    // Compute input coordinate (accounting for stride and padding)
    ivec2 inCoordBase = outCoord * stride - padding;

    // Calculate number of input texture layers
    int inTexLayers = (inChannels + 3) / 4;

    vec4 result = vec4(0.0);

    // Process 4 output channels for THIS single layer
    for (int oc = 0; oc < 4; oc++) {
        int outChannel = outLayerIndex * 4 + oc;
        if (outChannel >= outChannels) break;

        float sum = 0.0;

        // Convolve over all input channels
        for (int inTexIdx = 0; inTexIdx < inTexLayers; inTexIdx++) {
            for (int ic = 0; ic < 4; ic++) {
                int inChannel = inTexIdx * 4 + ic;
                if (inChannel >= inChannels) break;

                // Convolve kernel
                for (int ky = 0; ky < kernelSize; ky++) {
                    for (int kx = 0; kx < kernelSize; kx++) {
                        ivec2 inCoord = inCoordBase + ivec2(kx, ky);
                        ivec2 paddedCoord = reflectCoord(inCoord, inputSize);

                        vec4 inputPixel = imageLoad(inputImages, ivec3(paddedCoord, inTexIdx));
                        float inputVal = inputPixel[ic];

                        int weightIdx = ((outChannel * inChannels + inChannel) * kernelSize + ky) * kernelSize + kx;
                        sum += inputVal * weights[weightIdx];
                    }
                }
            }
        }

        sum += biases[outChannel];
        result[oc] = sum;
    }

    // Write output
    imageStore(outputImages, ivec3(outCoord, outLayerIndex), result);
}
)";

// Instance Normalization Pass 1: Compute Statistics
static const char* INSTANCE_NORM_STATS_SHADER = R"(
#version 430 core

layout(local_size_x = 256) in;

// Input texture array - supports arbitrary channel counts
layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;

// Output statistics buffer: vec2(mean, variance) per channel
layout(std430, binding = 2) writeonly buffer StatsBuffer {
    vec2 stats[];  // [channel] = (mean, variance)
};

uniform int channels;
uniform ivec2 imageSize;

shared float sharedMeans[256];
shared float sharedVars[256];

void main() {
    int channelIdx = int(gl_WorkGroupID.x);
    if (channelIdx >= channels) return;

    // Determine which texture layer and RGBA component this channel belongs to
    int texLayer = channelIdx / 4;
    int component = channelIdx % 4;

    // Parallel reduction to compute mean and variance
    int totalPixels = imageSize.x * imageSize.y;
    int pixelsPerThread = (totalPixels + 255) / 256;
    int startPixel = int(gl_LocalInvocationID.x) * pixelsPerThread;
    int endPixel = min(startPixel + pixelsPerThread, totalPixels);

    float sum = 0.0;
    float sumSq = 0.0;

    for (int i = startPixel; i < endPixel; i++) {
        ivec2 coord = ivec2(i % imageSize.x, i / imageSize.x);
        vec4 pixel = imageLoad(inputImages, ivec3(coord, texLayer));
        float val = pixel[component];
        sum += val;
        sumSq += val * val;
    }

    sharedMeans[gl_LocalInvocationID.x] = sum;
    sharedVars[gl_LocalInvocationID.x] = sumSq;
    barrier();

    // Tree reduction
    for (int stride = 128; stride > 0; stride >>= 1) {
        if (gl_LocalInvocationID.x < stride) {
            sharedMeans[gl_LocalInvocationID.x] += sharedMeans[gl_LocalInvocationID.x + stride];
            sharedVars[gl_LocalInvocationID.x] += sharedVars[gl_LocalInvocationID.x + stride];
        }
        barrier();
    }

    // Thread 0 writes final result
    if (gl_LocalInvocationID.x == 0) {
        float mean = sharedMeans[0] / float(totalPixels);
        float variance = (sharedVars[0] / float(totalPixels)) - (mean * mean);
        stats[channelIdx] = vec2(mean, variance);
    }
}
)";

// Instance Normalization Pass 2: Apply Normalization
// This can process all layers efficiently since it's just element-wise operations
static const char* INSTANCE_NORM_APPLY_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

// Input/output texture arrays - support arbitrary channel counts
layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

// Statistics from pass 1
layout(std430, binding = 2) readonly buffer StatsBuffer { vec2 stats[]; };

// Instance norm parameters (gamma and beta)
layout(std430, binding = 3) readonly buffer GammaBuffer { float gamma[]; };
layout(std430, binding = 4) readonly buffer BetaBuffer { float beta[]; };

uniform int channels;
uniform float epsilon;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImages).xy;

    if (coord.x >= size.x || coord.y >= size.y) return;

    // Calculate number of texture layers
    int numTexLayers = (channels + 3) / 4;

    // Process each texture layer (lightweight operation, OK to do all at once)
    for (int texLayer = 0; texLayer < numTexLayers; texLayer++) {
        vec4 inputPixel = imageLoad(inputImages, ivec3(coord, texLayer));
        vec4 result = vec4(0.0);

        // Process each RGBA component
        for (int c = 0; c < 4; c++) {
            int channel = texLayer * 4 + c;
            if (channel >= channels) {
                result[c] = 0.0;  // Zero out unused channels
                continue;
            }

            vec2 stat = stats[channel];
            float mean = stat.x;
            float variance = stat.y;
            float stddev = sqrt(variance + epsilon);

            // Normalize
            float normalized = (inputPixel[c] - mean) / stddev;

            // Apply affine transformation
            result[c] = gamma[channel] * normalized + beta[channel];
        }

        imageStore(outputImages, ivec3(coord, texLayer), result);
    }
}
)";

// Transposed Convolution (Deconvolution) for upsampling
// Processes ALL output layers in a SINGLE dispatch using Z dimension
static const char* TRANSPOSED_CONV_SHADER = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Input/output texture arrays
layout(binding = 0, rgba32f) uniform readonly image2DArray inputImages;
layout(binding = 1, rgba32f) uniform writeonly image2DArray outputImages;

// Weight and bias buffers
layout(std430, binding = 2) readonly buffer WeightBuffer { float weights[]; };
layout(std430, binding = 3) readonly buffer BiasBuffer { float biases[]; };

// Layer parameters
uniform int inChannels;
uniform int outChannels;
uniform int kernelSize;
uniform int stride;
uniform int padding;
uniform ivec2 inputSize;
uniform int outLayerStart;  // Starting output layer (process 4 at once: start, start+1, start+2, start+3)

void main() {
    ivec2 outCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 outputSize = imageSize(outputImages).xy;

    if (outCoord.x >= outputSize.x || outCoord.y >= outputSize.y) return;

    // Calculate number of input texture layers
    int inTexLayers = (inChannels + 3) / 4;

    // Process 4 output layers at once to reduce dispatch count
    for (int layerOffset = 0; layerOffset < 4; layerOffset++) {
        int outLayerIndex = outLayerStart + layerOffset;
        int numOutLayers = (outChannels + 3) / 4;
        if (outLayerIndex >= numOutLayers) break;

        vec4 result = vec4(0.0);

        // Process 4 output channels for this layer
        for (int oc = 0; oc < 4; oc++) {
            int outChannel = outLayerIndex * 4 + oc;
            if (outChannel >= outChannels) break;

            float sum = 0.0;

            // For transposed convolution with stride=2:
            // Each output pixel can receive contributions from multiple input pixels
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int inX = (outCoord.x + padding - kx) / stride;
                    int inY = (outCoord.y + padding - ky) / stride;

                    // Check if this input pixel is valid and aligned
                    if ((outCoord.x + padding - kx) % stride == 0 &&
                        (outCoord.y + padding - ky) % stride == 0 &&
                        inX >= 0 && inX < inputSize.x &&
                        inY >= 0 && inY < inputSize.y) {

                        ivec2 inCoord = ivec2(inX, inY);

                        // Accumulate contributions from all input channels
                        for (int inTexIdx = 0; inTexIdx < inTexLayers; inTexIdx++) {
                            for (int ic = 0; ic < 4; ic++) {
                                int inChannel = inTexIdx * 4 + ic;
                                if (inChannel >= inChannels) break;

                                vec4 inputPixel = imageLoad(inputImages, ivec3(inCoord, inTexIdx));
                                float inputVal = inputPixel[ic];

                                int weightIdx = ((outChannel * inChannels + inChannel) * kernelSize + ky) * kernelSize + kx;
                                sum += inputVal * weights[weightIdx];
                            }
                        }
                    }
                }
            }

            sum += biases[outChannel];
            result[oc] = sum;
        }

        // Write output for this layer
        imageStore(outputImages, ivec3(outCoord, outLayerIndex), result);
    }
}
)";

// ============================================================================
// Constructor / Destructor
// ============================================================================

ComputeStyleTransfer::ComputeStyleTransfer() {
    // Query GPU capabilities for adaptive batching
    GLint maxWorkGroupCount[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupCount[2]);

    // Use 50% of max Z workgroups to avoid hitting limits
    // Clamp between 4 and 32 for safety
    maxBatchSize = std::max(4, std::min(32, maxWorkGroupCount[2] / 2));

    std::cerr << "[ComputeStyleTransfer] GPU max Z workgroups: " << maxWorkGroupCount[2]
              << ", using batch size: " << maxBatchSize << std::endl;
}

ComputeStyleTransfer::~ComputeStyleTransfer() {
    cleanup();
}

void ComputeStyleTransfer::cleanup() {
    // Delete shader programs
    if (normalizeProgram) glDeleteProgram(normalizeProgram);
    if (denormalizeProgram) glDeleteProgram(denormalizeProgram);
    if (lanczos3Program) glDeleteProgram(lanczos3Program);
    if (conv2dProgram) glDeleteProgram(conv2dProgram);
    if (instanceNormStatsProgram) glDeleteProgram(instanceNormStatsProgram);
    if (instanceNormApplyProgram) glDeleteProgram(instanceNormApplyProgram);
    if (reluProgram) glDeleteProgram(reluProgram);
    if (tanhProgram) glDeleteProgram(tanhProgram);
    if (transposedConvProgram) glDeleteProgram(transposedConvProgram);
    if (residualAddProgram) glDeleteProgram(residualAddProgram);

    // Delete stats buffer
    if (statsBuffer) glDeleteBuffers(1, &statsBuffer);

    // Delete weight buffers
    for (auto& pair : layerWeights) {
        if (pair.second.weightBuffer) glDeleteBuffers(1, &pair.second.weightBuffer);
        if (pair.second.biasBuffer) glDeleteBuffers(1, &pair.second.biasBuffer);
    }
    layerWeights.clear();

    // Delete instance norm buffers
    for (auto& pair : instanceNormParams) {
        if (pair.second.gammaBuffer) glDeleteBuffers(1, &pair.second.gammaBuffer);
        if (pair.second.betaBuffer) glDeleteBuffers(1, &pair.second.betaBuffer);
    }
    instanceNormParams.clear();

    // Delete temp textures
    for (auto& temp : tempTextures) {
        if (temp.texture) glDeleteTextures(1, &temp.texture);
    }
    tempTextures.clear();

    modelLoaded = false;
}

// ============================================================================
// Shader Compilation
// ============================================================================

GLuint ComputeStyleTransfer::compileComputeShader(const char* source, const std::string& shaderName) {
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        setError("Failed to compile " + shaderName + ": " + infoLog);
        glDeleteShader(shader);
        return 0;
    }

    // Create program
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    // Check linking
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        setError("Failed to link " + shaderName + ": " + infoLog);
        glDeleteShader(shader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(shader);
    return program;
}

bool ComputeStyleTransfer::compileShaders() {
    std::cerr << "[ComputeStyleTransfer] Compiling compute shaders..." << std::endl;

    normalizeProgram = compileComputeShader(NORMALIZE_SHADER, "normalize");
    if (!normalizeProgram) return false;

    denormalizeProgram = compileComputeShader(DENORMALIZE_SHADER, "denormalize");
    if (!denormalizeProgram) return false;

    lanczos3Program = compileComputeShader(LANCZOS3_SHADER, "lanczos3");
    if (!lanczos3Program) return false;

    reluProgram = compileComputeShader(RELU_SHADER, "relu");
    if (!reluProgram) return false;

    tanhProgram = compileComputeShader(TANH_SHADER, "tanh");
    if (!tanhProgram) return false;

    clampNormalizeProgram = compileComputeShader(CLAMP_NORMALIZE_SHADER, "clamp_normalize");
    if (!clampNormalizeProgram) return false;

    copyArrayTo2DProgram = compileComputeShader(COPY_ARRAY_TO_2D_SHADER, "copy_array_to_2d");
    if (!copyArrayTo2DProgram) return false;

    residualAddProgram = compileComputeShader(RESIDUAL_ADD_SHADER, "residual_add");
    if (!residualAddProgram) return false;

    conv2dProgram = compileComputeShader(CONV2D_SHADER, "conv2d");
    if (!conv2dProgram) return false;

    instanceNormStatsProgram = compileComputeShader(INSTANCE_NORM_STATS_SHADER, "instance_norm_stats");
    if (!instanceNormStatsProgram) return false;

    instanceNormApplyProgram = compileComputeShader(INSTANCE_NORM_APPLY_SHADER, "instance_norm_apply");
    if (!instanceNormApplyProgram) return false;

    transposedConvProgram = compileComputeShader(TRANSPOSED_CONV_SHADER, "transposed_conv");
    if (!transposedConvProgram) return false;

    std::cerr << "[ComputeStyleTransfer] All compute shaders compiled successfully!" << std::endl;

    // Initialize uniform caches for performance
    conv2dCache = std::make_unique<UniformCache>(conv2dProgram);
    instanceNormStatsCache = std::make_unique<UniformCache>(instanceNormStatsProgram);
    instanceNormApplyCache = std::make_unique<UniformCache>(instanceNormApplyProgram);
    transposedConvCache = std::make_unique<UniformCache>(transposedConvProgram);
    residualAddCache = std::make_unique<UniformCache>(residualAddProgram);
    clampNormalizeCache = std::make_unique<UniformCache>(clampNormalizeProgram);

    return true;
}

// ============================================================================
// Model Loading (Placeholder - will implement ONNX loading)
// ============================================================================

bool ComputeStyleTransfer::loadModel(const std::string& modelPath) {
    std::cerr << "[ComputeStyleTransfer] Loading model: " << modelPath << std::endl;

    cleanup();

    if (!compileShaders()) {
        return false;
    }

    // If modelPath is empty, just compile shaders for testing
    if (modelPath.empty()) {
        std::cerr << "[ComputeStyleTransfer] Test mode - shaders compiled, no weights loaded" << std::endl;
        modelLoaded = false;
        return true;
    }

    // Load weights from ONNX model
    if (!loadWeightsFromONNX(modelPath)) {
        std::cerr << "[ComputeStyleTransfer] Warning: Failed to load weights, but shaders are compiled" << std::endl;
        modelLoaded = false;
        return true;  // Still return true so we can test shader compilation
    }

    modelLoaded = true;
    std::cerr << "[ComputeStyleTransfer] Model loaded successfully!" << std::endl;
    return true;
}

// ============================================================================
// Forward Pass (Main Processing Function)
// ============================================================================

// Helper to save texture as PPM (simple image format)
void debugSaveTexture(GLuint tex, int w, int h, const std::string& filename) {
    std::vector<unsigned char> pixels(w * h * 4);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    // Write PPM file (simple text-based image format)
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n" << w << " " << h << "\n255\n";

    // Write RGB data (flip Y and skip alpha)
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            file.put(pixels[idx]);     // R
            file.put(pixels[idx + 1]); // G
            file.put(pixels[idx + 2]); // B
        }
    }
    file.close();

    // Sample center pixel for console output
    int centerIdx = (h/2 * w + w/2) * 4;
    std::cerr << "[DEBUG SAVED " << filename << "] Center pixel: R=" << (int)pixels[centerIdx]
              << " G=" << (int)pixels[centerIdx+1] << " B=" << (int)pixels[centerIdx+2] << std::endl;
}

// Helper to save texture array (extracts first 3-4 channels from layer 0)
void debugSaveTextureArray(GLuint texArray, int w, int h, const std::string& filename) {
    std::vector<unsigned char> pixels(w * h * 4);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texArray, 0, 0);  // Layer 0

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    // Write PPM file
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n" << w << " " << h << "\n255\n";

    // Write RGB data (flip Y and skip alpha)
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            file.put(pixels[idx]);     // R
            file.put(pixels[idx + 1]); // G
            file.put(pixels[idx + 2]); // B
        }
    }
    file.close();

    int centerIdx = (h/2 * w + w/2) * 4;
    std::cerr << "[DEBUG SAVED " << filename << "] Center pixel (layer 0): R=" << (int)pixels[centerIdx]
              << " G=" << (int)pixels[centerIdx+1] << " B=" << (int)pixels[centerIdx+2] << std::endl;
}

bool ComputeStyleTransfer::process(GLuint inputTexture, GLuint outputTexture, int inputWidth, int inputHeight, int outputWidth, int outputHeight) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // GPU timer query
    static GLuint gpuQuery = 0;
    if (gpuQuery == 0) {
        glGenQueries(1, &gpuQuery);
    }
    glBeginQuery(GL_TIME_ELAPSED, gpuQuery);

    // Test mode: Just normalize → denormalize
    // This tests shader compilation and basic compute pipeline
    if (!modelLoaded) {
        std::cerr << "[ComputeStyleTransfer] Running in test mode (normalize→denormalize)" << std::endl;
    }

    // DEBUG: Enable to dump intermediate textures
    bool debugDump = false;  // Set to true for debugging

    // DEBUG: Dump input texture
    if (debugDump) {
        debugSaveTexture(inputTexture, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_input.ppm");
    }

    // Process at INPUT dimensions, then scale to OUTPUT dimensions
    // Allocate temp textures for full ReCoNet pipeline
    GLuint normalizedTex;  // Normalized input [H, W, 3]

    if (!getOrCreateTempTexture(0, inputWidth, inputHeight, 3, normalizedTex)) {
        return false;
    }

    // Normalize at INPUT dimensions
    if (!runNormalize(inputTexture, normalizedTex, inputWidth, inputHeight)) {
        return false;
    }

    if (debugDump) {
        debugSaveTextureArray(normalizedTex, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_01_normalized.ppm");
    }

    GLuint processedTex = normalizedTex;  // Track current texture through pipeline

    if (modelLoaded) {
        // === ENCODER ===
        // Conv1: 3→32, 9x9, stride=1 + InstanceNorm + ReLU  [H, W, 32]
        GLuint conv1Tex;
        if (!getOrCreateTempTexture(1, inputWidth, inputHeight, 32, conv1Tex)) return false;
        if (!runConv2D("conv1.conv2d", processedTex, conv1Tex, inputWidth, inputHeight)) return false;
        if (!runInstanceNorm("in1", conv1Tex, conv1Tex, inputWidth, inputHeight, 32)) return false;
        if (!runReLU(conv1Tex, conv1Tex, inputWidth, inputHeight, 32)) return false;

        if (debugDump) {
            debugSaveTextureArray(conv1Tex, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_02_conv1.ppm");
        }

        // Conv2: 32→64, 3x3, stride=2 + InstanceNorm + ReLU  [H/2, W/2, 64]
        int h2 = inputHeight / 2, w2 = inputWidth / 2;
        GLuint conv2Tex;
        if (!getOrCreateTempTexture(2, w2, h2, 64, conv2Tex)) return false;
        if (!runConv2D("conv2.conv2d", conv1Tex, conv2Tex, inputWidth, inputHeight)) return false;
        if (!runInstanceNorm("in2", conv2Tex, conv2Tex, w2, h2, 64)) return false;
        if (!runReLU(conv2Tex, conv2Tex, w2, h2, 64)) return false;

        if (debugDump) {
            debugSaveTextureArray(conv2Tex, w2, h2, "C:/ProgramData/EWOCvj2/debug_03_conv2.ppm");
        }

        // Conv3: 64→128, 3x3, stride=2 + InstanceNorm + ReLU  [H/4, W/4, 128]
        int h4 = h2 / 2, w4 = w2 / 2;
        GLuint conv3Tex;
        if (!getOrCreateTempTexture(3, w4, h4, 128, conv3Tex)) return false;
        if (!runConv2D("conv3.conv2d", conv2Tex, conv3Tex, w2, h2)) return false;
        if (!runInstanceNorm("in3", conv3Tex, conv3Tex, w4, h4, 128)) return false;
        if (!runReLU(conv3Tex, conv3Tex, w4, h4, 128)) return false;

        if (debugDump) {
            debugSaveTextureArray(conv3Tex, w4, h4, "C:/ProgramData/EWOCvj2/debug_04_conv3.ppm");
        }

        processedTex = conv3Tex;

        // === RESIDUAL BLOCKS (5 blocks) ===
        GLuint resTemp1, resTemp2, resTemp3;
        if (!getOrCreateTempTexture(4, w4, h4, 128, resTemp1)) return false;
        if (!getOrCreateTempTexture(5, w4, h4, 128, resTemp2)) return false;
        if (!getOrCreateTempTexture(10, w4, h4, 128, resTemp3)) return false;  // Additional buffer for output

        for (int i = 1; i <= 5; i++) {
            std::string blockName = "res" + std::to_string(i);
            GLuint residualInput = processedTex;

            // First conv + norm + relu (use resTemp1 as intermediate)
            if (!runConv2D(blockName + ".conv1.conv2d", residualInput, resTemp1, w4, h4)) return false;
            if (!runInstanceNorm(blockName + ".in1", resTemp1, resTemp1, w4, h4, 128)) return false;
            if (!runReLU(resTemp1, resTemp1, w4, h4, 128)) return false;

            // Second conv + norm (use resTemp2 as intermediate)
            if (!runConv2D(blockName + ".conv2.conv2d", resTemp1, resTemp2, w4, h4)) return false;
            if (!runInstanceNorm(blockName + ".in2", resTemp2, resTemp2, w4, h4, 128)) return false;

            // Residual add (output to resTemp3 to avoid overwriting residualInput)
            if (!runResidualAdd(resTemp2, residualInput, resTemp3, w4, h4, 128)) return false;

            // Swap: next iteration uses resTemp3 as input
            processedTex = resTemp3;
        }

        if (debugDump) {
            debugSaveTextureArray(processedTex, w4, h4, "C:/ProgramData/EWOCvj2/debug_05_residuals.ppm");
        }

        // === DECODER ===
        // DeConv1: 128→64, 3x3, stride=2 (upsample to H/2, W/2) + InstanceNorm + ReLU
        GLuint deconv1Tex;
        if (!getOrCreateTempTexture(6, w2, h2, 64, deconv1Tex)) return false;
        if (!runTransposedConv("deconv1.conv2d", processedTex, deconv1Tex, w4, h4, w2, h2)) return false;
        if (!runInstanceNorm("in4", deconv1Tex, deconv1Tex, w2, h2, 64)) return false;
        if (!runReLU(deconv1Tex, deconv1Tex, w2, h2, 64)) return false;

        if (debugDump) {
            debugSaveTextureArray(deconv1Tex, w2, h2, "C:/ProgramData/EWOCvj2/debug_06_deconv1.ppm");
        }

        // DeConv2: 64→32, 3x3, stride=2 (upsample to H, W) + InstanceNorm + ReLU
        GLuint deconv2Tex;
        if (!getOrCreateTempTexture(7, inputWidth, inputHeight, 32, deconv2Tex)) return false;
        if (!runTransposedConv("deconv2.conv2d", deconv1Tex, deconv2Tex, w2, h2, inputWidth, inputHeight)) return false;
        if (!runInstanceNorm("in5", deconv2Tex, deconv2Tex, inputWidth, inputHeight, 32)) return false;
        if (!runReLU(deconv2Tex, deconv2Tex, inputWidth, inputHeight, 32)) return false;

        if (debugDump) {
            debugSaveTextureArray(deconv2Tex, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_07_deconv2.ppm");
        }

        // Conv_out: 32→3, 9x9, stride=1 + Tanh
        GLuint convOutTex;
        if (!getOrCreateTempTexture(8, inputWidth, inputHeight, 3, convOutTex)) return false;
        if (!runConv2D("deconv3.conv2d", deconv2Tex, convOutTex, inputWidth, inputHeight)) return false;

        // DEBUG code removed for performance - was causing GPU stalls

        // THIS MODEL OUTPUTS PIXEL VALUES [0,255] DIRECTLY (NO TANH LAYER!)
        // Simply clamp to [0,255] and normalize to [0,1]
        if (!runClampAndNormalize(convOutTex, convOutTex, inputWidth, inputHeight, 3)) return false;

        if (debugDump) {
            debugSaveTextureArray(convOutTex, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_08_final_conv.ppm");
        }

        processedTex = convOutTex;

    } else {
        std::cerr << "[ComputeStyleTransfer] Model not loaded - using pass-through" << std::endl;
    }

    // Output is already in [0,1] from clamp+normalize. Just copy to regular 2D texture.
    GLuint outputReadyTex;
    if (!getOrCreateTempTexture(9, inputWidth, inputHeight, 4, outputReadyTex, true)) {
        return false;
    }

    // Copy from texture array to regular 2D (values already in [0,1])
    if (!runCopyArrayToRegular(processedTex, outputReadyTex, inputWidth, inputHeight)) {
        return false;
    }

    // DEBUG: Dump output before Lanczos
    if (debugDump) {
        debugSaveTexture(outputReadyTex, inputWidth, inputHeight, "C:/ProgramData/EWOCvj2/debug_before_lanczos.ppm");
    }

    // Scale to OUTPUT dimensions using Lanczos3 (high-quality resampling)
    if (inputWidth != outputWidth || inputHeight != outputHeight) {
        if (!runLanczos3(outputReadyTex, outputTexture, inputWidth, inputHeight, outputWidth, outputHeight)) {
            return false;
        }
    } else {
        // Same dimensions, just copy
        if (!runLanczos3(outputReadyTex, outputTexture, inputWidth, inputHeight, outputWidth, outputHeight)) {
            return false;
        }
    }

    // DEBUG: Dump final output
    if (debugDump) {
        debugSaveTexture(outputTexture, outputWidth, outputHeight, "C:/ProgramData/EWOCvj2/debug_final_output.ppm");
    }

    glEndQuery(GL_TIME_ELAPSED);

    auto endTime = std::chrono::high_resolution_clock::now();
    float cpuTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    // Get GPU time (blocking call - forces synchronization)
    GLuint64 gpuTimeNs = 0;
    glGetQueryObjectui64v(gpuQuery, GL_QUERY_RESULT, &gpuTimeNs);
    float gpuTimeMs = gpuTimeNs / 1000000.0f;

    lastInferenceTimeMs = cpuTimeMs;

    return true;
}

// ============================================================================
// Layer Implementations
// ============================================================================

bool ComputeStyleTransfer::runNormalize(GLuint inputTex, GLuint outputTex, int width, int height) {
    glUseProgram(normalizeProgram);

    // Input: regular GL_TEXTURE_2D, Output: GL_TEXTURE_2D_ARRAY
    glBindImageTexture(0, inputTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runDenormalize(GLuint inputTex, GLuint outputTex, int width, int height) {
    glUseProgram(denormalizeProgram);

    // Input: GL_TEXTURE_2D_ARRAY, Output: regular GL_TEXTURE_2D
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runCopyArrayToRegular(GLuint inputArrayTex, GLuint outputTex, int width, int height) {
    // Copy from RGBA32F array to RGBA8 2D texture using shader
    glUseProgram(copyArrayTo2DProgram);

    // Bind textures
    glBindImageTexture(0, inputArrayTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runLanczos3(GLuint inputTex, GLuint outputTex,
                                        int inputWidth, int inputHeight,
                                        int outputWidth, int outputHeight) {
    glUseProgram(lanczos3Program);

    // Bind textures
    glBindImageTexture(0, inputTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Set uniforms
    glUniform2i(glGetUniformLocation(lanczos3Program, "inputSize"), inputWidth, inputHeight);
    glUniform2i(glGetUniformLocation(lanczos3Program, "outputSize"), outputWidth, outputHeight);

    // Dispatch based on output dimensions
    int workGroupsX = (outputWidth + 15) / 16;
    int workGroupsY = (outputHeight + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

// Placeholder implementations for other layers
bool ComputeStyleTransfer::runReLU(GLuint inputTex, GLuint outputTex, int width, int height, int channels) {
    glUseProgram(reluProgram);

    // Set channels uniform
    glUniform1i(glGetUniformLocation(reluProgram, "channels"), channels);

    // Bind texture arrays
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runTanh(GLuint inputTex, GLuint outputTex, int width, int height, int channels) {
    glUseProgram(tanhProgram);

    // Set uniforms
    glUniform1i(glGetUniformLocation(tanhProgram, "channels"), channels);

    // BY THE BOOK: No scaling! Network output goes directly to tanh
    // According to fast style transfer theory: output = tanh(network_raw_output)
    // This produces [-1, 1] which is then denormalized to [0, 1] via (x+1)/2
    float scale = 1.0f;  // NO SCALING
    glUniform1f(glGetUniformLocation(tanhProgram, "scale"), scale);

    // Bind texture arrays
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runTanhSoftClamp(GLuint inputTex, GLuint outputTex, int width, int height, int channels) {
    // Apply tanh to values already in [0,1] range for soft clamping
    // This maps: 0→0, 0.5→0.46, 1.0→0.76
    // Effectively darkens and compresses dynamic range
    glUseProgram(tanhProgram);

    glUniform1i(glGetUniformLocation(tanhProgram, "channels"), channels);
    glUniform1f(glGetUniformLocation(tanhProgram, "scale"), 1.0f);  // No additional scaling

    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runClampAndNormalize(GLuint inputTex, GLuint outputTex, int width, int height, int channels) {
    glUseProgram(clampNormalizeProgram);

    // Set channels uniform using cache
    clampNormalizeCache->setInt("channels", channels);

    // Bind texture arrays
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runConv2D(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                                     int width, int height) {
    if (layerWeights.find(layerName) == layerWeights.end()) {
        setError("Conv2D layer not found: " + layerName);
        return false;
    }

    LayerWeights& lw = layerWeights[layerName];

    glUseProgram(conv2dProgram);

    // Bind weight and bias SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lw.weightBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lw.biasBuffer);

    // Set uniforms using cache
    conv2dCache->setInt("inChannels", lw.inChannels);
    conv2dCache->setInt("outChannels", lw.outChannels);
    conv2dCache->setInt("kernelSize", lw.kernelSize);
    conv2dCache->setInt("stride", lw.stride);
    conv2dCache->setInt("padding", lw.padding);
    conv2dCache->setInt2("inputSize", width, height);

    // Bind texture arrays (GL_TRUE for layered access)
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Calculate output dimensions (accounting for stride)
    int outputWidth = (width + 2 * lw.padding - lw.kernelSize) / lw.stride + 1;
    int outputHeight = (height + 2 * lw.padding - lw.kernelSize) / lw.stride + 1;

    // Dispatch serially - one layer at a time (no Z-dimension)
    int outTexLayers = (lw.outChannels + 3) / 4;
    int workGroupsX = (outputWidth + 15) / 16;
    int workGroupsY = (outputHeight + 15) / 16;

    // Dispatch one per layer - simple and doesn't hang (but slow)
    for (int layerIdx = 0; layerIdx < outTexLayers; layerIdx++) {
        conv2dCache->setInt("outLayerIndex", layerIdx);
        glDispatchCompute(workGroupsX, workGroupsY, 1);
    }

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runInstanceNorm(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                                           int width, int height, int channels) {
    if (instanceNormParams.find(layerName) == instanceNormParams.end()) {
        setError("Instance Norm layer not found: " + layerName);
        return false;
    }

    InstanceNormParams& inp = instanceNormParams[layerName];

    // Ensure stats buffer is large enough
    int requiredSize = channels * sizeof(float) * 2;  // vec2 per channel
    if (statsBufferSize < requiredSize) {
        if (statsBuffer) glDeleteBuffers(1, &statsBuffer);

        glGenBuffers(1, &statsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, statsBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        statsBufferSize = requiredSize;
    }

    // === Pass 1: Compute Statistics ===
    glUseProgram(instanceNormStatsProgram);

    // Bind input texture array (GL_TRUE for layered)
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);

    // Bind stats buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, statsBuffer);

    // Set uniforms using cache
    instanceNormStatsCache->setInt("channels", channels);
    instanceNormStatsCache->setInt2("imageSize", width, height);

    // Dispatch - one workgroup per channel, 256 threads per workgroup
    glDispatchCompute(channels, 1, 1);

    // Memory barrier - ensure stats are written before Pass 2 reads them
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // === Pass 2: Apply Normalization ===
    glUseProgram(instanceNormApplyProgram);

    // Bind input/output texture arrays (GL_TRUE for layered)
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Bind stats buffer (read-only in pass 2)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, statsBuffer);

    // Bind gamma and beta buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, inp.gammaBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, inp.betaBuffer);

    // Set uniforms using cache
    instanceNormApplyCache->setInt("channels", channels);
    instanceNormApplyCache->setFloat("epsilon", 1e-5f);

    // Dispatch
    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runTransposedConv(const std::string& layerName, GLuint inputTex, GLuint outputTex,
                                             int inputWidth, int inputHeight, int outputWidth, int outputHeight) {
    if (layerWeights.find(layerName) == layerWeights.end()) {
        setError("TransposedConv layer not found: " + layerName);
        std::cerr << "[TransposedConv] ERROR: Layer not found: " << layerName << std::endl;
        return false;
    }

    LayerWeights& lw = layerWeights[layerName];

    glUseProgram(transposedConvProgram);

    // Bind weight and bias SSBOs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lw.weightBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lw.biasBuffer);

    // Set uniforms using cache
    transposedConvCache->setInt("inChannels", lw.inChannels);
    transposedConvCache->setInt("outChannels", lw.outChannels);
    transposedConvCache->setInt("kernelSize", lw.kernelSize);
    transposedConvCache->setInt("stride", lw.stride);
    transposedConvCache->setInt("padding", lw.padding);
    transposedConvCache->setInt2("inputSize", inputWidth, inputHeight);

    // Bind texture arrays (GL_TRUE for layered)
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Dispatch in batches of 4 layers (processed in shader loop)
    // This reduces dispatch count by 4x
    int outTexLayers = (lw.outChannels + 3) / 4;
    int workGroupsX = (outputWidth + 15) / 16;
    int workGroupsY = (outputHeight + 15) / 16;

    for (int layerStart = 0; layerStart < outTexLayers; layerStart += 4) {
        transposedConvCache->setInt("outLayerStart", layerStart);
        glDispatchCompute(workGroupsX, workGroupsY, 1);
    }

    // Single memory barrier after ALL layers
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

bool ComputeStyleTransfer::runResidualAdd(GLuint inputTex, GLuint residualTex, GLuint outputTex,
                                          int width, int height, int channels) {
    glUseProgram(residualAddProgram);

    // Set channels uniform using cache
    residualAddCache->setInt("channels", channels);

    // Bind texture arrays
    glBindImageTexture(0, inputTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, residualTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(2, outputTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    int workGroupsX = (width + 15) / 16;
    int workGroupsY = (height + 15) / 16;
    glDispatchCompute(workGroupsX, workGroupsY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

bool ComputeStyleTransfer::getOrCreateTempTexture(int index, int width, int height, int channels, GLuint& outTexture, bool forceRegular2D) {
    // Ensure tempTextures vector is large enough
    if (index >= (int)tempTextures.size()) {
        tempTextures.resize(index + 1);
    }

    TempTexture& temp = tempTextures[index];

    // Calculate number of RGBA layers needed (ceil division)
    int numLayers = (channels + 3) / 4;

    // Recreate if size, channels, or TYPE changed
    if (temp.texture == 0 || temp.width != width || temp.height != height ||
        temp.channels != channels || temp.isRegular2D != forceRegular2D) {
        if (temp.texture) {
            glDeleteTextures(1, &temp.texture);
        }

        glGenTextures(1, &temp.texture);

        if (forceRegular2D) {
            // Regular 2D texture (for denormalize output, Lanczos3) - RGBA8 for final display
            glBindTexture(GL_TEXTURE_2D, temp.texture);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            // 2D texture array for multi-channel support - RGBA32F for neural network (supports negative values!)
            glBindTexture(GL_TEXTURE_2D_ARRAY, temp.texture);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, width, height, numLayers);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        temp.width = width;
        temp.height = height;
        temp.channels = channels;
        temp.isRegular2D = forceRegular2D;
    }

    outTexture = temp.texture;
    return true;
}

void ComputeStyleTransfer::uploadWeightToSSBO(const std::string& name,
                                               const std::vector<float>& data,
                                               const std::vector<int64_t>& dims) {
    std::cerr << "[ComputeStyleTransfer] Uploading " << name << " ("
              << data.size() << " floats, " << (data.size() * 4 / 1024.0 / 1024.0) << " MB)" << std::endl;

    // Create SSBO
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data.size() * sizeof(float),
                 data.data(),
                 GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Parse layer name and parameter type
    std::string layerName, paramType;
    size_t dotPos = name.rfind('.');
    if (dotPos != std::string::npos) {
        layerName = name.substr(0, dotPos);
        paramType = name.substr(dotPos + 1);
    }

    // Map to appropriate structure
    // Check if this is an instance norm layer (starts with "in" or contains ".in")
    bool isInstanceNorm = (layerName.find(".in") != std::string::npos) ||
                          (layerName.substr(0, 2) == "in" && layerName.length() >= 2 &&
                           layerName[2] >= '1' && layerName[2] <= '9');

    if (paramType == "weight") {
        if (isInstanceNorm) {
            // Instance norm gamma parameter
            InstanceNormParams& inp = instanceNormParams[layerName];
            inp.gammaBuffer = buffer;
            inp.channels = dims[0];
            std::cerr << "  InstanceNorm gamma: " << dims[0] << " channels" << std::endl;
        } else {
            // Convolution weight
            LayerWeights& lw = layerWeights[layerName];
            lw.weightBuffer = buffer;

            if (dims.size() == 4) {
                // Conv2D: [out_channels, in_channels, kernel_h, kernel_w]
                lw.outChannels = dims[0];
                lw.inChannels = dims[1];
                lw.kernelSize = dims[2];  // Assume square kernel

                // Set stride based on layer type
                // Only specific encoder layers and decoder upsampling layers use stride=2
                if (layerName == "conv2.conv2d" || layerName == "conv3.conv2d") {
                    lw.stride = 2;  // Encoder downsampling layers
                } else if (layerName == "deconv1.conv2d" || layerName == "deconv2.conv2d") {
                    lw.stride = 2;  // Decoder upsampling layers (transposed conv)
                } else {
                    lw.stride = 1;  // All other layers: conv1, residual blocks, deconv3
                }

                lw.padding = lw.kernelSize / 2;  // Default reflection padding

                std::cerr << "  Conv2D weight: out=" << lw.outChannels
                          << " in=" << lw.inChannels
                          << " kernel=" << lw.kernelSize << "x" << lw.kernelSize
                          << " stride=" << lw.stride << std::endl;

                // DEBUG: Print some sample weights to verify they're non-zero
                if (layerName == "conv1.conv2d" && data.size() >= 10) {
                    std::cerr << "  [DEBUG] First 10 weights: ";
                    for (int i = 0; i < 10; i++) {
                        std::cerr << data[i] << " ";
                    }
                    std::cerr << std::endl;
                }
            }
        }
    } else if (paramType == "bias") {
        if (isInstanceNorm) {
            // Instance norm beta parameter
            InstanceNormParams& inp = instanceNormParams[layerName];
            inp.betaBuffer = buffer;
            inp.channels = dims[0];
            std::cerr << "  InstanceNorm beta: " << dims[0] << " channels" << std::endl;
        } else {
            // Convolution bias
            LayerWeights& lw = layerWeights[layerName];
            lw.biasBuffer = buffer;
            std::cerr << "  Conv bias: " << dims[0] << " elements" << std::endl;

            // DEBUG: Print some sample biases
            if (layerName == "conv1.conv2d" && data.size() >= 5) {
                std::cerr << "  [DEBUG] First 5 biases: ";
                for (int i = 0; i < 5; i++) {
                    std::cerr << data[i] << " ";
                }
                std::cerr << std::endl;
            }
        }
    } else if (paramType == "gamma") {
        InstanceNormParams& inp = instanceNormParams[layerName];
        inp.gammaBuffer = buffer;
        inp.channels = dims[0];
        std::cerr << "  InstanceNorm gamma: " << dims[0] << " channels" << std::endl;
    } else if (paramType == "beta") {
        InstanceNormParams& inp = instanceNormParams[layerName];
        inp.betaBuffer = buffer;
        inp.channels = dims[0];
        std::cerr << "  InstanceNorm beta: " << dims[0] << " channels" << std::endl;
    } else {
        std::cerr << "  Unknown parameter type: " << paramType << std::endl;
        glDeleteBuffers(1, &buffer);
    }
}

bool ComputeStyleTransfer::loadWeightsFromONNX(const std::string& modelPath) {
    std::cerr << "[ComputeStyleTransfer] Loading model weights: " << modelPath << std::endl;

    // Try to load pre-extracted .weights file first (no ONNX dependency)
    std::string weightsPath = modelPath;
    size_t dotPos = weightsPath.rfind(".onnx");
    if (dotPos != std::string::npos) {
        weightsPath.replace(dotPos, 5, ".weights");
    }

    if (loadWeightsFromBinary(weightsPath)) {
        std::cerr << "[ComputeStyleTransfer] Successfully loaded from binary: " << weightsPath << std::endl;
        return true;
    }

    // Try to auto-extract weights using Python script
    std::cerr << "[ComputeStyleTransfer] Binary weights not found, attempting auto-extraction..." << std::endl;

    // Build absolute path to script
    std::string scriptPath;
    #ifdef _WIN32
        scriptPath = "C:/Users/gertd/source/EWOCvj2/CLion/EWOCvj2-git/scripts/extract_weights.py";
    #else
        scriptPath = "/usr/share/EWOCvj2/scripts/extract_weights.py";
    #endif

    // Use 'py' on Windows (Python launcher), 'python3' on Unix
    #ifdef _WIN32
        std::string pythonCmd = "py";
    #else
        std::string pythonCmd = "python3";
    #endif

    // Build command: py/python3 scripts/extract_weights.py model.onnx model.weights
    std::string command = pythonCmd + " \"" + scriptPath + "\" \"" + modelPath + "\" \"" + weightsPath + "\"";
    std::cerr << "[ComputeStyleTransfer] Running: " << command << std::endl;

    int result = system(command.c_str());
    if (result == 0) {
        std::cerr << "[ComputeStyleTransfer] Auto-extraction successful, loading weights..." << std::endl;
        // Try loading the freshly extracted weights
        if (loadWeightsFromBinary(weightsPath)) {
            return true;
        }
    } else {
        std::cerr << "[ComputeStyleTransfer] Auto-extraction failed (exit code " << result << ")" << std::endl;
    }

    // Fall back to ONNX protobuf parsing if available
#if HAS_ONNX_PROTO
    std::cerr << "[ComputeStyleTransfer] Trying ONNX protobuf parsing..." << std::endl;

    try {
        // Parse ONNX protobuf
        onnx::ModelProto model;
        std::ifstream input(modelPath, std::ios::binary);
        if (!input.is_open()) {
            setError("Failed to open ONNX model file: " + modelPath);
            return false;
        }

        if (!model.ParseFromIstream(&input)) {
            setError("Failed to parse ONNX model protobuf");
            return false;
        }

        input.close();

        std::cerr << "[ComputeStyleTransfer] Successfully parsed ONNX model" << std::endl;

        // Extract initializers (weights, biases, gamma, beta)
        const auto& graph = model.graph();
        std::cerr << "[ComputeStyleTransfer] Found " << graph.initializer_size() << " initializers" << std::endl;

        for (int i = 0; i < graph.initializer_size(); i++) {
            const auto& initializer = graph.initializer(i);
            std::string name = initializer.name();

            // Get float data
            std::vector<float> weights;
            if (initializer.has_raw_data()) {
                const std::string& raw = initializer.raw_data();
                weights.resize(raw.size() / sizeof(float));
                std::memcpy(weights.data(), raw.data(), raw.size());
            } else {
                weights.assign(initializer.float_data().begin(),
                              initializer.float_data().end());
            }

            // Parse dimensions
            std::vector<int64_t> dims;
            for (int j = 0; j < initializer.dims_size(); j++) {
                dims.push_back(initializer.dims(j));
            }

            // Upload to GPU SSBO
            uploadWeightToSSBO(name, weights, dims);
        }

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Exception loading ONNX weights: ") + e.what());
        return false;
    }
#else
    setError("ONNX protobuf not available and no .weights file found.\n"
             "  Run: python scripts/extract_weights.py " + modelPath + " " + weightsPath);
    return false;
#endif
}

bool ComputeStyleTransfer::loadWeightsFromBinary(const std::string& weightsPath) {
    std::ifstream file(weightsPath, std::ios::binary);
    if (!file.is_open()) {
        return false;  // Silent failure - will try ONNX next
    }

    try {
        // Read header
        uint32_t magic, version, numTensors;
        file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&numTensors), sizeof(uint32_t));

        if (magic != 0x4F4E4E58) {  // "ONNX"
            setError("Invalid magic number in weights file");
            return false;
        }

        std::cerr << "[ComputeStyleTransfer] Loading " << numTensors << " tensors from binary file" << std::endl;

        for (uint32_t i = 0; i < numTensors; i++) {
            // Read name
            uint32_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(uint32_t));
            std::string name(nameLen, '\0');
            file.read(&name[0], nameLen);

            // Read dimensions
            uint32_t numDims;
            file.read(reinterpret_cast<char*>(&numDims), sizeof(uint32_t));
            std::vector<int64_t> dims(numDims);
            file.read(reinterpret_cast<char*>(dims.data()), numDims * sizeof(int64_t));

            // Read float data
            uint32_t numFloats;
            file.read(reinterpret_cast<char*>(&numFloats), sizeof(uint32_t));
            std::vector<float> data(numFloats);
            file.read(reinterpret_cast<char*>(data.data()), numFloats * sizeof(float));

            // Upload to GPU
            uploadWeightToSSBO(name, data, dims);
        }

        file.close();

        // Set stride values for known layers (ReCoNet architecture)
        if (layerWeights.count("conv2.conv2d")) layerWeights["conv2.conv2d"].stride = 2;
        if (layerWeights.count("conv3.conv2d")) layerWeights["conv3.conv2d"].stride = 2;

        return true;

    } catch (const std::exception& e) {
        setError(std::string("Exception loading binary weights: ") + e.what());
        return false;
    }
}

void ComputeStyleTransfer::setError(const std::string& error) {
    lastError = error;
    std::cerr << "[ComputeStyleTransfer] ERROR: " << error << std::endl;
}
