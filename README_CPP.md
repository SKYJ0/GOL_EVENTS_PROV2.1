# GOLEvents Pro (C++ Edition)

This is the fully converted C++ version of the GOLEvents application.

## Structure
- `src/`: Source code.
- `src/tools/`: Implementation of all 10 tools.
- `build/`: CMake build directory.
- `publish/`: Final release ZIPs.
- `legacy_python/`: Original Python scripts (reference only).

## Build Instructions
1. Install CMake and Visual Studio 2022.
2. `mkdir build && cd build`
3. `cmake ..`
4. `cmake --build . --config Release`

## Resources
- Assets and icons are in `src/resources` and `assets/`.
- The executable requires the `resources` folder to be present in the same directory.

## Update v2.3 (Admin Edition)
- **Admin Dashboard:** Secure, role-restricted panel for user management.
- **Custom Roles:** Dashboard displays specific roles (e.g., "VIP ACCESS").
- **Release:** `GOLEventsPro_Final_Custom_Role.zip` created.
