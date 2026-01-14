# ARfit-kit Architecture

## システム概要

ARfit-kitは、クロスプラットフォームで動作するAR試着SDKです。

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
├─────────────┬─────────────┬──────────────┬──────────────────┤
│   iOS SDK   │ Android SDK │   Web SDK    │  React Native    │
│   (Swift)   │  (Kotlin)   │ (TypeScript) │  (JS Bridge)     │
├─────────────┴─────────────┴──────────────┴──────────────────┤
│                     Core Engine (C++)                        │
├──────────────┬──────────────┬──────────────┬────────────────┤
│    Body      │   Garment    │   Physics    │      AR        │
│   Tracker    │  Converter   │   Engine     │   Renderer     │
│ (MediaPipe)  │  (2D→3D ML)  │   (PBD)      │ (Metal/Vulkan) │
└──────────────┴──────────────┴──────────────┴────────────────┘
```

## コアモジュール

### 1. Body Tracker
- **MediaPipe Pose** を使用した33点のボディランドマーク検出
- **SMPL/SMPL-X** モデルへのフィッティング
- 3Dボディメッシュ生成

### 2. Garment Converter
- 2D画像からの衣服セグメンテーション
- ハイブリッド2D→3D変換（オンデバイス + サーバー）
- UV マッピング自動生成

### 3. Physics Engine
- **Position Based Dynamics (PBD)** による布シミュレーション
- body-cloth 衝突検出
- GPU アクセラレーション（Metal/Vulkan/WebGPU）

### 4. AR Renderer
- リアルタイムARオーバーレイ
- ライティング・シャドウ
- デプスオクルージョン

## データフロー

```
カメラ入力 → Body Tracking → Physics Sim → Rendering → 出力
     ↓                            ↑
衣服画像  → 2D→3D変換 ─────────────┘
```

## GPU バックエンド

| Platform | Primary | Fallback |
|----------|---------|----------|
| iOS | Metal | - |
| Android | Vulkan | OpenGL ES |
| Web | WebGPU | WebGL2 |
