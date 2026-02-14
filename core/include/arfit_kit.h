/**
 * @file arfit_kit.h
 * @brief ARFit-kit SDK メインヘッダー
 * 
 * このファイルは、AR試着機能を利用するための主要なインターフェースを定義します。
 */

#pragma once

#include "ar_renderer.h"
#include "body_tracker.h"
#include "garment_converter.h"
#include "physics_engine.h"
#include "types.h"

#include <functional>
#include <memory>
#include <string>

namespace arfit {

/**
 * @brief フレーム処理完了時のコールバック
 */
using FrameCallback = std::function<void(const ImageData &frame)>;

/**
 * @brief ボディトラッキング更新時のコールバック
 */
using PoseCallback = std::function<void(const BodyPose &pose)>;

/**
 * @brief エラー発生時のコールバック
 */
using ErrorCallback =
    std::function<void(ErrorCode code, const std::string &message)>;

/**
 * @brief ARFitKit SDK メインクラス
 *
 * AR試着機能を提供するプライマリインターフェースです。
 * 内部でボディー推定、衣服変換、物理シミュレーション、描画を管理します。
 */
class ARFitKit {
public:
  ARFitKit();
  ~ARFitKit();

  // コピー禁止
  ARFitKit(const ARFitKit &) = delete;
  ARFitKit &operator=(const ARFitKit &) = delete;

  // ムーブ許可
  ARFitKit(ARFitKit &&) noexcept;
  ARFitKit &operator=(ARFitKit &&) noexcept;

  /**
   * @brief SDKの初期化
   * @param config セッション設定
   * @return 初期化結果
   */
  Result<void> initialize(const SessionConfig &config = SessionConfig{});

  /**
   * @brief ARセッションの開始
   * @return 実行結果
   */
  Result<void> startSession();

  /**
   * @brief ARセッションの停止
   */
  void stopSession();

  /**
   * @brief セッションがアクティブかどうかを確認
   */
  bool isSessionActive() const;

  /**
   * @brief カメラフレームの処理
   * @param frame 入力カメラフレーム
   * @return AR合成済みの画像データ
   */
  Result<ImageData> processFrame(const CameraFrame &frame);

  /**
   * @brief 画像データから衣服を読み込む
   * @param image 衣服の画像データ
   * @param type 衣服の種類（指定しない場合は自動判定）
   * @return 読み込まれた衣服のID
   */
  Result<std::string>
  loadGarment(const ImageData &image, GarmentType type = GarmentType::UNKNOWN);

  /**
   * @brief URLから衣服を読み込む（ハイブリッドサーバー処理用）
   * @param url 衣服画像のURL
   * @return 読み込まれた衣服のID
   */
  Result<std::string> loadGarmentFromUrl(const std::string &url);

  /**
   * @brief 衣服を試着する
   * @param garmentId 試着する衣服のID
   * @return 実行結果
   */
  Result<void> tryOn(const std::string& garmentId);

  /**
   * @brief 指定した衣服を脱ぐ
   * @param garmentId 削除する衣服のID
   */
  void removeGarment(const std::string& garmentId);

  /**
   * @brief すべての衣服を脱ぐ
   */
  void removeAllGarments();

  /**
   * @brief 現在の表示のスクリーンショットを撮影
   * @return 撮影された画像データ
   */
  Result<ImageData> captureSnapshot();

  // コールバック設定
  void setFrameCallback(FrameCallback callback);
  void setPoseCallback(PoseCallback callback);
  void setErrorCallback(ErrorCallback callback);

  // コンポーネント取得（高度なカスタマイズ用）
  BodyTracker &getBodyTracker();
  GarmentConverter &getGarmentConverter();
  PhysicsEngine &getPhysicsEngine();
  ARRenderer &getARRenderer();

  // パフォーマンスメトリクス
  float getCurrentFPS() const;
  float getAverageLatency() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit

