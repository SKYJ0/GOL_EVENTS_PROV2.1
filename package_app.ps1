# GOLEvents Pro Packaging Script
# Run this AFTER building in Release mode!

$version = "v2.3.3"
$releaseDir = ".\out\build\x64-release"
$exeName = "GOLEventsPro.exe"
$outputDir = ".\Release_Package_$version"

Write-Host ">>> Starting Package Process for $version..." -ForegroundColor Cyan

# 1. Check if Build Exists
if (Test-Path "$releaseDir\Release\$exeName") {
    $releaseDir = "$releaseDir\Release"
}
elseif (-not (Test-Path "$releaseDir\$exeName")) {
    Write-Host "ERROR: Could not find Release build at $releaseDir\$exeName" -ForegroundColor Red
    Write-Host "Please go to VS Code -> Build Variant -> Select 'Release' -> Build."
    exit
}

# 2. Clean/Create Output Directory
if (Test-Path $outputDir) { Remove-Item -Recurse -Force $outputDir }
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

# 3. Copy Executable
Copy-Item "$releaseDir\$exeName" -Destination "$outputDir\$exeName"
Write-Host " - Copied Executable" -ForegroundColor Green

# 3.1 Copy All Dependencies (DLLs)
Write-Host " - Copying all DLL dependencies from Release Root..."
Copy-Item "$releaseDir\*.dll" -Destination "$outputDir"

# 3.2 FORCE Copy from VCPKG Bin (The 'Nuclear Option' for dependencies)
# This grabs zstd, brotli, freetype, etc. that might be hiding
$vcpkgBin = "$releaseDir\vcpkg_installed\x64-windows\bin"
if (Test-Path $vcpkgBin) {
    Write-Host " - Copying DEEP dependencies from VCPKG ($vcpkgBin)..."
    Copy-Item "$vcpkgBin\*.dll" -Destination "$outputDir"
}
Write-Host " - Dependencies fully synced." -ForegroundColor Green

# 4. Copy Resources
if (Test-Path ".\resources") {
    Copy-Item -Recurse ".\resources" -Destination "$outputDir\resources"
    Write-Host " - Copied Resources" -ForegroundColor Green
}
else {
    Write-Host "WARNING: resources folder not found!" -ForegroundColor Yellow
}

# 5. Attempt Windeployqt (Qt Deployment)
$windeployqt = ".\out\build\x64-release\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe"

Write-Host " - Looking for windeployqt..."
if (Test-Path $windeployqt) {
    Write-Host " - LOCATED! Running deployment tool... please wait." -ForegroundColor Cyan
    # --dir sets the output directory explicitly
    # Added exclusions to reduce size: opengl-sw, d3d-compiler
    $proc = Start-Process -FilePath $windeployqt -ArgumentList "--release --no-translations --no-opengl-sw --no-system-d3d-compiler --compiler-runtime --dir `"$outputDir`" `"$outputDir\$exeName`"" -NoNewWindow -PassThru -Wait
    if ($proc.ExitCode -eq 0) {
        Write-Host " - Deployment successful (DLLs added)" -ForegroundColor Green
        
        # 5.1 CLEANUP: Remvoe Bloat
        Write-Host " - Cleaning up unnecessary files to reduce size..."
        $bloat = "*.pdb", "*.lib", "*.exp", "*.ilk", "*.obj", "vc_redist*", "D3Dcompiler*", "opengl32sw.dll", "Qt6Quick*", "Qt6Qml*", "Qt6ShaderTools.dll"
        foreach ($pattern in $bloat) {
            Remove-Item "$outputDir\$pattern" -Force -ErrorAction SilentlyContinue
        }
        
        # 5.2 SYNC BACK TO BUILD FOLDER (For debugging/running immediately)
        Write-Host " - Syncing plugins back to build folder for local testing..."
        $plugins = "platforms", "styles", "imageformats", "tls", "iconengines", "resources"
        foreach ($p in $plugins) {
            if (Test-Path "$outputDir\$p") {
                Copy-Item -Recurse -Force "$outputDir\$p" "$releaseDir\$p" -ErrorAction SilentlyContinue
            }
        }
    }
    else {
        Write-Host "ERROR: windeployqt failed with code $($proc.ExitCode)." -ForegroundColor Red
    }
}
else {
    Write-Host "WARNING: Could not find windeployqt at $windeployqt" -ForegroundColor Yellow
    Write-Host "You must find it and run it manually, or add Qt to your PATH."
}

# 6. Zip It
$zipName = "GOLEventsPro_$version.zip"
if (Test-Path $zipName) { Remove-Item $zipName }
Compress-Archive -Path "$outputDir\*" -DestinationPath $zipName
Write-Host "DONE! Zip created: $zipName" -ForegroundColor Green
Write-Host "Location: $(Get-Location)\$zipName"
