@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ==========================================
echo      GOLEVENTS PRO - FINAL BUILDER
echo ==========================================

:: 1. Find Visual Studio
echo [1/5] Searching for Visual Studio...
set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found! Is Visual Studio installed?
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo ERROR: Could not find a valid Visual Studio installation with C++ tools.
    pause
    exit /b 1
)

echo Found VS at: %VS_PATH%

:: 2. Setup Environment
echo [2/5] Setting up Developer Environment...
set "DEVCMD=%VS_PATH%\Common7\Tools\VsDevCmd.bat"
if not exist "%DEVCMD%" (
    echo ERROR: VsDevCmd.bat not found at %DEVCMD%
    pause
    exit /b 1
)

call "%DEVCMD%" -arch=x64 -host_arch=x64

:: 3. Configure
echo [3/5] Configuring CMake...
if exist "out\build\x64-release" rmdir /s /q "out\build\x64-release"
mkdir "out\build\x64-release"

cmake -S . -B out/build/x64-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=vcpkg_new/scripts/buildsystems/vcpkg.cmake
if %errorlevel% neq 0 (
    echo ERROR: CMake Configuration failed.
    echo Trying fallback generator "Visual Studio 17 2022"...
    rmdir /s /q "out\build\x64-release"
    cmake -S . -B out/build/x64-release -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=vcpkg_new/scripts/buildsystems/vcpkg.cmake
    if !errorlevel! neq 0 (
        echo ERROR: Fallback Configuration also failed.
        pause
        exit /b 1
    )
)

:: 4. Build
echo [4/5] Building Project...
cmake --build out/build/x64-release --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

:: 5. Package
echo [5/5] Packaging...
powershell -ExecutionPolicy Bypass -File package_app.ps1

echo ==========================================
echo      BUILD SUCCESSFUL!
echo ==========================================
echo Zip file is ready in Release_Package_v2.3.3 folder.
pause
