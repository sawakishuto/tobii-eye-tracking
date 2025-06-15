# Tobii Eye Tracking C++ Demo

Tobiiのアイトラッカーを使用して視線情報を取得するC++プログラムです。

## 概要

このプロジェクトでは、Tobiiアイトラッカーから以下の情報を取得できます：
- 視線位置（X, Y座標）- 左右の目の個別データ
- 頭部姿勢（ヨー、ピッチ、ロール）
- 頭部位置（X, Y, Z）
- 3D眼球位置（研究用）
- 瞳孔径（研究用）
- ユーザーの存在検出

3つのSDKに対応：
1. **Stream Engine API** - 低レベルAPIで、より細かい制御が可能
2. **Game Integration API** - ゲーム開発向けの高レベルAPI
3. **Tobii Pro SDK** - 研究用の高精度・低レイテンシAPI（研究者向け）

## 必要なもの

- Windows 10/11（64ビット推奨）
- Tobiiアイトラッカー
  - ゲーム用：Tobii Eye Tracker 5, 4C, EyeX
  - 研究用：Tobii Pro Spectrum（最大1200Hz）, Pro Fusion（最大250Hz）, Pro Spark, Pro Nano
- Visual Studio 2019以降（C++17対応）
- CMake 3.10以降
- Tobii SDK（用途に応じて選択）
  - Stream Engine API - 汎用
  - Game Integration API - ゲーム開発
  - Tobii Pro SDK - 研究用（要ライセンス）

## セットアップ

### 1. Tobii SDKのダウンロード

[Tobii Developer Zone](https://developer.tobii.com/)から必要なSDKをダウンロードします：

- **Stream Engine API**: より低レベルな制御が必要な場合
- **Game Integration API**: ゲーム開発やより簡単な実装を求める場合
- **Tobii Pro SDK**: 研究用途（[Tobii Connect](https://connect.tobii.com/)でアカウント作成が必要）

### 2. SDKの配置

ダウンロードしたSDKを以下の構造でプロジェクトに配置します：

```
tobii-eye-tracking/
├── tobii_sdk/
│   ├── stream_engine/
│   │   ├── include/
│   │   │   └── tobii/
│   │   │       ├── tobii.h
│   │   │       └── tobii_streams.h
│   │   └── lib/
│   │       └── tobii/
│   │           ├── x64/
│   │           │   ├── tobii_stream_engine.lib
│   │           │   └── tobii_stream_engine.dll
│   │           └── x86/
│   ├── game_integration/
│   │   ├── include/
│   │   │   └── tobii_gameintegration.h
│   │   └── lib/
│   │       ├── x64/
│   │       │   ├── tobii_gameintegration.lib
│   │       │   └── tobii_gameintegration.dll
│   │       └── x86/
│   └── pro_sdk/                    # 研究用SDK
│       ├── include/
│       │   ├── tobii_research.h
│       │   ├── tobii_research_streams.h
│       │   └── tobii_research_eyetracker.h
│       └── lib/
│           ├── x64/
│           │   ├── tobii_research.lib
│           │   └── tobii_research.dll
│           └── x86/
├── src/
│   ├── main.cpp                    # Stream Engine APIの実装
│   ├── main_game_integration.cpp   # Game Integration APIの実装
│   ├── gaze_recorder.cpp           # データ記録（Game Integration API）
│   ├── tobii_pro_research.cpp      # Pro SDK リアルタイムビューア
│   └── tobii_pro_recorder.cpp      # Pro SDK 高性能レコーダー
└── CMakeLists.txt
```

### 3. ビルド

PowerShellまたはコマンドプロンプトで以下のコマンドを実行：

```powershell
# ビルドディレクトリの作成
mkdir build
cd build

# CMakeの実行（Visual Studio 2022の場合）
cmake .. -G "Visual Studio 17 2022" -A x64

# ビルド
cmake --build . --config Release
```

または、Visual Studioで`build/TobiiEyeTracking.sln`を開いてビルドします。

## 使い方

### Stream Engine版

```powershell
cd build/Release
./tobii_stream_engine.exe
```

このプログラムは、視線位置を継続的にコンソールに出力します。

### Game Integration版

```powershell
cd build/Release
./tobii_game_integration.exe
```

このプログラムは、視線位置に加えて頭部姿勢と位置も表示します。ESCキーで終了できます。

### 研究用 - Tobii Pro SDK版

#### リアルタイムビューア（低レイテンシ）
```powershell
cd build/Release
./tobii_pro_research.exe
```

- 左右の目の個別データ
- 3D眼球位置と瞳孔径
- レイテンシ測定
- 最大サンプリング周波数で動作

#### 高性能データレコーダー
```powershell
cd build/Release
./tobii_pro_recorder.exe
```

- 参加者ID入力機能
- ダブルバッファリングで低レイテンシ記録
- 高精度タイムスタンプ
- CSVファイルに全生データを保存

## トラブルシューティング

### "アイトラッカーが見つかりません"エラー

1. Tobiiアイトラッカーが正しく接続されているか確認
2. Tobii Eye Tracking Core SoftwareまたはTobii Experienceがインストールされているか確認
3. デバイスマネージャーでTobiiデバイスが認識されているか確認

### DLLが見つからないエラー

実行ファイルと同じディレクトリに以下のDLLが存在することを確認：
- `tobii_stream_engine.dll`（Stream Engine版）
- `tobii_gameintegration.dll`（Game Integration版）

### ビルドエラー

- Tobii SDKのパスが正しく設定されているか確認
- Visual StudioでC++開発環境がインストールされているか確認

## プログラムの説明

### Stream Engine API版（main.cpp）

低レベルAPIを使用した実装で、以下の特徴があります：
- コールバックベースの視線データ取得
- より細かい制御が可能
- 最小限の依存関係

### Game Integration API版（main_game_integration.cpp）

高レベルAPIを使用した実装で、以下の特徴があります：
- ポーリングベースのデータ取得
- 頭部トラッキングデータも取得可能
- ゲーム開発に適した設計

## ライセンス

このサンプルコードはMITライセンスで提供されています。
Tobii SDKの使用については、Tobiiのライセンス条項に従ってください。

## 参考リンク

- [Tobii Developer Zone](https://developer.tobii.com/)
- [Tobii Gaming](https://gaming.tobii.com/)
- [Tobii SDK Documentation](https://developer.tobii.com/product-integration/)
