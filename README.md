# ARFit-Kit

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-iOS%20%7C%20Android%20%7C%20Web-blue.svg)]()

**オープンソースのクロスプラットフォームAR試着SDK**

2D衣服画像から3Dモデルを生成し、リアルタイムで体の動きに追従するAR試着を実現します。

## ✨ 特徴

- 🎯 **リアルタイムボディトラッキング** - MediaPipe/ARKit/ARCoreベースの高精度ポーズ推定
- 👗 **2D→3D変換** - 衣服画像のシルエットから3Dメッシュを自動生成
- 🌊 **Position Based Dynamics** - エッジ制約＋球体衝突でリアルな布シミュレーション
- 🎨 **テクスチャサンプリング** - 元の衣服画像をそのまま3Dメッシュに投影
- 💡 **リアルタイムライティング** - ランバート反射＋アルファブレンディング
- 📱 **クロスプラットフォーム** - iOS, Android, React Native対応
- ⚡ **60fps** - 低遅延のリアルタイム処理

## 🚀 クイックスタート

### iOS (Swift)

```swift
import ARFitKit

let arFitKit = ARFitKit()
try arFitKit.initialize()

// ARセッション開始
try await arFitKit.startSession(view: arView)

// 衣服を読み込んで試着
let garment = try await arFitKit.loadGarment(image: clothingImage)
try arFitKit.tryOn(garment: garment)

// 削除
arFitKit.removeGarment(garment: garment)
arFitKit.removeAllGarments()
```

### Android (Kotlin)

```kotlin
import com.arfitkit.ARFitKit
import com.arfitkit.SessionConfig

val arFitKit = ARFitKit(context)
arFitKit.initialize(SessionConfig(targetFPS = 60))

// ARセッション開始
arFitKit.startSession(surfaceView, lifecycleOwner)

// 衣服を読み込んで試着
val garment = arFitKit.loadGarment(clothingBitmap, GarmentType.TSHIRT)
arFitKit.tryOn(garment)

// フレーム処理のコールバック
arFitKit.onFrameProcessed = { bitmap ->
    imageView.setImageBitmap(bitmap) // 結果の表示
}
```

### React Native

```tsx
import ARFitKit, { ARFitKitView, GarmentType } from 'arfit-kit-react-native';

// 初期化
await ARFitKit.initialize({ targetFPS: 60 });
await ARFitKit.startSession();

// 衣服を読み込んで試着
const garment = await ARFitKit.loadGarment(imageUri, GarmentType.TSHIRT);
await ARFitKit.tryOn(garment.id);

// ネイティブビューの表示
<ARFitKitView style={{ flex: 1 }} />

// イベントリスナー
ARFitKit.onPoseUpdated((pose) => console.log(pose));
ARFitKit.onFPSUpdated((fps) => console.log(`FPS: ${fps}`));
```

## 📦 インストール

### iOS (Swift Package Manager)
Xcodeで「Add Package Dependencies」から以下のURLを追加:
```
https://github.com/NEXRO-dev/ARFit-Kit.git
```

### Android (Gradle)
`settings.gradle.kts` に以下を追加してサブモジュールとして利用:
```kotlin
include(":arfitkit")
project(":arfitkit").projectDir = file("path/to/ARFit-Kit/platforms/android/arfitkit")
```

### React Native
```bash
npm install arfit-kit-react-native
cd ios && pod install
```

## 🏗️ アーキテクチャ

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
├─────────────┬─────────────┬──────────────────────────────────┤
│   iOS SDK   │ Android SDK │       React Native SDK           │
│   (Swift)   │  (Kotlin)   │    (TypeScript + Native)         │
├─────────────┴─────────────┴──────────────────────────────────┤
│           Platform Bridge (Obj-C++ / JNI)                    │
├──────────────────────────────────────────────────────────────┤
│                     Core Engine (C++)                         │
├──────────────┬──────────────┬──────────────┬─────────────────┤
│    Body      │   Garment    │   Physics    │      AR         │
│   Tracker    │  Converter   │   Engine     │   Renderer      │
│ (MediaPipe)  │ (シルエット   │  (PBD/衝突)  │ (Zバッファ/     │
│              │  変形+リグ)  │              │  テクスチャ)     │
├──────────────┴──────────────┴──────────────┴─────────────────┤
│     OpenCV  │  Texture Sampling  │  SMPL Model Fitting      │
└──────────────────────────────────────────────────────────────┘
```

### コアエンジンの処理フロー

```
カメラフレーム → ボディトラッキング → スケルトン検出
                                         ↓
2D衣服画像 → シルエット変形 → 3Dメッシュ生成 → 物理シミュレーション
                                                    ↓
                                         衝突判定 (体 vs 布)
                                                    ↓
                                         テクスチャサンプリング
                                                    ↓
                                         Zバッファ描画 + ライティング
                                                    ↓
                                              合成フレーム出力
```

## 📋 要件

| Platform | Minimum Version | Requirements |
|----------|-----------------|--------------|
| iOS | 14.0+ | ARKit対応デバイス (A12+) |
| Android | API 24+ | ARCore対応デバイス |

## 🔧 コアモジュール

| モジュール | ファイル | 概要 |
|-----------|---------|------|
| **Body Tracker** | `body_tracker.cpp` | スケルトン検出＋スムージング＋SMPL推定 |
| **Garment Converter** | `garment_converter.cpp` | 2D画像→3Dメッシュ変換（シルエット適合＋リギング） |
| **Physics Engine** | `physics_engine.cpp` | PBD布シミュレーション（エッジ制約＋球体衝突） |
| **AR Renderer** | `ar_renderer.cpp` | ソフトウェアラスタライザ（Zバッファ＋テクスチャ＋ライティング） |
| **Texture** | `texture.cpp` | UV座標からのピクセルサンプリング |
| **Mesh** | `mesh.cpp` | テンプレートメッシュ生成（Tシャツ等） |

## 📄 ライセンス

MIT License - 詳細は [LICENSE](LICENSE) をご覧ください。

## 🙏 謝辞

- [MediaPipe](https://mediapipe.dev/) - ポーズ推定
- [SMPL](https://smpl.is.tue.mpg.de/) - 人体モデル
- [OpenCV](https://opencv.org/) - 画像処理
- [ARKit](https://developer.apple.com/arkit/) - iOS AR
- [ARCore](https://developers.google.com/ar) - Android AR
