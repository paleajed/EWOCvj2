#!/bin/bash
# EWOCvj2 Dependency Installation Script
# Installs ONNX Runtime (CUDA), ncnn, Vulkan, and Real-ESRGAN for full functionality

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo_step() {
    echo -e "\n${BLUE}==>${NC} $1\n"
}

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
TEMP_DIR="/tmp/ewocvj2-deps"
NCNN_VERSION="20240820"
ONNX_VERSION="1.20.1"

# Detect system
detect_system() {
    echo_info "Detecting system configuration..."

    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
        OS_VERSION=$VERSION_ID
        echo_info "Detected OS: $OS $OS_VERSION"
    else
        echo_error "Cannot detect OS. /etc/os-release not found."
        exit 1
    fi

    # Check for CUDA - first check if nvcc is in PATH
    if command -v nvcc &> /dev/null; then
        CUDA_VERSION=$(nvcc --version | grep "release" | sed 's/.*release //' | sed 's/,.*//')
        CUDA_HOME=$(dirname $(dirname $(which nvcc)))
        echo_info "CUDA detected: version $CUDA_VERSION"
        echo_info "CUDA_HOME: $CUDA_HOME"
        HAS_CUDA=true
    else
        # Check for CUDA installation in common locations
        for cuda_dir in /usr/local/cuda-* /usr/local/cuda /opt/cuda*; do
            if [ -d "$cuda_dir" ] && [ -f "$cuda_dir/bin/nvcc" ]; then
                export CUDA_HOME="$cuda_dir"
                export PATH="$CUDA_HOME/bin:$PATH"
                export LD_LIBRARY_PATH="$CUDA_HOME/lib64:$LD_LIBRARY_PATH"
                CUDA_VERSION=$($CUDA_HOME/bin/nvcc --version | grep "release" | sed 's/.*release //' | sed 's/,.*//')
                echo_info "CUDA found (not in PATH): version $CUDA_VERSION"
                echo_info "CUDA_HOME: $CUDA_HOME"
                echo_info "Adding CUDA to PATH for this session..."
                HAS_CUDA=true
                break
            fi
        done

        if [ "$HAS_CUDA" != "true" ]; then
            echo_warn "CUDA not found. ONNX Runtime will use CPU only."
            echo_warn "Install CUDA Toolkit from: https://developer.nvidia.com/cuda-downloads"
            HAS_CUDA=false
        fi
    fi

    # Check GPU
    if command -v nvidia-smi &> /dev/null; then
        GPU_INFO=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -1)
        echo_info "NVIDIA GPU detected: $GPU_INFO"
    fi
}

# Install system packages
install_system_packages() {
    echo_step "Installing system packages"

    case $OS in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential cmake git wget curl \
                libvulkan-dev vulkan-tools \
                libprotobuf-dev protobuf-compiler \
                ninja-build \
                libgomp1 \
                patchelf
            ;;
        fedora|rhel|centos)
            sudo dnf install -y \
                gcc gcc-c++ cmake git wget curl \
                vulkan-devel vulkan-tools \
                protobuf-devel \
                ninja-build \
                libgomp \
                patchelf
            ;;
        opensuse*|suse)
            # Install packages individually to avoid dependency conflicts
            echo_info "Installing openSUSE packages (skipping if already installed)..."

            local packages=(
                "gcc" "gcc-c++" "cmake" "wget" "curl"
                "vulkan-devel" "vulkan-tools"
                "protobuf-devel" "ninja"
                "libgomp1" "patchelf"
            )

            for pkg in "${packages[@]}"; do
                if ! rpm -q "$pkg" &>/dev/null; then
                    echo_info "Installing $pkg..."
                    sudo zypper install -y "$pkg" || echo_warn "Failed to install $pkg (may not be critical)"
                else
                    echo_info "$pkg already installed"
                fi
            done

            # git is often problematic in Tumbleweed due to perl dependencies
            if ! command -v git &>/dev/null; then
                echo_info "git not found, attempting to install..."
                sudo zypper install -y git || echo_warn "git installation failed - please install manually if needed"
            else
                echo_info "git already installed: $(git --version)"
            fi
            ;;
        *)
            echo_warn "Unknown OS: $OS. Please install dependencies manually."
            echo_info "Required: cmake, git, wget, vulkan-dev, protobuf-dev, ninja, libgomp"
            ;;
    esac

    echo_info "System packages installed"
}

# Install Vulkan SDK (latest)
install_vulkan_sdk() {
    echo_step "Installing Vulkan SDK"

    # Check if already installed
    if pkg-config --exists vulkan; then
        VULKAN_VERSION=$(pkg-config --modversion vulkan)
        echo_info "Vulkan already installed: version $VULKAN_VERSION"
        return 0
    fi

    echo_info "Installing Vulkan SDK from LunarG..."

    # Download and install LunarG Vulkan SDK
    VULKAN_VERSION="1.3.290"
    VULKAN_SDK_URL="https://sdk.lunarg.com/sdk/download/${VULKAN_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_VERSION}.tar.xz"

    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"

    if [ ! -f "vulkan-sdk.tar.xz" ]; then
        echo_info "Downloading Vulkan SDK..."
        wget -O vulkan-sdk.tar.xz "$VULKAN_SDK_URL"
    fi

    echo_info "Extracting Vulkan SDK..."
    tar -xf vulkan-sdk.tar.xz

    VULKAN_SDK_DIR="$TEMP_DIR/${VULKAN_VERSION}/x86_64"

    # Copy to system directories
    echo_info "Installing Vulkan SDK to $INSTALL_PREFIX..."
    sudo cp -r "$VULKAN_SDK_DIR/include/"* "$INSTALL_PREFIX/include/"
    sudo cp -r "$VULKAN_SDK_DIR/lib/"* "$INSTALL_PREFIX/lib/"

    # Update library cache
    sudo ldconfig

    echo_info "Vulkan SDK installed successfully"
}

# Install ncnn
install_ncnn() {
    echo_step "Installing ncnn library"

    # Check if already installed
    if [ -f "$INSTALL_PREFIX/lib/libncnn.a" ] || [ -f "$INSTALL_PREFIX/lib/libncnn.so" ]; then
        echo_info "ncnn already installed at $INSTALL_PREFIX"
        return 0
    fi

    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"

    # Clone ncnn
    if [ ! -d "ncnn" ]; then
        echo_info "Cloning ncnn repository..."
        git clone --depth=1 https://github.com/Tencent/ncnn.git
    fi

    cd ncnn
    git submodule update --init --recursive

    # Build ncnn
    echo_info "Building ncnn with Vulkan support..."
    mkdir -p build
    cd build

    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DNCNN_VULKAN=ON \
        -DNCNN_BUILD_EXAMPLES=OFF \
        -DNCNN_BUILD_TOOLS=ON \
        -DNCNN_BUILD_BENCHMARK=OFF \
        -DNCNN_BUILD_TESTS=OFF \
        ..

    ninja
    sudo ninja install

    echo_info "ncnn installed successfully"
}

# Install ONNX Runtime with CUDA support
install_onnx_runtime_cuda() {
    echo_step "Installing ONNX Runtime with CUDA support"

    if [ "$HAS_CUDA" != "true" ]; then
        echo_warn "CUDA not available. Installing CPU-only ONNX Runtime..."
        install_onnx_runtime_cpu
        return 0
    fi

    # Detect CUDA version for compatibility
    CUDA_MAJOR=$(echo $CUDA_VERSION | cut -d. -f1)
    CUDA_MINOR=$(echo $CUDA_VERSION | cut -d. -f2)

    echo_info "Installing ONNX Runtime $ONNX_VERSION for CUDA $CUDA_MAJOR.$CUDA_MINOR..."

    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"

    # Determine the correct ONNX Runtime package based on CUDA version
    # ONNX Runtime 1.20.1 supports CUDA 11.8 and 12.x
    if [ "$CUDA_MAJOR" -ge 12 ]; then
        ONNX_CUDA_VERSION="12.x"
        ONNX_PACKAGE="onnxruntime-linux-x64-gpu-${ONNX_VERSION}"
    elif [ "$CUDA_MAJOR" -eq 11 ]; then
        ONNX_CUDA_VERSION="11.8"
        ONNX_PACKAGE="onnxruntime-linux-x64-gpu-${ONNX_VERSION}"
    else
        echo_warn "CUDA version $CUDA_VERSION is too old. Installing CPU-only ONNX Runtime..."
        install_onnx_runtime_cpu
        return 0
    fi

    ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/${ONNX_PACKAGE}.tgz"

    echo_info "Downloading ONNX Runtime from: $ONNX_URL"

    if [ ! -f "${ONNX_PACKAGE}.tgz" ]; then
        wget "$ONNX_URL"
    fi

    echo_info "Extracting ONNX Runtime..."
    tar -xzf "${ONNX_PACKAGE}.tgz"

    # Install to system
    echo_info "Installing ONNX Runtime to $INSTALL_PREFIX..."
    cd "$ONNX_PACKAGE"

    sudo cp -r include/* "$INSTALL_PREFIX/include/"
    sudo cp -r lib/* "$INSTALL_PREFIX/lib/"

    # Update library cache
    sudo ldconfig

    # Verify installation
    if [ -f "$INSTALL_PREFIX/lib/libonnxruntime.so" ]; then
        echo_info "ONNX Runtime CUDA installed successfully"
        echo_info "Library: $INSTALL_PREFIX/lib/libonnxruntime.so"

        # Check for CUDA provider
        if ldd "$INSTALL_PREFIX/lib/libonnxruntime.so" | grep -q cuda; then
            echo_info "✓ CUDA provider available"
        else
            echo_warn "CUDA provider may not be available. Check CUDA installation."
        fi
    else
        echo_error "ONNX Runtime installation failed"
        return 1
    fi
}

# Install ONNX Runtime (CPU only - fallback)
install_onnx_runtime_cpu() {
    echo_step "Installing ONNX Runtime (CPU only)"

    mkdir -p "$TEMP_DIR"
    cd "$TEMP_DIR"

    ONNX_PACKAGE="onnxruntime-linux-x64-${ONNX_VERSION}"
    ONNX_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/${ONNX_PACKAGE}.tgz"

    echo_info "Downloading ONNX Runtime from: $ONNX_URL"

    if [ ! -f "${ONNX_PACKAGE}.tgz" ]; then
        wget "$ONNX_URL"
    fi

    echo_info "Extracting ONNX Runtime..."
    tar -xzf "${ONNX_PACKAGE}.tgz"

    # Install to system
    echo_info "Installing ONNX Runtime to $INSTALL_PREFIX..."
    cd "$ONNX_PACKAGE"

    sudo cp -r include/* "$INSTALL_PREFIX/include/"
    sudo cp -r lib/* "$INSTALL_PREFIX/lib/"

    # Update library cache
    sudo ldconfig

    echo_info "ONNX Runtime CPU installed successfully"
}

# Setup Real-ESRGAN
setup_real_esrgan() {
    echo_step "Setting up Real-ESRGAN"

    REAL_ESRGAN_DIR="/usr/local/src/Real-ESRGAN-ncnn-vulkan"

    if [ -d "$REAL_ESRGAN_DIR/src" ] && [ -f "$REAL_ESRGAN_DIR/src/realesrgan.h" ]; then
        echo_info "Real-ESRGAN already exists at $REAL_ESRGAN_DIR"
        return 0
    fi

    echo_info "Cloning Real-ESRGAN repository..."
    sudo mkdir -p /usr/local/src
    cd /usr/local/src

    # Remove partial clone if exists
    if [ -d "$REAL_ESRGAN_DIR" ]; then
        sudo rm -rf "$REAL_ESRGAN_DIR"
    fi

    sudo git clone --recursive https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan.git

    if [ ! -d "$REAL_ESRGAN_DIR" ]; then
        echo_error "Failed to clone Real-ESRGAN repository"
        return 1
    fi

    cd Real-ESRGAN-ncnn-vulkan

    # Update submodules if not already done
    echo_info "Updating git submodules..."
    sudo git submodule update --init --recursive

    # Build Real-ESRGAN (optional - EWOCvj2 only needs source)
    echo_info "Building Real-ESRGAN..."
    cd src

    # Check if ncnn is available
    if [ ! -f "$INSTALL_PREFIX/lib/libncnn.a" ] && [ ! -f "$INSTALL_PREFIX/lib/libncnn.so" ]; then
        echo_warn "ncnn not found, building Real-ESRGAN with bundled ncnn..."
        USE_SYSTEM_NCNN=OFF
    else
        echo_info "Using system ncnn library"
        USE_SYSTEM_NCNN=ON
    fi

    sudo mkdir -p build
    cd build

    sudo cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_SYSTEM_NCNN=$USE_SYSTEM_NCNN \
        -DUSE_SYSTEM_WEBP=OFF \
        .. || {
            echo_warn "CMake configuration failed, but source is available for EWOCvj2"
            echo_info "Real-ESRGAN source at $REAL_ESRGAN_DIR (build not required)"
            return 0
        }

    sudo make -j$(nproc) || {
        echo_warn "Real-ESRGAN build failed, but source is available for EWOCvj2"
        echo_info "Real-ESRGAN source at $REAL_ESRGAN_DIR (build not required)"
        return 0
    }

    echo_info "Real-ESRGAN source setup complete at $REAL_ESRGAN_DIR"
    echo_info "Models will be downloaded at runtime by EWOCvj2"
}

# Verify installations
verify_installations() {
    echo_step "Verifying installations"

    local all_ok=true

    # Check Vulkan
    if pkg-config --exists vulkan || [ -f "$INSTALL_PREFIX/include/vulkan/vulkan.h" ]; then
        echo_info "✓ Vulkan installed"
    else
        echo_warn "✗ Vulkan not found"
        all_ok=false
    fi

    # Check ncnn
    if [ -f "$INSTALL_PREFIX/lib/libncnn.a" ] || [ -f "$INSTALL_PREFIX/lib/libncnn.so" ]; then
        echo_info "✓ ncnn installed"
    else
        echo_warn "✗ ncnn not found"
        all_ok=false
    fi

    # Check ONNX Runtime
    if [ -f "$INSTALL_PREFIX/lib/libonnxruntime.so" ]; then
        echo_info "✓ ONNX Runtime installed"

        # Check for CUDA support
        if ldd "$INSTALL_PREFIX/lib/libonnxruntime.so" | grep -q cuda; then
            echo_info "  → CUDA support: YES"
        else
            echo_info "  → CUDA support: NO (CPU only)"
        fi
    else
        echo_warn "✗ ONNX Runtime not found"
        all_ok=false
    fi

    # Check Real-ESRGAN
    if [ -d "/usr/local/src/Real-ESRGAN-ncnn-vulkan/src" ]; then
        echo_info "✓ Real-ESRGAN source available"
    else
        echo_warn "✗ Real-ESRGAN source not found"
        all_ok=false
    fi

    echo ""
    if [ "$all_ok" = true ]; then
        echo_info "All dependencies installed successfully!"
    else
        echo_warn "Some dependencies are missing. Check warnings above."
    fi
}

# Show summary
show_summary() {
    echo ""
    echo_step "Installation Summary"

    cat << EOF
${GREEN}Installation complete!${NC}

${BLUE}What was installed:${NC}
  • Vulkan SDK - GPU compute and graphics API
  • ncnn - Neural network inference framework
  • ONNX Runtime - AI model runtime with CUDA support
  • Real-ESRGAN - AI upscaling framework

${BLUE}Installation prefix:${NC} $INSTALL_PREFIX

${BLUE}Environment variables (add to ~/.bashrc if needed):${NC}
  export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:\$LD_LIBRARY_PATH
  export PATH=$INSTALL_PREFIX/bin:\$PATH

${BLUE}Next steps:${NC}
  1. Run: sudo ldconfig
  2. Build EWOCvj2: ./build-appimage.sh
  3. The AppImage will include all CUDA/GPU support

${BLUE}To verify CMake can find everything:${NC}
  cd /home/gert/CLion/EWOCvj2-git
  mkdir -p build-test && cd build-test
  cmake ../src
  cd .. && rm -rf build-test

EOF
}

# Main installation
main() {
    echo_info "EWOCvj2 Dependency Installation Script"
    echo_info "This will install: ONNX Runtime (CUDA), ncnn, Vulkan, Real-ESRGAN"
    echo ""

    # Create temp directory
    mkdir -p "$TEMP_DIR"

    detect_system
    install_system_packages
    install_vulkan_sdk
    install_ncnn
    install_onnx_runtime_cuda
    setup_real_esrgan
    verify_installations
    show_summary

    # Cleanup
    echo_info "Cleaning up temporary files..."
    # Uncomment to auto-cleanup (keeping for debugging)
    # rm -rf "$TEMP_DIR"

    echo ""
    echo_info "${GREEN}All done! You can now build EWOCvj2 with full AI features.${NC}"
}

# Run main
main "$@"
