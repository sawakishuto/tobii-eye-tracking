@echo off
echo Eye Tracking Reading Experiment - Set Selection
echo.
echo Select which set to run:
echo 1. Set 1 - エレベーター工事
echo 2. Set 2 - 配管工事
echo 3. Set 3 - 駐車場工事
echo 4. Exit
echo.
set /p choice="Enter your choice (1-4): "

if "%choice%"=="1" (
    echo Running Set 1...
    build\Release\tobii_pro_sequence_set1.exe
) else if "%choice%"=="2" (
    echo Running Set 2...
    build\Release\tobii_pro_sequence_set2.exe
) else if "%choice%"=="3" (
    echo Running Set 3...
    build\Release\tobii_pro_sequence_set3.exe
) else if "%choice%"=="4" (
    echo Exiting...
    exit /b 0
) else (
    echo Invalid choice. Please select 1-4.
    pause
    goto :eof
)

echo.
echo Set completed. Press any key to return to menu...
pause > nul
cls
goto :eof 