# ARfit-kit

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-iOS%20%7C%20Android%20%7C%20Web-blue.svg)]()

**オープンソースのクロスプラットフォームAR試着SDK**

2D衣服画像から3Dモデルを生成し、リアルタイムで体の動きに追従するAR試着を実現します。

## ✨ 特徴

- 🎯 **リアルタイムボディトラッキング** - MediaPipeベースの高精度ポーズ推定
- 👗 **2D→3D変換** - 衣服画像から3Dメッシュを自動生成
- 🌊 **高精度布シミュレーション** - リアルなシワと動きを再現
- 📱 **クロスプラットフォーム** - iOS, Android, Web対応
- ⚡ **60fps** - 低遅延のリアルタイム処理

## 🚀 クイックスタート

### iOS (Swift)

```swift
import ARFitKit

let arFitKit = ARFitKit()
arFitKit.startSession(view: arView)

let garment = try await arFitKit.loadGarment(image: clothingImage)
arFitKit.tryOn(garment: garment)
```

### Android (Kotlin)

```kotlin
import com.arfitkit.ARFitKit

val arFitKit = ARFitKit(context)
arFitKit.startSession(surfaceView)

val garment = arFitKit.loadGarment(clothingBitmap)
arFitKit.tryOn(garment)
```

### Web (TypeScript)

```typescript
import { ARFitKit } from 'arfit-kit';

const arFitKit = new ARFitKit();
await arFitKit.startSession(canvas);

const garment = await arFitKit.loadGarment(imageUrl);
arFitKit.tryOn(garment);
```

### React Native

```tsx
import ARFitKit, { GarmentType } from 'arfit-kit-react-native';

// Initialize
await ARFitKit.initialize({ targetFPS: 60 });
await ARFitKit.startSession();

// Load and try on
const garment = await ARFitKit.loadGarment(imageUri, GarmentType.TSHIRT);
await ARFitKit.tryOn(garment.id);

// Listen to events
ARFitKit.onPoseUpdated((pose) => console.log(pose));
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
project(":arfitkit").projectDir = file("path/to/ARfit-kit/platforms/android/arfitkit")
```

### Web (npm)
```bash
npm install arfit-kit
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
├─────────────┬─────────────┬──────────────┬──────────────────┤
│   iOS SDK   │ Android SDK │   Web SDK    │  React Native    │
│   (Swift)   │  (Kotlin)   │ (TypeScript) │                  │
├─────────────┴─────────────┴──────────────┴──────────────────┤
│                     Core Engine (C++)                        │
├──────────────┬──────────────┬──────────────┬────────────────┤
│    Body      │   Garment    │   Physics    │      AR        │
│   Tracker    │  Converter   │   Engine     │   Renderer     │
├──────────────┴──────────────┴──────────────┴────────────────┤
│                    ML Models & GPU Compute                   │
│         (MediaPipe, ONNX, Metal/Vulkan/WebGPU)              │
└─────────────────────────────────────────────────────────────┘
```

## 📋 要件

| Platform | Minimum Version | Requirements |
|----------|-----------------|--------------|
| iOS | 14.0+ | ARKit対応デバイス |
| Android | API 24+ | ARCore対応デバイス |
| Web | Chrome 94+ | WebGPU/WebGL2対応ブラウザ |

## 📄 ライセンス

MIT License - 詳細は [LICENSE](LICENSE) をご覧ください。

## 🙏 謝辞

- [MediaPipe](https://mediapipe.dev/) - ポーズ推定
- [SMPL](https://smpl.is.tue.mpg.de/) - 人体モデル
