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





セット 1：四半期売上レビュー ― パッセージ（段落分割版）
来週火曜日までに 四半期売上報告書の最終版 を共有ドライブへアップロードしてください。当日は午前 10 時から経営陣レビュー会議があり、佐藤さん が必ず参加しますので準備を万全にお願いします。さらに、今月30日18 時までに 請求書処理 を ERP システムへ入力・確定してください。

本日中に公開された 新しい顧客管理システムのオンライン研修 を受講し、完了証明を提出することが必須です。また、明後日9 時には倉庫で 販促資料 を受け取り、会議室 B へ搬入してください。

最後に、ネットワークアップグレードの影響で 本日21 時から 3 時間 社内 Wi-Fi が停止します。この間はオンライン作業ができないため、スケジュールを調整のうえ、市場調査アンケートの集計データ を Excel 形式で金曜日 17 時までに私へ送付するようお願いします。

セット 2：製品ローンチ準備 ― パッセージ（段落分割版）
今週金曜日までに 製品紹介スライドの最新版 をマーケティング共有フォルダへ保存してください。同じ日の午後 2 時にはメディア向けデモがあり、鈴木さん が登壇しますので、リハーサルを含む準備をお願いいたします。併せて、今月28日16 時までに 予算申請 を経理システムへ提出してください。

本日中に 新しいロゴガイドラインのオンライン講座 を視聴し、完了報告を提出することが必須です。明朝8 時には倉庫で デモ機 を受け取り、ホール C へ搬入してください。

加えて、電源工事のため 本日20 時から 1 時間 ショールームの照明が消灯します。作業計画に留意しつつ、顧客登録フォームのデータ を JSON 形式で木曜日 18 時までに私へ共有してください。

セット 3：年次カンファレンス準備 ― パッセージ（段落分割版）
来週水曜日までに 年次会議の議題案 を共有ドライブの “Conference25” フォルダへアップロードしてください。同日の午前 11 時には 基調講演リハーサル があり、高橋さん が必ず参加しますので対応をお願いします。また、今月27日15 時までに 旅費精算 を旅費システムへ登録してください。

本日中に 参加者管理アプリのチュートリアル動画 を視聴し、完了スクリーンショットを提出してください。さらに、明後日7 時に印刷会社で パンフレット を受け取り、ホール D へ搬入する手配をお願いします。

なお、クラウドメンテナンスのため 本日23 時から 2 時間 オンライン登録サイトが停止します。この時間帯を避けて作業を進めつつ、参加者アンケートの初期集計 を Google スプレッドシート形式で木曜日 20 時までに私へ送付してください。
