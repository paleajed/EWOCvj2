# EWOCvj2
Modern open source video mixer application.
A stable version is being developed as we speak, taking, at least, some more weeks.

Check out the website at http://www.ewocprojects.be/frontpage.html

# Documentation
http://www.ewocprojects.be/EWOCvj2.pdf
Not up to date till stable version release.

# Contributing
We are on the lookout for contributors of all shapes and sizes.
Read the CONTRIBUTING.md and CODE_OF_CONDUCT.md files.

# Installation

The binary installer is not currently available any more.  It followed development with all the problems, bugs and crashes associated with this.  It was decided to take it off the internet until the realease of the first stable version.
Users that want to compile themselves can currently only do this on Windows systems.  There has been a switch from gcc to Visual Studio development.

## Compilation

### WINDOWS
* install Visual Studio (VS)(community version is free from https://visualstudio.microsoft.com/)
Check out the Git repo of the project with VS at https://github.com/paleajed/EWOCvj2.git
Now you can access the EWOCvj2 and EWOCvjInstaller VS projects.
Learn how to use Visual Studio.
* install Vcpkg, a library package manager for Visual Studio on:
https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019
* install dependencies using vcpkg:
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
  This list could be over- or undercomplete, I don't remember where to access the complete list at the moment.  If you are stuck, just ask away!
* 3 exceptions:
* the need for ffmpeg with snappy support compiled in
  Best is to download the ffmpeg distributions from the ffmpeg page:
  https://ffmpeg.zeranoe.com/builds/
  You need the "Shared"(for running the program) and "Dev"(for compiling the proram) distributions.
  Set the VS project->Properties->Linker settings to where you installed the "Dev" distro.
* the need for rtmidi latest version
Get from https://www.music.mcgill.ca/~gary/rtmidi/index.html#download
Compile the rtmidi libraries and link from EWOCvj2 VS project->Properties->Linker settings
* best to use the latest Boost libraries from:
https://www.boost.org/
And link from EWOCvj2 VS project->Properties->Linker settings
* build the EWOCvj2 project, you can now test the program
* build the EWOCvjInstaller project to make a binary installer
The Release or Debug(you'll need the "d" version of some dll's) folder must contain:
avcodec-58.dll
avdevice-58.dll
avfilter-7.dll
avformat-58.dll
avutil-56.dll
bz2.dll
DevIL.dll
expressway.ttf (the main font file)
freetype.dll
glew32.dll
jasper.dll
jpeg62.dll
lcms2.dll
libpng16.dll
lo.dll
lzma.dll
OpenAL32.dll
postproc-55.dll
SDL2.dll
shader.fs (the GLSL fragment shader)
shader.vs (the GLSL vertex shader)
snappy.dll
swresample-3.dll
swscale-5.dll
tiff.dll
zlib1.dll


### LINUX
  * sorry, but the current source tree doesn't compile on Linux
