#!/bin/bash

# Save the current location
SCRIPT_DIR=$(pwd)

# Navigate to the test directory
cd ..

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# Navigate to the build directory
cd build

# Clean the entire build directory, but preserve CMake cache files to avoid full reconfiguration
find . -mindepth 1 -maxdepth 1 -not -name 'CMakeCache.txt' -not -name 'CMakeFiles' -not -name 'cmake_install.cmake' -exec rm -rf {} +

# Run cmake and make commands
echo "Configuring with CMake..."
cmake .. -DENABLE_CODE_COVERAGE=ON
if [ $? -ne 0 ]; then
    echo "CMake failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

echo "Cleaning previous builds..."
make clean
if [ $? -ne 0 ]; then
    echo "Make clean failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

echo "Building test cases..."
make all
if [ $? -ne 0 ]; then
    echo "Make all failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

# Return to the script directory
cd "$SCRIPT_DIR"
echo "Build completed successfully!"