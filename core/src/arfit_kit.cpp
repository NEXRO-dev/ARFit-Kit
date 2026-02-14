/**
 * @file arfit_kit.cpp
 * @brief ARFit-kit SDK メインの実装
 */

#include "arfit_kit.h"
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <algorithm>

namespace arfit {

/**
 * @brief ARFitKitの内部実装クラス (Pimpl)
 */
class ARFitKit::Impl {
public:
  SessionConfig config;
  bool sessionActive = false;

  // 各モジュールの管理
  std::unique_ptr<BodyTracker> bodyTracker;
  std::unique_ptr<GarmentConverter> garmentConverter;
  std::unique_ptr<PhysicsEngine> physicsEngine;
  std::unique_ptr<ARRenderer> renderer;

  // 読み込まれた衣服の管理 (ID -> 衣服オブジェクト)
  std::unordered_map<std::string, std::shared_ptr<Garment>> garmentRegistry;
  
  // 現在試着中の衣服リスト
  std::vector<std::shared_ptr<Garment>> activeGarments;

  // コールバック
  FrameCallback frameCallback;
  PoseCallback poseCallback;
  ErrorCallback errorCallback;

  // パフォーマンス測定用
  std::chrono::steady_clock::time_point lastFrameTime;
  float currentFPS = 0.0f;
  float averageLatency = 0.0f;
  int frameCount = 0;
  float totalLatency = 0.0f;

  std::mutex mutex;

  Impl() {
    bodyTracker = std::make_unique<BodyTracker>();
    garmentConverter = std::make_unique<GarmentConverter>();
    physicsEngine = std::make_unique<PhysicsEngine>();
    renderer = std::make_unique<ARRenderer>();
  }

  // 衣服IDを取得するヘルパー (単純なハッシュやUUIDなど)
  std::string generateId() {
    return std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  }
};

ARFitKit::ARFitKit() : pImpl(std::make_unique<Impl>()) {}

ARFitKit::~ARFitKit() { stopSession(); }

ARFitKit::ARFitKit(ARFitKit &&) noexcept = default;
ARFitKit &ARFitKit::operator=(ARFitKit &&) noexcept = default;

/**
 * SDKを初期化
 */
Result<void> ARFitKit::initialize(const SessionConfig &config) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  pImpl->config = config;

  // ボディトラッカーの初期化
  BodyTrackerConfig trackerConfig;
  auto trackerResult = pImpl->bodyTracker->initialize(trackerConfig);
  if (!trackerResult) {
    return {.error = trackerResult.error,
            .message = "ボディトラッカーの初期化に失敗しました"};
  }

  // 衣服コンバーターの初期化
  GarmentConverterConfig converterConfig;
  converterConfig.useServerProcessing = config.useHybridProcessing;
  converterConfig.serverEndpoint = config.serverEndpoint;
  auto converterResult = pImpl->garmentConverter->initialize(converterConfig);
  if (!converterResult) {
    return {.error = converterResult.error,
            .message = "衣服コンバーターの初期化に失敗しました"};
  }

  // 物理エンジンの初期化
  PhysicsConfig physicsConfig;
  auto physicsResult = pImpl->physicsEngine->initialize(physicsConfig);
  if (!physicsResult) {
    return {.error = physicsResult.error,
            .message = "物理エンジンの初期化に失敗しました"};
  }

  // レンダラーの初期化
  RenderConfig renderConfig;
  renderConfig.enableShadows = config.enableShadows;
  auto renderResult = pImpl->renderer->initialize(renderConfig);
  if (!renderResult) {
    return {.error = renderResult.error,
            .message = "レンダラーの初期化に失敗しました"};
  }

  return {.error = ErrorCode::SUCCESS};
}

/**
 * セッション開始
 */
Result<void> ARFitKit::startSession() {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  if (pImpl->sessionActive) {
    return {.error = ErrorCode::SUCCESS};
  }

  pImpl->sessionActive = true;
  pImpl->lastFrameTime = std::chrono::steady_clock::now();
  pImpl->frameCount = 0;
  pImpl->totalLatency = 0.0f;

  return {.error = ErrorCode::SUCCESS};
}

/**
 * セッション停止
 */
void ARFitKit::stopSession() {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  pImpl->sessionActive = false;
  pImpl->activeGarments.clear();
  pImpl->physicsEngine->reset();
}

bool ARFitKit::isSessionActive() const { return pImpl->sessionActive; }

/**
 * カメラフレーム処理
 */
Result<ImageData> ARFitKit::processFrame(const CameraFrame &frame) {
  auto startTime = std::chrono::steady_clock::now();

  if (!pImpl->sessionActive) {
    return {.error = ErrorCode::SESSION_NOT_STARTED,
            .message = "セッションが開始されていません"};
  }

  // 1. ボディトラッキング (ポーズ推定)
  auto trackingResult = pImpl->bodyTracker->processFrame(frame);
  if (trackingResult.hasError()) {
    if (pImpl->errorCallback) {
      pImpl->errorCallback(trackingResult.error, trackingResult.message);
    }
  } else {
    const auto &pose = trackingResult.value.pose;

    // コールバック通知
    if (pImpl->poseCallback) {
      pImpl->poseCallback(pose);
    }

    // 物理エンジン用の衝突判定ボディーを更新
    CollisionBody collisionBody;
    collisionBody.vertices = trackingResult.value.bodyMesh;
    pImpl->physicsEngine->updateCollisionBody(collisionBody);
  }

  // 2. 物理シミュレーション (布の動き)
  float deltaTime = 1.0f / pImpl->config.targetFPS;
  auto physicsResult = pImpl->physicsEngine->step(deltaTime);

  // 3. 衣服メッシュの更新
  for (auto &garment : pImpl->activeGarments) {
    auto positions = pImpl->physicsEngine->getParticlePositions(garment);
    pImpl->renderer->updateGarmentMesh(garment, positions);
  }

  // 4. 背景(カメラ映像)の設定
  pImpl->renderer->setCameraFrame(frame);

  // 5. レンダリング (合成)
  auto renderResult = pImpl->renderer->render();

  // 指標の計算
  auto endTime = std::chrono::steady_clock::now();
  float latencyMs =
      std::chrono::duration<float, std::milli>(endTime - startTime).count();
  pImpl->totalLatency += latencyMs;
  pImpl->frameCount++;
  pImpl->averageLatency = pImpl->totalLatency / pImpl->frameCount;

  auto frameDuration =
      std::chrono::duration<float>(endTime - pImpl->lastFrameTime);
  pImpl->currentFPS = 1.0f / frameDuration.count();
  pImpl->lastFrameTime = endTime;

  // フレームコールバック通知
  if (pImpl->frameCallback && renderResult.isSuccess()) {
    pImpl->frameCallback(renderResult.value);
  }

  return renderResult;
}

/**
 * 衣服を読み込む
 */
Result<std::string> ARFitKit::loadGarment(const ImageData &image,
                                                        GarmentType type) {
  auto result = pImpl->garmentConverter->convert(image, type);
  if (result.isSuccess()) {
    const std::string id = pImpl->generateId();
    pImpl->garmentRegistry[id] = result.value;
    return {.value = id, .error = ErrorCode::SUCCESS};
  }
  return {.error = result.error, .message = result.message};
}

/**
 * URLから衣服を読み込む
 */
Result<std::string>
ARFitKit::loadGarmentFromUrl(const std::string &url) {
  auto result = pImpl->garmentConverter->convertFromServer(url);
  if (result.isSuccess()) {
    const std::string id = pImpl->generateId();
    pImpl->garmentRegistry[id] = result.value;
    return {.value = id, .error = ErrorCode::SUCCESS};
  }
  return {.error = result.error, .message = result.message};
}

/**
 * 衣服を試着する
 */
Result<void> ARFitKit::tryOn(const std::string& garmentId) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  if (!pImpl->sessionActive) {
    return {.error = ErrorCode::SESSION_NOT_STARTED, .message = "セッションが開始されていません"};
  }

  // レジストリから検索
  auto it = pImpl->garmentRegistry.find(garmentId);
  if (it == pImpl->garmentRegistry.end()) {
    return {.error = ErrorCode::INVALID_IMAGE, .message = "指定された衣服IDが見つかりません"};
  }

  auto garment = it->second;

  // 最大衣服数のチェック
  if (pImpl->activeGarments.size() >=
      static_cast<size_t>(pImpl->config.maxGarments)) {
    // 最も古い衣服を脱ぐ
    auto oldest = pImpl->activeGarments.front();
    pImpl->activeGarments.erase(pImpl->activeGarments.begin());
    pImpl->physicsEngine->removeGarment(oldest);
    pImpl->renderer->removeGarment(oldest);
  }

  // 布シミュレーションのセットアップ
  auto setupResult = pImpl->garmentConverter->setupClothSimulation(garment);
  if (!setupResult) {
    return setupResult;
  }

  // 物理エンジンに追加
  auto physicsResult = pImpl->physicsEngine->addGarment(garment);
  if (!physicsResult) {
    return physicsResult;
  }

  // レンダラーに追加
  auto positions = pImpl->physicsEngine->getParticlePositions(garment);
  pImpl->renderer->addGarment(garment, positions);

  pImpl->activeGarments.push_back(garment);

  return {.error = ErrorCode::SUCCESS};
}

/**
 * 試着中の衣服を脱ぐ
 */
void ARFitKit::removeGarment(const std::string& garmentId) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  auto it_reg = pImpl->garmentRegistry.find(garmentId);
  if (it_reg == pImpl->garmentRegistry.end()) return;
  auto garment = it_reg->second;

  auto it = std::find(pImpl->activeGarments.begin(),
                      pImpl->activeGarments.end(), garment);
  if (it != pImpl->activeGarments.end()) {
    pImpl->physicsEngine->removeGarment(garment);
    pImpl->renderer->removeGarment(garment);
    pImpl->activeGarments.erase(it);
  }
}

/**
 * すべての衣服を脱ぐ
 */
void ARFitKit::removeAllGarments() {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  for (auto &garment : pImpl->activeGarments) {
    pImpl->physicsEngine->removeGarment(garment);
    pImpl->renderer->removeGarment(garment);
  }
  pImpl->activeGarments.clear();
}

/**
 * スクリーンショット撮影
 */
Result<ImageData> ARFitKit::captureSnapshot() {
  return pImpl->renderer->render();
}

void ARFitKit::setFrameCallback(FrameCallback callback) {
  pImpl->frameCallback = std::move(callback);
}

void ARFitKit::setPoseCallback(PoseCallback callback) {
  pImpl->poseCallback = std::move(callback);
}

void ARFitKit::setErrorCallback(ErrorCallback callback) {
  pImpl->errorCallback = std::move(callback);
}

// コンポーネントへのアクセサ
BodyTracker &ARFitKit::getBodyTracker() { return *pImpl->bodyTracker; }

GarmentConverter &ARFitKit::getGarmentConverter() {
  return *pImpl->garmentConverter;
}

PhysicsEngine &ARFitKit::getPhysicsEngine() { return *pImpl->physicsEngine; }

ARRenderer &ARFitKit::getARRenderer() { return *pImpl->renderer; }

float ARFitKit::getCurrentFPS() const { return pImpl->currentFPS; }

float ARFitKit::getAverageLatency() const { return pImpl->averageLatency; }

} // namespace arfit

