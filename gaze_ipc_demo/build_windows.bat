@echo off
echo Gaze IPC Demo - Windows Build Script
echo =====================================
echo.

REM Check if vcpkg is available
if not exist "C:\vcpkg" (
    echo ERROR: vcpkg not found at C:\vcpkg
    echo Please install vcpkg first:
    echo   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
    echo   cd C:\vcpkg
    echo   .\bootstrap-vcpkg.bat
    echo   .\vcpkg integrate install
    echo   .\vcpkg install glfw3:x64-windows
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

echo Configuring CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -A x64

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release

if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Executables created:
if exist "bin\Release\producer.exe" echo   - bin\Release\producer.exe
if exist "bin\Release\consumer.exe" echo   - bin\Release\consumer.exe
echo.
echo To run the demo:
echo   1. Start producer.exe in one terminal
echo   2. Start consumer.exe in another terminal
echo.
pause 