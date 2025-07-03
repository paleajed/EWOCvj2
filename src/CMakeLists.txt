cmake_minimum_required(VERSION 3.17)
project(EWOCvj2)

set(CMAKE_CXX_STANDARD 20)
if (UNIX)
    set(CMAKE_INSTALL_PREFIX /usr)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -pthread")
endif (UNIX)

ADD_DEFINITIONS(-D_AFXDLL -DNdebug)
SET(CMAKE_MFC_FLAG 2)

add_executable(EWOCvj2 WIN32
        bins.cpp
        bins.h
        box.h
        effect.h
        layer.h
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
    set(LIB_SEARCH_DIRS "/usr/lib;/usr/lib64;/usr/local/lib;/usr/local/lib64")
endif (UNIX)

# Include FFGL headers
set(FFGL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/ffgl)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFGL_ROOT_DIR}/source/lib/ffgl
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")

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
    find_library(PTHREAD_LIBRARY pthread HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${PTHREAD_LIBRARY})
    find_library(X11_LIBRARY X11 HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${X11_LIBRARY})
    find_library(GL_LIBRARY GL HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${GL_LIBRARY})
    find_library(Z_LIBRARY z HINTS)
    target_link_libraries(EWOCvj2 PUBLIC ${Z_LIBRARY})
endif (UNIX)


if (UNIX)
    install(TARGETS EWOCvj2 RUNTIME DESTINATION bin)
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-bin.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-deck.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-layer.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-mix.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-project.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-shelf.png)")
    install(CODE "execute_process(COMMAND xdg-icon-resource install --context mimetype --novendor --size 256 --mode user application-ewocvj2-state.png)")
    install(FILES ewocvj2-*.xml DESTINATION share/mime/application)
    install(CODE "execute_process(COMMAND update-mime-database /usr/share/mime)")
    install(DIRECTORY DESTINATION share/ewocvj2)
    install(FILES shader.* DESTINATION share/ewocvj2)
    install(FILES background.jpg DESTINATION share/ewocvj2)
    install(FILES splash.jpeg DESTINATION share/ewocvj2)
    install(FILES lock.png DESTINATION share/ewocvj2)
    install(FILES ../expressway.ttf DESTINATION share/fonts/truetype/)
endif (UNIX)
