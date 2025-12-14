#!/bin/bash

# Save current directory
SCRIPT_DIR=$(pwd)

# Remove existing directories if they exist
echo "Cleaning up existing directories..."
rm -rf build googletest

# Create lib directory if it doesn't exist
echo "Creating lib directory..."
mkdir -p ../lib

# Verify lib directory exists
if [ ! -d "../lib" ]; then
    echo "Error: Failed to create lib directory"
    exit 1
fi

# Clone googletest
echo "Cloning googletest..."
git clone https://github.com/google/googletest.git

# Check if clone was successful
if [ ! -d "googletest" ]; then
    echo "Error: Failed to clone googletest"
    exit 1
fi

# Create build directory and navigate to it
echo "Creating build directory..."
mkdir -p build
cd build

# Build googletest
echo "Building googletest..."
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ../googletest
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed"
    cd "$SCRIPT_DIR"
    exit 1
fi

cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Error: Build failed"
    cd "$SCRIPT_DIR"
    exit 1
fi

# Get absolute path for lib directory (relative to script directory)
LIB_DIR="$(cd "$SCRIPT_DIR/../lib" && pwd)"

# Copy .a files to lib directory with verbose output for debugging
echo "Copying .a files to lib directory: $LIB_DIR"
find . -name "*.a" -type f -exec cp {} "$LIB_DIR"/ \;

# Check if any .a files were copied
if [ -z "$(find "$LIB_DIR" -name '*.a' -type f 2>/dev/null)" ]; then
    echo "Warning: No .a files were copied to lib directory"
    # List what we have in the build directory
    echo "Build directory contents:"
    find . -name "*.a" -type f
    # List what we have in the lib directory
    echo "Lib directory contents:"
    find "$LIB_DIR" -mindepth 1 2>/dev/null || echo "Cannot list lib directory contents"
else
    echo "Successfully copied .a files to lib directory"
    # Show what was copied
    echo "Copied files:"
    find "$LIB_DIR" -name "*.a" -type f
fi

# Navigate back to the script directory
cd "$SCRIPT_DIR"

echo "Finish! Check ../lib directory for files."