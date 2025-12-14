#!/bin/bash

# Save current directory
SCRIPT_DIR=$(pwd)

# Navigate to the build directory where easy_test is located
cd "../build"

# Check if easy_test executable exists
if [ ! -f "./easy_test" ]; then
    echo "Error: easy_test executable not found in build directory"
    cd "$SCRIPT_DIR"
    exit 1
fi

# Run the test executable
echo "Running tests..."
./easy_test

# Check if the test execution was successful
if [ $? -eq 0 ]; then
    echo "Tests passed successfully"
    # Generate coverage report
    echo "Generating coverage report..."
    make easy_test_coverage_report
    if [ $? -ne 0 ]; then
        echo "Warning: Coverage report generation failed"
    else
        echo "Coverage report generated successfully"
    fi
else
    echo "Test execution failed with exit code $?"
    # Return to the script directory
    cd "$SCRIPT_DIR"
    exit 1
fi

# Return to the script directory
cd "$SCRIPT_DIR"
echo "Test run completed"