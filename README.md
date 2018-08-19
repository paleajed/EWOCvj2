# EWOCvj2
Modern open source video mixer application

Check out the website at http://www.ewocprojects.be/frontpage.html

# Documentation
http://www.ewocprojects.be/EWOCvj2.pdf

# Contributing
We are on the lookout for contributors of all shapes and sizes.
Read the CONTRIBUTING.md and CODE_OF_CONDUCT.md files.

# Installation

For Windows 64-bit there's a binary installer available at http://www.ewocprojects.be/download.html
Users that want to compile themselves or Linux users (cause there's no package builds yet)
follow these guidelines:

### WINDOWS
* install MSYS2 https://www.msys2.org/
* open MingW64 console (in C:/msys64)
* install build toolchain:    
  >> pacman -S mingw-w64-x86_64-toolchain
* install dependencies:
    openal  rtmidi  freeglut freetype2  sdl2  opengl32  glew32  libjpeg  libsnappy
    boost_system  boost_filesystem  shcore.lib  oleaut32  ole32  ws2_32  liblo
  search for each term by entering:
  >> pacman -S name-of-lib
  to see if there is an Mingw version (64bit!) of the package available
  some libs will be supplied with Windows
* one difficulty is the need for ffmpeg with snappy support compiled in
  Either search for a development build that includes snappy,
  or compile ffmpeg from source:
  - download ffmpeg source tarball
  >> tar -xvf name-of-tarball
  >> cd ffmpeg
  >> ./configure --enable-snappy --enable-shared
  >> make
  >> make install
* clone the git repository in some directory:
  >> git clone https://github.com/paleajed/EWOCvj2.git
* build executable:
  >> cd EWOCvj2/windows/debug for debugging build
  or >> cd EWOCvj2/windows/release for optimized build
  >> ./configure
  >> make
  Then copy executable EWOCvj2.exe, shader.vs, shader.fs and expressway.ttf in the 
  directory that will become a portable run environment for the application.
  Also copy these dlls from your system (most from C:/msys64/mwing64) to this directory also:
  avcodec-58.dll     libfreeglut.dll              libintl-8.dll        libwinpthread-1.dll
  avdevice-58.dll    libfreetype-6.dll            libjpeg-8.dll        OpenAL32.dll      swresample-3.dll
  avfilter-7.dll     glew32.dll                   libgcc_s_seh-1.dll   libpcre-1.dll     postproc-55.dll      
  avformat-58.dll    libboost_filesystem-mt.dll   libglib-2.0-0.dll    libpng16-16.dll   swscale-5.dll
  avutil-56.dll      libboost_system-mt.dll       libgraphite2.dll     librtmidi-3.dll   SDL2.dll             
  libharfbuzz-0.dll  libsnappy.dll                zlib1.dll
  libbz2-1.dll       libiconv-2.dll               libstdc++-6.dll
  You now have a portable Win64 installation of EWOCvj2.


  ### LINUX
  * be sure you have a full build environment, this does at least consist of gcc and autotools
    but most distributions have full build packages ready for installation
    their name? Google is your friend!
 * install dependencies:
    openal-soft  rtmidi  GL GLEW X11 glut freetype2  sdl2  libjpeg  libsnappy
    boost_system  boost_filesystem  liblo
    be sure to install the "devel" packages!
  * one difficulty is the need for ffmpeg with snappy support compiled in
    Either search for a development build that includes snappy,
    or compile ffmpeg from source:
    - download ffmpeg source tarball
    >> tar -xvf name-of-tarball
    >> cd ffmpeg
    >> ./configure --enable-snappy --enable-shared --prefix=/usr
    >> make
    >> make install
  * clone the git repository in some directory:
    >> git clone https://github.com/paleajed/EWOCvj2.git
   * build executable:
    >> cd EWOCvj2/linux/debug for debugging build
    or >> cd EWOCvj2/linux/release for optimized build
    >> ./configure
    >> make
    >> make install
    This last step puts everything in the right spot on your system,
    just enter ">> EWOCvj2" to start the application.
    
    
    
