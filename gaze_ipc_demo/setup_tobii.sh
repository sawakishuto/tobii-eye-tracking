#!/bin/bash

echo "Setting up Tobii Research SDK for macOS..."

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Homebrew is not installed. Please install Homebrew first:"
    echo "/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

# Install GLFW via Homebrew
echo "Installing GLFW..."
brew install glfw

# Create SDK directory structure
SDK_DIR="/opt/tobii/sdk"
echo "Creating SDK directory at $SDK_DIR..."

if [ ! -d "$SDK_DIR" ]; then
    sudo mkdir -p "$SDK_DIR"/{lib,include}
    sudo chown -R $(whoami) /opt/tobii
fi

echo "Please download the Tobii Research SDK from:"
echo "https://developer.tobii.com/product-integration/stream-engine/"
echo ""
echo "After downloading, extract and copy:"
echo "- Include files to: $SDK_DIR/include/"
echo "- Library files to: $SDK_DIR/lib/"
echo ""
echo "For macOS, you'll need:"
echo "- libtobii_research.dylib"
echo "- tobii_research.h and other header files"
echo ""
echo "If you're using an alternative SDK path, you can specify it during cmake:"
echo "cmake -DTOBII_SDK_PATH=/your/path/to/tobii/sdk .." 