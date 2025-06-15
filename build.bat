@echo off
setlocal

echo ===================================
echo Tobii Eye Tracking ビルドスクリプト
echo ===================================
echo.

:: Visual Studioのバージョンを選択
echo Visual Studioのバージョンを選択してください:
echo 1. Visual Studio 2022
echo 2. Visual Studio 2019
echo 3. Visual Studio 2017
set /p VS_VERSION="番号を入力 (1-3): "

if "%VS_VERSION%"=="1" (
    set CMAKE_GENERATOR="Visual Studio 17 2022"
) else if "%VS_VERSION%"=="2" (
    set CMAKE_GENERATOR="Visual Studio 16 2019"
) else if "%VS_VERSION%"=="3" (
    set CMAKE_GENERATOR="Visual Studio 15 2017"
) else (
    echo エラー: 無効な選択です
    pause
    exit /b 1
)

:: アーキテクチャの選択
echo.
echo アーキテクチャを選択してください:
echo 1. x64 (64ビット) - 推奨
echo 2. x86 (32ビット)
set /p ARCH_CHOICE="番号を入力 (1-2): "

if "%ARCH_CHOICE%"=="1" (
    set CMAKE_ARCH=x64
) else if "%ARCH_CHOICE%"=="2" (
    set CMAKE_ARCH=Win32
) else (
    echo エラー: 無効な選択です
    pause
    exit /b 1
)

:: ビルドディレクトリの作成
if not exist build (
    mkdir build
)

cd build

:: CMakeの実行
echo.
echo CMakeを実行中...
cmake .. -G %CMAKE_GENERATOR% -A %CMAKE_ARCH%

if %errorlevel% neq 0 (
    echo.
    echo エラー: CMakeの実行に失敗しました
    echo Tobii SDKが正しく配置されているか確認してください
    pause
    exit /b 1
)

:: ビルドの実行
echo.
echo ビルドを実行中...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo エラー: ビルドに失敗しました
    pause
    exit /b 1
)

echo.
echo ===================================
echo ビルドが完了しました！
echo ===================================
echo.
echo 実行ファイルは以下の場所にあります:
echo.
echo ゲーム開発用:
echo   build\Release\tobii_stream_engine.exe
echo   build\Release\tobii_game_integration.exe
echo   build\Release\gaze_recorder.exe
echo.
echo 研究用（Tobii Pro SDK）:
echo   build\Release\tobii_pro_research.exe    - リアルタイムビューア
echo   build\Release\tobii_pro_recorder.exe    - 高性能レコーダー
echo.
pause 