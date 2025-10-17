#!/bin/bash

# ARCOS Library Setup Script for PlatformIO
# This script helps set up the ARCOS library for PlatformIO projects

echo "ARCOS Library PlatformIO Setup"
echo "=============================="

# Check if we're in a PlatformIO project
if [ ! -f "platformio.ini" ]; then
    echo "Error: No platformio.ini found. Please run this script in your PlatformIO project root."
    exit 1
fi

# Create lib directory if it doesn't exist
mkdir -p lib

# Check if ARCOS is already cloned
if [ -d "lib/ARCOS" ]; then
    echo "ARCOS already exists in lib/. Updating..."
    cd lib/ARCOS
    git pull
    cd ../..
else
    echo "Cloning ARCOS repository..."
    cd lib
    git clone https://github.com/XCR1793/ARCOS.git
    cd ..
fi

# Check if library is properly structured
if [ -d "lib/ARCOS/ARCOS_library" ]; then
    echo "Repository structure detected. Setting up library..."
    
    # Option 1: Move library to proper location
    if [ ! -d "lib/ARCOS_Library" ]; then
        echo "Creating properly structured library..."
        cp -r lib/ARCOS/ARCOS_library lib/ARCOS_Library
        echo "✓ ARCOS library set up in lib/ARCOS_Library/"
        
        echo ""
        echo "Setup complete! You can now use:"
        echo "#include <arcos.hpp>"
        echo ""
        echo "Or modular includes like:"
        echo "#include <arcos_core.hpp>"
        echo "#include <arcos_algorithms.hpp>"
        echo "#include <arcos_drivers.hpp>"
    else
        echo "✓ ARCOS library already properly set up."
    fi
    
else
    echo "✓ Library is already at repository root."
fi

echo ""
echo "Alternative: Add this to your platformio.ini build_flags:"
echo "-I.pio/libdeps/\$PIOENV/ARCOS/ARCOS_library/include"
echo ""
echo "For Git dependency method, use:"
echo "lib_deps = https://github.com/XCR1793/ARCOS.git#main"