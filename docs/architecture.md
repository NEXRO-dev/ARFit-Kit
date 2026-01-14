# ARFitKit Architecture

## システム概要

ARFitKitは、クロスプラットフォームAR試着を実現するための4層アーキテクチャで構成されています。

```
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                          │
│              (Your App / Demo App)                          │
├─────────────┬─────────────┬──────────────┬──────────────────┤
│   iOS SDK   │ Android SDK │   Web SDK    │  React Native    │
│   (Swift)   │  (Kotlin)   │ (TypeScript) │    (RN Bridge)   │
├─────────────┴─────────────┴──────────────┴──────────────────┤
│                     Core Engine (C++)                        │
├──────────────┬──────────────┬──────────────┬────────────────┤
│    Body      │   Garment    │   Physics    │      AR        │
│   Tracker    │  Converter   │   Engine     │   Renderer     │
├──────────────┴──────────────┴──────────────┴────────────────┤
│                    GPU & ML Backend                          │
│    Metal (iOS) │ Vulkan (Android) │ WebGPU (Web)            │
│              TensorFlow Lite │ ONNX Runtime                 │
└─────────────────────────────────────────────────────────────┘
```

## コアモジュール

### 1. BodyTracker (body_tracker.cpp)

MediaPipeを使用してカメラ映像から人体のポーズを推定します。

**処理フロー:**
1. カメラフレーム取得
2. MediaPipe Pose推論
3. 33個のランドマーク抽出
4. SMPL体型パラメータ推定
5. 3Dボディメッシュ生成

### 2. GarmentConverter (garment_converter.cpp)

2D衣服画像を3Dメッシュに変換します。

**処理フロー:**
1. 衣服画像入力
2. セグメンテーション（背景除去）
3. 衣服タイプ自動検出
4. テンプレートメッシュ選択
5. UV座標マッピング
6. 3Dメッシュ出力

### 3. PhysicsEngine (physics_engine.cpp)

Position Based Dynamics (PBD) による布シミュレーション。

**シミュレーションステップ:**
1. 外力適用（重力、風）
2. 位置予測
3. 距離拘束解決
4. 衝突判定・解決
5. 速度更新

**GPUカーネル:**
- `applyForces` - 外力計算
- `solveConstraints` - 拘束解決
- `solveCollisions` - 衝突処理
- `updateVelocities` - 速度更新

### 4. ARRenderer (ar_renderer.cpp)

最終的なAR合成レンダリング。

**レンダリングパイプライン:**
1. 背景（カメラ映像）描画
2. 深度バッファ初期化
3. ボディメッシュ描画（オクルージョン用）
4. 衣服メッシュ描画
5. 影・ライティング適用
6. ポストプロセス

## GPUバックエンド

### プラットフォーム別実装

| Platform | Backend | Shader Language |
|----------|---------|-----------------|
| iOS/macOS | Metal | MSL (.metal) |
| Android | Vulkan | GLSL (.comp) |
| Web | WebGPU | WGSL (.wgsl) |

### 共通インターフェース

```cpp
class IGPUContext {
    virtual bool initialize() = 0;
    virtual shared_ptr<IGPUBuffer> createBuffer(size_t size) = 0;
    virtual shared_ptr<IGPUShader> createShader(string source) = 0;
    virtual void dispatch(shader, x, y, z, bindings) = 0;
};
```

## データフロー

```
Camera Frame
    │
    ▼
┌─────────────┐
│ BodyTracker │ → BodyPose (33 landmarks)
└─────────────┘
    │
    ▼
┌───────────────┐
│ PhysicsEngine │ → Updated cloth mesh vertices
└───────────────┘
    │
    ▼
┌────────────┐
│ ARRenderer │ → Final composited frame
└────────────┘
    │
    ▼
Display
```

## パフォーマンス目標

| Metric | Target |
|--------|--------|
| Frame Rate | 60 FPS |
| Latency | < 50ms |
| Body Tracking | < 10ms |
| Physics Step | < 5ms |
| Render | < 10ms |
