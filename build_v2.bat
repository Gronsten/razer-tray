@echo off
setlocal

echo ============================================
echo Building Razer Tray v2.0 (USB/libusb)
echo ============================================
echo.

:: Configuration
set BUILD_DIR=build_v2
set SRC_DIR=src_v2_usb
set LIBUSB_DIR=..\libusb-1.0.29

:: Check if libusb exists
if not exist "%LIBUSB_DIR%\include\libusb.h" (
    echo ERROR: libusb not found at %LIBUSB_DIR%
    echo Please ensure libusb-1.0.29 is extracted at C:\AppInstall\dev\libusb-1.0.29
    exit /b 1
)

:: Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\bin mkdir %BUILD_DIR%\bin

echo Compiler: g++ (MinGW64)
echo Build Type: Release
echo.

:: Compiler flags
set CXXFLAGS=-std=c++20 -Wall -Wextra -O2
set INCLUDES=-I%LIBUSB_DIR%\include -I%SRC_DIR%
set LIBS=%LIBUSB_DIR%\MinGW64\static\libusb-1.0.a -lsetupapi -lole32 -ladvapi32 -lgdi32 -lshell32 -luser32

:: Source files
set SOURCES=%SRC_DIR%\main.cpp ^
            %SRC_DIR%\devices\razer_device.cpp ^
            %SRC_DIR%\devices\device_manager.cpp ^
            %SRC_DIR%\ui\cli_menu.cpp ^
            %SRC_DIR%\ui\tray_app.cpp

:: Output binary
set OUTPUT=%BUILD_DIR%\bin\razer-tray.exe

echo Compiling...
g++ %CXXFLAGS% %INCLUDES% %SOURCES% %LIBS% -o %OUTPUT%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo Build successful!
    echo ============================================
    echo Output: %OUTPUT%
    echo.
    echo Run with:
    echo   %OUTPUT% --help
    echo   %OUTPUT% --discover
    echo   %OUTPUT% --test-battery
    echo   %OUTPUT% --menu
    echo.
) else (
    echo.
    echo ============================================
    echo Build failed!
    echo ============================================
    exit /b 1
)

endlocal
