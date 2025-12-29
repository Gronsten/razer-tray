cmake_minimum_required(VERSION 3.20)
project(RazerTray VERSION 1.0.0 LANGUAGES CXX)

# Use C++20 for modern features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable all warnings and treat them as errors for safety
if(MSVC)
    add_compile_options(/W4 /WX)
    # Enable static runtime linking for standalone executable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/DeviceMonitor.cpp
    src/BatteryIcon.cpp
    src/TrayApp.cpp
    src/ConfigManager.cpp
)

set(HEADERS
    src/DeviceMonitor.h
    src/BatteryIcon.h
    src/TrayApp.h
    src/SafeHandles.h
    src/ConfigManager.h
)

# Create Windows GUI application (no console window)
add_executable(RazerTray WIN32 ${SOURCES} ${HEADERS})

# Link required Windows libraries
target_link_libraries(RazerTray
    setupapi      # Device enumeration
    cfgmgr32      # Device properties
    shell32       # System tray
    gdi32         # Icon drawing
    gdiplus       # GDI+ for advanced graphics
    ole32         # COM initialization
    comctl32      # Common controls
)

# Set output directory
set_target_properties(RazerTray PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy runtime configuration files to output directory after build
add_custom_command(TARGET RazerTray POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        "${CMAKE_BINARY_DIR}/bin/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.example.json"
        "${CMAKE_BINARY_DIR}/bin/config.example.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/razer-config.ps1"
        "${CMAKE_BINARY_DIR}/bin/razer-config.ps1"
    COMMENT "Copying runtime files to output directory..."
)

# Installation
install(TARGETS RazerTray
    RUNTIME DESTINATION bin
)

# Install runtime configuration files
install(FILES
    config.json
    config.example.json
    razer-config.ps1
    DESTINATION bin
)
