cmake_minimum_required(VERSION 3.17)
project(EWOCvj2)

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

add_executable(EWOCvj2 WIN32
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
if (UNIX)
    include_directories(/usr/include/libjpeg-turbo)
    include_directories(/opt/libjpeg-turbo/include)
    include_directories(/usr/include/freetype2)
    include_directories(/usr/include/ffmpeg)
    include_directories(/usr/include/rtmidi)
    include_directories(/usr/local/include/ndi/include)
    set(LIB_SEARCH_DIRS "/usr/lib;/usr/lib64;/usr/local/lib;/usr/local/lib64;/usr/local/include/ndi/lib/x86_64-linux-gnu")
endif (UNIX)

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

if (UNIX)
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
endif (UNIX)

find_package( OpenGL )
target_link_libraries(EWOCvj2 PUBLIC ${OPENGL_LIBRARIES})
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
        target_link_libraries(EWOCvj2 PUBLIC ${WINMM_LIBRARY})
        find_library(IMM32_LIBRARY imm32 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${IMM32_LIBRARY})
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
        find_library(DEVIL_LIBRARY IL.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${DEVIL_LIBRARY})
        find_library(FREEGLUT_LIBRARY freeglut.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FREEGLUT_LIBRARY})
        find_library(FREETYPE_LIBRARY freetype.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FREETYPE_LIBRARY})
        find_library(OPENGL_LIBRARY opengl32 PATHS ${LIB1_SEARCH_DIR} C:/msys64/mingw64/x86_64-w64-mingw32/lib)
        target_link_libraries(EWOCvj2 PUBLIC optimized ${OPENGL_LIBRARY})
        find_library(GLEW32_LIBRARY glew32.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${GLEW32_LIBRARY})
        find_library(LO_LIBRARY lo.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${LO_LIBRARY})
        find_library(OPENAL32_LIBRARY openal.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${OPENAL32_LIBRARY})
        find_library(SNDFILE_LIBRARY sndfile PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SNDFILE_LIBRARY})
        find_library(FFTW_LIBRARY fftw3 PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${FFTW_LIBRARY})
        find_library(SDL2_LIBRARY SDL2.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SDL2_LIBRARY})
        find_library(SNAPPY_LIBRARY snappy.dll PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC ${SNAPPY_LIBRARY})
        find_library(TJPEG_LIBRARY turbojpeg HINTS /opt/libjpeg-turbo/lib64 C:/source/lib C:/msys64/mingw64/lib)
        target_link_libraries(EWOCvj2 PUBLIC ${TJPEG_LIBRARY})
        find_library(RTMIDI_LIBRARY rtmidi PATHS ${LIB1_SEARCH_DIR} ${LIB2_SEARCH_DIR})
        target_link_libraries(EWOCvj2 PUBLIC debug ${RTMIDI_LIBRARY})
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
endif (WIN32)

if (UNIX)
    find_library(BOOSTS_LIBRARY boost_system PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC optimized ${BOOSTS_LIBRARY})
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
    find_library(DEVIL_LIBRARY IL PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${DEVIL_LIBRARY})
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
    find_library(SDL2_LIBRARY SDL2 PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SDL2_LIBRARY})
    find_library(SNAPPY_LIBRARY snappy PATHS ${LIB_SEARCH_DIRS})
    target_link_libraries(EWOCvj2 PUBLIC ${SNAPPY_LIBRARY})
    find_library(TJPEG_LIBRARY turbojpeg HINTS /opt/libjpeg-turbo/lib64 ${LIB_SEARCH_DIRS})
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
endif (UNIX)

# ONNX Runtime for AI Style Transfer
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

# Real-ESRGAN library for AI Upscaling
if (WIN32)
    if (MINGW)
        set(REALESRGAN_SRC "C:/source/realesrgan-src/src" CACHE PATH "Real-ESRGAN source directory")
        set(NCNN_ROOT "C:/source/ncnn-mingw/build-mingw" CACHE PATH "ncnn build directory (MinGW build)")

        if(EXISTS ${REALESRGAN_SRC})
            message(STATUS "Real-ESRGAN source found at ${REALESRGAN_SRC}")
            target_include_directories(EWOCvj2 PRIVATE ${REALESRGAN_SRC})

            # Add Real-ESRGAN source files to build
            target_sources(EWOCvj2 PRIVATE ${REALESRGAN_SRC}/realesrgan.cpp)
        else()
            message(WARNING "Real-ESRGAN source not found at ${REALESRGAN_SRC}")
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

if (UNIX)
    # Real-ESRGAN and ncnn for Linux
    set(REALESRGAN_SRC "C:/source/realesrgan-src/src" CACHE PATH "Real-ESRGAN source directory")

    if(EXISTS ${REALESRGAN_SRC})
        message(STATUS "Real-ESRGAN source found at ${REALESRGAN_SRC}")
        include_directories(${REALESRGAN_SRC})
        target_sources(EWOCvj2 PRIVATE ${REALESRGAN_SRC}/realesrgan.cpp)
    endif()

    find_library(NCNN_LIB ncnn PATHS ${LIB_SEARCH_DIRS})
    if(NCNN_LIB)
        message(STATUS "ncnn library found: ${NCNN_LIB}")
        target_link_libraries(EWOCvj2 PUBLIC ${NCNN_LIB})
    else()
        message(WARNING "ncnn library not found")
    endif()

    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(EWOCvj2 PUBLIC ${Vulkan_LIBRARY})
    endif()
endif(UNIX)



if (UNIX)
    install(TARGETS EWOCvj2 RUNTIME DESTINATION bin)
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-bin.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-deck.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-layer.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-mix.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-project.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-shelf.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user ../application-ewocvj2-state.png)")
    install(FILES ewocvj2-bin.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-deck.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-layer.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-mix.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-project.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-shelf.xml DESTINATION share/mime/application)
    install(FILES ewocvj2-state.xml DESTINATION share/mime/application)
    install(CODE "execute_process(COMMAND update-mime-database /usr/share/mime)")
    install(DIRECTORY DESTINATION share/FFGL)
    install(DIRECTORY DESTINATION share/ISF)
    install(DIRECTORY DESTINATION share/ewocvj2)
    install(FILES shader.fs DESTINATION share/ewocvj2)
    install(FILES shader.vs DESTINATION share/ewocvj2)
    install(FILES background.png DESTINATION share/ewocvj2)
    install(FILES splash.jpeg DESTINATION share/ewocvj2)
    install(FILES lock.png DESTINATION share/ewocvj2)
    install(FILES ../expressway.ttf DESTINATION share/fonts/truetype/)
    install(DIRECTORY ISF DESTINATION share)
endif (UNIX)
