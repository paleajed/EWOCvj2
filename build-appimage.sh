#!/bin/bash
# EWOCvj2 AppImage Build Script
# Creates a complete AppImage with ComfyUI integration, custom nodes, workflows, and all assets

set -e  # Exit on error

# Configuration
VERSION="${VERSION:-0.98-beta}"
BUILD_DIR="$(pwd)/build"
SRC_DIR="$(pwd)/src"
APPDIR="$(pwd)/AppDir"
INSTALL_PREFIX="/usr"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check for required tools
check_dependencies() {
    echo_info "Checking dependencies..."

    local missing_deps=()

    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi

    if [ ! -f "./linuxdeploy-x86_64.AppImage" ]; then
        echo_warn "linuxdeploy-x86_64.AppImage not found, will download..."
        wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
        chmod +x linuxdeploy-x86_64.AppImage
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo_error "Missing dependencies: ${missing_deps[*]}"
        echo_error "Please install them and try again."
        exit 1
    fi
}

# Clean previous build artifacts
clean_build() {
    echo_info "Cleaning previous build artifacts..."
    rm -rf "$BUILD_DIR"
    rm -rf "$APPDIR"
    mkdir -p "$BUILD_DIR"
}

# Build the project
build_project() {
    echo_info "Building EWOCvj2..."

    cd "$BUILD_DIR"

    # Set up environment for CUDA if available
    if [ -f "$(dirname "$0")/setup-cuda-env.sh" ]; then
        source "$(dirname "$0")/setup-cuda-env.sh"
        echo_info "CUDA environment loaded"
    fi

    # Configure CMake with library hints
    cmake "$SRC_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_PREFIX_PATH="/usr/local;/opt/libjpeg-turbo"

    # Build with parallel jobs
    make -j$(nproc)

    echo_info "Build completed successfully"
}

# Install to AppDir
install_to_appdir() {
    echo_info "Installing to AppDir..."

    cd "$BUILD_DIR"

    # Use cmake --install instead of make install (more reliable)
    cmake --install . --prefix "$APPDIR/usr"

    echo_info "Base installation completed"
}

# Deploy ComfyUI integration files
deploy_comfyui_integration() {
    echo_info "Deploying ComfyUI integration files..."

    # Create resource directories in AppDir
    # ComfyUI-specific resources (workflows, custom_nodes) go under comfyui subdirectory
    # General resources (scripts, styles) go directly under ewocvj2
    local comfyui_dir="$APPDIR/usr/share/ewocvj2/comfyui"
    local resource_dir="$APPDIR/usr/share/ewocvj2"
    mkdir -p "$comfyui_dir"/{custom_nodes,workflows}
    mkdir -p "$resource_dir"/{scripts,models/styles}

    # Deploy custom_nodes (ComfyUI-specific)
    if [ -d "$SRC_DIR/custom_nodes" ]; then
        echo_info "Copying custom_nodes..."
        cp -r "$SRC_DIR/custom_nodes"/* "$comfyui_dir/custom_nodes/"
        echo_info "  - ComfyUI-SAM3"
        echo_info "  - EWOCvj2-MogeMetricExport"
    else
        echo_warn "custom_nodes directory not found at $SRC_DIR/custom_nodes"
    fi

    # Deploy workflows (ComfyUI-specific)
    if [ -d "$SRC_DIR/workflows" ]; then
        echo_info "Copying workflows..."
        cp -r "$SRC_DIR/workflows"/* "$comfyui_dir/workflows/"

        # Count workflows by backend
        local workflow_count=$(find "$comfyui_dir/workflows" -name "*.json" | wc -l)
        echo_info "  - Deployed $workflow_count workflow files"

        for backend_dir in "$comfyui_dir/workflows"/*; do
            if [ -d "$backend_dir" ]; then
                local backend_name=$(basename "$backend_dir")
                local count=$(find "$backend_dir" -name "*.json" | wc -l)
                echo_info "    - $backend_name: $count workflows"
            fi
        done
    else
        echo_warn "workflows directory not found at $SRC_DIR/workflows"
    fi

    # Deploy scripts (general - used by ReCoNet, etc.)
    if [ -d "$SRC_DIR/scripts" ]; then
        echo_info "Copying scripts..."
        cp -r "$SRC_DIR/scripts"/* "$resource_dir/scripts/"
        local script_count=$(find "$resource_dir/scripts" -name "*.py" | wc -l)
        echo_info "  - Deployed $script_count Python scripts"
    else
        echo_warn "scripts directory not found at $SRC_DIR/scripts"
    fi

    # Deploy styles (general - ONNX models for AI style transfer)
    if [ -d "$SRC_DIR/models/styles" ]; then
        echo_info "Copying style models..."
        cp -r "$SRC_DIR/models/styles"/* "$resource_dir/models/styles/"
        local style_count=$(find "$resource_dir/models/styles" -name "*.onnx" | wc -l)
        echo_info "  - Deployed $style_count ONNX style models"

        # List the models
        for model in "$resource_dir/models/styles"/*.onnx; do
            if [ -f "$model" ]; then
                local model_name=$(basename "$model")
                local model_size=$(du -h "$model" | cut -f1)
                echo_info "    - $model_name ($model_size)"
            fi
        done
    else
        echo_warn "styles directory not found at $SRC_DIR/models/styles"
    fi

    echo_info "ComfyUI integration deployment completed"
}

# Deploy additional assets not handled by CMake install
deploy_additional_assets() {
    echo_info "Deploying additional assets..."

    # Copy icon files for MIME types
    local icon_dir="$APPDIR/usr/share/icons/hicolor/256x256"
    mkdir -p "$icon_dir/apps"
    mkdir -p "$icon_dir/mimetypes"

    # Copy application icon (check multiple locations)
    if [ -f "$SRC_DIR/cmake-build-debug/EWOCvj2.png" ]; then
        cp "$SRC_DIR/cmake-build-debug/EWOCvj2.png" "$icon_dir/apps/"
    elif [ -f "$BUILD_DIR/EWOCvj2.png" ]; then
        cp "$BUILD_DIR/EWOCvj2.png" "$icon_dir/apps/"
    elif [ -f "$SRC_DIR/EWOCvj2.png" ]; then
        cp "$SRC_DIR/EWOCvj2.png" "$icon_dir/apps/"
    else
        echo_warn "EWOCvj2.png not found in build directories"
    fi

    # Copy MIME type icons
    for mime_icon in "$SRC_DIR"/application-ewocvj2-*.png; do
        if [ -f "$mime_icon" ]; then
            cp "$mime_icon" "$icon_dir/mimetypes/"
        fi
    done

    # Ensure desktop file is in place (check multiple locations)
    mkdir -p "$APPDIR/usr/share/applications"
    if [ -f "$SRC_DIR/cmake-build-debug/EWOCvj2.desktop" ]; then
        cp "$SRC_DIR/cmake-build-debug/EWOCvj2.desktop" "$APPDIR/usr/share/applications/"
    elif [ -f "$BUILD_DIR/EWOCvj2.desktop" ]; then
        cp "$BUILD_DIR/EWOCvj2.desktop" "$APPDIR/usr/share/applications/"
    elif [ -f "$SRC_DIR/EWOCvj2.desktop" ]; then
        cp "$SRC_DIR/EWOCvj2.desktop" "$APPDIR/usr/share/applications/"
    fi

    # Copy and fix desktop file for AppImage
    if [ -f "$APPDIR/usr/share/applications/EWOCvj2.desktop" ]; then
        # Fix absolute paths in desktop file
        sed -i 's|Icon=.*|Icon=EWOCvj2|g' "$APPDIR/usr/share/applications/EWOCvj2.desktop"
        sed -i 's|Exec=.*|Exec=EWOCvj2|g' "$APPDIR/usr/share/applications/EWOCvj2.desktop"

        # Copy to AppDir root
        cp "$APPDIR/usr/share/applications/EWOCvj2.desktop" "$APPDIR/"
    fi

    echo_info "Additional assets deployed"
}

# Create AppRun wrapper script
create_apprun() {
    echo_info "Creating AppRun wrapper..."

    cat > "$APPDIR/AppRun" << 'APPRUN_EOF'
#!/bin/bash
# AppRun wrapper for EWOCvj2
# Sets up environment and launches the application

# Get the directory where this AppImage is mounted
HERE="$(dirname "$(readlink -f "${0}")")"

# Set up library paths
export LD_LIBRARY_PATH="${HERE}/usr/lib:${HERE}/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}"

# Add libjpeg-turbo if installed in non-standard location
if [ -d "/opt/libjpeg-turbo/lib64" ]; then
    export LD_LIBRARY_PATH="/opt/libjpeg-turbo/lib64:${LD_LIBRARY_PATH}"
fi

# Add NDI SDK if installed
if [ -d "/usr/local/include/ndi/lib/x86_64-linux-gnu" ]; then
    export LD_LIBRARY_PATH="/usr/local/include/ndi/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}"
fi

# Set up data directories for bundled resources
export EWOCVJ2_DATA_DIR="${HERE}/usr/share/ewocvj2"

# Set up XDG directories if not set
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Note: The C++ startup code (start.cpp) handles copying bundled resources
# (scripts, workflows, custom_nodes, styles) from the AppImage to the user's
# ~/.local/share directory. The resourcedir is set to EWOCVJ2_DATA_DIR.

# Launch the application
exec "${HERE}/usr/bin/EWOCvj2" "$@"
APPRUN_EOF

    chmod +x "$APPDIR/AppRun"
    echo_info "AppRun wrapper created"
}

# Create the AppImage
create_appimage() {
    echo_info "Creating AppImage..."

    cd "$(dirname "$APPDIR")"

    # Find icon file
    local icon_file=""
    if [ -f "$APPDIR/usr/share/icons/hicolor/256x256/apps/EWOCvj2.png" ]; then
        icon_file="$APPDIR/usr/share/icons/hicolor/256x256/apps/EWOCvj2.png"
    elif [ -f "$APPDIR/usr/share/icons/hicolor/256x256/apps/application-ewocvj2-project.png" ]; then
        icon_file="$APPDIR/usr/share/icons/hicolor/256x256/apps/application-ewocvj2-project.png"
    fi

    # Find desktop file
    local desktop_file=""
    if [ -f "$APPDIR/usr/share/applications/EWOCvj2.desktop" ]; then
        desktop_file="$APPDIR/usr/share/applications/EWOCvj2.desktop"
    fi

    # Set up library paths for linuxdeploy to find all dependencies
    export LD_LIBRARY_PATH="/usr/local/lib:/opt/libjpeg-turbo/lib64:/usr/local/include/ndi/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}"

    # Manually copy libraries from non-standard locations to AppDir
    mkdir -p "$APPDIR/usr/lib"

    # Copy libjpeg-turbo
    echo_info "Copying libjpeg-turbo libraries..."
    cp -L /opt/libjpeg-turbo/lib64/libturbojpeg.so.0* "$APPDIR/usr/lib/" 2>/dev/null || true
    cp -L /opt/libjpeg-turbo/lib64/libjpeg.so.62* "$APPDIR/usr/lib/" 2>/dev/null || true

    # Copy NDI SDK if available
    if [ -d "/usr/local/include/ndi/lib/x86_64-linux-gnu" ]; then
        echo_info "Copying NDI SDK libraries..."
        cp -L /usr/local/include/ndi/lib/x86_64-linux-gnu/libndi.so.* "$APPDIR/usr/lib/" 2>/dev/null || true
    fi

    # Download runtime if not present (for offline builds)
    if [ ! -f "runtime-x86_64" ]; then
        echo_info "Downloading AppImage runtime..."
        wget -q https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64
        chmod +x runtime-x86_64
    fi

    # Build linuxdeploy command with custom runtime
    export LINUXDEPLOY_OUTPUT_VERSION="${VERSION}"
    export LDAI_UPDATE_INFORMATION=""
    export LDAI_RUNTIME_FILE="$(pwd)/runtime-x86_64"

    # Exclude system libraries that should be provided by the host system
    # This list follows AppImage best practices for maximum compatibility
    export LINUXDEPLOY_EXCLUDE_LIST="libc.so.6,libstdc++.so.6,libgcc_s.so.1,libpthread.so.0,libm.so.6,libdl.so.2,librt.so.1,libresolv.so.2,libnss_*.so.*,libGL.so.1,libGLX.so.0,libGLdispatch.so.0,libOpenGL.so.0,libEGL.so.1,libX*.so.*,libxcb*.so.*,libdrm.so.2,libgbm.so.1,libwayland*.so.*"

    local linuxdeploy_cmd="LD_LIBRARY_PATH=/usr/local/lib:/opt/libjpeg-turbo/lib64:/usr/local/include/ndi/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH ./linuxdeploy-x86_64.AppImage"
    linuxdeploy_cmd+=" --appdir AppDir"
    linuxdeploy_cmd+=" --executable=AppDir/usr/bin/EWOCvj2"

    if [ -n "$desktop_file" ]; then
        linuxdeploy_cmd+=" --desktop-file=${desktop_file}"
    fi

    if [ -n "$icon_file" ]; then
        linuxdeploy_cmd+=" --icon-file=${icon_file}"
    fi

    linuxdeploy_cmd+=" --output=appimage"

    echo_warn "Note: This AppImage will require the host system to provide:"
    echo_warn "  - OpenGL/Mesa drivers"
    echo_warn "  - X11 libraries"
    echo_warn "  - GLIBC (version on your system: $(ldd --version | head -1))"

    echo_info "Running: $linuxdeploy_cmd"
    eval "$linuxdeploy_cmd"

    # Find the generated AppImage
    local appimage_file=$(ls -t EWOCvj2-${VERSION}-*.AppImage 2>/dev/null | head -1)

    if [ -f "$appimage_file" ]; then
        chmod +x "$appimage_file"
        echo_info "AppImage created successfully: $appimage_file"

        # Show file size
        local size=$(du -h "$appimage_file" | cut -f1)
        echo_info "AppImage size: $size"

        # Create a symlink to latest
        ln -sf "$appimage_file" EWOCvj2-latest.AppImage
        echo_info "Symlink created: EWOCvj2-latest.AppImage -> $appimage_file"

        return 0
    else
        echo_error "AppImage creation failed - no output file found"
        return 1
    fi
}

# Show summary
show_summary() {
    echo ""
    echo_info "========================================="
    echo_info "EWOCvj2 AppImage Build Summary"
    echo_info "========================================="
    echo_info "Version: $VERSION"
    echo_info ""
    echo_info "Included Components:"
    echo_info "  ✓ Main executable: EWOCvj2"
    echo_info "  ✓ ISF shaders: $(find "$APPDIR/usr/share/ISF" -type f 2>/dev/null | wc -l) files"
    echo_info "  ✓ Custom nodes: 2 packages (ComfyUI-SAM3, EWOCvj2-MogeMetricExport)"
    echo_info "  ✓ Workflows: Multiple backends (FLUX, Hunyuan, LTX variants)"
    echo_info "  ✓ Scripts: Python training/utility scripts"
    echo_info "  ✓ Styles: ONNX style transfer models"
    echo_info "  ✓ Desktop integration: MIME types and icons"
    echo_info ""
    echo_info "To run the AppImage:"
    echo_info "  ./EWOCvj2-${VERSION}-x86_64.AppImage"
    echo_info ""
    echo_info "Or use the latest symlink:"
    echo_info "  ./EWOCvj2-latest.AppImage"
    echo_info "========================================="
}

# Main execution
main() {
    echo_info "Starting EWOCvj2 AppImage build process..."
    echo_info "Version: $VERSION"
    echo ""

    check_dependencies
    clean_build
    build_project
    install_to_appdir
    deploy_comfyui_integration
    deploy_additional_assets
    create_apprun
    create_appimage
    show_summary

    echo ""
    echo_info "Build process completed successfully!"
}

# Run main function
main "$@"
