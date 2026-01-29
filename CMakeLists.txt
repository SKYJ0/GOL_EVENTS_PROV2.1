# Force Rebuild Trigger v2
cmake_minimum_required(VERSION 3.16)
project(GOLEventsPro VERSION 2.3.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Svg PrintSupport Concurrent)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Tesseract CONFIG REQUIRED)
find_package(unofficial-poppler CONFIG REQUIRED)
find_package(libqrencode REQUIRED)
find_package(OpenXLSX CONFIG REQUIRED)

# Define executable
add_executable(${PROJECT_NAME} WIN32
    version.rc
    src/main.cpp
)

# ...

# Link libraries
target_link_libraries(GOLEventsPro PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Svg
    Qt6::PrintSupport
    Qt6::Concurrent
    CURL::libcurl
    nlohmann_json::nlohmann_json
    libqrencode::libqrencode
    Tesseract::libtesseract
    unofficial::poppler::poppler-cpp
    OpenXLSX::OpenXLSX
)

target_sources(GOLEventsPro PRIVATE
    src/main.cpp
    src/MainWindow.cpp
    src/AuthManager.cpp
    src/SecurityManager.cpp
    src/UpdateChecker.cpp
    src/UpdateChecker.h
    src/CosmicDialog.cpp
    src/CosmicDialog.h
    src/Utils.cpp
    src/Logger.cpp
    src/Logger.h
    src/SettingsPage.cpp
    src/SettingsPage.h
    src/NotificationIsland.cpp
    src/NotificationIsland.h
    src/AnimatedBackground.cpp
    src/AnimatedBackground.h
    src/FlowLayout.cpp
    src/GuideDialog.cpp
    src/ToolCard.cpp
    src/ToolCard.h
    src/tools/CalcStock.cpp
    src/tools/CheckListing.cpp
    src/tools/CheckPrice.cpp
    src/tools/DailyReport.cpp
    src/tools/ExpanderSeats.cpp
    src/tools/PdfsToTxt.cpp
    src/tools/Placeholder.cpp
    src/tools/QrGenerator.cpp
    src/tools/RenamerFv.cpp
    src/tools/SplitterRenamer.cpp
    src/tools/StockReport.cpp
    src/tools/VerifyOrders.cpp
    src/AdminPanel.cpp
    src/AdminPanel.h
    ${TOOL_HEADERS}
)

set(TOOL_HEADERS
    src/tools/CalcStock.h
    src/tools/CheckListing.h
    src/tools/CheckPrice.h
    src/tools/DailyReport.h
    src/tools/ExpanderSeats.h
    src/tools/PdfsToTxt.h
    src/tools/Placeholder.h
    src/tools/QrGenerator.h
    src/tools/RenamerFv.h
    src/tools/SplitterRenamer.h
    src/tools/StockReport.h
    src/tools/VerifyOrders.h
)

target_include_directories(GOLEventsPro PRIVATE 
    ${QRENCODE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Windows specific settings
if(WIN32)
    set_target_properties(GOLEventsPro PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Set application icon
    target_sources(GOLEventsPro PRIVATE resources/app_icon.rc)
endif()

# Install rules
install(TARGETS GOLEventsPro
    RUNTIME DESTINATION bin
)
