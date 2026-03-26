# EWOCvj2
Modern open source video mixer application.  We are in beta now.
A stable version is being developed as we speak, taking, at least, some more months.

Check out the website at http://www.ewocprojects.be/frontpage.html

If you want the bleeding edge development build for Windows, surf to the website above and download from there.  It often contains fixes for bugs that are still in the builds on the Releases page here on GitHub.  Downloading from the website is recommended.

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
* After cloning the repository do (to install the ffgl submodule):
       git submodule update --init --recursive
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





Optional Integration: HunyuanVideo

This application optionally supports video generation using Tencent HunyuanVideo models.

License Separation

This application is licensed under GPL-3.0-or-later.

HunyuanVideo models are NOT part of this project and are not distributed with it.

The HunyuanVideo model weights and related assets are licensed separately by Tencent under the Tencent Hunyuan Community License Agreement.

There is no license inheritance or linkage between this application and the HunyuanVideo models.
Model usage occurs only if the user manually downloads and configures them.

Territorial & Legal Notice

⚠️ Important for EU / UK / South Korea users

According to Tencent’s license terms, the Tencent Hunyuan Community License Agreement does not apply in the European Union, United Kingdom, or South Korea unless you have obtained a separate license from Tencent.

If you are located in one of these regions:

You are responsible for ensuring you have a valid legal right to download and use the HunyuanVideo models.

This project does not grant any rights to the models and does not provide legal authorization for their use.

User Responsibility

By enabling HunyuanVideo support, you acknowledge that:

You have reviewed and accepted Tencent’s applicable license terms.

You are legally permitted to use the models in your jurisdiction.

Any generated output and model usage comply with Tencent’s acceptable-use policies and local law.

No Warranty

This project:

Makes no guarantees regarding the legality, availability, or continued licensing of third-party AI models.

Is not affiliated with or endorsed by Tencent.

Alternatives

For users seeking fully open-source and GPL-compatible solutions, this application also supports (or plans to support) alternative video generation backends with OSI-approved licenses.
