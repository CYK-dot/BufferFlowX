# Save the current location
$scriptDir = Get-Location

# Navigate to the test directory
Set-Location ".."

# Create build directory if it doesn't exist
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Name "build" | Out-Null
}

# Navigate to the build directory
Set-Location "build"

# Clean the entire build directory
Get-ChildItem -Path . -Force | Where-Object { $_.Name -ne 'CMakeCache.txt' -and $_.Name -ne 'CMakeFiles' -and $_.Name -ne 'cmake_install.cmake' } | Remove-Item -Recurse -Force
# Alternative: Remove-Item * -Recurse -Force -ErrorAction SilentlyContinue

# Run cmake and make commands
cmake .. -DENABLE_CODE_COVERAGE=ON
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Set-Location $scriptDir
    exit $LASTEXITCODE
}

make clean
if ($LASTEXITCODE -ne 0) {
    Write-Host "Make clean failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Set-Location $scriptDir
    exit $LASTEXITCODE
}

make -j8 all
if ($LASTEXITCODE -ne 0) {
    Write-Host "Make all failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Set-Location $scriptDir
    exit $LASTEXITCODE
}

# Return to the script directory
Set-Location $scriptDir
Write-Host "Build completed successfully!" -ForegroundColor Green