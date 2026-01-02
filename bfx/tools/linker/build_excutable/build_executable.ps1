# BufferFlowX Linker GUI Application - Build Script
# This script creates an executable from the Python GUI application using PyInstaller
# It manages a virtual environment and handles all necessary dependencies

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("--build", "--clean", "--clean_all")]
    [string]$Action
)

# Define paths
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Join-Path $scriptDir ".."
$buildDir = $scriptDir
$venvDir = Join-Path $buildDir "build_excutable_env"
$srcDir = Join-Path $projectDir "src"
$templateDir = Join-Path $projectDir "template"
$mainScript = Join-Path $projectDir "bfx_linker_app_gui.py"
$outputDir = Join-Path $buildDir "dist"
$buildOutputDir = Join-Path $buildDir "build"

Write-Host "BufferFlowX Linker GUI Build Script" -ForegroundColor Green
Write-Host "Action: $Action" -ForegroundColor Yellow

# Function to check if Python is available
function Check-Python {
    try {
        $pythonVersion = python --version 2>&1
        Write-Host "Found Python: $pythonVersion" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "Error: Python is not installed or not in PATH" -ForegroundColor Red
        return $false
    }
}

# Function to check if virtual environment exists
function Test-VenvExists {
    return Test-Path $venvDir
}

# Function to create virtual environment
function Create-Venv {
    Write-Host "Creating virtual environment..." -ForegroundColor Cyan
    python -m venv $venvDir
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: Failed to create virtual environment" -ForegroundColor Red
        exit 1
    }
    Write-Host "Virtual environment created at: $venvDir" -ForegroundColor Green
}

# Function to activate virtual environment
function Activate-Venv {
    $activateScript = Join-Path $venvDir "Scripts\Activate.ps1"
    if (Test-Path $activateScript) {
        & $activateScript
        Write-Host "Virtual environment activated" -ForegroundColor Green
    } else {
        Write-Host "Error: Activation script not found at $activateScript" -ForegroundColor Red
        exit 1
    }
}

# Function to install requirements
function Install-Requirements {
    $requirementsFile = Join-Path $projectDir "requirements.txt"
    if (Test-Path $requirementsFile) {
        Write-Host "Installing requirements from $requirementsFile..." -ForegroundColor Cyan
        pip install --upgrade pip
        pip install -r $requirementsFile
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Error: Failed to install requirements" -ForegroundColor Red
            exit 1
        }
        Write-Host "Requirements installed successfully" -ForegroundColor Green
    } else {
        Write-Host "Warning: requirements.txt not found at $requirementsFile" -ForegroundColor Yellow
    }
}

# Function to build executable
function Build-Executable {
    Write-Host "Building executable..." -ForegroundColor Cyan
    
    # Activate virtual environment
    Activate-Venv
    
    # Check if PyInstaller is available
    $pyinstallerCheck = pip show pyinstaller 2>$null
    if (-not $pyinstallerCheck) {
        Write-Host "PyInstaller not found, installing..." -ForegroundColor Yellow
        pip install pyinstaller
    }
    
    # Prepare PyInstaller command with necessary options
    $pyinstallerCmd = "pyinstaller"
    $pyinstallerArgs = @(
        "--name=bfx_linker",
        "--windowed",
        "--onefile",
        "--add-data=`"$srcDir;src`"",
        "--add-data=`"$templateDir;template`"",
        "--collect-all=jinja2",
        "--hidden-import=jinja2",
        "--hidden-import=tkinter",
        "--hidden-import=json",
        "--hidden-import=re",
        "--hidden-import=datetime",
        "--clean",
        "--noconfirm",
        $mainScript
    )
    
    Write-Host "Running: $pyinstallerCmd $($pyinstallerArgs -join ' ')" -ForegroundColor Cyan
    
    & $pyinstallerCmd @pyinstallerArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Executable built successfully!" -ForegroundColor Green
        Write-Host "Output location: $outputDir" -ForegroundColor Green
    } else {
        Write-Host "Error: Failed to build executable" -ForegroundColor Red
        exit 1
    }
}

# Function to clean build artifacts
function Clean-BuildArtifacts {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Cyan
    
    $cleanPaths = @(
        $outputDir,
        $buildOutputDir,
        "$(Join-Path $buildDir 'bfx_linker.spec')"
    )
    
    foreach ($path in $cleanPaths) {
        if (Test-Path $path) {
            Remove-Item -Path $path -Recurse -Force
            $message = "Removed: $path"
            Write-Host $message -ForegroundColor Yellow
        }
    }
    
    Write-Host "Build artifacts cleaned" -ForegroundColor Green
}

# Function to clean everything including virtual environment
function Clean-All {
    Clean-BuildArtifacts
    
    if (Test-VenvExists) {
        Write-Host "Removing virtual environment..." -ForegroundColor Cyan
        Remove-Item -Path $venvDir -Recurse -Force
        Write-Host "Virtual environment removed" -ForegroundColor Yellow
    } else {
        Write-Host "Virtual environment not found, nothing to remove" -ForegroundColor Yellow
    }
}

# Main execution logic
if (-not (Check-Python)) {
    exit 1
}

switch ($Action) {
    "--build" {
        if (-not (Test-VenvExists)) {
            Write-Host "Virtual environment not found, creating one..." -ForegroundColor Yellow
            Create-Venv
            Activate-Venv
            Install-Requirements
        } else {
            Write-Host "Using existing virtual environment at: $venvDir" -ForegroundColor Green
            Activate-Venv
        }
        Build-Executable
    }
    
    "--clean" {
        Clean-BuildArtifacts
    }
    
    "--clean_all" {
        Clean-All
    }
}

Write-Host "Build script completed." -ForegroundColor Green