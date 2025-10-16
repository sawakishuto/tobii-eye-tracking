@echo off
echo Building all four set-specific executables...

REM Clean previous build
if exist build rmdir /s /q build
mkdir build
cd build

REM Configure and build
cmake .. -G "Visual Studio 16 2019" -A x64
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release --target tobii_pro_sequence_set1
if %errorlevel% neq 0 (
    echo Build failed for set 1!
    pause
    exit /b 1
)

cmake --build . --config Release --target tobii_pro_sequence_set2
if %errorlevel% neq 0 (
    echo Build failed for set 2!
    pause
    exit /b 1
)

cmake --build . --config Release --target tobii_pro_sequence_set3
if %errorlevel% neq 0 (
    echo Build failed for set 3!
    pause
    exit /b 1
)

cmake --build . --config Release --target tobii_pro_sequence_set4
if %errorlevel% neq 0 (
    echo Build failed for set 4!
    pause
    exit /b 1
)

cmake --build . --config Release --target tobii_pro_sequence_all
if %errorlevel% neq 0 (
    echo Build failed for integrated version!
    pause
    exit /b 1
)

echo All builds completed successfully!
echo.
echo Executables are located in:
echo - build\Release\tobii_pro_sequence_set1.exe
echo - build\Release\tobii_pro_sequence_set2.exe
echo - build\Release\tobii_pro_sequence_set3.exe
echo - build\Release\tobii_pro_sequence_set4.exe
echo - build\Release\tobii_pro_sequence_all.exe (統合版)
echo.
pause 