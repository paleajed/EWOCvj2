cmake_minimum_required(VERSION 3.17)
project(EWOCvj2)

if(APPLE)
    # MacFileDialogs.mm (native NSSavePanel/NSOpenPanel) is Objective-C++.
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Configure build type specific flags
if (MINGW OR UNIX)
    # Debug: No optimization, debug symbols
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

    # Release: Full optimization, no debug symbols
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

    # RelWithDebInfo: Moderate optimization + debug symbols (best for debugging optimized code)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

    # MinSizeRel: Optimize for size + debug symbols
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -g -DNDEBUG")

    # Display the flags being used for current build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Using Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Using Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(STATUS "Using RelWithDebInfo flags: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        message(STATUS "Using MinSizeRel flags: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()
endif()

if (UNIX)
    set(CMAKE_INSTALL_PREFIX /usr)
    # Add pthread for Unix builds
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif (UNIX)

ADD_DEFINITIONS(-D_AFXDLL -DNdebug)
SET(CMAKE_MFC_FLAG 2)

if (WIN32)
    enable_language(RC)
    set(WIN_RESOURCES EWOCvj2.rc)
endif()

if (APPLE)
    set(MACOS_BUNDLE_FLAG MACOSX_BUNDLE)
else()
    set(MACOS_BUNDLE_FLAG "")
endif()

add_executable(EWOCvj2 WIN32 ${MACOS_BUNDLE_FLAG}
        ${WIN_RESOURCES}
        bins.cpp
        bins.h
        box.h
        effect.h
        mixer.h
        loopstation.cpp
        loopstation.h
        mixer.cpp
        node.cpp
        node.h
        osc.cpp
        program.cpp
        program.h
        start.cpp
        tinyfiledialogs.c
        tinyfiledialogs.h
        window.h
        retarget.h
        BeatDetektor.cpp
        BeatDetektor.h
        FFGLHost.h
        FFGLHost.cpp
        FFGLTypes.h
        ISFLoader.h
        ISFLoader.cpp
        UniformCache.cpp
        UniformCache.h
        NDIManager.h
        NDIManager.cpp
        UPnPPortMapper.h
        UPnPPortMapper.cpp
        AIStyleTransfer.h
        AIStyleTransfer.cpp
        AIStyleEffect.cpp
        RealESRGANWrapper.h
        RealESRGANWrapper.cpp
        styleroom.h
        styleroom.cpp
        videogenroom.h
        videogenroom.cpp
        ReCoNetTrainer.h
        ReCoNetTrainer.cpp
        VideoUpscaler.h
        VideoUpscaler.cpp
        ComfyUIManager.h
        ComfyUIManager.cpp
        ComfyUIInstaller.h
        ComfyUIInstaller.cpp
        Camera3D.h
        CameraPathEditor.h
        CameraPathEditor.cpp
        RealESRGANInstaller.h
        RealESRGANInstaller.cpp
        ReCoNetInstaller.h
        ReCoNetInstaller.cpp
        VideoUpscalingInstaller.h
        VideoUpscalingInstaller.cpp
        InstallVerification.h
        InstallVerification.cpp
        VideoDatasetDownloader.h
        VideoDatasetDownloader.cpp
        ImageLoader.h
        ImageLoader.cpp
        SAMSegmentation.h
        SAMSegmentation.cpp
        SAMInstaller.h
        SAMInstaller.cpp
        segmentationroom.h
        segmentationroom.cpp
)

set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_STATIC_LIBS ON)

include_directories(.)

if (WIN32)
    include_directories(BEFORE SYSTEM C:/source/include)
    include_directories(C:/source/rtmidi-5.0.0/include)
    if (MINGW)
        set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        set(BUILD_SHARED_LIBS OFF)
        set(CMAKE_EXE_LINKER_FLAGS "-static")
        include_directories(C:/msys64/mingw64/include)
        include_directories(C:/msys64/mingw64/include/freetype2)
        set(LIB1_SEARCH_DIR "C:/source/lib")
        set(LIB2_SEARCH_DIR "C:/msys64/mingw64/lib")
        endif (MINGW)
endif (WIN32)
if (UNIX AND NOT APPLE)
    include_directories(/usr/include/libjpeg-turbo)
    include_directories(/opt/libjpeg-turbo/include)
    include_directories(/usr/include/freetype2)
    include_directories(/usr/include/ffmpeg)
    include_directories(/usr/include/rtmidi)
    include_directories(/usr/include/stb)
    include_directories(/usr/local/include/ndi/include)
    set(LIB_SEARCH_DIRS "/usr/lib;/usr/lib64;/usr/local/lib;/usr/local/lib64;/usr/local/include/ndi/lib/x86_64-linux-gnu")
endif (UNIX AND NOT APPLE)

# Include FFGL headers
set(FFGL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/ffgl)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFGL_ROOT_DIR}/source/lib/ffgl
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/relwithdebinfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/minsizerel)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
# NDI SDK Configuration
if (WIN32)
    # NDI 6 SDK path - adjust if installed elsewhere
    set(NDI_SDK_ROOT "C:/Program Files/NDI/NDI 6 SDK" CACHE PATH "NDI SDK root directory")
    if(NOT EXISTS ${NDI_SDK_ROOT})
        set(NDI_SDK_ROOT "C:/Program Files/NewTek/NDI 6 SDK" CACHE PATH "NDI SDK root directory")
    endif()

    if(EXISTS ${NDI_SDK_ROOT})
        message(STATUS "Found NDI SDK at: ${NDI_SDK_ROOT}")
        include_directories(${NDI_SDK_ROOT}/Include)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            link_directories(${NDI_SDK_ROOT}/Lib/x64)
            set(NDI_RUNTIME_DIR ${NDI_SDK_ROOT}/Bin/x64)
        else()
            link_directories(${NDI_SDK_ROOT}/Lib/x86)
            set(NDI_RUNTIME_DIR ${NDI_SDK_ROOT}/Bin/x86)
        endif()
        set(NDI_FOUND TRUE)
    else()
        message(WARNING "NDI SDK not found at: ${NDI_SDK_ROOT}")
        set(NDI_FOUND FALSE)
    endif()
endif (WIN32)

if (UNIX AND NOT APPLE)
    # NDI SDK for Linux
    set(NDI_SDK_ROOT "/usr/local/include/ndi" CACHE PATH "NDI SDK root directory")
    if(NOT EXISTS ${NDI_SDK_ROOT})
        set(NDI_SDK_ROOT "/opt/ndi/include" CACHE PATH "NDI SDK root directory")
    endif()

    if(EXISTS ${NDI_SDK_ROOT})
        message(STATUS "Found NDI SDK at: ${NDI_SDK_ROOT}")
        include_directories(${NDI_SDK_ROOT})
        set(NDI_FOUND TRUE)
    else()
        message(WARNING "NDI SDK not found at: ${NDI_SDK_ROOT}")
        set(NDI_FOUND FALSE)
    endif()
endif (UNIX AND NOT APPLE)

# Option: test ANGLE (OpenGL ES 3.1) on Windows via D3D11 backend.
# Usage: cmake -DUSE_ANGLE_WINDOWS=ON -DANGLE_ROOT=C:/path/to/angle ...
# ANGLE_ROOT must contain include/ (with GLES3/, GLES2/, EGL/) and lib/ (libGLESv2.lib, libEGL.lib).
# Place libGLESv2.dll, libEGL.dll, and libANGLE.dll next to the built .exe.
option(USE_ANGLE_WINDOWS "Build with ANGLE (OpenGL ES 3.1 via D3D11) on Windows for GLES testing" OFF)

if (NOT APPLE AND NOT USE_ANGLE_WINDOWS)
    find_package( OpenGL )
    target_link_libraries(EWOCvj2 PUBLIC ${OPENGL_LIBRARIES})
endif ()
if (WIN32)
    if (MINGW)
        find_library(AVCODEC_LIBRARY avcodec.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${AVCODEC_LIBRARY})
        find_library(AVDEVICE_LIBRARY avdevice.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${AVDEVICE_LIBRARY})
        find_library(AVFORMAT_LIBRARY avformat.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${AVFORMAT_LIBRARY})
        find_library(AVUTIL_LIBRARY avutil.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${AVUTIL_LIBRARY})
        find_library(AVFILTER_LIBRARY avfilter.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${AVFILTER_LIBRARY})
        find_library(SWSCALE_LIBRARY swscale.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SWSCALE_LIBRARY})
        find_library(SWRESAMPLE_LIBRARY swresample.dll PATHS ${LIB1_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SWRESAMPLE_LIBRARY})
        find_library(RTMIDI_LIBRARY rtmidi HINTS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC optimized ${RTMIDI_LIBRARY})
        find_library(WINMM_LIBRARY winmm PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        find_library(IMM32_LIBRARY imm32 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        # winmm/imm32 are linked AFTER both rtmidi variants below so GNU ld sees
        # them after the __imp_midi* references are introduced by librtmidi.
        find_library(CFGMGR32_LIBRARY cfgmgr32 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${CFGMGR32_LIBRARY})
        find_library(SETUPAPI_LIBRARY setupapi PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SETUPAPI_LIBRARY})
        find_library(VERSION_LIBRARY Version PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${VERSION_LIBRARY})
        find_library(SHCORE_LIBRARY shcore PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SHCORE_LIBRARY})
        find_library(WS2_LIBRARY ws2_32 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${WS2_LIBRARY})
        # WinHTTP for ComfyUI REST/WebSocket communication
        find_library(WINHTTP_LIBRARY winhttp PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${WINHTTP_LIBRARY})
        find_library(FREEGLUT_LIBRARY freeglut.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FREEGLUT_LIBRARY})
        find_library(FREETYPE_LIBRARY freetype.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FREETYPE_LIBRARY})
        if (USE_ANGLE_WINDOWS)
            # ANGLE replaces opengl32 + GLEW entirely.
            # Install via MSYS2: pacman -S mingw-w64-x86_64-angleproject
            # Default ANGLE_ROOT points at the MinGW64 prefix where pacman installs it.
            set(ANGLE_ROOT "C:/msys64/mingw64" CACHE PATH "ANGLE installation root for Windows (contains include/ and lib/)")
            # __glew_h__ is GLEW's own include guard. Defining it as a compile definition
            # prevents glew.h from being processed even when pulled in transitively via FFGL.h.
            # Without this, GLEW's "#define glGenRenderbuffers __glewGenRenderbuffers" conflicts
            # with the actual function declarations in GLES3/gl3.h.
            # GL_GLEXT_PROTOTYPES causes gl2ext.h to declare extension function prototypes
            # (without it only the PFNGL... typedefs are declared, not the functions themselves).
            add_compile_definitions(USE_GLES __glew_h__ GL_GLEXT_PROTOTYPES)
            include_directories(${ANGLE_ROOT}/include)
            # Use the .dll suffix trick: find_library("GLESv2.dll") resolves to libGLESv2.dll.a
            # This forces selection of the DLL import library over the static archive libGLESv2.a.
            # The static archive does not export __imp_glXxx symbols required by GL_APICALL=__declspec(dllimport).
            find_library(ANGLE_GLES_LIBRARY NAMES GLESv2.dll libGLESv2.dll PATHS ${ANGLE_ROOT}/lib NO_DEFAULT_PATH)
            find_library(ANGLE_EGL_LIBRARY  NAMES EGL.dll    libEGL.dll    PATHS ${ANGLE_ROOT}/lib NO_DEFAULT_PATH)
            if (ANGLE_GLES_LIBRARY AND ANGLE_EGL_LIBRARY)
                message(STATUS "ANGLE GLESv2 (Windows/MSYS2): ${ANGLE_GLES_LIBRARY}")
                message(STATUS "ANGLE EGL    (Windows/MSYS2): ${ANGLE_EGL_LIBRARY}")
                target_link_libraries(EWOCvj2 PUBLIC ${ANGLE_GLES_LIBRARY} ${ANGLE_EGL_LIBRARY})
            else()
                message(FATAL_ERROR "ANGLE libs not found at ANGLE_ROOT=${ANGLE_ROOT}/lib\n"
                    "Install via MSYS2:  pacman -S mingw-w64-x86_64-angleproject\n"
                    "Or set -DANGLE_ROOT=<path> if installed elsewhere.")
            endif()
        else()
            find_library(OPENGL_LIBRARY opengl32 PATHS ${LIB1_SEARCH_DIR} C:/msys64/mingw64/x86_64-w64-mingw32/lib)
            target_link_libraries(EWOCvj2 PUBLIC optimized ${OPENGL_LIBRARY})
            find_library(GLEW32_LIBRARY glew32.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
            target_link_libraries(EWOCvj2 PUBLIC ${GLEW32_LIBRARY})
        endif()
        find_library(LO_LIBRARY lo.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${LO_LIBRARY})
        find_library(OPENAL32_LIBRARY openal.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${OPENAL32_LIBRARY})
        find_library(SNDFILE_LIBRARY sndfile PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SNDFILE_LIBRARY})
        find_library(FFTW_LIBRARY fftw3 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FFTW_LIBRARY})
        # NOTE: migrated to SDL3 alongside macOS (see the SDL3 block under
        # APPLE below) but untested here — no Windows build environment
        # available to verify this link line or that SDL3.dll exists at
        # this path.
        find_library(SDL3_LIBRARY SDL3.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SDL3_LIBRARY})
        find_library(SNAPPY_LIBRARY snappy.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SNAPPY_LIBRARY})
        find_library(TJPEG_LIBRARY turbojpeg HINTS /opt/libjpeg-turbo/lib64 C:/source/lib C:/msys64/mingw64/lib)
        target_link_libraries(EWOCvj2 PUBLIC ${TJPEG_LIBRARY})
        find_library(RTMIDI_LIBRARY rtmidi PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC debug ${RTMIDI_LIBRARY})
        # winmm and imm32 come after both rtmidi variants so GNU ld resolves __imp_midi* references
        target_link_libraries(EWOCvj2 PUBLIC ${WINMM_LIBRARY} ${IMM32_LIBRARY})
        # Link miniupnpc DLL import library before iphlpapi (miniupnpc depends on iphlpapi)
        find_library(MINIUPNPC_LIBRARY miniupnpc.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        if(MINIUPNPC_LIBRARY)
            message(STATUS "Found miniupnpc library: ${MINIUPNPC_LIBRARY}")
            target_link_libraries(EWOCvj2 PUBLIC ${MINIUPNPC_LIBRARY})
        else()
            message(WARNING "miniupnpc library not found - UPnP port mapping will not be available")
        endif()
        find_library(IPHLPAPI_LIBRARY iphlpapi PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${IPHLPAPI_LIBRARY})
        if(NDI_FOUND)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(NDI_LIBRARY_PATH "${NDI_SDK_ROOT}/Lib/x64/Processing.NDI.Lib.x64.lib")
            else()
                set(NDI_LIBRARY_PATH "${NDI_SDK_ROOT}/Lib/x86/Processing.NDI.Lib.x86.lib")
            endif()

            # Check if the library file exists
            if(EXISTS ${NDI_LIBRARY_PATH})
                message(STATUS "Found NDI library: ${NDI_LIBRARY_PATH}")
                # Link directly to the .lib file - MinGW can handle MSVC .lib files
                target_link_libraries(EWOCvj2 PUBLIC ${NDI_LIBRARY_PATH})

                # Copy NDI DLLs to output directory
                file(GLOB NDI_DLLS "${NDI_RUNTIME_DIR}/*.dll")
                if(NDI_DLLS)
                    foreach(NDI_DLL ${NDI_DLLS})
                        get_filename_component(DLL_NAME ${NDI_DLL} NAME)
                        configure_file(${NDI_DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${DLL_NAME} COPYONLY)
                        configure_file(${NDI_DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${DLL_NAME} COPYONLY)
                        configure_file(${NDI_DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}/${DLL_NAME} COPYONLY)
                        configure_file(${NDI_DLL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}/${DLL_NAME} COPYONLY)
                    endforeach()
                    message(STATUS "NDI DLLs will be copied to output directory")
                else()
                    message(WARNING "No NDI DLLs found in ${NDI_RUNTIME_DIR}")
                endif()
            else()
                message(FATAL_ERROR "NDI library file not found at: ${NDI_LIBRARY_PATH}")
            endif()
        endif(NDI_FOUND)
    endif (MINGW)

    # Shaders are loaded as plain text at runtime (program.cpp's set_shader()/
    # set_shader_from_files()), from mainprogram->exedir - never compiled into the exe - so a
    # stale copy sitting next to a build's output silently keeps running the OLD shader even after
    # editing the source .vs/.fs and rebuilding (bit us directly: cmake-build-release's
    # pointcloud.vs was still the pre-fix version while cmake-build-debug-mingw's had already been
    # hand-copied, so which one was actually running depended on which build config CLion had
    # active - not on which shader file had actually been edited). $<TARGET_FILE_DIR:EWOCvj2>
    # resolves to the real output directory for whichever config is being built, so this keeps
    # every configuration's deployed copy in sync on every build, not just whichever one someone
    # remembered to copy by hand.
    add_custom_command(TARGET EWOCvj2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/shader.vs"
            "${CMAKE_CURRENT_SOURCE_DIR}/shader.fs"
            "${CMAKE_CURRENT_SOURCE_DIR}/boxshader.fs"
            "${CMAKE_CURRENT_SOURCE_DIR}/pointcloud.vs"
            "${CMAKE_CURRENT_SOURCE_DIR}/pointcloud.fs"
            "$<TARGET_FILE_DIR:EWOCvj2>/"
        COMMENT "Copying shader files next to the built exe"
    )

    # src/custom_nodes/ is meant to be deployed into ComfyUI's own custom_nodes/ the same way
    # ComfyUI-SAM3 is (see start.cpp's runtime copy step, "custom ComfyUI nodes... bundled into
    # Contents/Resources/custom_nodes") - but that copy step reads from mainprogram->resourcedir,
    # which is ONLY ever set on macOS (program.cpp's constructor sets it inside an #ifdef MACOS
    # block); on Windows resourcedir stays empty, so that step silently never fires and nothing
    # ever lands in ComfyUI's custom_nodes/ on this platform. Rather than plumb a Windows
    # resourcedir through just for this, copy straight to ComfyUI's actual install location on
    # every build - the same $ENV{ProgramData}/EWOCvj2/ComfyUI/ComfyUI path
    # mainprogram->programData resolves to at runtime. A no-op (silently skipped) if ComfyUI
    # hasn't been installed yet; deploys automatically once it has, on the very next build.
    add_custom_command(TARGET EWOCvj2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/custom_nodes/EWOCvj2-MogeMetricExport"
            "$ENV{ProgramData}/EWOCvj2/ComfyUI/ComfyUI/custom_nodes/EWOCvj2-MogeMetricExport"
        COMMENT "Deploying EWOCvj2-MogeMetricExport into ComfyUI's custom_nodes"
    )
endif (WIN32)

if (UNIX AND NOT APPLE)
    find_library(AVCODEC_LIBRARY avcodec PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${AVCODEC_LIBRARY})
    find_library(AVDEVICE_LIBRARY avdevice PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${AVDEVICE_LIBRARY})
    find_library(AVFORMAT_LIBRARY avformat PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${AVFORMAT_LIBRARY})
    find_library(AVUTIL_LIBRARY avutil PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${AVUTIL_LIBRARY})
    find_library(SWSCALE_LIBRARY swscale PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SWSCALE_LIBRARY})
    find_library(SWRESAMPLE_LIBRARY swresample PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SWRESAMPLE_LIBRARY})
    find_library(AVFILTER_LIBRARY avfilter PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${AVFILTER_LIBRARY})
    find_library(FREEGLUT_LIBRARY glut PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${FREEGLUT_LIBRARY})
    find_library(FREETYPE_LIBRARY freetype PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${FREETYPE_LIBRARY})
    find_library(GLEW32_LIBRARY GLEW PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${GLEW32_LIBRARY})
    find_library(LO_LIBRARY lo PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${LO_LIBRARY})
    find_library(OPENAL32_LIBRARY openal PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${OPENAL32_LIBRARY})
    find_library(SNDFILE_LIBRARY sndfile PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SNDFILE_LIBRARY})
    find_library(FFTW_LIBRARY fftw3 PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${FFTW_LIBRARY})
    # NOTE: migrated to SDL3 alongside macOS (see the SDL3 block under APPLE
    # below) but untested here — no Linux build environment available to
    # verify this link line or that libSDL3 is installed at this path.
    find_library(SDL3_LIBRARY SDL3 PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SDL3_LIBRARY})
    find_library(SNAPPY_LIBRARY snappy PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SNAPPY_LIBRARY})
    find_library(TJPEG_LIBRARY turbojpeg PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${TJPEG_LIBRARY})
    find_library(RTMIDI_LIBRARY rtmidi PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${RTMIDI_LIBRARY})
    find_library(ALSA_LIBRARY asound PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${ALSA_LIBRARY})
    find_library(MINIUPNPC_LIBRARY miniupnpc PATHS ${LIB_SEARCH_DIRS})
    if(MINIUPNPC_LIBRARY)
        message(STATUS "Found miniupnpc library: ${MINIUPNPC_LIBRARY}")
        target_link_libraries(EWOCvj2 PUBLIC ${MINIUPNPC_LIBRARY})
    else()
        message(WARNING "miniupnpc library not found - UPnP port mapping will not be available")
    endif()
    if(NDI_FOUND)
        find_library(NDI_LIBRARY ndi PATHS ${LIB_SEARCH_DIRS})
        target_link_libraries(EWOCvj2 PUBLIC ${NDI_LIBRARY})
    endif(NDI_FOUND)
    find_library(PTHREAD_LIBRARY pthread HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${PTHREAD_LIBRARY})
    find_library(X11_LIBRARY X11 HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${X11_LIBRARY})
    find_library(GL_LIBRARY GL HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${GL_LIBRARY})
    find_library(Z_LIBRARY z HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${Z_LIBRARY})
    # libcurl for ComfyUI REST/WebSocket communication
    find_library(CURL_LIBRARY curl PATHS ${LIB_SEARCH_DIRS})
    if(CURL_LIBRARY)
        message(STATUS "Found libcurl: ${CURL_LIBRARY}")
        target_link_libraries(EWOCvj2 PUBLIC ${CURL_LIBRARY})
    else()
        message(WARNING "libcurl not found - ComfyUI integration will not be available")
    endif()
endif (UNIX AND NOT APPLE)

# ONNX Runtime for AI Style Transfer
if (UNIX AND NOT APPLE)
    find_library(ONNXRUNTIME_LIBRARY onnxruntime PATHS ${LIB_SEARCH_DIRS} /usr/local/lib)
    if(ONNXRUNTIME_LIBRARY)
        message(STATUS "ONNX Runtime found: ${ONNXRUNTIME_LIBRARY}")
        # Headers live directly in /usr/local/include, not a subdirectory
        target_include_directories(EWOCvj2 PRIVATE /usr/local/include)
        target_link_libraries(EWOCvj2 PUBLIC ${ONNXRUNTIME_LIBRARY})

        # CUDA and TensorRT providers are loaded via dlopen at runtime —
        # no link-time dependency needed, but detect them so we can report status
        # and package them alongside the binary.
        find_library(ONNXRUNTIME_PROVIDERS_CUDA_LIBRARY
            NAMES onnxruntime_providers_cuda
            PATHS /usr/local/lib ${LIB_SEARCH_DIRS})
        find_library(ONNXRUNTIME_PROVIDERS_TENSORRT_LIBRARY
            NAMES onnxruntime_providers_tensorrt
            PATHS /usr/local/lib ${LIB_SEARCH_DIRS})
        find_library(ONNXRUNTIME_PROVIDERS_SHARED_LIBRARY
            NAMES onnxruntime_providers_shared
            PATHS /usr/local/lib ${LIB_SEARCH_DIRS})

        if(ONNXRUNTIME_PROVIDERS_CUDA_LIBRARY)
            message(STATUS "ONNX Runtime CUDA provider found: ${ONNXRUNTIME_PROVIDERS_CUDA_LIBRARY}")
            add_compile_definitions(ONNXRUNTIME_CUDA_PROVIDER_AVAILABLE)
        else()
            message(WARNING "ONNX Runtime CUDA provider not found - GPU inference unavailable. "
                "Install onnxruntime-linux-x64-gpu and copy libonnxruntime_providers_cuda.so "
                "to /usr/local/lib/")
        endif()
        if(ONNXRUNTIME_PROVIDERS_TENSORRT_LIBRARY)
            message(STATUS "ONNX Runtime TensorRT provider found: ${ONNXRUNTIME_PROVIDERS_TENSORRT_LIBRARY}")
            add_compile_definitions(ONNXRUNTIME_TENSORRT_PROVIDER_AVAILABLE)
        endif()
    else()
        message(WARNING "ONNX Runtime not found - AI style transfer will not be available")
    endif()
endif (UNIX AND NOT APPLE)

if (WIN32)
    if (MINGW)
        set(ONNXRUNTIME_ROOT "C:/source/onnxruntime" CACHE PATH "ONNX Runtime root directory")
        if(EXISTS ${ONNXRUNTIME_ROOT})
            message(STATUS "ONNX Runtime found at ${ONNXRUNTIME_ROOT}")
            include_directories(${ONNXRUNTIME_ROOT}/include)
            include_directories(${ONNXRUNTIME_ROOT}/include/onnxruntime)
            include_directories(${ONNXRUNTIME_ROOT}/include/onnxruntime/core/session)

            set(ONNXRUNTIME_LIB "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib")
            if(EXISTS ${ONNXRUNTIME_LIB})
                message(STATUS "ONNX Runtime library found: ${ONNXRUNTIME_LIB}")
                target_link_libraries(EWOCvj2 PUBLIC ${ONNXRUNTIME_LIB})
            else()
                message(WARNING "ONNX Runtime library not found at ${ONNXRUNTIME_LIB}")
            endif()
        else()
            message(WARNING "ONNX Runtime not found at ${ONNXRUNTIME_ROOT}")
        endif()
    endif(MINGW)
endif(WIN32)

# ONNX Protobuf for weight extraction (compute shader implementation)
if (WIN32)
    if (MINGW)
        # Find Protobuf for ONNX model parsing
        set(PROTOBUF_ROOT "C:/msys64/mingw64" CACHE PATH "Protobuf root directory")
        if(EXISTS ${PROTOBUF_ROOT})
            message(STATUS "Protobuf found at ${PROTOBUF_ROOT}")
            include_directories(${PROTOBUF_ROOT}/include)

            find_library(PROTOBUF_LIBRARY protobuf PATHS ${PROTOBUF_ROOT}/lib ${LIB2_SEARCH_DIR})
            if(PROTOBUF_LIBRARY)
                message(STATUS "Protobuf library found: ${PROTOBUF_LIBRARY}")
                target_link_libraries(EWOCvj2 PUBLIC ${PROTOBUF_LIBRARY})
            else()
                message(WARNING "Protobuf library not found - install with: pacman -S mingw-w64-x86_64-protobuf")
            endif()
        else()
            message(WARNING "Protobuf not found at ${PROTOBUF_ROOT}")
        endif()

        # ONNX protobuf library for model parsing
        set(ONNX_ROOT "C:/source/onnx" CACHE PATH "ONNX root directory")
        if(EXISTS ${ONNX_ROOT})
            message(STATUS "ONNX found at ${ONNX_ROOT}")
            include_directories(${ONNX_ROOT})
            include_directories(${ONNX_ROOT}/include)

            find_library(ONNX_LIBRARY onnx PATHS ${ONNX_ROOT}/lib)
            find_library(ONNX_PROTO_LIBRARY onnx_proto PATHS ${ONNX_ROOT}/lib)
            if(ONNX_LIBRARY AND ONNX_PROTO_LIBRARY)
                message(STATUS "ONNX libraries found")
                target_link_libraries(EWOCvj2 PUBLIC ${ONNX_LIBRARY} ${ONNX_PROTO_LIBRARY})
            else()
                message(WARNING "ONNX libraries not found - compute shader weight loading will be unavailable")
            endif()
        else()
            message(WARNING "ONNX not found at ${ONNX_ROOT}")
        endif()
    endif(MINGW)
endif(WIN32)

add_compile_definitions(RECONET_TRAINING_ENABLED)

# Real-ESRGAN library for AI Upscaling
if (WIN32)
    if (MINGW)
        set(REALESRGAN_SRC "C:/source/realesrgan-src/src" CACHE PATH "Real-ESRGAN source directory")
        set(NCNN_ROOT "C:/source/ncnn-mingw/build-mingw" CACHE PATH "ncnn build directory (MinGW build)")

        if(EXISTS ${REALESRGAN_SRC})
            message(STATUS "Real-ESRGAN source found at ${REALESRGAN_SRC}")
            target_include_directories(EWOCvj2 PRIVATE ${REALESRGAN_SRC})
            target_sources(EWOCvj2 PRIVATE ${REALESRGAN_SRC}/realesrgan.cpp)
            add_compile_definitions(REALESRGAN_ENABLED)
        else()
            message(WARNING "Real-ESRGAN source not found at ${REALESRGAN_SRC} - upscaling disabled")
        endif()

        if(EXISTS ${NCNN_ROOT})
            message(STATUS "ncnn (MinGW build) found at ${NCNN_ROOT}")
            # Include ncnn headers - build directory first (has generated headers like platform.h)
            target_include_directories(EWOCvj2 PRIVATE
                ${NCNN_ROOT}/src
                C:/source/ncnn-mingw/src
            )

            # Link to MinGW-built ncnn library
            set(NCNN_LIB "${NCNN_ROOT}/src/libncnn.dll.a")
            set(NCNN_DLL "${NCNN_ROOT}/src/libncnn.dll")

            if(EXISTS ${NCNN_LIB} AND EXISTS ${NCNN_DLL})
                message(STATUS "ncnn library found: ${NCNN_LIB}")
                message(STATUS "ncnn DLL found: ${NCNN_DLL}")

                # Link with MinGW-compatible import library
                target_link_libraries(EWOCvj2 PUBLIC ${NCNN_LIB})

                # Copy DLL to output directories (keep original name libncnn.dll)
                configure_file(${NCNN_DLL} ${CMAKE_BINARY_DIR}/libncnn.dll COPYONLY)
                configure_file(${NCNN_DLL} ${CMAKE_SOURCE_DIR}/../cmake-build-debug-mingw/libncnn.dll COPYONLY)
            else()
                message(WARNING "ncnn library or DLL not found. Expected:")
                message(WARNING "  Library: ${NCNN_LIB}")
                message(WARNING "  DLL: ${NCNN_DLL}")
                message(WARNING "Run C:/source/ncnn-mingw/build_ncnn_mingw.bat to build ncnn with MinGW")
            endif()
        else()
            message(WARNING "ncnn not found at ${NCNN_ROOT}")
            message(WARNING "Run C:/source/ncnn-mingw/build_ncnn_mingw.bat to build ncnn with MinGW")
        endif()

        # Link Vulkan for GPU acceleration
        set(VULKAN_SDK "C:/VulkanSDK/1.4.328.1" CACHE PATH "Vulkan SDK directory")

        if(EXISTS ${VULKAN_SDK})
            message(STATUS "Vulkan SDK found at ${VULKAN_SDK}")
            include_directories(${VULKAN_SDK}/Include)

            set(VULKAN_LIB "${VULKAN_SDK}/Lib/vulkan-1.lib")
            if(EXISTS ${VULKAN_LIB})
                message(STATUS "Vulkan library found: ${VULKAN_LIB}")
                target_link_libraries(EWOCvj2 PUBLIC ${VULKAN_LIB})
            else()
                message(WARNING "Vulkan library not found at ${VULKAN_LIB}")
            endif()
        else()
            message(WARNING "Vulkan SDK not found at ${VULKAN_SDK} - GPU acceleration will not be available")
        endif()
    endif(MINGW)
endif(WIN32)

if (UNIX AND NOT APPLE)
    # Real-ESRGAN and ncnn for Linux
    set(REALESRGAN_SRC "/usr/local/src/Real-ESRGAN-ncnn-vulkan/src" CACHE PATH "Real-ESRGAN source directory" FORCE)

    if(EXISTS ${REALESRGAN_SRC})
        message(STATUS "Real-ESRGAN source found at ${REALESRGAN_SRC}")
        include_directories(${REALESRGAN_SRC})
        target_sources(EWOCvj2 PRIVATE ${REALESRGAN_SRC}/realesrgan.cpp)
        add_compile_definitions(REALESRGAN_ENABLED)

        find_program(GLSLANGVALIDATOR_EXECUTABLE NAMES glslangValidator)
        if(GLSLANGVALIDATOR_EXECUTABLE)
            message(STATUS "glslangValidator found: ${GLSLANGVALIDATOR_EXECUTABLE}")
            set(REALESRGAN_SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/realesrgan_shaders)
            file(MAKE_DIRECTORY ${REALESRGAN_SHADER_OUT_DIR})
            target_include_directories(EWOCvj2 PRIVATE ${REALESRGAN_SHADER_OUT_DIR})

            set(REALESRGAN_SHADER_HEADERS "")
            foreach(SHADER realesrgan_preproc realesrgan_postproc realesrgan_preproc_tta realesrgan_postproc_tta)
                set(SHADER_FULL ${REALESRGAN_SRC}/${SHADER}.comp)

                foreach(VARIANT "" "_fp16s" "_int8s")
                    if(VARIANT STREQUAL "")
                        set(EXTRA_ARGS "")
                    elseif(VARIANT STREQUAL "_fp16s")
                        set(EXTRA_ARGS -DNCNN_fp16_storage=1)
                    else()
                        set(EXTRA_ARGS -DNCNN_fp16_storage=1 -DNCNN_int8_storage=1)
                    endif()
                    set(SPV_HEX ${REALESRGAN_SHADER_OUT_DIR}/${SHADER}${VARIANT}.spv.hex.h)
                    add_custom_command(OUTPUT ${SPV_HEX}
                        COMMAND ${GLSLANGVALIDATOR_EXECUTABLE} ${EXTRA_ARGS} -V -s -x -o ${SPV_HEX} ${SHADER_FULL}
                        DEPENDS ${SHADER_FULL}
                        COMMENT "Compiling SPIR-V shader ${SHADER}${VARIANT}"
                        VERBATIM)
                    list(APPEND REALESRGAN_SHADER_HEADERS ${SPV_HEX})
                endforeach()
            endforeach()

            add_custom_target(realesrgan-shaders DEPENDS ${REALESRGAN_SHADER_HEADERS})
            add_dependencies(EWOCvj2 realesrgan-shaders)
        else()
            message(WARNING "glslangValidator not found - install glslang-devel; Real-ESRGAN shaders cannot be compiled")
        endif()
    else()
        message(WARNING "Real-ESRGAN source not found at ${REALESRGAN_SRC} - upscaling disabled")
    endif()

    find_package(ncnn QUIET)
    if(ncnn_FOUND)
        message(STATUS "ncnn found")
        target_link_libraries(EWOCvj2 PUBLIC ncnn)
    else()
        find_library(NCNN_LIB ncnn PATHS ${LIB_SEARCH_DIRS})
        if(NCNN_LIB)
            message(STATUS "ncnn library found: ${NCNN_LIB}")
            target_include_directories(EWOCvj2 PRIVATE /usr/include/ncnn)
            target_link_libraries(EWOCvj2 PUBLIC ${NCNN_LIB})
        else()
            message(WARNING "ncnn library not found")
        endif()
    endif()

    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(EWOCvj2 PUBLIC ${Vulkan_LIBRARY})
    endif()
endif(UNIX AND NOT APPLE)



if (APPLE)
    # -----------------------------------------------------------------------
    # macOS: ANGLE (OpenGL ES → Metal) build
    # Install prerequisites via Homebrew on Apple Silicon:
    #   brew install ffmpeg sdl2 freetype libsndfile fftw rtmidi liblo snappy
    #                libjpeg-turbo curl miniupnpc boost
    # ...or via MacPorts (pass -DHOMEBREW_PREFIX=/opt/local to cmake):
    #   sudo port install ffmpeg libsdl2 freetype libsndfile fftw-3 rtmidi liblo
    #                      snappy libjpeg-turbo curl miniupnpc boost angle
    # ANGLE can also be built from source or provided as pre-built dylibs:
    #   https://github.com/google/angle  (set ANGLE_ROOT below if not under
    #   HOMEBREW_PREFIX or /opt/local)
    # -----------------------------------------------------------------------

    # .app bundle properties
    set_target_properties(EWOCvj2 PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "EWOCvj2"
        MACOSX_BUNDLE_BUNDLE_VERSION "2.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.ewocvj.ewocvj2"
    )

    # Bundle fonts/shaders/images into Contents/Resources so the app is
    # self-contained — Program::Program() resolves this directory at
    # runtime from the executable's own path into fontdir/resourcedir (see
    # program.cpp), and program.cpp's MACOS shader-loading branches plus
    # start.cpp's MACOS splash/background/lock loading look for these
    # files there (mirroring how LINUX uses appimagedir + "/usr/share/ewocvj2").
    # MoltenVK is a Vulkan ICD (driver) — discovered by the Vulkan loader via
    # a JSON manifest + dlopen() at runtime, never a Mach-O load command, so
    # dylibbundler (see EWOCvj2_portable below) can never find or bundle it.
    # Ship our own copy plus a manifest with a relative library_path (the
    # Vulkan loader spec resolves that against the manifest's own directory)
    # and point the loader at it via VK_ICD_FILENAMES, set at startup in
    # program.cpp — so RealESRGAN's Vulkan backend doesn't depend on the
    # end user having MacPorts/Homebrew's Vulkan stack installed at all.
    find_library(MOLTENVK_LIBRARY MoltenVK PATHS /opt/local/lib /opt/homebrew/lib)

    add_custom_command(TARGET EWOCvj2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../expressway.ttf"
            "${CMAKE_CURRENT_SOURCE_DIR}/shader.fs"
            "${CMAKE_CURRENT_SOURCE_DIR}/shader.vs"
            "${CMAKE_CURRENT_SOURCE_DIR}/boxshader.fs"
            "${CMAKE_CURRENT_SOURCE_DIR}/background.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/lock.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/splash.jpeg"
            "${CMAKE_CURRENT_SOURCE_DIR}/MAINICON.icns"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ISF"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/ISF"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/scripts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/workflows"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/workflows"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/custom_nodes"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/custom_nodes"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/models/styles"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/models/styles"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/vulkan"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/vulkan"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MOLTENVK_LIBRARY}"
            "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Resources/vulkan/libMoltenVK.dylib"
        COMMENT "Copying fonts/shaders/images into app bundle Resources"
    )

    # Fast path for every regular dev build: codesign so the freshly-linked
    # binary can actually launch. Dylib bundling (below) is deliberately NOT
    # run here — reprocessing 139 libraries plus a --deep re-sign on every
    # single build is far too slow for an edit/build/test loop. It only
    # needs to run when producing a build meant to leave this machine.
    #
    # Signed with a local self-signed identity ("Gert De Roost" in Keychain)
    # rather than ad-hoc ("-"): ad-hoc signatures are derived from a hash of
    # the binary's own content, so they change on every single rebuild —
    # macOS's TCC folder-permission grants and window-activation/key-window
    # identity tracking are keyed off the signature, so a CLion-launched dev
    # build looked like a brand-new, never-before-seen app on every run
    # (Finder-launched builds appeared unaffected only because that copy
    # wasn't being rebuilt constantly). A stable certificate identity keeps
    # the signature - and therefore macOS's notion of "this app" - the same
    # across rebuilds, so grants made once actually persist.
    add_custom_command(TARGET EWOCvj2 POST_BUILD
        COMMAND codesign --sign "Gert De Roost" --force "$<TARGET_BUNDLE_DIR:EWOCvj2>"
        COMMENT "Codesigning app bundle (Gert De Roost identity)"
    )

    # Opt-in target: bundle every third-party dylib (MacPorts, Homebrew, NDI
    # SDK, the manually-installed /usr/local ffmpeg) into Contents/Frameworks
    # and rewrite all load commands to @rpath/ — without this the app only
    # runs on a machine with the exact same libraries at the exact same
    # absolute paths as this dev machine. Run explicitly, only when you want
    # a portable/distributable build:
    #   cmake --build . --target EWOCvj2_portable
    # `-p @rpath/` (not the longer @executable_path/../Frameworks/) because
    # several MacPorts ffmpeg dylibs weren't built with enough Mach-O header
    # padding for the longer form and install_name_tool fails ("larger
    # updated load commands do not fit"). dylibbundler's own -p @rpath/
    # handling sets a literal, useless "@rpath/" LC_RPATH entry, so we
    # delete and re-add it correctly pointing at the bundle.
    #
    # libvulkan needs its own manual bundling pass below, separate from
    # dylibbundler's: ncnn (statically linked into EWOCvj2 — its own
    # Framework binary never appears as a runtime dependency) weak-links
    # the Vulkan loader (LC_LOAD_WEAK_DYLIB, so a machine without any Vulkan
    # loader installed can still launch and fall back to CPU), and
    # dylibbundler 1.0.5 silently skips weak dylib dependencies entirely —
    # verified: it never touches libvulkan even though `otool -L` lists it
    # like any other dependency. There's no CLI flag to opt weak dylibs back
    # in, so it's bundled explicitly here with the same cp + install_name_tool
    # steps dylibbundler itself uses for everything else, preserving the
    # weak link (install_name_tool -change only rewrites the path, not the
    # load command's weak/strong flag).
    #
    # The actual MoltenVK/Vulkan ICD *driver* underneath the loader is a
    # separate problem from the loader above: it's found via a JSON manifest
    # + dlopen() at runtime, never a Mach-O load command, so dylibbundler
    # can't see it either. That one's handled elsewhere (see the MoltenVK
    # bundling + VK_ICD_FILENAMES setup above/in program.cpp) rather than
    # here, since it doesn't go through install_name_tool rewriting at all.
    #
    # glib's own module system (used transitively via freetype's
    # harfbuzz/glib chain in some builds) would have the same dlopen()-plus-
    # manifest issue in principle, but isn't actually reachable here: this
    # build's freetype doesn't link harfbuzz at all (verified via otool -L),
    # so no glib reference exists anywhere in the bundled dependency closure.
    find_program(DYLIBBUNDLER_EXECUTABLE dylibbundler)
    if(DYLIBBUNDLER_EXECUTABLE)
        add_custom_target(EWOCvj2_portable
            COMMAND ${DYLIBBUNDLER_EXECUTABLE} -od -b
                -x "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/MacOS/EWOCvj2"
                -d "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Frameworks"
                -p "@rpath/"
                -s "/Library/NDI SDK for Apple/lib/macOS"
                -ns
            COMMAND install_name_tool -delete_rpath "@rpath/"
                "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/MacOS/EWOCvj2"
            COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks/"
                "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/MacOS/EWOCvj2"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "/opt/local/lib/libvulkan.1.dylib"
                "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Frameworks/libvulkan.1.dylib"
            COMMAND chmod +w "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Frameworks/libvulkan.1.dylib"
            COMMAND install_name_tool -id "@rpath/libvulkan.1.dylib"
                "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/Frameworks/libvulkan.1.dylib"
            COMMAND install_name_tool -change "/opt/local/lib/libvulkan.1.dylib" "@rpath/libvulkan.1.dylib"
                "$<TARGET_BUNDLE_CONTENT_DIR:EWOCvj2>/MacOS/EWOCvj2"
            COMMAND codesign --sign "Gert De Roost" --force --deep "$<TARGET_BUNDLE_DIR:EWOCvj2>"
            DEPENDS EWOCvj2
            COMMENT "Bundling third-party dylibs (incl. weak-linked libvulkan) into Contents/Frameworks and re-signing"
        )
    else()
        add_custom_target(EWOCvj2_portable
            COMMAND ${CMAKE_COMMAND} -E echo
                "dylibbundler not found (brew install dylibbundler) — cannot build a portable bundle."
            DEPENDS EWOCvj2
        )
    endif()

    # Release target: package the fully self-contained (EWOCvj2_portable)
    # .app into a distributable .dmg — a staging folder holding the .app
    # plus an /Applications symlink (so Finder's disk-image window is a
    # standard drag-to-install layout), compressed with hdiutil (ships with
    # every macOS, no extra tool to install unlike dylibbundler/create-dmg).
    #   cmake --build . --target EWOCvj2_dmg
    # Not notarized — recipients need a one-time right-click > Open (a local,
    # non-Apple-issued identity only satisfies Gatekeeper on machines that
    # already trust that certificate, same as ad-hoc would).
    # Real notarization needs an Apple Developer ID we don't have here.
    set(DMG_STAGING_DIR "${CMAKE_BINARY_DIR}/dmg_staging")
    add_custom_target(EWOCvj2_dmg
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DMG_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DMG_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:EWOCvj2>" "${DMG_STAGING_DIR}/EWOCvj2.app"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "/Applications" "${DMG_STAGING_DIR}/Applications"
        COMMAND ${CMAKE_COMMAND} -E rm -f
            "${CMAKE_BINARY_DIR}/EWOCvj2-$<TARGET_PROPERTY:EWOCvj2,MACOSX_BUNDLE_SHORT_VERSION_STRING>.dmg"
        COMMAND hdiutil create
            -volname "EWOCvj2 $<TARGET_PROPERTY:EWOCvj2,MACOSX_BUNDLE_SHORT_VERSION_STRING>"
            -srcfolder "${DMG_STAGING_DIR}"
            -format UDZO -imagekey zlib-level=9
            "${CMAKE_BINARY_DIR}/EWOCvj2-$<TARGET_PROPERTY:EWOCvj2,MACOSX_BUNDLE_SHORT_VERSION_STRING>.dmg"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DMG_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo
            "Created ${CMAKE_BINARY_DIR}/EWOCvj2-$<TARGET_PROPERTY:EWOCvj2,MACOSX_BUNDLE_SHORT_VERSION_STRING>.dmg"
        DEPENDS EWOCvj2_portable
        COMMENT "Packaging portable app bundle into a distributable .dmg"
    )

    # __glew_h__ is GLEW's include guard — prevents glew.h from conflicting
    # with GLES headers if pulled in transitively (e.g. via FFGL.h).
    # GL_GLEXT_PROTOTYPES causes gl2ext.h to declare extension function prototypes.
    add_compile_definitions(USE_GLES __glew_h__ GL_GLEXT_PROTOTYPES)

    # Native Cocoa file dialogs (see MacFileDialogs.h for why tinyfiledialogs'
    # osascript-based dialogs don't work with a screen-covering borderless
    # window). AppKit/Cocoa are already linked below for ANGLE.
    target_sources(EWOCvj2 PRIVATE MacFileDialogs.mm)
    # NSWindow.backingScaleFactor query (see MacWindowUtils.h for why
    # SDL_GL_GetDrawableSize() isn't reliable with ANGLE's EGL surface).
    target_sources(EWOCvj2 PRIVATE MacWindowUtils.mm)
    # Native macOS menu bar (File/Configure/Rooms/Help), mirroring the app's
    # own custom-drawn top-bar menus (see MacMenuBar.h).
    target_sources(EWOCvj2 PRIVATE MacMenuBar.mm)

    # MacPorts' boost port is 1.76, whose Asio headers assume std::result_of
    # exists (removed in C++20). Force the std::invoke_result-based path.
    add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT=1)

    # Homebrew prefix: /opt/homebrew on Apple Silicon, /usr/local on Intel
    set(HOMEBREW_PREFIX "/opt/homebrew" CACHE PATH "Homebrew prefix")
    # /usr/local is searched first: it's this project's manually-installed-override
    # location (e.g. a self-compiled ffmpeg built with Snappy support, since stock
    # Homebrew/MacPorts ffmpeg builds don't enable it). Previously ${HOMEBREW_PREFIX}
    # was searched first, so if a standard Homebrew ffmpeg was also present there
    # (pulled in as some other package's dependency), the compiler picked up ITS
    # avcodec.h while the linker fell through to the self-compiled /usr/local one
    # for the actual library (find_library only matched a .dylib under /usr/local,
    # not under ${HOMEBREW_PREFIX}) - headers and library from two different ffmpeg
    # builds, so compiled-in constants like AV_CODEC_ID_HAP silently didn't match
    # the linked library's actual enum layout, breaking all HAP format detection.
    set(MACOS_LIB_DIRS  "/usr/local/lib;${HOMEBREW_PREFIX}/lib")
    set(MACOS_INC_DIRS  "/usr/local/include;${HOMEBREW_PREFIX}/include")

    # /usr/local/include is also one of AppleClang's own implicit default
    # system include dirs (see `c++ -v -E -x c++ - </dev/null`), and CMake
    # silently strips any include_directories()/target_include_directories()
    # entry that string-matches CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES
    # exactly from the actual -I flags it emits - it assumes the compiler
    # already searches it there. That's still fine on its own (clang
    # checks implicit dirs after all -I, but /usr/local/include is the
    # FIRST implicit dir and nothing else providing libavcodec/avcodec.h
    # sits ahead of it - except /opt/local/include, added below for ANGLE,
    # which is a normal -I and therefore searched before ALL implicit
    # dirs, and also happens to ship MacPorts' own libavcodec/avcodec.h).
    # Net effect: the self-compiled /usr/local/include/libavcodec/avcodec.h
    # silently lost out to MacPorts' one. Neither reordering MACOS_INC_DIRS
    # nor re-adding /usr/local/include explicitly (via target_compile_options
    # or a trailing-slash string to dodge the implicit-dir string match)
    # fixes this, since /opt/local/include still comes first as a plain -I
    # either way. The actual fix is below: /opt/local/include is demoted to
    # -idirafter (searched only after implicit dirs) at its one addition
    # site, so /usr/local/include (implicit, first) wins for avcodec.h
    # while /opt/local/include is still searched (just later) for the
    # ANGLE/EGL headers nothing else provides.
    include_directories(${MACOS_INC_DIRS})
    include_directories(${HOMEBREW_PREFIX}/include/freetype2 /usr/local/include/freetype2 /opt/local/include/freetype2)
    # stb_image.h/stb_image_write.h — MacPorts' "stb" port installs these
    # under <prefix>/include/stb/, matching Linux's /usr/include/stb.
    include_directories(${HOMEBREW_PREFIX}/include/stb /usr/local/include/stb /opt/local/include/stb)

    # --- ANGLE ---
    # No single standard prefix: Homebrew installs under HOMEBREW_PREFIX,
    # MacPorts (port install angle) under /opt/local, and source builds
    # can land anywhere via ANGLE_ROOT. Search all of them.
    set(ANGLE_ROOT "" CACHE PATH "ANGLE installation root (contains include/ and lib/), if not under a standard Homebrew/MacPorts prefix")
    set(ANGLE_SEARCH_DIRS "${HOMEBREW_PREFIX}" "/opt/local" "/usr/local")
    if(ANGLE_ROOT)
        list(INSERT ANGLE_SEARCH_DIRS 0 "${ANGLE_ROOT}")
    endif()
    set(ANGLE_LIB_SEARCH_DIRS "")
    foreach(_dir ${ANGLE_SEARCH_DIRS})
        if(EXISTS "${_dir}/include")
            if("${_dir}" STREQUAL "/opt/local")
                # -idirafter, not include_directories(): see MACOS_INC_DIRS
                # comment above - /opt/local/include also ships MacPorts'
                # ffmpeg headers, which must not shadow the self-compiled
                # /usr/local ones. -idirafter is searched only after
                # clang's implicit default dirs (of which /usr/local/include
                # is the first), so this still finds EGL/GLES2 headers here
                # (nothing else provides them) without the avcodec.h clash.
                target_compile_options(EWOCvj2 PRIVATE -idirafter "${_dir}/include")
            else()
                include_directories("${_dir}/include")
            endif()
        endif()
        list(APPEND ANGLE_LIB_SEARCH_DIRS "${_dir}/lib")
    endforeach()
    find_library(ANGLE_GLES_LIBRARY NAMES GLESv2 PATHS ${ANGLE_LIB_SEARCH_DIRS} NO_DEFAULT_PATH)
    find_library(ANGLE_EGL_LIBRARY  NAMES EGL     PATHS ${ANGLE_LIB_SEARCH_DIRS} NO_DEFAULT_PATH)
    if(ANGLE_GLES_LIBRARY AND ANGLE_EGL_LIBRARY)
        message(STATUS "ANGLE GLESv2: ${ANGLE_GLES_LIBRARY}")
        message(STATUS "ANGLE EGL:    ${ANGLE_EGL_LIBRARY}")
        target_link_libraries(EWOCvj2 PUBLIC ${ANGLE_GLES_LIBRARY} ${ANGLE_EGL_LIBRARY})
    else()
        message(FATAL_ERROR "ANGLE (libGLESv2/libEGL) not found in: ${ANGLE_LIB_SEARCH_DIRS}\n"
            "Install via MacPorts ('sudo port install angle'), Homebrew, build from "
            "source, or set -DANGLE_ROOT=<install_prefix>.\n"
            "See: https://github.com/google/angle")
    endif()

    # macOS system frameworks required by ANGLE / Metal backend
    target_link_libraries(EWOCvj2 PUBLIC
        "-framework Metal"
        "-framework MetalKit"
        "-framework CoreGraphics"
        "-framework IOSurface"
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework AppKit"
    )

    # --- FFmpeg ---
    # NO_DEFAULT_PATH is required here: without it, find_library() also
    # searches CMake's default system paths (env PATH's bin/->lib/ siblings,
    # CMAKE_SYSTEM_PREFIX_PATH) BEFORE the explicit PATHS list below, and
    # since /opt/local/bin precedes /usr/local/bin in PATH on this machine,
    # MacPorts' avcodec always won regardless of MACOS_LIB_DIRS's order -
    # silently pairing headers from one ffmpeg build with the library from
    # another (see MACOS_LIB_DIRS comment above).
    foreach(FFLIB avcodec avdevice avformat avutil swscale swresample avfilter)
        find_library(${FFLIB}_LIBRARY ${FFLIB} PATHS ${MACOS_LIB_DIRS} NO_DEFAULT_PATH)
        if(${FFLIB}_LIBRARY)
            target_link_libraries(EWOCvj2 PUBLIC ${${FFLIB}_LIBRARY})
        else()
            message(WARNING "${FFLIB} not found — brew install ffmpeg")
        endif()
    endforeach()

    # --- SDL3 (migrated from SDL2 for better macOS fullscreen/Dock/menu-bar
    # behavior — see SDL_HINT_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY usage in
    # start.cpp) ---
    find_library(SDL3_LIBRARY SDL3 PATHS ${MACOS_LIB_DIRS})
    if(SDL3_LIBRARY)
        target_link_libraries(EWOCvj2 PUBLIC ${SDL3_LIBRARY})
    else()
        message(FATAL_ERROR "SDL3 not found — brew install sdl3, or on MacPorts: sudo port install SDL3")
    endif()

    # --- Freetype / libjpeg-turbo ---
    find_library(FREETYPE_LIBRARY freetype PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${FREETYPE_LIBRARY})
    find_library(TJPEG_LIBRARY turbojpeg PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${TJPEG_LIBRARY})

    # --- Audio ---
    find_library(OPENAL32_LIBRARY openal PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${OPENAL32_LIBRARY})
    find_library(SNDFILE_LIBRARY sndfile PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SNDFILE_LIBRARY})
    find_library(FFTW_LIBRARY fftw3 PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${FFTW_LIBRARY})

    # --- MIDI (rtmidi needs CoreMIDI/CoreAudio on macOS) ---
    # Both Homebrew and MacPorts install RtMidi.h under <prefix>/include/rtmidi/
    include_directories(${HOMEBREW_PREFIX}/include/rtmidi /usr/local/include/rtmidi /opt/local/include/rtmidi)
    find_library(RTMIDI_LIBRARY rtmidi PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${RTMIDI_LIBRARY})
    target_link_libraries(EWOCvj2 PUBLIC
        "-framework CoreMIDI"
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )

    # --- OSC ---
    find_library(LO_LIBRARY lo PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${LO_LIBRARY})

    # --- Compression ---
    find_library(SNAPPY_LIBRARY snappy PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SNAPPY_LIBRARY})
    find_library(Z_LIBRARY z)
    target_link_libraries(EWOCvj2 PUBLIC ${Z_LIBRARY})

    # --- Networking ---
    find_library(CURL_LIBRARY curl PATHS ${MACOS_LIB_DIRS})
    if(CURL_LIBRARY)
        target_link_libraries(EWOCvj2 PUBLIC ${CURL_LIBRARY})
    else()
        message(WARNING "curl not found — ComfyUI integration unavailable")
    endif()
    find_library(MINIUPNPC_LIBRARY miniupnpc PATHS ${MACOS_LIB_DIRS})
    if(MINIUPNPC_LIBRARY)
        target_link_libraries(EWOCvj2 PUBLIC ${MINIUPNPC_LIBRARY})
    endif()

    # --- ONNX Runtime (use CoreML provider for Apple Silicon) ---
    set(ONNXRUNTIME_ROOT "$ENV{HOME}/onnxruntime" CACHE PATH "ONNX Runtime root")
    # ONNXRUNTIME_ROOT is a CACHE var, so a stale value from a prior
    # configure (before this default existed) won't be replaced by the
    # set(...CACHE...) above — re-derive it with FORCE if it's not usable.
    if(NOT EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h"
       AND EXISTS "$ENV{HOME}/onnxruntime/include/onnxruntime_cxx_api.h")
        set(ONNXRUNTIME_ROOT "$ENV{HOME}/onnxruntime" CACHE PATH "ONNX Runtime root" FORCE)
    endif()
    find_library(ONNXRUNTIME_LIBRARY onnxruntime
        PATHS ${ONNXRUNTIME_ROOT}/lib ${MACOS_LIB_DIRS})
    if(ONNXRUNTIME_LIBRARY)
        message(STATUS "ONNX Runtime: ${ONNXRUNTIME_LIBRARY}")
        target_include_directories(EWOCvj2 PRIVATE ${ONNXRUNTIME_ROOT}/include)
        target_link_libraries(EWOCvj2 PUBLIC ${ONNXRUNTIME_LIBRARY})
    else()
        message(WARNING "ONNX Runtime not found — AI style transfer unavailable")
    endif()

    # --- Real-ESRGAN + ncnn (Vulkan via MoltenVK) ---
    # realesrgan.cpp unconditionally requires ncnn's Vulkan backend (it sets
    # net.opt.use_vulkan_compute = true and includes precompiled SPIR-V
    # shader headers) — there's no CPU-only path in the upstream source.
    # Setup:
    #   1. realesrgan.h/.cpp + the 4 .comp shaders from
    #      https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan (src/ dir),
    #      all in one directory -> set REALESRGAN_SRC to it.
    #   2. ncnn (built with Vulkan support) from
    #      https://github.com/Tencent/ncnn releases (the "*-macos-vulkan.zip"
    #      asset) -> ncnn.framework placed under NCNN_FRAMEWORK_DIR
    #      (defaults to ~/Library/Frameworks, no sudo required).
    #   3. Vulkan-over-Metal runtime + shader compiler:
    #        sudo port install glslang moltenvk vulkan-loader vulkan-headers
    set(REALESRGAN_SRC "$ENV{HOME}/realesrgan-src" CACHE PATH "Real-ESRGAN source directory (realesrgan.h/.cpp + .comp shaders)")
    # REALESRGAN_SRC is a CACHE var, so a stale empty value from a prior
    # configure (before this default existed) won't be replaced by the
    # set(...CACHE...) above — re-derive it with FORCE if it's not usable.
    if(NOT EXISTS "${REALESRGAN_SRC}" AND EXISTS "$ENV{HOME}/realesrgan-src")
        set(REALESRGAN_SRC "$ENV{HOME}/realesrgan-src" CACHE PATH "Real-ESRGAN source directory (realesrgan.h/.cpp + .comp shaders)" FORCE)
    endif()
    if(EXISTS "${REALESRGAN_SRC}")
        message(STATUS "Real-ESRGAN source found at ${REALESRGAN_SRC}")
        target_include_directories(EWOCvj2 PRIVATE ${REALESRGAN_SRC})
        target_sources(EWOCvj2 PRIVATE ${REALESRGAN_SRC}/realesrgan.cpp)
        add_compile_definitions(REALESRGAN_ENABLED)

        find_program(GLSLANGVALIDATOR_EXECUTABLE NAMES glslangValidator)
        if(GLSLANGVALIDATOR_EXECUTABLE)
            message(STATUS "glslangValidator found: ${GLSLANGVALIDATOR_EXECUTABLE}")
            set(REALESRGAN_SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/realesrgan_shaders)
            file(MAKE_DIRECTORY ${REALESRGAN_SHADER_OUT_DIR})
            target_include_directories(EWOCvj2 PRIVATE ${REALESRGAN_SHADER_OUT_DIR})

            set(REALESRGAN_SHADER_HEADERS "")
            foreach(SHADER realesrgan_preproc realesrgan_postproc realesrgan_preproc_tta realesrgan_postproc_tta)
                set(SHADER_FULL ${REALESRGAN_SRC}/${SHADER}.comp)

                foreach(VARIANT "" "_fp16s" "_int8s")
                    if(VARIANT STREQUAL "")
                        set(EXTRA_ARGS "")
                    elseif(VARIANT STREQUAL "_fp16s")
                        set(EXTRA_ARGS -DNCNN_fp16_storage=1)
                    else()
                        set(EXTRA_ARGS -DNCNN_fp16_storage=1 -DNCNN_int8_storage=1)
                    endif()
                    set(SPV_HEX ${REALESRGAN_SHADER_OUT_DIR}/${SHADER}${VARIANT}.spv.hex.h)
                    add_custom_command(OUTPUT ${SPV_HEX}
                        COMMAND ${GLSLANGVALIDATOR_EXECUTABLE} ${EXTRA_ARGS} -V -s -x -o ${SPV_HEX} ${SHADER_FULL}
                        DEPENDS ${SHADER_FULL}
                        COMMENT "Compiling SPIR-V shader ${SHADER}${VARIANT}"
                        VERBATIM)
                    list(APPEND REALESRGAN_SHADER_HEADERS ${SPV_HEX})
                endforeach()
            endforeach()

            add_custom_target(realesrgan-shaders DEPENDS ${REALESRGAN_SHADER_HEADERS})
            add_dependencies(EWOCvj2 realesrgan-shaders)
        else()
            message(WARNING "glslangValidator not found - install glslang (e.g. 'sudo port install glslang'); Real-ESRGAN shaders cannot be compiled")
        endif()
    else()
        message(WARNING "Real-ESRGAN source not found at ${REALESRGAN_SRC} - upscaling disabled")
    endif()

    find_package(ncnn QUIET)
    if(ncnn_FOUND)
        target_link_libraries(EWOCvj2 PUBLIC ncnn)
    else()
        set(NCNN_FRAMEWORK_DIR "$ENV{HOME}/Library/Frameworks" CACHE PATH "Directory containing ncnn.framework")
        find_library(NCNN_LIB ncnn PATHS ${NCNN_FRAMEWORK_DIR} ${MACOS_LIB_DIRS})
        if(NCNN_LIB)
            message(STATUS "ncnn: ${NCNN_LIB}")
            target_include_directories(EWOCvj2 PRIVATE ${NCNN_FRAMEWORK_DIR}/ncnn.framework/Headers/ncnn ${HOMEBREW_PREFIX}/include/ncnn)
            target_link_libraries(EWOCvj2 PUBLIC ${NCNN_LIB})

            # ncnn's Vulkan backend needs glslang (runtime shader compilation)
            # and OpenMP, both shipped as separate frameworks in the same
            # Tencent/ncnn release archive as ncnn.framework itself — and the
            # Vulkan loader (MacPorts vulkan-loader) for vkGetInstanceProcAddr.
            find_library(GLSLANG_LIB glslang PATHS ${NCNN_FRAMEWORK_DIR})
            find_library(NCNN_OPENMP_LIB openmp PATHS ${NCNN_FRAMEWORK_DIR})
            find_library(VULKAN_LOADER_LIB vulkan PATHS ${MACOS_LIB_DIRS})
            if(GLSLANG_LIB)
                target_link_libraries(EWOCvj2 PUBLIC ${GLSLANG_LIB})
            else()
                message(WARNING "glslang.framework not found in ${NCNN_FRAMEWORK_DIR} — ncnn link will fail")
            endif()
            if(NCNN_OPENMP_LIB)
                target_link_libraries(EWOCvj2 PUBLIC ${NCNN_OPENMP_LIB})
            else()
                message(WARNING "openmp.framework not found in ${NCNN_FRAMEWORK_DIR} — ncnn link will fail")
            endif()
            if(VULKAN_LOADER_LIB)
                target_link_libraries(EWOCvj2 PUBLIC ${VULKAN_LOADER_LIB})
            else()
                message(WARNING "Vulkan loader not found — install via 'sudo port install vulkan-loader'; ncnn link will fail")
            endif()
        else()
            message(WARNING "ncnn not found — Real-ESRGAN upscaling unavailable")
        endif()
    endif()

    # --- NDI SDK for macOS ---
    # The NDI installer places the SDK at /Library (system-wide) by default;
    # some versions/installs use ~/Library instead. Check both. NDI_SDK_ROOT
    # is a CACHE var, so a stale value from a prior configure (e.g. before
    # this default changed) won't be replaced by a plain set(...CACHE...) —
    # explicitly re-derive it with FORCE whenever it doesn't point anywhere.
    if(NOT EXISTS "${NDI_SDK_ROOT}")
        if(EXISTS "/Library/NDI SDK for Apple")
            set(NDI_SDK_ROOT "/Library/NDI SDK for Apple" CACHE PATH "NDI SDK root (macOS)" FORCE)
        elseif(EXISTS "$ENV{HOME}/Library/NDI SDK for Apple")
            set(NDI_SDK_ROOT "$ENV{HOME}/Library/NDI SDK for Apple" CACHE PATH "NDI SDK root (macOS)" FORCE)
        else()
            set(NDI_SDK_ROOT "/Library/NDI SDK for Apple" CACHE PATH "NDI SDK root (macOS)")
        endif()
    endif()
    if(EXISTS "${NDI_SDK_ROOT}")
        message(STATUS "Found NDI SDK: ${NDI_SDK_ROOT}")
        include_directories("${NDI_SDK_ROOT}/include")
        find_library(NDI_LIBRARY ndi PATHS "${NDI_SDK_ROOT}/lib/macOS")
        if(NDI_LIBRARY)
            target_link_libraries(EWOCvj2 PUBLIC ${NDI_LIBRARY})
        endif()
    else()
        message(WARNING "NDI SDK not found at ${NDI_SDK_ROOT} — NDI I/O unavailable")
    endif()

    # --- pthread (macOS has it in libc, no separate link needed) ---
    # --- Boost (headers via MACOS_INC_DIRS; link thread/system explicitly) ---
    # MacPorts' boost libs carry a "-mt" suffix (libboost_thread-mt.dylib);
    # Homebrew's don't. Search both names.
    find_library(BOOST_THREAD_LIBRARY    NAMES boost_thread-mt    boost_thread    PATHS ${MACOS_LIB_DIRS})
    find_library(BOOST_SYSTEM_LIBRARY    NAMES boost_system-mt    boost_system    PATHS ${MACOS_LIB_DIRS})
    find_library(BOOST_DATETIME_LIBRARY  NAMES boost_date_time-mt boost_date_time PATHS ${MACOS_LIB_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_DATETIME_LIBRARY}
    )

endif(APPLE)

if (UNIX AND NOT APPLE)
    install(TARGETS EWOCvj2 RUNTIME DESTINATION bin)
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-bin.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-deck.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-layer.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-mix.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-project.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-shelf.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-state.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-style.png)")
    install(FILES ewocvj2-bin.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-deck.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-layer.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-mix.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-project.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-shelf.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-state.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-style.xml DESTINATION share/mime/application)
    install(CODE "execute_process(COMMAND update-mime-database /usr/share/mime)")
    install(DIRECTORY DESTINATION share/FFGL)
    install(DIRECTORY DESTINATION share/ISF)
    install(DIRECTORY DESTINATION share/ewocvj2)
    install(FILES shader.fs DESTINATION share/ewocvj2)
    install(FILES boxshader.fs DESTINATION share/ewocvj2)
    install(FILES shader.vs DESTINATION share/ewocvj2)
    install(FILES background.png DESTINATION share/ewocvj2)
    install(FILES splash.jpeg DESTINATION share/ewocvj2)
    install(FILES lock.png DESTINATION share/ewocvj2)
    install(FILES .F./expressway.ttf DESTINATION share/fonts/truetype/)
    install(FILES ../NotoSans-Regular.ttf DESTINATION share/fonts/truetype/)
    install(FILES ../NotoSansCJKsc-Regular.otf DESTINATION share/fonts/truetype/)
    install(DIRECTORY ISF DESTINATION share)
endif (UNIX AND NOT APPLE)

if (APPLE)
    install(TARGETS EWOCvj2 BUNDLE DESTINATION .)
endif (APPLE)
