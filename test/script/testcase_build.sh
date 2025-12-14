#!/bin/bash

# Save the current location
SCRIPT_DIR=$(pwd)

# Navigate to the test directory
cd ..

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir -p build
fi

# Navigate to the build directory
cd build

# Run cmake and make commands
cmake .. -DNABLE_CODE_COVERAGE=ON
if [ $? -ne 0 ]; then
    echo "CMake failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

make clean
if [ $? -ne 0 ]; then
    echo "Make clean failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

make all
if [ $? -ne 0 ]; then
    echo "Make all failed with exit code $?"
    cd "$SCRIPT_DIR"
    exit $?
fi

# Return to the script directory
cd "$SCRIPT_DIR"
echo "Build completed successfully!"