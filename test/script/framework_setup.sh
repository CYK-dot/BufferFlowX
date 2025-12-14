#!/bin/bash

# Remove existing directories if they exist
rm -rf build googletest

# Create lib directory if it doesn't exist
mkdir -p ../lib

# Clone googletest
git clone https://github.com/google/googletest.git

# Create build directory and navigate to it
mkdir -p build
cd build

# Build googletest
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ../googletest
cmake --build . --config Release

# Copy .a files to lib directory
cp lib/*.a ../lib/ 2>/dev/null || true
cp *.a ../lib/ 2>/dev/null || true

find . -name "*.a" -type f -exec cp {} ../lib/ \;

# Navigate back to the script directory
cd ..

echo "Finish! Check ../lib directory for files."