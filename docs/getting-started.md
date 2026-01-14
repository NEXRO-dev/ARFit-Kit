# Getting Started with ARFitKit

ARFitKitを使って、あなたのアプリにAR試着機能を追加しましょう。

## 📦 インストール

### iOS (Swift Package Manager)

1. Xcodeでプロジェクトを開く
2. **File → Add Package Dependencies...** を選択
3. 以下のURLを入力:
   ```
   https://github.com/NEXRO-dev/ARFit-Kit.git
   ```
4. バージョンを選択して **Add Package**

### Android (Gradle)

`settings.gradle.kts` に追加:
```kotlin
include(":arfitkit")
project(":arfitkit").projectDir = file("libs/ARfit-kit/platforms/android/arfitkit")
```

`app/build.gradle.kts` に依存関係を追加:
```kotlin
dependencies {
    implementation(project(":arfitkit"))
}

### React Native

```bash
npm install arfit-kit-react-native
cd ios && pod install
```

## 🚀 基本的な使い方

### Step 1: 初期化

```swift
// iOS
import ARFitKit

let arFitKit = ARFitKit()
try await arFitKit.initialize(config: SessionConfig(
    targetFPS: 60,
    enableClothSimulation: true
))
```

```kotlin
// Android
import com.arfitkit.ARFitKit

val arFitKit = ARFitKit(context)
arFitKit.initialize(SessionConfig(
    targetFPS = 60,
    enableClothSimulation = true
))
```

### Step 2: セッション開始

ARセッションを開始し、カメラフィードの取得を開始します。

```swift
// iOS - ARViewを渡す
arFitKit.startSession(view: arView)
```

```kotlin
// Android - SurfaceViewを渡す
arFitKit.startSession(surfaceView, lifecycleOwner)
```

### Step 3: 衣服を読み込み

2D画像から3D衣服モデルを生成します。

```swift
// iOS
let garment = try await arFitKit.loadGarment(
    image: UIImage(named: "tshirt")!,
    type: .tshirt
)
```

```kotlin
// Android
val garment = arFitKit.loadGarment(
    bitmap = BitmapFactory.decodeResource(resources, R.drawable.tshirt),
    type = GarmentType.TSHIRT
)
```

### Step 4: 試着

```swift
arFitKit.tryOn(garment: garment)
```

### Step 5: クリーンアップ

```swift
arFitKit.stopSession()
```

## 📸 スナップショット撮影

現在のAR画面をキャプチャ:

```swift
if let snapshot = arFitKit.captureSnapshot() {
    UIImageWriteToSavedPhotosAlbum(snapshot, nil, nil, nil)
}
```

## 🎯 パフォーマンスのヒント

1. **targetFPS**: デバイスに応じて30〜60を設定
2. **maxGarments**: 同時に表示する衣服は3枚まで推奨
3. **enableClothSimulation**: 低スペック端末ではfalseにして軽量化

## 🔧 トラブルシューティング

### "ARKit対応デバイスが必要です"
→ iPhone 6s以降、iOS 14以上が必要です

### "ARCore not available"
→ Google Play ServicesのARCoreがインストールされているか確認

### FPSが低い
→ `enableClothSimulation: false` を試す、または `targetFPS` を下げる
