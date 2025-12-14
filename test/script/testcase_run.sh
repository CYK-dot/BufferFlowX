#!/bin/bash

# Navigate to the build directory where easy_test is located
cd "../build"

# Run the test executable
./easy_test

# Check if the test execution was successful
if [ $? -eq 0 ]; then
    # Generate coverage report
    make easy_test_coverage_report
else
    echo "Test execution failed with exit code $?"
    # Return to the script directory
    cd "../script"
    exit $?
fi

# Return to the script directory
cd "../script"