# Build and Run Script for GOLEvents Pro
# RUN THIS IN "x64 Native Tools Command Prompt for VS 2022" (or similar)

Write-Host ">>> GOLEVENTS PRO BUILDER <<<" -ForegroundColor Cyan

# 0. Environment Check
if (-not (Get-Command "cl.exe" -ErrorAction SilentlyContinue)) {
    Write-Host "Compilers not found. Attempting to locate Visual Studio..." -ForegroundColor Yellow
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($path) {
            $vcvars = "$path\VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars) {
                Write-Host "Found VS at $path. Launching build in developer environment..." -ForegroundColor Cyan
                cmd /c "call `"$vcvars`" && powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`""
                exit
            }
        }
    }
    Write-Host "Error: Visual Studio C++ compilers (cl.exe) not found in PATH." -ForegroundColor Red
    Write-Host "Please run this script from 'x64 Native Tools Command Prompt for VS 2022'."
    exit
}

# 1. Configure
Write-Host "1. Configuring CMake (x64-release)..."
cmake --preset x64-release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: CMake Configuration failed." -ForegroundColor Red
    exit
}

# 2. Build
Write-Host "2. Building Project..."
cmake --build out/build/x64-release --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Build failed." -ForegroundColor Red
    exit
}

# 3. Package
Write-Host "3. Packaging..."
./package_app.ps1

Write-Host ">>> BUILD COMPLETE <<<" -ForegroundColor Green
Write-Host "You can find the packaged app in the 'Release_Package_v2.3.0' folder."

# 4. Optional Run
$choice = Read-Host "Do you want to run the app now? (y/n)"
if ($choice -eq 'y') {
    Start-Process ".\Release_Package_v2.3.0\GOLEventsPro.exe"
}
