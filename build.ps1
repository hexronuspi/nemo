# Path: c:\Users\aruna\Desktop\backtest\build.ps1
# PowerShell build script for SOTA Backtesting Engine

param(
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$EnablePython,
    [switch]$EnableTests,
    [int]$Jobs = 4
)

Write-Host "🚀 Building SOTA Backtesting Engine" -ForegroundColor Green
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow

# Create build directory
$BuildDir = "build"
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "🧹 Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

# Configure CMake
$CMakeArgs = @(
    "..",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_CXX_STANDARD=20"
)

if ($EnablePython) {
    $CMakeArgs += "-DENABLE_PYTHON_BINDINGS=ON"
    Write-Host "🐍 Python bindings enabled" -ForegroundColor Cyan
}

if ($EnableTests) {
    $CMakeArgs += "-DBUILD_TESTS=ON"
    Write-Host "🧪 Tests enabled" -ForegroundColor Cyan
}

Write-Host "⚙️ Configuring with CMake..." -ForegroundColor Yellow
cmake @CMakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build
Write-Host "🔨 Building project..." -ForegroundColor Yellow
cmake --build . --config $BuildType --parallel $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Build completed successfully!" -ForegroundColor Green

# Run tests if enabled
if ($EnableTests) {
    Write-Host "🧪 Running tests..." -ForegroundColor Yellow
    ctest --config $BuildType --output-on-failure
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Tests failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✅ All tests passed!" -ForegroundColor Green
}

# Show executable location
$ExePath = if ($IsWindows) { ".\$BuildType\backtest_engine.exe" } else { "./backtest_engine" }
Write-Host "🎯 Executable location: $ExePath" -ForegroundColor Cyan

# Return to original directory
Set-Location ..

Write-Host "🎉 Build process complete!" -ForegroundColor Green
Write-Host "To run: .\build\$BuildType\backtest_engine.exe config\experiments\sample_experiment.yaml" -ForegroundColor Cyan

