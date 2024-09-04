# EWOCvj2
Modern open source video mixer application.  We are in beta now.
A stable version is being developed as we speak, taking, at least, some more weeks.

Check out the website at http://www.ewocprojects.be/frontpage.html

# Documentation
http://www.ewocprojects.be/EWOCvj2.pdf

Very old version: not up to date till it is reworked at the stable version release.

# Contributing
We are on the lookout for contributors of all shapes and sizes.
Read the CONTRIBUTING.md and CODE_OF_CONDUCT.md files.

# Installation

The binary installer is not currently available any more.  It followed development with all the problems, bugs and crashes associated with this.  It was decided to take it off the internet until the release of the first stable version.
Users that want to compile themselves can currently only do this on Windows systems.  The Visual Studio code is replaced by a CMake scheme.  Linux compilation is working now too.

## Compilation

### WINDOWS and LINUX now both use the CMake system:
* All code is in the src directory.
* Edit the CMakeLists.txt file so everything points to the right directories.  You'll need the packages mentioned below.
* install dependencies:
  OpenAL32
  
  freeglut

  freetype2
  sdl2

  opengl32

  glew32

  libjpeg

  snappy
  ole32

  oleaut32

  ws2_32

  shcore

  comdlg32

  liblo

  devil

  boost system
*
* 3 exceptions:
* the need for ffmpeg 4 with snappy support compiled in
  For Windows, best is to download the ffmpeg distributions from the ffmpeg page:
  https://ffmpeg.zeranoe.com/builds/
  You need the "Shared"(for running the program) and "Dev"(for compiling the program) distributions.
  On Linux, you can compile ffmpeg 4 yourself, with snappy support.
* the need for rtmidi latest version
Get the source from https://www.music.mcgill.ca/~gary/rtmidi/index.html#download
Compile the rtmidi libraries
* best to use the latest Boost library from:
https://www.boost.org/

* Use CMake to build the program.  You can find the executable in the cmake-build-debug or cmake-build-release directories.

On Linux, copy src/vshader.vs and src/vshader.fs to /usr/share/ewocvj2 (create dir first time).  Also install the expressway.ttf font to your system.

For Windows you will need to have these dll's in your build directory to run the program. (you'll need the "d" version of some dll's) folder must contain
* avcodec-58.dll
* avdevice-58.dll
* avfilter-7.dll
* avformat-58.dll
* avutil-56.dll
* bz2.dll
* DevIL.dll
* expressway.ttf (the main font file)
* freetype.dll
* glew32.dll
* jasper.dll
* jpeg62.dll
* lcms2.dll
* libpng16.dll
* lo.dll
* lzma.dll
* OpenAL32.dll
* postproc-55.dll
* SDL2.dll
* shader.fs (the GLSL fragment shader)
* shader.vs (the GLSL vertex shader)
* snappy.dll
* swresample-3.dll
* swscale-5.dll
* tiff.dll
* zlib1.dll
