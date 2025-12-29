@echo off
REM Build script for Razer Battery Tray (C++)

echo Building Razer Tray...
echo.

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Configure with CMake
echo [1/2] Configuring with CMake...
cd build
cmake .. -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build with MinGW
echo.
echo [2/2] Building with MinGW...
mingw32-make
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
echo Executable: build\bin\RazerTray.exe

REM Show file size
for %%F in (bin\RazerTray.exe) do (
    set size=%%~zF
    set /a sizekb=%%~zF / 1024
)
echo Size: %sizekb% KB
echo.
echo To run: build\bin\RazerTray.exe
echo.

cd ..
