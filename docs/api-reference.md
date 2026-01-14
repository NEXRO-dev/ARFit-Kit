# ARFitKit API Reference

## Core Classes

### ARFitKit

メインのSDKクラス。すべての機能へのエントリーポイント。

#### Methods

| Method | Description |
|--------|-------------|
| `initialize(config)` | SDKを初期化 |
| `startSession(view)` | ARセッションを開始 |
| `stopSession()` | ARセッションを停止 |
| `loadGarment(image, type)` | 衣服を読み込み |
| `tryOn(garment)` | 衣服を試着 |
| `removeGarment(garment)` | 衣服を削除 |
| `removeAllGarments()` | すべての衣服を削除 |
| `captureSnapshot()` | スナップショットを撮影 |

---

### SessionConfig

セッション設定。

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `targetFPS` | Int | 60 | ターゲットフレームレート |
| `enableClothSimulation` | Bool | true | 布シミュレーションの有効化 |
| `enableShadows` | Bool | true | 影の描画 |
| `maxGarments` | Int | 3 | 同時表示可能な衣服数 |

---

### Garment

衣服データ。

| Property | Type | Description |
|----------|------|-------------|
| `id` | String | 一意のID |
| `type` | GarmentType | 衣服の種類 |
| `image` | Image | 元画像 |

---

### GarmentType

衣服の種類を表すenum。

| Value | Description |
|-------|-------------|
| `.unknown` | 不明（自動検出） |
| `.tshirt` | Tシャツ |
| `.shirt` | シャツ |
| `.jacket` | ジャケット |
| `.coat` | コート |
| `.dress` | ドレス |
| `.pants` | パンツ |
| `.shorts` | ショートパンツ |
| `.skirt` | スカート |

---

### BodyPose

ボディポーズデータ。

| Property | Type | Description |
|----------|------|-------------|
| `landmarks` | [Point3D] | 33個のランドマーク座標 |
| `visibility` | [Float] | 各ランドマークの可視性 |
| `confidence` | Float | 全体の信頼度 |

---

## Events / Callbacks

### iOS

```swift
arFitKit.onPoseUpdated = { pose in
    print("Pose confidence: \(pose.confidence)")
}

arFitKit.onFPSUpdated = { fps in
    print("Current FPS: \(fps)")
}

arFitKit.onError = { error in
    print("Error: \(error)")
}
```

### Android

```kotlin
arFitKit.currentPose.collect { pose ->
    // Handle pose
}

arFitKit.currentFPS.collect { fps ->
    // Handle FPS
}
```

### React Native

```typescript
ARFitKit.onPoseUpdated((pose) => {
    console.log('Pose:', pose);
});

ARFitKit.onFPSUpdated((fps) => {
    console.log('FPS:', fps);
});

ARFitKit.onError((error) => {
    console.error('Error:', error);
});
```

---

## Error Codes

| Code | Description |
|------|-------------|
| `INITIALIZATION_FAILED` | 初期化失敗 |
| `CAMERA_ACCESS_DENIED` | カメラアクセス拒否 |
| `GPU_NOT_AVAILABLE` | GPU利用不可 |
| `MODEL_LOAD_FAILED` | モデル読み込み失敗 |
| `GARMENT_CONVERSION_FAILED` | 衣服変換失敗 |
| `SESSION_NOT_STARTED` | セッション未開始 |
| `ARCORE_NOT_AVAILABLE` | ARCore利用不可 |
