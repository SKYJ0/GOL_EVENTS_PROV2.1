@echo off
echo ">>> FORCE BUILD INITIATED (NINJA) <<<"

:: 1. Setup Environment
set "VSPATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
echo "Checking VSPATH: %VSPATH%"
dir "%VSPATH%"
if exist "%VSPATH%" (
    call "%VSPATH%" -arch=x64
) else (
    echo "ERROR: VsDevCmd.bat not found at %VSPATH%"
    exit /b 1
)

:: 2. Configure (Clean)
if exist "out\build\x64-release" rmdir /s /q "out\build\x64-release"
mkdir "out\build\x64-release"

echo "Configuring CMake (Ninja)..."
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=vcpkg_new/scripts/buildsystems/vcpkg.cmake -S . -B out/build/x64-release
if %errorlevel% neq 0 exit /b %errorlevel%

:: 3. Build
echo "Building..."
cmake --build out/build/x64-release
if %errorlevel% neq 0 exit /b %errorlevel%

:: 4. Package
echo "Packaging..."
powershell -ExecutionPolicy Bypass -File package_app.ps1

echo ">>> FORCE BUILD COMPLETE <<<"
