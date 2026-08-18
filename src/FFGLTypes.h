#pragma once

// Local stand-in for the FFGL SDK header (external/ffgl, a git submodule)
// on macOS. The real SDK header unconditionally pulls in desktop
// <OpenGL/gl3.h>, which conflicts with the GLES headers our ANGLE-based
// macOS renderer uses. These types/constants are otherwise plain data
// (ints, floats, pointers) with no OpenGL dependency, so they're mirrored
// here verbatim (same names/fields as external/ffgl/source/lib/ffgl/FFGL.h)
// to let FFGLHost.cpp/.h compile unchanged.
//
// This does NOT provide a working FFGL plugin ABI: real third-party
// .bundle FFGL plugins are not supported on macOS. FF_TYPE_* and FFMixed
// remain in use here only because they're also the generic parameter-type
// vocabulary used throughout the app's own Param system (mixer.cpp,
// program.cpp, effect.h, styleroom.cpp, etc.), independent of plugin loading.

#include <cstdint>

typedef uint16_t FFUInt16;
typedef uint32_t FFUInt32;
typedef uint64_t FFUInt64;

// Function codes
static const FFUInt32 FF_GET_INFO                          = 0;
static const FFUInt32 FF_INITIALISE_V2                     = 34;
static const FFUInt32 FF_DEINITIALISE                      = 2;
static const FFUInt32 FF_GET_NUM_PARAMETERS                = 4;
static const FFUInt32 FF_GET_PARAMETER_NAME                = 5;
static const FFUInt32 FF_GET_PARAMETER_DEFAULT             = 6;
static const FFUInt32 FF_GET_PARAMETER_DISPLAY             = 7;
static const FFUInt32 FF_SET_PARAMETER                     = 8;
static const FFUInt32 FF_GET_PARAMETER                     = 9;
static const FFUInt32 FF_GET_PLUGIN_CAPS                   = 10;
static const FFUInt32 FF_ENABLE_PLUGIN_CAP                 = 49;
static const FFUInt32 FF_GET_EXTENDED_INFO                 = 13;
static const FFUInt32 FF_GET_PARAMETER_TYPE                = 15;
static const FFUInt32 FF_GET_INPUT_STATUS                  = 16;
static const FFUInt32 FF_PROCESS_OPENGL                    = 17;
static const FFUInt32 FF_INSTANTIATE_GL                    = 18;
static const FFUInt32 FF_DEINSTANTIATE_GL                  = 19;
static const FFUInt32 FF_SET_TIME                          = 20;
static const FFUInt32 FF_CONNECT                           = 21;
static const FFUInt32 FF_DISCONNECT                        = 22;
static const FFUInt32 FF_RESIZE                            = 23;
static const FFUInt32 FF_GET_NUM_PARAMETER_ELEMENTS        = 31;
static const FFUInt32 FF_GET_PARAMETER_ELEMENT_NAME        = 35;
static const FFUInt32 FF_GET_PARAMETER_ELEMENT_VALUE       = 36;
static const FFUInt32 FF_SET_PARAMETER_ELEMENT_VALUE       = 37;
static const FFUInt32 FF_GET_PARAMETER_USAGE               = 32;
static const FFUInt32 FF_GET_PLUGIN_SHORT_NAME             = 33;
static const FFUInt32 FF_SET_BEATINFO                      = 38;
static const FFUInt32 FF_SET_HOSTINFO                      = 39;
static const FFUInt32 FF_SET_SAMPLERATE                    = 40;
static const FFUInt32 FF_GET_RANGE                         = 41;
static const FFUInt32 FF_GET_PARAM_GROUP                   = 50;
static const FFUInt32 FF_GET_PARAM_DISPLAY_NAME            = 51;
static const FFUInt32 FF_GET_THUMBNAIL                     = 42;
static const FFUInt32 FF_GET_NUM_FILE_PARAMETER_EXTENSIONS = 43;
static const FFUInt32 FF_GET_FILE_PARAMETER_EXTENSION      = 44;
static const FFUInt32 FF_GET_PRAMETER_VISIBILITY           = 45;
static const FFUInt32 FF_GET_PARAMETER_EVENTS              = 46;
static const FFUInt32 FF_GET_NUM_ELEMENT_SEPARATORS        = 47;
static const FFUInt32 FF_GET_SEPARATOR_ELEMENT_INDEX       = 48;

// FreeFrame defines
enum : FFUInt32
{
    FF_SUCCESS = 0,
    FF_FAIL    = 0xFFFFFFFF
};
typedef FFUInt32 FFResult;

// Return values
static const FFUInt32 FF_TRUE        = 1;
static const FFUInt32 FF_FALSE       = 0;
static const FFUInt32 FF_SUPPORTED   = 1;
static const FFUInt32 FF_UNSUPPORTED = 0;

// Plugin types
static const FFUInt32 FF_EFFECT = 0;
static const FFUInt32 FF_SOURCE = 1;
static const FFUInt32 FF_MIXER  = 2;

// Plugin capabilities
static const FFUInt32 FF_CAP_SET_TIME                     = 5;
static const FFUInt32 FF_CAP_MINIMUM_INPUT_FRAMES         = 10;
static const FFUInt32 FF_CAP_MAXIMUM_INPUT_FRAMES         = 11;
static const FFUInt32 FF_CAP_TOP_LEFT_TEXTURE_ORIENTATION = 16;

// Parameter types
static const FFUInt32 FF_TYPE_BOOLEAN    = 0;
static const FFUInt32 FF_TYPE_EVENT      = 1;
static const FFUInt32 FF_TYPE_RED        = 2;
static const FFUInt32 FF_TYPE_GREEN      = 3;
static const FFUInt32 FF_TYPE_BLUE       = 4;
static const FFUInt32 FF_TYPE_XPOS       = 5;
static const FFUInt32 FF_TYPE_YPOS       = 6;
static const FFUInt32 FF_TYPE_STANDARD   = 10;
static const FFUInt32 FF_TYPE_OPTION     = 11;
static const FFUInt32 FF_TYPE_BUFFER     = 12;
static const FFUInt32 FF_TYPE_INTEGER    = 13;
static const FFUInt32 FF_TYPE_FILE       = 14;
static const FFUInt32 FF_TYPE_TEXT       = 100;
static const FFUInt32 FF_TYPE_HUE        = 200;
static const FFUInt32 FF_TYPE_SATURATION = 201;
static const FFUInt32 FF_TYPE_BRIGHTNESS = 202;
static const FFUInt32 FF_TYPE_ALPHA      = 203;

// Input status
static const FFUInt32 FF_INPUT_NOTINUSE = 0;
static const FFUInt32 FF_INPUT_INUSE    = 1;

// Parameter usages
static const FFUInt32 FF_USAGE_STANDARD = 0;
static const FFUInt32 FF_USAGE_FFT      = 1;

// Parameter events flags
static const FFUInt64 FF_EVENT_FLAG_VISIBILITY   = 0x01;
static const FFUInt64 FF_EVENT_FLAG_DISPLAY_NAME = 0x02;
static const FFUInt64 FF_EVENT_FLAG_VALUE        = 0x04;
static const FFUInt64 FF_EVENT_FLAG_ELEMENTS     = 0x08;

////////////////////////////////////////////////////////////////////////////
// FreeFrame Types
////////////////////////////////////////////////////////////////////////////

typedef union FFMixed
{
    FFUInt32 UIntValue;
    void* PointerValue;
} FFMixed;

typedef void* FFInstanceID;

typedef struct PluginInfoStructTag
{
    FFUInt32 APIMajorVersion;
    FFUInt32 APIMinorVersion;
    char PluginUniqueID[ 4 ];
    char PluginName[ 16 ];
    FFUInt32 PluginType;
} PluginInfoStruct;

typedef struct PluginExtendedInfoStructTag
{
    FFUInt32 PluginMajorVersion;
    FFUInt32 PluginMinorVersion;
    const char* Description;
    const char* About;
    FFUInt32 FreeFrameExtendedDataSize;
    void* FreeFrameExtendedDataBlock;
} PluginExtendedInfoStruct;

typedef struct SetParameterStructTag
{
    FFUInt32 ParameterNumber;
    FFMixed NewParameterValue;
} SetParameterStruct;

typedef struct SetBeatinfoStructTag
{
    float bpm;
    float barPhase;
} SetBeatinfoStruct;

typedef struct SetHostinfoStructTag
{
    const char* name;
    const char* version;
} SetHostinfoStruct;

typedef struct RangeStructTag
{
    float min;
    float max;
} RangeStruct;

typedef struct GetRangeStructTag
{
    FFUInt32 parameterNumber;
    RangeStruct range;
} GetRangeStruct;

typedef struct StringBufferStructTag
{
    char* address;
    FFUInt32 maxToWrite;
} StringBufferStruct;
typedef struct GetStringStructTag
{
    FFUInt32 parameterNumber;
    StringBufferStruct stringBuffer;
} GetStringStruct;

typedef struct GetThumbnailStructTag
{
    FFUInt32 width;
    FFUInt32 height;
    void* rgbaPixelBuffer;
} GetThumbnailStruct;

// FFGLViewportStruct/FFGLTextureStruct use plain unsigned int in place of
// GLuint — identical underlying type in both desktop GL and GLES, so no
// OpenGL header is needed to define these.
typedef struct FFGLViewportStructTag
{
    unsigned int x, y, width, height;
} FFGLViewportStruct;

typedef struct FFGLTextureStructTag
{
    FFUInt32 Width, Height;
    FFUInt32 HardwareWidth, HardwareHeight;
    unsigned int Handle;
} FFGLTextureStruct;

typedef struct ProcessOpenGLStructTag
{
    FFUInt32 numInputTextures;
    FFGLTextureStruct** inputTextures;
    unsigned int HostFBO;
} ProcessOpenGLStruct;

typedef struct GetParameterElementNameStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt32 ElementNumber;
} GetParameterElementNameStruct;

typedef struct GetParameterElementValueStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt32 ElementNumber;
} GetParameterElementValueStruct;

typedef struct SetParameterElementValueStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt32 ElementNumber;
    FFMixed NewParameterValue;
} SetParameterElementValueStruct;

typedef struct GetSeparatorElementIndexStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt32 SeparatorIndex;
} GetSeparatorElementIndexStruct;

typedef struct GetFileParameterExtensionStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt32 ExtensionNumber;
} GetFileParameterExtensionStruct;

typedef struct ParamEventStructTag
{
    FFUInt32 ParameterNumber;
    FFUInt64 eventFlags;
} ParamEventStruct;
typedef struct GetParamEventsStructTag
{
    FFUInt32 numEvents;
    ParamEventStruct* events;
} GetParamEventsStruct;

////////////////////////////////////////////////////////////////////////////
// Function prototypes (host-side typedefs only; no real plugin ABI on macOS)
////////////////////////////////////////////////////////////////////////////

typedef void ( *PFNLog )( char* cStr );
typedef FFMixed ( *FF_Main_FuncPtr )( FFUInt32, FFMixed, FFInstanceID );
typedef void ( *FF_SetLogCallback_FuncPtr )( PFNLog );
