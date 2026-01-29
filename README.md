# GOLEVENTS PRO - C++ Edition

Complete C++ conversion of the GOLEVENTS ticket inventory management system.

## Features

- ✅ Firebase authentication with HWID locking
- ✅ GitHub update checking
- ✅ Qt6-based modern dark UI
- ✅ 10 inventory management tools
- ✅ PRO/Basic user role system
- ✅ Credit-based subscription system

## Prerequisites

### Required Software

1. **Visual Studio 2022** (or compatible C++ compiler)
2. **CMake** 3.20 or higher
3. **vcpkg** package manager
4. **Git**

### Installing vcpkg

```powershell
# Clone vcpkg
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH (optional but recommended)
$env:PATH += ";C:\vcpkg"
```

## Building the Project

### Step 1: Install Dependencies via vcpkg

```powershell
cd C:\vcpkg

# Install all required packages
.\vcpkg install qt6-base:x64-windows
.\vcpkg install qt6-svg:x64-windows
.\vcpkg install curl:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install qrencode:x64-windows
.\vcpkg install tesseract:x64-windows
.\vcpkg install poppler:x64-windows
.\vcpkg install leptonica:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

**Note**: This will take 30-60 minutes depending on your system.

### Step 2: Configure CMake

```powershell
cd C:\Users\kawka\.gemini\antigravity\scratch\app_cpp_conversion

# Create build directory
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Step 3: Build

```powershell
# Build Release version
cmake --build . --config Release

# Or open in Visual Studio
start GOLEventsPro.sln
```

### Step 4: Run

```powershell
cd Release
.\GOLEventsPro.exe
```

## Project Structure

```
app_cpp_conversion/
├── CMakeLists.txt          # Build configuration
├── vcpkg.json              # Dependency manifest
├── src/
│   ├── main.cpp            # Entry point
│   ├── MainWindow.h/cpp    # Main application window
│   ├── AuthManager.h/cpp   # Firebase authentication
│   ├── UpdateChecker.h/cpp # GitHub update checker
│   ├── Utils.h/cpp         # Utility functions
│   └── tools/              # Tool implementations
│       ├── CalcStock.h/cpp
│       ├── QrGenerator.h/cpp
│       ├── ExpanderSeats.h/cpp
│       └── ... (7 more tools)
└── resources/
    ├── knight.png          # Splash screen image
    ├── knight.ico          # Application icon
    └── resources.qrc       # Qt resource file
```

## Implemented Tools

### Fully Functional
- ✅ **QR Generator**: Generate 1035x1035 QR codes from text/URLs
- ✅ **Seat Expander**: Expand seat ranges to individual seat IDs
- ✅ **Stock Calculator**: Basic PDF scanning and report generation

### Stub Implementations (UI ready, logic pending)
- 🔧 **Price Checker**: Market price monitoring
- 🔧 **Listing Checker**: Verify marketplace listings
- 🔧 **PDFs to TXT**: Extract PDF filenames
- 🔧 **Placeholder Generator**: Create empty placeholder PDFs
- 🔧 **Renamer FV**: Rename files with face value (OCR)
- 🔧 **Splitter Renamer**: Split multi-page PDFs
- 🔧 **Order Verifier**: Cross-check orders with inventory

## Firebase Configuration

The application connects to:
```
https://golevents1-default-rtdb.europe-west1.firebasedatabase.app
```

License keys are validated against the `/keys/{license_key}` endpoint.

## Development Notes

### Adding New Features

1. Tools are implemented as `QDialog` subclasses in `src/tools/`
2. Each tool has its own `.h` and `.cpp` file
3. Tools are launched from `MainWindow::launchTool()`

### Styling

All colors are defined in `Utils.h`:
- `ACCENT_COLOR`: #1f6aa5 (Blue)
- `PRO_COLOR`: #ffcc00 (Gold)
- `BG_COLOR`: #0b0b0b (Dark background)
- `SUCCESS_COLOR`: #10b981 (Green)
- `ERROR_COLOR`: #f43f5e (Red)

## Troubleshooting

### CMake can't find Qt6
```powershell
# Make sure vcpkg integration is active
C:\vcpkg\vcpkg integrate install

# Verify Qt6 is installed
C:\vcpkg\vcpkg list | findstr qt6
```

### Missing DLLs at runtime
```powershell
# Copy Qt DLLs to build directory
windeployqt Release\GOLEventsPro.exe
```

### Linker errors
- Ensure all vcpkg packages are installed for `x64-windows` triplet
- Rebuild vcpkg packages if needed: `vcpkg remove <package> && vcpkg install <package>:x64-windows`

## Next Steps

1. Complete remaining tool implementations (PDF parsing, OCR, etc.)
2. Add comprehensive error handling
3. Implement update download mechanism
4. Create installer with InnoSetup or NSIS
5. Add unit tests

## License

Proprietary - GOLEVENTS PRO
Created by Omar
