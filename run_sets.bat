@echo off
echo Eye Tracking Reading Experiment - Set Selection
echo.
echo Select which set to run:
echo 1. Set A - データ分析研修レポート
echo 2. Set B - 業務報告改善プロジェクト
echo 3. Set C - 入札不正問題への対応
echo 4. Set D - 製造ライン機械故障の対応
echo 5. 統合版 - すべてのレポート
echo 6. Exit
echo.
set /p choice="Enter your choice (1-6): "

if "%choice%"=="1" (
    echo Running Set A...
    build\Release\tobii_pro_sequence_set1.exe
) else if "%choice%"=="2" (
    echo Running Set B...
    build\Release\tobii_pro_sequence_set2.exe
) else if "%choice%"=="3" (
    echo Running Set C...
    build\Release\tobii_pro_sequence_set3.exe
) else if "%choice%"=="4" (
    echo Running Set D...
    build\Release\tobii_pro_sequence_set4.exe
) else if "%choice%"=="5" (
    echo Running Integrated Version...
    build\Release\tobii_pro_sequence_all.exe
) else if "%choice%"=="6" (
    echo Exiting...
    exit /b 0
) else (
    echo Invalid choice. Please select 1-6.
    pause
    goto :eof
)

echo.
echo Set completed. Press any key to return to menu...
pause > nul
cls
goto :eof 