/**
 * @file arfit_kit.cpp
 * @brief Main ARfit-kit SDK implementation
 */

#include "arfit_kit.h"
#include <chrono>
#include <mutex>

namespace arfit {

class ARFitKit::Impl {
public:
  SessionConfig config;
  bool sessionActive = false;

  std::unique_ptr<BodyTracker> bodyTracker;
  std::unique_ptr<GarmentConverter> garmentConverter;
  std::unique_ptr<PhysicsEngine> physicsEngine;
  std::unique_ptr<ARRenderer> renderer;

  std::vector<std::shared_ptr<Garment>> activeGarments;

  FrameCallback frameCallback;
  PoseCallback poseCallback;
  ErrorCallback errorCallback;

  // Performance metrics
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
};

ARFitKit::ARFitKit() : pImpl(std::make_unique<Impl>()) {}

ARFitKit::~ARFitKit() { stopSession(); }

ARFitKit::ARFitKit(ARFitKit &&) noexcept = default;
ARFitKit &ARFitKit::operator=(ARFitKit &&) noexcept = default;

Result<void> ARFitKit::initialize(const SessionConfig &config) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  pImpl->config = config;

  // Initialize body tracker
  BodyTrackerConfig trackerConfig;
  auto trackerResult = pImpl->bodyTracker->initialize(trackerConfig);
  if (!trackerResult) {
    return {.error = trackerResult.error,
            .message = "Failed to initialize body tracker"};
  }

  // Initialize garment converter
  GarmentConverterConfig converterConfig;
  converterConfig.useServerProcessing = config.useHybridProcessing;
  converterConfig.serverEndpoint = config.serverEndpoint;
  auto converterResult = pImpl->garmentConverter->initialize(converterConfig);
  if (!converterResult) {
    return {.error = converterResult.error,
            .message = "Failed to initialize garment converter"};
  }

  // Initialize physics engine
  PhysicsConfig physicsConfig;
  auto physicsResult = pImpl->physicsEngine->initialize(physicsConfig);
  if (!physicsResult) {
    return {.error = physicsResult.error,
            .message = "Failed to initialize physics engine"};
  }

  // Initialize renderer
  RenderConfig renderConfig;
  renderConfig.enableShadows = config.enableShadows;
  auto renderResult = pImpl->renderer->initialize(renderConfig);
  if (!renderResult) {
    return {.error = renderResult.error,
            .message = "Failed to initialize renderer"};
  }

  return {.error = ErrorCode::SUCCESS};
}

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

void ARFitKit::stopSession() {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  pImpl->sessionActive = false;
  pImpl->activeGarments.clear();
  pImpl->physicsEngine->reset();
}

bool ARFitKit::isSessionActive() const { return pImpl->sessionActive; }

Result<ImageData> ARFitKit::processFrame(const CameraFrame &frame) {
  auto startTime = std::chrono::steady_clock::now();

  if (!pImpl->sessionActive) {
    return {.error = ErrorCode::SESSION_NOT_STARTED,
            .message = "Session not started"};
  }

  // 1. Body tracking
  auto trackingResult = pImpl->bodyTracker->processFrame(frame);
  if (trackingResult.hasError()) {
    if (pImpl->errorCallback) {
      pImpl->errorCallback(trackingResult.error, trackingResult.message);
    }
  } else {
    const auto &pose = trackingResult.value.pose;

    // Notify pose callback
    if (pImpl->poseCallback) {
      pImpl->poseCallback(pose);
    }

    // Update collision body for physics
    // Convert SMPL mesh (vector<Point3D>) to simple collision vertices
    CollisionBody collisionBody;
    collisionBody.vertices = trackingResult.value.bodyMesh;
    pImpl->physicsEngine->updateCollisionBody(collisionBody);
  }

  // 2. Physics simulation
  float deltaTime = 1.0f / pImpl->config.targetFPS;
  auto physicsResult = pImpl->physicsEngine->step(deltaTime);

  // 3. Update garment meshes
  for (auto &garment : pImpl->activeGarments) {
    auto positions = pImpl->physicsEngine->getParticlePositions(garment);
    pImpl->renderer->updateGarmentMesh(garment, positions);
  }

  // 4. Set camera frame background
  pImpl->renderer->setCameraFrame(frame);

  // 5. Render
  auto renderResult = pImpl->renderer->render();

  // Update FPS metrics
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

  // Notify frame callback
  if (pImpl->frameCallback && renderResult.isSuccess()) {
    pImpl->frameCallback(renderResult.value);
  }

  return renderResult;
}

Result<std::shared_ptr<Garment>> ARFitKit::loadGarment(const ImageData &image,
                                                       GarmentType type) {
  return pImpl->garmentConverter->convert(image, type);
}

Result<std::shared_ptr<Garment>>
ARFitKit::loadGarmentFromUrl(const std::string &url) {
  return pImpl->garmentConverter->convertFromServer(url);
}

Result<void> ARFitKit::tryOn(std::shared_ptr<Garment> garment) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  if (!garment) {
    return {.error = ErrorCode::INVALID_IMAGE, .message = "Garment is null"};
  }

  // Check max garments
  if (pImpl->activeGarments.size() >=
      static_cast<size_t>(pImpl->config.maxGarments)) {
    // Remove oldest garment
    auto oldest = pImpl->activeGarments.front();
    pImpl->activeGarments.erase(pImpl->activeGarments.begin());
    pImpl->physicsEngine->removeGarment(oldest);
    pImpl->renderer->removeGarment(oldest);
  }

  // Setup cloth simulation
  auto setupResult = pImpl->garmentConverter->setupClothSimulation(garment);
  if (!setupResult) {
    return setupResult;
  }

  // Add to physics
  auto physicsResult = pImpl->physicsEngine->addGarment(garment);
  if (!physicsResult) {
    return physicsResult;
  }

  // Add to renderer
  auto positions = pImpl->physicsEngine->getParticlePositions(garment);
  pImpl->renderer->addGarment(garment, positions);

  pImpl->activeGarments.push_back(garment);

  return {.error = ErrorCode::SUCCESS};
}

void ARFitKit::removeGarment(std::shared_ptr<Garment> garment) {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  auto it = std::find(pImpl->activeGarments.begin(),
                      pImpl->activeGarments.end(), garment);
  if (it != pImpl->activeGarments.end()) {
    pImpl->physicsEngine->removeGarment(garment);
    pImpl->renderer->removeGarment(garment);
    pImpl->activeGarments.erase(it);
  }
}

void ARFitKit::removeAllGarments() {
  std::lock_guard<std::mutex> lock(pImpl->mutex);

  for (auto &garment : pImpl->activeGarments) {
    pImpl->physicsEngine->removeGarment(garment);
    pImpl->renderer->removeGarment(garment);
  }
  pImpl->activeGarments.clear();
}

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

BodyTracker &ARFitKit::getBodyTracker() { return *pImpl->bodyTracker; }

GarmentConverter &ARFitKit::getGarmentConverter() {
  return *pImpl->garmentConverter;
}

PhysicsEngine &ARFitKit::getPhysicsEngine() { return *pImpl->physicsEngine; }

ARRenderer &ARFitKit::getARRenderer() { return *pImpl->renderer; }

float ARFitKit::getCurrentFPS() const { return pImpl->currentFPS; }

float ARFitKit::getAverageLatency() const { return pImpl->averageLatency; }

} // namespace arfit
