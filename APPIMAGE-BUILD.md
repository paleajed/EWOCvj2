# EWOCvj2 AppImage Build Guide

This guide explains how to build a complete AppImage for EWOCvj2 with full ComfyUI integration and GPU acceleration.

## Quick Start

### Option 1: Full Installation (Recommended - includes CUDA/GPU support)

Install all dependencies first:
```bash
cd /home/gert/CLion/EWOCvj2-git
./install-dependencies.sh
```

This will install:
- ✅ ONNX Runtime with CUDA support (AI style transfer with GPU)
- ✅ ncnn library (neural network inference)
- ✅ Vulkan SDK (GPU compute)
- ✅ Real-ESRGAN (AI upscaling)

Then build the AppImage:
```bash
./build-appimage.sh
```

### Option 2: Minimal Build (CPU only, no AI features)

Skip dependency installation and build directly:
```bash
./build-appimage.sh
```

> **Note**: Without ONNX Runtime, ncnn, and Vulkan, AI style transfer and upscaling features will be disabled.

### Custom Version

To build with a custom version number:
```bash
VERSION=1.0.0 ./build-appimage.sh
```

## What's Included

The AppImage build script automatically packages:

### Core Application
- **EWOCvj2 executable**: Main VJing application binary
- **Shaders**: All vertex and fragment shaders (shader.vs, shader.fs, boxshader.fs, pointcloud shaders)
- **ISF Library**: Complete collection of 10,000+ Interactive Shader Format effects
- **Assets**: Backgrounds, splash screen, fonts, icons

### ComfyUI Integration
The AppImage includes complete ComfyUI integration with:

#### 1. Custom Nodes (2 packages)
- **ComfyUI-SAM3**: Segmentation and tracking node pack (314 files)
  - SAM3 server integration
  - Web UI components
  - Model inference capabilities

- **EWOCvj2-MogeMetricExport**: MoGe depth export node
  - Saves raw per-pixel metric depth
  - Camera Path Editor integration
  - Point cloud reconstruction support

#### 2. Workflows (25+ presets organized by backend)
- **flux2klein/**: Image generation workflows
  - `edit_image.json`
  - `text_to_image.json`

- **hunyuan/**: Video generation workflows
  - `text_to_video.json`
  - `image_to_motion.json`
  - `frame_interpolation.json`
  - `controlnet_canny.json`
  - `batch_variation.json` (i2v/t2v variants)
  - `video_continuation.json`
  - `upscale.json`
  - `remix_clip.json` (styled variant)

- **ltx_bf16/**, **ltx_gguf/**, **ltx_nvfp4/**: LTX-2.5 workflows
  - `text_to_video.json`
  - `image_to_video.json`
  - `camera_warp.json`
  - `character_retention.json`
  - `cutout_guides.json`
  - `first_frame_all_frames.json`
  - `flf2v.json` (First/Last Frame to Video)

- **sam/**: SAM3 segmentation workflows

#### 3. Scripts (14 Python files)
Training and utility scripts for AI model development:
- `train_reconet.py`, `train_reconet2.py`: ReCoNet training
- `upscale_edvr.py`, `upscale_flashvsr.py`: Video upscaling
- `build_video_dataset.py`: Dataset creation
- `loss_functions.py`, `loss_functions2.py`: Custom loss functions
- `reconet_model.py`, `reconet_model2.py`: Model definitions
- Testing utilities

#### 4. Styles (5 ONNX models, ~6.7 MB each)
Pre-trained neural style transfer models:
- `candy-9.onnx`
- `mosaic-9.onnx`
- `pointilism-9.onnx`
- `rain-princess-9.onnx`
- `udnie-9.onnx`

### Desktop Integration
- Desktop entry file with MIME type associations
- Application icon (256x256 PNG)
- MIME type icons for all file formats:
  - `.ewocvj2-project` - Main project files
  - `.ewocvj2-state` - Application state
  - `.ewocvj2-mix` - Mix configurations
  - `.ewocvj2-deck` - Deck setups
  - `.ewocvj2-layer` - Layer definitions
  - `.ewocvj2-shelf` - Shelf organization
  - `.ewocvj2-bin` - Binary data
  - `.ewocvj2-style` - Style presets

## Prerequisites

### Automated Installation (Recommended)

Use the provided script to install ALL dependencies including CUDA support:

```bash
./install-dependencies.sh
```

This script will:
1. Detect your system and CUDA installation
2. Install Vulkan SDK (latest version)
3. Build and install ncnn with Vulkan support
4. Install ONNX Runtime with CUDA support (if CUDA is available)
5. Setup Real-ESRGAN source for AI upscaling
6. Verify all installations

### Manual Installation

If you prefer to install dependencies manually:

#### Required (Core Dependencies)
- **CMake** (3.10+)
- **C++ compiler** with C++17 support (GCC 7+ or Clang 5+)
- **Make** or **Ninja**

#### System Libraries (Required)

**Graphics & Display:**
- OpenGL (libgl1-mesa-dev)
- GLEW (libglew-dev)
- FreeType2 (libfreetype6-dev)
- SDL3 (from source or distro package)

**Audio:**
- OpenAL (libopenal-dev)
- libsndfile (libsndfile1-dev)
- FFTW3 (libfftw3-dev)
- RtMidi (librtmidi-dev)
- liblo - OSC support (liblo-dev)

**Video/Image:**
- FFmpeg (libavcodec-dev, libavformat-dev, libavutil-dev, libswscale-dev, libswresample-dev, libavfilter-dev)
- libjpeg-turbo (libjpeg-turbo8-dev)
- libpng (libpng-dev)

**Compression:**
- Snappy (libsnappy-dev)
- zlib (zlib1g-dev)

**Networking:**
- libcurl (libcurl4-openssl-dev)
- miniupnpc (libminiupnpc-dev)

#### AI/ML Dependencies (Optional but Recommended)

**For GPU-Accelerated AI Features:**

1. **CUDA Toolkit** (12.x or 11.8+)
   - Download from: https://developer.nvidia.com/cuda-downloads
   - Required for ONNX Runtime CUDA support
   - Enables GPU-accelerated style transfer

2. **ONNX Runtime with CUDA** (1.20.1+)
   - Installed by `install-dependencies.sh`
   - Or download from: https://github.com/microsoft/onnxruntime/releases
   - Enables neural style transfer with 5 ONNX models

3. **Vulkan SDK** (1.3.x)
   - Installed by `install-dependencies.sh`
   - Or download from: https://vulkan.lunarg.com/
   - Required for GPU compute and Real-ESRGAN

4. **ncnn** (latest)
   - Installed by `install-dependencies.sh`
   - Or build from: https://github.com/Tencent/ncnn
   - Required for Real-ESRGAN upscaling

5. **Real-ESRGAN-ncnn-vulkan** (source)
   - Installed by `install-dependencies.sh`
   - Or clone from: https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan
   - Must be at: `/usr/local/src/Real-ESRGAN-ncnn-vulkan`

#### System
- pthread (included in glibc)
- X11 (libx11-dev)
- Boost (libboost-all-dev or specific: thread, system, date-time)

### Ubuntu/Debian Quick Install
```bash
sudo apt-get update
sudo apt-get install -y \
    cmake build-essential git \
    libgl1-mesa-dev libglew-dev libfreetype6-dev libvulkan-dev \
    libopenal-dev libsndfile1-dev libfftw3-dev librtmidi-dev liblo-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libavfilter-dev \
    libjpeg-turbo8-dev libpng-dev \
    libsnappy-dev zlib1g-dev \
    libcurl4-openssl-dev libminiupnpc-dev \
    libx11-dev libboost-all-dev \
    wget
```

### Special Dependencies

#### libjpeg-turbo (if installed in /opt)
If you have libjpeg-turbo installed in `/opt/libjpeg-turbo`, the AppRun wrapper will automatically add it to LD_LIBRARY_PATH.

#### NDI SDK (optional)
If you have the NDI SDK installed at `/usr/local/include/ndi`, the AppRun wrapper will automatically include it.

## Build Process

The build script performs these steps:

### 1. Dependency Check
Verifies cmake is installed and downloads linuxdeploy if needed.

### 2. Clean Build
Removes previous `build/` and `AppDir/` directories to ensure a fresh build.

### 3. CMake Configuration
Configures the project with:
- Build type: Release
- Install prefix: `/usr`
- All CMake options from your project

### 4. Compilation
Builds the project using all available CPU cores (`make -j$(nproc)`).

### 5. Installation to AppDir
Runs `make install DESTDIR=AppDir` to install:
- Executable to `AppDir/usr/bin/`
- Shared data to `AppDir/usr/share/`
- ISF shaders, fonts, assets

### 6. ComfyUI Integration Deployment
Copies all ComfyUI-related files to `AppDir/usr/share/ewocvj2/comfyui/`:
- `custom_nodes/` - Both node packages
- `workflows/` - All backend workflows
- `scripts/` - Training/utility scripts
- `models/styles/` - ONNX style models

### 7. Additional Assets
Ensures all icons, desktop files, and MIME type definitions are in place.

### 8. AppRun Wrapper Creation
Creates an intelligent wrapper that:
- Sets up `LD_LIBRARY_PATH` for all dependencies
- Detects libjpeg-turbo and NDI SDK locations
- On first run, copies ComfyUI files to user's home directory:
  - Custom nodes → `~/.local/share/EWOCvj2/ComfyUI/custom_nodes/`
  - Workflows → `~/.local/share/EWOCvj2/ComfyUI/workflows/`
  - Scripts → `~/.local/share/EWOCvj2/ComfyUI/scripts/`
  - Styles → `~/.local/share/EWOCvj2/ComfyUI/models/styles/`
- Only updates files if source is newer (efficient updates)
- Exports environment variables for the application

### 9. AppImage Creation
Uses linuxdeploy to:
- Bundle all dependencies
- Create a portable executable
- Package with desktop integration

## Running the AppImage

After building, you'll have:
- `EWOCvj2-0.98-beta-x86_64.AppImage` (or your custom version)
- `EWOCvj2-latest.AppImage` (symlink to the latest build)

To run:
```bash
./EWOCvj2-0.98-beta-x86_64.AppImage
```

Or simply:
```bash
./EWOCvj2-latest.AppImage
```

### First Run Behavior
On the first run, the AppImage will:
1. Create `~/.local/share/EWOCvj2/ComfyUI/` directory structure
2. Copy all custom nodes to the appropriate locations
3. Deploy workflows organized by backend
4. Install training scripts
5. Copy ONNX style models

This ensures your ComfyUI integration is ready to use immediately.

### Subsequent Runs
The AppRun wrapper intelligently checks if files need updating:
- If AppImage is newer than deployed files, they're updated automatically
- If files already exist and are current, nothing is copied (fast startup)

## Customization

### Change Version Number
```bash
VERSION=1.0.0-rc1 ./build-appimage.sh
```

### Build Directory
By default, builds in `./build`. The script automatically creates and manages this.

### AppDir Location
By default, creates AppDir in the project root. This is cleaned on each build.

### Adding New Libraries
If you need additional libraries in the LD_LIBRARY_PATH, edit the AppRun section in `build-appimage.sh`:

```bash
# Add your custom library path
export LD_LIBRARY_PATH="/path/to/your/libs:${LD_LIBRARY_PATH}"
```

## Troubleshooting

### "Missing dependencies" error
Install the required system packages listed in the Prerequisites section.

### "cmake: command not found"
```bash
sudo apt-get install cmake
```

### linuxdeploy download fails
Manually download from: https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

Place it in the project root and make it executable:
```bash
chmod +x linuxdeploy-x86_64.AppImage
```

### Library not found when running AppImage
Some libraries might not be bundled by linuxdeploy. Check the AppRun wrapper and add the path manually if needed.

### ComfyUI files not deploying
Verify the source directories exist:
```bash
ls -la src/custom_nodes
ls -la src/workflows
ls -la src/scripts
ls -la src/models/styles
```

If any are missing, the script will warn but continue building.

### AppImage too large
The AppImage includes 10,000+ ISF shader files and may be 500MB-1GB+. This is normal. To reduce size, you could:
- Remove unused ISF shaders before building
- Use external compression (upx on the executable before packaging)
- Split assets into a separate download

## Advanced: Manual Build

If you prefer to build manually without the script:

```bash
# 1. Create build directory
mkdir -p build && cd build

# 2. Configure
cmake ../src -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

# 3. Build
make -j$(nproc)

# 4. Install to AppDir
mkdir -p ../AppDir
make install DESTDIR=../AppDir

# 5. Deploy ComfyUI files manually
mkdir -p ../AppDir/usr/share/ewocvj2/comfyui
cp -r ../src/custom_nodes ../AppDir/usr/share/ewocvj2/comfyui/
cp -r ../src/workflows ../AppDir/usr/share/ewocvj2/comfyui/
cp -r ../src/scripts ../AppDir/usr/share/ewocvj2/comfyui/
cp -r ../src/models/styles ../AppDir/usr/share/ewocvj2/comfyui/models/

# 6. Create AppRun (see build-appimage.sh for full content)
# ... create AppRun script ...

# 7. Run linuxdeploy
cd ..
VERSION=0.98-beta ./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --executable=AppDir/usr/bin/EWOCvj2 \
    --desktop-file=AppDir/usr/share/applications/EWOCvj2.desktop \
    --icon-file=AppDir/usr/share/icons/hicolor/256x256/apps/EWOCvj2.png \
    --output=appimage
```

## File Locations Reference

### In AppImage (read-only)
- Executable: `/usr/bin/EWOCvj2`
- Shaders: `/usr/share/ewocvj2/shader.*`
- ISF: `/usr/share/ISF/`
- Fonts: `/usr/share/fonts/truetype/expressway.ttf`
- ComfyUI templates: `/usr/share/ewocvj2/comfyui/`

### User's Home Directory (writable)
- Custom nodes: `~/.local/share/EWOCvj2/ComfyUI/custom_nodes/`
- Workflows: `~/.local/share/EWOCvj2/ComfyUI/workflows/`
- Scripts: `~/.local/share/EWOCvj2/ComfyUI/scripts/`
- Styles: `~/.local/share/EWOCvj2/ComfyUI/models/styles/`

## Support

For issues with:
- **AppImage building**: Check this guide and the build script comments
- **EWOCvj2 application**: See main README.md or http://www.ewocprojects.be/
- **ComfyUI integration**: Check ComfyUIManager.h/cpp and related documentation

## License

Same as the main EWOCvj2 project (see LICENSE file in project root).
