# Navigate to the build directory where easy_test.exe is located
Set-Location "../build"

# Run the test executable
.\easy_test.exe

# Check if the test execution was successful
if ($LASTEXITCODE -eq 0) {
    # Generate coverage report
    make easy_test_coverage_report
} else {
    Write-Host "Test execution failed with exit code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

# Return to the script directory
Set-Location "..\script"