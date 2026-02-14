/**
 * @file body_tracker.cpp
 * @brief ボディトラッキング実装
 *
 * MediaPipe/ARKitから取得した関節座標を処理し、
 * SMPLモデルへのフィッティングと衝突判定用のボディメッシュを生成します。
 */

#include "body_tracker.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace arfit {

const int MEDIA_PIPE_LANDMARKS = 33;

class BodyTracker::Impl {
public:
  BodyTrackerConfig config;
  bool initialized = false;

  // SMPLテンプレート頂点（初期Tポーズ）
  std::vector<Point3D> smplTemplate;
  
  // 前フレームのランドマーク（スムージング用）
  std::array<Point3D, 33> prevLandmarks;
  bool hasPrevFrame = false;
  
  // スムージング係数（0.0=前フレームのみ, 1.0=現在フレームのみ）
  float smoothingFactor = 0.6f;

  Impl() { initializeSMPLTemplate(); }

  void initializeSMPLTemplate() {
    // SMPLの基本テンプレート (6890頂点) を初期化
    // 実際の製品ではSMPLモデルファイル(.pkl)をロードする
    smplTemplate.resize(6890);
    for (auto &v : smplTemplate) {
      v = {0.0f, 0.0f, 0.0f};
    }
  }

  float distance(const Point3D &a, const Point3D &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }
  
  /**
   * ランドマークのスムージング（ジッター軽減）
   */
  Point3D smoothLandmark(const Point3D& current, const Point3D& prev) {
    return {
        prev.x + (current.x - prev.x) * smoothingFactor,
        prev.y + (current.y - prev.y) * smoothingFactor,
        prev.z + (current.z - prev.z) * smoothingFactor
    };
  }
};

BodyTracker::BodyTracker() : pImpl(std::make_unique<Impl>()) {}
BodyTracker::~BodyTracker() = default;

Result<void> BodyTracker::initialize(const BodyTrackerConfig &config) {
  pImpl->config = config;
  pImpl->initialized = true;
  return {.error = ErrorCode::SUCCESS};
}

/**
 * フレームを処理してボディトラッキング結果を返す
 */
Result<BodyTrackingResult> BodyTracker::processFrame(const CameraFrame &frame) {
  if (!pImpl->initialized) {
    return {.error = ErrorCode::INITIALIZATION_FAILED,
            .message = "Body tracker not initialized"};
  }

  BodyTrackingResult result;
  auto startTime = std::chrono::steady_clock::now();

  // 画像の前処理（RGBAからRGBへ変換）
  cv::Mat cvImage(frame.image.height, frame.image.width, CV_8UC4,
                  const_cast<uint8_t *>(frame.image.pixels.data()));
  cv::Mat rgbImage;
  if (frame.image.channels == 4) {
    cv::cvtColor(cvImage, rgbImage, cv::COLOR_RGBA2RGB);
  } else {
    rgbImage = cvImage;
  }

  // ※ 実機では ARKit(iOS) / ARCore(Android) / MediaPipe からの
  //   スケルトンデータを直接受け取るため、TFLite推論は不要。
  //   ここではデモ用にシミュレーションデータを生成する。

  float time = std::chrono::duration<float>(startTime.time_since_epoch()).count();
  float sway = std::sin(time * 2.0f) * 0.05f;

  // 主要ランドマークの配置（正規化座標: 画面の中央が原点）
  result.pose.landmarks[0]  = {0.0f + sway, -0.8f, 0.0f};    // NOSE
  result.pose.landmarks[11] = {-0.2f + sway, -0.5f, 0.0f};   // LEFT_SHOULDER
  result.pose.landmarks[12] = {0.2f + sway, -0.5f, 0.0f};    // RIGHT_SHOULDER
  result.pose.landmarks[13] = {-0.35f + sway, -0.2f, 0.05f}; // LEFT_ELBOW
  result.pose.landmarks[14] = {0.35f + sway, -0.2f, 0.05f};  // RIGHT_ELBOW
  result.pose.landmarks[15] = {-0.4f, 0.0f, 0.1f};           // LEFT_WRIST
  result.pose.landmarks[16] = {0.4f, 0.0f, 0.1f};            // RIGHT_WRIST
  result.pose.landmarks[23] = {-0.12f, 0.1f, 0.0f};          // LEFT_HIP
  result.pose.landmarks[24] = {0.12f, 0.1f, 0.0f};           // RIGHT_HIP
  result.pose.landmarks[25] = {-0.15f, 0.5f, 0.0f};          // LEFT_KNEE
  result.pose.landmarks[26] = {0.15f, 0.5f, 0.0f};           // RIGHT_KNEE

  // 信頼度の設定
  for (int i = 0; i < MEDIA_PIPE_LANDMARKS; ++i) {
    result.pose.visibility[i] = 0.95f;
  }
  result.pose.confidence = 0.98f;

  // スムージング適用（前フレームとの補間でジッターを軽減）
  if (pImpl->hasPrevFrame) {
    for (int i = 0; i < MEDIA_PIPE_LANDMARKS; ++i) {
      result.pose.landmarks[i] = pImpl->smoothLandmark(
          result.pose.landmarks[i], pImpl->prevLandmarks[i]);
    }
  }
  pImpl->prevLandmarks = result.pose.landmarks;
  pImpl->hasPrevFrame = true;

  // SMPLパラメータの推定
  result.smplParams = fitSMPL(result.pose);

  // ボディメッシュの生成
  result.bodyMesh = getSMPLMesh(result.smplParams);

  auto endTime = std::chrono::steady_clock::now();
  result.processingTimeMs =
      std::chrono::duration<float, std::milli>(endTime - startTime).count();

  return {.value = result, .error = ErrorCode::SUCCESS};
}

BodyPose BodyTracker::estimate3DPose(const std::array<Point2D, 33> &landmarks2D,
                                     const Point2D &frameSize) {
  BodyPose pose;
  for (int i = 0; i < 33; ++i) {
    // 2D→3D変換（簡易的にZ=0とする。実際にはDepth推定ネットワークを使用）
    pose.landmarks[i] = {
        (landmarks2D[i].x / frameSize.x) * 2.0f - 1.0f,
        (landmarks2D[i].y / frameSize.y) * 2.0f - 1.0f,
        0.0f
    };
    pose.visibility[i] = 1.0f;
  }
  return pose;
}

SMPLParams BodyTracker::fitSMPL(const BodyPose &pose) {
  SMPLParams params;
  params.pose.fill(0.0f);
  params.shape.fill(0.0f);
  params.translation = {0.0f, 0.0f, 0.0f};

  // 腰の中央を基準にトランスレーション推定
  Point3D hipCenter = (pose.getLandmark(BodyLandmark::LEFT_HIP) +
                       pose.getLandmark(BodyLandmark::RIGHT_HIP)) * 0.5f;
  params.translation = hipCenter;

  // 肩の中央を計算してスケール推定
  Point3D shoulderCenter = (pose.getLandmark(BodyLandmark::LEFT_SHOULDER) +
                            pose.getLandmark(BodyLandmark::RIGHT_SHOULDER)) * 0.5f;
  float torsoLength = pImpl->distance(shoulderCenter, hipCenter);
  float standardTorso = 0.6f;
  params.scale = torsoLength / standardTorso;
  if (params.scale < 0.01f) params.scale = 1.0f;

  return params;
}

std::vector<Point3D> BodyTracker::getSMPLMesh(const SMPLParams &params) {
  std::vector<Point3D> mesh = pImpl->smplTemplate;
  for (auto &v : mesh) {
    v = v * params.scale + params.translation;
  }
  return mesh;
}

bool BodyTracker::isInitialized() const { return pImpl->initialized; }
void BodyTracker::reset() { 
  pImpl->initialized = false; 
  pImpl->hasPrevFrame = false;
}

} // namespace arfit
