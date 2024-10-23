# EWOCvj2
Modern open source video mixer application.  We are in beta now.
A stable version is being developed as we speak, taking, at least, some more weeks.

Check out the website at http://www.ewocprojects.be/frontpage.html

# Documentation
http://www.ewocprojects.com/EWOCvj2.pdf

Very old version: not up to date till it is reworked at the stable version release.

# Contributing
We are on the lookout for contributors of all shapes and sizes.
Read the CONTRIBUTING.md and CODE_OF_CONDUCT.md files.

# Installation

There is a Win64 binary beta installer available. 
It is also possible to compile yourself.  The Visual Studio code has been replaced by a MingW/CMake scheme that covers both Windows and Linux.  Linux compilation is working now too.

## Compilation

### WINDOWS and LINUX now both use the CMake system:
* All code is in the src directory.
* Edit the CMakeLists.txt file so everything points to the right directories.  The Linux install searches for libraries in the default libs locations.  For Windows, either get your libs from the msys2 package(using the pacman system) or put them in C:\source\lib.  You can edit directories that are searched in the CMakeLists.txt file.  You'll need the packages mentioned below.
* install dependencies:

  OpenAL32 (although audio support is at the moment been disabled in the source)
  
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
  
* 3 exceptions:
* the need for ffmpeg with snappy support compiled in
  For Windows, best is to download the ffmpeg full distributions from:
  https://www.gyan.dev/ffmpeg/builds/
  You need the ffmpeg-release-full-shared.7z file.  In the bin directory you find the dll files(for running the program: put them in your build directory), int the libs directory the .a files (for compiling the program: set the directory in CMakeLists.txt) and in the include directory the .h files (for compiling the program: set the directory in CMakeLists.txt).
  On Linux, you can compile ffmpeg yourself, with snappy support.
* the need for rtmidi 5.0.0/6.0.0/7.0.0
Get the source from https://www.music.mcgill.ca/~gary/rtmidi/index.html#download or use the MSYS2/Mingw64 lib

* Use CMake to build the program.  You can find the executable in the cmake-build-debug or cmake-build-release directories.

On Linux, if CMake doesn't do this automatically copy the following files to /usr/share/ewocvj2 (create dir first time):
* src/background.png
* src/lock.png
* src/shader.fs
* src/shader.vs

Also install the expressway.ttf font to your system.

For Windows you will need to have these files in your build directory to run the program:
* avcodec-58.dll
* avdevice-58.dll
* avfilter-7.dll
* avformat-58.dll
* avutil-56.dll
* background.png
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
* lock.png
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
