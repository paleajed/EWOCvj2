# EWOCvj2
Modern open source video mixer application.  We are in beta now.
A stable version is being developed as we speak, taking, at least, some more weeks.

Check out the website at http://www.ewocprojects.be/frontpage.html

If you want the bleeding edge development build for Windows, surf to the website above and download from there.  It often contains fixes for bugs that are still in the builds on the Releases page here on GitHub.  Downloading from the website is recommended.

# Documentation
http://www.ewocprojects.com/build

# Contributing
We are on the lookout for contributors of all shapes and sizes.
Read the CONTRIBUTING.md and CODE_OF_CONDUCT.md files.

# Installation

There are installers available on the Releases page. 
It is also possible to compile yourself.  The Visual Studio code has been replaced by a CMake scheme using MingW/MSYS for Windows.  Linux and OSX are working now too.

## Compilation

### WINDOWS and LINUX now both use the CMake system:
* After cloning the repository do (to install the ffgl submodule):
       git submodule update --init --recursive
* All code is in the src directory.
* Edit the CMakeLists.txt file so everything points to the right directories.  The Linux install searches for libraries in the default libs locations.  For Windows, either get your libs from the msys2 package(using the pacman system) or put them in C:\source\lib.  You can edit directories that are searched in the CMakeLists.txt file.
* You will need ffmpeg with snappy support compiled in (./configure -enable-shared --enable-libsnappy)

* Use CMake to build the program.  You can find the executable in the cmake-build-debug or cmake-build-release directories.

On Linux, if CMake doesn't do this automatically copy the following files to /usr/share/ewocvj2 (create dir first time):
* src/background.png
* src/lock.png
* src/shader.fs
* src/shader.vs
* src/boxshader.fs
* src/pointcloud.fs
* src/pointcloud.vs

Also install the expressway.ttf, NotoSans-Regular.ttf and NotoSansCJKsc-Regular.otf fonts to your system.

For Windows you will need to have these files in your build directory to run the program:
* avcodec-63.dll
* avdevice-63.dll
* avfilter-12.dll
* avformat-63.dll
* avutil-61.dll
* background.png
* boxshader.fs
* expressway.ttf
* libbrotlicommon.dll
* libbrotlidec.dll
* libbz2-1.dll
* libcrypto-3-x64.dll
* libcurl-4.dll
* libfreeglut.dll
* libfreetype-6.dll
* libgcc_s_seh-1.dll
* libglib-2.0-0.dll
* libgomp-1.dll
* libgraphite2.dll
* libharfbuzz-0.dll
* libiconv-2.dll
* libidn2-0.dll
* libIL.dll
* libintl-8.dll
* libjasper.dll
* libjpeg-8.dll
* liblcms2-2.dll
* liblo-7.dll
* liblzma-5.dll
* libminiupnpc.dll
* libncnn.dll
* libnghttp2-14.dll
* libnghttp3-9.dll
* libngtcp2_crypto_ossl-0.dll
* libngtcp2-16.dll
* libopenal-1.dll
* libpcre2-8-0.dll
* libpcre2-16-0.dll
* libpcre2-32-0.dll
* libpng16-16.dll
* libpsl-5.dll
* librtmidi-7.dll
* libsnappy.dll
* libsquish.dll
* libssh2-1.dll
* libssl-3-x64.dll
* libstdc++-6.dll
* libtiff-6.dll
* libturbojpeg.dll
* libunistring-5.dll
* libva.dll
* libva_win32.dll
* libwinpthread-1.dll
* libzstd.dll
* lock.png
* NotoSansCJKsc-Regular.otf
* NotoSans-Regular.ttf
* onnxruntime.dll
* onnxruntime_providers_cuda.dll
* onnxruntime_providers_shared.dll
* onnxruntime_providers_tensorrt.dll
* pointcloud.fs
* pointcloud.vs
* Processing.NDI.Lib.x64.dll
* SDL3.dll
* shader.fs
* shader.vs
* splash.jpeg
* swresample-7.dll
* swscale-10.dll
* zlib1.dll
