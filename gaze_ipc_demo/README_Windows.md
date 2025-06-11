# Gaze IPC Demo - Windows ビルド手順

## 必要な環境

### 1. Visual Studio または Build Tools

- **Visual Studio 2019/2022** (Community Edition 可)
- または **Visual Studio Build Tools 2019/2022**

### 2. CMake

- **CMake 3.20 以上**
- [https://cmake.org/download/](https://cmake.org/download/) からダウンロード

### 3. vcpkg (パッケージマネージャー)

```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 4. 依存ライブラリのインストール

```cmd
# GLFW のインストール
.\vcpkg install glfw3:x64-windows

# OpenGL は Windows に標準搭載
```

### 5. Tobii Research SDK (オプション)

- [Tobii Developer Portal](https://developer.tobii.com/product-integration/stream-engine/) からダウンロード
- `C:\Program Files\Tobii\Tobii Pro SDK` にインストール

## ビルド手順

### 方法 1: Visual Studio GUI

1. **プロジェクトフォルダを開く**

   ```cmd
   cd gaze_ipc_demo
   ```

2. **Visual Studio でフォルダを開く**

   - Visual Studio を起動
   - "フォルダーを開く" → `gaze_ipc_demo` フォルダを選択

3. **CMake 設定**

   - CMakeSettings.json が自動生成される
   - vcpkg toolchain を設定：
     ```json
     {
       "configurations": [
         {
           "name": "x64-Debug",
           "generator": "Ninja",
           "configurationType": "Debug",
           "buildRoot": "${projectDir}\\out\\build\\${name}",
           "installRoot": "${projectDir}\\out\\install\\${name}",
           "cmakeCommandArgs": "",
           "buildCommandArgs": "",
           "ctestCommandArgs": "",
           "cmakeToolchain": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
         }
       ]
     }
     ```

4. **ビルド実行**
   - `Ctrl+Shift+B` でビルド実行

### 方法 2: コマンドライン

```cmd
# ビルドディレクトリ作成
mkdir build
cd build

# CMake設定 (vcpkg使用)
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -A x64

# ビルド実行
cmake --build . --config Release

# または Debug ビルド
cmake --build . --config Debug
```

### 方法 3: 開発者コマンドプロンプト

```cmd
# Visual Studio Developer Command Prompt を開く
# (スタートメニュー → Visual Studio → Developer Command Prompt)

cd gaze_ipc_demo
mkdir build && cd build

# CMake設定
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# MSBuild でビルド
msbuild gaze_ipc_demo.sln /p:Configuration=Release
```

## 実行方法

### アイトラッカーありの場合

```cmd
# ターミナル1: プロデューサー起動
cd build\bin\Release  # または Debug
producer.exe

# ターミナル2: コンシューマー起動
consumer.exe
```

### シミュレーションモード

アイトラッカーが検出されない場合、自動的にシミュレーションモードになります。

## トラブルシューティング

### よくあるエラー

1. **"GLFW not found"**

   ```cmd
   .\vcpkg install glfw3:x64-windows
   ```

2. **"Tobii SDK not found"**

   - SDK パスを確認：`C:\Program Files\Tobii\Tobii Pro SDK`
   - またはカスタムパスを指定：
     ```cmd
     cmake .. -DTOBII_SDK_PATH="D:/YourPath/TobiiSDK"
     ```

3. **DLL not found エラー**
   - 実行ファイルと同じディレクトリに必要な DLL を配置
   - `tobii_research.dll`
   - `glfw3.dll` (動的リンクの場合)

### Visual Studio 版別の注意点

- **Visual Studio 2019**: `-G "Visual Studio 16 2019"`
- **Visual Studio 2022**: `-G "Visual Studio 17 2022"`

### 64bit/32bit

デフォルトは 64bit (`-A x64`) ですが、32bit 版が必要な場合：

```cmd
cmake .. -A Win32 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## パフォーマンス最適化

### Release ビルド推奨

```cmd
cmake --build . --config Release
```

### マルチコアビルド

```cmd
cmake --build . --config Release -- /m
```
